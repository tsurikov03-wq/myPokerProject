#ifndef GAME_H
#define GAME_H

#include "Player.h"
#include "Deck.h"
#include <vector>
#include <SDL3/SDL.h>

class Game {
public:
    Game(const std::vector<Player*>& players);
    virtual ~Game() = default;

    virtual void run() = 0;
    virtual void render() = 0;
    virtual bool handleEvent(const SDL_Event& event) = 0;
    virtual bool isGameOver() const { return m_gameOver; }

protected:
    std::vector<Player*> m_players;
    Deck m_deck;
    bool m_gameOver;
};

#endif