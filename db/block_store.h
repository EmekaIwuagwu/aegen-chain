#pragma once
#include "core/block.h"
#include "util/crypto.h"
#include "rocksdb_wrapper.h"
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <sstream>
#include <memory>

namespace aegen {

/**
 * BlockStore - Persistent block storage using RocksDB
 * 
 * Stores finalized blocks with disk persistence.
 * Keys: "block:{height}" -> serialized block
 * Meta: "meta:height" -> current blockchain height
 */
class BlockStore {
    std::unique_ptr<RocksDBWrapper> db;
    std::vector<Block> cache;  // In-memory cache for fast access
    std::map<uint64_t, size_t> heightIndex;
    std::mutex mtx;
    uint64_t currentHeight = 0;

    // Serialize block to string
    std::string serializeBlock(const Block& block) {
        std::stringstream ss;
        ss << block.header.height << "|";
        ss << crypto::to_hex(block.header.previousHash) << "|";
        ss << crypto::to_hex(block.header.stateRoot) << "|";
        ss << block.header.timestamp << "|";
        ss << block.transactions.size() << "|";
        
        for (const auto& tx : block.transactions) {
            ss << tx.sender << "," << tx.receiver << "," << tx.amount << "," 
               << tx.nonce << "," << crypto::to_hex(tx.hash) << ";";
        }
        return ss.str();
    }

    // Deserialize block from string
    // Convert vector to Hash array
    Hash vectorToHash(const std::vector<uint8_t>& vec) {
        Hash h{};
        size_t copyLen = std::min(vec.size(), h.size());
        std::copy(vec.begin(), vec.begin() + copyLen, h.begin());
        return h;
    }

    Block deserializeBlock(const std::string& data) {
        Block block;
        if (data.empty()) return block;

        std::stringstream ss(data);
        std::string token;

        std::getline(ss, token, '|');
        block.header.height = std::stoull(token);

        std::getline(ss, token, '|');
        block.header.previousHash = vectorToHash(crypto::from_hex(token));

        std::getline(ss, token, '|');
        block.header.stateRoot = vectorToHash(crypto::from_hex(token));

        std::getline(ss, token, '|');
        block.header.timestamp = std::stoull(token);

        std::getline(ss, token, '|');
        size_t txCount = std::stoull(token);

        // Parse transactions
        std::string txData;
        std::getline(ss, txData);
        if (!txData.empty()) {
            std::stringstream txStream(txData);
            std::string txStr;
            while (std::getline(txStream, txStr, ';')) {
                if (txStr.empty()) continue;
                Transaction tx;
                std::stringstream txParts(txStr);
                std::string part;
                
                std::getline(txParts, part, ','); tx.sender = part;
                std::getline(txParts, part, ','); tx.receiver = part;
                std::getline(txParts, part, ','); tx.amount = std::stoull(part);
                std::getline(txParts, part, ','); tx.nonce = std::stoull(part);
                std::getline(txParts, part, ','); tx.hash = vectorToHash(crypto::from_hex(part));
                
                block.transactions.push_back(tx);
            }
        }

        return block;
    }

    void loadFromDisk() {
        // Load height
        std::string heightStr = db->get("meta:height");
        if (!heightStr.empty()) {
            currentHeight = std::stoull(heightStr);
        }

        // Load all blocks into cache
        for (uint64_t h = 0; h <= currentHeight; h++) {
            std::string key = "block:" + std::to_string(h);
            std::string data = db->get(key);
            if (!data.empty()) {
                Block block = deserializeBlock(data);
                cache.push_back(block);
                heightIndex[h] = cache.size() - 1;
            }
        }
    }

public:
    BlockStore() : db(nullptr) {}
    
    BlockStore(const std::string& dbPath) {
        db = std::make_unique<RocksDBWrapper>(dbPath + "/blocks");
        loadFromDisk();
    }

    void init(const std::string& dbPath) {
        if (!db) {
            db = std::make_unique<RocksDBWrapper>(dbPath + "/blocks");
            loadFromDisk();
        }
    }

    void addBlock(const Block& block) {
        std::lock_guard<std::mutex> lock(mtx);
        
        cache.push_back(block);
        heightIndex[block.header.height] = cache.size() - 1;
        currentHeight = block.header.height;

        // Persist to disk
        if (db) {
            std::string key = "block:" + std::to_string(block.header.height);
            db->put(key, serializeBlock(block));
            db->put("meta:height", std::to_string(currentHeight));
        }
    }

    Block getBlock(uint64_t height) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = heightIndex.find(height);
        if (it != heightIndex.end() && it->second < cache.size()) {
            return cache[it->second];
        }
        return Block{};
    }

    std::vector<Block> getBlocks(uint64_t start, uint64_t count) {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Block> result;
        
        // Return blocks in reverse order (newest first)
        int64_t startIdx = (int64_t)cache.size() - (int64_t)start;
        for (int64_t i = startIdx; i > startIdx - (int64_t)count && i > 0; --i) {
            result.push_back(cache[i - 1]);
        }
        return result;
    }

    uint64_t getHeight() {
        std::lock_guard<std::mutex> lock(mtx);
        return currentHeight > 0 ? currentHeight : cache.size();
    }

    uint64_t getTotalTransactions() {
        std::lock_guard<std::mutex> lock(mtx);
        uint64_t total = 0;
        for (const auto& block : cache) {
            total += block.transactions.size();
        }
        return total;
    }

    // Force save to disk
    void flush() {
        if (db) {
            db->forceCompact();
        }
    }
};

}
