#include <SDL3/SDL.h>
#include "Renderer.h"
#include "Menu.h"
#include "Player.h"
#include "BlackjackGame.h"
#include "PokerGame.h"
#include "LANBlackjackGame.h"
#include "NetworkManager.h"
#include "NetworkDiscovery.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!Renderer::getInstance().init("Casino Suite - Setup", 1024, 768))
        return -1;

    Menu::Resolution res = Menu::showResolutionMenu();
    Renderer::getInstance().shutdown();

    if (!Renderer::getInstance().init("Casino Suite", res.width, res.height))
        return -1;

    bool quit = false;
    while (!quit) {
        Menu::GameChoice choice = Menu::showMainMenu();
        if (choice == Menu::GameChoice::Quit) break;

        if (choice == Menu::GameChoice::LANBlackjackHost || choice == Menu::GameChoice::LANBlackjackJoin) {
            auto& net = NetworkManager::getInstance();
            auto& disc = NetworkDiscovery::getInstance();
            if (!net.init()) continue;
            disc.init();

            if (choice == Menu::GameChoice::LANBlackjackHost) {
                uint16_t port = 12345;
                if (!net.createServer(port)) {
                    net.quit();
                    disc.quit();
                    continue;
                }
                // Создаём двух игроков: индекс 0 – хост, индекс 1 – клиент
                std::vector<Player*> players;
                players.push_back(new HumanPlayer("Host", 1000));
                players.push_back(new HumanPlayer("Client", 1000));
                bool nextGame = false;
                do {
                    LANBlackjackGame game(players, true);
                    game.run();
                    bool gameRunning = true;
                    int exitCode = 0;
                    uint64_t lastStateSend = SDL_GetTicks();
                    while (gameRunning) {
                        SDL_Event event;
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_EVENT_QUIT) {
                                gameRunning = false;
                                quit = true;
                                exitCode = 1;
                            }
                            int code = game.handleEvent(event);
                            if (code > 0 && game.isGameOver()) {
                                exitCode = code;
                                gameRunning = false;
                            }
                        }
                        // Периодически отправляем состояние клиентам
                        uint64_t now = SDL_GetTicks();
                        if (now - lastStateSend >= 500) {
                            game.sendFullStateToClient();
                            lastStateSend = now;
                        }
                        net.update();
                        disc.update();
                        game.render();
                        SDL_Delay(16);
                    }
                    if (exitCode == 1 || quit) {
                        nextGame = false;
                        break;
                    } else if (exitCode == 2) {
                        nextGame = true;
                        continue;
                    }
                } while (nextGame);
                for (auto p : players) delete p;
            } else {
                std::string chosenIP = Menu::inputText("Enter server IP:", "192.168.10.103", 400, 300);
                uint16_t port = 12345;
                if (!net.connectToServer(chosenIP.c_str(), port)) {
                    net.quit();
                    disc.quit();
                    continue;
                }
                bool nextGame = false;
                do {
                    std::vector<Player*> players;
                    LANBlackjackGame game(players, false);
                    game.run();
                    bool gameRunning = true;
                    int exitCode = 0;
                    while (gameRunning) {
                        SDL_Event event;
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_EVENT_QUIT) {
                                gameRunning = false;
                                quit = true;
                                exitCode = 1;
                            }
                            int code = game.handleEvent(event);
                            if (code > 0 && game.isGameOver()) {
                                exitCode = code;
                                gameRunning = false;
                            }
                        }
                        net.update();
                        disc.update();
                        game.render();
                        SDL_Delay(16);
                    }
                    if (exitCode == 1 || quit) {
                        nextGame = false;
                        break;
                    } else if (exitCode == 2) {
                        nextGame = true;
                        continue;
                    }
                } while (nextGame);
            }
            disc.quit();
            net.quit();
            continue;
        }

        // Обычные режимы Blackjack / Poker
        Menu::PlayerSetup playersSetup = Menu::showPlayerSetupMenu();
        if (playersSetup.humans == 0 && playersSetup.bots == 0) continue;

        std::vector<Player*> players;
        for (int i = 0; i < playersSetup.humans; ++i)
            players.push_back(new HumanPlayer("Player " + std::to_string(i + 1), 1000));
        for (int i = 0; i < playersSetup.bots; ++i)
            players.push_back(new BotPlayer("Bot " + std::to_string(i + 1), 1000));

        bool nextGame = false;
        do {
            Game* game = nullptr;
            if (choice == Menu::GameChoice::Blackjack)
                game = new BlackjackGame(players);
            else if (choice == Menu::GameChoice::Poker)
                game = new PokerGame(players);

            if (game) {
                game->run();
                bool gameRunning = true;
                int exitCode = 0;

                while (gameRunning) {
                    SDL_Event event;
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_EVENT_QUIT) {
                            gameRunning = false;
                            quit = true;
                            exitCode = 1;
                        }
                        int code = game->handleEvent(event);
                        if (code > 0 && game->isGameOver()) {
                            exitCode = code;
                            gameRunning = false;
                        }
                    }
                    game->render();
                    SDL_Delay(16);
                }
                delete game;

                if (exitCode == 1 || quit) {
                    nextGame = false;
                    break;
                } else if (exitCode == 2) {
                    nextGame = true;
                    continue;
                }
            }
            nextGame = false;
        } while (nextGame);

        for (Player* p : players) delete p;
        players.clear();
    }

    Renderer::getInstance().shutdown();
    return 0;
}