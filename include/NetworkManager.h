#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <SDL3/SDL.h>
#include <SDL_net.h>
#include <vector>
#include <functional>
#include <cstdint>

class NetworkManager {
public:
    enum class Role { None, Server, Client };

    static NetworkManager& getInstance();

    bool init();
    void quit();

    bool createServer(uint16_t port);
    bool connectToServer(const char* host, uint16_t port);
    void disconnect();

    bool sendToAll(const void* data, int len);
    bool sendToClient(const void* data, int len);
    bool sendToServer(const void* data, int len);

    void setOnClientMessage(std::function<void(const void*, int)> callback);
    void update();

    Role getRole() const { return m_role; }

private:
    NetworkManager() = default;
    ~NetworkManager();

    Role m_role = Role::None;
    NET_Server* m_server = nullptr;
    NET_StreamSocket* m_serverSocket = nullptr;
    std::vector<NET_StreamSocket*> m_clients;
    std::function<void(const void*, int)> m_onClientMessage;
};

#endif