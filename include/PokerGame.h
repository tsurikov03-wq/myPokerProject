#ifndef POKERGAME_H
#define POKERGAME_H

#include "Game.h"
#include <vector>

class PokerGame : public Game {
public:
    PokerGame(const std::vector<Player*>& players);
    void run() override;
    void render() override;
    bool handleEvent(const SDL_Event& event) override;

private:
    enum class State { Preflop, Flop, Turn, River, Showdown };
    State m_state;
    std::vector<Card> m_communityCards;
    int m_currentBet;
    int m_pot;
    int m_currentPlayerIndex;
    std::vector<int> m_playerBets; // -1 = fold
    bool m_waitingForAction;

    void nextPlayer();
    void endRound();
    void evaluateWinner();
    void dealHoleCards();
    void dealCommunity(int count);
    void resetRoundBets();
    void processAction(Player* player, const std::string& action);
    bool allPlayersActed() const;
    int getMinBet() const;
    int getPlayerBetAmount(Player* p);
};

#endif