#include "endpoints.h"
#include "util/crypto.h"
#include "wallet/keypair.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>

namespace aegen {

RPCEndpoints::RPCEndpoints(Mempool& mp, StateManager& sm, TokenManager& tm, RPCServer& srv) 
    : mempool(mp), stateManager(sm), tokenManager(tm), server(srv) {}

void RPCEndpoints::registerAll() {
    // Native Token Operations
    server.registerEndpoint("sendTransaction", [this](const std::string& json) {
        return this->handleSendTransaction(json);
    });
    server.registerEndpoint("getBalance", [this](const std::string& json) {
        return this->handleGetBalance(json);
    });
    server.registerEndpoint("getChainInfo", [this](const std::string& json) {
        return this->handleGetChainInfo(json);
    });
    server.registerEndpoint("getNonce", [this](const std::string& json) {
        return this->handleGetNonce(json);
    });
    
    // Pact fungible-v2 Token Operations
    server.registerEndpoint("createFungible", [this](const std::string& json) {
        return this->handleCreateToken(json);
    });
    server.registerEndpoint("transfer", [this](const std::string& json) {
        return this->handleTokenTransfer(json);
    });
    server.registerEndpoint("get-balance", [this](const std::string& json) {
        return this->handleGetTokenBalance(json);
    });
    server.registerEndpoint("details", [this](const std::string& json) {
        return this->handleListTokens(json);
    });
    server.registerEndpoint("mint", [this](const std::string& json) {
        return this->handleMintToken(json);
    });
    
    // Explorer Endpoints
    server.registerEndpoint("getBlocks", [this](const std::string& json) {
        return this->handleGetBlocks(json);
    });
    server.registerEndpoint("getBlock", [this](const std::string& json) {
        return this->handleGetBlock(json);
    });
    server.registerEndpoint("getTransactions", [this](const std::string& json) {
        return this->handleGetTransactions(json);
    });
    server.registerEndpoint("getTransaction", [this](const std::string& json) {
        return this->handleGetTransaction(json);
    });
    server.registerEndpoint("generateWallet", [this](const std::string& json) {
        return this->handleGenerateWallet(json);
    });
    
    // Metrics & Health Endpoints
    server.registerEndpoint("getMetrics", [this](const std::string& json) {
        return this->handleGetMetrics(json);
    });
    server.registerEndpoint("health", [this](const std::string& json) {
        return "{\"status\": \"healthy\", \"version\": \"1.0.0\"}";
    });
}

std::string extractJsonValue(const std::string& json, const std::string& key) {
    std::string token = "\"" + key + "\":";
    size_t pos = json.find(token);
    if (pos == std::string::npos) return "";
    
    size_t valStart = pos + token.length();
    while(valStart < json.length() && (json[valStart] == ' ' || json[valStart] == '"' || json[valStart] == ':')) valStart++;
    
    size_t valEnd = valStart;
    while(valEnd < json.length() && json[valEnd] != ',' && json[valEnd] != '}' && json[valEnd] != '"') valEnd++;
    
    return json.substr(valStart, valEnd - valStart);
}

std::string RPCEndpoints::handleSendTransaction(const std::string& json) {
    // Accept both "from/to" and "sender/receiver" param names
    Address sender = extractJsonValue(json, "sender");
    if (sender.empty()) sender = extractJsonValue(json, "from");
    
    Address receiver = extractJsonValue(json, "receiver");
    if (receiver.empty()) receiver = extractJsonValue(json, "to");
    
    std::string amtStr = extractJsonValue(json, "amount");
    std::string nonceStr = extractJsonValue(json, "nonce");
    
    if (sender.empty() || receiver.empty() || amtStr.empty()) 
        return "{\"error\": \"Invalid params - need from/to/amount or sender/receiver/amount\"}";

    Transaction tx;
    tx.sender = sender;
    tx.receiver = receiver;
    tx.amount = std::stoull(amtStr);
    tx.nonce = nonceStr.empty() ? 0 : std::stoull(nonceStr);
    tx.gasLimit = 21000; 
    tx.gasPrice = 1;
    tx.calculateHash();
    
    mempool.add(tx);
    return "{\"result\": {\"requestKey\": \"" + crypto::to_hex(tx.hash) + "\"}}";
}

std::string RPCEndpoints::handleGetBalance(const std::string& json) {
    Address address = extractJsonValue(json, "account");
    if (address.empty()) address = extractJsonValue(json, "address");
    
    AccountState state = stateManager.getAccountState(address);
    return "{\"result\": " + std::to_string(state.balance) + "}";
}

std::string RPCEndpoints::handleGetNonce(const std::string& json) {
    Address address = extractJsonValue(json, "account");
    if (address.empty()) address = extractJsonValue(json, "address");
    
    AccountState state = stateManager.getAccountState(address);
    return "{\"result\": " + std::to_string(state.nonce) + "}";
}

std::string RPCEndpoints::handleGetChainInfo(const std::string& json) {
    std::stringstream ss;
    ss << "{\"result\": {";
    ss << "\"networkId\": \"aegen-l2\",";
    ss << "\"chainId\": \"0\",";
    ss << "\"nodeVersion\": \"1.0.0\",";
    ss << "\"blockHeight\": " << (blockStore ? blockStore->getHeight() : 0) << ",";
    ss << "\"mempoolSize\": " << mempool.size() << ",";
    ss << "\"peerCount\": 3,";
    ss << "\"totalTransactions\": " << (blockStore ? blockStore->getTotalTransactions() : 0) << ",";
    ss << "\"tokenCount\": " << tokenManager.listTokens().size() << ",";
    ss << "\"l1Network\": \"kadena-mainnet\"";
    ss << "}}";
    return ss.str();
}

std::string RPCEndpoints::handleCreateToken(const std::string& json) {
    std::string name = extractJsonValue(json, "name");
    std::string symbol = extractJsonValue(json, "symbol");
    std::string precisionStr = extractJsonValue(json, "precision");
    std::string supplyStr = extractJsonValue(json, "initialSupply");
    Address creator = extractJsonValue(json, "creator");
    
    if (name.empty() || symbol.empty() || creator.empty())
        return "{\"error\": \"Missing required fields: name, symbol, creator\"}";
    
    uint8_t precision = precisionStr.empty() ? 12 : std::stoi(precisionStr);
    uint64_t supply = supplyStr.empty() ? 0 : std::stoull(supplyStr);
    
    TokenId tokenId = tokenManager.createFungible(name, symbol, precision, supply, creator);
    
    return "{\"result\": {\"module\": \"" + tokenId + "\", \"status\": \"deployed\"}}";
}

std::string RPCEndpoints::handleTokenTransfer(const std::string& json) {
    TokenId token = extractJsonValue(json, "token");
    Address sender = extractJsonValue(json, "sender");
    Address receiver = extractJsonValue(json, "receiver");
    std::string amtStr = extractJsonValue(json, "amount");
    
    if (token.empty() || sender.empty() || receiver.empty() || amtStr.empty())
        return "{\"error\": \"Missing required fields: token, sender, receiver, amount\"}";
    
    uint64_t amount = std::stoull(amtStr);
    
    auto result = tokenManager.transfer(token, sender, receiver, amount);
    
    if (result.success) {
        return "{\"result\": {\"status\": \"success\", \"requestKey\": \"" + crypto::to_hex(result.txHash) + "\"}}";
    } else {
        return "{\"error\": \"" + result.message + "\"}";
    }
}

std::string RPCEndpoints::handleGetTokenBalance(const std::string& json) {
    TokenId token = extractJsonValue(json, "token");
    Address account = extractJsonValue(json, "account");
    
    if (token.empty() || account.empty())
        return "{\"error\": \"Missing required fields: token, account\"}";
    
    uint64_t balance = tokenManager.getBalance(token, account);
    return "{\"result\": " + std::to_string(balance) + "}";
}

std::string RPCEndpoints::handleListTokens(const std::string& json) {
    auto tokens = tokenManager.listTokens();
    
    std::stringstream ss;
    ss << "{\"result\": [";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{";
        ss << "\"module\": \"" << tokens[i].id << "\",";
        ss << "\"name\": \"" << tokens[i].name << "\",";
        ss << "\"symbol\": \"" << tokens[i].symbol << "\",";
        ss << "\"precision\": " << (int)tokens[i].precision << ",";
        ss << "\"totalSupply\": " << tokens[i].totalSupply;
        ss << "}";
    }
    ss << "]}";
    return ss.str();
}

std::string RPCEndpoints::handleMintToken(const std::string& json) {
    TokenId token = extractJsonValue(json, "token");
    Address account = extractJsonValue(json, "account");
    Address minter = extractJsonValue(json, "minter");
    std::string amtStr = extractJsonValue(json, "amount");
    
    if (token.empty() || account.empty() || minter.empty() || amtStr.empty())
        return "{\"error\": \"Missing required fields: token, account, minter, amount\"}";
    
    uint64_t amount = std::stoull(amtStr);
    
    if (tokenManager.mint(token, account, amount, minter)) {
        return "{\"result\": {\"status\": \"success\"}}";
    } else {
        return "{\"error\": \"Mint failed - not authorized or invalid token\"}";
    }
}

// ===== Explorer Endpoints =====

std::string RPCEndpoints::handleGetBlocks(const std::string& json) {
    if (!blockStore) return "{\"result\": {\"blocks\": [], \"total\": 0}}";
    
    std::string pageStr = extractJsonValue(json, "page");
    std::string limitStr = extractJsonValue(json, "limit");
    
    uint64_t page = pageStr.empty() ? 1 : std::stoull(pageStr);
    uint64_t limit = limitStr.empty() ? 10 : std::stoull(limitStr);
    if (limit > 100) limit = 100;
    
    uint64_t total = blockStore->getHeight();
    uint64_t start = (page - 1) * limit + 1;
    auto blocks = blockStore->getBlocks(start, limit);
    
    std::stringstream ss;
    ss << "{\"result\": {\"blocks\": [";
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& block = blocks[i];
        
        // Get timestamp
        time_t now = std::time(nullptr);
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        
        ss << "{";
        ss << "\"height\": " << block.header.height << ",";
        ss << "\"hash\": \"" << crypto::to_hex(block.header.stateRoot) << "\",";
        ss << "\"parentHash\": \"" << crypto::to_hex(block.header.previousHash) << "\",";
        ss << "\"stateRoot\": \"" << crypto::to_hex(block.header.stateRoot) << "\",";
        ss << "\"txCount\": " << block.transactions.size() << ",";
        ss << "\"timestamp\": \"" << timestamp << "\",";
        ss << "\"validator\": \"validator-1\",";
        ss << "\"gasUsed\": " << (block.transactions.size() * 21000) << ",";
        ss << "\"gasLimit\": 100000";
        ss << "}";
    }
    
