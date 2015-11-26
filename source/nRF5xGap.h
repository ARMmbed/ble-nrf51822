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

#ifndef __NRF5x_GAP_H__
#define __NRF5x_GAP_H__

#include "mbed.h"
#include "ble/blecommon.h"
#include "ble.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/Gap.h"
#include "ble/GapScanningParams.h"

#include "nrf_soc.h"

extern "C" {
#include "ble_radio_notification.h"
}

#include "btle_security.h"

void radioNotificationStaticCallback(bool param);

/**************************************************************************/
/*!
    \brief

*/
/**************************************************************************/
class nRF5xGap : public Gap
{
public:
    static nRF5xGap &getInstance();

    /* Functions that must be implemented from Gap */
    virtual ble_error_t setAddress(AddressType_t  type,  const Address_t address);
    virtual ble_error_t getAddress(AddressType_t *typeP, Address_t address);
    virtual ble_error_t setAdvertisingData(const GapAdvertisingData &, const GapAdvertisingData &);

    virtual uint16_t    getMinAdvertisingInterval(void) const {return GapAdvertisingParams::ADVERTISEMENT_DURATION_UNITS_TO_MS(BLE_GAP_ADV_INTERVAL_MIN);}
    virtual uint16_t    getMinNonConnectableAdvertisingInterval(void) const {return GapAdvertisingParams::ADVERTISEMENT_DURATION_UNITS_TO_MS(BLE_GAP_ADV_NONCON_INTERVAL_MIN);}
    virtual uint16_t    getMaxAdvertisingInterval(void) const {return GapAdvertisingParams::ADVERTISEMENT_DURATION_UNITS_TO_MS(BLE_GAP_ADV_INTERVAL_MAX);}

    virtual ble_error_t startAdvertising(const GapAdvertisingParams &);
    virtual ble_error_t stopAdvertising(void);
    virtual ble_error_t connect(const Address_t, Gap::AddressType_t peerAddrType, const ConnectionParams_t *connectionParams, const GapScanningParams *scanParams);
    virtual ble_error_t disconnect(Handle_t connectionHandle, DisconnectionReason_t reason);
    virtual ble_error_t disconnect(DisconnectionReason_t reason);

    virtual ble_error_t setDeviceName(const uint8_t *deviceName);
    virtual ble_error_t getDeviceName(uint8_t *deviceName, unsigned *lengthP);
    virtual ble_error_t setAppearance(GapAdvertisingData::Appearance appearance);
    virtual ble_error_t getAppearance(GapAdvertisingData::Appearance *appearanceP);

    virtual ble_error_t setTxPower(int8_t txPower);
    virtual void        getPermittedTxPowerValues(const int8_t **valueArrayPP, size_t *countP);

    void     setConnectionHandle(uint16_t con_handle);
    uint16_t getConnectionHandle(void);

    virtual ble_error_t getPreferredConnectionParams(ConnectionParams_t *params);
    virtual ble_error_t setPreferredConnectionParams(const ConnectionParams_t *params);
    virtual ble_error_t updateConnectionParams(Handle_t handle, const ConnectionParams_t *params);

    virtual ble_error_t initRadioNotification(void) {
        if (ble_radio_notification_init(NRF_APP_PRIORITY_HIGH, NRF_RADIO_NOTIFICATION_DISTANCE_800US, radioNotificationStaticCallback) == NRF_SUCCESS) {
            return BLE_ERROR_NONE;
        }

        return BLE_ERROR_UNSPECIFIED;
    }

/* Observer role is not supported by S110, return BLE_ERROR_NOT_IMPLEMENTED */
#if !defined(TARGET_MCU_NRF51_16K_S110) && !defined(TARGET_MCU_NRF51_32K_S110)
    virtual ble_error_t startRadioScan(const GapScanningParams &scanningParams) {
        ble_gap_scan_params_t scanParams = {
            .active      = scanningParams.getActiveScanning(), /**< If 1, perform active scanning (scan requests). */
            .selective   = 0,    /**< If 1, ignore unknown devices (non whitelisted). */
            .p_whitelist = NULL, /**< Pointer to whitelist, NULL if none is given. */
            .interval    = scanningParams.getInterval(),  /**< Scan interval between 0x0004 and 0x4000 in 0.625ms units (2.5ms to 10.24s). */
            .window      = scanningParams.getWindow(),    /**< Scan window between 0x0004 and 0x4000 in 0.625ms units (2.5ms to 10.24s). */
            .timeout     = scanningParams.getTimeout(),   /**< Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout. */
        };

        if (sd_ble_gap_scan_start(&scanParams) != NRF_SUCCESS) {
            return BLE_ERROR_PARAM_OUT_OF_RANGE;
        }

        return BLE_ERROR_NONE;
    }

