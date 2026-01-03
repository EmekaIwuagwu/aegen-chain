#pragma once
#include "core/types.h"
#include "token_manager.h"
#include <map>

namespace aegen {

class TokenTransfer {
private:
    // Shared state - in production this would be managed by StateManager
    static std::map<std::string, uint64_t>& getBalances();
    static std::map<std::string, uint64_t>& getAllowances();
    
    static std::string makeBalanceKey(const TokenId& token, const Address& addr);
    static std::string makeAllowanceKey(const TokenId& token, const Address& owner, const Address& spender);

public:
    static bool transfer(const TokenId& token, const Address& from, const Address& to, uint64_t amount);
    static bool transferFrom(const TokenId& token, const Address& from, const Address& to, uint64_t amount, const Address& spender);
    static bool approve(const TokenId& token, const Address& owner, const Address& spender, uint64_t amount);
    static uint64_t getBalance(const TokenId& token, const Address& addr);
    static uint64_t getAllowance(const TokenId& token, const Address& owner, const Address& spender);
    static void setBalance(const TokenId& token, const Address& addr, uint64_t amount);
};

}
