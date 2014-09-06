/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common/common.h"
#include "nordic_common.h"

#include "app_timer.h"
#include "btle.h"

#include "ble_stack_handler_types.h"
#include "ble_radio_notification.h"
#include "ble_flash.h"
#if NEED_BOND_MANAGER
#include "ble_bondmngr.h"
#endif
#include "ble_conn_params.h"

#include "btle_gap.h"
#include "btle_advertising.h"
#include "custom/custom_helper.h"

#include "softdevice_handler.h"
#include "pstorage.h"

#include "GapEvents.h"
#include "nRF51Gap.h"
#include "nRF51GattServer.h"

#include "ble_hci.h"

#if NEED_BOND_MANAGER /* disabled by default */
static void service_error_callback(uint32_t nrf_error);
#endif
extern "C" void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name);
void            app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name);

#if NEED_BOND_MANAGER /* disabled by default */
static error_t bond_manager_init(void);
#endif

static void btle_handler(ble_evt_t *p_ble_evt);

static void sys_evt_dispatch(uint32_t sys_evt)
{
#if NEED_PSTORAGE /* disabled by default */
    pstorage_sys_event_handler(sys_evt);
#endif
}

error_t btle_init(void)
{
    const bool useScheduler = false;
#ifdef TARGET_HRM1017
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_4000MS_CALIBRATION, false);
#else
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, useScheduler);
#endif

    // Enable BLE stack
    /**
     * Using this call, the application can select whether to include the
     * Service Changed characteristic in the GATT Server. The default in all
     * previous releases has been to include the Service Changed characteristic,
     * but this affects how GATT clients behave. Specifically, it requires
     * clients to subscribe to this attribute and not to cache attribute handles
     * between connections unless the devices are bonded. If the application
     * does not need to change the structure of the GATT server attributes at
     * runtime this adds unnecessary complexity to the interaction with peer
     * clients. If the SoftDevice is enabled with the Service Changed
     * Characteristics turned off, then clients are allowed to cache attribute
     * handles making applications simpler on both sides.
     */
    static const bool IS_SRVC_CHANGED_CHARACT_PRESENT = false;
    ble_enable_params_t enableParams = {
        .gatts_enable_params = {
            .service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT
        }
    };
    if (sd_ble_enable(&enableParams) != NRF_SUCCESS) {
        return ERROR_INVALID_PARAM;
    }

    ble_gap_addr_t addr;
    if (sd_ble_gap_address_get(&addr) != NRF_SUCCESS) {
        return ERROR_INVALID_PARAM;
    }
    if (sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE, &addr) != NRF_SUCCESS) {
        return ERROR_INVALID_PARAM;
    }

    ASSERT_STATUS( softdevice_ble_evt_handler_set(btle_handler));
    ASSERT_STATUS( softdevice_sys_evt_handler_set(sys_evt_dispatch));

#if NEED_BOND_MANAGER /* disabled by default */
    bond_manager_init();
#endif
    btle_gap_init();

    return ERROR_NONE;
}

