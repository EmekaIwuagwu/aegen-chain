#include "vm.h"
#include <iostream>
#include <sstream>

namespace aegen {

void VM::execute(const std::vector<Instruction>& code) {
    size_t pc = 0;
    while(pc < code.size()) {
        const auto& inst = code[pc];
        switch(inst.op) {
            case PUSH:
                stack.push(std::stoll(inst.arg));
                break;
            case ADD: {
                if (stack.size() < 2) return;
                int64_t b = stack.top(); stack.pop();
                int64_t a = stack.top(); stack.pop();
                stack.push(a + b);
                break;
            }
            case SUB: {
                if (stack.size() < 2) return;
                int64_t b = stack.top(); stack.pop();
                int64_t a = stack.top(); stack.pop();
                stack.push(a - b);
                break;
            }
            case STORE: {
                // Simplified: Store numeric val at "addr" (string for now)
                // For this toy VM: PUSH val, PUSH key_addr, STORE
                if (stack.size() < 2) return;
                // We need keys to be strings? Or memory is int-addressable?
                // Let's assume memory is simple storage
                // ARG: We really need a string stack or ABI.
                // Simplified: Memory[stack.top()] = value
                int64_t key = stack.top(); stack.pop();
                int64_t val = stack.top(); stack.pop();
                memory[std::to_string(key)] = val;
                std::cout << "[VM] Stored " << val << " at " << key << std::endl;
                break;
            }
            case EQ: {
                if (stack.size() < 2) return;
                int64_t b = stack.top(); stack.pop();
                int64_t a = stack.top(); stack.pop();
                stack.push(a == b ? 1 : 0);
                break;
            }
            case STOP:
                return;
        }
        pc++;
    }
}

int64_t VM::getStackTop() const {
    if (stack.empty()) return 0;
    return stack.top();
}

std::vector<Instruction> SmartContract::compile(const std::string& script) {
    std::vector<Instruction> bytecode;
    std::stringstream ss(script);
    std::string token;
    while(ss >> token) {
        if (token == "PUSH") {
            std::string arg;
            ss >> arg;
            bytecode.push_back({PUSH, arg});
        } else if (token == "ADD") bytecode.push_back({ADD, ""});
        else if (token == "SUB") bytecode.push_back({SUB, ""});
        else if (token == "STORE") bytecode.push_back({STORE, ""});
        else if (token == "EQ") bytecode.push_back({EQ, ""});
        else if (token == "STOP") bytecode.push_back({STOP, ""});
    }
    return bytecode;
}

}
