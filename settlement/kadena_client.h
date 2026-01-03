#pragma once
#include <string>
#include <vector>
#include <map>

namespace aegen {

struct ChainwebConfig {
    std::string baseUrl = "https://api.chainweb.com";
    std::string networkId = "mainnet01";  // or "testnet04"
    std::string chainId = "0";
    std::string senderAccount;
    std::string senderPublicKey;
    std::string senderPrivateKey;
    uint64_t gasLimit = 100000;
    double gasPrice = 0.00000001;
};

struct PactCommand {
    std::string cmd;      // JSON stringified command
    std::string hash;     // Blake2b hash of cmd
    std::vector<std::string> sigs;  // Signatures
};

struct PactResult {
    bool success;
    std::string requestKey;
    std::string result;
    std::string error;
    uint64_t gas;
};

class KadenaClient {
    ChainwebConfig config;
    
    std::string buildPactPayload(const std::string& pactCode, const std::map<std::string, std::string>& envData);
    std::string signPayload(const std::string& payload);
    std::string httpPost(const std::string& url, const std::string& body);

public:
    KadenaClient();
    explicit KadenaClient(const ChainwebConfig& cfg);
    
    void setConfig(const ChainwebConfig& cfg);
    ChainwebConfig getConfig() const { return config; }
    
    // Submit Pact command to Kadena
    PactResult submitPactCmd(const std::string& pactCode);
    
    // Poll for transaction result
    PactResult pollResult(const std::string& requestKey);
    
    // Submit batch settlement
    PactResult settleBatch(const std::string& batchId, const std::string& stateRoot, uint64_t blockCount);
    
    // Check if connected to Kadena
    bool testConnection();
};

}
