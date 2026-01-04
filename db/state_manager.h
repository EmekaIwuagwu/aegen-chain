#pragma once
#include "rocksdb_wrapper.h"
#include "core/types.h"
#include "core/account.h"
#include <unordered_map>
#include <shared_mutex>

namespace aegen {

class StateManager {
    RocksDBWrapper& db;
    std::unordered_map<Address, AccountState> cache;
    mutable std::shared_mutex cacheMutex;

public:
    StateManager(RocksDBWrapper& db);
    
    AccountState getAccountState(const Address& addr);
    void setAccountState(const Address& addr, const AccountState& state);
    
    // Contract Storage Support
    std::string getContractStorage(const std::string& contractAddr, const std::string& key);
    void setContractStorage(const std::string& contractAddr, const std::string& key, const std::string& value);

    // Code Support
    std::string getContractCode(const std::string& contractAddr);
    void setContractCode(const std::string& contractAddr, const std::string& code);

    void commit();
    void rollback();
    Hash getRootHash();
};

}
