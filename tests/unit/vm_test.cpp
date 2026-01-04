#include <iostream>
#include <cassert>
#include <vector>
#include <map>
#include "exec/vm.h"
#include "exec/storage_interface.h"
#include "util/uint256.h"

using namespace aegen;

class MockStorage : public StorageInterface {
public:
    std::map<std::string, UInt256> db;

    void setStorage(const UInt256& contractAddr, const UInt256& key, const UInt256& value) override {
        std::string dbKey = contractAddr.toHex() + "_" + key.toHex();
        db[dbKey] = value;
    }

    UInt256 getStorage(const UInt256& contractAddr, const UInt256& key) const override {
        std::string dbKey = contractAddr.toHex() + "_" + key.toHex();
        if (db.count(dbKey)) return db.at(dbKey);
        return UInt256(0);
    }
};

void test_uint256() {
    std::cout << "Testing UInt256..." << std::endl;
    UInt256 a(100);
    UInt256 b(50);
    assert((a + b).toUint64() == 150);
    std::cout << "UInt256 PASS" << std::endl;
}

void test_evm_ops() {
    std::cout << "Testing EVM Operations..." << std::endl;
    VM vm;
    CallContext ctx;
    ctx.gasLimit = 100000;
    
    // 1. ADD
    std::vector<uint8_t> code1 = { (uint8_t)OpCode::PUSH1, 0x10, (uint8_t)OpCode::PUSH1, 0x20, (uint8_t)OpCode::ADD, (uint8_t)OpCode::STOP };
    auto res1 = vm.execute(code1, ctx);
    assert(res1.success);
    assert(vm.getStackTop().toUint64() == 0x30);
    std::cout << "EVM Operations PASS" << std::endl;
}

void test_evm_storage() {
    std::cout << "Testing EVM Storage..." << std::endl;
    MockStorage storage;
    VM vm(&storage);
    CallContext ctx;
    ctx.gasLimit = 100000;
    ctx.address = UInt256(123); 
    
    // SSTORE(1, 0xAA)
    std::vector<uint8_t> code = { (uint8_t)OpCode::PUSH1, 0xAA, (uint8_t)OpCode::PUSH1, 0x01, (uint8_t)OpCode::SSTORE, (uint8_t)OpCode::STOP };
    auto res = vm.execute(code, ctx);
    assert(res.success);
    assert(storage.getStorage(ctx.address, UInt256(1)).toUint64() == 0xAA);
    std::cout << "EVM Storage PASS" << std::endl;
}

void test_zk_precompile() {
    std::cout << "Testing ZK Precompile (0x09)..." << std::endl;
    VM vm;
    CallContext ctx;
    ctx.gasLimit = 1000000; // High gas for zk
    
    // Call precompile 0x09
    // STATICCALL(gas, addr, argsOff, argsSize, retOff, retSize)
    // Addr = 9
    // gas = 50000
    // args = 0 size 0 (for now)
    // ret = 0 size 32
    
    std::vector<uint8_t> code = {
        (uint8_t)OpCode::PUSH1, 32, // retSize
        (uint8_t)OpCode::PUSH1, 0,  // retOff
        (uint8_t)OpCode::PUSH1, 0,  // argsSize
        (uint8_t)OpCode::PUSH1, 0,  // argsOff
        (uint8_t)OpCode::PUSH1, 9,  // Addr (0x09)
        (uint8_t)OpCode::PUSH32,    // Gas (large enough)
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0x20,0x00, // ~8192 ? No wait 0x2000 is 8192. 
        // 50000 is C350. Let's push big enough.
        (uint8_t)OpCode::STATICCALL,
        // Now check return val in memory
        (uint8_t)OpCode::POP, // Success flag
        (uint8_t)OpCode::PUSH1, 0, // Offset 0
        (uint8_t)OpCode::MLOAD,    // Load result
        (uint8_t)OpCode::STOP
    };
    
    // Fix PUSH32 above: 0x2000 is not enough if verify cost is 50000. 
    // Let's use PUSH1 0xFF (255) repeated? No.
    // Just PUSH32 0...00 01 00 00 -> 65536. 
    code[28] = 0x01; // making gas argument 0x010000... no wait big endian. 
    // code indexes 6 to 37 are args for PUSH32.
    // gas is last arg of STATICCALL, so top of stack.
    // Order: gas, addr.... 
    
    // Let's rebuild code cleanly
    std::vector<uint8_t> cleanCode;
    auto push1 = [&](uint8_t v) { cleanCode.push_back((uint8_t)OpCode::PUSH1); cleanCode.push_back(v); };
    
    push1(32); // retSize
    push1(0);  // retOff
    push1(0);  // argsSize
    push1(0);  // argsOff
    push1(9);  // Addr 0x09
    
    // Gas: PUSH32 ... 01 00 00 (65536)
    cleanCode.push_back((uint8_t)OpCode::PUSH32);
    for(int i=0; i<29; i++) cleanCode.push_back(0); 
    cleanCode.push_back(0x01); // 00 00
    cleanCode.push_back(0x00);
    cleanCode.push_back(0x00);
    
    cleanCode.push_back((uint8_t)OpCode::STATICCALL);
    
    // Stack has Success(1) or 0.
    // We also want to check Memory[0..32].
    
    // Check stack top (success)
    cleanCode.push_back((uint8_t)OpCode::JUMPDEST); // Just a marker
    cleanCode.push_back((uint8_t)OpCode::STOP);
    
    ExecutionResult res = vm.execute(cleanCode, ctx);
    
    if (!res.success) std::cout << "Error: " << res.error << std::endl;
    assert(res.success);
    assert(vm.getStackTop().toUint64() == 1); // Call success
    
    std::cout << "ZK Precompile PASS" << std::endl;
}

int main() {
    try {
        test_uint256();
        test_evm_ops();
        test_evm_storage();
        test_zk_precompile();
        std::cout << "ALL TESTS PASSED" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
