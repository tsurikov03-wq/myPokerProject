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
    PokerAction = 10,
    PokerState = 11,
};

enum class BlackjackAction : uint8_t {
    Hit, Stand, Double
};

enum class PokerActionType : uint8_t {
    Fold, Check, Call, Raise, AllIn
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
    char result[20];
    uint8_t resultColorR;
    uint8_t resultColorG;
    uint8_t resultColorB;
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


struct PokerActionPacket {
    PacketType type;
    uint32_t playerId;
    PokerActionType action;
    uint32_t raiseAmount;
};

struct PlayerPokerState {
    uint32_t playerId;
    char name[20];
    int32_t money;
    int32_t currentBet;
    uint8_t cardCount;
    uint8_t cards[2][2];
    bool isFolded;
    bool isAllIn;
    bool hasActed;
    bool isBot;
};

struct PokerStatePacket {
    PacketType type;
    uint32_t currentPlayerId;
    uint32_t pot;
    uint32_t currentBet;
    uint8_t stage;
    uint8_t communityCardCount;
    uint8_t communityCards[5][2];
    uint8_t playerCount;
    PlayerPokerState players[6];
    bool gameOver;
    char winnerText[100];
};

#pragma pack(pop)

#endif