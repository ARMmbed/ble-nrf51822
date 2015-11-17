#include "nRF5xCharacteristicDescriptorDiscoverer.h"
#include "ble_err.h"
#include "mbed-drivers/mbed_error.h"
#include "ble/DiscoveredCharacteristicDescriptor.h"

namespace { 
    void emptyDiscoveryCallback(const CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t*) { }
    void emptyTerminationCallback(const CharacteristicDescriptorDiscovery::TerminationCallbackParams_t*) { }
}

nRF5xCharacteristicDescriptorDiscoverer::nRF5xCharacteristicDescriptorDiscoverer(size_t concurrentConnectionsCount) : 
    maximumConcurrentConnectionsCount(concurrentConnectionsCount), 
    discoveryRunning(new Discovery[concurrentConnectionsCount]) {

}

nRF5xCharacteristicDescriptorDiscoverer::~nRF5xCharacteristicDescriptorDiscoverer() { 
    delete [] discoveryRunning;
}

ble_error_t nRF5xCharacteristicDescriptorDiscoverer::launch(
    const DiscoveredCharacteristic& characteristic, 
    const CharacteristicDescriptorDiscovery::DiscoveryCallback_t& discoveryCallback,
    const CharacteristicDescriptorDiscovery::TerminationCallback_t& terminationCallback
) {
    Gap::Handle_t connHandle = characteristic.getConnectionHandle();
    // it is ok to deduce that the start handle for descriptors is after 
    // the characteristic declaration and the characteristic value declaration
    // see BLUETOOTH SPECIFICATION Version 4.2 [Vol 3, Part G] (3.3)
    Gap::Handle_t descriptorStartHandle = characteristic.getDeclHandle() + 2;
    Gap::Handle_t descriptorEndHandle = characteristic.getLastHandle();

    // check if their is any descriptor to discover
    if (descriptorEndHandle < descriptorStartHandle) { 
        CharacteristicDescriptorDiscovery::TerminationCallbackParams_t termParams = { 
            characteristic,
            BLE_ERROR_NONE
        };
        terminationCallback.call(&termParams);
        return BLE_ERROR_NONE;
    }

    // check if we can run this discovery
    if (isConnectionInUse(connHandle)) { 
        return BLE_STACK_BUSY;
    }

    // get a new discovery slot, if none are available, just return
    Discovery* discovery = getAvailableDiscoverySlot();
    if(discovery == NULL) {
        return BLE_STACK_BUSY; 
    }

    // try to launch the discovery 
    ble_error_t err = gattc_descriptors_discover(connHandle, descriptorStartHandle, descriptorEndHandle);
    if(!err) { 
        // commit the new discovery to its slot
        *discovery = Discovery(characteristic, discoveryCallback, terminationCallback);
    }

    return err;
}

bool nRF5xCharacteristicDescriptorDiscoverer::isActive(const DiscoveredCharacteristic& characteristic) const {
    return findRunningDiscovery(characteristic) != NULL;
}

void nRF5xCharacteristicDescriptorDiscoverer::requestTerminate(const DiscoveredCharacteristic& characteristic) {
    Discovery* discovery = findRunningDiscovery(characteristic);
    if(discovery) { 
        discovery->onDiscovery = emptyDiscoveryCallback;
        // call terminate anyway
        discovery->terminate(BLE_ERROR_NONE);
        discovery->onTerminate = emptyTerminationCallback;
    }
}

void nRF5xCharacteristicDescriptorDiscoverer::process(uint16_t connectionHandle, const ble_gattc_evt_desc_disc_rsp_t& descriptors) {
    Discovery* discovery = findRunningDiscovery(connectionHandle);
    if(!discovery) { 
        error("logic error in nRF5xCharacteristicDescriptorDiscoverer::process !!!");
    }

    for (uint16_t i = 0; i < descriptors.count; ++i) {
        discovery->process(
            descriptors.descs[i].handle, UUID(descriptors.descs[i].uuid.uuid)
        );
    }

    // prepare the next discovery request (if needed)
    uint16_t startHandle = descriptors.descs[descriptors.count - 1].handle + 1;
    uint16_t endHandle = discovery->characteristic.getLastHandle();

    if(startHandle > endHandle || 
      (discovery->onDiscovery == emptyDiscoveryCallback && discovery->onTerminate == emptyTerminationCallback)) { 
        terminate(connectionHandle, BLE_ERROR_NONE);
        return;
    }

    ble_error_t err = gattc_descriptors_discover(connectionHandle, startHandle, endHandle);
    if(err) { 
        terminate(connectionHandle, err);
        return;
    }
}

