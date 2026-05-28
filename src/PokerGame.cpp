#include "PokerGame.h"
#include "Renderer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

PokerGame::PokerGame(const std::vector<Player*>& players) : Game(players),
    m_state(State::Preflop), m_currentBet(0), m_pot(0), m_currentPlayerIndex(0), m_waitingForAction(false)
{
    m_gameOver = false;
}

void PokerGame::run() {
    m_deck.reset();
    m_communityCards.clear();
    m_pot = 0;
    m_currentBet = 0;
    m_state = State::Preflop;
    m_currentPlayerIndex = 0;
    m_gameOver = false;
    m_waitingForAction = true;

    for (auto p : m_players) p->clearHand();
    m_playerBets.assign(m_players.size(), 0);

    dealHoleCards();
}

int PokerGame::getMinBet() const {
    return 10;
}

int PokerGame::getPlayerBetAmount(Player* p) {
    int maxBet = p->getMoney();
    int minBet = getMinBet();
    if (p->isBot()) {
        if (maxBet < minBet) return 0;
        int bet = minBet + (rand() % std::max(1, maxBet / 20));
        return std::min(bet, maxBet);
    } else {
        std::cout << p->getName() << ", your balance: $" << maxBet << "\n";
        std::cout << "Enter bet amount (0 to fold, " << minBet << ".." << maxBet << "): ";
        int bet;
        std::cin >> bet;
        if (bet == 0) return 0;
        if (bet < minBet) bet = minBet;
        if (bet > maxBet) bet = maxBet;
        return bet;
    }
}

void PokerGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();

    // Общие карты (центр)
    int startX = 400 - (int)(m_communityCards.size() * 70 / 2);
    for (size_t i = 0; i < m_communityCards.size(); ++i) {
        r.drawCard(m_communityCards[i], startX + i * 100, 300, 80, 120);
    }

    // Размещение игроков по кругу (радиус 250, центр стола ~ 512, 350)
    int centerX = 512, centerY = 350;
    int radius = 280;
    int count = (int)m_players.size();
    for (int i = 0; i < count; ++i) {
        double angle = 2 * M_PI * i / count - M_PI_2; // начиная сверху
        int x = centerX + (int)(radius * cos(angle));
        int y = centerY + (int)(radius * sin(angle));
        // Корректировка, чтобы не вылезало за края
        if (x < 50) x = 50;
        if (x > 974 - 120) x = 974 - 120;
        if (y < 50) y = 50;
        if (y > 718 - 90) y = 718 - 90;

        bool isActive = (m_playerBets[i] != -1);
        std::string status = isActive ? "" : " (FOLD)";
        // Имя и деньги
        r.drawText(m_players[i]->getName() + ": $" + std::to_string(m_players[i]->getMoney()) + status,
                   x, y - 20, {255,255,255,255});
        // Карты игрока (всегда видны)
        int cx = x;
        int cy = y;
        for (size_t cardIdx = 0; cardIdx < m_players[i]->getHand().size(); ++cardIdx) {
            r.drawCard(m_players[i]->getHand()[cardIdx], cx + cardIdx * 70, cy, 60, 90);
        }
        // Указатель текущего игрока
        if (i == m_currentPlayerIndex && m_waitingForAction && isActive) {
            r.drawText("-->", x - 30, y, {255,255,0,255});
        }
    }

    r.drawText("Pot: $" + std::to_string(m_pot), 20, 20, {255,255,0,255});
    r.drawText("State: " + std::to_string((int)m_state), 20, 50, {200,200,200,255});

    // Кнопки для человека (только если ход человека и он не сбросил)
    if (m_waitingForAction && m_currentPlayerIndex < (int)m_players.size()) {
        Player* current = m_players[m_currentPlayerIndex];
        if (!current->isBot() && m_playerBets[m_currentPlayerIndex] != -1) {
            r.drawButton("Fold", 100, 650, 100, 50, false);
            r.drawButton("Check/Call", 250, 650, 100, 50, false);
            r.drawButton("Bet", 400, 650, 100, 50, false);
        }
    }

    r.present();
}

