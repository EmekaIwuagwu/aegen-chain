#pragma once
#include <vector>
#include <string>
#include <stack>
#include <map>
#include <optional>
#include "core/types.h"
#include "util/uint256.h"
#include "storage_interface.h"

namespace aegen {

// Comprehensive EVM implementation
enum class OpCode : uint8_t {
    STOP = 0x00,
    ADD = 0x01, MUL = 0x02, SUB = 0x03, DIV = 0x04, 
    SDIV = 0x05, MOD = 0x06, SMOD = 0x07, ADDMOD = 0x08, MULMOD = 0x09, 
    EXP = 0x0A, SIGNEXTEND = 0x0B,
    
    LT = 0x10, GT = 0x11, SLT = 0x12, SGT = 0x13, EQ = 0x14, 
    ISZERO = 0x15, AND = 0x16, OR = 0x17, XOR = 0x18, NOT = 0x19, BYTE = 0x1A,
    SHL = 0x1B, SHR = 0x1C, SAR = 0x1D,
    
    SHA3 = 0x20,
    
    ADDRESS = 0x30, BALANCE = 0x31, ORIGIN = 0x32, CALLER = 0x33,
    CALLVALUE = 0x34, CALLDATALOAD = 0x35, CALLDATASIZE = 0x36, 
    CALLDATACOPY = 0x37, CODESIZE = 0x38, CODECOPY = 0x39, 
    GASPRICE = 0x3A, EXTCODESIZE = 0x3B, EXTCODECOPY = 0x3C, 
    RETURNDATASIZE = 0x3D, RETURNDATACOPY = 0x3E, EXTCODEHASH = 0x3F, 
    BLOCKHASH = 0x40, COINBASE = 0x41, TIMESTAMP = 0x42, 
    NUMBER = 0x43, DIFFICULTY = 0x44, GASLIMIT = 0x45, CHAINID = 0x46, 
    SELFBALANCE = 0x47, BASEFEE = 0x48,
    
    POP = 0x50, MLOAD = 0x51, MSTORE = 0x52, MSTORE8 = 0x53, 
    SLOAD = 0x54, SSTORE = 0x55, JUMP = 0x56, JUMPI = 0x57, 
    PC = 0x58, MSIZE = 0x59, GAS = 0x5A, JUMPDEST = 0x5B,
    
    PUSH1 = 0x60, PUSH32 = 0x7F,
    DUP1 = 0x80, DUP16 = 0x8F,
    SWAP1 = 0x90, SWAP16 = 0x9F,
    
    LOG0 = 0xA0, LOG1 = 0xA1, LOG2 = 0xA2, LOG3 = 0xA3, LOG4 = 0xA4,
    
    CREATE = 0xF0, CALL = 0xF1, CALLCODE = 0xF2, RETURN = 0xF3, 
    DELEGATECALL = 0xF4, CREATE2 = 0xF5, STATICCALL = 0xF6, 
    REVERT = 0xFD, INVALID = 0xFE, SELFDESTRUCT = 0xFF
};

struct LogEntry {
    UInt256 address;
    std::vector<UInt256> topics;
    std::vector<uint8_t> data;
};

struct ExecutionResult {
    bool success;
    uint64_t gasUsed;
    std::vector<uint8_t> output; 
    std::string error;
    std::vector<LogEntry> logs; // captured logs
};

struct CallContext {
    UInt256 caller;
    UInt256 address;
    UInt256 value;
    std::vector<uint8_t> data; // Call data
    uint64_t gasLimit;
};

class VM {
    std::vector<UInt256> stack;
    std::vector<uint8_t> memory;
    StorageInterface* storage; // Pointer to storage backend
    
    // EVM Execution Context
    uint64_t pc;
    uint64_t gasRemaining;
    bool reverted;
    std::vector<uint8_t> returnData;
    std::vector<LogEntry> currentLogs;
    
    void stackPush(const UInt256& val);
    UInt256 stackPop();
    UInt256 stackPeek(int offset) const;
    void stackSwap(int n);
    void stackDup(int n);
    
    // Memory Ops
    void memStore(uint64_t offset, const UInt256& val);
    void memStore8(uint64_t offset, uint8_t val);
    UInt256 memLoad(uint64_t offset);
    void expandMemory(uint64_t offset, uint64_t size);
    
    // Gas
    bool consumeGas(uint64_t amount);
    
    // Precompile Logic
    bool executePrecompile(const UInt256& addr, const std::vector<uint8_t>& input, std::vector<uint8_t>& output, uint64_t& gasUsed);

public:
    VM(StorageInterface* storageBackend = nullptr) : storage(storageBackend) {}

    ExecutionResult execute(const std::vector<uint8_t>& code, const CallContext& ctx);
    
    // Accessors for testing
    UInt256 getStackTop() const;
};

}
