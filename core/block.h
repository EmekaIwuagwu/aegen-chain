#pragma once
#include "types.h"
#include "transaction.h"
#include <vector>

namespace aegen {

struct BlockHeader {
    uint64_t height;
    uint64_t timestamp;
    Hash previousHash;
    Hash stateRoot;
    Hash txRoot; // Merkle root of transactions
    Address producer;
    Signature signature;
};

class Block {
public:
    BlockHeader header;
    std::vector<Transaction> transactions;

    Hash calculateHash() const;
    void addTransaction(const Transaction& tx);
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static Block deserialize(const std::vector<uint8_t>& data);
};

}