static void btle_handler(ble_evt_t *p_ble_evt)
{
    /* Library service handlers */
#if NEED_BOND_MANAGER /* disabled by default */
    ble_bondmngr_on_ble_evt(p_ble_evt);
#endif
#if SDK_CONN_PARAMS_MODULE_ENABLE
    ble_conn_params_on_ble_evt(p_ble_evt);
#endif

    /* Custom event handler */
    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED: {
            Gap::Handle_t handle = p_ble_evt->evt.gap_evt.conn_handle;
            nRF51Gap::getInstance().setConnectionHandle(handle);
            const Gap::ConnectionParams_t *params = reinterpret_cast<Gap::ConnectionParams_t *>(&(p_ble_evt->evt.gap_evt.params.connected.conn_params));
            nRF51Gap::getInstance().processConnectionEvent(handle, params);
            break;
        }

        case BLE_GAP_EVT_DISCONNECTED: {
            Gap::Handle_t handle = p_ble_evt->evt.gap_evt.conn_handle;
            // Since we are not in a connection and have not started advertising,
            // store bonds
            nRF51Gap::getInstance().setConnectionHandle (BLE_CONN_HANDLE_INVALID);
#if NEED_BOND_MANAGER /* disabled by default */
            ASSERT_STATUS_RET_VOID ( ble_bondmngr_bonded_centrals_store());
#endif

            if (p_ble_evt->evt.gap_evt.params.disconnected.reason == BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION) {
                nRF51Gap::getInstance().processDisconnectionEvent(handle, Gap::LOCAL_HOST_TERMINATED_CONNECTION);
            }
            break;
        }

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST: {
            ble_gap_sec_params_t sec_params = {0};

            sec_params.timeout      = 30; /*< Timeout for Pairing Request or
                                   * Security Request (in seconds). */
            sec_params.bond         = 1;  /**< Perform bonding. */
            sec_params.mitm         = CFG_BLE_SEC_PARAM_MITM;
            sec_params.io_caps      = CFG_BLE_SEC_PARAM_IO_CAPABILITIES;
            sec_params.oob          = CFG_BLE_SEC_PARAM_OOB;
            sec_params.min_key_size = CFG_BLE_SEC_PARAM_MIN_KEY_SIZE;
            sec_params.max_key_size = CFG_BLE_SEC_PARAM_MAX_KEY_SIZE;

            ASSERT_STATUS_RET_VOID(sd_ble_gap_sec_params_reply(nRF51Gap::getInstance().getConnectionHandle(),
                                                               BLE_GAP_SEC_STATUS_SUCCESS, &sec_params));
        }
        break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT) {
                nRF51Gap::getInstance().processEvent(GapEvents::GAP_EVENT_TIMEOUT);
            }
            break;

        case BLE_GATTC_EVT_TIMEOUT:
        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server and Client timeout events.
            // ASSERT_STATUS_RET_VOID (sd_ble_gap_disconnect(m_conn_handle,
            // BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
            break;

        default:
            break;
    }

    nRF51GattServer::getInstance().hwCallback(p_ble_evt);
}

#if NEED_BOND_MANAGER /* disabled by default */
/**************************************************************************/
/*!
    @brief      Initialises the bond manager

    @note       Bond data will be cleared on reset if the bond delete
                button is pressed during initialisation (the button is
                defined as CFG_BLE_BOND_DELETE_BUTTON_NUM).

    @returns
*/
/**************************************************************************/
static error_t bond_manager_init(void)
{
    ble_bondmngr_init_t bond_para = {0};

    ASSERT_STATUS ( pstorage_init());

    bond_para.flash_page_num_bond     = CFG_BLE_BOND_FLASH_PAGE_BOND;
    bond_para.flash_page_num_sys_attr = CFG_BLE_BOND_FLASH_PAGE_SYS_ATTR;
    //bond_para.bonds_delete            = boardButtonCheck(CFG_BLE_BOND_DELETE_BUTTON_NUM) ;
    bond_para.evt_handler   = NULL;
    bond_para.error_handler = service_error_callback;

    ASSERT_STATUS( ble_bondmngr_init( &bond_para ));

    /* Init radio active/inactive notification to flash (to only perform flashing when the radio is inactive) */
    //  ASSERT_STATUS( ble_radio_notification_init(NRF_APP_PRIORITY_HIGH,
    //                                             NRF_RADIO_NOTIFICATION_DISTANCE_4560US,
    //                                             ble_flash_on_radio_active_evt) );

    return ERROR_NONE;
}
#endif // #if NEED_BOND_MANAGER

#if NEED_BOND_MANAGER /* disabled by default */
/**************************************************************************/
/*!
    @brief
    @param[in]  nrf_error
    @returns
*/
/**************************************************************************/
static void service_error_callback(uint32_t nrf_error)
{
    ASSERT_STATUS_RET_VOID( nrf_error );
}
#endif // #if NEED_BOND_MANAGER

/**************************************************************************/
/*!
    @brief      Callback when an error occurs inside the SoftDevice

    @param[in]  line_num
    @param[in]  p-file_name

    @returns
*/
/**************************************************************************/
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    ASSERT(false, (void) 0);
}

/**************************************************************************/
/*!
    @brief      Handler for general errors above the SoftDevice layer.
                Typically we can' recover from this so we do a reset.

    @param[in]  error_code
    @param[in]  line_num
    @param[in]  p-file_name

    @returns
*/
/**************************************************************************/
void app_error_handler(uint32_t       error_code,
                       uint32_t       line_num,
                       const uint8_t *p_file_name)
{
    ASSERT_STATUS_RET_VOID( error_code );
    NVIC_SystemReset();
}
