#pragma once
#include "core/block.h"
#include "core/mempool.h"
#include "exec/execution_engine.h"
#include "wallet/keypair.h"
#include "db/state_manager.h"

namespace aegen {

class Leader {
    Mempool& mempool;
    ExecutionEngine& executionEngine;
    StateManager& stateManager;
    KeyPair nodeKeys;
    Address nodeAddress;

public:
    Leader(Mempool& mp, ExecutionEngine& exec, StateManager& state, const KeyPair& keys, const Address& addr);

    Block proposeBlock(uint64_t height, uint64_t previousTimestamp, const Hash& previousHash);
};

}
