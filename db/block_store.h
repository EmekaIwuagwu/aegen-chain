#pragma once
#include "core/block.h"
#include "util/crypto.h"
#include <vector>
#include <map>
#include <mutex>
#include <string>

namespace aegen {

/**
 * BlockStore - Stores finalized blocks for explorer queries
 */
class BlockStore {
    std::vector<Block> blocks;
    std::map<uint64_t, size_t> heightIndex; // height -> block index
    std::mutex mtx;

public:
    void addBlock(const Block& block) {
        std::lock_guard<std::mutex> lock(mtx);
        blocks.push_back(block);
        heightIndex[block.header.height] = blocks.size() - 1;
    }

    Block getBlock(uint64_t height) {
        std::lock_guard<std::mutex> lock(mtx);
        if (height > 0 && height <= blocks.size()) {
            return blocks[height - 1];
        }
        return Block{};
    }

    std::vector<Block> getBlocks(uint64_t start, uint64_t count) {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Block> result;
        
        // Return blocks in reverse order (newest first)
        int64_t startIdx = (int64_t)blocks.size() - (int64_t)start;
        for (int64_t i = startIdx; i > startIdx - (int64_t)count && i > 0; --i) {
            result.push_back(blocks[i - 1]);
        }
        return result;
    }

    uint64_t getHeight() {
        std::lock_guard<std::mutex> lock(mtx);
        return blocks.size();
    }

    uint64_t getTotalTransactions() {
        std::lock_guard<std::mutex> lock(mtx);
        uint64_t total = 0;
        for (const auto& block : blocks) {
            total += block.transactions.size();
        }
        return total;
    }
};

}
