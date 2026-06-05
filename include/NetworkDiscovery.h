#ifndef NETWORK_DISCOVERY_H
#define NETWORK_DISCOVERY_H

#include <SDL3/SDL.h>
#include <SDL_net.h>
#include <string>
#include <vector>
#include <functional>

class NetworkDiscovery {
public:
    static NetworkDiscovery& getInstance();

    bool init();
    void quit();

    void startBroadcasting(uint16_t port, const char* serverName = "PokerServer");
    void stopBroadcasting();

    void startDiscovery(uint16_t discoveryPort);
    void stopDiscovery();

    struct ServerInfo {
        std::string ip;
        uint16_t port;
        std::string name;
    };
    std::vector<ServerInfo> getServers() const;

    void setOnServerDiscovered(std::function<void(const ServerInfo&)> callback);

    void update();

private:
    NetworkDiscovery() = default;
    ~NetworkDiscovery();

    bool m_broadcasting = false;
    bool m_discovering = false;
    NET_DatagramSocket* m_broadcastSocket = nullptr;
    NET_DatagramSocket* m_discoverySocket = nullptr;
    uint16_t m_broadcastPort = 0;
    uint16_t m_discoveryPort = 0;
    std::vector<ServerInfo> m_servers;
    std::function<void(const ServerInfo&)> m_onServerDiscovered;
    uint64_t m_lastBroadcastTime = 0;
    static const uint64_t BROADCAST_INTERVAL_MS = 2000;
};

#endif