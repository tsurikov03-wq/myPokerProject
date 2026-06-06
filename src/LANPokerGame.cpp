#include "LANPokerGame.h"
#include "NetworkManager.h"
#include "Renderer.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>

// Структура для результата ранга руки
struct HandRankResult {
    int rank;
    std::vector<int> kickers;
};

static HandRankResult getFullHandRank(const std::vector<Card>& hand) {
    int bestCategory = -1;
    std::vector<int> bestKickers;
    std::vector<std::vector<int>> combos;
    for (int i = 0; i < 7; ++i)
        for (int j = i+1; j < 7; ++j) {
            std::vector<int> combo;
            for (int k = 0; k < 7; ++k)
                if (k != i && k != j) combo.push_back(k);
            combos.push_back(combo);
        }
    for (const auto& combo : combos) {
        std::vector<Card> five;
        for (int idx : combo) five.push_back(hand[idx]);
        int cat = evaluateHandRank(five);
        if (cat > bestCategory) {
            bestCategory = cat;
            std::vector<int> vals;
            for (const auto& c : five) vals.push_back(c.getPokerValue());
            std::sort(vals.begin(), vals.end(), std::greater<int>());
            bestKickers = vals;
        } else if (cat == bestCategory) {
            std::vector<int> vals;
            for (const auto& c : five) vals.push_back(c.getPokerValue());
            std::sort(vals.begin(), vals.end(), std::greater<int>());
            if (vals > bestKickers) bestKickers = vals;
        }
    }
    return {bestCategory, bestKickers};
}

LANPokerGame::LANPokerGame(const std::vector<Player*>& players, bool isServer)
    : Game(players), m_isServer(isServer), m_clientPlayerId(0),
      m_stage(Stage::Preflop), m_smallBlind(10), m_bigBlind(20),
      m_currentBet(0), m_pot(0), m_dealerButton(0), m_currentPlayerIdx(0),
      m_lastRaiserIdx(-1), m_playersInHand(0), m_waitingForAction(false) {
    memset(&m_lastState, 0, sizeof(PokerStatePacket));
    if (!m_isServer) {
        m_players.clear(); // клиент не хранит игроков
    }
}

LANPokerGame::~LANPokerGame() {}

void LANPokerGame::run() {
    auto& net = NetworkManager::getInstance();
    if (m_isServer) {
        net.setOnClientMessage([this](const void* data, int len) { onPacketReceived(data, len); });
        startNewRound();
        std::cout << "[POKER SERVER] Game started" << std::endl;
    } else {
        net.setOnClientMessage([this](const void* data, int len) { onPacketReceived(data, len); });
        m_gameOver = false;
        m_waitingForAction = false;
        std::cout << "[POKER CLIENT] Waiting for game state..." << std::endl;
    }
}

