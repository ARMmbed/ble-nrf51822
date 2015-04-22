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

#include "btle.h"
#include "pstorage.h"
#include "nRF51Gap.h"
#include "device_manager.h"
#include "btle_security.h"

static uint8_t applicationInstance;
static ret_code_t dm_handler(dm_handle_t const *p_handle, dm_event_t const *p_event, ret_code_t event_result);

ble_error_t
btle_initializeSecurity()
{
    if (pstorage_init() != NRF_SUCCESS) {
        return BLE_ERROR_UNSPECIFIED;
    }

    dm_init_param_t dm_init_param = {
        .clear_persistent_data = false /* Set to true in case the module should clear all persistent data. */
    };
    if (dm_init(&dm_init_param) != NRF_SUCCESS) {
        return BLE_ERROR_UNSPECIFIED;
    }

    const dm_application_param_t dm_param = {
        .evt_handler  = dm_handler,
        .service_type = DM_PROTOCOL_CNTXT_GATT_CLI_ID,
        .sec_param    = {
            .bond          = 1,            /**< Perform bonding. */
            .mitm          = 1,            /**< Man In The Middle protection required. */
            .io_caps       = BLE_GAP_IO_CAPS_NONE, /**< IO capabilities, see @ref BLE_GAP_IO_CAPS. */
            .oob           = 0,            /**< Out Of Band data available. */
            .min_key_size  = 16,           /**< Minimum encryption key size in octets between 7 and 16. If 0 then not applicable in this instance. */
            .max_key_size  = 16,           /**< Maximum encryption key size in octets between min_key_size and 16. */
            .kdist_periph  = {
              .enc  = 1,                     /**< Long Term Key and Master Identification. */
              .id   = 1,                     /**< Identity Resolving Key and Identity Address Information. */
              .sign = 1,                     /**< Connection Signature Resolving Key. */
            },                             /**< Key distribution bitmap: keys that the peripheral device will distribute. */
        }
    };

    ret_code_t rc;
    if ((rc = dm_register(&applicationInstance, &dm_param)) == NRF_SUCCESS) {
        return BLE_ERROR_NONE;
    }

    switch (rc) {
        case NRF_ERROR_INVALID_STATE:
            return BLE_ERROR_INVALID_STATE;
        case NRF_ERROR_NO_MEM:
            return BLE_ERROR_NO_MEM;
        default:
            return BLE_ERROR_UNSPECIFIED;
    }
}

ble_error_t
btle_deleteAllStoredDevices(void)
{
    ret_code_t rc;
    if ((rc = dm_device_delete_all(&applicationInstance)) == NRF_SUCCESS) {
        return BLE_ERROR_NONE;
    }

    switch (rc) {
        case NRF_ERROR_INVALID_STATE:
            return BLE_ERROR_INVALID_STATE;
        case NRF_ERROR_NO_MEM:
            return BLE_ERROR_NO_MEM;
        default:
            return BLE_ERROR_UNSPECIFIED;
    }
}

ble_error_t
btle_getLinkSecurity(Gap::Handle_t connectionHandle, Gap::LinkSecurityStatus_t *securityStatusP)
{
    dm_handle_t dmHandle;
    ret_code_t rc;
    if ((rc = dm_handle_get(connectionHandle, &dmHandle)) != NRF_SUCCESS) {
        if (rc == NRF_ERROR_NOT_FOUND) {
            return BLE_ERROR_INVALID_PARAM;
        } else {
            return BLE_ERROR_UNSPECIFIED;
        }
    }

    if ((rc = dm_security_status_req(&dmHandle, reinterpret_cast<dm_security_status_t *>(securityStatusP))) != NRF_SUCCESS) {
        switch (rc) {
            case NRF_ERROR_INVALID_STATE:
                return BLE_ERROR_INVALID_STATE;
            case NRF_ERROR_NO_MEM:
                return BLE_ERROR_NO_MEM;
            default:
                return BLE_ERROR_UNSPECIFIED;
        }
    }

    return BLE_ERROR_NONE;
}

ret_code_t
dm_handler(dm_handle_t const *p_handle, dm_event_t const *p_event, ret_code_t event_result)
{
    switch (p_event->event_id) {
        case DM_EVT_SECURITY_SETUP: /* started */
            nRF51Gap::getInstance().processSecuritySetupStartedEvent(p_event->event_param.p_gap_param->conn_handle);
            break;
        case DM_EVT_SECURITY_SETUP_COMPLETE:
            nRF51Gap::getInstance().processSecuritySetupCompletedEvent(p_event->event_param.p_gap_param->conn_handle);
            break;
        case DM_EVT_LINK_SECURED:
            nRF51Gap::getInstance().processLinkSecuredEvent(p_event->event_param.p_gap_param->conn_handle);
            break;
        case DM_EVT_DEVICE_CONTEXT_STORED:
            nRF51Gap::getInstance().processSecurityContextStoredEvent(p_event->event_param.p_gap_param->conn_handle);
            break;
        default:
            break;
    }

    return NRF_SUCCESS;
}
