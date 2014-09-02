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

#ifndef __NRF51822_GATT_SERVER_H__
#define __NRF51822_GATT_SERVER_H__

#include "mbed.h"
#include "blecommon.h"
#include "ble.h" /* nordic ble */
#include "GattService.h"
#include "public/GattServer.h"

#define BLE_TOTAL_CHARACTERISTICS 10
#define BLE_TOTAL_DESCRIPTORS     10

class nRF51GattServer : public GattServer
{
public:
    static nRF51GattServer &getInstance() {
        static nRF51GattServer m_instance;
        return m_instance;
    }

    /* Functions that must be implemented from GattServer */
    virtual ble_error_t addService(GattService &);
    virtual ble_error_t readValue(uint16_t handle, uint8_t buffer[], uint16_t *const lengthP);
    virtual ble_error_t updateValue(uint16_t, uint8_t[], uint16_t, bool localOnly = false);

    /* nRF51 Functions */
    void eventCallback(void);
    void hwCallback(ble_evt_t *p_ble_evt);

private:
    GattCharacteristic *p_characteristics[BLE_TOTAL_CHARACTERISTICS];
    ble_gatts_char_handles_t nrfCharacteristicHandles[BLE_TOTAL_CHARACTERISTICS];
    GattAttribute *p_descriptors[BLE_TOTAL_DESCRIPTORS];
    uint16_t nrfDescriptorHandles[BLE_TOTAL_DESCRIPTORS];

    nRF51GattServer() {
        serviceCount = 0;
        characteristicCount = 0;
        descriptorCount = 0;
    };

    nRF51GattServer(nRF51GattServer const &);
    void operator=(nRF51GattServer const &);
};

#endif
