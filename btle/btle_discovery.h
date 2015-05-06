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

#ifndef _BTLE_DISCOVERY_H_
#define _BTLE_DISCOVERY_H_

#include <stdio.h>
#include "ble.h"
#include "UUID.h"
#include "Gap.h"

void bleGattcEventHandler(const ble_evt_t *p_ble_evt);

/**@brief Structure for holding information about the service and the characteristics found during
 *        the discovery process.
 */
struct DiscoveredService {
    void setup(ShortUUIDBytes_t uuidIn, Gap::Handle_t start, Gap::Handle_t end) {
        uuid        = uuidIn;
        startHandle = start;
        endHandle   = end;
    }

    ShortUUIDBytes_t uuid;        /**< UUID of the service. */
    Gap::Handle_t    startHandle; /**< Service Handle Range. */
    Gap::Handle_t    endHandle;   /**< Service Handle Range. */
};

/**@brief Structure for holding information about the service and the characteristics found during
 *        the discovery process.
 */
struct DiscoveredCharacteristic {
    struct Properties_t {
        static const uint8_t BROADCAST_PROPERTY_MASK         = 0x01;
        static const uint8_t READ_PROPERTY_MASK              = 0x02;
        static const uint8_t WRITE_WO_RESPONSE_PROPERTY_MASK = 0x04;
        static const uint8_t WRITE_PROPERTY_MASK             = 0x08;
        static const uint8_t NOTIFY_PROPERTY_MASK            = 0x10;
        static const uint8_t INDICATE_PROPERTY_MASK          = 0x20;
        static const uint8_t AUTH_SIGNED_PROPERTY_MASK       = 0x40;

        Properties_t() : broadcast(0), read(0), write_wo_resp(0), write(0), notify(0), indicate(0), auth_signed_wr(0) {
            /* empty */
        }

        Properties_t(uint8_t props) :
            broadcast(props & BROADCAST_PROPERTY_MASK),
            read(props & READ_PROPERTY_MASK),
            write_wo_resp(props & WRITE_WO_RESPONSE_PROPERTY_MASK),
            write(props & WRITE_PROPERTY_MASK),
            notify(props & NOTIFY_PROPERTY_MASK),
            indicate(props & INDICATE_PROPERTY_MASK),
            auth_signed_wr(props & AUTH_SIGNED_PROPERTY_MASK) {
            /* empty*/
        }

        uint8_t broadcast       :1; /**< Broadcasting of the value permitted. */
        uint8_t read            :1; /**< Reading the value permitted. */
        uint8_t write_wo_resp   :1; /**< Writing the value with Write Command permitted. */
        uint8_t write           :1; /**< Writing the value with Write Request permitted. */
        uint8_t notify          :1; /**< Notications of the value permitted. */
        uint8_t indicate        :1; /**< Indications of the value permitted. */
        uint8_t auth_signed_wr  :1; /**< Writing the value with Signed Write Command permitted. */
    };

    void setup(ShortUUIDBytes_t uuidIn, Properties_t propsIn, Gap::Handle_t declHandleIn, Gap::Handle_t valueHandleIn) {
        uuid        = uuidIn;
        props       = propsIn;
        declHandle  = declHandleIn;
        valueHandle = valueHandleIn;
    }

    ShortUUIDBytes_t uuid;
    Properties_t     props;
    Gap::Handle_t    declHandle;
    Gap::Handle_t    valueHandle;
};

class ServiceDiscovery {
public:
    typedef void (*ServiceCallback_t)(void);
    typedef void (*CharacteristicCallback_t)(void);

public:
    static ble_error_t launch(Gap::Handle_t            connectionHandle,
                              ServiceCallback_t        sc,
                              CharacteristicCallback_t cc = NULL);
    static ble_error_t launch(Gap::Handle_t            connectionHandle,
                              UUID                     matchingServiceUUID,
                              ServiceCallback_t        sc,
                              UUID                     matchingCharacteristicUUID = ShortUUIDBytes_t(BLE_UUID_UNKNOWN),
                              CharacteristicCallback_t cc = NULL);

    static void        terminate(void);

protected:
    Gap::Handle_t            connHandle; /**< Connection handle as provided by the SoftDevice. */
    ServiceCallback_t        serviceCallback;
    CharacteristicCallback_t characteristicCallback;
};

