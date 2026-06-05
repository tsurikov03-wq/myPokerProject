#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <cstdint>

enum class PacketType : uint8_t {
    ServerInfo = 1,
    JoinRequest = 2,
    JoinAccept = 3,
    PlayerAction = 4,
    GameState = 5,
    GameOver = 6,
};

enum class BlackjackAction : uint8_t {
    Hit, Stand, Double
};

#pragma pack(push, 1)
struct PlayerActionPacket {
    PacketType type;
    uint32_t playerId;
    BlackjackAction action;
};

struct PlayerNetState {
    uint32_t playerId;
    char name[20];
    int16_t money;
    int16_t bet;
    uint8_t handValue;
    uint8_t cardCount;
    uint8_t cards[5][2];
    bool bust;
    bool done;
    bool isBot;
};

struct GameStatePacket {
    PacketType type;
    uint32_t currentPlayerId;
    uint8_t dealerCardCount;
    uint8_t dealerCards[10][2];
    uint8_t dealerHandValue;
    uint8_t playerCount;
    PlayerNetState players[6];
    bool gameOver;
};
#pragma pack(pop)

#endif