#ifndef BLACKJACKGAME_H
#define BLACKJACKGAME_H

#include "Game.h"
#include <SDL3/SDL.h>
#include <vector>
#include <string>

class BlackjackGame : public Game {
public:
    BlackjackGame(const std::vector<Player*>& players);
    ~BlackjackGame();

    void run() override;
    void render() override;
    int handleEvent(const SDL_Event& event) override;


    const std::vector<Player*>& getPlayers() const { return m_players; }

private:
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
};

#endif
