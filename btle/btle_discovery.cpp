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

#include <stdio.h>

#include "blecommon.h"
#include "UUID.h"
#include "Gap.h"
#include "nrf_error.h"
#include "btle_discovery.h"
#include "ble_err.h"

static NordicServiceDiscovery sdSingleton;
DiscoveredCharacteristic::ReadCallback_t DiscoveredCharacteristic::onDataReadCallback = NULL;

ble_error_t
ServiceDiscovery::launch(Gap::Handle_t             connectionHandle,
                         ServiceCallback_t         sc,
                         CharacteristicCallback_t  cc,
                         const UUID               &matchingServiceUUIDIn,
                         const UUID               &matchingCharacteristicUUIDIn)
{
    if (isActive()) {
        return BLE_ERROR_INVALID_STATE;
    }

    sdSingleton.serviceCallback            = sc;
    sdSingleton.characteristicCallback     = cc;
    sdSingleton.matchingServiceUUID        = matchingServiceUUIDIn;
    sdSingleton.matchingCharacteristicUUID = matchingCharacteristicUUIDIn;

    sdSingleton.serviceDiscoveryStarted(connectionHandle);

    uint32_t rc;
    if ((rc = sd_ble_gattc_primary_services_discover(connectionHandle, NordicServiceDiscovery::SRV_DISC_START_HANDLE, NULL)) != NRF_SUCCESS) {
        sdSingleton.terminate();
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
    sdSingleton.terminateServiceDiscovery();
}

bool
ServiceDiscovery::isActive(void)
{
    return sdSingleton.isActive();
}

void ServiceDiscovery::onTermination(TerminationCallback_t callback) {
    sdSingleton.setOnTermination(callback);
}

ble_error_t
NordicServiceDiscovery::launchCharacteristicDiscovery(Gap::Handle_t connectionHandle,
                                                      Gap::Handle_t startHandle,
                                                      Gap::Handle_t endHandle)
{
    sdSingleton.characteristicDiscoveryStarted(connectionHandle);

    ble_gattc_handle_range_t handleRange = {
        .start_handle = startHandle,
        .end_handle   = endHandle
    };
    uint32_t rc;
    if ((rc = sd_ble_gattc_characteristics_discover(connectionHandle, &handleRange)) != NRF_SUCCESS) {
        sdSingleton.terminateCharacteristicDiscovery();
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
NordicServiceDiscovery::ServiceUUIDDiscoveryQueue::triggerFirst(void)
{
    while (numIndices) { /* loop until a call to char_value_by_uuid_read() succeeds or we run out of pending indices. */
        parentDiscoveryObject->state = DISCOVER_SERVICE_UUIDS;

        unsigned serviceIndex = getFirst();
        ble_uuid_t uuid = {
            .uuid = BLE_UUID_SERVICE_PRIMARY,
            .type = BLE_UUID_TYPE_BLE,
        };
        ble_gattc_handle_range_t handleRange = {
            .start_handle = parentDiscoveryObject->services[serviceIndex].getStartHandle(),
            .end_handle   = parentDiscoveryObject->services[serviceIndex].getEndHandle(),
        };
        if (sd_ble_gattc_char_value_by_uuid_read(parentDiscoveryObject->connHandle, &uuid, &handleRange) == NRF_SUCCESS) {
            return;
        }

        /* Skip this service if we fail to launch a read for its service-declaration
         * attribute. Its UUID will remain INVALID, and it may not match any filters. */
        dequeue();
    }

    /* Switch back to service discovery upon exhausting the service-indices pending UUID discovery. */
    if (parentDiscoveryObject->state == DISCOVER_SERVICE_UUIDS) {
        parentDiscoveryObject->state = SERVICE_DISCOVERY_ACTIVE;
    }
}

void
NordicServiceDiscovery::CharUUIDDiscoveryQueue::triggerFirst(void)
{
    while (numIndices) { /* loop until a call to char_value_by_uuid_read() succeeds or we run out of pending indices. */
        parentDiscoveryObject->state = DISCOVER_CHARACTERISTIC_UUIDS;

        unsigned charIndex = getFirst();
        ble_uuid_t uuid = {
            .uuid = BLE_UUID_CHARACTERISTIC,
            .type = BLE_UUID_TYPE_BLE,
        };
        ble_gattc_handle_range_t handleRange = {
            .start_handle = parentDiscoveryObject->characteristics[charIndex].getDeclHandle(),
            .end_handle   = parentDiscoveryObject->characteristics[charIndex].getDeclHandle() + 1,
        };
        if (sd_ble_gattc_char_value_by_uuid_read(parentDiscoveryObject->connHandle, &uuid, &handleRange) == NRF_SUCCESS) {
            return;
        }

        /* Skip this service if we fail to launch a read for its service-declaration
         * attribute. Its UUID will remain INVALID, and it may not match any filters. */
        dequeue();
    }

    /* Switch back to service discovery upon exhausting the service-indices pending UUID discovery. */
    if (parentDiscoveryObject->state == DISCOVER_CHARACTERISTIC_UUIDS) {
        parentDiscoveryObject->state = CHARACTERISTIC_DISCOVERY_ACTIVE;
    }
}

void
NordicServiceDiscovery::processDiscoverUUIDResponse(const ble_gattc_evt_char_val_by_uuid_read_rsp_t *response)
{
    if (state == DISCOVER_SERVICE_UUIDS) {
        if ((response->count == 1) && (response->value_len == UUID::LENGTH_OF_LONG_UUID)) {
            UUID::LongUUIDBytes_t uuid;
            /* Switch longUUID bytes to MSB */
            for (unsigned i = 0; i < UUID::LENGTH_OF_LONG_UUID; i++) {
                uuid[i] = response->handle_value[0].p_value[UUID::LENGTH_OF_LONG_UUID - 1 - i];
            }

            unsigned serviceIndex = serviceUUIDDiscoveryQueue.dequeue();
            services[serviceIndex].setupLongUUID(uuid);

            serviceUUIDDiscoveryQueue.triggerFirst();
        } else {
            serviceUUIDDiscoveryQueue.dequeue();
        }
    } else if (state == DISCOVER_CHARACTERISTIC_UUIDS) {
        if ((response->count == 1) && (response->value_len == UUID::LENGTH_OF_LONG_UUID + 1 /* props */ + 2 /* value handle */)) {
            UUID::LongUUIDBytes_t uuid;
            /* Switch longUUID bytes to MSB */
            for (unsigned i = 0; i < UUID::LENGTH_OF_LONG_UUID; i++) {
                uuid[i] = response->handle_value[0].p_value[3 + UUID::LENGTH_OF_LONG_UUID - 1 - i];
            }

            unsigned charIndex = charUUIDDiscoveryQueue.dequeue();
            characteristics[charIndex].setupLongUUID(uuid);

            charUUIDDiscoveryQueue.triggerFirst();
        } else {
            charUUIDDiscoveryQueue.dequeue();
        }
    }
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

    serviceUUIDDiscoveryQueue.reset();
    for (unsigned serviceIndex = 0; serviceIndex < numServices; serviceIndex++) {
        if (response->services[serviceIndex].uuid.type == BLE_UUID_TYPE_UNKNOWN) {
            serviceUUIDDiscoveryQueue.enqueue(serviceIndex);
            services[serviceIndex].setup(response->services[serviceIndex].handle_range.start_handle,
                                         response->services[serviceIndex].handle_range.end_handle);
        } else {
            services[serviceIndex].setup(response->services[serviceIndex].uuid.uuid,
                                         response->services[serviceIndex].handle_range.start_handle,
                                         response->services[serviceIndex].handle_range.end_handle);
        }
    }

    /* Trigger discovery of service UUID if necessary. */
    if (serviceUUIDDiscoveryQueue.getCount()) {
        serviceUUIDDiscoveryQueue.triggerFirst();
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

    charUUIDDiscoveryQueue.reset();
    for (unsigned charIndex = 0; charIndex < numCharacteristics; charIndex++) {
        if (response->chars[charIndex].uuid.type == BLE_UUID_TYPE_UNKNOWN) {
            charUUIDDiscoveryQueue.enqueue(charIndex);
            characteristics[charIndex].setup(connHandle,
                                             *(const uint8_t *)(&response->chars[charIndex].char_props),
                                             response->chars[charIndex].handle_decl,
                                             response->chars[charIndex].handle_value);
        } else {
            characteristics[charIndex].setup(connHandle,
                                             response->chars[charIndex].uuid.uuid,
                                             *(const uint8_t *)(&response->chars[charIndex].char_props),
                                             response->chars[charIndex].handle_decl,
                                             response->chars[charIndex].handle_value);
        }
    }

    /* Trigger discovery of char UUID if necessary. */
    if (charUUIDDiscoveryQueue.getCount()) {
        charUUIDDiscoveryQueue.triggerFirst();
    }
}

void
NordicServiceDiscovery::progressCharacteristicDiscovery(void)
{
    /* Iterate through the previously discovered characteristics cached in characteristics[]. */
    while ((state == CHARACTERISTIC_DISCOVERY_ACTIVE) && (characteristicIndex < numCharacteristics)) {
        if ((matchingCharacteristicUUID == UUID::ShortUUIDBytes_t(BLE_UUID_UNKNOWN)) ||
            (matchingCharacteristicUUID == characteristics[characteristicIndex].getShortUUID())) {
            if (characteristicCallback) {
                characteristicCallback(&characteristics[characteristicIndex]);
            }
        }

        characteristicIndex++;
    }

    /* Relaunch discovery of new characteristics beyond the last entry cached in characteristics[]. */
    if (state == CHARACTERISTIC_DISCOVERY_ACTIVE) {
        /* Determine the ending handle of the last cached characteristic. */
        Gap::Handle_t startHandle = characteristics[characteristicIndex - 1].getValueHandle() + 1;
        Gap::Handle_t endHandle   = services[serviceIndex].getEndHandle();
        resetDiscoveredCharacteristics(); /* Note: resetDiscoveredCharacteristics() must come after fetching start and end Handles. */

        if (startHandle < endHandle) {
            ble_gattc_handle_range_t handleRange = {
                .start_handle = startHandle,
                .end_handle   = endHandle
            };
            if (sd_ble_gattc_characteristics_discover(connHandle, &handleRange) != NRF_SUCCESS) {
                terminateCharacteristicDiscovery();
            }
        } else {
            terminateCharacteristicDiscovery();
        }
    }
}

void
NordicServiceDiscovery::progressServiceDiscovery(void)
{
    /* Iterate through the previously discovered services cached in services[]. */
    while ((state == SERVICE_DISCOVERY_ACTIVE) && (serviceIndex < numServices)) {
        if ((matchingServiceUUID == UUID::ShortUUIDBytes_t(BLE_UUID_UNKNOWN)) ||
            (matchingServiceUUID == services[serviceIndex].getUUID().getShortUUID())) {
            if (serviceCallback) {
                serviceCallback(services[serviceIndex]);
            }

            if ((state == SERVICE_DISCOVERY_ACTIVE) && characteristicCallback) {
                launchCharacteristicDiscovery(connHandle, services[serviceIndex].getStartHandle(), services[serviceIndex].getEndHandle());
            } else {
                serviceIndex++;
            }
        } else {
            serviceIndex++;
        }
    }

    /* Relaunch discovery of new services beyond the last entry cached in services[]. */
    if ((state == SERVICE_DISCOVERY_ACTIVE) && (numServices > 0) && (serviceIndex > 0)) {
        /* Determine the ending handle of the last cached service. */
        Gap::Handle_t endHandle = services[serviceIndex - 1].getEndHandle();
        resetDiscoveredServices(); /* Note: resetDiscoveredServices() must come after fetching endHandle. */

        if (endHandle == SRV_DISC_END_HANDLE) {
            terminateServiceDiscovery();
        } else {
            if (sd_ble_gattc_primary_services_discover(connHandle, endHandle, NULL) != NRF_SUCCESS) {
                terminateServiceDiscovery();
            }
        }
    }
}

void bleGattcEventHandler(const ble_evt_t *p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS:
                    sdSingleton.setupDiscoveredServices(&p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp);
                    break;

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND:
                default:
                    sdSingleton.terminate();
                    break;
            }
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS:
                    sdSingleton.setupDiscoveredCharacteristics(&p_ble_evt->evt.gattc_evt.params.char_disc_rsp);
                    break;

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND:
                default:
                    sdSingleton.terminateCharacteristicDiscovery();
                    break;
            }
            break;

        case BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP:
            if (sdSingleton.isActive()) {
                sdSingleton.processDiscoverUUIDResponse(&p_ble_evt->evt.gattc_evt.params.char_val_by_uuid_read_rsp);
            }
            break;

        case BLE_GATTC_EVT_READ_RSP:
            printf("BLE_GATTC_EVT_READ_RSP\r\n");
            if (DiscoveredCharacteristic::onDataReadCallback != NULL) {
                DiscoveredCharacteristic::ReadResponse_t response = {
                    .handle = p_ble_evt->evt.gattc_evt.params.read_rsp.handle,
                    .offset = p_ble_evt->evt.gattc_evt.params.read_rsp.offset,
                    .len    = p_ble_evt->evt.gattc_evt.params.read_rsp.len,
                    .data   = p_ble_evt->evt.gattc_evt.params.read_rsp.data,
                };
                DiscoveredCharacteristic::onDataReadCallback(&response);
            }
            break;
    }

    sdSingleton.progressCharacteristicDiscovery();
    sdSingleton.progressServiceDiscovery();
}
