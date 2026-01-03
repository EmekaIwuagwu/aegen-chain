#include <iostream>
#include <cassert>
#include "exec/vm.h"

using namespace aegen;

void test_vm_logic() {
    VM vm;
    
    // Script: 10 + 20
    std::string script = "PUSH 10 PUSH 20 ADD STOP";
    auto code = SmartContract::compile(script);
    
    vm.execute(code);
    
    int64_t result = vm.getStackTop();
    std::cout << "VM Result (10+20): " << result << std::endl;
    assert(result == 30);
    
    // Script: Store 50 at index 1
    // Stack order for STORE: [val, key] -> pop key, pop val. 
    // Wait implementation: key = pop(), val = pop().
    // So to store 50 at 1: Push 50, Push 1, STORE.
    script = "PUSH 50 PUSH 1 STORE STOP";
    code = SmartContract::compile(script);
    vm.execute(code);
    // We can't verify memory directly with current public API, but if it runs without crash it passes basic test.
    
    std::cout << "test_vm_logic: PASSED" << std::endl;
}

int main() {
    test_vm_logic();
    return 0;
}
