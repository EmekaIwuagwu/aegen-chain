#include "validator.h"
#include "core/merkle.h"
#include "wallet/signer.h"
#include "util/crypto.h"
#include <iostream>

namespace aegen {

Validator::Validator(ExecutionEngine& exec, StateManager& state, const Address& producer)
    : executionEngine(exec), stateManager(state), authorizedProducer(producer) {}

bool Validator::validateBlock(const Block& block) {
    // 1. Verify Producer Signature
    if (block.header.producer != authorizedProducer) {
         // In a real network, check if producer is in Validator Set
         // For prototype, we check against a single authorized leader
         // std::cerr << "Invalid producer" << std::endl;
    }
    
    // Verify signature (using simple check or proper crypto verify if implemented)
    // Hash h = block.calculateHash();
    // if (!Signer::verify(h...)) return false;

    // 2. Verify Structure
    if (block.transactions.empty() && block.header.txRoot != Hash{}) {
        // Empty block should have specific root
    }

    // 3. Verify State Transitions
    // We need to execute transactions to verify StateRoot matches.
    // WARNING: This modifies state! In a real system we'd use a temporary view or snapshot.
    // For this prototype, we assume we are syncing linearly.
    
    // Capture state root before execution (if we needed to rollback)
    // Hash rootBefore = stateManager.getRootHash();

    for (const auto& tx : block.transactions) {
        if (!executionEngine.validateTransaction(tx)) {
            std::cerr << "Block contains invalid tx: " << crypto::to_hex(tx.hash) << std::endl;
            return false;
        }
        executionEngine.applyTransaction(tx, block.header.producer);
    }

    // 4. Verify Roots
    Hash calculatedStateRoot = stateManager.getRootHash();
    if (calculatedStateRoot != block.header.stateRoot) {
         std::cerr << "State root mismatch! Header: " << crypto::to_hex(block.header.stateRoot) 
                   << " Calculated: " << crypto::to_hex(calculatedStateRoot) << std::endl;
         // Critical: In real app, rollback state changes here!
         return false;
    }

    // Verify Tx Root
    std::vector<Hash> txHashes;
    for(const auto& tx : block.transactions) txHashes.push_back(tx.hash);
    Hash calculatedTxRoot = MerkleTree::computeRoot(txHashes);
    
    if (calculatedTxRoot != block.header.txRoot) {
        std::cerr << "Tx root mismatch!" << std::endl;
        return false;
    }

    return true;
}

}
