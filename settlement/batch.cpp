#include "batch.h"
#include "util/crypto.h"
#include <sstream>
#include <iomanip>

namespace aegen {

void BatchManager::addBlock(const Block& block) {
    pendingBlocks.push_back(block);
}

bool BatchManager::shouldBatch() const {
    return pendingBlocks.size() >= batchSizeLimit;
}

Batch BatchManager::createBatch() {
    Batch batch;
    batch.id = currentBatchId++;
    
    // Generate batch ID string
    std::stringstream ss;
    ss << "BATCH-" << std::setw(6) << std::setfill('0') << batch.id;
    batch.batchId = ss.str();
    
    batch.blocks = pendingBlocks;
    
    // Compute batch root
    std::vector<uint8_t> combined;
    for (const auto& block : batch.blocks) {
        combined.insert(combined.end(), block.header.stateRoot.begin(), block.header.stateRoot.end());
    }
    batch.batchRoot = crypto::sha256_bytes(combined);
    
    pendingBlocks.clear();
    return batch;
}

}
