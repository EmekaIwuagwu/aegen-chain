#pragma once
#include "core/block.h"
#include "exec/execution_engine.h"
#include "db/state_manager.h"

namespace aegen {

class Validator {
    ExecutionEngine& executionEngine;
    StateManager& stateManager;
    Address authorizedProducer; // Simple authority for now

public:
    Validator(ExecutionEngine& exec, StateManager& state, const Address& producer);

    bool validateBlock(const Block& block);
};

}
