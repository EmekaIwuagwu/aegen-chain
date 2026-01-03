#pragma once
#include "core/mempool.h"
#include "db/state_manager.h"
#include "db/block_store.h"
#include "tokens/token_manager.h"
#include "network/rpc_server.h"

namespace aegen {

class RPCEndpoints {
    Mempool& mempool;
    StateManager& stateManager;
    TokenManager& tokenManager;
    RPCServer& server;
    BlockStore* blockStore = nullptr;

public:
    RPCEndpoints(Mempool& mp, StateManager& sm, TokenManager& tm, RPCServer& srv);
    void setBlockStore(BlockStore* store) { blockStore = store; }
    void registerAll();

    // Transaction Handlers
    std::string handleSendTransaction(const std::string& json);
    std::string handleGetBalance(const std::string& json);
    std::string handleGetChainInfo(const std::string& json);
    std::string handleGetNonce(const std::string& json);
    
    // Token Handlers
    std::string handleCreateToken(const std::string& json);
    std::string handleTokenTransfer(const std::string& json);
    std::string handleGetTokenBalance(const std::string& json);
    std::string handleListTokens(const std::string& json);
    std::string handleMintToken(const std::string& json);
    
    // Explorer Handlers
    std::string handleGetBlocks(const std::string& json);
    std::string handleGetBlock(const std::string& json);
    std::string handleGetTransactions(const std::string& json);
    std::string handleGetTransaction(const std::string& json);
    std::string handleGenerateWallet(const std::string& json);
    std::string handleGetMetrics(const std::string& json);
};

}
