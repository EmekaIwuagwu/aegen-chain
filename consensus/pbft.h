#pragma once
#include "core/block.h"
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <string>
#include <fstream>

namespace aegen {

enum class ConsensusState {
    IDLE,
    PRE_PREPARE,
    PREPARE,
    COMMIT,
    FINALIZED
};

struct Vote {
    std::string voterId;
    Hash blockHash;
    bool approve;
    Signature signature;
};

class PBFT {
    std::string nodeId;
    std::vector<std::string> validators;
    std::map<Hash, std::vector<Vote>> prepareVotes;
    std::map<Hash, std::vector<Vote>> commitVotes;
    std::map<Hash, Block> pendingBlocks; // Store blocks during consensus
    ConsensusState state;
    std::recursive_mutex voteMutex;
    std::string consensusDbPath; // For persistence
    
    size_t quorumSize() const { return (validators.size() * 2) / 3 + 1; }
    
    // SECURITY FIX: Add persistence to prevent double-voting after restart
    void persistVote(const std::string& voteType, const Vote& vote);
    void loadPersistedVotes();

public:
    PBFT(const std::string& id, const std::vector<std::string>& validatorSet);
    
    bool isLeader(uint64_t round) const;
    std::string getLeader(uint64_t round) const;
    
    void onPrePrepare(const Block& block);
    void onPrepare(const Vote& vote);
    void onCommit(const Vote& vote);
    
    bool hasPrepareQuorum(const Hash& blockHash) const;
    bool hasCommitQuorum(const Hash& blockHash) const;
    
    ConsensusState getState() const { return state; }
    
    std::function<void(const Block&)> onBlockFinalized;
    std::function<void(const Vote&, const std::string&)> broadcastVote;
};

}
