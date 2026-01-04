#include "rpc_server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSE_SOCKET closesocket
    #define SOCKET_INVALID INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int socket_t;
    #define CLOSE_SOCKET close
    #define SOCKET_INVALID -1
#endif

namespace aegen {

RPCServer::RPCServer() : running(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    // Initialize thread pool
    std::cout << "[RPC] Initializing thread pool with " << THREAD_POOL_SIZE << " workers..." << std::endl;
    for (size_t i = 0; i < THREAD_POOL_SIZE; ++i) {
        workerPool.emplace_back(&RPCServer::workerThread, this);
    }
}

RPCServer::~RPCServer() {
    stop();
    
    // Shutdown thread pool
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        running = false;
    }
    queueCV.notify_all();
    
    for (auto& worker : workerPool) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
}

void RPCServer::workerThread() {
    while (true) {
        uintptr_t clientSocket;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this] { return !running || !taskQueue.empty(); });
            
            if (!running && taskQueue.empty()) {
                return; // Exit worker thread
            }
            
            if (taskQueue.empty()) {
                continue;
            }
            
            clientSocket = taskQueue.front();
            taskQueue.pop();
        }
        
        // Process the client request
        handleClient(clientSocket);
    }
}

void RPCServer::start(int port) {
    running = true;
    serverThread = std::thread(&RPCServer::listenLoop, this, port);
}

void RPCServer::stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.detach(); 
    }
}

void RPCServer::registerEndpoint(const std::string& name, Handler handler) {
    handlers[name] = handler;
}

void RPCServer::listenLoop(int port) {
    socket_t ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == SOCKET_INVALID) return;

    // Allow address reuse
    int optval = 1;
    setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));

    sockaddr_in service;
    memset(&service, 0, sizeof(service));
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(port);

    if (bind(ListenSocket, (struct sockaddr*)&service, sizeof(service)) < 0) {
        CLOSE_SOCKET(ListenSocket);
        return;
    }

    if (listen(ListenSocket, SOMAXCONN) < 0) {
        CLOSE_SOCKET(ListenSocket);
        return;
    }

    std::cout << "RPC Server listening on port " << port << std::endl;

    while (running) {
        socket_t ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == SOCKET_INVALID) {
            continue;
        }
        
        // SECURITY FIX: Use thread pool instead of spawning unlimited threads
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push((uintptr_t)ClientSocket);
        }
        queueCV.notify_one();
    }

    CLOSE_SOCKET(ListenSocket);
}

// Helper: Extract JSON string value for a key
std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string searchPatterns[] = {
        "\"" + key + "\":",
        "\"" + key + "\" :",
        "\"" + key + "\": ",
        "\"" + key + "\" : "
    };
    
    size_t keyPos = std::string::npos;
    for (const auto& pattern : searchPatterns) {
        keyPos = json.find(pattern);
        if (keyPos != std::string::npos) {
            keyPos += pattern.length();
            break;
        }
    }
    
    if (keyPos == std::string::npos) return "";
    
    while (keyPos < json.size() && std::isspace(json[keyPos])) keyPos++;
    
    if (keyPos >= json.size() || json[keyPos] != '"') return "";
    
    size_t startQuote = keyPos;
    size_t endQuote = json.find('"', startQuote + 1);
    
    if (endQuote == std::string::npos) return "";
    
    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

// Helper: Get Content-Length from HTTP headers
int getContentLength(const std::string& headers) {
    std::string lowerHeaders = headers;
    std::transform(lowerHeaders.begin(), lowerHeaders.end(), lowerHeaders.begin(), ::tolower);
    
    size_t pos = lowerHeaders.find("content-length:");
    if (pos == std::string::npos) return -1;
    
    pos += 15;
    while (pos < lowerHeaders.size() && std::isspace(lowerHeaders[pos])) pos++;
    
    std::string numStr;
    while (pos < lowerHeaders.size() && std::isdigit(lowerHeaders[pos])) {
        numStr += lowerHeaders[pos++];
    }
    
    return numStr.empty() ? -1 : std::stoi(numStr);
}

void RPCServer::handleClient(uintptr_t clientSocket) {
    socket_t client = (socket_t)clientSocket;
    
    // Set receive timeout
#ifdef _WIN32
    DWORD timeout = 5000;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif
    
    std::string fullRequest;
    char recvbuf[8192];
    int totalReceived = 0;
    int contentLength = -1;
    size_t headerEnd = std::string::npos;
    
    while (true) {
        int iResult = recv(client, recvbuf, sizeof(recvbuf) - 1, 0);
        if (iResult <= 0) break;
        
        recvbuf[iResult] = '\0';
        fullRequest.append(recvbuf, iResult);
        totalReceived += iResult;
        
        if (headerEnd == std::string::npos) {
            headerEnd = fullRequest.find("\r\n\r\n");
        }
        
        if (headerEnd != std::string::npos) {
            if (contentLength < 0) {
                contentLength = getContentLength(fullRequest.substr(0, headerEnd));
            }
            
            size_t bodyStart = headerEnd + 4;
            size_t bodyReceived = fullRequest.size() - bodyStart;
            
            if (contentLength >= 0 && (int)bodyReceived >= contentLength) {
                break;
            }
        }
        
        if (totalReceived > 65536) break;
    }
    
    if (fullRequest.empty()) {
        CLOSE_SOCKET(client);
        return;
    }
    
    std::string responseBody;
    
    // Handle CORS Preflight
    if (fullRequest.find("OPTIONS") == 0) {
        std::string resp = "HTTP/1.1 204 No Content\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
        send(client, resp.c_str(), (int)resp.length(), 0);
        CLOSE_SOCKET(client);
        return;
    }
    
    size_t bodyPos = fullRequest.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        std::string body = fullRequest.substr(bodyPos + 4);
        
        while (!body.empty() && std::isspace(body.front())) body.erase(body.begin());
        while (!body.empty() && std::isspace(body.back())) body.pop_back();
        
        if (body.empty()) {
            responseBody = "{\"error\": \"Empty request body\"}";
        } else {
            std::string methodName = extractJsonString(body, "method");
            
            if (methodName.empty()) {
                std::cerr << "[RPC] Failed to parse method from body: " << body.substr(0, 200) << std::endl;
                responseBody = "{\"error\": \"Invalid JSON-RPC: method not found\"}";
            } else if (handlers.count(methodName)) {
                try {
                    responseBody = handlers[methodName](body);
                } catch (const std::exception& e) {
                    responseBody = "{\"error\": \"Handler exception: " + std::string(e.what()) + "\"}";
                } catch (...) {
                    responseBody = "{\"error\": \"Unknown handler exception\"}";
                }
            } else {
                responseBody = "{\"error\": \"Method not found: " + methodName + "\"}";
            }
        }
    } else {
        responseBody = "{\"error\": \"Invalid HTTP request\"}";
    }

    std::string httpResponse = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: " + std::to_string(responseBody.length()) + "\r\n"
        "Connection: close\r\n\r\n" +
        responseBody;

    send(client, httpResponse.c_str(), (int)httpResponse.length(), 0);
    CLOSE_SOCKET(client);
}

}
