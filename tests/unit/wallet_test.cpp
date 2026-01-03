#include "wallet/keypair.h"
#include "wallet/signer.h"
#include "util/crypto.h"
#include <cassert>
#include <iostream>

using namespace aegen;

void test_key_generation() {
    KeyPair kp = Wallet::generateKeyPair();
    
    // Kadena address format: k:<64 hex chars>
    assert(!kp.address.empty());
    assert(kp.address.substr(0, 2) == "k:");
    assert(kp.address.length() == 66); // k: + 64 hex chars
    assert(kp.privateKey.size() == 32);
    assert(kp.publicKey.size() == 32);
    
    std::cout << "Generated Kadena Address: " << kp.address << std::endl;
    std::cout << "test_key_generation: PASSED" << std::endl;
}

void test_address_validation() {
    // Valid Kadena addresses
    assert(Wallet::validateAddress("k:1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
    assert(Wallet::validateAddress("w:abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"));
    assert(Wallet::validateAddress("alice"));
    assert(Wallet::validateAddress("bob-account"));
    assert(Wallet::validateAddress("my_wallet_123"));
    
    // Invalid addresses
    assert(!Wallet::validateAddress("0x1234567890abcdef1234567890abcdef12345678")); // Ethereum format!
    assert(!Wallet::validateAddress("k:1234")); // Too short
    assert(!Wallet::validateAddress("ab")); // Too short for simple name
    assert(!Wallet::validateAddress("k:123456789g")); // Invalid hex char
    
    std::cout << "test_address_validation: PASSED" << std::endl;
}

void test_signing() {
    KeyPair kp = Wallet::generateKeyPair();
    
    Bytes message = {'H', 'e', 'l', 'l', 'o'};
    Signature sig = Signer::sign(message, kp.privateKey);
    
    assert(sig.size() == 64);
    
    // Verify signature
    bool valid = Signer::verify(message, sig, kp.publicKey);
    assert(valid);
    
    // Verify with wrong message fails
    Bytes wrongMessage = {'W', 'r', 'o', 'n', 'g'};
    bool invalid = Signer::verify(wrongMessage, sig, kp.publicKey);
    assert(!invalid);
    
    std::cout << "test_signing: PASSED" << std::endl;
}

void test_deterministic_keys() {
    // Same private key should always derive same public key and address
    std::vector<uint8_t> pk1 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
    auto pub1 = crypto::derive_public_key(pk1);
    auto pub2 = crypto::derive_public_key(pk1);
    
    assert(pub1 == pub2);
    
    auto addr1 = crypto::derive_kadena_address(pub1);
    auto addr2 = crypto::derive_kadena_address(pub2);
    assert(addr1 == addr2);
    assert(addr1.substr(0, 2) == "k:");
    
    std::cout << "test_deterministic_keys: PASSED" << std::endl;
}

int main() {
    try {
        test_key_generation();
        test_address_validation();
        test_signing();
        test_deterministic_keys();
        std::cout << "\nAll Kadena wallet tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
