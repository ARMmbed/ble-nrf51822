/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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

#ifndef __NRF_CHARACTERISTIC_DESCRIPTOR_DISCOVERY_H__
#define __NRF_CHARACTERISTIC_DESCRIPTOR_DISCOVERY_H__

#include "ble/Gap.h"
#include "ble/DiscoveredCharacteristic.h"
#include "ble/CharacteristicDescriptorDiscovery.h"
#include "ble/GattClient.h"
#include "ble_gattc.h"

/**
 * @brief Manage the discovery of Characteristic descriptors 
 * @details is a bridge beetween BLE API and nordic stack regarding Characteristic
 * Descriptor discovery. The BLE API can launch, monitorate and ask for termination
 * of a discovery. The nordic stack will provide new descriptors and indicate when
 * the discovery is done
 */
class nRF5xCharacteristicDescriptorDiscoverer
{
    typedef CharacteristicDescriptorDiscovery::DiscoveryCallback_t DiscoveryCallback_t;
    typedef CharacteristicDescriptorDiscovery::TerminationCallback_t TerminationCallback_t;

public:
    nRF5xCharacteristicDescriptorDiscoverer(size_t concurrentConnectionsCount = 3);

    ~nRF5xCharacteristicDescriptorDiscoverer();

    /**
     * Launch a new characteristic descriptor discovery for a given 
     * DiscoveredCharacteristic.
     * @note: this will be called by BLE API side
     */
    ble_error_t launch(
        const DiscoveredCharacteristic& characteristic, 
        const DiscoveryCallback_t& callback,
        const TerminationCallback_t& terminationCallback
    );

    /**
     * @brief indicate if a characteristic descriptor discovery is active for a 
     * given DiscoveredCharacteristic
     * @note: this will be called by BLE API side
     */
    bool isActive(const DiscoveredCharacteristic& characteristic) const;

    /**
     * @brief reauest the termination of characteristic descriptor discovery 
     * for a give DiscoveredCharacteristic
     * @note: this will be called by BLE API side
     */
    void requestTerminate(const DiscoveredCharacteristic& characteristic);

    /**
     * @brief process descriptors discovered from the nordic stack
     */
    void process(uint16_t handle, const ble_gattc_evt_desc_disc_rsp_t& descriptors);

    /**
     * @brief Called by the nordic stack when the discovery is over.
     */
    void terminate(uint16_t handle, ble_error_t err);

private:
    nRF5xCharacteristicDescriptorDiscoverer(const nRF5xCharacteristicDescriptorDiscoverer&);
    nRF5xCharacteristicDescriptorDiscoverer& operator=(const nRF5xCharacteristicDescriptorDiscoverer&);

    struct Discovery {
        Discovery() : characteristic(), onDiscovery(), onTerminate() { }

        Discovery(const DiscoveredCharacteristic& c, const DiscoveryCallback_t& dCb, const TerminationCallback_t& tCb) : 
            characteristic(c), 
            onDiscovery(dCb), 
            onTerminate(tCb) { 
        }

        DiscoveredCharacteristic characteristic;
        DiscoveryCallback_t onDiscovery;
        TerminationCallback_t onTerminate;

        void process(GattAttribute::Handle_t handle, const UUID& uuid) { 
            CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t params = { 
                characteristic,
                DiscoveredCharacteristicDescriptor(
                    characteristic.getGattClient(),
                    characteristic.getConnectionHandle(),
                    handle,
                    uuid
                )
            };
            onDiscovery.call(&params);
        }

        void terminate(ble_error_t err) { 
            CharacteristicDescriptorDiscovery::TerminationCallbackParams_t params = {
                characteristic,
                err
            };
            onTerminate.call(&params);
        }

        friend bool operator==(const Discovery& lhs, const Discovery& rhs) {
            return lhs.characteristic == rhs.characteristic &&
                lhs.onDiscovery == rhs.onDiscovery &&
                lhs.onTerminate == rhs.onTerminate;
        }

        friend bool operator!=(const Discovery& lhs, const Discovery& rhs) {
            return !(lhs == rhs);
        }
    };

    Discovery* findRunningDiscovery(const DiscoveredCharacteristic& characteristic);
    Discovery* findRunningDiscovery(const DiscoveredCharacteristic& characteristic) const;
    Discovery* findRunningDiscovery(uint16_t handle);
    void removeDiscovery(Discovery* discovery);
    Discovery* getAvailableDiscoverySlot(); 
    bool isConnectionInUse(uint16_t connHandle);
    static ble_error_t gattc_descriptors_discover(uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle); 


    size_t maximumConcurrentConnectionsCount;
    Discovery *discoveryRunning;
};

#endif /*__NRF_CHARACTERISTIC_DESCRIPTOR_DISCOVERY_H__*/
