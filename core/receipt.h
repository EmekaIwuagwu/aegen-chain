#pragma once
#include "types.h"
#include "util/uint256.h"
#include <vector>
#include <string>

namespace aegen {

struct Log {
    Address address;
    std::vector<Hash> topics; // 32-byte topics
    std::vector<uint8_t> data;
};

struct TransactionReceipt {
    Hash transactionHash;
    uint64_t transactionIndex;
    Hash blockHash;
    uint64_t blockNumber;
    Address from;
    Address to;
    uint64_t gasUsed;
    Address contractAddress; // If deployment
    std::vector<Log> logs;
    bool status; // 1 success, 0 failure
};

}
