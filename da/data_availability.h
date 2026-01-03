#pragma once
#include "core/block.h"
#include "util/crypto.h"
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <cmath>
#include <random>

namespace aegen {

/**
 * DataBlob - A chunk of data for DA sampling
 */
struct DataBlob {
    std::vector<uint8_t> data;
    Hash hash;
    size_t index;
    
    void computeHash() {
        hash = crypto::sha256_bytes(data.data(), data.size());
    }
};

/**
 * DACommitment - Data Availability commitment for L1
 */
struct DACommitment {
    std::string batchId;
    Hash dataRoot;           // Merkle root of all blobs
    Hash erasureRoot;        // Root after erasure coding
    size_t blobCount;
    size_t totalSize;
    uint64_t timestamp;
    
    std::string serialize() const {
        std::stringstream ss;
        ss << batchId << "|";
        ss << crypto::to_hex(dataRoot) << "|";
        ss << crypto::to_hex(erasureRoot) << "|";
        ss << blobCount << "|";
        ss << totalSize << "|";
        ss << timestamp;
        return ss.str();
    }
};

/**
 * DASample - A sample for Data Availability Sampling
 */
struct DASample {
    size_t row;
    size_t col;
    DataBlob blob;
    std::vector<Hash> proof;
    bool verified;
};

/**
 * DataAvailabilityLayer - Manages data availability for the L2
 * 
 * Implements:
 * - Data blob storage and retrieval
 * - Erasure coding (2D Reed-Solomon style)
 * - Data Availability Sampling (DAS)
 * - Commitment generation for L1
 */
class DataAvailabilityLayer {
private:
    std::map<std::string, std::vector<DataBlob>> batchBlobs;
    std::map<std::string, DACommitment> commitments;
    std::mutex mtx;
    
    static constexpr size_t BLOB_SIZE = 4096;  // 4KB blobs
    static constexpr size_t SAMPLE_COUNT = 8;   // Samples per verification
    
    // Simple XOR-based erasure code (production would use Reed-Solomon)
    std::vector<DataBlob> generateErasureCodes(const std::vector<DataBlob>& original) {
        std::vector<DataBlob> parity;
        
        // Generate parity blobs (XOR pairs for simplicity)
        for (size_t i = 0; i < original.size(); i += 2) {
            DataBlob parityBlob;
            parityBlob.index = original.size() + i / 2;
            parityBlob.data.resize(BLOB_SIZE, 0);
            
            for (size_t j = 0; j < BLOB_SIZE; ++j) {
                uint8_t b1 = (i < original.size() && j < original[i].data.size()) 
                    ? original[i].data[j] : 0;
                uint8_t b2 = (i + 1 < original.size() && j < original[i + 1].data.size()) 
                    ? original[i + 1].data[j] : 0;
                parityBlob.data[j] = b1 ^ b2;
            }
            
            parityBlob.computeHash();
            parity.push_back(parityBlob);
        }
        
        return parity;
    }
    
    // Build Merkle tree from blobs
    Hash buildMerkleRoot(const std::vector<DataBlob>& blobs) {
        if (blobs.empty()) {
            return Hash{};
        }
        
        std::vector<Hash> hashes;
        for (const auto& blob : blobs) {
            hashes.push_back(blob.hash);
        }
        
        // Pad to power of 2
        size_t n = 1;
        while (n < hashes.size()) n *= 2;
        while (hashes.size() < n) {
            hashes.push_back(Hash{});
        }
        
        // Build tree
        while (hashes.size() > 1) {
            std::vector<Hash> next;
            for (size_t i = 0; i < hashes.size(); i += 2) {
                std::vector<uint8_t> concat;
                concat.insert(concat.end(), hashes[i].begin(), hashes[i].end());
                concat.insert(concat.end(), hashes[i + 1].begin(), hashes[i + 1].end());
                next.push_back(crypto::sha256_bytes(concat.data(), concat.size()));
            }
            hashes = next;
        }
        
        return hashes[0];
    }

public:
    /**
     * Store batch data and generate DA commitment
     */
    DACommitment storeData(const std::string& batchId, const std::vector<Block>& blocks) {
        std::lock_guard<std::mutex> lock(mtx);
        
        // Serialize blocks into blobs
        std::vector<DataBlob> blobs;
        std::vector<uint8_t> buffer;
        
        for (const auto& block : blocks) {
            // Serialize block (simplified)
            std::stringstream ss;
            ss << block.header.height << "|";
            ss << crypto::to_hex(block.header.stateRoot) << "|";
            ss << crypto::to_hex(block.header.previousHash) << "|";
            ss << block.transactions.size() << "|";
            
            for (const auto& tx : block.transactions) {
                ss << tx.sender << "," << tx.receiver << "," << tx.amount << ";";
            }
            
            std::string data = ss.str();
            buffer.insert(buffer.end(), data.begin(), data.end());
        }
        
        // Split into blobs
        for (size_t i = 0; i < buffer.size(); i += BLOB_SIZE) {
            DataBlob blob;
            blob.index = blobs.size();
            size_t end = std::min(i + BLOB_SIZE, buffer.size());
            blob.data.assign(buffer.begin() + i, buffer.begin() + end);
            blob.computeHash();
            blobs.push_back(blob);
        }
        
        // Generate erasure codes
        auto parityBlobs = generateErasureCodes(blobs);
        
        // Combine original and parity
        std::vector<DataBlob> allBlobs = blobs;
        allBlobs.insert(allBlobs.end(), parityBlobs.begin(), parityBlobs.end());
        
        // Store
        batchBlobs[batchId] = allBlobs;
        
        // Create commitment
        DACommitment commitment;
        commitment.batchId = batchId;
        commitment.dataRoot = buildMerkleRoot(blobs);
        commitment.erasureRoot = buildMerkleRoot(allBlobs);
        commitment.blobCount = allBlobs.size();
        commitment.totalSize = buffer.size();
        commitment.timestamp = static_cast<uint64_t>(std::time(nullptr));
        
        commitments[batchId] = commitment;
        
        return commitment;
    }
    