    ss << "], \"total\": " << total << ", \"page\": " << page << ", \"limit\": " << limit << "}}";
    return ss.str();
}

std::string RPCEndpoints::handleGetBlock(const std::string& json) {
    if (!blockStore) return "{\"error\": \"Block store not available\"}";
    
    std::string heightStr = extractJsonValue(json, "height");
    if (heightStr.empty()) return "{\"error\": \"Missing height parameter\"}";
    
    uint64_t height = std::stoull(heightStr);
    Block block = blockStore->getBlock(height);
    
    if (block.header.height == 0) {
        return "{\"error\": \"Block not found\"}";
    }
    
    time_t now = std::time(nullptr);
    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    
    std::stringstream ss;
    ss << "{\"result\": {";
    ss << "\"height\": " << block.header.height << ",";
    ss << "\"hash\": \"" << crypto::to_hex(block.header.stateRoot) << "\",";
    ss << "\"parentHash\": \"" << crypto::to_hex(block.header.previousHash) << "\",";
    ss << "\"stateRoot\": \"" << crypto::to_hex(block.header.stateRoot) << "\",";
    ss << "\"txCount\": " << block.transactions.size() << ",";
    ss << "\"timestamp\": \"" << timestamp << "\",";
    ss << "\"validator\": \"validator-1\",";
    ss << "\"gasUsed\": " << (block.transactions.size() * 21000) << ",";
    ss << "\"gasLimit\": 100000,";
    
    // Include transactions
    ss << "\"transactions\": [";
    for (size_t i = 0; i < block.transactions.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& tx = block.transactions[i];
        ss << "{";
        ss << "\"hash\": \"" << crypto::to_hex(tx.hash) << "\",";
        ss << "\"from\": \"" << tx.sender << "\",";
        ss << "\"to\": \"" << tx.receiver << "\",";
        ss << "\"amount\": " << tx.amount << ",";
        ss << "\"nonce\": " << tx.nonce << ",";
        ss << "\"gasUsed\": 21000,";
        ss << "\"status\": \"Success\"";
        ss << "}";
    }
    ss << "]}}";
    
    return ss.str();
}

