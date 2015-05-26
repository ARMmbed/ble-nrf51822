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

#ifndef _BTLE_DISCOVERY_H_
#define _BTLE_DISCOVERY_H_

#include "ble.h"
#include "ServiceDiscovery.h"

void bleGattcEventHandler(const ble_evt_t *p_ble_evt);

class NordicServiceDiscovery : public ServiceDiscovery
{
public:
    static const uint16_t SRV_DISC_START_HANDLE             = 0x0001; /**< The start handle value used during service discovery. */
    static const uint16_t SRV_DISC_END_HANDLE               = 0xFFFF; /**< The end handle value used during service discovery. */

private:
    static const unsigned BLE_DB_DISCOVERY_MAX_SRV          = 4;      /**< Maximum number of services we can retain information for after a single discovery. */
    static const unsigned BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV = 4;      /**< Maximum number of characteristics per service we can retain information for. */

public:
    ble_error_t launchCharacteristicDiscovery(Gap::Handle_t connectionHandle, Gap::Handle_t startHandle, Gap::Handle_t endHandle);

public:
    void setupDiscoveredServices(const ble_gattc_evt_prim_srvc_disc_rsp_t *response);
    void setupDiscoveredCharacteristics(const ble_gattc_evt_char_disc_rsp_t *response);

    void terminateServiceDiscovery(void) {
        bool wasActive = isActive();
        state = INACTIVE;

        if (wasActive && onTerminationCallback) {
            onTerminationCallback(connHandle);
        }
    }

    void terminateCharacteristicDiscovery(void) {
        if (state == CHARACTERISTIC_DISCOVERY_ACTIVE) {
            state = SERVICE_DISCOVERY_ACTIVE;
        }
        serviceIndex++; /* Progress service index to keep discovery alive. */
    }

    bool isActive(void) const {
        return state != INACTIVE;
    }

    void setOnTermination(TerminationCallback_t callback) {
        onTerminationCallback = callback;
    }

private:
    void resetDiscoveredServices(void) {
        numServices  = 0;
        serviceIndex = 0;
        memset(services, 0, sizeof(DiscoveredService) * BLE_DB_DISCOVERY_MAX_SRV);
    }

    void resetDiscoveredCharacteristics(void) {
        numCharacteristics  = 0;
        characteristicIndex = 0;
        memset(characteristics, 0, sizeof(DiscoveredCharacteristic) * BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV);
    }

public:
    void serviceDiscoveryStarted(Gap::Handle_t connectionHandle) {
        connHandle       = connectionHandle;
        resetDiscoveredServices();
        state = SERVICE_DISCOVERY_ACTIVE;
    }

private:
    void characteristicDiscoveryStarted(Gap::Handle_t connectionHandle) {
        connHandle       = connectionHandle;
        resetDiscoveredCharacteristics();
        state = CHARACTERISTIC_DISCOVERY_ACTIVE;
    }

private:
    friend void bleGattcEventHandler(const ble_evt_t *p_ble_evt);
    void progressCharacteristicDiscovery(void);
    void progressServiceDiscovery(void);

private:
    uint8_t  serviceIndex;        /**< Index of the current service being discovered. This is intended for internal use during service discovery.*/
    uint8_t  numServices;         /**< Number of services at the peers GATT database.*/
    uint8_t  characteristicIndex; /**< Index of the current characteristic being discovered. This is intended for internal use during service discovery.*/
    uint8_t  numCharacteristics;  /**< Number of characteristics within the service.*/

    enum State_t {
        INACTIVE,
        SERVICE_DISCOVERY_ACTIVE,
        CHARACTERISTIC_DISCOVERY_ACTIVE,
    } state;

    DiscoveredService        services[BLE_DB_DISCOVERY_MAX_SRV];  /**< Information related to the current service being discovered.
                                                                   *  This is intended for internal use during service discovery. */
    DiscoveredCharacteristic characteristics[BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV];

    TerminationCallback_t onTerminationCallback;
};

#endif /*_BTLE_DISCOVERY_H_*/
