#include <iostream>
#include <thread>
#include <chrono>
#include "core/mempool.h"
#include "db/rocksdb_wrapper.h"
#include "db/state_manager.h"
#include "db/block_store.h"
#include "exec/execution_engine.h"
#include "consensus/leader.h"
#include "consensus/pbft.h"
#include "network/rpc_server.h"
#include "network/gossip.h"
#include "rpc/endpoints.h"
#include "tokens/token_manager.h"
#include "settlement/batch.h"
#include "settlement/bridge.h"
#include "settlement/kadena_client.h"
#include "util/crypto.h"

using namespace aegen;

int main() {
    std::cout << R"(
    ___                         
   /   | ___  ____ ____  ____  
  / /| |/ _ \/ __ `/ _ \/ __ \ 
 / ___ /  __/ /_/ /  __/ / / / 
/_/  |_\___/\__, /\___/_/ /_/  
           /____/              
    
  Layer-2 Blockchain on Kadena
  ============================
)" << std::endl;

    std::cout << "[INIT] Starting Aegen Node..." << std::endl;

    // 1. Initialize Core Components
    std::string dataDir = "aegen_data";
    RocksDBWrapper dbWrapper(dataDir + "/state");
    StateManager stateManager(dbWrapper);
    Mempool mempool;
    ExecutionEngine execEngine(stateManager);
    TokenManager tokenManager;
    BlockStore blockStore(dataDir);  // Persistent block storage
    RPCServer rpcServer;

    // 2. Setup RPC Endpoints (now with TokenManager and BlockStore)
    RPCEndpoints endpoints(mempool, stateManager, tokenManager, rpcServer);
    endpoints.setBlockStore(&blockStore);  // Connect block store
    endpoints.registerAll();

    // 3. Start RPC Server
    int rpcPort = 8545;
    rpcServer.start(rpcPort);

    // 4. Initialize Consensus
    std::string nodeId = "node-1";
    std::vector<std::string> validators = {nodeId}; // Single node for now
    PBFT consensus(nodeId, validators);

    // 5. Setup P2P Networking
    Gossip gossip;
    int p2pPort = 30303;
    gossip.start(p2pPort);

    // 6. Setup Settlement Bridge
    BatchManager batchManager;
    KadenaClient kadenaClient;
    SettlementBridge bridge(kadenaClient);

    // 7. Initialize Leader
    KeyPair leaderKeys = Wallet::generateKeyPair();
    Leader leader(mempool, execEngine, stateManager, leaderKeys, leaderKeys.address);

    // 8. Genesis State
    stateManager.setAccountState("alice", {0, 10000000}); // 10M native tokens
    stateManager.setAccountState("bob", {0, 10000000});
    
    // Create genesis token (fungible-v2 style)
    TokenId genesisToken = tokenManager.createFungible("Aegen Token", "AE", 12, 1000000000, "k:genesis");
    
    std::cout << "\n[GENESIS] State initialized:" << std::endl;
    std::cout << "  - alice: 10,000,000 AE" << std::endl;
    std::cout << "  - bob:   10,000,000 AE" << std::endl;
    std::cout << "  - Genesis Token: " << genesisToken << std::endl;
    std::cout << "\n[READY] Node running on:" << std::endl;
    std::cout << "  - RPC: http://localhost:" << rpcPort << std::endl;
    std::cout << "  - P2P: port " << p2pPort << std::endl;
    std::cout << "  - Explorer: file://explorer/index.html" << std::endl;
    std::cout << "\nPress Ctrl+C to stop.\n" << std::endl;

    uint64_t height = 1;
    Hash prevHash = {};

    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (mempool.size() > 0) {
            std::cout << "\n[BLOCK " << height << "] Producing block with " << mempool.size() << " txs..." << std::endl;
            
            Block block = leader.proposeBlock(height, 0, prevHash);
            
            // PBFT Consensus (simplified for single node)
            consensus.onPrePrepare(block);
            
            std::cout << "[BLOCK " << height << "] Finalized! Txs: " << block.transactions.size() 
                      << " | StateRoot: " << crypto::to_hex(block.header.stateRoot).substr(0, 16) << "..." << std::endl;
            
            // Store finalized block
            blockStore.addBlock(block);
            
            // Broadcast to P2P network
            gossip.broadcastBlock(block);
            
            // Add to batch
            batchManager.addBlock(block);
            if (batchManager.shouldBatch()) {
                std::cout << "\n[SETTLEMENT] Creating batch..." << std::endl;
                Batch batch = batchManager.createBatch();
                bridge.settleBatch(batch);
            }
            
            prevHash = block.calculateHash();
            height++;
        }
    }

    return 0;
}
