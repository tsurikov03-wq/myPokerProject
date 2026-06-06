#ifndef LANPOKERGAME_H
#define LANPOKERGAME_H

#include "Game.h"
#include "NetworkProtocol.h"
#include <vector>
#include <string>
#include <SDL3/SDL.h>

class LANPokerGame : public Game {
public:
    LANPokerGame(const std::vector<Player*>& players, bool isServer);
    ~LANPokerGame();

    void run() override;
    void render() override;
    int handleEvent(const SDL_Event& event) override;

    void sendFullStateToClient();  // для периодической отправки

private:
    bool m_isServer;
    uint32_t m_clientPlayerId;
    PokerStatePacket m_lastState;

    // Локальные данные для сервера
    enum class Stage { Preflop, Flop, Turn, River, Showdown };
    Stage m_stage;
    std::vector<Card> m_communityCards;
    int m_smallBlind;
    int m_bigBlind;
    uint32_t m_currentBet;
    uint32_t m_pot;
    int m_dealerButton;
    int m_currentPlayerIdx;
    int m_lastRaiserIdx;
    int m_playersInHand;
    bool m_waitingForAction;
    std::string m_winnerText;

    void startNewRound();
    void advanceStage();
    void nextPlayer();
    bool isRoundComplete();
    void evaluateAndPayWinners();
    void collectBlinds();

    void sendAction(PokerActionType action, uint32_t raiseAmount = 0);
    void onPacketReceived(const void* data, int len);
};

#endif