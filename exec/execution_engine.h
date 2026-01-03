#pragma once
#include "core/transaction.h"
#include "core/block.h"
#include "db/state_manager.h"

namespace aegen {

class ExecutionEngine {
    StateManager& stateManager;
    
    // Execute data field operations (token transfers, etc.)
    void executeData(const Transaction& tx);
    
public:
    ExecutionEngine(StateManager& sm);

    void applyBlock(const Block& block);
    void applyTransaction(const Transaction& tx);
    bool validateTransaction(const Transaction& tx);
};

}
