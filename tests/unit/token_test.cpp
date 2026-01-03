#include "tokens/token_manager.h"
#include <cassert>
#include <iostream>

using namespace aegen;

void test_create_fungible() {
    TokenManager tm;
    Address creator = "k:creator-keyset";
    
    // Create fungible token (Pact style)
    TokenId id = tm.createFungible("Aegen Coin", "AEG", 12, 1000000, creator);
    
    assert(!id.empty());
    auto info = tm.details(id);
    assert(info.has_value());
    assert(info->name == "Aegen Coin");
    assert(info->symbol == "AEG");
    assert(info->precision == 12);  // Kadena uses "precision"
    assert(info->totalSupply == 1000000);
    assert(tm.getBalance(id, creator) == 1000000);
    
    std::cout << "test_create_fungible: PASSED" << std::endl;
}

void test_mint_burn() {
    TokenManager tm;
    Address creator = "k:creator-keyset";
    TokenId id = tm.createFungible("Test Token", "TST", 12, 1000, creator);
    
    // Mint requires minter to be creator (governance)
    bool success = tm.mint(id, creator, 500, creator);
    assert(success);
    assert(tm.getBalance(id, creator) == 1500);
    
    // Burn tokens
    success = tm.burn(id, creator, 200);
    assert(success);
    assert(tm.getBalance(id, creator) == 1300);
    
    // Verify total supply updated
    assert(tm.totalSupply(id) == 1300);
    
    std::cout << "test_mint_burn: PASSED" << std::endl;
}

void test_transfer() {
    TokenManager tm;
    Address alice = "k:alice-keyset";
    Address bob = "k:bob-keyset";
    TokenId id = tm.createFungible("Transfer Token", "TFR", 12, 1000, alice);
    
    assert(tm.getBalance(id, alice) == 1000);
    assert(tm.getBalance(id, bob) == 0);
    
    // Pact-style transfer
    auto result = tm.transfer(id, alice, bob, 300);
    assert(result.success);
    assert(tm.getBalance(id, alice) == 700);
    assert(tm.getBalance(id, bob) == 300);
    
    // Transfer more than balance should fail
    result = tm.transfer(id, bob, alice, 500);
    assert(!result.success);
    assert(result.message == "Insufficient balance");
    assert(tm.getBalance(id, bob) == 300);
    
    std::cout << "test_transfer: PASSED" << std::endl;
}

void test_precision() {
    TokenManager tm;
    Address creator = "k:creator";
    
    // Kadena default precision is 12
    TokenId id = tm.createFungible("KDA Clone", "KDA2", 12, 0, creator);
    assert(tm.precision(id) == 12);
    
    // Custom precision
    TokenId id2 = tm.createFungible("Low Precision", "LP", 2, 0, creator);
    assert(tm.precision(id2) == 2);
    
    std::cout << "test_precision: PASSED" << std::endl;
}

int main() {
    try {
        test_create_fungible();
        test_mint_burn();
        test_transfer();
        test_precision();
        std::cout << "\nAll fungible-v2 tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
