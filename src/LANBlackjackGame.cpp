#include "LANBlackjackGame.h"
#include "NetworkManager.h"
#include "Renderer.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

LANBlackjackGame::LANBlackjackGame(const std::vector<Player*>& players, bool isServer)
    : Game(players), m_isServer(isServer), m_clientPlayerId(0) {
    m_dealer = new BotPlayer("Dealer", 999999);
    if (m_isServer) {
        m_playerBets.resize(m_players.size(), 0);
        m_playerDone.resize(m_players.size(), false);
        m_playerResult.resize(m_players.size(), "");
        m_playerResultColor.resize(m_players.size(), SDL_Color{200,200,200,255});
    } else {
        m_playerResult.resize(6, "");
        m_playerResultColor.resize(6, SDL_Color{200,200,200,255});
    }
    memset(&m_lastState, 0, sizeof(GameStatePacket));
}

LANBlackjackGame::~LANBlackjackGame() {
    delete m_dealer;
}

void LANBlackjackGame::run() {
    auto& net = NetworkManager::getInstance();
    if (m_isServer) {
        net.setOnClientMessage([this](const void* data, int len) { onPacketReceived(data, len); });
        startNewRound();
        std::cout << "[SERVER] Game started, waiting for clients..." << std::endl;
    } else {
        net.setOnClientMessage([this](const void* data, int len) { onPacketReceived(data, len); });
        m_gameOver = false;
        m_waitingForAction = false;
        m_clientPlayerId = 1; // Клиент управляет вторым игроком (индекс 1)
        std::cout << "[CLIENT] Waiting for game state from server..." << std::endl;
    }
}

void LANBlackjackGame::startNewRound() {
    m_deck.reset();
    m_gameOver = false;
    m_waitingForAction = true;
    m_currentPlayerIndex = 0;
    m_botTimer = 0;
    m_playerBets.assign(m_players.size(), 0);
    m_playerDone.assign(m_players.size(), false);
    m_playerResult.assign(m_players.size(), "");
    m_playerResultColor.assign(m_players.size(), SDL_Color{200,200,200,255});

    for (auto p : m_players) p->clearHand();
    m_dealer->clearHand();

    for (size_t i = 0; i < m_players.size(); ++i)
        askBet(m_players[i], static_cast<int>(i));

    for (int i = 0; i < 2; ++i) {
        for (size_t j = 0; j < m_players.size(); ++j) {
            if (m_playerBets[j] > 0 && !m_playerDone[j]) {
                m_players[j]->addCard(m_deck.dealCard());
            }
        }
        m_dealer->addCard(m_deck.dealCard());
    }

    while (m_currentPlayerIndex < (int)m_players.size() &&
           (m_playerBets[m_currentPlayerIndex] == 0 || m_playerDone[m_currentPlayerIndex])) {
        m_currentPlayerIndex++;
    }

    if (m_currentPlayerIndex >= (int)m_players.size()) {
        m_waitingForAction = false;
        dealerTurn();
    }
    sendFullStateToClient();
}

void LANBlackjackGame::askBet(Player* p, int idx) {
    int maxBet = p->getMoney();
    if (maxBet <= 0) {
        m_playerBets[idx] = 0;
        m_playerDone[idx] = true;
        return;
    }
    int bet = std::min(50, maxBet);
    if (bet < 1) bet = 1;
    m_playerBets[idx] = bet;
    p->placeBet(bet);
}

void LANBlackjackGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();
    int winW, winH;
    r.getWindowSize(winW, winH);

    if (!m_isServer) {
        // Клиент отрисовывает m_lastState
        std::string dealerScore = m_lastState.gameOver ? std::to_string(m_lastState.dealerHandValue) : "?";
        r.drawText("DEALER: " + dealerScore, winW/2 - 60, 20, {255,215,0,255});
        int dx = winW/2 - m_lastState.dealerCardCount * 40;
        for (int i = 0; i < m_lastState.dealerCardCount; ++i) {
            Card card(static_cast<Suit>(m_lastState.dealerCards[i][1]), static_cast<Rank>(m_lastState.dealerCards[i][0]));
            bool hidden = (i == 1 && !m_lastState.gameOver);
            r.drawCard(card, dx, 60, 70, 105, hidden);
            dx += 80;
        }
        int count = m_lastState.playerCount;
        int radiusX = winW/2 - 120;
        int radiusY = winH/3;
        int centerX = winW/2;
        int centerY = winH - 280;
        for (int i = 0; i < count; ++i) {
            double angle = M_PI;
            if (count > 1) angle = M_PI - (M_PI * i / (count-1));
            else angle = M_PI/2;
            int x = centerX + (int)(radiusX * cos(angle)) - 35;
            int y = centerY - (int)(radiusY * sin(angle));
            SDL_Color nameColor = (m_lastState.currentPlayerId == m_lastState.players[i].playerId && m_waitingForAction && !m_gameOver) ?
                                  SDL_Color{0,255,0,255} : SDL_Color{255,255,255,255};
            r.drawText(m_lastState.players[i].name, x-20, y-50, nameColor);
            r.drawText("Money: $" + std::to_string(m_lastState.players[i].money), x-20, y-30, {200,200,200,255});
            r.drawText("Bet: $" + std::to_string(m_lastState.players[i].bet), x-20, y+120, {255,215,0,255});
            r.drawText("Score: " + std::to_string(m_lastState.players[i].handValue), x-20, y+140, {255,255,255,255});
            if (strlen(m_lastState.players[i].result) > 0) {
                SDL_Color resColor = {m_lastState.players[i].resultColorR, m_lastState.players[i].resultColorG, m_lastState.players[i].resultColorB, 255};
                r.drawText(m_lastState.players[i].result, x-20, y+160, resColor);
            }
            for (int c = 0; c < m_lastState.players[i].cardCount; ++c) {
                Card card(static_cast<Suit>(m_lastState.players[i].cards[c][1]), static_cast<Rank>(m_lastState.players[i].cards[c][0]));
                r.drawCard(card, x + c*25, y, 70, 105, false);
            }
        }
        // Рисуем кнопки для клиента (с учётом возможности Double)
        if (m_waitingForAction && !m_gameOver) {
            int btnY = winH - 70;
            r.drawButton("Hit", winW/2 - 220, btnY, 100, 50);
            r.drawButton("Stand", winW/2 - 110, btnY, 100, 50);
            // Добавляем Double, если у клиента две карты и достаточно денег
            if (m_lastState.playerCount > m_clientPlayerId && m_lastState.players[m_clientPlayerId].cardCount == 2) {
                int16_t money = m_lastState.players[m_clientPlayerId].money;
                int16_t bet = m_lastState.players[m_clientPlayerId].bet;
                if (money >= bet) {
                    r.drawButton("Double", winW/2, btnY, 100, 50);
                }
            }
        } else if (m_gameOver) {
            r.drawButton("Main Menu", winW/2 - 160, winH/2 + 120, 140, 40);
            r.drawButton("Next Game", winW/2 + 20, winH/2 + 120, 140, 40);
        }
        r.present();
        return;
    }

    // Серверная отрисовка (локальная) – без изменений
    std::string dealerScore = m_waitingForAction ? "?" : std::to_string(m_dealer->getHandValue());
    r.drawText("DEALER: " + dealerScore, winW/2 - 60, 20, {255,215,0,255});
    int dx = winW/2 - (int)m_dealer->getHand().size() * 40;
    for (size_t i = 0; i < m_dealer->getHand().size(); ++i) {
        bool hidden = (i == 1 && m_waitingForAction);
        r.drawCard(m_dealer->getHand()[i], dx, 60, 70, 105, hidden);
        dx += 80;
    }
    int count = (int)m_players.size();
    int radiusX = winW/2 - 120;
    int radiusY = winH/3;
    int centerX = winW/2;
    int centerY = winH - 280;
    for (int i = 0; i < count; ++i) {
        if (m_playerBets[i] == 0 && m_playerDone[i]) continue;
        double angle = M_PI;
        if (count > 1) angle = M_PI - (M_PI * i / (count-1));
        else angle = M_PI/2;
        int x = centerX + (int)(radiusX * cos(angle)) - 35;
        int y = centerY - (int)(radiusY * sin(angle));
        SDL_Color nameColor = (i == m_currentPlayerIndex && m_waitingForAction) ? SDL_Color{0,255,0,255} : SDL_Color{255,255,255,255};
        std::string status = "";
        if (m_players[i]->isBust()) status = " (Bust!)";
        else if (m_playerDone[i]) status = " (Done)";
        r.drawText(m_players[i]->getName() + status, x-20, y-50, nameColor);
        r.drawText("Money: $" + std::to_string(m_players[i]->getMoney()), x-20, y-30, {200,200,200,255});
        r.drawText("Bet: $" + std::to_string(m_playerBets[i]), x-20, y+120, {255,215,0,255});
        r.drawText("Score: " + std::to_string(m_players[i]->getHandValue()), x-20, y+140, {255,255,255,255});
        if (!m_playerResult[i].empty()) {
            r.drawText(m_playerResult[i], x-20, y+160, m_playerResultColor[i]);
        }
        for (size_t cardIdx = 0; cardIdx < m_players[i]->getHand().size(); ++cardIdx) {
            r.drawCard(m_players[i]->getHand()[cardIdx], x + cardIdx*25, y, 70, 105, false);
        }
    }
    if (m_waitingForAction && m_currentPlayerIndex < count) {
        Player* current = m_players[m_currentPlayerIndex];
        if (current->isBot()) {
            uint64_t currentTicks = SDL_GetTicks();
            if (m_botTimer == 0) m_botTimer = currentTicks;
            if (currentTicks - m_botTimer >= 600) {
                std::string action = current->getAction({"hit","stand"});
                if (action == "hit") {
                    current->addCard(m_deck.dealCard());
                    if (current->isBust()) {
                        m_playerDone[m_currentPlayerIndex] = true;
                        nextPlayer();
                    }
                } else {
                    m_playerDone[m_currentPlayerIndex] = true;
                    nextPlayer();
                }
                m_botTimer = 0;
                sendFullStateToClient();
            }
        } else {
            int btnY = winH - 70;
            r.drawButton("Hit", winW/2 - 220, btnY, 100, 50);
            r.drawButton("Stand", winW/2 - 110, btnY, 100, 50);
            if (current->getHand().size() == 2 && current->canAfford(m_playerBets[m_currentPlayerIndex])) {
                r.drawButton("Double", winW/2, btnY, 100, 50);
            }
        }
    } else if (m_gameOver) {
        r.drawButton("Main Menu", winW/2 - 160, winH/2 + 120, 140, 40);
        r.drawButton("Next Game", winW/2 + 20, winH/2 + 120, 140, 40);
    }
    r.present();
}

