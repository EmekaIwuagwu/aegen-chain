#pragma once
#include <vector>
#include <string>
#include <stack>
#include <map>
#include "core/types.h"

namespace aegen {

enum OpCode {
    PUSH,   // Push value to stack
    ADD,    // Pop 2, Add, Push
    SUB,    // Pop 2, Sub, Push
    STORE,  // Pop Key, Pop Value, Store in Memory
    LOAD,   // Pop Key, Load from Memory, Push
    EQ,     // Pop 2, Push 1 if equal, 0 else
    JUMPI,  // Pop condition, Pop destination, Jump if condition != 0
    STOP    // Halt
};

struct Instruction {
    OpCode op;
    std::string arg; // For PUSH
};

class VM {
    std::stack<int64_t> stack;
    std::map<std::string, int64_t> memory;
public:
    void execute(const std::vector<Instruction>& code);
    int64_t getStackTop() const;
};

class SmartContract {
public:
    static std::vector<Instruction> compile(const std::string& script);
};

}
