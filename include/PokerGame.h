#ifndef POKERGAME_H
#define POKERGAME_H

#include "Game.h"
#include <vector>
#include <string>

class PokerGame : public Game {
public:
    PokerGame(const std::vector<Player*>& players);
    ~PokerGame();

    void run() override;
    void render() override;
    int handleEvent(const SDL_Event& event) override;

private:
    enum class Stage { Preflop, Flop, Turn, River, Showdown };
    Stage m_stage;
    std::vector<Card> m_communityCards;
    int m_smallBlind;
    int m_bigBlind;
    int m_currentBet;
    int m_pot;
    int m_dealerButton;
    int m_currentPlayerIdx;
    int m_lastRaiserIdx;
    int m_playersInHand;
    bool m_waitingForAction;
    std::string m_winnerText;

    void advanceStage();
    void startNewRound();
    void collectBlinds();
    void evaluateAndPayWinners();
    void nextPlayer();
    bool isRoundComplete();
    void resetPlayerActions();
    int getPlayerBet(Player* p);
};

#endif
