#include <SDL3/SDL.h>
#include "Renderer.h"
#include "Menu.h"
#include "Player.h"
#include "BlackjackGame.h"
#include "PokerGame.h"
#include "LANBlackjackGame.h"
#include "NetworkManager.h"
#include "NetworkDiscovery.h"
#include <vector>
#include <iostream>

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

        // LAN Blackjack режимы
        if (choice == Menu::GameChoice::LANBlackjackHost || choice == Menu::GameChoice::LANBlackjackJoin) {
            auto& net = NetworkManager::getInstance();
            auto& disc = NetworkDiscovery::getInstance();
            if (!net.init()) {
                continue;
            }
            disc.init();

            if (choice == Menu::GameChoice::LANBlackjackHost) {
                uint16_t port = 12345;
                std::string portStr = Menu::inputText("Enter game port (default 12345):", "12345", 400, 300);
                if (!portStr.empty()) port = (uint16_t)std::stoi(portStr);
                if (!net.createServer(port)) {
                    net.quit();
                    disc.quit();
                    continue;
                }
                // Запускаем broadcast на порту 12346 (discovery)
                disc.startBroadcasting(port, "BlackjackServer");
                std::cout << "Hosting game on port " << port << ". Broadcasting discovery on port 12346" << std::endl;

                // Создаём игроков на сервере (хост играет локально)
                std::vector<Player*> players;
                players.push_back(new HumanPlayer("Host", 1000));
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
                    game.render();
                    disc.update(); // обновляем discovery (отправка broadcast)
                    net.update(); // обработка входящих сообщений от клиента
                    SDL_Delay(16);
                }
                for (auto p : players) delete p;
                disc.stopBroadcasting();
            } else {
                // LAN Blackjack Join (клиент)
                const uint16_t discoveryPort = 12346;
                disc.startDiscovery(discoveryPort);
                std::cout << "Searching for servers on port " << discoveryPort << "..." << std::endl;
                // Ждём 3 секунды для сбора ответов
                uint64_t startWait = SDL_GetTicks();
                while (SDL_GetTicks() - startWait < 3000) {
                    disc.update();
                    SDL_Delay(50);
                }
                auto servers = disc.getServers();
                std::string chosenIP;
                uint16_t chosenPort = 12345;
                if (!servers.empty()) {
                    std::cout << "Found " << servers.size() << " server(s):" << std::endl;
                    for (size_t i = 0; i < servers.size(); ++i) {
                        std::cout << i+1 << ". " << servers[i].ip << ":" << servers[i].gamePort << " - " << servers[i].name << std::endl;
                    }
                    std::string choiceIdx = Menu::inputText("Enter server number (or press Enter for first):", "1", 400, 300);
                    int idx = 1;
                    if (!choiceIdx.empty()) idx = std::stoi(choiceIdx);
                    if (idx >= 1 && idx <= (int)servers.size()) {
                        chosenIP = servers[idx-1].ip;
                        chosenPort = servers[idx-1].gamePort;
                    } else {
                        chosenIP = servers[0].ip;
                        chosenPort = servers[0].gamePort;
                    }
                } else {
                    std::cout << "No servers found. Please enter IP manually." << std::endl;
                    chosenIP = Menu::inputText("Enter server IP (default 127.0.0.1):", "127.0.0.1", 400, 300);
                    std::string portStr = Menu::inputText("Enter game port (default 12345):", "12345", 400, 350);
                    if (!portStr.empty()) chosenPort = (uint16_t)std::stoi(portStr);
                }
                std::cout << "Connecting to " << chosenIP << ":" << chosenPort << std::endl;
                if (!net.connectToServer(chosenIP.c_str(), chosenPort)) {
                    std::cerr << "Failed to connect to server." << std::endl;
                    disc.stopDiscovery();
                    net.quit();
                    disc.quit();
                    continue;
                }
                // Создаём пустой список игроков (клиент получит состояние от сервера)
                std::vector<Player*> players; // пусто
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
                    game.render();
                    net.update(); // приём входящих сообщений от сервера
                    SDL_Delay(16);
                }
                disc.stopDiscovery();
            }
            net.quit();
            disc.quit();
            continue;
        }

        // Обычные режимы (Blackjack, Poker)
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