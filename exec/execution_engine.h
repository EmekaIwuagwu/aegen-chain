#pragma once
#include "core/transaction.h"
#include "core/block.h"
#include "db/state_manager.h"
#include "core/receipt.h"
#include <map>
#include <optional>

namespace aegen {

class ExecutionEngine {
    StateManager& stateManager;
    
    // Execute data field operations (token transfers, etc.)
    void executeData(const Transaction& tx);
    
public:
    ExecutionEngine(StateManager& sm);

    void applyBlock(const Block& block);
    void applyTransaction(const Transaction& tx, const Address& coinbase);
    bool validateTransaction(const Transaction& tx);
    
    // Simulate execution without state changes (for eth_call)
    // Returns hex-encoded output
    std::string simulateTransaction(const Transaction& tx);

    std::optional<TransactionReceipt> getReceipt(const std::string& txHash);

private:
    std::map<std::string, TransactionReceipt> receiptCache;
    void executeData(const Transaction& tx, TransactionReceipt& receipt);
};

}
