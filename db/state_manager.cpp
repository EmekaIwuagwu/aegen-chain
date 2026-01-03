#include "state_manager.h"
#include <map>

namespace aegen {

// In-Memory cache for development/testing when RocksDB isn't setup
static std::map<std::string, std::string> g_mockDb;
static std::map<Address, AccountState> g_accountCache;

StateManager::StateManager(RocksDBWrapper& db) : db(db) {}

AccountState StateManager::getAccountState(const Address& addr) {
    if (g_accountCache.contains(addr)) {
        return g_accountCache[addr];
    }
    // Return empty/default state if not found
    return AccountState{0, 0};
}

void StateManager::setAccountState(const Address& addr, const AccountState& state) {
    g_accountCache[addr] = state;
}

void StateManager::commit() {
    // In a real impl, this would serialize state changes to batch writes in RocksDB
    // For now, we keep it in memory
}

void StateManager::rollback() {
    // Clean cache
    g_accountCache.clear();
}

Hash StateManager::getRootHash() {
    // Mock root hash
    return Hash{};
}

}
