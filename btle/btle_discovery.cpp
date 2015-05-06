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

#include "blecommon.h"
#include "UUID.h"
#include "Gap.h"
#include "nrf_error.h"
#include "btle_discovery.h"
#include "ble_err.h"

static NordicServiceDiscovery discoverySingleton;

ble_error_t
ServiceDiscovery::launch(Gap::Handle_t connectionHandle, ServiceCallback_t sc, CharacteristicCallback_t cc)
{
    discoverySingleton.serviceDiscoveryStarted(connectionHandle);
    discoverySingleton.serviceCallback        = sc;
    discoverySingleton.characteristicCallback = cc;

    uint32_t rc;
    if ((rc = sd_ble_gattc_primary_services_discover(connectionHandle, NordicServiceDiscovery::SRV_DISC_START_HANDLE, NULL)) != NRF_SUCCESS) {
        discoverySingleton.terminate();
        switch (rc) {
            case NRF_ERROR_INVALID_PARAM:
            case BLE_ERROR_INVALID_CONN_HANDLE:
                return BLE_ERROR_INVALID_PARAM;
            case NRF_ERROR_BUSY:
                return BLE_STACK_BUSY;
            default:
            case NRF_ERROR_INVALID_STATE:
                return BLE_ERROR_INVALID_STATE;
        }
    }

    return BLE_ERROR_NONE;
}

void
ServiceDiscovery::terminate(void)
{
    discoverySingleton.terminateServiceDiscovery();
}

ble_error_t
NordicServiceDiscovery::launchCharacteristicDiscovery(Gap::Handle_t connectionHandle,
                                                      Gap::Handle_t startHandle,
                                                      Gap::Handle_t endHandle)
{
    discoverySingleton.characteristicDiscoveryStarted(connectionHandle);

    ble_gattc_handle_range_t handleRange = {
        .start_handle = startHandle,
        .end_handle   = endHandle
    };
    uint32_t rc;
    if ((rc = sd_ble_gattc_characteristics_discover(connectionHandle, &handleRange)) != NRF_SUCCESS) {
        discoverySingleton.terminateCharacteristicDiscovery();
        switch (rc) {
            case BLE_ERROR_INVALID_CONN_HANDLE:
            case NRF_ERROR_INVALID_ADDR:
                return BLE_ERROR_INVALID_PARAM;
            case NRF_ERROR_BUSY:
                return BLE_STACK_BUSY;
            default:
            case NRF_ERROR_INVALID_STATE:
                return BLE_ERROR_INVALID_STATE;
        }
    }

    return BLE_ERROR_NONE;
}

void
NordicServiceDiscovery::setupDiscoveredServices(const ble_gattc_evt_prim_srvc_disc_rsp_t *response)
{
    serviceIndex = 0;
    numServices  = response->count;

    /* Account for the limitation on the number of discovered services we can handle at a time. */
    if (numServices > BLE_DB_DISCOVERY_MAX_SRV) {
        numServices = BLE_DB_DISCOVERY_MAX_SRV;
    }

    for (unsigned serviceIndex = 0; serviceIndex < numServices; serviceIndex++) {
        services[serviceIndex].setup(response->services[serviceIndex].uuid.uuid,
                                     response->services[serviceIndex].handle_range.start_handle,
                                     response->services[serviceIndex].handle_range.end_handle);
    }
}

void
NordicServiceDiscovery::setupDiscoveredCharacteristics(const ble_gattc_evt_char_disc_rsp_t *response)
{
    characteristicIndex = 0;
    numCharacteristics  = response->count;

    /* Account for the limitation on the number of discovered characteristics we can handle at a time. */
    if (numCharacteristics > BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV) {
        numCharacteristics = BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV;
    }

    for (unsigned charIndex = 0; charIndex < numCharacteristics; charIndex++) {
        characteristics[charIndex].setup(response->chars[charIndex].uuid.uuid,
                                         *(const uint8_t *)(&response->chars[charIndex].char_props),
                                         response->chars[charIndex].handle_decl,
                                         response->chars[charIndex].handle_value);
    }
}

void
NordicServiceDiscovery::progressCharacteristicDiscovery(void)
{
    while (cDiscoveryActive && (characteristicIndex < numCharacteristics)) {
        /* THIS IS WHERE THE CALLBACK WILL GO */
        printf("%x [%u]\r\n", characteristics[characteristicIndex].uuid, characteristics[characteristicIndex].valueHandle);

        characteristicIndex++;
    }

    if (cDiscoveryActive) {
        Gap::Handle_t startHandle = characteristics[characteristicIndex - 1].valueHandle + 1;
        Gap::Handle_t endHandle   = services[serviceIndex].endHandle;
        resetDiscoveredCharacteristics();

        if (startHandle < endHandle) {
            ble_gattc_handle_range_t handleRange = {
                .start_handle = startHandle,
                .end_handle   = endHandle
            };
            printf("char discovery returned %u\r\n", sd_ble_gattc_characteristics_discover(connHandle, &handleRange));
        } else {
           terminateCharacteristicDiscovery();
        }
    }
}

void
NordicServiceDiscovery::progressServiceDiscovery(void)
{
    while (sDiscoveryActive && (serviceIndex < numServices)) {
        if (serviceCallback) {
            serviceCallback(services[serviceIndex]);
        }

        if (true) { /* characteristic discovery is optional. */
            launchCharacteristicDiscovery(connHandle, services[serviceIndex].startHandle, services[serviceIndex].endHandle);
        } else {
            serviceIndex++; /* Progress service index to keep discovery alive. */
        }
    }
    if (sDiscoveryActive && (numServices > 0) && (serviceIndex > 0)) {
        Gap::Handle_t endHandle = services[serviceIndex - 1].endHandle;
        resetDiscoveredServices();

        printf("services discover returned %u\r\n", sd_ble_gattc_primary_services_discover(connHandle, endHandle, NULL));
    }
}

void bleGattcEventHandler(const ble_evt_t *p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS:
                    discoverySingleton.setupDiscoveredServices(&p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp);
                    break;

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND:
                default:
                    discoverySingleton.terminate();
                    break;
            }
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS:
                    discoverySingleton.setupDiscoveredCharacteristics(&p_ble_evt->evt.gattc_evt.params.char_disc_rsp);
                    break;

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND:
                default:
                    discoverySingleton.terminateCharacteristicDiscovery();
                    break;
            }
            break;
    }

    discoverySingleton.progressCharacteristicDiscovery();
    discoverySingleton.progressServiceDiscovery();
}
