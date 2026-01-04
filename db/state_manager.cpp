#include "state_manager.h"
#include <map>

namespace aegen {

// In-Memory cache replaced by member cache with mutex for thread safety

StateManager::StateManager(RocksDBWrapper& db) : db(db) {}

AccountState StateManager::getAccountState(const Address& addr) {
    std::shared_lock<std::shared_mutex> lock(cacheMutex);
    auto it = cache.find(addr);
    if (it != cache.end()) {
        return it->second;
    }
    // Return empty/default state if not found
    return AccountState{0, 0};
}

void StateManager::setAccountState(const Address& addr, const AccountState& state) {
    std::unique_lock<std::shared_mutex> lock(cacheMutex);
    cache[addr] = state;
}

void StateManager::commit() {
    // In a real impl, this would serialize state changes to batch writes in RocksDB
    // For now, we keep it in memory
}

void StateManager::rollback() {
    std::unique_lock<std::shared_mutex> lock(cacheMutex);
    cache.clear();
}

Hash StateManager::getRootHash() {
    // Mock root hash
    return Hash{};
}

}
