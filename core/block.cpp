#include "block.h"
#include "util/crypto.h"
#include <sstream>
#include <cstring>

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

std::vector<uint8_t> Block::serialize() const {
    std::vector<uint8_t> buffer;
    auto append = [&](const void* data, size_t size) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        buffer.insert(buffer.end(), p, p + size);
    };

    append(&header.height, sizeof(header.height));
    append(&header.timestamp, sizeof(header.timestamp));
    append(header.previousHash.data(), header.previousHash.size());
    append(header.stateRoot.data(), header.stateRoot.size());
    append(header.txRoot.data(), header.txRoot.size());
    
    uint32_t prodLen = (uint32_t)header.producer.size();
    append(&prodLen, sizeof(prodLen));
    append(header.producer.data(), prodLen);
    
    append(header.signature.data(), header.signature.size());
    
    uint32_t txCount = (uint32_t)transactions.size();
    append(&txCount, sizeof(txCount));
    
    for(const auto& tx : transactions) {
        auto txBytes = tx.serialize();
        uint32_t len = (uint32_t)txBytes.size();
        append(&len, sizeof(len));
        buffer.insert(buffer.end(), txBytes.begin(), txBytes.end());
    }
    return buffer;
}

Block Block::deserialize(const std::vector<uint8_t>& data) {
    Block block;
    size_t offset = 0;
    
    auto read = [&](void* dest, size_t size) {
        if (offset + size > data.size()) return false;
        std::memcpy(dest, &data[offset], size);
        offset += size;
        return true;
    };
    
    read(&block.header.height, sizeof(uint64_t));
    read(&block.header.timestamp, sizeof(uint64_t));
    read(block.header.previousHash.data(), 32);
    read(block.header.stateRoot.data(), 32);
    read(block.header.txRoot.data(), 32);
    
    uint32_t prodLen;
    read(&prodLen, sizeof(uint32_t));
    if (offset + prodLen <= data.size()) {
        block.header.producer.assign((char*)&data[offset], prodLen);
        offset += prodLen;
    }
    
    read(block.header.signature.data(), 64);
    
    uint32_t txCount;
    read(&txCount, sizeof(uint32_t));
    
    for(uint32_t i=0; i<txCount; i++) {
        uint32_t len;
        read(&len, sizeof(uint32_t));
        if (offset + len <= data.size()) {
            std::vector<uint8_t> txData(data.begin() + offset, data.begin() + offset + len);
            block.transactions.push_back(Transaction::deserialize(txData));
            offset += len;
        }
    }
    
    return block;
}

}
