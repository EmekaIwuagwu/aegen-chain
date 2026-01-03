#include "pbft.h"
#include "util/crypto.h"
#include <iostream>
#include <algorithm>

namespace aegen {

PBFT::PBFT(const std::string& id, const std::vector<std::string>& validatorSet)
    : nodeId(id), validators(validatorSet), state(ConsensusState::IDLE) {}

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
        
        commitVotes[vote.blockHash].push_back(commitVote);
        
        if (broadcastVote) broadcastVote(commitVote);
    }
}

void PBFT::onCommit(const Vote& vote) {
    std::lock_guard<std::mutex> lock(voteMutex);
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