int LANBlackjackGame::handleEvent(const SDL_Event& event) {
    if (m_gameOver) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            auto& r = Renderer::getInstance();
            if (r.isButtonClicked(winW/2 - 160, winH/2 + 120, 140, 40)) return 1;
            if (r.isButtonClicked(winW/2 + 20, winH/2 + 120, 140, 40)) return 2;
        }
        return 0;
    }

    if (m_isServer) {
        // Сервер управляет только игроком с индексом 0 (Host)
        if (m_waitingForAction && m_currentPlayerIndex == 0 && m_currentPlayerIndex < (int)m_players.size()) {
            Player* current = m_players[m_currentPlayerIndex];
            if (!current->isBot() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int winW, winH;
                Renderer::getInstance().getWindowSize(winW, winH);
                auto& r = Renderer::getInstance();
                int btnY = winH - 70;
                if (r.isButtonClicked(winW/2 - 220, btnY, 100, 50)) {
                    current->addCard(m_deck.dealCard());
                    if (current->isBust()) {
                        m_playerDone[m_currentPlayerIndex] = true;
                        nextPlayer();
                    }
                    sendFullStateToClient();
                } else if (r.isButtonClicked(winW/2 - 110, btnY, 100, 50)) {
                    m_playerDone[m_currentPlayerIndex] = true;
                    nextPlayer();
                    sendFullStateToClient();
                } else if (current->getHand().size() == 2 && current->canAfford(m_playerBets[m_currentPlayerIndex]) &&
                           r.isButtonClicked(winW/2, btnY, 100, 50)) {
                    int doubleBet = m_playerBets[m_currentPlayerIndex];
                    current->placeBet(doubleBet);
                    m_playerBets[m_currentPlayerIndex] += doubleBet;
                    current->addCard(m_deck.dealCard());
                    m_playerDone[m_currentPlayerIndex] = true;
                    nextPlayer();
                    sendFullStateToClient();
                }
            }
        }
    } else {
        // Клиент управляет только игроком с индексом 1 (Client)
        if (m_waitingForAction && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            auto& r = Renderer::getInstance();
            int btnY = winH - 70;
            if (r.isButtonClicked(winW/2 - 220, btnY, 100, 50)) {
                sendAction(BlackjackAction::Hit);
            } else if (r.isButtonClicked(winW/2 - 110, btnY, 100, 50)) {
                sendAction(BlackjackAction::Stand);
            } else if (r.isButtonClicked(winW/2, btnY, 100, 50)) {
                // Проверка возможности Double
                if (m_lastState.playerCount > m_clientPlayerId && m_lastState.players[m_clientPlayerId].cardCount == 2) {
                    int16_t money = m_lastState.players[m_clientPlayerId].money;
                    int16_t bet = m_lastState.players[m_clientPlayerId].bet;
                    if (money >= bet) {
                        sendAction(BlackjackAction::Double);
                    }
                }
            }
        }
    }
    return 0;
}

