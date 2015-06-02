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

#ifndef __NRF_DISCOVERED_CHARACTERISTIC_H__
#define __NRF_DISCOVERED_CHARACTERISTIC_H__

class nRFDiscoveredCharacteristic : public DiscoveredCharacteristic {
public:
    void setup(Gap::Handle_t           connectionHandleIn,
               Properties_t            propsIn,
               GattAttribute::Handle_t declHandleIn,
               GattAttribute::Handle_t valueHandleIn) {
        connHandle  = connectionHandleIn;
        props       = propsIn;
        declHandle  = declHandleIn;
        valueHandle = valueHandleIn;
    }

    void setup(Gap::Handle_t connectionHandleIn,
               UUID::ShortUUIDBytes_t  uuidIn,
               Properties_t            propsIn,
               GattAttribute::Handle_t declHandleIn,
               GattAttribute::Handle_t valueHandleIn) {
        connHandle  = connectionHandleIn;
        uuid        = uuidIn;
        props       = propsIn;
        declHandle  = declHandleIn;
        valueHandle = valueHandleIn;
    }

public:
    /**
     * Initiate (or continue) a read for the value attribute, optionally at a
     * given offset. If the Characteristic or Descriptor to be read is longer
     * than ATT_MTU - 1, this function must be called multiple times with
     * appropriate offset to read the complete value.
     *
     * @return BLE_ERROR_NONE if a read has been initiated, else
     *         BLE_ERROR_INVALID_STATE if some internal state about the connection is invalid, or
     *         BLE_STACK_BUSY if some client procedure already in progress.
     */
    virtual ble_error_t read(uint16_t offset = 0) const {
        uint32_t rc = sd_ble_gattc_read(connHandle, valueHandle, offset);
        if (rc == NRF_SUCCESS) {
            return BLE_ERROR_NONE;
        }
        switch (rc) {
            case NRF_ERROR_BUSY:
                return BLE_STACK_BUSY;
            case BLE_ERROR_INVALID_CONN_HANDLE:
            case NRF_ERROR_INVALID_STATE:
            case NRF_ERROR_INVALID_ADDR:
            default:
                return BLE_ERROR_INVALID_STATE;
        }
    }
};

#endif /* __NRF_DISCOVERED_CHARACTERISTIC_H__ */
