#include "PokerGame.h"
#include "Renderer.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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

PokerGame::PokerGame(const std::vector<Player*>& players)
    : Game(players), m_stage(Stage::Preflop), m_smallBlind(10), m_bigBlind(20),
      m_currentBet(0), m_pot(0), m_dealerButton(0), m_currentPlayerIdx(0),
      m_lastRaiserIdx(-1), m_playersInHand(0), m_waitingForAction(false), m_winnerText("") {}

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
    m_winnerText = "";

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
    for (auto p : m_players) if (!p->m_hasFolded) m_playersInHand++;
    m_waitingForAction = true;
    m_lastRaiserIdx = bbIdx;
}

void PokerGame::advanceStage() {

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
        m_winnerText = lastPlayer->getName() + " wins the pot (everyone folded)!";
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
}

void PokerGame::nextPlayer() {
    int start = m_currentPlayerIdx;
    do {
        m_currentPlayerIdx = (m_currentPlayerIdx + 1) % m_players.size();
    } while (m_players[m_currentPlayerIdx]->m_hasFolded && m_currentPlayerIdx != start);

    if (isRoundComplete()) {
        advanceStage();
    }
}

bool PokerGame::isRoundComplete() {

    int active = 0;
    int acted = 0;
    for (auto p : m_players) {
        if (!p->m_hasFolded) {
            active++;
            if (p->m_hasActed || p->m_isAllIn) acted++;
        }
    }

    int targetBet = -1;
    bool allBetsEqual = true;
    for (auto p : m_players) {
        if (!p->m_hasFolded && !p->m_isAllIn) {
            if (targetBet == -1) targetBet = p->m_currentBet;
            else if (p->m_currentBet != targetBet) allBetsEqual = false;
        }
    }

    bool complete = (acted >= active && allBetsEqual) || (m_lastRaiserIdx == -1 && allBetsEqual);
    return complete && active > 0;
}

void PokerGame::evaluateAndPayWinners() {
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

void PokerGame::render() {
    auto& r = Renderer::getInstance();
    r.clear();
    r.drawTable();

    int winW, winH;
    r.getWindowSize(winW, winH);

    std::string stageText;
    switch (m_stage) {
        case Stage::Preflop: stageText = "PRE-FLOP"; break;
        case Stage::Flop: stageText = "FLOP"; break;
        case Stage::Turn: stageText = "TURN"; break;
        case Stage::River: stageText = "RIVER"; break;
        case Stage::Showdown: stageText = "SHOWDOWN"; break;
    }
    r.drawText(stageText, winW/2 - 40, 10, {255, 200, 100, 255});

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

        bool showCards = (m_stage == Stage::Showdown) || (i == m_currentPlayerIdx && m_waitingForAction && !m_gameOver);
        if (m_players[i]->isBot() && m_stage != Stage::Showdown) showCards = false;

        int cardX = x;
        for (const auto& card : m_players[i]->getHand()) {
            r.drawCard(card, cardX, y + 15, 45, 65, !showCards);
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
    } else if (m_gameOver) {
        if (!m_winnerText.empty()) {
            r.drawText(m_winnerText, winW/2 - 150, winH/2 + 40, {100, 255, 100, 255});
        }
        r.drawButton("Main Menu", winW/2 - 160, winH/2 + 100, 140, 40);
        r.drawButton("Next Game", winW/2 + 20, winH/2 + 100, 140, 40);
    }

    r.present();
}

int PokerGame::handleEvent(const SDL_Event& event) {
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

    if (m_waitingForAction) {
        Player* current = m_players[m_currentPlayerIdx];
        if (!current->isBot() && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int winW, winH;
            Renderer::getInstance().getWindowSize(winW, winH);
            int btnY = winH - 80;
            auto& r = Renderer::getInstance();
            int callAmount = m_currentBet - current->m_currentBet;

            if (r.isButtonClicked(winW/2 - 300, btnY, 80, 40)) {
                current->m_hasFolded = true;
                nextPlayer();
            } else if (r.isButtonClicked(winW/2 - 200, btnY, 120, 40)) {
                int toCall = std::min(callAmount, current->getMoney());
                if (toCall > 0) {
                    current->placeBet(toCall);
                    m_pot += toCall;
                }
                if (current->getMoney() == 0) current->m_isAllIn = true;
                current->m_hasActed = true;
                nextPlayer();
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
                }
            }
        }
    }
    return 0;
}
