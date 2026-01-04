#include "pbft.h"
#include "util/crypto.h"
#include <iostream>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
    #include <direct.h>  // For _mkdir
#else
    #include <sys/stat.h>
#endif

namespace aegen {

PBFT::PBFT(const std::string& id, const std::vector<std::string>& validatorSet)
    : nodeId(id), validators(validatorSet), state(ConsensusState::IDLE) {
    
    // SECURITY FIX: Initialize persistent storage
    consensusDbPath = "./data/consensus_" + nodeId + ".log";
    
    // Create data directory if it doesn't exist
#ifdef _WIN32
    _mkdir("./data");
#else
    mkdir("./data", 0755);
#endif
    
    loadPersistedVotes();
    std::cout << "[PBFT] Initialized with " << validators.size() << " validators (quorum: " 
              << quorumSize() << ")" << std::endl;
}

void PBFT::persistVote(const std::string& voteType, const Vote& vote) {
    std::ofstream file(consensusDbPath, std::ios::app | std::ios::binary);
    if (file.is_open()) {
        // Simple format: TYPE|VOTER_ID|BLOCK_HASH|APPROVE\n
        file << voteType << "|" << vote.voterId << "|" 
             << crypto::to_hex(vote.blockHash) << "|" 
             << (vote.approve ? "1" : "0") << "\n";
        file.flush();
    } else {
        std::cerr << "[PBFT] ERROR: Failed to persist vote to " << consensusDbPath << std::endl;
    }
}

void PBFT::loadPersistedVotes() {
    std::ifstream file(consensusDbPath);
    if (!file.is_open()) {
        std::cout << "[PBFT] No persisted votes found (new node)" << std::endl;
        return;
    }
    
    std::string line;
    int loadedCount = 0;
    while (std::getline(file, line)) {
        // Parse: TYPE|VOTER_ID|BLOCK_HASH|APPROVE
        std::stringstream ss(line);
        std::string type, voterId, blockHashHex, approveStr;
        
        std::getline(ss, type, '|');
        std::getline(ss, voterId, '|');
        std::getline(ss, blockHashHex, '|');
        std::getline(ss, approveStr, '|');
        
        Vote vote;
        vote.voterId = voterId;
        vote.approve = (approveStr == "1");
        
        // Convert hex to Hash
        if (blockHashHex.length() == 64) {
            for (size_t i = 0; i < 32; ++i) {
                std::string byteStr = blockHashHex.substr(i * 2, 2);
                vote.blockHash[i] = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            }
            
            if (type == "PREPARE") {
                prepareVotes[vote.blockHash].push_back(vote);
            } else if (type == "COMMIT") {
                commitVotes[vote.blockHash].push_back(vote);
            }
            loadedCount++;
        }
    }
    
    std::cout << "[PBFT] Loaded " << loadedCount << " persisted votes" << std::endl;
}

bool PBFT::isLeader(uint64_t round) const {
    return getLeader(round) == nodeId;
}

std::string PBFT::getLeader(uint64_t round) const {
    if (validators.empty()) return "";
    return validators[round % validators.size()];
}

void PBFT::onPrePrepare(const Block& block) {
    std::cout << "[PBFT] Received PrePrepare for block " << block.header.height << std::endl;
    state = ConsensusState::PREPARE;
    
    // Create and broadcast PREPARE vote
    Vote vote;
    vote.voterId = nodeId;
    vote.blockHash = block.calculateHash();
    vote.approve = true;
    
    {
        std::lock_guard<std::mutex> lock(voteMutex);
        prepareVotes[vote.blockHash].push_back(vote);
    }
    
    if (broadcastVote) broadcastVote(vote);
}

void PBFT::onPrepare(const Vote& vote) {
    std::lock_guard<std::mutex> lock(voteMutex);
    
    // SECURITY FIX: Persist vote before processing
    persistVote("PREPARE", vote);
    
    prepareVotes[vote.blockHash].push_back(vote);
    
    std::cout << "[PBFT] PREPARE votes for block: " << prepareVotes[vote.blockHash].size() 
              << "/" << quorumSize() << std::endl;
    
    if (hasPrepareQuorum(vote.blockHash) && state == ConsensusState::PREPARE) {
        state = ConsensusState::COMMIT;
        
        // Create COMMIT vote
        Vote commitVote;
        commitVote.voterId = nodeId;
        commitVote.blockHash = vote.blockHash;
        commitVote.approve = true;
        
        // Persist before broadcasting
        persistVote("COMMIT", commitVote);
        commitVotes[vote.blockHash].push_back(commitVote);
        
        if (broadcastVote) broadcastVote(commitVote);
    }
}

void PBFT::onCommit(const Vote& vote) {
    std::lock_guard<std::mutex> lock(voteMutex);
    
    // SECURITY FIX: Persist vote before processing
    persistVote("COMMIT", vote);
    
    commitVotes[vote.blockHash].push_back(vote);
    
    std::cout << "[PBFT] COMMIT votes for block: " << commitVotes[vote.blockHash].size() 
              << "/" << quorumSize() << std::endl;
    
    if (hasCommitQuorum(vote.blockHash) && state == ConsensusState::COMMIT) {
        state = ConsensusState::FINALIZED;
        std::cout << "[PBFT] Block FINALIZED with 2/3 consensus!" << std::endl;
        
        // Clear votes for this block
        prepareVotes.erase(vote.blockHash);
        commitVotes.erase(vote.blockHash);
        
        state = ConsensusState::IDLE;
    }
}

bool PBFT::hasPrepareQuorum(const Hash& blockHash) const {
    auto it = prepareVotes.find(blockHash);
    if (it == prepareVotes.end()) return false;
    return it->second.size() >= quorumSize();
}

bool PBFT::hasCommitQuorum(const Hash& blockHash) const {
    auto it = commitVotes.find(blockHash);
    if (it == commitVotes.end()) return false;
    return it->second.size() >= quorumSize();
}

}
