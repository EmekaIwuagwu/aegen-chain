#pragma once
#include "core/block.h"
#include "core/transaction.h"
#include "core/merkle.h"
#include "util/crypto.h"
#include <vector>
#include <string>
#include <sstream>

namespace aegen {

/**
 * FraudProofType - Types of fraud proofs
 */
enum class FraudProofType {
    INVALID_STATE_TRANSITION,  // State root mismatch
    INVALID_TRANSACTION,       // Invalid tx in block
    DOUBLE_SPEND,              // Same tx in multiple blocks
    INVALID_SIGNATURE,         // Tx signature verification failed
    MERKLE_PROOF_FAILURE,      // Merkle proof doesn't verify
    DATA_UNAVAILABLE           // DA commitment mismatch
};

/**
 * FraudProof - Structure for submitting fraud proofs
 */
struct FraudProof {
    FraudProofType type;
    std::string batchId;
    uint64_t blockHeight;
    Hash expectedStateRoot;
    Hash actualStateRoot;
    std::vector<Hash> merkleProof;
    Transaction invalidTx;
    std::string proofData;
    
    std::string serialize() const {
        std::stringstream ss;
        ss << static_cast<int>(type) << "|";
        ss << batchId << "|";
        ss << blockHeight << "|";
        ss << crypto::to_hex(expectedStateRoot) << "|";
        ss << crypto::to_hex(actualStateRoot) << "|";
        ss << merkleProof.size() << "|";
        for (const auto& h : merkleProof) {
            ss << crypto::to_hex(h) << ",";
        }
        ss << "|" << proofData;
        return ss.str();
    }
};

/**
 * FraudProofResult - Result of fraud proof verification
 */
struct FraudProofResult {
    bool valid;
    bool fraudProven;
    std::string message;
    std::string operatorToSlash;
};

/**
 * FraudProofVerifier - Verifies fraud proofs for the L2 rollup
 */
class FraudProofVerifier {
public:
    /**
     * Verify an invalid state transition fraud proof
     * 
     * Proves that applying transactions in a block to the previous state
     * does not result in the claimed state root.
     */
    FraudProofResult verifyInvalidStateTransition(
        const Block& block,
        const Hash& prevStateRoot,
        const Hash& claimedStateRoot,
        const Hash& computedStateRoot
    ) {
        FraudProofResult result;
        result.valid = true;
        
        // Verify the previous state root matches block's parent
        if (block.header.previousHash != prevStateRoot) {
            result.fraudProven = false;
            result.message = "Proof invalid: previous state root mismatch";
            return result;
        }
        
        // Compare claimed vs computed state roots
        if (claimedStateRoot != computedStateRoot) {
            result.fraudProven = true;
            result.message = "Fraud proven: state transition invalid. Expected " +
                crypto::to_hex(computedStateRoot).substr(0, 16) + "... got " +
                crypto::to_hex(claimedStateRoot).substr(0, 16) + "...";
            return result;
        }
        
        result.fraudProven = false;
        result.message = "No fraud detected: state transition valid";
        return result;
    }
    
    /**
     * Verify an invalid transaction fraud proof
     * 
     * Proves that a transaction in a block is invalid (e.g., insufficient balance)
     */
    FraudProofResult verifyInvalidTransaction(
        const Transaction& tx,
        uint64_t senderBalance,
        uint64_t senderNonce
    ) {
        FraudProofResult result;
        result.valid = true;
        
        // Check balance
        if (tx.amount > senderBalance) {
            result.fraudProven = true;
            result.message = "Fraud proven: insufficient balance. Sender has " +
                std::to_string(senderBalance) + " but tx requires " + 
                std::to_string(tx.amount);
            return result;
        }
        
        // Check nonce
        if (tx.nonce != senderNonce) {
            result.fraudProven = true;
            result.message = "Fraud proven: invalid nonce. Expected " +
                std::to_string(senderNonce) + " got " + std::to_string(tx.nonce);
            return result;
        }
        
        result.fraudProven = false;
        result.message = "Transaction is valid";
        return result;
    }
    
