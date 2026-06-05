#include "NetworkManager.h"
#include <iostream>

NetworkManager& NetworkManager::getInstance() {
    static NetworkManager instance;
    return instance;
}

bool NetworkManager::init() {
    if (!NET_Init()) {
        std::cerr << "NET_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

void NetworkManager::quit() {
    disconnect();
    NET_Quit();
}

bool NetworkManager::createServer(uint16_t port) {
    if (m_role != Role::None) return false;
    m_server = NET_CreateServer(nullptr, port, 0);
    if (!m_server) {
        std::cerr << "NET_CreateServer failed: " << SDL_GetError() << std::endl;
        return false;
    }
    m_role = Role::Server;
    std::cout << "Server created on port " << port << std::endl;
    return true;
}

bool NetworkManager::connectToServer(const char* host, uint16_t port) {
    if (m_role != Role::None) return false;
    NET_Address* addr = NET_ResolveHostname(host);
    if (!addr) {
        std::cerr << "NET_ResolveHostname failed: " << SDL_GetError() << std::endl;
        return false;
    }
    if (NET_WaitUntilResolved(addr, 3000) != NET_SUCCESS) {
        std::cerr << "Resolve timeout or failed" << std::endl;
        NET_UnrefAddress(addr);
        return false;
    }
    m_serverSocket = NET_CreateClient(addr, port, 0);
    NET_UnrefAddress(addr);
    if (!m_serverSocket) {
        std::cerr << "NET_CreateClient failed: " << SDL_GetError() << std::endl;
        return false;
    }
    if (NET_WaitUntilConnected(m_serverSocket, 3000) != NET_SUCCESS) {
        std::cerr << "Connection failed" << std::endl;
        NET_DestroyStreamSocket(m_serverSocket);
        m_serverSocket = nullptr;
        return false;
    }
    m_role = Role::Client;
    std::cout << "Connected to " << host << ":" << port << std::endl;
    return true;
}

void NetworkManager::disconnect() {
    if (m_role == Role::Server) {
        for (auto c : m_clients) NET_DestroyStreamSocket(c);
        m_clients.clear();
        if (m_server) NET_DestroyServer(m_server);
        m_server = nullptr;
    } else if (m_role == Role::Client) {
        if (m_serverSocket) NET_DestroyStreamSocket(m_serverSocket);
        m_serverSocket = nullptr;
    }
    m_role = Role::None;
}

bool NetworkManager::sendToAll(const void* data, int len) {
    if (m_role == Role::Server) {
        bool ok = true;
        for (auto c : m_clients) {
            if (!NET_WriteToStreamSocket(c, data, len)) ok = false;
        }
        return ok;
    } else if (m_role == Role::Client && m_serverSocket) {
        return NET_WriteToStreamSocket(m_serverSocket, data, len);
    }
    return false;
}

bool NetworkManager::sendToClient(const void* data, int len) {
    if (m_role != Role::Server || m_clients.empty()) return false;
    return NET_WriteToStreamSocket(m_clients[0], data, len);
}

bool NetworkManager::sendToServer(const void* data, int len) {
    if (m_role == Role::Client && m_serverSocket) {
        return NET_WriteToStreamSocket(m_serverSocket, data, len);
    }
    return false;
}

void NetworkManager::setOnClientMessage(std::function<void(const void*, int)> callback) {
    m_onClientMessage = callback;
}

void NetworkManager::update() {
    if (m_role == Role::Server) {
        NET_StreamSocket* newClient = nullptr;
        while (NET_AcceptClient(m_server, &newClient) && newClient) {
            m_clients.push_back(newClient);
            std::cout << "New client connected, total: " << m_clients.size() << std::endl;
        }
        if (!m_clients.empty()) {
            char buffer[2048];
            int len = sizeof(buffer);
            int ret = NET_ReadFromStreamSocket(m_clients[0], buffer, len);
            if (ret > 0 && m_onClientMessage) {
                m_onClientMessage(buffer, ret);
            }
        }
    } else if (m_role == Role::Client && m_serverSocket) {
        char buffer[2048];
        int len = sizeof(buffer);
        int ret = NET_ReadFromStreamSocket(m_serverSocket, buffer, len);
        if (ret > 0 && m_onClientMessage) {
            m_onClientMessage(buffer, ret);
        }
    }
}

NetworkManager::~NetworkManager() {
    quit();
}