void LANBlackjackGame::nextPlayer() {
    m_currentPlayerIndex++;
    m_botTimer = 0;
    while (m_currentPlayerIndex < (int)m_players.size() &&
           (m_playerBets[m_currentPlayerIndex] == 0 || m_playerDone[m_currentPlayerIndex])) {
        m_currentPlayerIndex++;
    }
    if (m_currentPlayerIndex >= (int)m_players.size()) {
        m_waitingForAction = false;
        dealerTurn();
        sendFullStateToClient();
    }
}

void LANBlackjackGame::dealerTurn() {
    while (m_dealer->getHandValue() < 17) {
        m_dealer->addCard(m_deck.dealCard());
    }
    resolveBets();
    m_gameOver = true;
    sendFullStateToClient();
}

void LANBlackjackGame::resolveBets() {
    int dealerVal = m_dealer->getHandValue();
    bool dealerBust = dealerVal > 21;
    for (size_t i = 0; i < m_players.size(); ++i) {
        if (m_playerBets[i] == 0) continue;
        Player* p = m_players[i];
        int pVal = p->getHandValue();
        if (pVal > 21) {
            m_playerResult[i] = "Lose!";
            m_playerResultColor[i] = {255,80,80,255};
            continue;
        }
        if (dealerBust || pVal > dealerVal) {
            p->winMoney(m_playerBets[i] * 2);
            m_playerResult[i] = "Win! +$" + std::to_string(m_playerBets[i]);
            m_playerResultColor[i] = {80,255,80,255};
        } else if (pVal == dealerVal) {
            p->winMoney(m_playerBets[i]);
            m_playerResult[i] = "Push";
            m_playerResultColor[i] = {255,255,100,255};
        } else {
            m_playerResult[i] = "Lose!";
            m_playerResultColor[i] = {255,80,80,255};
        }
    }
}

void LANBlackjackGame::sendAction(BlackjackAction action) {
    PlayerActionPacket packet;
    packet.type = PacketType::PlayerAction;
    packet.playerId = m_clientPlayerId;
    packet.action = action;
    NetworkManager::getInstance().sendToServer(&packet, sizeof(packet));
    std::cout << "[CLIENT] Sent action: " << (int)action << std::endl;
}

