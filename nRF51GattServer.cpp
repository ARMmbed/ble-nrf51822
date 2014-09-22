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

#include "nRF51GattServer.h"
#include "mbed.h"

#include "common/common.h"
#include "btle/custom/custom_helper.h"

#include "nRF51Gap.h"

/**************************************************************************/
/*!
    @brief  Adds a new service to the GATT table on the peripheral

    @returns    ble_error_t

    @retval     BLE_ERROR_NONE
                Everything executed properly

    @section EXAMPLE

    @code

    @endcode
*/
/**************************************************************************/
ble_error_t nRF51GattServer::addService(GattService &service)
{
    /* ToDo: Make sure we don't overflow the array, etc. */
    /* ToDo: Make sure this service UUID doesn't already exist (?) */
    /* ToDo: Basic validation */

    /* Add the service to the nRF51 */
    ble_uuid_t nordicUUID;
    nordicUUID = custom_convert_to_nordic_uuid(service.getUUID());

    uint16_t serviceHandle;
    ASSERT( ERROR_NONE ==
            sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                     &nordicUUID,
                                     &serviceHandle),
            BLE_ERROR_PARAM_OUT_OF_RANGE );
    service.setHandle(serviceHandle);

    /* Add characteristics to the service */
    for (uint8_t i = 0; i < service.getCharacteristicCount(); i++) {
        GattCharacteristic *p_char = service.getCharacteristic(i);

        /* Skip any incompletely defined, read-only characteristics. */
        if ((p_char->getValueAttribute().getValuePtr() == NULL) &&
            (p_char->getValueAttribute().getInitialLength() == 0) &&
            (p_char->getProperties() == GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ)) {
            continue;
        }

        nordicUUID = custom_convert_to_nordic_uuid(p_char->getValueAttribute().getUUID());

        ASSERT ( ERROR_NONE ==
                 custom_add_in_characteristic(BLE_GATT_HANDLE_INVALID,
                                              &nordicUUID,
                                              p_char->getProperties(),
                                              p_char->getValueAttribute().getValuePtr(),
                                              p_char->getValueAttribute().getInitialLength(),
                                              p_char->getValueAttribute().getMaxLength(),
                                              &nrfCharacteristicHandles[characteristicCount]),
                 BLE_ERROR_PARAM_OUT_OF_RANGE );

        /* Update the characteristic handle */
        uint16_t charHandle = characteristicCount;
        p_characteristics[characteristicCount++] = p_char;

        p_char->getValueAttribute().setHandle(charHandle);

        /* Add optional descriptors if any */
        /* ToDo: Make sure we don't overflow the array */
        for (uint8_t j = 0; j < p_char->getDescriptorCount(); j++) {
             GattAttribute *p_desc = p_char->getDescriptor(j);

             nordicUUID = custom_convert_to_nordic_uuid(p_desc->getUUID());

             ASSERT ( ERROR_NONE ==
                      custom_add_in_descriptor(BLE_GATT_HANDLE_INVALID,
                                               &nordicUUID,
                                               p_desc->getValuePtr(),
                                               p_desc->getInitialLength(),
                                               p_desc->getMaxLength(),
                                               &nrfDescriptorHandles[descriptorCount]),
                 BLE_ERROR_PARAM_OUT_OF_RANGE );

            uint16_t descHandle = descriptorCount;
            p_descriptors[descriptorCount++] = p_desc;
            p_desc->setHandle(descHandle);
        }
    }

    serviceCount++;

    return BLE_ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief  Reads the value of a characteristic, based on the service
            and characteristic index fields

    @param[in]  charHandle
                The handle of the GattCharacteristic to read from
    @param[in]  buffer
                Buffer to hold the the characteristic's value
                (raw byte array in LSB format)
    @param[in]  len
                The number of bytes read into the buffer

    @returns    ble_error_t

    @retval     BLE_ERROR_NONE
                Everything executed properly

    @section EXAMPLE

    @code

    @endcode
