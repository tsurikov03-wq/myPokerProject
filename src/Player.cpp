#include "Player.h"
#include <algorithm>
#include <random>
#include <chrono>

Player::Player(const std::string& name, int startingMoney)
    : m_name(name), m_money(startingMoney) {}

void Player::addCard(const Card& card) {
    m_hand.push_back(card);
}

void Player::clearHand() {
    m_hand.clear();
    m_currentBet = 0;
    m_hasFolded = false;
    m_isAllIn = false;
    m_hasActed = false;
}

const std::vector<Card>& Player::getHand() const {
    return m_hand;
}

int Player::getHandValue() const {
    int value = 0;
    int aces = 0;
    for (const auto& card : m_hand) {
        int val = card.getValue();
        if (val == 11) aces++;
        value += val;
    }
    while (value > 21 && aces > 0) {
        value -= 10;
        aces--;
    }
    return value;
}

std::string Player::getName() const { return m_name; }
int Player::getMoney() const { return m_money; }
void Player::setMoney(int amount) { m_money = amount; }

void Player::placeBet(int amount) {
    if (amount > m_money) amount = m_money;
    m_money -= amount;
    m_currentBet += amount;
}

void Player::winMoney(int amount) {
    m_money += amount;
}

HumanPlayer::HumanPlayer(const std::string& name, int startingMoney)
    : Player(name, startingMoney), m_pendingAction("") {}

std::string HumanPlayer::getAction(const std::vector<std::string>& options) {
    std::string action = m_pendingAction;
    m_pendingAction = "";
    if (action.empty()) return "stand";
    return action;
}

void HumanPlayer::setPendingAction(const std::string& action) {
    m_pendingAction = action;
}

BotPlayer::BotPlayer(const std::string& name, int startingMoney)
    : Player(name, startingMoney) {}

std::string BotPlayer::getAction(const std::vector<std::string>& options) {
    if (std::find(options.begin(), options.end(), "hit") != options.end()) {
        if (getHandValue() < 17) return "hit";
        return "stand";
    }

    if (std::find(options.begin(), options.end(), "check") != options.end()) {
        return "check";
    }
    if (std::find(options.begin(), options.end(), "call") != options.end()) {
        return "call";
    }
    return "fold";
}
