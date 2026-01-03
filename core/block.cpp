#include "block.h"
#include "util/crypto.h"
#include <sstream>

namespace aegen {

Hash Block::calculateHash() const {
    // Hash relevant header fields
    std::stringstream ss;
    ss << header.height 
       << header.timestamp 
       << crypto::to_hex(header.previousHash)
       << crypto::to_hex(header.stateRoot)
       << crypto::to_hex(header.txRoot)
       << header.producer;
    
    std::string data = ss.str();
    std::vector<uint8_t> bytes(data.begin(), data.end());
    return crypto::sha256_bytes(bytes);
}

void Block::addTransaction(const Transaction& tx) {
    transactions.push_back(tx);
}

}