std::string RPCEndpoints::handleGetTransactions(const std::string& json) {
    if (!blockStore) return "{\"result\": {\"transactions\": [], \"total\": 0}}";
    
    std::string pageStr = extractJsonValue(json, "page");
    std::string limitStr = extractJsonValue(json, "limit");
    
    uint64_t page = pageStr.empty() ? 1 : std::stoull(pageStr);
    uint64_t limit = limitStr.empty() ? 10 : std::stoull(limitStr);
    if (limit > 100) limit = 100;
    
    // Collect all transactions from blocks
    std::vector<std::pair<Transaction, uint64_t>> allTxs; // tx, block height
    uint64_t height = blockStore->getHeight();
    
    for (uint64_t h = height; h >= 1 && allTxs.size() < page * limit; --h) {
        Block block = blockStore->getBlock(h);
        for (const auto& tx : block.transactions) {
            allTxs.push_back({tx, h});
        }
    }
    
    uint64_t total = blockStore->getTotalTransactions();
    uint64_t startIdx = (page - 1) * limit;
    uint64_t endIdx = std::min(startIdx + limit, (uint64_t)allTxs.size());
    
    time_t now = std::time(nullptr);
    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    
    std::stringstream ss;
    ss << "{\"result\": {\"transactions\": [";
    
    for (size_t i = startIdx; i < endIdx; ++i) {
        if (i > startIdx) ss << ",";
        const auto& [tx, blockHeight] = allTxs[i];
        ss << "{";
        ss << "\"hash\": \"" << crypto::to_hex(tx.hash) << "\",";
        ss << "\"blockHeight\": " << blockHeight << ",";
        ss << "\"from\": \"" << tx.sender << "\",";
        ss << "\"to\": \"" << tx.receiver << "\",";
        ss << "\"amount\": " << tx.amount << ",";
        ss << "\"nonce\": " << tx.nonce << ",";
        ss << "\"gasUsed\": 21000,";
        ss << "\"gasPrice\": 0.00000001,";
        ss << "\"status\": \"Success\",";
        ss << "\"type\": \"Native Transfer\",";
        ss << "\"timestamp\": \"" << timestamp << "\"";
        ss << "}";
    }
    
    ss << "], \"total\": " << total << ", \"page\": " << page << ", \"limit\": " << limit << "}}";
    return ss.str();
}

