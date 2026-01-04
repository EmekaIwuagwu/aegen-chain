#include "execution_engine.h"
#include <iostream>
#include "util/crypto.h"
#include "tokens/token_transfer.h"

namespace aegen {

ExecutionEngine::ExecutionEngine(StateManager& sm) : stateManager(sm) {}

bool ExecutionEngine::validateTransaction(const Transaction& tx) {
    // 1. Check signature - CRITICAL SECURITY FIX
    // Extract public key from sender address
    // For Kadena-style "k:pubkey" addresses
    if (tx.sender.substr(0, 2) == "k:") {
        std::string pubKeyHex = tx.sender.substr(2);
        // Convert hex to bytes
        PublicKey senderPubKey;
        if (pubKeyHex.length() == 64) { // 32 bytes = 64 hex chars
            senderPubKey.resize(32);
            for (size_t i = 0; i < 32; ++i) {
                std::string byteStr = pubKeyHex.substr(i * 2, 2);
                senderPubKey[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            }
            
            // Verify signature
            if (tx.signature.empty() || !tx.isSignedBy(senderPubKey)) {
                std::cerr << "[SECURITY] Signature verification FAILED for " << tx.sender << std::endl;
                return false;
            }
        } else {
            std::cerr << "[SECURITY] Invalid public key format in address: " << tx.sender << std::endl;
            return false;
        }
    } else {
        // For simple addresses (alice, bob, etc.) - require signature in production
        // For now, log a warning
        std::cerr << "[WARNING] Simple address used without key verification: " << tx.sender << std::endl;
    }
    
    // 2. Check nonce
    AccountState senderState = stateManager.getAccountState(tx.sender);
    if (tx.nonce != senderState.nonce) {
        std::cerr << "[VALIDATION] Nonce mismatch. Expected: " << senderState.nonce 
                  << ", Got: " << tx.nonce << std::endl;
        return false;
    }
    
    // 3. Check balance
    uint64_t totalCost = tx.amount + (tx.gasLimit * tx.gasPrice);
    if (senderState.balance < totalCost) {
        std::cerr << "[VALIDATION] Insufficient balance. Required: " << totalCost 
                  << ", Available: " << senderState.balance << std::endl;
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

    // Deduct from sender (amount + gas fee)
    uint64_t gasFee = tx.gasLimit * tx.gasPrice;
    uint64_t totalCost = tx.amount + gasFee;
    senderState.balance -= totalCost;
    senderState.nonce++;
    stateManager.setAccountState(tx.sender, senderState);

    // Add to receiver
    AccountState receiverState = stateManager.getAccountState(tx.receiver);
    receiverState.balance += tx.amount;
    stateManager.setAccountState(tx.receiver, receiverState);
    
    // Handle data field for smart contract / token operations
    if (!tx.data.empty()) {
        executeData(tx);
    }
}

void ExecutionEngine::executeData(const Transaction& tx) {
    // Convert Bytes to string for parsing
    std::string data(tx.data.begin(), tx.data.end());
    
    // Parse data field for token operations
    // Format: "operation:tokenId:args..."
    
    // Simple parsing - find first colon
    size_t colonPos = data.find(':');
    if (colonPos == std::string::npos) return;
    
    std::string operation = data.substr(0, colonPos);
    std::string args = data.substr(colonPos + 1);
    
    if (operation == "token_transfer") {
        // Format: token_transfer:tokenId:to:amount
        size_t pos1 = args.find(':');
        if (pos1 == std::string::npos) return;
        
        std::string tokenId = args.substr(0, pos1);
        std::string remaining = args.substr(pos1 + 1);
        
        size_t pos2 = remaining.find(':');
        if (pos2 == std::string::npos) return;
        
        std::string to = remaining.substr(0, pos2);
        uint64_t amount = std::stoull(remaining.substr(pos2 + 1));
        
        TokenTransfer::transfer(tokenId, tx.sender, to, amount);
    }
    else if (operation == "token_approve") {
        // Format: token_approve:tokenId:spender:amount
        size_t pos1 = args.find(':');
        if (pos1 == std::string::npos) return;
        
        std::string tokenId = args.substr(0, pos1);
        std::string remaining = args.substr(pos1 + 1);
        
        size_t pos2 = remaining.find(':');
        if (pos2 == std::string::npos) return;
        
        std::string spender = remaining.substr(0, pos2);
        uint64_t amount = std::stoull(remaining.substr(pos2 + 1));
        
        TokenTransfer::approve(tokenId, tx.sender, spender, amount);
    }
    // Additional operations can be added here
}

}
