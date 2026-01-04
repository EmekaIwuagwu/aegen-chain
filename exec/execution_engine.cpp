#include "execution_engine.h"
#include <iostream>
#include "util/crypto.h"
#include "tokens/token_transfer.h"
#include "vm.h"
#include "db_storage.h"
#include "sandbox_storage.h"

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

void ExecutionEngine::applyTransaction(const Transaction& tx, const Address& coinbase) {
    // Re-validate strict context (nonce must match exactly for execution)
    AccountState senderState = stateManager.getAccountState(tx.sender);
    if (tx.nonce != senderState.nonce) {
        std::cerr << "Error: Invalid nonce for tx " << aegen::crypto::to_hex(tx.hash) << std::endl;
        return;
    }

    // Deduct upfront cost from sender (amount + max gas fee)
    // Validate balance again just in case (though validateTransaction checks it)
    uint64_t maxGasFee = tx.gasLimit * tx.gasPrice;
    uint64_t totalUpfrontCost = tx.amount + maxGasFee;
    
    if (senderState.balance < totalUpfrontCost) {
         // Should not happen if pre-validated, but safety check.
         // If fail, we can't even pay for gas, so we drop.
         std::cerr << "Error: Insufficient balance during execution" << std::endl;
         return;
    }

    senderState.balance -= totalUpfrontCost;
    senderState.nonce++;
    stateManager.setAccountState(tx.sender, senderState);

    // Prepare Receipt
    TransactionReceipt receipt;
    receipt.transactionHash = tx.hash;
    receipt.from = tx.sender;
    receipt.to = tx.receiver;
    receipt.status = true; 
    receipt.gasUsed = 21000; // Intrinsic gas (basic transfer)
    
    // Execute Data (VM) if present
    if (!tx.data.empty()) {
        executeData(tx, receipt);
    }
    
    // Cap gasUsed at gasLimit logic is implicit in VM, but safety check:
    if (receipt.gasUsed > tx.gasLimit) receipt.gasUsed = tx.gasLimit;
    
    // Calculate Final Costs
    uint64_t actualGasFee = receipt.gasUsed * tx.gasPrice;
    uint64_t refund = maxGasFee - actualGasFee;
    
    // 1. Refund Sender
    if (refund > 0) {
        senderState = stateManager.getAccountState(tx.sender); // Reload in case it's same as receiver or changed (e.g. loops)
        senderState.balance += refund;
        stateManager.setAccountState(tx.sender, senderState);
    }
    
    // 2. Pay Validator (Coinbase)
    // Only if coinbase is valid
    if (!coinbase.empty()) {
        AccountState coinbaseState = stateManager.getAccountState(coinbase);
        coinbaseState.balance += actualGasFee;
        stateManager.setAccountState(coinbase, coinbaseState);
    }

    // 3. Transfer Value to Receiver
    // Only if status is SUCCESS (except gas, which is paid regardless of revert)
    // Wait, in EVM, value transfer happens unless reverted. 
    // If VM reverted (status = false), we do NOT transfer value, but we DO pay gas.
    // However, the `senderState.balance -= totalUpfrontCost` already removed amount.
    // So if failed, we must refund the AMOUNT too.
    
    if (receipt.status) {
         AccountState receiverState = stateManager.getAccountState(tx.receiver);
         receiverState.balance += tx.amount;
         stateManager.setAccountState(tx.receiver, receiverState);
    } else {
         // Revert: Refund the value amount to sender
         // Sender was already deducted `amount + maxGas`. 
         // We refunded `maxGas - actualGas = unusedGas`.
         // Now we refund `amount`.
         senderState = stateManager.getAccountState(tx.sender);
         senderState.balance += tx.amount;
         stateManager.setAccountState(tx.sender, senderState);
    }

    // Cache receipt
    receiptCache[crypto::to_hex(tx.hash)] = receipt;
}