void LANBlackjackGame::onPacketReceived(const void* data, int len) {
    if (len < 1) return;
    PacketType type = *(PacketType*)data;
    if (type == PacketType::GameState && len >= sizeof(GameStatePacket)) {
        GameStatePacket state;
        memcpy(&state, data, sizeof(GameStatePacket));
        m_lastState = state;
        m_gameOver = state.gameOver;
        if (!m_isServer) {
            for (uint8_t i = 0; i < state.playerCount; ++i) {
                m_playerResult[i] = std::string(state.players[i].result);
                m_playerResultColor[i] = {state.players[i].resultColorR, state.players[i].resultColorG, state.players[i].resultColorB, 255};
            }
            m_waitingForAction = !m_gameOver && (state.currentPlayerId == m_clientPlayerId);
            std::cout << "[CLIENT] Received GameState, gameOver=" << m_gameOver << ", myTurn=" << m_waitingForAction << std::endl;
        } else {
            for (uint8_t i = 0; i < state.playerCount; ++i) {
                if (i < m_players.size()) {
                    m_playerResult[i] = std::string(state.players[i].result);
                    m_playerResultColor[i] = {state.players[i].resultColorR, state.players[i].resultColorG, state.players[i].resultColorB, 255};
                }
            }
        }
    } else if (type == PacketType::PlayerAction && m_isServer && len >= sizeof(PlayerActionPacket)) {
        PlayerActionPacket action;
        memcpy(&action, data, sizeof(PlayerActionPacket));
        if (m_waitingForAction && action.playerId == 1 && m_currentPlayerIndex == 1) {
            Player* current = m_players[m_currentPlayerIndex];
            if (action.action == BlackjackAction::Hit) {
                current->addCard(m_deck.dealCard());
                if (current->isBust()) {
                    m_playerDone[m_currentPlayerIndex] = true;
                    nextPlayer();
                }
            } else if (action.action == BlackjackAction::Stand) {
                m_playerDone[m_currentPlayerIndex] = true;
                nextPlayer();
            } else if (action.action == BlackjackAction::Double) {
                if (current->getHand().size() == 2 && current->canAfford(m_playerBets[m_currentPlayerIndex])) {
                    int doubleBet = m_playerBets[m_currentPlayerIndex];
                    current->placeBet(doubleBet);
                    m_playerBets[m_currentPlayerIndex] += doubleBet;
                    current->addCard(m_deck.dealCard());
                    m_playerDone[m_currentPlayerIndex] = true;
                    nextPlayer();
                }
            }
            sendFullStateToClient();
        }
    }
}

void LANBlackjackGame::sendFullStateToClient() {
    if (!m_isServer) return;
    GameStatePacket packet;
    packet.type = PacketType::GameState;
    packet.currentPlayerId = m_currentPlayerIndex;
    packet.dealerCardCount = (uint8_t)m_dealer->getHand().size();
    for (size_t i = 0; i < m_dealer->getHand().size(); ++i) {
        packet.dealerCards[i][0] = (uint8_t)m_dealer->getHand()[i].getRank();
        packet.dealerCards[i][1] = (uint8_t)m_dealer->getHand()[i].getSuit();
    }
    packet.dealerHandValue = m_dealer->getHandValue();
    packet.playerCount = (uint8_t)m_players.size();
    for (size_t i = 0; i < m_players.size(); ++i) {
        packet.players[i].playerId = (uint32_t)i;
        strncpy(packet.players[i].name, m_players[i]->getName().c_str(), 19);
        packet.players[i].name[19] = '\0';
        packet.players[i].money = m_players[i]->getMoney();
        packet.players[i].bet = m_playerBets[i];
        packet.players[i].handValue = m_players[i]->getHandValue();
        packet.players[i].cardCount = (uint8_t)m_players[i]->getHand().size();
        for (size_t c = 0; c < m_players[i]->getHand().size(); ++c) {
            packet.players[i].cards[c][0] = (uint8_t)m_players[i]->getHand()[c].getRank();
            packet.players[i].cards[c][1] = (uint8_t)m_players[i]->getHand()[c].getSuit();
        }
        packet.players[i].bust = m_players[i]->isBust();
        packet.players[i].done = m_playerDone[i];
        packet.players[i].isBot = m_players[i]->isBot();
        strncpy(packet.players[i].result, m_playerResult[i].c_str(), 19);
        packet.players[i].result[19] = '\0';
        packet.players[i].resultColorR = m_playerResultColor[i].r;
        packet.players[i].resultColorG = m_playerResultColor[i].g;
        packet.players[i].resultColorB = m_playerResultColor[i].b;
    }
    packet.gameOver = m_gameOver;
    NetworkManager::getInstance().sendToClient(&packet, sizeof(packet));
    std::cout << "[SERVER] Sent GameStatePacket to client" << std::endl;
}