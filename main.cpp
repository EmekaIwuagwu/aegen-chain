#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>
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
#include "wallet/signer.h"
#include "util/crypto.h"

using namespace aegen;

// Helper to split string
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    // Defaults
    std::string nodeId = "node-1";
    int rpcPort = 8545;
    int p2pPort = 30303;
    std::string peersStr = "";
    std::string dataDir = "aegen_data";
    
    // Parse arguments
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg == "--node" && i + 1 < argc) nodeId = argv[++i];
        else if(arg == "--rpc" && i + 1 < argc) rpcPort = std::stoi(argv[++i]);
        else if(arg == "--p2p" && i + 1 < argc) p2pPort = std::stoi(argv[++i]);
        else if(arg == "--peers" && i + 1 < argc) peersStr = argv[++i];
        else if(arg == "--data" && i + 1 < argc) dataDir = argv[++i];
    }

    std::cout << "[INIT] " << nodeId << " (RPC: " << rpcPort << ", P2P: " << p2pPort << ")" << std::endl;

    // Core Components
    RocksDBWrapper dbWrapper(dataDir + "/state");
    StateManager stateManager(dbWrapper);
    Mempool mempool;
    ExecutionEngine execEngine(stateManager);
    TokenManager tokenManager;
    BlockStore blockStore(dataDir);
    RPCServer rpcServer;

    RPCEndpoints endpoints(mempool, stateManager, tokenManager, rpcServer);
    endpoints.setBlockStore(&blockStore);
    endpoints.setExecutionEngine(&execEngine);
    endpoints.registerAll();

    rpcServer.start(rpcPort);

    // Multi-validator Setup
    std::vector<std::string> validators = {"node-1"};
    PBFT consensus(nodeId, validators);
    Gossip gossip;
    gossip.start(p2pPort);
    
    if (!peersStr.empty()) {
        auto peersList = split(peersStr, ',');
        for (const auto& peerAddr : peersList) {
            auto parts = split(peerAddr, ':');
            if (parts.size() == 2) {
                PeerInfo p;
                p.host = parts[0];
                p.port = std::stoi(parts[1]);
                gossip.addPeer(p);
            }
        }
    }

    BatchManager batchManager;
    KadenaClient kadenaClient;
    SettlementBridge bridge(kadenaClient);

    KeyPair leaderKeys = Wallet::generateKeyPair();
    Leader leader(mempool, execEngine, stateManager, leaderKeys, leaderKeys.address);

    // Genesis
    stateManager.setAccountState("alice", {0, 10000000});
    stateManager.setAccountState("bob", {0, 10000000});
    TokenId genesisToken = tokenManager.createFungible("Aegen Token", "AE", 12, 1000000000, "k:genesis");
    
    Block genesisBlock;
    genesisBlock.header.height = 0;
    genesisBlock.header.timestamp = 1704351600;
    genesisBlock.header.previousHash = {};
    genesisBlock.header.stateRoot = stateManager.getRootHash();
    genesisBlock.header.producer = "genesis";
    Hash genesisHash = genesisBlock.calculateHash();
    blockStore.addBlock(genesisBlock);

    // Chain State protected by mutex
    std::mutex chainMutex;
    uint64_t height = 1;
    Hash prevHash = genesisHash;
    uint64_t lastBlockTime = std::time(nullptr);

    // ------------------------------------------------------------------------
    // Consensus Wiring
    // ------------------------------------------------------------------------

    // 1. Broadcast Vote (Outbound)
    consensus.broadcastVote = [&](const Vote& vote, const std::string& type) {
        NetworkMessage msg;
        msg.type = MessageType::VOTE;
        msg.timestamp = std::time(nullptr);
        msg.senderId = nodeId;
        
        // Serialize: TYPE|VOTER|HASH|APPROVE
        std::stringstream ss;
        ss << type << "|" << vote.voterId << "|" << crypto::to_hex(vote.blockHash) << "|" << (vote.approve ? "1" : "0");
        msg.payload = ss.str();
        
        gossip.broadcast(msg);
        
        // Loopback for self-consensus/local processing
        if (type == "PREPARE") consensus.onPrepare(vote);
        else if (type == "COMMIT") consensus.onCommit(vote);
    };

    // 2. Block Finalized (Consensus Reached) - Update State
    consensus.onBlockFinalized = [&](const Block& block) {
        std::lock_guard<std::mutex> lock(chainMutex);
        
        // Only verify height order (prevent replays)
        // If we kept `height` strictly, ensure block.header.height == height
        
        if (block.header.height >= height) {
            std::cout << "[CONSENSUS] Finalized Block " << block.header.height << "!" << std::endl;
            
            blockStore.addBlock(block); // Persistence
            
            // Execute batching
            if (block.transactions.size() > 0) {
                 batchManager.addBlock(block);
                 
                 if (batchManager.shouldBatch()) {
                     std::cout << "[BATCH] Batch threshold reached. Triggering L1 Settlement..." << std::endl;
                     auto batch = batchManager.createBatch();
                     
                     // Run in detached thread to avoid blocking consensus
                     std::thread([&bridge, batch]() {
                        bridge.settleBatch(batch);
                     }).detach();
                 }
            }
            
            // Update Chain Tip
            // Warning: If we missed blocks, just jumping height is unsafe without state replay.
            // For this testnet, we assume sequential.
            height = block.header.height + 1; 
            prevHash = block.calculateHash();
            lastBlockTime = block.header.timestamp;
        }
    };

    // 3. Incoming Messages (Inbound)
    gossip.setMessageHandler([&](const NetworkMessage& msg) {
        try {
            if (msg.type == MessageType::VOTE) {
                // Parse: TYPE|VOTER|HASH|APPROVE
                auto parts = split(msg.payload, '|');
                if (parts.size() >= 4) {
                    std::string type = parts[0];
                    Vote v;
                    v.voterId = parts[1];
                    auto hashBytes = crypto::from_hex(parts[2]);
                    if (hashBytes.size() == 32) {
                         std::copy(hashBytes.begin(), hashBytes.end(), v.blockHash.begin());
                    }
                    v.approve = (parts[3] == "1");

                    if (type == "PREPARE") consensus.onPrepare(v);
                    else if (type == "COMMIT") consensus.onCommit(v);
                }
            }
            else if (msg.type == MessageType::BLOCK) {
                // Proposed Block
                std::vector<uint8_t> blockData = crypto::from_hex(msg.payload);
                Block block = Block::deserialize(blockData);
                
                // std::cout << "[NET] Received Block Proposal " << block.header.height << std::endl;

                // Validate and Start Consensus
                // Only participate if it matches our expected next height
                // LOCK to check height
                {
                    std::lock_guard<std::mutex> lock(chainMutex);
                    if (block.header.height != height) {
                        // Ignore future/past blocks for now
                        return;
                    }
                }
                
                // Trigger PBFT (This will vote PREPARE)
                consensus.onPrePrepare(block); 
            }
        } catch (const std::exception& e) {
            std::cout << "[NET] Error handling message: " << e.what() << std::endl;
        }
    });

    // ------------------------------------------------------------------------
    // Main Loop (Block Production)
    // ------------------------------------------------------------------------
    while(true) {
        bool shouldProduce = false;
        
        {
            std::lock_guard<std::mutex> lock(chainMutex);
            if (consensus.isLeader(height) && consensus.getState() == ConsensusState::IDLE) {
                // Heartbeat condition
                if (mempool.size() > 0 || (std::time(nullptr) - lastBlockTime >= 10)) {
                    shouldProduce = true;
                }
            }
        }
        
        if (shouldProduce) {
            // Need snapshot of state to produce
            Hash parentHash;
            uint64_t targetHeight;
            uint64_t parentTime;
            {
               std::lock_guard<std::mutex> lock(chainMutex); 
               parentHash = prevHash;
               targetHeight = height;
               parentTime = lastBlockTime;
            }

            std::cout << "\n[PROPOSER] I am Leader (" << nodeId << "). Proposing Block " << targetHeight << "..." << std::endl;
            Block block = leader.proposeBlock(targetHeight, parentTime, parentHash);
            
            // 1. Broadcast Block Proposal
            gossip.broadcastBlock(block);
            
            // 2. Kickstart local consensus (Leader also votes PREPARE)
            consensus.onPrePrepare(block);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
