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

#ifndef __NRF51822_GAP_H__
#define __NRF51822_GAP_H__

#include "mbed.h"
#include "blecommon.h"
#include "ble.h"
#include "GapAdvertisingParams.h"
#include "GapAdvertisingData.h"
#include "public/Gap.h"

#include "nrf_soc.h"
#include "ble_radio_notification.h"

/**************************************************************************/
/*!
    \brief

*/
/**************************************************************************/
class nRF51Gap : public Gap
{
public:
    static nRF51Gap &getInstance() {
        static nRF51Gap m_instance;
        return m_instance;
    }

    /* Functions that must be implemented from Gap */
    virtual ble_error_t setAddress(addr_type_t  type,  const address_t address);
    virtual ble_error_t getAddress(addr_type_t *typeP, address_t address);
    virtual ble_error_t setAdvertisingData(const GapAdvertisingData &, const GapAdvertisingData &);

    virtual uint16_t    getMinAdvertisingInterval(void) const {return GAP_DURATION_UNITS_TO_MS(BLE_GAP_ADV_INTERVAL_MIN);}
    virtual uint16_t    getMinNonConnectableAdvertisingInterval(void) const {return GAP_DURATION_UNITS_TO_MS(BLE_GAP_ADV_NONCON_INTERVAL_MIN);}
    virtual uint16_t    getMaxAdvertisingInterval(void) const {return GAP_DURATION_UNITS_TO_MS(BLE_GAP_ADV_INTERVAL_MAX);}

    virtual ble_error_t startAdvertising(const GapAdvertisingParams &);
    virtual ble_error_t stopAdvertising(void);
    virtual ble_error_t disconnect(DisconnectionReason_t reason);

    virtual ble_error_t setDeviceName(const uint8_t *deviceName);
    virtual ble_error_t getDeviceName(uint8_t *deviceName, unsigned *lengthP);
    virtual ble_error_t setAppearance(uint16_t appearance);
    virtual ble_error_t getAppearance(uint16_t *appearanceP);

    void     setConnectionHandle(uint16_t con_handle);
    uint16_t getConnectionHandle(void);

    virtual ble_error_t getPreferredConnectionParams(ConnectionParams_t *params);
    virtual ble_error_t setPreferredConnectionParams(const ConnectionParams_t *params);
    virtual ble_error_t updateConnectionParams(Handle_t handle, const ConnectionParams_t *params);

    virtual void setOnRadioNotification(RadioNotificationEventCallback_t callback) {
        Gap::setOnRadioNotification(callback);
        ble_radio_notification_init(NRF_APP_PRIORITY_HIGH, NRF_RADIO_NOTIFICATION_DISTANCE_800US, onRadioNotification);
    }

private:
    uint16_t m_connectionHandle;
    nRF51Gap() {
        m_connectionHandle = BLE_CONN_HANDLE_INVALID;
    }

    nRF51Gap(nRF51Gap const &);
    void operator=(nRF51Gap const &);
};

#endif // ifndef __NRF51822_GAP_H__
