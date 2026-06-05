#ifndef LANBLACKJACKGAME_H
#define LANBLACKJACKGAME_H

#include "Game.h"
#include "NetworkProtocol.h"
#include <vector>
#include <string>
#include <SDL3/SDL.h>

class LANBlackjackGame : public Game {
public:
    LANBlackjackGame(const std::vector<Player*>& players, bool isServer);
    ~LANBlackjackGame();

    void run() override;
    void render() override;
    int handleEvent(const SDL_Event& event) override;

private:
    bool m_isServer;
    uint32_t m_clientPlayerId;
    GameStatePacket m_lastState;

    Player* m_dealer;
    int m_currentPlayerIndex;
    bool m_waitingForAction;
    std::vector<int> m_playerBets;
    std::vector<bool> m_playerDone;
    uint64_t m_botTimer;
    std::vector<std::string> m_playerResult;
    std::vector<SDL_Color> m_playerResultColor;

    void dealerTurn();
    void resolveBets();
    void nextPlayer();
    void askBet(Player* p, int idx);
    void startNewRound();
    void sendAction(BlackjackAction action);
    void onPacketReceived(const void* data, int len);
    void sendFullStateToClient();
};

#endif