void LANPokerGame::startNewRound() {
    m_gameOver = false;
    m_deck.reset();
    m_stage = Stage::Preflop;
    m_communityCards.clear();
    m_pot = 0;
    m_currentBet = 0;
    m_lastRaiserIdx = -1;
    m_winnerText = "";

    for (auto p : m_players) {
        p->clearHand();
        if (p->getMoney() <= 0) {
            p->m_hasFolded = true;
            p->m_isAllIn = false;
            p->m_hasActed = true;
        } else {
            p->m_hasFolded = false;
            p->m_isAllIn = false;
            p->m_hasActed = false;
        }
        p->m_currentBet = 0;
    }

    for (int i = 0; i < 2; ++i) {
        for (auto p : m_players) {
            if (!p->m_hasFolded) {
                p->addCard(m_deck.dealCard());
            }
        }
    }

    while (m_players[m_dealerButton]->m_hasFolded) {
        m_dealerButton = (m_dealerButton + 1) % m_players.size();
    }
    int sbIdx = (m_dealerButton + 1) % m_players.size();
    int bbIdx = (sbIdx + 1) % m_players.size();
    while (m_players[sbIdx]->m_hasFolded) sbIdx = (sbIdx + 1) % m_players.size();
    while (m_players[bbIdx]->m_hasFolded) bbIdx = (bbIdx + 1) % m_players.size();

    if (m_players[sbIdx]->getMoney() >= m_smallBlind) {
        m_players[sbIdx]->placeBet(m_smallBlind);
        m_pot += m_smallBlind;
        m_players[sbIdx]->m_currentBet = m_smallBlind;
    } else {
        m_players[sbIdx]->m_hasFolded = true;
    }
    if (m_players[bbIdx]->getMoney() >= m_bigBlind) {
        m_players[bbIdx]->placeBet(m_bigBlind);
        m_pot += m_bigBlind;
        m_players[bbIdx]->m_currentBet = m_bigBlind;
    } else {
        m_players[bbIdx]->m_hasFolded = true;
    }

    m_currentBet = m_bigBlind;
    m_currentPlayerIdx = (bbIdx + 1) % m_players.size();
    while (m_players[m_currentPlayerIdx]->m_hasFolded) {
        m_currentPlayerIdx = (m_currentPlayerIdx + 1) % m_players.size();
    }

    m_playersInHand = 0;
    for (auto p : m_players) if (!p->m_hasFolded) m_playersInHand++;
    m_waitingForAction = true;
    m_lastRaiserIdx = bbIdx;

    if (m_playersInHand <= 1) {
        for (auto p : m_players) {
            if (!p->m_hasFolded) {
                p->winMoney(m_pot);
                m_winnerText = p->getName() + " wins the pot (everyone else folded)!";
                m_gameOver = true;
            }
        }
    }
    sendFullStateToClient();
}

void LANPokerGame::advanceStage() {
    int activeCount = 0;
    Player* lastPlayer = nullptr;
    for (auto p : m_players) {
        if (!p->m_hasFolded) {
            activeCount++;
            lastPlayer = p;
        }
    }
    if (activeCount == 1 && m_stage != Stage::Showdown) {
        lastPlayer->winMoney(m_pot);
        m_pot = 0;
        m_gameOver = true;
        m_winnerText = lastPlayer->getName() + " wins the pot (everyone else folded)!";
        sendFullStateToClient();
        return;
    }

    switch (m_stage) {
        case Stage::Preflop:
            m_stage = Stage::Flop;
            m_communityCards.push_back(m_deck.dealCard());
            m_communityCards.push_back(m_deck.dealCard());
            m_communityCards.push_back(m_deck.dealCard());
            break;
        case Stage::Flop:
            m_stage = Stage::Turn;
            m_communityCards.push_back(m_deck.dealCard());
            break;
        case Stage::Turn:
            m_stage = Stage::River;
            m_communityCards.push_back(m_deck.dealCard());
            break;
        case Stage::River:
            m_stage = Stage::Showdown;
            evaluateAndPayWinners();
            m_gameOver = true;
            sendFullStateToClient();
            return;
        default: break;
    }
    m_currentBet = 0;
    m_lastRaiserIdx = -1;
    for (auto p : m_players) {
        if (!p->m_hasFolded && !p->m_isAllIn) p->m_hasActed = false;
        p->m_currentBet = 0;
    }
    m_currentPlayerIdx = (m_dealerButton + 1) % m_players.size();
    while (m_players[m_currentPlayerIdx]->m_hasFolded)
        m_currentPlayerIdx = (m_currentPlayerIdx + 1) % m_players.size();
    m_waitingForAction = true;
    sendFullStateToClient();
}

void LANPokerGame::nextPlayer() {
    int start = m_currentPlayerIdx;
    do {
        m_currentPlayerIdx = (m_currentPlayerIdx + 1) % m_players.size();
    } while (m_players[m_currentPlayerIdx]->m_hasFolded && m_currentPlayerIdx != start);
    if (isRoundComplete()) advanceStage();
    else sendFullStateToClient();
}

