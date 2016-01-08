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

#ifndef __NRF51822_SECURITY_MANAGER_H__
#define __NRF51822_SECURITY_MANAGER_H__

#include <stddef.h>

#include "nRF5xGap.h"
#include "ble/SecurityManager.h"
#include "btle_security.h"

class nRF5xSecurityManager : public SecurityManager
{
public:
    /* Functions that must be implemented from SecurityManager */
    virtual ble_error_t init(bool                     enableBonding,
                             bool                     requireMITM,
                             SecurityIOCapabilities_t iocaps,
                             const Passkey_t          passkey) {
        return btle_initializeSecurity(enableBonding, requireMITM, iocaps, passkey);
    }

    virtual ble_error_t getLinkSecurity(Gap::Handle_t connectionHandle, LinkSecurityStatus_t *securityStatusP) {
        return btle_getLinkSecurity(connectionHandle, securityStatusP);
    }

    virtual ble_error_t setLinkSecurity(Gap::Handle_t connectionHandle, SecurityMode_t securityMode) {
        return btle_setLinkSecurity(connectionHandle, securityMode);
    }

    virtual ble_error_t purgeAllBondingState(void) {
        return btle_purgeAllBondingState();
    }

    /**
     * @brief  Clear nRF5xSecurityManager's state.
     *
     * @return
     *           BLE_ERROR_NONE if successful.
     */
    virtual ble_error_t reset(void)
    {
        if (SecurityManager::reset() != BLE_ERROR_NONE) {
            return BLE_ERROR_INVALID_STATE;
        }

        return BLE_ERROR_NONE;
    }

    bool hasInitialized(void) const {
        return btle_hasInitializedSecurity();
    }

public:
    /*
     * Allow instantiation from nRF5xn when required.
     */
    friend class nRF5xn;

    nRF5xSecurityManager() {
        /* empty */
    }

private:
    nRF5xSecurityManager(const nRF5xSecurityManager &);
    const nRF5xSecurityManager& operator=(const nRF5xSecurityManager &);

    ble_error_t createWhitelistFromBondTable(ble_gap_whitelist_t &whitelistFromBondTable) const {
        return btle_createWhitelistFromBondTable(&whitelistFromBondTable);
    }

    bool matchAddressAndIrk(ble_gap_addr_t *address, ble_gap_irk_t *irk) const {
        return btle_matchAddressAndIrk(address, irk);
    }
    friend class nRF5xGap;
};

#endif // ifndef __NRF51822_SECURITY_MANAGER_H__