*/
/**************************************************************************/
ble_error_t nRF51GattServer::readValue(uint16_t charHandle, uint8_t buffer[], uint16_t *const lengthP)
{
    ASSERT( ERROR_NONE ==
            sd_ble_gatts_value_get(nrfCharacteristicHandles[charHandle].value_handle, 0, lengthP, buffer),
            BLE_ERROR_PARAM_OUT_OF_RANGE);

    return BLE_ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief  Updates the value of a characteristic, based on the service
            and characteristic index fields

    @param[in]  charHandle
                The handle of the GattCharacteristic to write to
    @param[in]  buffer
                Data to use when updating the characteristic's value
                (raw byte array in LSB format)
    @param[in]  len
                The number of bytes in buffer

    @returns    ble_error_t

    @retval     BLE_ERROR_NONE
                Everything executed properly

    @section EXAMPLE

    @code

    @endcode
*/
/**************************************************************************/
ble_error_t nRF51GattServer::updateValue(uint16_t charHandle, uint8_t buffer[], uint16_t len, bool localOnly)
{
    uint16_t gapConnectionHandle = nRF51Gap::getInstance().getConnectionHandle();

    if (localOnly) {
        /* Only update locally regardless of notify/indicate */
        ASSERT_INT( ERROR_NONE,
                    sd_ble_gatts_value_set(nrfCharacteristicHandles[charHandle].value_handle, 0, &len, buffer),
                    BLE_ERROR_PARAM_OUT_OF_RANGE );
		return BLE_ERROR_NONE;
    }

    if ((p_characteristics[charHandle]->getProperties() &
         (GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE |
          GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)) &&
        (gapConnectionHandle != BLE_CONN_HANDLE_INVALID)) {
        /* HVX update for the characteristic value */
        ble_gatts_hvx_params_t hvx_params;

        hvx_params.handle = nrfCharacteristicHandles[charHandle].value_handle;
        hvx_params.type   =
            (p_characteristics[charHandle]->getProperties() &
             GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)  ?
            BLE_GATT_HVX_NOTIFICATION : BLE_GATT_HVX_INDICATION;
        hvx_params.offset = 0;
        hvx_params.p_data = buffer;
        hvx_params.p_len  = &len;

        error_t error = (error_t) sd_ble_gatts_hvx(gapConnectionHandle, &hvx_params);

        /* ERROR_INVALID_STATE, ERROR_BUSY, ERROR_GATTS_SYS_ATTR_MISSING and
         *ERROR_NO_TX_BUFFERS the ATT table has been updated. */
        if ((error != ERROR_NONE) && (error != ERROR_INVALID_STATE) &&
                (error != ERROR_BLE_NO_TX_BUFFERS) && (error != ERROR_BUSY) &&
                (error != ERROR_BLEGATTS_SYS_ATTR_MISSING)) {
            ASSERT_INT( ERROR_NONE,
                        sd_ble_gatts_value_set(nrfCharacteristicHandles[charHandle].value_handle, 0, &len, buffer),
                        BLE_ERROR_PARAM_OUT_OF_RANGE );
        }
    } else {
        ASSERT_INT( ERROR_NONE,
                    sd_ble_gatts_value_set(nrfCharacteristicHandles[charHandle].value_handle, 0, &len, buffer),
                    BLE_ERROR_PARAM_OUT_OF_RANGE );
    }

    return BLE_ERROR_NONE;
}

/**************************************************************************/
/*!
    @brief  Callback handler for events getting pushed up from the SD
*/
/**************************************************************************/
void nRF51GattServer::hwCallback(ble_evt_t *p_ble_evt)
{
    uint16_t                       handle_value;
    GattServerEvents::gattEvent_t  eventType;
    const ble_gatts_evt_t         *gattsEventP = &p_ble_evt->evt.gatts_evt;

    switch (p_ble_evt->header.evt_id) {
        case BLE_GATTS_EVT_WRITE:
            /* There are 2 use case here: Values being updated & CCCD (indicate/notify) enabled */

            /* 1.) Handle CCCD changes */
            handle_value = gattsEventP->params.write.handle;
            for (uint8_t i = 0; i<characteristicCount; i++) {
                if ((p_characteristics[i]->getProperties() & (GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)) &&
                    (nrfCharacteristicHandles[i].cccd_handle == handle_value)) {
                    uint16_t cccd_value =
                        (gattsEventP->params.write.data[1] << 8) |
                        gattsEventP->params.write.data[0]; /* Little Endian but M0 may be mis-aligned */

                    if (((p_characteristics[i]->getProperties() & GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE) && (cccd_value & BLE_GATT_HVX_INDICATION)) ||
                        ((p_characteristics[i]->getProperties() & GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY) && (cccd_value & BLE_GATT_HVX_NOTIFICATION))) {
                        eventType = GattServerEvents::GATT_EVENT_UPDATES_ENABLED;
                    } else {
                        eventType = GattServerEvents::GATT_EVENT_UPDATES_DISABLED;
                    }

                    handleEvent(eventType, i);
                    return;
                }
            }

            /* 2.) Changes to the characteristic value will be handled with other events below */
            eventType = GattServerEvents::GATT_EVENT_DATA_WRITTEN;
            break;

        case BLE_GATTS_EVT_HVC:
            /* Indication confirmation received */
            eventType    = GattServerEvents::GATT_EVENT_CONFIRMATION_RECEIVED;
            handle_value = gattsEventP->params.hvc.handle;
            break;

        case BLE_EVT_TX_COMPLETE: {
            handleDataSentEvent(p_ble_evt->evt.common_evt.params.tx_complete.count);
            return;
        }

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            sd_ble_gatts_sys_attr_set(gattsEventP->conn_handle, NULL, 0);
            return;

        default:
            return;
    }

    /* Find index (charHandle) in the pool */
    for (uint8_t i = 0; i<characteristicCount; i++) {
        if (nrfCharacteristicHandles[i].value_handle == handle_value) {
            switch (eventType) {
                case GattServerEvents::GATT_EVENT_DATA_WRITTEN: {
                    GattCharacteristicWriteCBParams cbParams = {
                        .charHandle = i,
                        .op     = static_cast<GattCharacteristicWriteCBParams::Type>(gattsEventP->params.write.op),
                        .offset = gattsEventP->params.write.offset,
                        .len    = gattsEventP->params.write.len,
                        .data   = gattsEventP->params.write.data
                    };
                    handleDataWrittenEvent(&cbParams);
                    break;
                }
                default:
                    handleEvent(eventType, i);
                    break;
            }
        }
    }
}
