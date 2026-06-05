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
                std::string portStr = Menu::inputText("Enter port (default 12345):", "12345", 400, 300);
                if (!portStr.empty()) port = (uint16_t)std::stoi(portStr);
                if (!net.createServer(port)) {
                    net.quit();
                    disc.quit();
                    continue;
                }
                disc.startBroadcasting(port, "BlackjackServer");
                disc.startDiscovery(12346);
                std::vector<Player*> players;
                players.push_back(new HumanPlayer("Host", 1000));
                players.push_back(new HumanPlayer("Client", 1000)); // второй игрок для клиента
                LANBlackjackGame game(players, true);
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
                for (auto p : players) delete p;
                disc.stopBroadcasting();
            } else {
                disc.startDiscovery(12346);
                std::cout << "Searching for servers..." << std::endl;
                uint64_t discoveryEnd = SDL_GetTicks() + 3000;
                while (SDL_GetTicks() < discoveryEnd) {
                    disc.update();
                    SDL_Delay(100);
                }
                auto servers = disc.getServers();
                std::string chosenIP;
                uint16_t port = 12345;
                if (!servers.empty()) {
                    for (size_t i = 0; i < servers.size(); ++i) {
                        std::cout << i+1 << ". " << servers[i].ip << ":" << servers[i].port << " (" << servers[i].name << ")" << std::endl;
                    }
                    std::cout << "Select server (1-" << servers.size() << "): ";
                    int idx;
                    std::cin >> idx;
                    if (idx >= 1 && idx <= (int)servers.size()) {
                        chosenIP = servers[idx-1].ip;
                        port = servers[idx-1].port;
                    } else {
                        chosenIP = Menu::inputText("Invalid, enter IP manually:", "127.0.0.1", 400, 300);
                        std::string portStr = Menu::inputText("Enter port (default 12345):", "12345", 400, 350);
                        if (!portStr.empty()) port = (uint16_t)std::stoi(portStr);
                    }
                } else {
                    chosenIP = Menu::inputText("No servers found, enter IP manually:", "127.0.0.1", 400, 300);
                    std::string portStr = Menu::inputText("Enter port (default 12345):", "12345", 400, 350);
                    if (!portStr.empty()) port = (uint16_t)std::stoi(portStr);
                }
                if (!net.connectToServer(chosenIP.c_str(), port)) {
                    disc.stopDiscovery();
                    net.quit();
                    disc.quit();
                    continue;
                }
                std::vector<Player*> players; // пусто, клиент не создаёт игроков
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
                disc.stopDiscovery();
            }
            disc.quit();
            net.quit();
            continue;
        }

        // Обычные режимы
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