class NordicServiceDiscovery : public ServiceDiscovery {
public:
    static const unsigned BLE_DB_DISCOVERY_MAX_SRV          = 4;      /**< Maximum number of services we can retain information for after a single discovery. */
    static const unsigned BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV = 4;      /**< Maximum number of characteristics per service we can retain information for. */
    static const uint16_t SRV_DISC_START_HANDLE             = 0x0001; /**< The start handle value used during service discovery. */

public:
    void setupDiscoveredServices(const ble_gattc_evt_prim_srvc_disc_rsp_t *response);
    void setupDiscoveredCharacteristics(const ble_gattc_evt_char_disc_rsp_t *response);

    void terminateServiceDiscovery(void) {
        sDiscoveryActive = false;
        cDiscoveryActive = false;
        printf("end of service discovery\r\n");
    }

    void terminateCharacteristicDiscovery(void) {
        cDiscoveryActive = false;
        sDiscoveryActive = true;
        serviceIndex++; /* Progress service index to keep discovery alive. */
    }

    void resetDiscoveredServices(void) {
        numServices  = 0;
        serviceIndex = 0;
        memset(services, 0, sizeof(DiscoveredService) * BLE_DB_DISCOVERY_MAX_SRV);
    }

protected:
    void resetDiscoveredCharacteristics(void) {
        numCharacteristics  = 0;
        characteristicIndex = 0;
        memset(characteristics, 0, sizeof(DiscoveredCharacteristic) * BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV);
    }

public:
    void serviceDiscoveryStarted(Gap::Handle_t connectionHandle) {
        connHandle       = connectionHandle;
        resetDiscoveredServices();
        sDiscoveryActive = true;
        cDiscoveryActive = false;
    }

protected:
    void characteristicDiscoveryStarted(Gap::Handle_t connectionHandle) {
        connHandle       = connectionHandle;
        resetDiscoveredCharacteristics();
        cDiscoveryActive = true;
        sDiscoveryActive = false;
    }

public:
    ble_error_t launchCharacteristicDiscovery(Gap::Handle_t connectionHandle, Gap::Handle_t startHandle, Gap::Handle_t endHandle);

public:
    void progressCharacteristicDiscovery() {
        while (cDiscoveryActive && (characteristicIndex < numCharacteristics)) {
            /* THIS IS WHERE THE CALLBACK WILL GO */
            printf("%x [%u]\r\n", characteristics[characteristicIndex].uuid, characteristics[characteristicIndex].valueHandle);

            characteristicIndex++;
        }

        if (cDiscoveryActive) {
            Gap::Handle_t startHandle = characteristics[characteristicIndex - 1].valueHandle + 1;
            Gap::Handle_t endHandle   = services[serviceIndex].endHandle;
            resetDiscoveredCharacteristics();

            if (startHandle < endHandle) {
                ble_gattc_handle_range_t handleRange = {
                    .start_handle = startHandle,
                    .end_handle   = endHandle
                };
                printf("char discovery returned %u\r\n", sd_ble_gattc_characteristics_discover(connHandle, &handleRange));
            } else {
               terminateCharacteristicDiscovery();
            }
        }
    }

    void progressServiceDiscovery() {
        while (sDiscoveryActive && (serviceIndex < numServices)) {
            /* THIS IS WHERE THE CALLBACK WILL GO */
            printf("%x [%u %u]\r\n", services[serviceIndex].uuid, services[serviceIndex].startHandle, services[serviceIndex].endHandle);

            if (true) { /* characteristic discovery is optional. */
                launchCharacteristicDiscovery(connHandle, services[serviceIndex].startHandle, services[serviceIndex].endHandle);
            } else {
                serviceIndex++; /* Progress service index to keep discovery alive. */
            }
        }
        if (sDiscoveryActive && (numServices > 0) && (serviceIndex > 0)) {
            Gap::Handle_t endHandle = services[serviceIndex - 1].endHandle;
            resetDiscoveredServices();

            printf("services discover returned %u\r\n", sd_ble_gattc_primary_services_discover(connHandle, endHandle, NULL));
        }
    }

private:
    uint8_t  serviceIndex;        /**< Index of the current service being discovered. This is intended for internal use during service discovery.*/
    uint8_t  numServices;         /**< Number of services at the peers GATT database.*/
    uint8_t  characteristicIndex; /**< Index of the current characteristic being discovered. This is intended for internal use during service discovery.*/
    uint8_t  numCharacteristics;  /**< Number of characteristics within the service.*/

    bool     sDiscoveryActive;
    bool     cDiscoveryActive;

    DiscoveredService        services[BLE_DB_DISCOVERY_MAX_SRV];  /**< Information related to the current service being discovered.
                                                                   *  This is intended for internal use during service discovery. */
    DiscoveredCharacteristic characteristics[BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV];
};

#endif /*_BTLE_DISCOVERY_H_*/
