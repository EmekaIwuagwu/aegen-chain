#pragma once
#include "core/transaction.h"
#include "core/block.h"
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace aegen {

struct PeerInfo {
    std::string host;
    int port;
    std::string nodeId;
    bool isValidator;
};

enum class MessageType {
    TRANSACTION,
    BLOCK,
    SYNC_REQUEST,
    SYNC_RESPONSE,
    VOTE,
    PREPARE,
    COMMIT
};

struct NetworkMessage {
    MessageType type;
    std::string payload;
    std::string senderId;
    uint64_t timestamp;
};

class Gossip {
    std::vector<PeerInfo> peers;
    std::map<std::string, bool> seenMessages;
    std::mutex peerMutex;
    std::atomic<bool> running;
    std::thread listenerThread;
    int listenPort;
    
    std::function<void(const Transaction&)> onTransaction;
    std::function<void(const Block&)> onBlock;
    std::function<void(const NetworkMessage&)> onMessage;

public:
    Gossip();
    ~Gossip();
    
    void start(int port);
    void stop();
    
    void addPeer(const PeerInfo& peer);
    void removePeer(const std::string& nodeId);
    std::vector<PeerInfo> getPeers() const;
    
    void broadcastTransaction(const Transaction& tx);
    void broadcastBlock(const Block& block);
    void sendMessage(const std::string& peerId, const NetworkMessage& msg);
    void broadcast(const NetworkMessage& msg);
    
    void setTransactionHandler(std::function<void(const Transaction&)> handler);
    void setBlockHandler(std::function<void(const Block&)> handler);
    void setMessageHandler(std::function<void(const NetworkMessage&)> handler);

private:
    void listenLoop();
    void handleIncoming(const std::string& data, const std::string& fromPeer);
    std::string serializeMessage(const NetworkMessage& msg);
    NetworkMessage deserializeMessage(const std::string& data);
    bool sendToPeer(const PeerInfo& peer, const std::string& data);
};

}
