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

#include "mbed.h"
#include "nRF5xn.h"
#include "ble/blecommon.h"
#include "nrf_soc.h"

#include "btle/btle.h"
#include "nrf_delay.h"

extern "C" {
#include "softdevice_handler.h"
}

/**
 * The singleton which represents the nRF51822 transport for the BLE.
 */
static nRF5xn deviceInstance;

/**
 * BLE-API requires an implementation of the following function in order to
 * obtain its transport handle.
 */
BLEInstanceBase *
createBLEInstance(void)
{
    return (&deviceInstance);
}

nRF5xn::nRF5xn(void) : initialized(false), instanceID(BLE::DEFAULT_INSTANCE)
{
}

nRF5xn::~nRF5xn(void)
{
}

const char *nRF5xn::getVersion(void)
{
    if (!initialized) {
        return "INITIALIZATION_INCOMPLETE";
    }

    static char versionString[32];
    static bool versionFetched = false;

    if (!versionFetched) {
        ble_version_t version;
        if ((sd_ble_version_get(&version) == NRF_SUCCESS) && (version.company_id == 0x0059)) {
            switch (version.version_number) {
                case 0x07:
                case 0x08:
                    snprintf(versionString, sizeof(versionString), "Nordic BLE4.1 ver:%u fw:%04x", version.version_number, version.subversion_number);
                    break;
                default:
                    snprintf(versionString, sizeof(versionString), "Nordic (spec unknown) ver:%u fw:%04x", version.version_number, version.subversion_number);
                    break;
            }
            versionFetched = true;
        } else {
            strncpy(versionString, "unknown", sizeof(versionString));
        }
    }

    return versionString;
}

ble_error_t nRF5xn::init(BLE::InstanceID_t instanceID, FunctionPointerWithContext<BLE::InitializationCompleteCallbackContext *> callback)
{
    if (initialized) {
        BLE::InitializationCompleteCallbackContext context = {
            BLE::Instance(instanceID),
            BLE_ERROR_ALREADY_INITIALIZED
        };
        callback.call(&context);
        return BLE_ERROR_ALREADY_INITIALIZED;
    }

    instanceID   = instanceID;

    /* ToDo: Clear memory contents, reset the SD, etc. */
    btle_init();

    initialized = true;
    BLE::InitializationCompleteCallbackContext context = {
        BLE::Instance(instanceID),
        BLE_ERROR_NONE
    };
    callback.call(&context);
    return BLE_ERROR_NONE;
}

ble_error_t nRF5xn::shutdown(void)
{
    if (!initialized) {
        return BLE_ERROR_INITIALIZATION_INCOMPLETE;
    }

    if(softdevice_handler_sd_disable() != NRF_SUCCESS) {
        return BLE_STACK_BUSY;
    }

    initialized = false;
    return BLE_ERROR_NONE;
}

void
nRF5xn::waitForEvent(void)
{
    sd_app_evt_wait();
}
