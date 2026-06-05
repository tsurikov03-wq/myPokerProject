#include "BlackjackGame.h"
#include "Renderer.h"
#include <algorithm>
#include <cmath>

BlackjackGame::BlackjackGame(const std::vector<Player*>& players)
    : Game(players), m_dealer(nullptr), m_currentPlayerIndex(0), m_waitingForAction(false), m_botTimer(0) {
    m_dealer = new BotPlayer("Dealer", 999999);
    m_playerBets.resize(players.size(), 0);
    m_playerDone.resize(players.size(), false);
    m_playerResult.resize(players.size(), "");
    m_playerResultColor.resize(players.size(), SDL_Color{200,200,200,255});
}

BlackjackGame::~BlackjackGame() {
    delete m_dealer;
}

void BlackjackGame::askBet(Player* p, int idx) {
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

void BlackjackGame::run() {
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
}

void BlackjackGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();

    int winW, winH;
    r.getWindowSize(winW, winH);

    std::string dealerScore = m_waitingForAction ? "?" : std::to_string(m_dealer->getHandValue());
    r.drawText("DEALER: " + dealerScore, winW / 2 - 60, 20, {255, 215, 0, 255});

    int dx = winW / 2 - (int)m_dealer->getHand().size() * 40;
    for (size_t i = 0; i < m_dealer->getHand().size(); ++i) {
        bool hidden = (i == 1 && m_waitingForAction);
        r.drawCard(m_dealer->getHand()[i], dx, 60, 70, 105, hidden);
        dx += 80;
    }

    int count = (int)m_players.size();
    int radiusX = winW / 2 - 120;
    int radiusY = winH / 3;
    int centerX = winW / 2;


    int centerY;
    if (count >= 5) {
        centerY = winH - 220;
    } else {
        centerY = winH - 280;
    }

    for (int i = 0; i < count; ++i) {
        if (m_playerBets[i] == 0 && m_playerDone[i]) continue;

        double angle = M_PI;
        if (count > 1) angle = M_PI - (M_PI * i / (count - 1));
        else angle = M_PI / 2;

        int x = centerX + (int)(radiusX * cos(angle)) - 35;
        int y = centerY - (int)(radiusY * sin(angle));

        SDL_Color nameColor = (i == m_currentPlayerIndex && m_waitingForAction) ?
                              SDL_Color{0, 255, 0, 255} : SDL_Color{255, 255, 255, 255};

        std::string status = "";
        if (m_players[i]->isBust()) status = " (Bust!)";
        else if (m_playerDone[i]) status = " (Done)";

        r.drawText(m_players[i]->getName() + status, x - 20, y - 50, nameColor);
        r.drawText("Money: $" + std::to_string(m_players[i]->getMoney()), x - 20, y - 30, {200, 200, 200, 255});
        r.drawText("Bet: $" + std::to_string(m_playerBets[i]), x - 20, y + 120, {255, 215, 0, 255});
        r.drawText("Score: " + std::to_string(m_players[i]->getHandValue()), x - 20, y + 140, {255, 255, 255, 255});

        if (!m_playerResult[i].empty()) {
            r.drawText(m_playerResult[i], x - 20, y + 160, m_playerResultColor[i]);
        }

        for (size_t cardIdx = 0; cardIdx < m_players[i]->getHand().size(); ++cardIdx) {
            r.drawCard(m_players[i]->getHand()[cardIdx], x + cardIdx * 25, y, 70, 105, false);
        }
    }

    if (m_waitingForAction && m_currentPlayerIndex < count) {
        Player* current = m_players[m_currentPlayerIndex];
        if (current->isBot()) {
            uint64_t currentTicks = SDL_GetTicks();
            if (m_botTimer == 0) m_botTimer = currentTicks;
            if (currentTicks - m_botTimer >= 600) {
                std::string action = current->getAction({"hit", "stand"});
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
            }
        } else {
            int btnY = winH - 70;
            r.drawButton("Hit", winW / 2 - 220, btnY, 100, 50);
            r.drawButton("Stand", winW / 2 - 110, btnY, 100, 50);
            if (current->getHand().size() == 2 && current->canAfford(m_playerBets[m_currentPlayerIndex])) {
                r.drawButton("Double", winW / 2, btnY, 100, 50);
            }
        }
    } else if (m_gameOver) {
        r.drawButton("Main Menu", winW / 2 - 160, winH / 2 + 120, 140, 40);
        r.drawButton("Next Game", winW / 2 + 20, winH / 2 + 120, 140, 40);
    }

    r.present();
}

int BlackjackGame::handleEvent(const SDL_Event& event) {
    if (m_gameOver) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            auto& r = Renderer::getInstance();
            if (r.isButtonClicked(winW / 2 - 160, winH / 2 + 120, 140, 40)) return 1;
            if (r.isButtonClicked(winW / 2 + 20, winH / 2 + 120, 140, 40)) return 2;
        }
        return 0;
    }

    if (m_waitingForAction && m_currentPlayerIndex < (int)m_players.size()) {
        Player* current = m_players[m_currentPlayerIndex];
        if (!current->isBot() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            auto& r = Renderer::getInstance();
            int btnY = winH - 70;

            if (r.isButtonClicked(winW / 2 - 220, btnY, 100, 50)) {
                current->addCard(m_deck.dealCard());
                if (current->isBust()) {
                    m_playerDone[m_currentPlayerIndex] = true;
                    nextPlayer();
                }
            } else if (r.isButtonClicked(winW / 2 - 110, btnY, 100, 50)) {
                m_playerDone[m_currentPlayerIndex] = true;
                nextPlayer();
            } else if (current->getHand().size() == 2 && current->canAfford(m_playerBets[m_currentPlayerIndex]) &&
                       r.isButtonClicked(winW / 2, btnY, 100, 50)) {
                int doubleBet = m_playerBets[m_currentPlayerIndex];
                current->placeBet(doubleBet);
                m_playerBets[m_currentPlayerIndex] += doubleBet;
                current->addCard(m_deck.dealCard());
                m_playerDone[m_currentPlayerIndex] = true;
                nextPlayer();
            }
        }
    }
    return 0;
}

void BlackjackGame::nextPlayer() {
    m_currentPlayerIndex++;
    m_botTimer = 0;
    while (m_currentPlayerIndex < (int)m_players.size() &&
           (m_playerBets[m_currentPlayerIndex] == 0 || m_playerDone[m_currentPlayerIndex])) {
        m_currentPlayerIndex++;
    }
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
    bool dealerBust = dealerVal > 21;

    for (size_t i = 0; i < m_players.size(); ++i) {
        if (m_playerBets[i] == 0) continue;

        Player* p = m_players[i];
        int pVal = p->getHandValue();

        if (pVal > 21) {
            m_playerResult[i] = "Lose!";
            m_playerResultColor[i] = {255, 80, 80, 255};
            continue;
        }

        if (dealerBust || pVal > dealerVal) {
            p->winMoney(m_playerBets[i] * 2);
            m_playerResult[i] = "Win! +$" + std::to_string(m_playerBets[i]);
            m_playerResultColor[i] = {80, 255, 80, 255};
        } else if (pVal == dealerVal) {
            p->winMoney(m_playerBets[i]);
            m_playerResult[i] = "Push";
            m_playerResultColor[i] = {255, 255, 100, 255};
        } else {
            m_playerResult[i] = "Lose!";
            m_playerResultColor[i] = {255, 80, 80, 255};
        }
    }
}