bool LANPokerGame::isRoundComplete() {
    int active = 0, acted = 0;
    for (auto p : m_players) {
        if (!p->m_hasFolded) {
            active++;
            if (p->m_hasActed || p->m_isAllIn) acted++;
        }
    }
    bool allBetsEqual = true;
    int betAmount = -1;
    for (auto p : m_players) {
        if (!p->m_hasFolded && !p->m_isAllIn) {
            if (betAmount == -1) betAmount = p->m_currentBet;
            else if (p->m_currentBet != betAmount) allBetsEqual = false;
        }
    }
    return (acted >= active && allBetsEqual) && active > 0;
}

void LANPokerGame::evaluateAndPayWinners() {
    std::vector<Player*> activePlayers;
    for (auto p : m_players) {
        if (!p->m_hasFolded) activePlayers.push_back(p);
    }
    if (activePlayers.empty()) return;

    std::vector<std::pair<Player*, HandRankResult>> ranks;
    for (Player* p : activePlayers) {
        std::vector<Card> allCards = p->getHand();
        allCards.insert(allCards.end(), m_communityCards.begin(), m_communityCards.end());
        HandRankResult res = getFullHandRank(allCards);
        ranks.push_back({p, res});
    }

    std::sort(ranks.begin(), ranks.end(),
        [](const std::pair<Player*, HandRankResult>& a,
           const std::pair<Player*, HandRankResult>& b) {
            if (a.second.rank != b.second.rank)
                return a.second.rank > b.second.rank;
            return a.second.kickers > b.second.kickers;
        });

    int bestRank = ranks[0].second.rank;
    auto bestKickers = ranks[0].second.kickers;
    std::vector<Player*> winners;
    for (const auto& entry : ranks) {
        if (entry.second.rank == bestRank && entry.second.kickers == bestKickers)
            winners.push_back(entry.first);
        else
            break;
    }

    int share = m_pot / (int)winners.size();
    for (Player* w : winners) {
        w->winMoney(share);
    }

    m_winnerText = "Winner: ";
    for (size_t i = 0; i < winners.size(); ++i) {
        if (i > 0) m_winnerText += ", ";
        m_winnerText += winners[i]->getName();
    }
    m_winnerText += " wins $" + std::to_string(share) + (winners.size() > 1 ? " each!" : "!");
    m_pot = 0;
}

void LANPokerGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();
    int winW, winH;
    r.getWindowSize(winW, winH);

    if (!m_isServer) {
        // Клиент отрисовывает m_lastState
        std::string stageText;
        switch (m_lastState.stage) {
            case 0: stageText = "PRE-FLOP"; break;
            case 1: stageText = "FLOP"; break;
            case 2: stageText = "TURN"; break;
            case 3: stageText = "RIVER"; break;
            case 4: stageText = "SHOWDOWN"; break;
        }
        r.drawText(stageText, winW/2 - 40, 10, {255,200,100,255});
        r.drawText("POT: $" + std::to_string(m_lastState.pot), winW/2 - 50, 30, {255,215,0,255});

        int ccX = winW/2 - m_lastState.communityCardCount * 40;
        for (int i = 0; i < m_lastState.communityCardCount; ++i) {
            Card card(static_cast<Suit>(m_lastState.communityCards[i][1]), static_cast<Rank>(m_lastState.communityCards[i][0]));
            r.drawCard(card, ccX, winH/2 - 50, 70, 105, false);
            ccX += 80;
        }

        int count = m_lastState.playerCount;
        int centerX = winW/2;
        int centerY = winH/2;
        int radiusX = winW/2 - 150;
        int radiusY = winH/2 - 150;
        for (int i = 0; i < count; ++i) {
            double angle = 2 * M_PI * i / count;
            int x = centerX + (int)(radiusX * cos(angle)) - 50;
            int y = centerY + (int)(radiusY * sin(angle)) - 50;
            bool isFolded = m_lastState.players[i].isFolded;
            SDL_Color nameColor = (i == m_lastState.currentPlayerId && m_waitingForAction && !m_gameOver && !isFolded) ?
                                  SDL_Color{0,255,0,255} : SDL_Color{255,255,255,255};
            std::string status = "";
            if (m_lastState.players[i].isFolded) status = " (Folded)";
            else if (m_lastState.players[i].isAllIn) status = " (All-In)";
            r.drawText(std::string(m_lastState.players[i].name) + " ($" + std::to_string(m_lastState.players[i].money) + ")" + status, x, y - 25, nameColor);
            r.drawText("Bet: $" + std::to_string(m_lastState.players[i].currentBet), x, y - 5, {255,255,0,255});
            bool showCards = (m_lastState.stage == 4) || (i == m_lastState.currentPlayerId && m_waitingForAction && !m_gameOver);
            if (m_lastState.players[i].isBot && m_lastState.stage != 4) showCards = false;
            if (isFolded) showCards = false;
            int cardX = x;
            for (int c = 0; c < m_lastState.players[i].cardCount; ++c) {
                Card card(static_cast<Suit>(m_lastState.players[i].cards[c][1]), static_cast<Rank>(m_lastState.players[i].cards[c][0]));
                r.drawCard(card, cardX, y + 15, 45, 65, !showCards);
                cardX += 50;
            }
        }
        if (m_waitingForAction && !m_gameOver && m_lastState.currentPlayerId == m_clientPlayerId) {
            int btnY = winH - 80;
            int callAmount = m_lastState.currentBet - m_lastState.players[m_clientPlayerId].currentBet;
            r.drawButton("Fold", winW/2 - 300, btnY, 80, 40);
            if (callAmount == 0) {
                r.drawButton("Check", winW/2 - 200, btnY, 120, 40);
            } else {
                r.drawButton("Call $" + std::to_string(callAmount), winW/2 - 200, btnY, 120, 40);
            }
            if (m_lastState.players[m_clientPlayerId].money > callAmount) {
                r.drawButton("Raise +20", winW/2 - 60, btnY, 90, 40);
                r.drawButton("Raise +50", winW/2 + 40, btnY, 90, 40);
                r.drawButton("Raise +100", winW/2 + 140, btnY, 100, 40);
                r.drawButton("All-in", winW/2 + 250, btnY, 80, 40);
            }
        } else if (m_gameOver) {
            if (strlen(m_lastState.winnerText) > 0) {
                r.drawText(m_lastState.winnerText, winW/2 - 150, winH/2 - 80, {100,255,100,255});
            }
            r.drawButton("Main Menu", winW/2 - 160, winH/2 + 100, 140, 40);
            r.drawButton("Next Game", winW/2 + 20, winH/2 + 100, 140, 40);
        }
        r.present();
        return;
    }

    // Серверная отрисовка (локальная)
    std::string stageText;
    switch (m_stage) {
        case Stage::Preflop: stageText = "PRE-FLOP"; break;
        case Stage::Flop: stageText = "FLOP"; break;
        case Stage::Turn: stageText = "TURN"; break;
        case Stage::River: stageText = "RIVER"; break;
        case Stage::Showdown: stageText = "SHOWDOWN"; break;
    }
    r.drawText(stageText, winW/2 - 40, 10, {255,200,100,255});
    r.drawText("POT: $" + std::to_string(m_pot), winW/2 - 50, 30, {255,215,0,255});

    int ccX = winW/2 - (int)m_communityCards.size() * 40;
    for (size_t i = 0; i < m_communityCards.size(); ++i) {
        r.drawCard(m_communityCards[i], ccX, winH/2 - 50, 70, 105, false);
        ccX += 80;
    }

    int count = (int)m_players.size();
    int centerX = winW/2;
    int centerY = winH/2;
    int radiusX = winW/2 - 150;
    int radiusY = winH/2 - 150;
    for (int i = 0; i < count; ++i) {
        double angle = 2 * M_PI * i / count;
        int x = centerX + (int)(radiusX * cos(angle)) - 50;
        int y = centerY + (int)(radiusY * sin(angle)) - 50;
        bool isFolded = m_players[i]->m_hasFolded || m_players[i]->getMoney() <= 0;
        SDL_Color nameColor = (i == m_currentPlayerIdx && m_waitingForAction && !m_gameOver && !isFolded) ?
                              SDL_Color{0,255,0,255} : SDL_Color{255,255,255,255};
        std::string status = "";
        if (m_players[i]->m_hasFolded) status = " (Folded)";
        else if (m_players[i]->getMoney() <= 0) status = " (Broke)";
        else if (m_players[i]->m_isAllIn) status = " (All-In)";
        r.drawText(m_players[i]->getName() + " ($" + std::to_string(m_players[i]->getMoney()) + ")" + status, x, y - 25, nameColor);
        r.drawText("Bet: $" + std::to_string(m_players[i]->m_currentBet), x, y - 5, {255,255,0,255});
        bool showCards = (m_stage == Stage::Showdown) || (i == m_currentPlayerIdx && m_waitingForAction && !m_gameOver);
        if (m_players[i]->isBot() && m_stage != Stage::Showdown) showCards = false;
        if (isFolded) showCards = false;
        int cardX = x;
        for (const auto& card : m_players[i]->getHand()) {
            r.drawCard(card, cardX, y + 15, 45, 65, !showCards);
            cardX += 50;
        }
    }

    if (m_waitingForAction && !m_gameOver) {
        Player* current = m_players[m_currentPlayerIdx];
        if (current->m_hasFolded || current->getMoney() <= 0) {
            current->m_hasActed = true;
            nextPlayer();
        } else if (current->isBot()) {
            SDL_Delay(400);
            int callAmount = m_currentBet - current->m_currentBet;
            if (callAmount == 0) {
                current->m_hasActed = true;
                nextPlayer();
            } else if (current->getMoney() >= callAmount) {
                current->placeBet(callAmount);
                m_pot += callAmount;
                current->m_hasActed = true;
                nextPlayer();
            } else {
                current->m_hasFolded = true;
                nextPlayer();
            }
        } else {
            int btnY = winH - 80;
            int callAmount = m_currentBet - current->m_currentBet;
            if (callAmount > current->getMoney()) {
                r.drawButton("Fold (Can't call)", winW/2 - 200, btnY, 200, 40);
            } else {
                r.drawButton("Fold", winW/2 - 300, btnY, 80, 40);
                r.drawButton(callAmount == 0 ? "Check" : "Call $" + std::to_string(callAmount), winW/2 - 200, btnY, 120, 40);
                if (current->getMoney() > callAmount) {
                    int raise20 = callAmount + 20;
                    int raise50 = callAmount + 50;
                    int raise100 = callAmount + 100;
                    if (raise20 <= current->getMoney() + current->m_currentBet)
                        r.drawButton("Raise +20", winW/2 - 60, btnY, 90, 40);
                    if (raise50 <= current->getMoney() + current->m_currentBet)
                        r.drawButton("Raise +50", winW/2 + 40, btnY, 90, 40);
                    if (raise100 <= current->getMoney() + current->m_currentBet)
                        r.drawButton("Raise +100", winW/2 + 140, btnY, 100, 40);
                    r.drawButton("All-in", winW/2 + 250, btnY, 80, 40);
                }
            }
        }
    } else if (m_gameOver) {
        if (!m_winnerText.empty()) {
            r.drawText(m_winnerText, winW/2 - 150, winH/2 - 80, {100,255,100,255});
        }
        r.drawButton("Main Menu", winW/2 - 160, winH/2 + 100, 140, 40);
        r.drawButton("Next Game", winW/2 + 20, winH/2 + 100, 140, 40);
    }
    r.present();
}