std::string RPCEndpoints::handleGetTransaction(const std::string& json) {
    if (!blockStore) return "{\"error\": \"Block store not available\"}";
    
    std::string hash = extractJsonValue(json, "hash");
    if (hash.empty()) return "{\"error\": \"Missing hash parameter\"}";
    
    // Search through blocks for the transaction
    uint64_t height = blockStore->getHeight();
    for (uint64_t h = height; h >= 1; --h) {
        Block block = blockStore->getBlock(h);
        for (const auto& tx : block.transactions) {
            if (crypto::to_hex(tx.hash) == hash) {
                time_t now = std::time(nullptr);
                char timestamp[64];
                std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
                
                std::stringstream ss;
                ss << "{\"result\": {";
                ss << "\"hash\": \"" << crypto::to_hex(tx.hash) << "\",";
                ss << "\"blockHeight\": " << h << ",";
                ss << "\"from\": \"" << tx.sender << "\",";
                ss << "\"to\": \"" << tx.receiver << "\",";
                ss << "\"amount\": " << tx.amount << ",";
                ss << "\"nonce\": " << tx.nonce << ",";
                ss << "\"gasUsed\": 21000,";
                ss << "\"gasPrice\": 0.00000001,";
                ss << "\"fee\": 0.00021,";
                ss << "\"status\": \"Success\",";
                ss << "\"type\": \"Native Transfer\",";
                ss << "\"timestamp\": \"" << timestamp << "\"";
                ss << "}}";
                return ss.str();
            }
        }
    }
    
    return "{\"error\": \"Transaction not found\"}";
}

std::string RPCEndpoints::handleGenerateWallet(const std::string& json) {
    KeyPair kp = Wallet::generateKeyPair();
    
    std::stringstream ss;
    ss << "{\"result\": {";
    ss << "\"address\": \"" << kp.address << "\",";
    ss << "\"publicKey\": \"" << crypto::to_hex(kp.publicKey) << "\",";
    ss << "\"privateKey\": \"" << crypto::to_hex(kp.privateKey) << "\"";
    ss << "}}";
    return ss.str();
}

std::string RPCEndpoints::handleGetMetrics(const std::string& json) {
    std::stringstream ss;
    ss << "{\"result\": {";
    ss << "\"blocks_produced\": " << (blockStore ? blockStore->getHeight() : 0) << ",";
    ss << "\"transactions_processed\": " << (blockStore ? blockStore->getTotalTransactions() : 0) << ",";
    ss << "\"transactions_pending\": " << mempool.size() << ",";
    ss << "\"peers_connected\": 3,";
    ss << "\"tokens_created\": " << tokenManager.listTokens().size() << ",";
    ss << "\"uptime_seconds\": " << (std::time(nullptr) % 86400) << ",";
    ss << "\"version\": \"1.0.0\"";
    ss << "}}";
    return ss.str();
}

}
