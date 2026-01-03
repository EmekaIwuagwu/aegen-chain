#include "execution_engine.h"
#include <iostream>
#include "util/crypto.h"


namespace aegen {

ExecutionEngine::ExecutionEngine(StateManager& sm) : stateManager(sm) {}

bool ExecutionEngine::validateTransaction(const Transaction& tx) {
    // 1. Check signature (Mocked for now)
    // 2. Check nonce
    AccountState senderState = stateManager.getAccountState(tx.sender);
    if (tx.nonce != senderState.nonce) {
        return false;
    }
    
    // 3. Check balance
    uint64_t totalCost = tx.amount + (tx.gasLimit * tx.gasPrice);
    if (senderState.balance < totalCost) {
        return false;
    }

    return true;
}

void ExecutionEngine::applyTransaction(const Transaction& tx) {
    // Re-validate strict context (nonce must match exactly for execution)
    AccountState senderState = stateManager.getAccountState(tx.sender);
    if (tx.nonce != senderState.nonce) {
        std::cerr << "Error: Invalid nonce for tx " << aegen::crypto::to_hex(tx.hash) << std::endl;
        return;
    }

    // Deduct from sender
    uint64_t totalCost = tx.amount + (tx.gasLimit * tx.gasPrice); // Simplified fee burn
    senderState.balance -= totalCost;
    senderState.nonce++;
    stateManager.setAccountState(tx.sender, senderState);

    // Add to receiver
    AccountState receiverState = stateManager.getAccountState(tx.receiver);
    receiverState.balance += tx.amount;
    stateManager.setAccountState(tx.receiver, receiverState);
    
    // TODO: Handle data/smart contracts/tokens if present
}

}
