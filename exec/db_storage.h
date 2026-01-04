#pragma once
#include "storage_interface.h"
#include "db/state_manager.h"

namespace aegen {

// Adapter to bridge VM storage calls to the actual StateManager (RocksDB)
class DBStorage : public StorageInterface {
    StateManager& stateManager;
public:
    DBStorage(StateManager& sm) : stateManager(sm) {}
    
    void setStorage(const UInt256& contractAddr, const UInt256& key, const UInt256& value) override {
        // Construct a unique key for RocksDB: "storage:<contract>:<key>"
        // Since StateManager uses Address (string) as key, we format it.
        std::string dbKey = "storage:" + contractAddr.toHex() + ":" + key.toHex();
        
        // This is a direct DB write. 
        // Note: Real state manager might need a specific 'setStorage' method to separate accounts from raw storage.
        // For MVP, we piggyback on a generic set/get or assume keyspaces dont collide significantly 
        // if we prefix. But StateManager only has getAccountState/setAccountState which takes Address -> AccountState.
        // AccountState is struct {balance, nonce}. It doesn't hold storage.
        
        // SOLUTION: We need to extend StateManager or direct DB access.
        // To keep it clean, let's assume valid stateManager interface for generic KV, 
        // OR we implement a specific method in StateManager.
        // Let's assume we add `setContractStorage` to StateManager.
         stateManager.setContractStorage(contractAddr.toHex(), key.toHex(), value.toHex());
    }

    UInt256 getStorage(const UInt256& contractAddr, const UInt256& key) const override {
        std::string valHex = stateManager.getContractStorage(contractAddr.toHex(), key.toHex());
        return UInt256::fromHex(valHex);
    }
};

}
