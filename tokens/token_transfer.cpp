#include "token_transfer.h"

namespace aegen {

// Static storage
static std::map<std::string, uint64_t> balanceStore;
static std::map<std::string, uint64_t> allowanceStore;

std::map<std::string, uint64_t>& TokenTransfer::getBalances() {
    return balanceStore;
}

std::map<std::string, uint64_t>& TokenTransfer::getAllowances() {
    return allowanceStore;
}

std::string TokenTransfer::makeBalanceKey(const TokenId& token, const Address& addr) {
    return token + ":" + addr;
}

std::string TokenTransfer::makeAllowanceKey(const TokenId& token, const Address& owner, const Address& spender) {
    return token + ":" + owner + ":" + spender;
}

uint64_t TokenTransfer::getBalance(const TokenId& token, const Address& addr) {
    std::string key = makeBalanceKey(token, addr);
    auto it = getBalances().find(key);
    return (it != getBalances().end()) ? it->second : 0;
}

void TokenTransfer::setBalance(const TokenId& token, const Address& addr, uint64_t amount) {
    std::string key = makeBalanceKey(token, addr);
    getBalances()[key] = amount;
}

uint64_t TokenTransfer::getAllowance(const TokenId& token, const Address& owner, const Address& spender) {
    std::string key = makeAllowanceKey(token, owner, spender);
    auto it = getAllowances().find(key);
    return (it != getAllowances().end()) ? it->second : 0;
}

bool TokenTransfer::approve(const TokenId& token, const Address& owner, const Address& spender, uint64_t amount) {
    std::string key = makeAllowanceKey(token, owner, spender);
    getAllowances()[key] = amount;
    return true;
}

bool TokenTransfer::transfer(const TokenId& token, const Address& from, const Address& to, uint64_t amount) {
    // Check sender balance
    uint64_t fromBalance = getBalance(token, from);
    if (fromBalance < amount) {
        return false; // Insufficient balance
    }
    
    // Update balances
    setBalance(token, from, fromBalance - amount);
    uint64_t toBalance = getBalance(token, to);
    setBalance(token, to, toBalance + amount);
    
    return true;
}

bool TokenTransfer::transferFrom(const TokenId& token, const Address& from, const Address& to, uint64_t amount, const Address& spender) {
    // Check allowance
    uint64_t currentAllowance = getAllowance(token, from, spender);
    if (currentAllowance < amount) {
        return false; // Insufficient allowance
    }
    
    // Check sender balance
    uint64_t fromBalance = getBalance(token, from);
    if (fromBalance < amount) {
        return false; // Insufficient balance
    }
    
    // Update allowance
    std::string allowanceKey = makeAllowanceKey(token, from, spender);
    getAllowances()[allowanceKey] = currentAllowance - amount;
    
    // Update balances
    setBalance(token, from, fromBalance - amount);
    uint64_t toBalance = getBalance(token, to);
    setBalance(token, to, toBalance + amount);
    
    return true;
}

}
