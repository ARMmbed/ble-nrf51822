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

#ifndef __NRF51822_GATT_CLIENT_H__
#define __NRF51822_GATT_CLIENT_H__

#include "GattClient.h"

class nRF51GattClient : public GattClient
{
public:
    static nRF51GattClient &getInstance();

#if 0
    /* Functions that must be implemented from GattClient */
    virtual ble_error_t addService(GattService &);
    virtual ble_error_t readValue(GattAttribute::Handle_t attributeHandle, uint8_t buffer[], uint16_t *lengthP);
    virtual ble_error_t readValue(Gap::Handle_t connectionHandle, GattAttribute::Handle_t attributeHandle, uint8_t buffer[], uint16_t *lengthP);
    virtual ble_error_t updateValue(GattAttribute::Handle_t, const uint8_t[], uint16_t, bool localOnly = false);
    virtual ble_error_t updateValue(Gap::Handle_t connectionHandle, GattAttribute::Handle_t, const uint8_t[], uint16_t, bool localOnly = false);
    virtual ble_error_t initializeGATTDatabase(void);

    /* nRF51 Functions */
    void eventCallback(void);
    void hwCallback(ble_evt_t *p_ble_evt);

private:
#endif

public:
    nRF51GattClient() {
        /* empty */
    }

private:
    nRF51GattClient(const nRF51GattClient &);
    const nRF51GattClient& operator=(const nRF51GattClient &);
};

#endif // ifndef __NRF51822_GATT_CLIENT_H__
