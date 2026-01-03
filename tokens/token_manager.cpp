#include "token_manager.h"
#include "util/crypto.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>

namespace aegen {

static std::string generateTokenId() {
    // Kadena-style module name format
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dist(0, 255);
    
    std::stringstream ss;
    ss << "coin.";  // Namespace prefix (Kadena style)
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dist(gen);
    }
    return ss.str();
}

TokenId TokenManager::createFungible(const std::string& name, const std::string& symbol,
                                      uint8_t precision, uint64_t initialSupply, 
                                      const Address& creator) {
    TokenInfo info;
    info.id = generateTokenId();
    info.name = name;
    info.symbol = symbol;
    info.precision = precision;
    info.totalSupply = initialSupply;
    info.creator = creator;
    info.createdAt = std::time(nullptr);
    info.uri = "";
    
    tokens[info.id] = info;
    
    // Credit initial supply to creator (like Pact coin.create-account + mint)
    if (initialSupply > 0) {
        balances[balanceKey(info.id, creator)] = initialSupply;
    }
    
    return info.id;
}

uint64_t TokenManager::getBalance(const TokenId& token, const Address& account) const {
    std::string key = token + ":" + account;
    auto it = balances.find(key);
    if (it == balances.end()) return 0;
    return it->second;
}

TransferResult TokenManager::transfer(const TokenId& token, const Address& sender,
                                       const Address& receiver, uint64_t amount) {
    TransferResult result;
    result.success = false;
    
    auto it = tokens.find(token);
    if (it == tokens.end()) {
        result.message = "Token not found";
        return result;
    }
    
    std::string senderKey = balanceKey(token, sender);
    std::string receiverKey = balanceKey(token, receiver);
    
    if (balances[senderKey] < amount) {
        result.message = "Insufficient balance";
        return result;
    }
    
    // Perform transfer (atomic in Pact via capabilities)
    balances[senderKey] -= amount;
    balances[receiverKey] += amount;
    
    result.success = true;
    result.message = "Transfer successful";
    
    // Generate tx hash
    std::vector<uint8_t> data;
    std::string txData = sender + receiver + std::to_string(amount);
    data.assign(txData.begin(), txData.end());
    auto hash = crypto::sha256_bytes(data);
    std::copy(hash.begin(), hash.end(), result.txHash.begin());
    
    return result;
}

TransferResult TokenManager::transferCreate(const TokenId& token, const Address& sender,
                                             const Address& receiver, uint64_t amount,
                                             const Address& guard) {
    // In Pact, transfer-create creates the receiver account if it doesn't exist
    // with the specified guard/keyset
    return transfer(token, sender, receiver, amount);
}

bool TokenManager::rotateGuard(const TokenId& token, const Address& account, const Address& newGuard) {
    // Placeholder for guard rotation (Pact keyset management)
    return true;
}

std::optional<TokenInfo> TokenManager::details(const TokenId& token) const {
    auto it = tokens.find(token);
    if (it == tokens.end()) return std::nullopt;
    return it->second;
}

uint8_t TokenManager::precision(const TokenId& token) const {
    auto it = tokens.find(token);
    if (it == tokens.end()) return 0;
    return it->second.precision;
}

uint64_t TokenManager::totalSupply(const TokenId& token) const {
    auto it = tokens.find(token);
    if (it == tokens.end()) return 0;
    return it->second.totalSupply;
}

bool TokenManager::mint(const TokenId& token, const Address& account, uint64_t amount, const Address& minter) {
    auto it = tokens.find(token);
    if (it == tokens.end()) return false;
    
    // Only creator can mint (governance check)
    if (it->second.creator != minter) return false;
    
    it->second.totalSupply += amount;
    balances[balanceKey(token, account)] += amount;
    
    return true;
}

bool TokenManager::burn(const TokenId& token, const Address& account, uint64_t amount) {
    auto it = tokens.find(token);
    if (it == tokens.end()) return false;
    
    std::string key = balanceKey(token, account);
    if (balances[key] < amount) return false;
    
    balances[key] -= amount;
    it->second.totalSupply -= amount;
    
    return true;
}

std::vector<TokenInfo> TokenManager::listTokens() const {
    std::vector<TokenInfo> list;
    for (const auto& [id, info] : tokens) {
        list.push_back(info);
    }
    return list;
}

std::vector<TokenBalance> TokenManager::getAccountBalances(const Address& account) const {
    std::vector<TokenBalance> result;
    for (const auto& [key, balance] : balances) {
        size_t pos = key.find(':');
        if (pos != std::string::npos) {
            std::string tokenId = key.substr(0, pos);
            std::string owner = key.substr(pos + 1);
            if (owner == account && balance > 0) {
                result.push_back({tokenId, owner, balance, ""});
            }
        }
    }
    return result;
}

}