int LANPokerGame::handleEvent(const SDL_Event& event) {
    if (m_gameOver) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            auto& r = Renderer::getInstance();
            if (r.isButtonClicked(winW/2 - 160, winH/2 + 100, 140, 40)) return 1;
            if (r.isButtonClicked(winW/2 + 20, winH/2 + 100, 140, 40)) return 2;
        }
        return 0;
    }

    if (m_isServer) {
        // Сервер обрабатывает действия только для локального игрока (индекс 0)
        if (m_waitingForAction && m_currentPlayerIdx == 0 && m_currentPlayerIdx < (int)m_players.size()) {
            Player* current = m_players[m_currentPlayerIdx];
            if (!current->isBot() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int winW, winH;
                Renderer::getInstance().getWindowSize(winW, winH);
                int btnY = winH - 80;
                auto& r = Renderer::getInstance();
                int callAmount = m_currentBet - current->m_currentBet;
                if (callAmount > current->getMoney()) {
                    if (r.isButtonClicked(winW/2 - 200, btnY, 200, 40)) {
                        current->m_hasFolded = true;
                        nextPlayer();
                        sendFullStateToClient();
                    }
                } else {
                    if (r.isButtonClicked(winW/2 - 300, btnY, 80, 40)) {
                        current->m_hasFolded = true;
                        nextPlayer();
                        sendFullStateToClient();
                    } else if (r.isButtonClicked(winW/2 - 200, btnY, 120, 40)) {
                        int toCall = std::min(callAmount, current->getMoney());
                        if (toCall > 0) {
                            current->placeBet(toCall);
                            m_pot += toCall;
                        }
                        if (current->getMoney() == 0) current->m_isAllIn = true;
                        current->m_hasActed = true;
                        nextPlayer();
                        sendFullStateToClient();
                    } else if (current->getMoney() > callAmount) {
                        int raise20 = callAmount + 20;
                        int raise50 = callAmount + 50;
                        int raise100 = callAmount + 100;
                        if (r.isButtonClicked(winW/2 - 60, btnY, 90, 40) && raise20 <= current->getMoney() + current->m_currentBet) {
                            int totalNeeded = raise20;
                            current->placeBet(totalNeeded);
                            m_pot += totalNeeded;
                            m_currentBet = current->m_currentBet;
                            m_lastRaiserIdx = m_currentPlayerIdx;
                            for (auto p : m_players) {
                                if (p != current && !p->m_hasFolded && !p->m_isAllIn)
                                    p->m_hasActed = false;
                            }
                            if (current->getMoney() == 0) current->m_isAllIn = true;
                            current->m_hasActed = true;
                            nextPlayer();
                            sendFullStateToClient();
                        } else if (r.isButtonClicked(winW/2 + 40, btnY, 90, 40) && raise50 <= current->getMoney() + current->m_currentBet) {
                            int totalNeeded = raise50;
                            current->placeBet(totalNeeded);
                            m_pot += totalNeeded;
                            m_currentBet = current->m_currentBet;
                            m_lastRaiserIdx = m_currentPlayerIdx;
                            for (auto p : m_players) {
                                if (p != current && !p->m_hasFolded && !p->m_isAllIn)
                                    p->m_hasActed = false;
                            }
                            if (current->getMoney() == 0) current->m_isAllIn = true;
                            current->m_hasActed = true;
                            nextPlayer();
                            sendFullStateToClient();
                        } else if (r.isButtonClicked(winW/2 + 140, btnY, 100, 40) && raise100 <= current->getMoney() + current->m_currentBet) {
                            int totalNeeded = raise100;
                            current->placeBet(totalNeeded);
                            m_pot += totalNeeded;
                            m_currentBet = current->m_currentBet;
                            m_lastRaiserIdx = m_currentPlayerIdx;
                            for (auto p : m_players) {
                                if (p != current && !p->m_hasFolded && !p->m_isAllIn)
                                    p->m_hasActed = false;
                            }
                            if (current->getMoney() == 0) current->m_isAllIn = true;
                            current->m_hasActed = true;
                            nextPlayer();
                            sendFullStateToClient();
                        } else if (r.isButtonClicked(winW/2 + 250, btnY, 80, 40)) {
                            int totalNeeded = current->getMoney() + current->m_currentBet;
                            current->placeBet(totalNeeded);
                            m_pot += totalNeeded;
                            m_currentBet = current->m_currentBet;
                            m_lastRaiserIdx = m_currentPlayerIdx;
                            for (auto p : m_players) {
                                if (p != current && !p->m_hasFolded && !p->m_isAllIn)
                                    p->m_hasActed = false;
                            }
                            current->m_isAllIn = true;
                            current->m_hasActed = true;
                            nextPlayer();
                            sendFullStateToClient();
                        }
                    }
                }
            }
        }
    } else {
        // Клиент отправляет действия
        if (m_waitingForAction && m_lastState.currentPlayerId == m_clientPlayerId && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            int btnY = winH - 80;
            auto& r = Renderer::getInstance();
            int callAmount = m_lastState.currentBet - m_lastState.players[m_clientPlayerId].currentBet;
            if (r.isButtonClicked(winW/2 - 300, btnY, 80, 40)) {
                sendAction(PokerActionType::Fold);
            } else if (r.isButtonClicked(winW/2 - 200, btnY, 120, 40)) {
                if (callAmount == 0) sendAction(PokerActionType::Check);
                else sendAction(PokerActionType::Call);
            } else if (m_lastState.players[m_clientPlayerId].money > callAmount) {
                if (r.isButtonClicked(winW/2 - 60, btnY, 90, 40)) {
                    sendAction(PokerActionType::Raise, callAmount + 20);
                } else if (r.isButtonClicked(winW/2 + 40, btnY, 90, 40)) {
                    sendAction(PokerActionType::Raise, callAmount + 50);
                } else if (r.isButtonClicked(winW/2 + 140, btnY, 100, 40)) {
                    sendAction(PokerActionType::Raise, callAmount + 100);
                } else if (r.isButtonClicked(winW/2 + 250, btnY, 80, 40)) {
                    sendAction(PokerActionType::AllIn);
                }
            }
        }
    }
    return 0;
}

