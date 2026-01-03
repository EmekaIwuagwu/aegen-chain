#include "mempool.h"
#include <algorithm>

namespace aegen {

bool Mempool::validate(const Transaction& tx) {
    // Basic structural validation
    if (tx.amount == 0 && tx.data.empty()) return false;
    return true;
}

void Mempool::add(const Transaction& tx) {
    if (validate(tx)) {
        queue.push_back(tx);
        // Sort by gasPrice desc (simple priority)
        std::sort(queue.begin(), queue.end(), [](const Transaction& a, const Transaction& b) {
            return a.gasPrice > b.gasPrice;
        });
    }
}

Transaction Mempool::pop() {
    if (queue.empty()) return Transaction{};
    Transaction tx = queue.front();
    queue.pop_front();
    return tx;
}

size_t Mempool::size() const {
    return queue.size();
}

}