void nRF5xCharacteristicDescriptorDiscoverer::terminate(uint16_t handle, ble_error_t err) {
    Discovery* discovery = findRunningDiscovery(handle);
    if(!discovery) { 
        error("logic error in nRF5xCharacteristicDescriptorDiscoverer::process !!!");
    }

    Discovery tmp = *discovery;
    *discovery = Discovery();
    tmp.terminate(err);
}

nRF5xCharacteristicDescriptorDiscoverer::Discovery* 
nRF5xCharacteristicDescriptorDiscoverer::findRunningDiscovery(const DiscoveredCharacteristic& characteristic) {
    for(size_t i = 0; i < maximumConcurrentConnectionsCount; ++i) { 
        if(discoveryRunning[i].characteristic == characteristic) { 
            return &discoveryRunning[i];
        }
    }
    return NULL;
}

nRF5xCharacteristicDescriptorDiscoverer::Discovery* 
nRF5xCharacteristicDescriptorDiscoverer::findRunningDiscovery(const DiscoveredCharacteristic& characteristic) const {
    for(size_t i = 0; i < maximumConcurrentConnectionsCount; ++i) { 
        if(discoveryRunning[i].characteristic == characteristic) { 
            return &discoveryRunning[i];
        }
    }
    return NULL;
}

nRF5xCharacteristicDescriptorDiscoverer::Discovery* 
nRF5xCharacteristicDescriptorDiscoverer::findRunningDiscovery(uint16_t handle) {
    for(size_t i = 0; i < maximumConcurrentConnectionsCount; ++i) {
        if(discoveryRunning[i].characteristic.getConnectionHandle() == handle && 
           discoveryRunning[i] != Discovery()) {
            return &discoveryRunning[i];
        }
    }
    return NULL;
}     

void nRF5xCharacteristicDescriptorDiscoverer::removeDiscovery(Discovery* discovery) {
    for(size_t i = 0; i < maximumConcurrentConnectionsCount; ++i) { 
        if(&discoveryRunning[i] == discovery) { 
            discoveryRunning[i] = Discovery();
        }
    }
}

nRF5xCharacteristicDescriptorDiscoverer::Discovery* 
nRF5xCharacteristicDescriptorDiscoverer::getAvailableDiscoverySlot() {
    for(size_t i = 0; i < maximumConcurrentConnectionsCount; ++i) { 
        if(discoveryRunning[i] == Discovery()) {
            return &discoveryRunning[i];
        }
    }     
    return NULL;
}

bool nRF5xCharacteristicDescriptorDiscoverer::isConnectionInUse(uint16_t connHandle) {
     return findRunningDiscovery(connHandle) != NULL;
}

ble_error_t nRF5xCharacteristicDescriptorDiscoverer::gattc_descriptors_discover(
    uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle) { 

    ble_gattc_handle_range_t discoveryRange = {
        start_handle,
        end_handle
    };
    uint32_t err = sd_ble_gattc_descriptors_discover(connection_handle, &discoveryRange);

    switch(err) { 
        case NRF_SUCCESS:
            return BLE_ERROR_NONE;            
        case BLE_ERROR_INVALID_CONN_HANDLE:
            return BLE_ERROR_INVALID_PARAM;
        case NRF_ERROR_INVALID_ADDR:
            return BLE_ERROR_PARAM_OUT_OF_RANGE;
        case NRF_ERROR_BUSY:
            return BLE_STACK_BUSY;
        default:
            return BLE_ERROR_UNSPECIFIED;
    }
}
