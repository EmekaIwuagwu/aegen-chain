#pragma once
#include "storage_interface.h"
#include "db/state_manager.h"
#include <map>
#include <string>

namespace aegen {

class SandboxStorage : public StorageInterface {
    StateManager& backend;
    std::map<std::string, UInt256> dirtyStorage; // Key: "addr_key", Value: val

public:
    SandboxStorage(StateManager& sm) : backend(sm) {}

    void setStorage(const UInt256& contractAddr, const UInt256& key, const UInt256& value) override {
        std::string mapKey = contractAddr.toHex() + "_" + key.toHex();
        dirtyStorage[mapKey] = value;
    }

    UInt256 getStorage(const UInt256& contractAddr, const UInt256& key) const override {
        std::string mapKey = contractAddr.toHex() + "_" + key.toHex();
        
        // 1. Check local dirty cache
        auto it = dirtyStorage.find(mapKey);
        if (it != dirtyStorage.end()) {
            return it->second;
        }

        // 2. Check persistent backend
        std::string valHex = const_cast<StateManager&>(backend).getContractStorage(contractAddr.toHex(), key.toHex());
        return UInt256::fromHex(valHex);
    }
};

}
