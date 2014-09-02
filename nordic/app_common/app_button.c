/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "app_button.h"
#include <string.h>
#include "nordic_common.h"
#include "app_util.h"
#include "app_gpiote.h"
#include "app_timer.h"
#include "app_error.h"


static app_button_cfg_t *             mp_buttons = NULL;           /**< Button configuration. */
static uint8_t                        m_button_count;              /**< Number of configured buttons. */
static uint32_t                       m_detection_delay;           /**< Delay before a button is reported as pushed. */
static app_button_evt_schedule_func_t m_evt_schedule_func;         /**< Pointer to function for propagating button events to the scheduler. */
static app_gpiote_user_id_t           m_gpiote_user_id;            /**< GPIOTE user id for buttons module. */
static app_timer_id_t                 m_detection_delay_timer_id;  /**< Polling timer id. */
static pin_transition_t               m_pin_transition;            /**< pin transaction direction. */


/**@brief Function for executing the application button handler for specified button.
 *
 * @param[in]  p_btn   Button that has been pushed.
 */
static void button_handler_execute(app_button_cfg_t * p_btn, uint32_t transition)
{
    if (m_evt_schedule_func != NULL)
    {
        uint32_t err_code = m_evt_schedule_func(p_btn->button_handler, p_btn->pin_no,transition);
        APP_ERROR_CHECK(err_code);
    }
    else
    {
        if(transition == APP_BUTTON_PUSH)
        {
            p_btn->button_handler(p_btn->pin_no, APP_BUTTON_PUSH);
        }
        else if(transition == APP_BUTTON_RELEASE)
        {
            p_btn->button_handler(p_btn->pin_no, APP_BUTTON_RELEASE);
        }
    }
}


/**@brief Function for handling the timeout that delays reporting buttons as pushed.
 *
 * @details    The detection_delay_timeout_handler(...) is a call-back issued from the app_timer
 *             module. It is called with the p_context parameter. The p_context parameter is
 *             provided to the app_timer module when a timer is started, using the call
 *             @ref app_timer_start. On @ref app_timer_start the p_context will be holding the
 *             currently pressed buttons.
 *
 * @param[in]  p_context   Pointer used for passing information app_start_timer() was called.
 *                         In the app_button module the p_context holds information on pressed
 *                         buttons.
 */
static void detection_delay_timeout_handler(void * p_context)
{
    uint32_t err_code;
    uint32_t current_state_pins;

    // Get current state of pins.
    err_code = app_gpiote_pins_state_get(m_gpiote_user_id, &current_state_pins);

    if (err_code != NRF_SUCCESS)
    {
        return;
    }

    uint8_t i;
    
    // Pushed button(s) detected, execute button handler(s).
    for (i = 0; i < m_button_count; i++)
    {
        app_button_cfg_t * p_btn = &mp_buttons[i];

        if (((m_pin_transition.high_to_low & (1 << p_btn->pin_no)) != 0) && (p_btn->button_handler != NULL))
        {
            //If it's active high then going from high to low was a release of the button.
            if(p_btn->active_state == APP_BUTTON_ACTIVE_HIGH)
            {
                button_handler_execute(p_btn, APP_BUTTON_RELEASE);
            }
            //If it's active low then going from high to low was a push of the button.
            else
            {
                button_handler_execute(p_btn, APP_BUTTON_PUSH);
            }
        }
        else if (((m_pin_transition.low_to_high & (1 << p_btn->pin_no)) != 0) && (p_btn->button_handler != NULL))
        {
            //If it's active high then going from low to high was a push of the button.
            if(p_btn->active_state == APP_BUTTON_ACTIVE_HIGH)
            {
                button_handler_execute(p_btn,APP_BUTTON_PUSH);
            }
            //If it's active low then going from low to high was a release of the button.
            else
            {
                button_handler_execute(p_btn,APP_BUTTON_RELEASE);
            }
        }
    }
}


/**@brief Function for handling the GPIOTE event.
 *
 * @details Saves the current status of the button pins, and starts a timer. If the timer is already
 *          running, it will be restarted.
 *
 * @param[in]  event_pins_low_to_high   Mask telling which pin(s) had a low to high transition.
 * @param[in]  event_pins_high_to_low   Mask telling which pin(s) had a high to low transition.
 */
