#include "BlackjackGame.h"
#include "Renderer.h"
#include <iostream>

BlackjackGame::BlackjackGame(const std::vector<Player*>& players) : Game(players),
    m_dealer(nullptr), m_currentPlayerIndex(0), m_waitingForAction(false) {
    m_dealer = new BotPlayer("Dealer");
    m_gameOver = false;
}
BlackjackGame::~BlackjackGame() { delete m_dealer; }

void BlackjackGame::run() {
    m_deck.reset();
    m_gameOver = false;
    m_waitingForAction = true;
    m_currentPlayerIndex = 0;
    for (auto p : m_players) p->clearHand();
    m_dealer->clearHand();
    for (int i = 0; i < 2; ++i) {
        for (auto p : m_players) p->addCard(m_deck.dealCard());
        m_dealer->addCard(m_deck.dealCard());
    }
}

void BlackjackGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();

    r.drawText("Dealer: " + std::to_string(m_dealer->getHandValue()), 300, 50, {255,255,255,255});
    int dx = 200;
    for (const auto& card : m_dealer->getHand()) {
        r.drawCard(card, dx, 100, 60, 90);
        dx += 70;
    }

    int yStart = 300;
    int stepY = 120;
    int maxPlayers = (int)m_players.size();
    if (maxPlayers > 6) stepY = 90;

    for (int i = 0; i < maxPlayers; ++i) {
        if (i == m_currentPlayerIndex && m_waitingForAction) {
            r.drawText("-->", 50, yStart + i*stepY, {255,255,0,255});
        }
        r.drawText(m_players[i]->getName() + ": " + std::to_string(m_players[i]->getHandValue()),
                   100, yStart + i*stepY, {255,255,255,255});
        int cx = 200;
        for (const auto& card : m_players[i]->getHand()) {
            r.drawCard(card, cx, yStart + i*stepY, 60, 90);
            cx += 70;
        }
    }

    if (m_waitingForAction && m_currentPlayerIndex < maxPlayers) {
        r.drawButton("Hit", 100, 550, 100, 50, false);
        r.drawButton("Stand", 250, 550, 100, 50, false);
        r.drawButton("Double", 400, 550, 100, 50, false);
    }
    r.present();
}

bool BlackjackGame::handleEvent(const SDL_Event& event) {
    if (m_gameOver) return false;
    if (!m_waitingForAction) return false;
    if (m_currentPlayerIndex >= (int)m_players.size()) {
        dealerTurn();
        return false;
    }
    Player* current = m_players[m_currentPlayerIndex];
    if (current->isBot()) {
        std::string action = current->getAction({"hit", "stand"});
        if (action == "hit") {
            current->addCard(m_deck.dealCard());
            if (current->getHandValue() > 21) nextPlayer();
        } else if (action == "stand") {
            nextPlayer();
        }
    } else {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            auto& r = Renderer::getInstance();
            if (r.isButtonClicked("Hit", 100, 550, 100, 50)) {
                current->addCard(m_deck.dealCard());
                if (current->getHandValue() > 21) nextPlayer();
            } else if (r.isButtonClicked("Stand", 250, 550, 100, 50)) {
                nextPlayer();
            } else if (r.isButtonClicked("Double", 400, 550, 100, 50)) {
                int bet = std::min(current->getMoney() / 2, 100);
                if (bet < 10) bet = 10;
                if (current->getMoney() >= bet) {
                    current->placeBet(bet);
                    current->addCard(m_deck.dealCard());
                    nextPlayer();
                }
            }
        }
    }
    return false;
}

void BlackjackGame::nextPlayer() {
    m_currentPlayerIndex++;
    if (m_currentPlayerIndex >= (int)m_players.size()) {
        m_waitingForAction = false;
        dealerTurn();
    }
}

void BlackjackGame::dealerTurn() {
    while (m_dealer->getHandValue() < 17) {
        m_dealer->addCard(m_deck.dealCard());
    }
    resolveBets();
    m_gameOver = true;
}

void BlackjackGame::resolveBets() {
    int dealerVal = m_dealer->getHandValue();
    for (auto p : m_players) {
        int pVal = p->getHandValue();
        if (pVal > 21) {
            // проигрыш
        } else if (dealerVal > 21 || pVal > dealerVal) {
            p->winMoney(10);
        } else if (pVal == dealerVal) {
            p->winMoney(0);
        }
    }
}