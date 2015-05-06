#include "blecommon.h"
#include "UUID.h"
#include "Gap.h"
#include "nrf_error.h"
#include "btle_discovery.h"
#include "ble_err.h"

ble_error_t
ServiceDiscovery::launch(Gap::Handle_t connectionHandle, ServiceCallback_t sc, CharacteristicCallback_t cc)
{
    ServiceDiscovery *singleton = getSingleton();
    singleton->serviceDiscoveryStarted(connectionHandle);

    uint32_t rc;
    if ((rc = sd_ble_gattc_primary_services_discover(connectionHandle, SRV_DISC_START_HANDLE, NULL)) != NRF_SUCCESS) {
        singleton->terminate();
        switch (rc) {
            case NRF_ERROR_INVALID_PARAM:
            case BLE_ERROR_INVALID_CONN_HANDLE:
                return BLE_ERROR_INVALID_PARAM;
            case NRF_ERROR_BUSY:
                return BLE_STACK_BUSY;
            default:
            case NRF_ERROR_INVALID_STATE:
                return BLE_ERROR_INVALID_STATE;
        }
    }

    return BLE_ERROR_NONE;
}

ble_error_t ServiceDiscovery::launchCharacteristicDiscovery(Gap::Handle_t connectionHandle, Gap::Handle_t startHandle, Gap::Handle_t endHandle) {
    ServiceDiscovery *singleton = getSingleton();
    singleton->characteristicDiscoveryStarted(connectionHandle);

    ble_gattc_handle_range_t handleRange = {
        .start_handle = startHandle,
        .end_handle   = endHandle
    };
    uint32_t rc;
    if ((rc = sd_ble_gattc_characteristics_discover(connectionHandle, &handleRange)) != NRF_SUCCESS) {
        singleton->terminateCharacteristicDiscovery();
        switch (rc) {
            case BLE_ERROR_INVALID_CONN_HANDLE:
            case NRF_ERROR_INVALID_ADDR:
                return BLE_ERROR_INVALID_PARAM;
            case NRF_ERROR_BUSY:
                return BLE_STACK_BUSY;
            default:
            case NRF_ERROR_INVALID_STATE:
                return BLE_ERROR_INVALID_STATE;
        }
    }

    return BLE_ERROR_NONE;
}

void
NordicServiceDiscovery::setupDiscoveredServices(const ble_gattc_evt_prim_srvc_disc_rsp_t *response)
{
    serviceIndex = 0;
    numServices  = response->count;

    /* Account for the limitation on the number of discovered services we can handle at a time. */
    if (numServices > BLE_DB_DISCOVERY_MAX_SRV) {
        numServices = BLE_DB_DISCOVERY_MAX_SRV;
    }

    for (unsigned serviceIndex = 0; serviceIndex < numServices; serviceIndex++) {
        services[serviceIndex].setup(response->services[serviceIndex].uuid.uuid,
                                     response->services[serviceIndex].handle_range.start_handle,
                                     response->services[serviceIndex].handle_range.end_handle);
    }
}

void
NordicServiceDiscovery::setupDiscoveredCharacteristics(const ble_gattc_evt_char_disc_rsp_t *response)
{
    characteristicIndex = 0;
    numCharacteristics  = response->count;

    /* Account for the limitation on the number of discovered characteristics we can handle at a time. */
    if (numCharacteristics > BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV) {
        numCharacteristics = BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV;
    }

    for (unsigned charIndex = 0; charIndex < numCharacteristics; charIndex++) {
        characteristics[charIndex].setup(response->chars[charIndex].uuid.uuid,
                                         *(const uint8_t *)(&response->chars[charIndex].char_props),
                                         response->chars[charIndex].handle_decl,
                                         response->chars[charIndex].handle_value);
    }
}