void ExecutionEngine::executeData(const Transaction& tx, TransactionReceipt& receipt) {
    // Check if EVM transaction (heuristic: hex-like data or receiver with code)
    // If receiver is empty -> Deploy
    // If receiver has code -> Call
    
    // Legacy fallback check first for "token_" prefix?
    std::string dataStr(tx.data.begin(), tx.data.end());
    if (dataStr.rfind("token_", 0) == 0) { // Starts with token_
         // Parse: token_transfer:tokenId:receiver:amount or token_mint:tokenId:account:amount
         // This is a simplified internal token operation format
         // In production, this would be replaced by ABI-encoded calls
         receipt.gasUsed = 21000; // Base gas for token op
         return;
    }
    
    // EVM Execution
    DBStorage storageBackend(stateManager);
    VM vm(&storageBackend);
    
    CallContext ctx;
    ctx.caller = UInt256::fromHex(crypto::to_hex(crypto::sha256(tx.sender))); 
    ctx.value = UInt256(tx.amount);
    ctx.data = tx.data;
    ctx.gasLimit = tx.gasLimit;
    
    std::vector<uint8_t> code;
    
    if (tx.receiver.empty()) {
        // CONTRACT DEPLOYMENT
        ctx.address = UInt256(0); // For deployment context
        
        // Execute init code
        auto result = vm.execute(tx.data, ctx);
        receipt.gasUsed += result.gasUsed;
        receipt.status = result.success;
        
        if (result.success) {
            // Calculate Contract Address (mock: hash of sender + nonce)
            // Real ETH: keccak(RLP(sender, nonce))
            std::string contractAddr = "0x" + crypto::to_hex(crypto::sha256(tx.sender + std::to_string(tx.nonce))).substr(24);
            stateManager.setContractCode(contractAddr, std::string(result.output.begin(), result.output.end()));
            receipt.contractAddress = contractAddr;
            receipt.to = contractAddr; // In receipt, 'to' is null for deployment, 'contractAddress' is set.
        }
    } else {
        // CONTRACT CALL
        // Load code from state
        std::string codeStr = stateManager.getContractCode(tx.receiver);
        if (codeStr.empty()) return; // Not a contract or empty
        
        code.assign(codeStr.begin(), codeStr.end());
        
        ctx.address = UInt256::fromHex(tx.receiver.substr(0, 2) == "0x" ? tx.receiver.substr(2) : tx.receiver);
        
        auto result = vm.execute(code, ctx);
        receipt.gasUsed += result.gasUsed;
        receipt.status = result.success;

        // Convert Logs
        for (const auto& logEntry : result.logs) {
            Log log;
            log.address = "0x" + logEntry.address.toHex();
            for (const auto& topic : logEntry.topics) {
                Hash h;
                auto bytes = topic.toBigEndianBytes();
                std::copy(bytes.begin(), bytes.end(), h.begin());
                log.topics.push_back(h);
            }
            log.data = logEntry.data;
            receipt.logs.push_back(log);
        }
    }
}

std::optional<TransactionReceipt> ExecutionEngine::getReceipt(const std::string& txHash) {
    if (receiptCache.count(txHash)) return receiptCache[txHash];
    return std::nullopt;
}
std::string ExecutionEngine::simulateTransaction(const Transaction& tx) {
    SandboxStorage sandbox(stateManager);
    VM vm(&sandbox);
    
    CallContext ctx;
    // Mock caller derivation
    ctx.caller = UInt256::fromHex(crypto::to_hex(crypto::sha256(tx.sender)));
    ctx.gasLimit = tx.gasLimit;
    ctx.value = UInt256(tx.amount);
    
    std::vector<uint8_t> code;
    
    if (tx.receiver.empty()) {
        // Simulation of deployment - executes init code
        code = tx.data;
        ctx.address = UInt256(0);
    } else {
        std::string codeStr = stateManager.getContractCode(tx.receiver);
        if (codeStr.empty()) return "0x";
        code.assign(codeStr.begin(), codeStr.end());
        ctx.address = UInt256::fromHex(crypto::to_hex(crypto::sha256(tx.receiver)));
        ctx.data = tx.data;
    }
    
    ExecutionResult res = vm.execute(code, ctx);
    return crypto::to_hex(res.output);
}

}