static void gpiote_event_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{
    uint32_t err_code;
 
    // Start detection timer. If timer is already running, the detection period is restarted.
    // NOTE: Using the p_context parameter of app_timer_start() to transfer the pin states to the
    //       timeout handler (by casting event_pins_mask into the equally sized void * p_context
    //       parameter).
    STATIC_ASSERT(sizeof(void *) == sizeof(uint32_t));

    err_code = app_timer_stop(m_detection_delay_timer_id);
    if (err_code != NRF_SUCCESS)
    {
        // The impact in app_button of the app_timer queue running full is losing a button press.
        // The current implementation ensures that the system will continue working as normal. 
        return;
    }

    m_pin_transition.low_to_high = event_pins_low_to_high;
    m_pin_transition.high_to_low = event_pins_high_to_low;
    
    err_code = app_timer_start(m_detection_delay_timer_id,
                               m_detection_delay,
                               (void *)(event_pins_low_to_high | event_pins_high_to_low));
    if (err_code != NRF_SUCCESS)
    {
        // The impact in app_button of the app_timer queue running full is losing a button press.
        // The current implementation ensures that the system will continue working as normal. 
    }
}


uint32_t app_button_init(app_button_cfg_t *             p_buttons,
                         uint8_t                        button_count,
                         uint32_t                       detection_delay,
                         app_button_evt_schedule_func_t evt_schedule_func)
{
    uint32_t err_code;
    
    if (detection_delay < APP_TIMER_MIN_TIMEOUT_TICKS)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    // Save configuration.
    mp_buttons          = p_buttons;
    m_button_count      = button_count;
    m_detection_delay   = detection_delay;
    m_evt_schedule_func = evt_schedule_func;
  
    // Configure pins.
    uint32_t pins_transition_mask = 0;
    
    while (button_count--)
    {
        app_button_cfg_t * p_btn = &p_buttons[button_count];

        // Configure pin.
        nrf_gpio_cfg_input(p_btn->pin_no, p_btn->pull_cfg);
        
        // Build GPIOTE user registration masks.
        pins_transition_mask |= (1 << p_btn->pin_no);
    }
    
    // Register button module as a GPIOTE user.
    err_code = app_gpiote_user_register(&m_gpiote_user_id,
                                        pins_transition_mask,
                                        pins_transition_mask,
                                        gpiote_event_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Create polling timer.
    return app_timer_create(&m_detection_delay_timer_id,
                            APP_TIMER_MODE_SINGLE_SHOT,
                            detection_delay_timeout_handler);
}


uint32_t app_button_enable(void)
{
    if (mp_buttons == NULL)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return app_gpiote_user_enable(m_gpiote_user_id);
}


uint32_t app_button_disable(void)
{
    uint32_t err_code;
    
    if (mp_buttons == NULL)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    err_code = app_gpiote_user_disable(m_gpiote_user_id);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Make sure polling timer is not running.
    return app_timer_stop(m_detection_delay_timer_id);
}


uint32_t app_button_is_pushed(uint8_t pin_no, bool * p_is_pushed)
{
    uint32_t err_code;
    uint32_t active_pins;
    
    app_button_cfg_t * p_btn = &mp_buttons[pin_no];

    if (mp_buttons == NULL)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    err_code = app_gpiote_pins_state_get(m_gpiote_user_id, &active_pins);
    
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if(p_btn->active_state == APP_BUTTON_ACTIVE_LOW)
    {
        // If the pin is active low, then the pin being high means it is not pushed.
        if(((active_pins >> pin_no) & 0x01))
        {
            *p_is_pushed = false;
        }
        else
        {
            *p_is_pushed = true;  
        }            
    }
    else if(p_btn->active_state == APP_BUTTON_ACTIVE_HIGH)
    {
        // If the pin is active high, then the pin being high means it is pushed.
        if(((active_pins >> pin_no) & 0x01))
        {
            *p_is_pushed = true;
        }
        else
        {
            *p_is_pushed = false;   
        }
    }
    
    return NRF_SUCCESS;
}