void LANPokerGame::sendAction(PokerActionType action, uint32_t raiseAmount) {
    PokerActionPacket packet;
    packet.type = PacketType::PokerAction;
    packet.playerId = m_clientPlayerId;
    packet.action = action;
    packet.raiseAmount = raiseAmount;
    NetworkManager::getInstance().sendToServer(&packet, sizeof(packet));
    std::cout << "[POKER CLIENT] Sent action: " << (int)action << std::endl;
}

void LANPokerGame::onPacketReceived(const void* data, int len) {
    if (len < 1) return;
    PacketType type = *(PacketType*)data;
    if (type == PacketType::PokerState && len >= sizeof(PokerStatePacket)) {
        memcpy(&m_lastState, data, sizeof(PokerStatePacket));
        m_gameOver = m_lastState.gameOver;
        if (!m_isServer) {
            m_waitingForAction = !m_gameOver && (m_lastState.currentPlayerId == m_clientPlayerId);
            std::cout << "[POKER CLIENT] Received state, myTurn=" << m_waitingForAction << std::endl;
        }
    } else if (type == PacketType::PokerAction && m_isServer && len >= sizeof(PokerActionPacket)) {
        PokerActionPacket action;
        memcpy(&action, data, sizeof(PokerActionPacket));
        if (m_waitingForAction && action.playerId == m_currentPlayerIdx) {
            Player* current = m_players[action.playerId];
            int callAmount = m_currentBet - current->m_currentBet;
            switch (action.action) {
                case PokerActionType::Fold:
                    current->m_hasFolded = true;
                    nextPlayer();
                    break;
                case PokerActionType::Check:
                    if (callAmount == 0) {
                        current->m_hasActed = true;
                        nextPlayer();
                    }
                    break;
                case PokerActionType::Call:
                    if (callAmount <= current->getMoney()) {
                        current->placeBet(callAmount);
                        m_pot += callAmount;
                        if (current->getMoney() == 0) current->m_isAllIn = true;
                        current->m_hasActed = true;
                        nextPlayer();
                    }
                    break;
                case PokerActionType::Raise:
                    {
                        int totalNeeded = action.raiseAmount;
                        if (totalNeeded <= current->getMoney() + current->m_currentBet) {
                            current->placeBet(totalNeeded);
                            m_pot += totalNeeded;
                            m_currentBet = current->m_currentBet;
                            m_lastRaiserIdx = action.playerId;
                            for (auto p : m_players) {
                                if (p != current && !p->m_hasFolded && !p->m_isAllIn)
                                    p->m_hasActed = false;
                            }
                            if (current->getMoney() == 0) current->m_isAllIn = true;
                            current->m_hasActed = true;
                            nextPlayer();
                        }
                    }
                    break;
                case PokerActionType::AllIn:
                    {
                        int totalNeeded = current->getMoney() + current->m_currentBet;
                        current->placeBet(totalNeeded);
                        m_pot += totalNeeded;
                        m_currentBet = current->m_currentBet;
                        m_lastRaiserIdx = action.playerId;
                        for (auto p : m_players) {
                            if (p != current && !p->m_hasFolded && !p->m_isAllIn)
                                p->m_hasActed = false;
                        }
                        current->m_isAllIn = true;
                        current->m_hasActed = true;
                        nextPlayer();
                    }
                    break;
            }
            sendFullStateToClient();
        }
    }
}

