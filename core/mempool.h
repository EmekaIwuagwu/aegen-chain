#pragma once
#include "transaction.h"
#include <deque>

namespace aegen {

class Mempool {
    std::deque<Transaction> queue;
public:
    bool validate(const Transaction& tx);
    void add(const Transaction& tx);
    Transaction pop();
    size_t size() const;
    // Add peek or other utility
};

}
