#pragma once
#include "core/block.h"
#include <vector>
#include <string>

namespace aegen {

struct Batch {
    std::string batchId;
    uint64_t id;
    std::vector<Block> blocks;
    Hash batchRoot;
};

class BatchManager {
    std::vector<Block> pendingBlocks;
    uint64_t currentBatchId = 1;
    size_t batchSizeLimit = 2;

public:
    void addBlock(const Block& block);
    bool shouldBatch() const;
    Batch createBatch();
};

}
