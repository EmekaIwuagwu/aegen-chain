#include "leader.h"
#include "core/merkle.h"
#include "wallet/signer.h"
#include <ctime>

namespace aegen {

Leader::Leader(Mempool& mp, ExecutionEngine& exec, StateManager& state, const KeyPair& keys, const Address& addr)
    : mempool(mp), executionEngine(exec), stateManager(state), nodeKeys(keys), nodeAddress(addr) {}

Block Leader::proposeBlock(uint64_t height, uint64_t previousTimestamp, const Hash& previousHash) {
    Block block;
    block.header.height = height;
    block.header.previousHash = previousHash;
    block.header.producer = nodeAddress;
    
    // Simple timestamping (ensure > prev)
    uint64_t now = std::time(nullptr);
    if (now <= previousTimestamp) now = previousTimestamp + 1;
    block.header.timestamp = now;

    // Fill block with transactions from mempool
    // Cap at say 100 for now
    int count = 0;
    while(mempool.size() > 0 && count < 100) {
        Transaction tx = mempool.pop();
        if (executionEngine.validateTransaction(tx)) {
             // Execute to update state
             executionEngine.applyTransaction(tx);
             block.addTransaction(tx);
             count++;
        } else {
            // Drop invalid tx
        }
    }

    // Compute Roots
    block.header.stateRoot = stateManager.getRootHash();
    
    std::vector<Hash> txHashes;
    for(const auto& tx : block.transactions) {
        txHashes.push_back(tx.hash);
    }
    block.header.txRoot = MerkleTree::computeRoot(txHashes);

    // Sign Block
    // We need to serialize header to sign it. 
    // For now we mock signature or implement simple serialization for signing.
    // Convert Hash to Bytes
    Hash h = block.calculateHash();
    Bytes hashBytes(h.begin(), h.end());
    block.header.signature = Signer::sign(hashBytes, nodeKeys.privateKey);

    return block;
}

}
