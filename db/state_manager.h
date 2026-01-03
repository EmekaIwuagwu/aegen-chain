#pragma once
#include "rocksdb_wrapper.h"
#include "core/types.h"
#include "core/account.h"

namespace aegen {

class StateManager {
    RocksDBWrapper& db;
public:
    StateManager(RocksDBWrapper& db);
    
    AccountState getAccountState(const Address& addr);
    void setAccountState(const Address& addr, const AccountState& state);
    void commit();
    void rollback();
    Hash getRootHash();
};

}
