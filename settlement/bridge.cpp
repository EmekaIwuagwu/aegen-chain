#include "bridge.h"
#include "util/crypto.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace aegen {

SettlementBridge::SettlementBridge(KadenaClient& k) : kadena(k) {}

std::string SettlementBridge::generatePactCmd(const Batch& batch) {
    std::vector<uint8_t> combined;
    for (const auto& block : batch.blocks) {
        combined.insert(combined.end(), block.header.stateRoot.begin(), block.header.stateRoot.end());
    }
    auto aggregateRoot = crypto::sha256_bytes(combined);
    
    std::stringstream pactCmd;
    pactCmd << "(aegen.submit-batch ";
    pactCmd << "\"" << batch.batchId << "\" ";
    pactCmd << "\"" << crypto::to_hex(aggregateRoot) << "\" ";
    pactCmd << batch.blocks.size() << " ";
    pactCmd << batch.blocks.front().header.height << " ";
    pactCmd << batch.blocks.back().header.height << ")";
    
    return pactCmd.str();
}

void SettlementBridge::settleBatch(const Batch& batch) {
    // ASCII-compatible box drawing
    std::cout << "\n+--------------------------------------------------------------+" << std::endl;
    std::cout << "|             AEGEN L2 -> KADENA L1 SETTLEMENT                 |" << std::endl;
    std::cout << "+--------------------------------------------------------------+" << std::endl;
    
    std::vector<uint8_t> combined;
    for (const auto& block : batch.blocks) {
        combined.insert(combined.end(), block.header.stateRoot.begin(), block.header.stateRoot.end());
    }
    auto aggregateRoot = crypto::sha256_bytes(combined);
    std::string stateRootHex = crypto::to_hex(aggregateRoot);
    
    std::cout << "|  Batch ID:      " << std::left << std::setw(45) << batch.batchId << "|" << std::endl;
    std::cout << "|  Block Range:   " << std::setw(45) 
              << (std::to_string(batch.blocks.front().header.height) + " - " + 
                  std::to_string(batch.blocks.back().header.height)) << "|" << std::endl;
    std::cout << "|  Block Count:   " << std::setw(45) << batch.blocks.size() << "|" << std::endl;
    std::cout << "|  State Root:    " << std::setw(45) << stateRootHex.substr(0, 40) + "..." << "|" << std::endl;
    std::cout << "+--------------------------------------------------------------+" << std::endl;
    
    auto result = kadena.settleBatch(batch.batchId, stateRootHex, batch.blocks.size());
    
    if (result.success) {
        std::cout << "|  Status:        " << std::setw(45) << "[OK] SUBMITTED" << "|" << std::endl;
        std::cout << "|  Request Key:   " << std::setw(45) << result.requestKey.substr(0, 40) + "..." << "|" << std::endl;
    } else {
        std::cout << "|  Status:        " << std::setw(45) << "[FAIL]" << "|" << std::endl;
        std::cout << "|  Error:         " << std::setw(45) << result.error.substr(0, 40) << "|" << std::endl;
    }
    
    std::cout << "+--------------------------------------------------------------+\n" << std::endl;
}

}
