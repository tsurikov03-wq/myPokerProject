#include "NetworkDiscovery.h"
#include <iostream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <functional>

NetworkDiscovery& NetworkDiscovery::getInstance() {
    static NetworkDiscovery instance;
    return instance;
}

bool NetworkDiscovery::init() {
    return true;
}

void NetworkDiscovery::quit() {
    stopBroadcasting();
    stopDiscovery();
}

void NetworkDiscovery::startBroadcasting(uint16_t port, const char* serverName) {
    if (m_broadcasting) return;
    m_broadcastPort = port;          // игровой порт сервера (12345)
    m_broadcastSocket = NET_CreateDatagramSocket(nullptr, 0, 0);
    if (!m_broadcastSocket) {
        std::cerr << "Failed to create broadcast socket: " << SDL_GetError() << std::endl;
        return;
    }
    m_broadcasting = true;
    m_lastBroadcastTime = 0;
    (void)serverName;
    std::cout << "Broadcasting started on port " << port << " (game port)" << std::endl;
}

void NetworkDiscovery::stopBroadcasting() {
    if (m_broadcastSocket) {
        NET_DestroyDatagramSocket(m_broadcastSocket);
        m_broadcastSocket = nullptr;
    }
    m_broadcasting = false;
}

void NetworkDiscovery::startDiscovery(uint16_t discoveryPort) {
    if (m_discovering) return;
    m_discoveryPort = discoveryPort;   // порт для приёма broadcast-пакетов (12346)
    m_discoverySocket = NET_CreateDatagramSocket(nullptr, discoveryPort, 0);
    if (!m_discoverySocket) {
        std::cerr << "Failed to create discovery socket: " << SDL_GetError() << std::endl;
        return;
    }
    m_discovering = true;
    std::cout << "Discovery started on port " << discoveryPort << std::endl;
}

void NetworkDiscovery::stopDiscovery() {
    if (m_discoverySocket) {
        NET_DestroyDatagramSocket(m_discoverySocket);
        m_discoverySocket = nullptr;
    }
    m_discovering = false;
    m_servers.clear();
}

void NetworkDiscovery::update() {
    // Отправка broadcast (сервер)
    if (m_broadcasting && m_broadcastSocket) {
        uint64_t now = SDL_GetTicks();
        if (now - m_lastBroadcastTime >= BROADCAST_INTERVAL_MS) {
            m_lastBroadcastTime = now;
            // Формируем сообщение: "PokerServer:PORT:NAME"
            // PORT – это ИГРОВОЙ порт (m_broadcastPort)
            std::string msg = "PokerServer:" + std::to_string(m_broadcastPort) + ":Host";
            // Отправляем на широковещательный адрес 255.255.255.255, порт m_discoveryPort (12346)
            NET_Address* broadcastAddr = NET_ResolveHostname("255.255.255.255");
            if (broadcastAddr && NET_WaitUntilResolved(broadcastAddr, 100) == NET_SUCCESS) {
                NET_SendDatagram(m_broadcastSocket, broadcastAddr, m_discoveryPort, msg.c_str(), (int)msg.size() + 1);
            }
            if (broadcastAddr) NET_UnrefAddress(broadcastAddr);
        }
    }

    // Приём broadcast (клиент)
    if (m_discovering && m_discoverySocket) {
        NET_Datagram* dgram = nullptr;
        while (NET_ReceiveDatagram(m_discoverySocket, &dgram) && dgram) {
            if (dgram->buf && dgram->buflen > 0) {
                std::string msg((char*)dgram->buf, dgram->buflen - 1);
                if (msg.find("PokerServer:") == 0) {
                    size_t p1 = msg.find(':', 12);
                    if (p1 != std::string::npos) {
                        uint16_t gamePort = (uint16_t)std::stoi(msg.substr(12, p1 - 12));
                        std::string name = msg.substr(p1 + 1);
                        const char* ip = NET_GetAddressString(dgram->addr);
                        if (ip) {
                            ServerInfo info{std::string(ip), gamePort, name};
                            bool found = false;
                            for (const auto& s : m_servers) {
                                if (s.ip == info.ip && s.port == info.port) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                m_servers.push_back(info);
                                if (m_onServerDiscovered) m_onServerDiscovered(info);
                                std::cout << "Discovered server: " << info.ip << ":" << info.port << " (" << info.name << ")" << std::endl;
                            }
                        }
                    }
                }
            }
            NET_DestroyDatagram(dgram);
        }
    }
}

std::vector<NetworkDiscovery::ServerInfo> NetworkDiscovery::getServers() const {
    return m_servers;
}

void NetworkDiscovery::setOnServerDiscovered(std::function<void(const ServerInfo&)> callback) {
    m_onServerDiscovered = callback;
}

NetworkDiscovery::~NetworkDiscovery() {
    quit();
}