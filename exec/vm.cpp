#include "vm.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "proofs/zk_proof.h"

namespace aegen {

// Constants
constexpr uint64_t MAX_STACK_SIZE = 1024;
constexpr uint64_t GAS_COST_JUMPDEST = 1;
constexpr uint64_t GAS_COST_BASE = 2; // Simplified base cost
constexpr uint64_t GAS_COST_VERYLOW = 3;
constexpr uint64_t GAS_COST_LOW = 5;
constexpr uint64_t GAS_COST_MID = 8;
constexpr uint64_t GAS_COST_HIGH = 10;
constexpr uint64_t GAS_COST_SSTORE_SET = 20000;
constexpr uint64_t GAS_COST_SSTORE_RESET = 5000;
constexpr uint64_t GAS_COST_SLOAD = 800;
constexpr uint64_t GAS_COST_CREATE = 32000;

void VM::stackPush(const UInt256& val) {
    if (stack.size() >= MAX_STACK_SIZE) throw std::runtime_error("Stack overflow");
    stack.push_back(val);
}

UInt256 VM::stackPop() {
    if (stack.empty()) throw std::runtime_error("Stack underflow");
    UInt256 val = stack.back();
    stack.pop_back();
    return val;
}

UInt256 VM::stackPeek(int offset) const {
    if (offset < 0 || offset >= stack.size()) throw std::runtime_error("Stack underflow");
    return stack[stack.size() - 1 - offset];
}

void VM::stackSwap(int n) {
    if (stack.size() <= n) throw std::runtime_error("Stack underflow for SWAP");
    std::swap(stack[stack.size()-1], stack[stack.size()-1-n]);
}

void VM::stackDup(int n) {
    if (stack.size() < n) throw std::runtime_error("Stack underflow for DUP");
    stackPush(stack[stack.size()-n]);
}

bool VM::consumeGas(uint64_t amount) {
    if (gasRemaining < amount) {
        return false;
    }
    gasRemaining -= amount;
    return true;
}

// Simplified memory expansion cost model (linear for MVP)
void VM::expandMemory(uint64_t offset, uint64_t size) {
    uint64_t required = offset + size;
    if (required > memory.size()) {
        // Round up to word size (32 bytes)
        uint64_t newSize = (required + 31) / 32 * 32;
        uint64_t expansion = newSize - memory.size();
        
        // Gas cost for memory: 3 per word + quadratic. 
        // Simplified: 3 per word
        uint64_t cost = (expansion / 32) * 3;
        if (!consumeGas(cost)) throw std::runtime_error("Out of gas (memory expansion)");
        
        memory.resize(newSize, 0);
    }
}

void VM::memStore(uint64_t offset, const UInt256& val) {
    expandMemory(offset, 32);
    auto bytes = val.toBigEndianBytes();
    std::copy(bytes.begin(), bytes.end(), memory.begin() + offset);
}

void VM::memStore8(uint64_t offset, uint8_t val) {
    expandMemory(offset, 1);
    memory[offset] = val;
}

UInt256 VM::memLoad(uint64_t offset) {
    expandMemory(offset, 32);
    std::vector<uint8_t> bytes(32);
    std::copy(memory.begin() + offset, memory.begin() + offset + 32, bytes.begin());
    return UInt256::fromBigEndianBytes(bytes);
}

ExecutionResult VM::execute(const std::vector<uint8_t>& code, const CallContext& ctx) {
    stack.clear();
    memory.clear();
    currentLogs.clear(); // Clear logs from previous run if any
    pc = 0;
    gasRemaining = ctx.gasLimit;
    reverted = false;
    
    ExecutionResult result;
    result.success = true;
    
    // Jump table validation could happen here (for JUMPDEST analysis)
    // Production EVM pre-scans code for JUMPDESTs. Skipped for brevity.

    try {
        while (pc < code.size()) {
            uint8_t op = code[pc];
            
            // Basic gas cost
            if (!consumeGas(GAS_COST_BASE)) throw std::runtime_error("Out of gas2");
            
            pc++;

            switch (static_cast<OpCode>(op)) {
                case OpCode::STOP: 
                    goto end_execution;
                
                // Arithmetic
                case OpCode::ADD: stackPush(stackPop() + stackPop()); break;
                case OpCode::MUL: stackPush(stackPop() * stackPop()); break;
                case OpCode::SUB: {
                     UInt256 a = stackPop();
                     UInt256 b = stackPop();
                     stackPush(a - b); 
                     break;
                }
                case OpCode::DIV: {
                    UInt256 b = stackPop();
                    UInt256 a = stackPop();
                    stackPush(a / b);
                    break;
                }
                case OpCode::MOD: {
                     UInt256 b = stackPop();
                     UInt256 a = stackPop();
                     stackPush(a % b);
                     break;
                }
                // Bitwise
                case OpCode::AND: stackPush(stackPop() & stackPop()); break;
                case OpCode::OR: stackPush(stackPop() | stackPop()); break;
                case OpCode::XOR: stackPush(stackPop() ^ stackPop()); break;
                case OpCode::NOT: stackPush(~stackPop()); break;
                
                // Comparision
                case OpCode::LT: {
                    UInt256 b = stackPop();
                    UInt256 a = stackPop();
                    stackPush(a < b ? UInt256(1) : UInt256(0));
                    break;
                }
                case OpCode::EQ: {
                    UInt256 a = stackPop();
                    UInt256 b = stackPop();
                    stackPush(a == b ? UInt256(1) : UInt256(0));
                    break;
                }
                case OpCode::ISZERO: {
                    UInt256 a = stackPop();
                    stackPush(a == UInt256(0) ? UInt256(1) : UInt256(0));
                    break;
                }
                
                // Stack
                case OpCode::POP: stackPop(); break;
                
                // Memory
                case OpCode::MLOAD: {
                    UInt256 offset = stackPop();
                    stackPush(memLoad(offset.toUint64()));
                    break;
                }
                case OpCode::MSTORE: {
                    UInt256 offset = stackPop();
                    UInt256 val = stackPop();
                    memStore(offset.toUint64(), val);
                    break;
                }
                case OpCode::MSTORE8: {
                    UInt256 offset = stackPop();
                     UInt256 val = stackPop();
                     memStore8(offset.toUint64(), (uint8_t)val.toUint64());
                     break;
                }
                
                // Storage
                case OpCode::SLOAD: {
                    if (!consumeGas(GAS_COST_SLOAD)) throw std::runtime_error("Out of gas (SLOAD)");
                    UInt256 key = stackPop();
                    UInt256 val(0);
                    if (storage) {
                        val = storage->getStorage(ctx.address, key);
                    }
                    stackPush(val);
                    break;
                }
                case OpCode::SSTORE: {
                    if (!consumeGas(GAS_COST_SSTORE_SET)) throw std::runtime_error("Out of gas (SSTORE)");
                    UInt256 key = stackPop();
                    UInt256 val = stackPop();
                    if (storage) {
                        storage->setStorage(ctx.address, key, val);
                    }
                    break;
                }
                
                // Flow
                case OpCode::JUMP: {
                    UInt256 dest = stackPop();
                    uint64_t target = dest.toUint64();
                    if (target >= code.size() || code[target] != (uint8_t)OpCode::JUMPDEST) {
                        throw std::runtime_error("Invalid Jump Destination");
                    }
                    pc = target;
                    break;
                }
                case OpCode::JUMPI: {
                    UInt256 dest = stackPop();
                    UInt256 cond = stackPop();
                    if (cond.toUint64() != 0) {
                        uint64_t target = dest.toUint64();
                        if (target >= code.size() || code[target] != (uint8_t)OpCode::JUMPDEST) {
                            throw std::runtime_error("Invalid JUMPI Destination");
                        }
                        pc = target;
                    }
                    break;
                }
                case OpCode::JUMPDEST:
                    consumeGas(GAS_COST_JUMPDEST);
                    break;
                    
                // Push
                case OpCode::PUSH1: {
                    if (pc >= code.size()) throw std::runtime_error("PUSH1 OOB");
                    stackPush(UInt256(code[pc]));
                    pc++;
                    break;
                }
                case OpCode::PUSH32: {
                    if (pc + 32 > code.size()) throw std::runtime_error("PUSH32 OOB");
                    std::vector<uint8_t> bytes(code.begin() + pc, code.begin() + pc + 32);
                    stackPush(UInt256::fromBigEndianBytes(bytes));
                    pc += 32;
                    break;
                }

                // Logs
                case OpCode::LOG0:
                case OpCode::LOG1:
                case OpCode::LOG2:
                case OpCode::LOG3:
                case OpCode::LOG4: {
                    uint8_t numTopics = (uint8_t)op - (uint8_t)OpCode::LOG0;
                    UInt256 offset = stackPop();
                    UInt256 size = stackPop();
                    
                    std::vector<UInt256> topics;
                    for (uint8_t i = 0; i < numTopics; ++i) {
                        topics.push_back(stackPop());
                    }
                    
                    uint64_t memOffset = offset.toUint64();
                    uint64_t len = size.toUint64();
                    
                    // Cost check: 375 + 8 * byte_size + 375 * numTopics
                    uint64_t cost = 375 + 8 * len + 375 * numTopics;
                    if (!consumeGas(cost)) throw std::runtime_error("Out of gas (LOG)");
                    
                    // Expand memory if needed
                    expandMemory(memOffset, len);
                    
                    std::vector<uint8_t> data;
                    if (len > 0) {
                        data.assign(memory.begin() + memOffset, memory.begin() + memOffset + len);
                    }
                    
                    // Add to logs
                    currentLogs.push_back({ctx.address, topics, data});
                    break;
                }
                
                // Swap/Dup
                case OpCode::DUP1: stackDup(1); break;
                case OpCode::SWAP1: stackSwap(1); break;
                
                // Invalid
                case OpCode::REVERT: {
                     // EVM REVERT: pops offset and size, returns data from memory as revert reason
                     UInt256 offset = stackPop();
                     UInt256 size = stackPop();
                     uint64_t off = offset.toUint64();
                     uint64_t len = size.toUint64();
                     
                     std::string reason;
                     if (len > 0 && off + len <= memory.size()) {
                         // Skip first 4 bytes (function selector for Error(string)) + 64 bytes offset/length if present
                         // Simplified: just take raw bytes as reason
                         reason.assign(memory.begin() + off, memory.begin() + off + std::min(len, (uint64_t)256));
                     }
                     
                     result.success = false;
                     result.error = reason.empty() ? "REVERT" : "REVERT: " + reason;
                     result.output.assign(memory.begin() + off, memory.begin() + off + std::min(len, (uint64_t)memory.size() - off));
                     goto end_execution;
                }
                case OpCode::INVALID: {
                    throw std::runtime_error("INVALID Opcode");
                }
                
                case OpCode::STATICCALL: {
                    // Stack: gas, addr, argsOffset, argsSize, retOffset, retSize
                    UInt256 gas = stackPop();
                    UInt256 addr = stackPop();
                    UInt256 argsOff = stackPop();
                    UInt256 argsSize = stackPop();
                    UInt256 retOff = stackPop();
                    UInt256 retSize = stackPop();
                    
                    // Consume Gas (Simplified)
                    if (!consumeGas(700)) throw std::runtime_error("Out of gas (STATICCALL)"); // Base cost
                    
                    // Prepare Input
                    expandMemory(argsOff.toUint64(), argsSize.toUint64());
                    std::vector<uint8_t> input;
                    if (argsSize.toUint64() > 0) {
                        input.resize(argsSize.toUint64());
                        std::copy(memory.begin() + argsOff.toUint64(), 
                                  memory.begin() + argsOff.toUint64() + argsSize.toUint64(), 
                                  input.begin());
                    }
                    
                    std::vector<uint8_t> output;
                    uint64_t precompileGas = 0;
                    bool success = false;
                    
                    // Check Precompile
                    if (addr.toUint64() > 0 && addr.toUint64() < 100) {
                        success = executePrecompile(addr, input, output, precompileGas);
                        if (!consumeGas(precompileGas)) throw std::runtime_error("Out of gas (Precompile)");
                    } else {
                        // Internal contract call - execute target contract code
                        // In production, this would load code from storage, create sub-context, and execute
                        // For now, we mark success if address is non-zero (contract exists check would go here)
                        if (storage != nullptr) {
                            UInt256 storedCode = storage->getStorage(addr, UInt256(0)); // Check if contract exists
                            if (storedCode != UInt256(0)) {
                                // Contract exists, execution would happen here
                                // For this iteration, return success with empty output
                                success = true;
                            }
                        }
                    }
                    
                    if (success) {
                        stackPush(UInt256(1)); // Success flag on stack
                        
                        // Write output to memory
                        if (retSize.toUint64() > 0) {
                            expandMemory(retOff.toUint64(), retSize.toUint64());
                            size_t copyLen = std::min((size_t)retSize.toUint64(), output.size());
                            std::copy(output.begin(), output.begin() + copyLen, memory.begin() + retOff.toUint64());
                            // Zero pad rest?
                            if (copyLen < retSize.toUint64()) {
                                std::fill(memory.begin() + retOff.toUint64() + copyLen, 
                                          memory.begin() + retOff.toUint64() + retSize.toUint64(), 0);
                            }
                        }
                    } else {
                         stackPush(UInt256(0));
                    }
                    break;
                }
                
                default:
                    // If between PUSH1 and PUSH32
                    if (op >= 0x60 && op <= 0x7F) {
                        int size = op - 0x5F;
                        if (pc + size > code.size()) throw std::runtime_error("PUSH OOB");
                        std::vector<uint8_t> slice(code.begin() + pc, code.begin() + pc + size);
                        stackPush(UInt256::fromBigEndianBytes(slice));
                        pc += size;
                    } else if (op >= 0x80 && op <= 0x8F) {
                        stackDup(op - 0x80 + 1);
                    } else if (op >= 0x90 && op <= 0x9F) {
                        stackSwap(op - 0x90 + 1);
                    } else {
                        throw std::runtime_error("Unknown Opcode: " + std::to_string(op));
                    }
                    break;
            }
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }

end_execution:
    result.gasUsed = ctx.gasLimit - gasRemaining;
    result.logs = currentLogs;
    return result;
}

bool VM::executePrecompile(const UInt256& addr, const std::vector<uint8_t>& input, std::vector<uint8_t>& output, uint64_t& gasUsed) {
    uint64_t id = addr.toUint64();
    if (id == 9) {
        // ZK Snark Verifier (Groth16)
        // Input Format: A(64) | B(128) | C(64) | numInputs(32) | inputs(n*32)
        // Minimum: 64 + 128 + 64 + 32 = 288 bytes
        
        gasUsed = 50000;
        
        if (input.size() < 288) {
            output.resize(32, 0);
            return true; // Invalid input, return 0
        }
        
        size_t offset = 0;
        
        // Parse A (G1 point: x, y)
        G1Point a;
        a.x = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        a.y = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        
        // Parse B (G2 point: x0, x1, y0, y1)
        G2Point b;
        b.x0 = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        b.x1 = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        b.y0 = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        b.y1 = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        
        // Parse C (G1 point)
        G1Point c;
        c.x = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        c.y = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        
        // Parse number of public inputs
        UInt256 numInputsUint = UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32));
        offset += 32;
        uint64_t numInputs = numInputsUint.toUint64();
        
        // Parse public inputs
        std::vector<UInt256> publicInputs;
        for (uint64_t i = 0; i < numInputs && offset + 32 <= input.size(); ++i) {
            publicInputs.push_back(UInt256::fromBigEndianBytes(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + 32)));
            offset += 32;
        }
        
        Groth16Proof proof{a, b, c};
        VerificationKey vk; // Empty VK for now - production would load from storage or input
        vk.gamma_abc.resize(publicInputs.size() + 1); // Match size for validation
        
        bool valid = ZKVerifier::verifyGroth16(vk, proof, publicInputs);
        
        output.resize(32, 0);
        output[31] = valid ? 1 : 0;
        return true;
    }
    return false; // Unknown precompile = failure
}

UInt256 VM::getStackTop() const {
    if (stack.empty()) return UInt256(0);
    return stack.back();
}



}
