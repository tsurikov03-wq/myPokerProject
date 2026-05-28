#ifndef BLACKJACKGAME_H
#define BLACKJACKGAME_H

#include "Game.h"

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
    void dealerTurn();
    void resolveBets();
    void nextPlayer();
};

#endif