void LANPokerGame::sendFullStateToClient() {
    if (!m_isServer) return;
    PokerStatePacket packet;
    packet.type = PacketType::PokerState;
    packet.currentPlayerId = m_currentPlayerIdx;
    packet.pot = m_pot;
    packet.currentBet = m_currentBet;
    packet.stage = static_cast<uint8_t>(m_stage);
    packet.communityCardCount = static_cast<uint8_t>(m_communityCards.size());
    for (size_t i = 0; i < m_communityCards.size(); ++i) {
        packet.communityCards[i][0] = static_cast<uint8_t>(m_communityCards[i].getRank());
        packet.communityCards[i][1] = static_cast<uint8_t>(m_communityCards[i].getSuit());
    }
    packet.playerCount = static_cast<uint8_t>(m_players.size());
    for (size_t i = 0; i < m_players.size(); ++i) {
        packet.players[i].playerId = static_cast<uint32_t>(i);
        strncpy(packet.players[i].name, m_players[i]->getName().c_str(), 19);
        packet.players[i].name[19] = '\0';
        packet.players[i].money = m_players[i]->getMoney();
        packet.players[i].currentBet = m_players[i]->m_currentBet;
        packet.players[i].cardCount = static_cast<uint8_t>(m_players[i]->getHand().size());
        for (size_t c = 0; c < m_players[i]->getHand().size(); ++c) {
            packet.players[i].cards[c][0] = static_cast<uint8_t>(m_players[i]->getHand()[c].getRank());
            packet.players[i].cards[c][1] = static_cast<uint8_t>(m_players[i]->getHand()[c].getSuit());
        }
        packet.players[i].isFolded = m_players[i]->m_hasFolded;
        packet.players[i].isAllIn = m_players[i]->m_isAllIn;
        packet.players[i].hasActed = m_players[i]->m_hasActed;
        packet.players[i].isBot = m_players[i]->isBot();
    }
    packet.gameOver = m_gameOver;
    strncpy(packet.winnerText, m_winnerText.c_str(), 99);
    packet.winnerText[99] = '\0';
    NetworkManager::getInstance().sendToClient(&packet, sizeof(packet));
    std::cout << "[POKER SERVER] Sent state to client" << std::endl;
}