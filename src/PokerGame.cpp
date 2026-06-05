#include "PokerGame.h"
#include "Renderer.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PokerGame::PokerGame(const std::vector<Player*>& players)
    : Game(players), m_stage(Stage::Preflop), m_smallBlind(10), m_bigBlind(20),
      m_currentBet(0), m_pot(0), m_dealerButton(0), m_currentPlayerIdx(0),
      m_lastRaiserIdx(-1), m_playersInHand(0), m_waitingForAction(false) {}

PokerGame::~PokerGame() {}

void PokerGame::run() {
    startNewRound();
}

void PokerGame::startNewRound() {
    m_gameOver = false;
    m_deck.reset();
    m_stage = Stage::Preflop;
    m_communityCards.clear();
    m_pot = 0;
    m_currentBet = 0;
    m_lastRaiserIdx = -1;

    for (auto p : m_players) {
        p->clearHand();
        p->m_hasFolded = false;
        p->m_isAllIn = false;
        p->m_hasActed = false;
        p->m_currentBet = 0;
    }

    for (int i = 0; i < 2; ++i)
        for (auto p : m_players)
            if (p->getMoney() > 0)
                p->addCard(m_deck.dealCard());

    m_dealerButton = (m_dealerButton + 1) % m_players.size();
    int sbIdx = (m_dealerButton + 1) % m_players.size();
    int bbIdx = (sbIdx + 1) % m_players.size();

    if (m_players[sbIdx]->getMoney() >= m_smallBlind) {
        m_players[sbIdx]->placeBet(m_smallBlind);
        m_pot += m_smallBlind;
        m_players[sbIdx]->m_currentBet = m_smallBlind;
    }
    if (m_players[bbIdx]->getMoney() >= m_bigBlind) {
        m_players[bbIdx]->placeBet(m_bigBlind);
        m_pot += m_bigBlind;
        m_players[bbIdx]->m_currentBet = m_bigBlind;
    }

    m_currentBet = m_bigBlind;
    m_currentPlayerIdx = (bbIdx + 1) % m_players.size();
    m_playersInHand = 0;
    for (auto p : m_players) if (!p->m_hasFolded && p->getMoney() > 0) m_playersInHand++;
    m_waitingForAction = true;
    m_lastRaiserIdx = bbIdx;
}

void PokerGame::advanceStage() {
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
            return;
        default: break;
    }
    m_currentBet = 0;
    m_lastRaiserIdx = -1;
    for (auto p : m_players) {
        if (!p->m_hasFolded) p->m_hasActed = false;
        p->m_currentBet = 0;
    }
    m_currentPlayerIdx = (m_dealerButton + 1) % m_players.size();
    while (m_players[m_currentPlayerIdx]->m_hasFolded)
        m_currentPlayerIdx = (m_currentPlayerIdx + 1) % m_players.size();
    m_waitingForAction = true;
}

void PokerGame::nextPlayer() {
    int start = m_currentPlayerIdx;
    do {
        m_currentPlayerIdx = (m_currentPlayerIdx + 1) % m_players.size();
    } while (m_players[m_currentPlayerIdx]->m_hasFolded && m_currentPlayerIdx != start);

    if (isRoundComplete()) advanceStage();
}

bool PokerGame::isRoundComplete() {
    int active = 0, acted = 0;
    for (auto p : m_players) {
        if (!p->m_hasFolded && p->getMoney() > 0) {
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
    return (acted >= active || (allBetsEqual && m_lastRaiserIdx == -1)) && active > 0;
}

void PokerGame::evaluateAndPayWinners() {
    for (auto p : m_players) {
        if (!p->m_hasFolded) {
            p->winMoney(m_pot);
            break;
        }
    }
    m_pot = 0;
}

void PokerGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();

    int winW, winH;
    r.getWindowSize(winW, winH);

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

        SDL_Color nameColor = (i == m_currentPlayerIdx && m_waitingForAction && !m_gameOver) ? SDL_Color{0,255,0,255} : SDL_Color{255,255,255,255};
        std::string status = "";
        if (m_players[i]->m_hasFolded) status = " (Folded)";
        else if (m_players[i]->m_isAllIn) status = " (All-In)";

        r.drawText(m_players[i]->getName() + " ($" + std::to_string(m_players[i]->getMoney()) + ")" + status, x, y - 25, nameColor);
        r.drawText("Bet: $" + std::to_string(m_players[i]->m_currentBet), x, y - 5, {255,255,0,255});

        int cardX = x;
        for (const auto& card : m_players[i]->getHand()) {
            bool hidden = (m_players[i]->isBot() && m_stage != Stage::Showdown);
            r.drawCard(card, cardX, y + 15, 45, 65, hidden);
            cardX += 50;
        }
    }

    if (m_waitingForAction && !m_gameOver) {
        Player* current = m_players[m_currentPlayerIdx];
        if (current->isBot()) {
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

            r.drawButton("Fold", winW/2 - 220, btnY, 100, 40);
            r.drawButton(callAmount == 0 ? "Check" : "Call $" + std::to_string(callAmount), winW/2 - 100, btnY, 120, 40);

            if (current->getMoney() > callAmount) {
                r.drawButton("Raise +$" + std::to_string(m_bigBlind), winW/2 + 40, btnY, 150, 40);
            }
        }
    } else if (m_gameOver) {
        r.drawButton("Main Menu", winW/2 - 80, winH/2 + 100, 160, 40);
    }

    r.present();
}

bool PokerGame::handleEvent(const SDL_Event& event) {
    if (m_gameOver) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            if (Renderer::getInstance().isButtonClicked(winW/2 - 80, winH/2 + 100, 160, 40)) {
                return true;
            }
        }
        return false;
    }

    if (m_waitingForAction) {
        Player* current = m_players[m_currentPlayerIdx];
        if (!current->isBot() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            int btnY = winH - 80;
            auto& r = Renderer::getInstance();
            int callAmount = m_currentBet - current->m_currentBet;

            if (r.isButtonClicked(winW/2 - 220, btnY, 100, 40)) {
                current->m_hasFolded = true;
                nextPlayer();
            } else if (r.isButtonClicked(winW/2 - 100, btnY, 120, 40)) {
                int toCall = std::min(callAmount, current->getMoney());
                if (toCall > 0) {
                    current->placeBet(toCall);
                    m_pot += toCall;
                }
                if (current->getMoney() == 0) current->m_isAllIn = true;
                current->m_hasActed = true;
                nextPlayer();
            } else if (current->getMoney() > callAmount && r.isButtonClicked(winW/2 + 40, btnY, 150, 40)) {
                int raiseAmount = m_bigBlind;
                int totalNeeded = callAmount + raiseAmount;
                if (totalNeeded > current->getMoney()) totalNeeded = current->getMoney();

                current->placeBet(totalNeeded);
                m_pot += totalNeeded;
                m_currentBet = current->m_currentBet;
                m_lastRaiserIdx = m_currentPlayerIdx;

                for (auto p : m_players) {
                    if (p != current && !p->m_hasFolded && !p->m_isAllIn) {
                        p->m_hasActed = false;
                    }
                }
                if (current->getMoney() == 0) current->m_isAllIn = true;
                current->m_hasActed = true;
                nextPlayer();
            }
        }
    }
    return false;
}