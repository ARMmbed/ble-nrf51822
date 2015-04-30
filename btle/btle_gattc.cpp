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

#include "btle_gattc.h"
#include "UUID.h"

#define BLE_DB_DISCOVERY_MAX_SRV          4  /**< Maximum number of services supported by this module. This also indicates the maximum number of users allowed to be registered to this module. (one user per service). */
#define BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV 4  /**< Maximum number of characteristics per service supported by this module. */

#define SRV_DISC_START_HANDLE  0x0001                    /**< The start handle value used during service discovery. */


/**@brief Structure for holding information about the service and the characteristics found during
 *        the discovery process.
 */
struct DiscoveredService {
    DiscoveredService() {
        /* empty */
    }
    DiscoveredService(ShortUUIDBytes_t uuidIn, Gap::Handle_t start, Gap::Handle_t end) {
        setup(uuidIn, start, end);
    }

    void setup(ShortUUIDBytes_t uuidIn, Gap::Handle_t start, Gap::Handle_t end) {
        uuid        = uuidIn;
        startHandle = start;
        endHandle   = end;
    }

    ShortUUIDBytes_t         uuid;                                           /**< UUID of the service. */
    // uint8_t                  char_count;                                         /**< Number of characteristics present in the service. */
    // ble_db_discovery_char_t  charateristics[BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV];  /**< Array of information related to the characteristics present in the service. */
    Gap::Handle_t startHandle;    /**< Service Handle Range. */
    Gap::Handle_t endHandle;      /**< Service Handle Range. */
};

struct ble_db_discovery_t {
    DiscoveredService services[BLE_DB_DISCOVERY_MAX_SRV];  /**< Information related to the current service being discovered. This is intended for internal use during service discovery.*/
    uint16_t               connHandle;                 /**< Connection handle as provided by the SoftDevice. */
    uint8_t                srvCount;                   /**< Number of services at the peers GATT database.*/
    // uint8_t                currCharInd;             /**< Index of the current characteristic being discovered. This is intended for internal use during service discovery.*/
    uint8_t                currSrvInd;                 /**< Index of the current service being discovered. This is intended for internal use during service discovery.*/
    bool                   serviceDiscoveryInProgress;
    bool                   characteristicDiscoveryInProgress;
};

static ble_db_discovery_t  discoveryStatus;

void launchServiceDiscovery(Gap::Handle_t connectionHandle)
{
    discoveryStatus.connHandle                        = connectionHandle;
    discoveryStatus.srvCount                          = 0;
    discoveryStatus.currSrvInd                        = 0;

    discoveryStatus.serviceDiscoveryInProgress        = true;
    discoveryStatus.characteristicDiscoveryInProgress = false;
    printf("launch service discovery returned %u\r\n", sd_ble_gattc_primary_services_discover(connectionHandle, SRV_DISC_START_HANDLE, NULL));
}

void launchCharacteristicDiscovery(Gap::Handle_t connectionHandle, Gap::Handle_t startHandle, Gap::Handle_t endHandle) {
    /* TODO */
}

void bleGattcEventHandler(const ble_evt_t *p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                case BLE_GATT_STATUS_SUCCESS: {
                    printf("count of primary services: %u\r\n", p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.count);
                    discoveryStatus.connHandle = p_ble_evt->evt.gattc_evt.conn_handle;
                    discoveryStatus.currSrvInd = 0;
                    discoveryStatus.srvCount   = p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.count;

                    for (unsigned serviceIndex = 0; serviceIndex < discoveryStatus.srvCount; serviceIndex++) {
                        discoveryStatus.services[serviceIndex].
                            setup(p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[serviceIndex].uuid.uuid,
                                  p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[serviceIndex].handle_range.start_handle,
                                  p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[serviceIndex].handle_range.end_handle);
                    }
                    break;
                }

                case BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND: {
                    discoveryStatus.serviceDiscoveryInProgress = false;
                    printf("end of service discovery\r\n");
                    break;
                }

                default: {
                    discoveryStatus.serviceDiscoveryInProgress = false;
                    printf("gatt failure status: %u\r\n", p_ble_evt->evt.gattc_evt.gatt_status);
                    break;
                }
            }
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP: {
            switch (p_ble_evt->evt.gattc_evt.gatt_status) {
                default:
                    printf("gatt failure status: %u\r\n", p_ble_evt->evt.gattc_evt.gatt_status);
                    break;
            }
            break;
        }
    }

    while (discoveryStatus.serviceDiscoveryInProgress && (discoveryStatus.currSrvInd < discoveryStatus.srvCount)) {
        printf("%x [%u %u]\r\n",
            p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[discoveryStatus.currSrvInd].uuid.uuid,
            p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[discoveryStatus.currSrvInd].handle_range.start_handle,
            p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[discoveryStatus.currSrvInd].handle_range.end_handle);

        // ble_gattc_handle_range_t handleRange = {
        //     .start_handle = p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[discoveryStatus.currSrvInd].handle_range.start_handle,
        //     .end_handle   = p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[discoveryStatus.currSrvInd].handle_range.end_handle
        // };
        // printf("characteristics_discover returned %u\r\n",
        //        sd_ble_gattc_characteristics_discover(p_ble_evt->evt.gattc_evt.conn_handle, &handleRange));

        discoveryStatus.currSrvInd++;
    }
    if (discoveryStatus.serviceDiscoveryInProgress && (discoveryStatus.srvCount > 0)) {
        printf("services discover returned %u\r\n",
            sd_ble_gattc_primary_services_discover(p_ble_evt->evt.gattc_evt.conn_handle,
                                                   p_ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp.services[discoveryStatus.currSrvInd -1].handle_range.end_handle,
                                                   NULL));
    }
}
