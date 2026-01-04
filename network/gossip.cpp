#include "gossip.h"
#include "util/crypto.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSE_SOCKET closesocket
    #define SOCKET_INVALID INVALID_SOCKET
    #define SOCKET_ERROR_CODE SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int socket_t;
    #define CLOSE_SOCKET close
    #define SOCKET_INVALID -1
    #define SOCKET_ERROR_CODE -1
#endif

namespace aegen {

Gossip::Gossip() : running(false), listenPort(0) {}

Gossip::~Gossip() { stop(); }

void Gossip::start(int port) {
    listenPort = port;
    running = true;
    listenerThread = std::thread(&Gossip::listenLoop, this);
    std::cout << "[P2P] Gossip started on port " << port << std::endl;
}

void Gossip::stop() {
    running = false;
    if (listenerThread.joinable()) {
        listenerThread.detach();
    }
}

void Gossip::addPeer(const PeerInfo& peer) {
    std::lock_guard<std::mutex> lock(peerMutex);
    peers.push_back(peer);
    std::cout << "[P2P] Added peer: " << peer.host << ":" << peer.port << std::endl;
}

void Gossip::removePeer(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(peerMutex);
    peers.erase(std::remove_if(peers.begin(), peers.end(),
        [&](const PeerInfo& p) { return p.nodeId == nodeId; }), peers.end());
}

std::vector<PeerInfo> Gossip::getPeers() const {
    return peers;
}

void Gossip::broadcastTransaction(const Transaction& tx) {
    NetworkMessage msg;
    msg.type = MessageType::TRANSACTION;
    msg.timestamp = std::time(nullptr);
    
    std::stringstream ss;
    ss << tx.sender << "|" << tx.receiver << "|" << tx.amount << "|" << tx.nonce;
    msg.payload = ss.str();
    
    broadcast(msg);
}

void Gossip::broadcastBlock(const Block& block) {
    NetworkMessage msg;
    msg.type = MessageType::BLOCK;
    msg.timestamp = std::time(nullptr);
    
    // Send full block for sync
    std::vector<uint8_t> data = block.serialize();
    msg.payload = crypto::to_hex(data);
    
    broadcast(msg);
}

void Gossip::broadcast(const NetworkMessage& msg) {
    std::string data = serializeMessage(msg);
    
    std::lock_guard<std::mutex> lock(peerMutex);
    for (const auto& peer : peers) {
        sendToPeer(peer, data);
    }
}

void Gossip::sendMessage(const std::string& peerId, const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(peerMutex);
    for (const auto& peer : peers) {
        if (peer.nodeId == peerId) {
            sendToPeer(peer, serializeMessage(msg));
            break;
        }
    }
}

void Gossip::setTransactionHandler(std::function<void(const Transaction&)> handler) {
    onTransaction = handler;
}

void Gossip::setBlockHandler(std::function<void(const Block&)> handler) {
    onBlock = handler;
}

void Gossip::setMessageHandler(std::function<void(const NetworkMessage&)> handler) {
    onMessage = handler;
}

void Gossip::listenLoop() {
    socket_t ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == SOCKET_INVALID) return;

    // Allow address reuse
    int optval = 1;
    setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));

    sockaddr_in service;
    memset(&service, 0, sizeof(service));
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(listenPort);

    if (bind(ListenSocket, (struct sockaddr*)&service, sizeof(service)) < 0) {
        CLOSE_SOCKET(ListenSocket);
        return;
    }

    listen(ListenSocket, SOMAXCONN);

    while (running) {
        socket_t ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == SOCKET_INVALID) continue;

        char buffer[4096];
        int bytes = recv(ClientSocket, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            std::string data(buffer, bytes);
            handleIncoming(data, "unknown");
        }
        CLOSE_SOCKET(ClientSocket);
    }

    CLOSE_SOCKET(ListenSocket);
}

void Gossip::handleIncoming(const std::string& data, const std::string& fromPeer) {
    NetworkMessage msg = deserializeMessage(data);
    
    // Deduplication
    std::string msgId = data.substr(0, 64);
    if (seenMessages.count(msgId)) return;
    seenMessages[msgId] = true;
    
    if (onMessage) onMessage(msg);
    
    // Re-broadcast to other peers
    broadcast(msg);
}

std::string Gossip::serializeMessage(const NetworkMessage& msg) {
    std::stringstream ss;
    ss << (int)msg.type << "|" << msg.timestamp << "|" << msg.senderId << "|" << msg.payload;
    return ss.str();
}

NetworkMessage Gossip::deserializeMessage(const std::string& data) {
    NetworkMessage msg;
    std::stringstream ss(data);
    std::string token;
    
    std::getline(ss, token, '|'); msg.type = (MessageType)std::stoi(token);
    std::getline(ss, token, '|'); msg.timestamp = std::stoull(token);
    std::getline(ss, token, '|'); msg.senderId = token;
    std::getline(ss, msg.payload);
    
    return msg;
}

bool Gossip::sendToPeer(const PeerInfo& peer, const std::string& data) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == SOCKET_INVALID) return false;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(peer.port);
    inet_pton(AF_INET, peer.host.c_str(), &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        CLOSE_SOCKET(sock);
        return false;
    }

    send(sock, data.c_str(), (int)data.size(), 0);
    CLOSE_SOCKET(sock);
    return true;
}

}
