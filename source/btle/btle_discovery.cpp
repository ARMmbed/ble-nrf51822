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

#include "nRF5xServiceDiscovery.h"
#include "nRF5xGattClient.h"

#if !defined(TARGET_MCU_NRF51_16K_S110) && !defined(TARGET_MCU_NRF51_32K_S110)
void bleGattcEventHandler(const ble_evt_t *p_ble_evt)
{
    nRF5xServiceDiscovery &sdSingleton = nRF5xGattClient::getInstance().discovery;

    switch (p_ble_evt->header.evt_id) {
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS:
                    sdSingleton.setupDiscoveredServices(&p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp);
                    break;

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND:
                default:
                    sdSingleton.terminate();
                    break;
            }
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS:
                    sdSingleton.setupDiscoveredCharacteristics(&p_ble_evt->evt.gattc_evt.params.char_disc_rsp);
                    break;

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND:
                default:
                    sdSingleton.terminateCharacteristicDiscovery();
                    break;
            }
            break;

        case BLE_GATTC_EVT_CHAR_VAL_BY_UUID_READ_RSP:
            if (sdSingleton.isActive()) {
                sdSingleton.processDiscoverUUIDResponse(&p_ble_evt->evt.gattc_evt.params.char_val_by_uuid_read_rsp);
            }
            break;

        case BLE_GATTC_EVT_READ_RSP: {
                GattReadCallbackParams response = {
                    .connHandle = p_ble_evt->evt.gattc_evt.conn_handle,
                    .handle     = p_ble_evt->evt.gattc_evt.params.read_rsp.handle,
                    .offset     = p_ble_evt->evt.gattc_evt.params.read_rsp.offset,
                    .len        = p_ble_evt->evt.gattc_evt.params.read_rsp.len,
                    .data       = p_ble_evt->evt.gattc_evt.params.read_rsp.data,
                };
                nRF5xGattClient::getInstance().processReadResponse(&response);
            }
            break;

        case BLE_GATTC_EVT_WRITE_RSP: {
                GattWriteCallbackParams response = {
                    .connHandle = p_ble_evt->evt.gattc_evt.conn_handle,
                    .handle     = p_ble_evt->evt.gattc_evt.params.write_rsp.handle,
                    .writeOp    = (GattWriteCallbackParams::WriteOp_t)(p_ble_evt->evt.gattc_evt.params.write_rsp.write_op),
                    .offset     = p_ble_evt->evt.gattc_evt.params.write_rsp.offset,
                    .len        = p_ble_evt->evt.gattc_evt.params.write_rsp.len,
                    .data       = p_ble_evt->evt.gattc_evt.params.write_rsp.data,
                };
                nRF5xGattClient::getInstance().processWriteResponse(&response);
            }
            break;

        case BLE_GATTC_EVT_HVX: {
                GattHVXCallbackParams params;
                params.connHandle = p_ble_evt->evt.gattc_evt.conn_handle;
                params.handle     = p_ble_evt->evt.gattc_evt.params.hvx.handle;
                params.type       = static_cast<HVXType_t>(p_ble_evt->evt.gattc_evt.params.hvx.type);
                params.len        = p_ble_evt->evt.gattc_evt.params.hvx.len;
                params.data       = p_ble_evt->evt.gattc_evt.params.hvx.data;

                nRF5xGattClient::getInstance().processHVXEvent(&params);
            }
            break;
    }

    sdSingleton.progressCharacteristicDiscovery();
    sdSingleton.progressServiceDiscovery();
}
#endif

