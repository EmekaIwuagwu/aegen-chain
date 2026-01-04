#include "state_manager.h"
#include "util/crypto.h"
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
    std::shared_lock<std::shared_mutex> lock(cacheMutex);
    if (cache.empty()) return Hash{};
    
    // Compute Merkle root of all account states
    std::vector<Hash> leaves;
    for (const auto& [addr, state] : cache) {
        // Hash each account: H(addr || nonce || balance)
        std::string data = addr + std::to_string(state.nonce) + std::to_string(state.balance);
        std::vector<uint8_t> bytes(data.begin(), data.end());
        leaves.push_back(crypto::sha256_bytes(bytes));
    }
    
    // Build Merkle tree
    while (leaves.size() > 1) {
        std::vector<Hash> nextLevel;
        for (size_t i = 0; i < leaves.size(); i += 2) {
            std::vector<uint8_t> combined;
            combined.insert(combined.end(), leaves[i].begin(), leaves[i].end());
            if (i + 1 < leaves.size()) {
                combined.insert(combined.end(), leaves[i+1].begin(), leaves[i+1].end());
            } else {
                combined.insert(combined.end(), leaves[i].begin(), leaves[i].end()); // Duplicate if odd
            }
            nextLevel.push_back(crypto::sha256_bytes(combined));
        }
        leaves = nextLevel;
    }
    return leaves[0];
}

// Storage keys will be prefixed with "storage:" in RocksDB
std::string StateManager::getContractStorage(const std::string& contractAddr, const std::string& key) {
   std::string dbKey = "storage:" + contractAddr + ":" + key;
   return db.get(dbKey);
}

void StateManager::setContractStorage(const std::string& contractAddr, const std::string& key, const std::string& value) {
   std::string dbKey = "storage:" + contractAddr + ":" + key;
   db.put(dbKey, value);
}

std::string StateManager::getContractCode(const std::string& contractAddr) {
    return db.get("code:" + contractAddr);
}

void StateManager::setContractCode(const std::string& contractAddr, const std::string& code) {
    db.put("code:" + contractAddr, code);
}

}
