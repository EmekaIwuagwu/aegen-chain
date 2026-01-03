#include <iostream>
#include <vector>
#include <cassert>
#include "consensus/validator.h"
#include "consensus/leader.h"
#include "core/mempool.h"
#include "db/rocksdb_wrapper.h"

using namespace aegen;

void test_consensus_validator() {
    RocksDBWrapper db("test_val_db");
    StateManager state(db);
    ExecutionEngine exec(state);
    Mempool mempool;
    
    Address alice = "alice";
    Address bob = "bob";
    KeyPair producerKeys = Wallet::generateKeyPair(); // Stub
    Address producerAddr = "producer_node_1";
    
    Leader leader(mempool, exec, state, producerKeys, producerAddr);
    Validator validator(exec, state, producerAddr);

    // 1. Setup State: Alice has 1000
    state.setAccountState(alice, {0, 1000000});
    
    // 2. Add Valid Tx 
    Transaction tx1;
    tx1.sender = alice;
    tx1.receiver = bob;
    tx1.amount = 100;
    tx1.nonce = 0;
    tx1.gasLimit = 21000;
    tx1.gasPrice = 1;
    tx1.calculateHash();
    mempool.add(tx1);

    // 3. Propose Block
    Block block = leader.proposeBlock(1, 1000000, {});
    assert(block.transactions.size() == 1);
    
    // 4. Validate Block
    // Note: In real life, Validator would run on a DIFFERENT node with DIFFERENT state instance,
    // which has NOT yet applied the block.
    // However, Leader::proposeBlock executes the txs to generate state root! 
    // And since we share the same `state` instance here, the state IS ALREADY UPDATED via leader.
    // So Validator will run applyTransaction AGAIN on expected-nonce+1 and FAIL!
    
    // To test Validator correctly, we must revert state or use separate State instances.
    // Rollback is mocked to clear cache.
    // But `setAccountState` might have persisted if not mocked.
    
    // Let's reset state for Validator test
    state.setAccountState(alice, {0, 1000000});
    state.setAccountState(bob, {0, 0});
    
    bool isValid = validator.validateBlock(block);
    
    if (isValid) {
        std::cout << "test_consensus_validator: PASSED" << std::endl;
    } else {
        std::cout << "test_consensus_validator: FAILED" << std::endl;
        assert(false);
    }
}

int main() {
    test_consensus_validator();
    return 0;
}
