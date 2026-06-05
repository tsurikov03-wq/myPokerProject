#ifndef BLACKJACKGAME_H
#define BLACKJACKGAME_H

#include "Game.h"
#include <vector>

class BlackjackGame : public Game {
public:
    BlackjackGame(const std::vector<Player*>& players);
    ~BlackjackGame();

    void run() override;
    void render() override;
    bool handleEvent(const SDL_Event& event) override;

private:
    Player* m_dealer;
    int m_currentPlayerIndex;
    bool m_waitingForAction;
    std::vector<int> m_playerBets;
    std::vector<bool> m_playerDone;
    uint64_t m_botTimer;

    void dealerTurn();
    void resolveBets();
    void nextPlayer();
    void askBet(Player* p, int idx);
};

#endif