    virtual ble_error_t stopScan(void) {
        if (sd_ble_gap_scan_stop() == NRF_SUCCESS) {
            return BLE_ERROR_NONE;
        }

        return BLE_STACK_BUSY;
    }
#endif

private:
#ifdef YOTTA_CFG_MBED_OS
    /*
     * In mbed OS, all user-facing BLE events (interrupts) are posted to the
     * MINAR scheduler to be executed as callbacks in thread mode. MINAR guards
     * its critical sections from interrupts by acquiring CriticalSectionLock,
     * which results in a call to sd_nvic_critical_region_enter(). Thus, it is
     * safe to invoke MINAR APIs from interrupt context as long as those
     * interrupts are blocked by sd_nvic_critical_region_enter().
     *
     * Radio notifications are a special case for the above. The Radio
     * Notification IRQ is handled at a very high priority--higher than the
     * level blocked by sd_nvic_critical_region_enter(). Thus Radio Notification
     * events can preempt MINAR's critical sections. Using MINAR APIs (such as
     * posting an event) directly in processRadioNotification() may result in a
     * race condition ending in a hard-fault.
     *
     * The solution is to *not* call MINAR APIs directly from the Radio
     * Notification handling; i.e. to do the bulk of RadioNotification
     * processing at a reduced priority which respects MINAR's critical
     * sections. Unfortunately, on a cortex-M0, there is no clean way to demote
     * priority for the currently executing interrupt--we wouldn't want to
     * demote the radio notification handling anyway because it is sensitive to
     * timing, and the system expects to finish this handling very quickly. The
     * workaround is to employ a Timeout to trigger
     * postRadioNotificationCallback() after a very short delay (~0 us) and post
     * the MINAR callback that context.
     *
     * !!!WARNING!!! Radio notifications are very time critical events. The
     * current solution is expected to work under the assumption that
     * postRadioNotificationCalback() will be executed BEFORE the next radio
     * notification event is generated.
     */

    bool    radioNotificationCallbackParam; /* parameter to be passed into the Timeout-generated radio notification callback. */
    Timeout radioNotificationTimeout;

    /*
     * A helper function to post radio notification callbacks through MINAR when using mbed OS.
     */
    void postRadioNotificationCallback(void) {
        minar::Scheduler::postCallback(
            mbed::util::FunctionPointer1<void, bool>(&radioNotificationCallback, &FunctionPointerWithContext<bool>::call).bind(radioNotificationCallbackParam)
        );
    }
#endif /* #ifdef YOTTA_CFG_MBED_OS */

    /**
     * A helper function to process radio-notification events; to be called internally.
     * @param param [description]
     */
    void processRadioNotificationEvent(bool param) {
#ifdef YOTTA_CFG_MBED_OS
        /* When using mbed OS the callback to the user-defined function will be posted through minar */
        radioNotificationCallbackParam = param;
        radioNotificationTimeout.attach_us(this, &nRF5xGap::postRadioNotificationCallback, 0);
#else
        radioNotificationCallback.call(param);
#endif
    }
    friend void radioNotificationStaticCallback(bool param); /* allow invocations of processRadioNotificationEvent() */

private:
    uint16_t m_connectionHandle;
    nRF5xGap() {
        m_connectionHandle = BLE_CONN_HANDLE_INVALID;
    }

    nRF5xGap(nRF5xGap const &);
    void operator=(nRF5xGap const &);
};

#endif // ifndef __NRF5x_GAP_H__