bool PokerGame::handleEvent(const SDL_Event& event) {
    if (m_gameOver) return false;
    if (!m_waitingForAction) return false;
    if (m_currentPlayerIndex >= (int)m_players.size()) {
        endRound();
        return false;
    }

    Player* current = m_players[m_currentPlayerIndex];
    if (m_playerBets[m_currentPlayerIndex] == -1) {
        nextPlayer();
        return false;
    }

    if (current->isBot()) {
        int toCall = m_currentBet - m_playerBets[m_currentPlayerIndex];
        std::string action;
        if (toCall == 0) action = "check";
        else if (current->getMoney() >= toCall) action = "call";
        else action = "fold";
        processAction(current, action);
    } else {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            auto& r = Renderer::getInstance();
            if (r.isButtonClicked("Fold", 100, 650, 100, 50)) {
                processAction(current, "fold");
            } else if (r.isButtonClicked("Check/Call", 250, 650, 100, 50)) {
                int toCall = m_currentBet - m_playerBets[m_currentPlayerIndex];
                if (toCall == 0) processAction(current, "check");
                else processAction(current, "call");
            } else if (r.isButtonClicked("Bet", 400, 650, 100, 50)) {
                processAction(current, "bet");
            }
        }
    }
    return false;
}

void PokerGame::processAction(Player* player, const std::string& action) {
    int toCall = m_currentBet - m_playerBets[m_currentPlayerIndex];
    if (action == "fold") {
        m_playerBets[m_currentPlayerIndex] = -1;
        nextPlayer();
    }
    else if (action == "check" && toCall == 0) {
        nextPlayer();
    }
    else if (action == "call" && toCall > 0 && player->getMoney() >= toCall) {
        player->placeBet(toCall);
        m_pot += toCall;
        m_playerBets[m_currentPlayerIndex] = m_currentBet;
        nextPlayer();
    }
    else if (action == "bet") {
        int bet = getPlayerBetAmount(player);
        if (bet == 0) {
            m_playerBets[m_currentPlayerIndex] = -1;
            nextPlayer();
            return;
        }
        if (player->getMoney() >= bet) {
            player->placeBet(bet);
            m_pot += bet;
            m_playerBets[m_currentPlayerIndex] += bet;
            m_currentBet = m_playerBets[m_currentPlayerIndex];
            m_currentPlayerIndex = 0;
            while (m_currentPlayerIndex < (int)m_players.size() && m_playerBets[m_currentPlayerIndex] == -1)
                m_currentPlayerIndex++;
        } else {
            m_playerBets[m_currentPlayerIndex] = -1;
        }
        nextPlayer();
    }
}

void PokerGame::nextPlayer() {
    do {
        m_currentPlayerIndex++;
    } while (m_currentPlayerIndex < (int)m_players.size() && m_playerBets[m_currentPlayerIndex] == -1);

    if (m_currentPlayerIndex >= (int)m_players.size() || allPlayersActed()) {
        endRound();
    }
}

bool PokerGame::allPlayersActed() const {
    int active = 0, acted = 0;
    for (size_t i = 0; i < m_players.size(); ++i) {
        if (m_playerBets[i] != -1) {
            active++;
            if (m_playerBets[i] == m_currentBet) acted++;
        }
    }
    return (active == acted) && active > 0;
}

void PokerGame::endRound() {
    m_waitingForAction = false;
    switch (m_state) {
        case State::Preflop:
            dealCommunity(3);
            m_state = State::Flop;
            break;
        case State::Flop:
            dealCommunity(1);
            m_state = State::Turn;
            break;
        case State::Turn:
            dealCommunity(1);
            m_state = State::River;
            break;
        case State::River:
            m_state = State::Showdown;
            evaluateWinner();
            m_gameOver = true;
            return;
        default: break;
    }
    resetRoundBets();
    m_waitingForAction = true;
}

void PokerGame::resetRoundBets() {
    m_currentBet = 0;
    m_currentPlayerIndex = 0;
    for (size_t i = 0; i < m_players.size(); ++i) {
        if (m_playerBets[i] != -1) m_playerBets[i] = 0;
    }
    while (m_currentPlayerIndex < (int)m_players.size() && m_playerBets[m_currentPlayerIndex] == -1)
        m_currentPlayerIndex++;
}

void PokerGame::dealHoleCards() {
    for (auto p : m_players) {
        p->addCard(m_deck.dealCard());
        p->addCard(m_deck.dealCard());
    }
}

void PokerGame::dealCommunity(int count) {
    for (int i = 0; i < count; ++i)
        m_communityCards.push_back(m_deck.dealCard());
}

void PokerGame::evaluateWinner() {
    for (size_t i = 0; i < m_players.size(); ++i) {
        if (m_playerBets[i] != -1) {
            m_players[i]->winMoney(m_pot);
            break;
        }
    }
}