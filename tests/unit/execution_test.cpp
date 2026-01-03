#include <iostream>
#include <cassert>
#include "exec/execution_engine.h"
#include "db/state_manager.h"
#include "core/account.h"
#include "db/rocksdb_wrapper.h"

using namespace aegen;

void test_transaction_execution() {
    // Setup
    RocksDBWrapper db("test_db"); // Mocked
    StateManager state(db);
    ExecutionEngine exec(state);
    
    Address alice = "alice_addr";
    Address bob = "bob_addr";
    
    // Mint initial balance
    AccountState aliceState;
    aliceState.balance = 1000;
    aliceState.nonce = 0;
    state.setAccountState(alice, aliceState);
    
    // Create Tx
    Transaction tx;
    tx.sender = alice;
    tx.receiver = bob;
    tx.amount = 100;
    tx.nonce = 0;
    tx.gasLimit = 21000;
    tx.gasPrice = 1;
    tx.calculateHash();
    
    // Execute
    assert(exec.validateTransaction(tx));
    exec.applyTransaction(tx);
    
    // Verify
    AccountState aliceNew = state.getAccountState(alice);
    AccountState bobNew = state.getAccountState(bob);
    
    // 1000 - 100 - 21000*1 = 1000 - 100 - 21000 ... Oops, negative balance test? 
    // Wait, 1000 is too small for gas. Let's adjust initial balance.
}

void test_execution_flow() {
    RocksDBWrapper db("test_db");
    StateManager state(db);
    ExecutionEngine exec(state);
    
    Address alice = "alice";
    Address bob = "bob";
    
    // 1. Setup Alice with 1,000,000
    state.setAccountState(alice, {0, 1000000});
    
    // 2. Tx: Alice sends 5000 to Bob. Gas cost: 100 * 1 = 100.
    Transaction tx;
    tx.sender = alice;
    tx.receiver = bob;
    tx.amount = 5000;
    tx.nonce = 0;
    tx.gasLimit = 100;
    tx.gasPrice = 1;
    tx.calculateHash();
    
    exec.applyTransaction(tx);
    
    // 3. Check
    assert(state.getAccountState(bob).balance == 5000);
    assert(state.getAccountState(alice).balance == (1000000 - 5000 - 100));
    assert(state.getAccountState(alice).nonce == 1);
    
    std::cout << "test_execution_flow: PASSED" << std::endl;
}

int main() {
    try {
        test_execution_flow();
    } catch (const std::exception& e) {
        std::cerr << "Failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
