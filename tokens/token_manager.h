#pragma once
#include "core/types.h"
#include <map>
#include <optional>
#include <vector>

namespace aegen {

// Kadena/Pact fungible-v2 compatible token interface
struct TokenInfo {
    TokenId id;
    std::string name;
    std::string symbol;
    uint8_t precision;  // Kadena uses "precision" not "decimals"
    uint64_t totalSupply;
    Address creator;    // Token creator (has governance rights)
    uint64_t createdAt;
    std::string uri;    // Optional metadata URI
};

struct TokenBalance {
    TokenId tokenId;
    Address account;    // Kadena uses "account" terminology
    uint64_t balance;
    Address guard;      // Guard/keyset for the account (Pact concept)
};

// Pact-style transfer result
struct TransferResult {
    bool success;
    std::string message;
    Hash txHash;
};

class TokenManager {
    std::map<TokenId, TokenInfo> tokens;
    std::map<std::string, uint64_t> balances; // key: tokenId:account
    
    std::string balanceKey(const TokenId& token, const Address& account) const {
        return token + ":" + account;
    }

public:
    // ========================================
    // fungible-v2 Interface (Pact Standard)
    // ========================================
    
    // Create a new fungible token (like deploying a Pact module)
    TokenId createFungible(const std::string& name, const std::string& symbol, 
                           uint8_t precision, uint64_t initialSupply, 
                           const Address& creator);
    
    // Get token balance for account (fungible-v2: get-balance)
    uint64_t getBalance(const TokenId& token, const Address& account) const;
    
    // Transfer tokens (fungible-v2: transfer)
    TransferResult transfer(const TokenId& token, const Address& sender, 
                            const Address& receiver, uint64_t amount);
    
    // Transfer with capability (fungible-v2: transfer-create)
    TransferResult transferCreate(const TokenId& token, const Address& sender,
                                  const Address& receiver, uint64_t amount,
                                  const Address& guard);
    
    // Rotate guard/keyset for account
    bool rotateGuard(const TokenId& token, const Address& account, const Address& newGuard);
    
    // Get token details (fungible-v2: details)
    std::optional<TokenInfo> details(const TokenId& token) const;
    
    // Get token precision
    uint8_t precision(const TokenId& token) const;
    
    // Get total supply
    uint64_t totalSupply(const TokenId& token) const;
    
    // ========================================
    // Governance Functions (Creator Only)
    // ========================================
    
    bool mint(const TokenId& token, const Address& account, uint64_t amount, const Address& minter);
    bool burn(const TokenId& token, const Address& account, uint64_t amount);
    
    // ========================================
    // Query Functions
    // ========================================
    
    std::vector<TokenInfo> listTokens() const;
    std::vector<TokenBalance> getAccountBalances(const Address& account) const;
};

}