    /**
     * Verify a Merkle inclusion proof
     * 
     * Proves that a transaction is included in a block's Merkle tree
     */
    FraudProofResult verifyMerkleInclusion(
        const Transaction& tx,
        const std::vector<Hash>& proof,
        const Hash& txRoot,
        size_t index
    ) {
        FraudProofResult result;
        result.valid = true;
        
        Hash currentHash = tx.hash;
        size_t idx = index;
        
        for (const auto& sibling : proof) {
            Hash combined;
            if (idx % 2 == 0) {
                // Current is left child
                combined = crypto::sha256(currentHash.data(), currentHash.size());
                auto siblingHash = crypto::sha256(sibling.data(), sibling.size());
                std::vector<uint8_t> concat;
                concat.insert(concat.end(), combined.begin(), combined.end());
                concat.insert(concat.end(), siblingHash.begin(), siblingHash.end());
                currentHash = crypto::sha256(concat.data(), concat.size());
            } else {
                // Current is right child
                auto siblingHash = crypto::sha256(sibling.data(), sibling.size());
                combined = crypto::sha256(currentHash.data(), currentHash.size());
                std::vector<uint8_t> concat;
                concat.insert(concat.end(), siblingHash.begin(), siblingHash.end());
                concat.insert(concat.end(), combined.begin(), combined.end());
                currentHash = crypto::sha256(concat.data(), concat.size());
            }
            idx /= 2;
        }
        
        if (currentHash != txRoot) {
            result.fraudProven = true;
            result.message = "Fraud proven: Merkle proof invalid";
            return result;
        }
        
        result.fraudProven = false;
        result.message = "Merkle inclusion verified";
        return result;
    }
    
    /**
     * Verify a double-spend fraud proof
     * 
     * Proves that the same transaction appears in multiple blocks
     */
    FraudProofResult verifyDoubleSpend(
        const Transaction& tx,
        const Block& block1,
        const Block& block2
    ) {
        FraudProofResult result;
        result.valid = true;
        
        // Find tx in both blocks
        bool inBlock1 = false, inBlock2 = false;
        
        for (const auto& btx : block1.transactions) {
            if (btx.hash == tx.hash) {
                inBlock1 = true;
                break;
            }
        }
        
        for (const auto& btx : block2.transactions) {
            if (btx.hash == tx.hash) {
                inBlock2 = true;
                break;
            }
        }
        
        if (inBlock1 && inBlock2 && block1.header.height != block2.header.height) {
            result.fraudProven = true;
            result.message = "Fraud proven: double spend detected in blocks " +
                std::to_string(block1.header.height) + " and " +
                std::to_string(block2.header.height);
            return result;
        }
        
        result.fraudProven = false;
        result.message = "No double spend detected";
        return result;
    }
    
    /**
     * Verify signature fraud proof
     * 
     * Proves that a transaction has an invalid signature
     */
    FraudProofResult verifyInvalidSignature(
        const Transaction& tx,
        const PublicKey& publicKey
    ) {
        FraudProofResult result;
        result.valid = true;
        
        // Verify signature
        if (!crypto::verify(tx.signature, tx.hash, publicKey)) {
            result.fraudProven = true;
            result.message = "Fraud proven: invalid signature on transaction " +
                crypto::to_hex(tx.hash).substr(0, 16) + "...";
            return result;
        }
        
        result.fraudProven = false;
        result.message = "Signature is valid";
        return result;
    }
    
    /**
     * Generate a fraud proof for submission to L1
     */
    FraudProof generateProof(
        FraudProofType type,
        const std::string& batchId,
        const Block& block,
        const Transaction& tx,
        const std::vector<Hash>& merkleProof = {}
    ) {
        FraudProof proof;
        proof.type = type;
        proof.batchId = batchId;
        proof.blockHeight = block.header.height;
        proof.expectedStateRoot = block.header.stateRoot;
        proof.actualStateRoot = block.header.stateRoot; // Will be computed
        proof.merkleProof = merkleProof;
        proof.invalidTx = tx;
        
        std::stringstream ss;
        ss << "Type:" << static_cast<int>(type);
        ss << ",Block:" << block.header.height;
        ss << ",Tx:" << crypto::to_hex(tx.hash);
        proof.proofData = ss.str();
        
        return proof;
    }
};

}
