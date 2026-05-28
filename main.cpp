#include <SDL3/SDL.h>
#include "include/Renderer.h"
#include "include/Menu.h"
#include "include/Player.h"
#include "include/PokerGame.h"
#include "include/BlackjackGame.h"
#include <vector>

int main(int argc, char* argv[]) {
    if (!Renderer::getInstance().init("Card Games - Resolution", 800, 600))
        return -1;

    Menu::Resolution res = Menu::showResolutionMenu();
    Renderer::getInstance().shutdown();

    if (!Renderer::getInstance().init("Card Games", res.width, res.height))
        return -1;

    bool quit = false;
    while (!quit) {
        Menu::Choice choice = Menu::showMainMenu();
        if (choice == Menu::Choice::Quit) break;

        Menu::PlayerSetup playersSetup = Menu::showPlayerSetupMenu();
        int humanCount = playersSetup.humans;
        int botCount = playersSetup.bots;

        std::vector<Player*> players;
        for (int i = 0; i < humanCount; ++i)
            players.push_back(new HumanPlayer("Player " + std::to_string(i+1)));
        for (int i = 0; i < botCount; ++i)
            players.push_back(new BotPlayer("Bot " + std::to_string(i+1)));

        if (choice == Menu::Choice::Poker) {
            PokerGame game(players);
            game.run();
            bool gameRunning = true;
            while (gameRunning) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) { gameRunning = false; quit = true; }
                    game.handleEvent(event);
                }
                game.render();
                SDL_Delay(16);
                if (game.isGameOver()) gameRunning = false;
            }
        }
        else if (choice == Menu::Choice::Blackjack) {
            BlackjackGame game(players);
            game.run();
            bool gameRunning = true;
            while (gameRunning) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) { gameRunning = false; quit = true; }
                    game.handleEvent(event);
                }
                game.render();
                SDL_Delay(16);
                if (game.isGameOver()) gameRunning = false;
            }
        }

        for (auto p : players) delete p;
    }
    return 0;
}