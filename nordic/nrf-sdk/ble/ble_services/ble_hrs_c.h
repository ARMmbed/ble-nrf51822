/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/**@file
 *
 * @defgroup ble_sdk_srv_hrs_c   Heart Rate Service Client
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Heart Rate Service Client module.
 *
 * @details  This module contains the APIs and types exposed by the Heart Rate Service Client
 *           module. These APIs and types can be used by the application to perform discovery of
 *           Heart Rate Service at the peer and interact with it.
 *
 * @warning  Currently this module only has support for Heart Rate Measurement characteristic. This
 *           means that it will be able to enable notification of the characteristic at the peer and
 *           be able to receive Heart Rate Measurement notifications from the peer. It does not
 *           support the Body Sensor Location and the Heart Rate Control Point characteristics.
 *           When a Heart Rate Measurement is received, this module will decode only the
 *           Heart Rate Measurement Value (both 8 bit and 16 bit) field from it and provide it to
 *           the application.
 *
 * @note     The application must propagate BLE stack events to this module by calling
 *           ble_hrs_c_on_ble_evt().
 *
 */

#ifndef BLE_HRS_C_H__
#define BLE_HRS_C_H__

#include <stdint.h>
#include "ble.h"

/**
 * @defgroup hrs_c_enums Enumerations
 * @{
 */

/**@brief HRS Client event type. */
typedef enum
{
    BLE_HRS_C_EVT_DISCOVERY_COMPLETE = 1,  /**< Event indicating that the Heart Rate Service has been discovered at the peer. */
    BLE_HRS_C_EVT_HRM_NOTIFICATION         /**< Event indicating that a notification of the Heart Rate Measurement characteristic has been received from the peer. */
} ble_hrs_c_evt_type_t;

/** @} */

/**
 * @defgroup hrs_c_structs Structures
 * @{
 */

/**@brief Structure containing the heart rate measurement received from the peer. */
typedef struct
{
    uint16_t hr_value;  /**< Heart Rate Value. */
} ble_hrm_t;

/**@brief Heart Rate Event structure. */
typedef struct
{
    ble_hrs_c_evt_type_t evt_type;  /**< Type of the event. */
    union
    {
        ble_hrm_t hrm;  /**< Heart rate measurement received. This will be filled if the evt_type is @ref BLE_HRS_C_EVT_HRM_NOTIFICATION. */
    } params;
} ble_hrs_c_evt_t;

/** @} */

/**
 * @defgroup hrs_c_types Types
 * @{
 */

// Forward declaration of the ble_bas_t type.
typedef struct ble_hrs_c_s ble_hrs_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module in order to receive events.
 */
typedef void (* ble_hrs_c_evt_handler_t) (ble_hrs_c_t * p_ble_hrs_c, ble_hrs_c_evt_t * p_evt);

/** @} */

/**
 * @addtogroup hrs_c_structs
 * @{
 */

/**@brief Heart Rate Client structure.
 */
typedef struct ble_hrs_c_s
{
    uint16_t                conn_handle;      /**< Connection handle as provided by the SoftDevice. */
    uint16_t                hrm_cccd_handle;  /**< Handle of the CCCD of the Heart Rate Measurement characteristic. */
    uint16_t                hrm_handle;       /**< Handle of the Heart Rate Measurement characteristic as provided by the SoftDevice. */
    ble_hrs_c_evt_handler_t evt_handler;      /**< Application event handler to be called when there is an event related to the heart rate service. */
} ble_hrs_c_t;

/**@brief Heart Rate Client initialization structure.
 */
typedef struct
{
    ble_hrs_c_evt_handler_t evt_handler;  /**< Event handler to be called by the Heart Rate Client module whenever there is an event related to the Heart Rate Service. */
} ble_hrs_c_init_t;

/** @} */

/**
 * @defgroup hrs_c_functions Functions
 * @{
 */

/**@brief     Function for initializing the heart rate client module.
 *
 * @details   This function will register with the DB Discovery module. There it
 *            registers for the Heart Rate Service. Doing so will make the DB Discovery
 *            module look for the presence of a Heart Rate Service instance at the peer when a
 *            discovery is started.
 *
 * @param[in] p_ble_hrs_c      Pointer to the heart rate client structure.
 * @param[in] p_ble_hrs_c_init Pointer to the heart rate initialization structure containing the
 *                             initialization information.
 *
 * @retval    NRF_SUCCESS On successful initialization. Otherwise an error code. This function
 *                        propagates the error code returned by the Database Discovery module API
 *                        @ref ble_db_discovery_evt_register.
 */
uint32_t ble_hrs_c_init(ble_hrs_c_t * p_ble_hrs_c, ble_hrs_c_init_t * p_ble_hrs_c_init);

/**@brief     Function for handling BLE events from the SoftDevice.
 *
 * @details   This function will handle the BLE events received from the SoftDevice. If a BLE
 *            event is relevant to the Heart Rate Client module, then it uses it to update
 *            interval variables and, if necessary, send events to the application.
 *
 * @param[in] p_ble_hrs_c Pointer to the heart rate client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event.
 */
void ble_hrs_c_on_ble_evt(ble_hrs_c_t * p_ble_hrs_c, const ble_evt_t * p_ble_evt);


/**@brief   Function for requesting the peer to start sending notification of Heart Rate
 *          Measurement.
 *
 * @details This function will enable to notification of the Heart Rate Measurement at the peer
 *          by writing to the CCCD of the Heart Rate Measurement Characteristic.
 *
 * @param   p_ble_hrs_c Pointer to the heart rate client structure.
 *
 * @retval  NRF_SUCCESS If the SoftDevice has been requested to write to the CCCD of the peer.
 *                      Otherwise, an error code. This function propagates the error code returned 
 *                      by the SoftDevice API @ref sd_ble_gattc_write.
 */
uint32_t ble_hrs_c_hrm_notif_enable(ble_hrs_c_t * p_ble_hrs_c);

/** @} */ // End tag for Function group.

#endif // BLE_HRS_C_H__

/** @} */ // End tag for the file.
