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

#include "DiscoveredCharacteristic.h"
#include "ble_gatt.h"

class nRF51GattClient; /* forward declaration */

class nRFDiscoveredCharacteristic : public DiscoveredCharacteristic {
public:
    void setup(nRF51GattClient         *gattcIn,
               Gap::Handle_t            connectionHandleIn,
               ble_gatt_char_props_t    propsIn,
               GattAttribute::Handle_t  declHandleIn,
               GattAttribute::Handle_t  valueHandleIn);

    void setup(nRF51GattClient         *gattcIn,
               Gap::Handle_t            connectionHandleIn,
               UUID::ShortUUIDBytes_t   uuidIn,
               ble_gatt_char_props_t    propsIn,
               GattAttribute::Handle_t  declHandleIn,
               GattAttribute::Handle_t  valueHandleIn);

#if 0
public:
    /**
     * Initiate (or continue) a read for the value attribute, optionally at a
     * given offset. If the Characteristic or Descriptor to be read is longer
     * than ATT_MTU - 1, this function must be called multiple times with
     * appropriate offset to read the complete value.
     *
     * @return BLE_ERROR_NONE if a read has been initiated, else
     *         BLE_ERROR_INVALID_STATE if some internal state about the connection is invalid, or
     *         BLE_STACK_BUSY if some client procedure already in progress, or
     *         BLE_ERROR_OPERATION_NOT_PERMITTED due to the characteristic's properties.
     */
    virtual ble_error_t read(uint16_t offset = 0) const {

        printf("in nRFDiscoveredCharacteristic::read\r\n");
        ble_error_t err = DiscoveredCharacteristic::read(offset);
        if (err != BLE_ERROR_NONE) {
            return err;
        }

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

    /**
     * Perform a write without response procedure.
     *
     * @param  length
     *           The amount of data being written.
     * @param  value
     *           The bytes being written.
     *
     * @note    It is important to note that a write without response will
     *          <b>consume an application buffer</b>, and will therefore
     *          generate a onDataWritten() callback when the packet has been
     *          transmitted. The application should ensure that the buffer is
     *          valid until the callback.
     *
     * @retval BLE_ERROR_NONE Successfully started the Write procedure, else
     *         BLE_ERROR_INVALID_STATE if some internal state about the connection is invalid, or
     *         BLE_STACK_BUSY if some client procedure already in progress, or
     *         BLE_ERROR_NO_MEM if there are no available buffers left to process the request or
     *         BLE_ERROR_OPERATION_NOT_PERMITTED due to the characteristic's properties.
     */
    virtual ble_error_t writeWoResponse(uint16_t length, const uint8_t *value) const {
        ble_error_t err = DiscoveredCharacteristic::writeWoResponse(length, value);
        if (err != BLE_ERROR_NONE) {
            return err;
        }

        ble_gattc_write_params_t writeParams = {
            .write_op = BLE_GATT_OP_WRITE_CMD,
            // .flags  = 0,
            .handle   = valueHandle,
            .offset   = 0,
            .len      = length,
            .p_value  = const_cast<uint8_t *>(value),
        };

        uint32_t rc = sd_ble_gattc_write(connHandle, &writeParams);
        if (rc == NRF_SUCCESS) {
            return BLE_ERROR_NONE;
        }
        switch (rc) {
            case NRF_ERROR_BUSY:
                return BLE_STACK_BUSY;
            case BLE_ERROR_NO_TX_BUFFERS:
                return BLE_ERROR_NO_MEM;
            case BLE_ERROR_INVALID_CONN_HANDLE:
            case NRF_ERROR_INVALID_STATE:
            case NRF_ERROR_INVALID_ADDR:
            default:
                return BLE_ERROR_INVALID_STATE;
        }
    }
#endif
};

#endif /* __NRF_DISCOVERED_CHARACTERISTIC_H__ */