    /**
     * Retrieve a specific blob
     */
    DataBlob getBlob(const std::string& batchId, size_t index) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = batchBlobs.find(batchId);
        if (it != batchBlobs.end() && index < it->second.size()) {
            return it->second[index];
        }
        
        return DataBlob{};
    }
    
    /**
     * Get all blobs for a batch
     */
    std::vector<DataBlob> getAllBlobs(const std::string& batchId) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = batchBlobs.find(batchId);
        if (it != batchBlobs.end()) {
            return it->second;
        }
        
        return {};
    }
    
    /**
     * Generate Merkle proof for a blob
     */
    std::vector<Hash> generateProof(const std::string& batchId, size_t index) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = batchBlobs.find(batchId);
        if (it == batchBlobs.end()) {
            return {};
        }
        
        const auto& blobs = it->second;
        std::vector<Hash> proof;
        
        // Build Merkle proof (simplified)
        std::vector<Hash> hashes;
        for (const auto& blob : blobs) {
            hashes.push_back(blob.hash);
        }
        
        size_t n = 1;
        while (n < hashes.size()) n *= 2;
        while (hashes.size() < n) {
            hashes.push_back(Hash{});
        }
        
        size_t idx = index;
        while (hashes.size() > 1) {
            std::vector<Hash> next;
            for (size_t i = 0; i < hashes.size(); i += 2) {
                if (i == (idx / 2) * 2) {
                    proof.push_back(hashes[i + (idx % 2 == 0 ? 1 : 0)]);
                }
                std::vector<uint8_t> concat;
                concat.insert(concat.end(), hashes[i].begin(), hashes[i].end());
                concat.insert(concat.end(), hashes[i + 1].begin(), hashes[i + 1].end());
                next.push_back(crypto::sha256_bytes(concat.data(), concat.size()));
            }
            hashes = next;
            idx /= 2;
        }
        
        return proof;
    }
    
    /**
     * Perform Data Availability Sampling (DAS)
     * 
     * Randomly samples blobs to verify data is available
     */
    bool performDAS(const std::string& batchId) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = batchBlobs.find(batchId);
        if (it == batchBlobs.end()) {
            return false;
        }
        
        const auto& blobs = it->second;
        if (blobs.empty()) {
            return false;
        }
        
        // Sample random blobs
        size_t successful = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, blobs.size() - 1);
        
        for (size_t i = 0; i < SAMPLE_COUNT; ++i) {
            size_t idx = dist(gen);
            
            // Verify blob hash
            DataBlob sample = blobs[idx];
            Hash computed = crypto::sha256_bytes(sample.data.data(), sample.data.size());
            
            if (computed == sample.hash) {
                successful++;
            }
        }
        
        // Require all samples to pass
        return successful == SAMPLE_COUNT;
    }
    
    /**
     * Verify a DA commitment
     */
    bool verifyCommitment(const DACommitment& commitment) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = commitments.find(commitment.batchId);
        if (it == commitments.end()) {
            return false;
        }
        
        const auto& stored = it->second;
        return stored.dataRoot == commitment.dataRoot &&
               stored.erasureRoot == commitment.erasureRoot &&
               stored.blobCount == commitment.blobCount;
    }
    
    /**
     * Get commitment for a batch
     */
    DACommitment getCommitment(const std::string& batchId) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = commitments.find(batchId);
        if (it != commitments.end()) {
            return it->second;
        }
        
        return DACommitment{};
    }
    
    /**
     * Reconstruct missing data using erasure codes
     */
    bool reconstructData(const std::string& batchId, size_t missingIndex) {
        std::lock_guard<std::mutex> lock(mtx);
        
        auto it = batchBlobs.find(batchId);
        if (it == batchBlobs.end()) {
            return false;
        }
        
        auto& blobs = it->second;
        size_t originalCount = blobs.size() / 2; // Half are parity
        
        if (missingIndex >= originalCount) {
            return false; // Can only reconstruct original data
        }
        
        // Find partner index and parity
        size_t partner = (missingIndex % 2 == 0) ? missingIndex + 1 : missingIndex - 1;
        size_t parityIdx = originalCount + missingIndex / 2;
        
        if (partner >= originalCount || parityIdx >= blobs.size()) {
            return false;
        }
        
        // Reconstruct using XOR
        DataBlob reconstructed;
        reconstructed.index = missingIndex;
        reconstructed.data.resize(BLOB_SIZE);
        
        for (size_t i = 0; i < BLOB_SIZE; ++i) {
            uint8_t p = (i < blobs[parityIdx].data.size()) ? blobs[parityIdx].data[i] : 0;
            uint8_t b = (i < blobs[partner].data.size()) ? blobs[partner].data[i] : 0;
            reconstructed.data[i] = p ^ b;
        }
        
        reconstructed.computeHash();
        blobs[missingIndex] = reconstructed;
        
        return true;
    }
    
    /**
     * Get statistics
     */
    struct Stats {
        size_t totalBatches;
        size_t totalBlobs;
        size_t totalBytes;
    };
    
    Stats getStats() {
        std::lock_guard<std::mutex> lock(mtx);
        Stats stats{};
        stats.totalBatches = batchBlobs.size();
        for (const auto& [id, blobs] : batchBlobs) {
            stats.totalBlobs += blobs.size();
            for (const auto& blob : blobs) {
                stats.totalBytes += blob.data.size();
            }
        }
        return stats;
    }
};

}
