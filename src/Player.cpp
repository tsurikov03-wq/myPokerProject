#include "Player.h"
#include <SDL3/SDL.h>

Player::Player(const std::string& name) : m_name(name), m_money(1000) {}
void Player::addCard(const Card& card) { m_hand.push_back(card); }
void Player::clearHand() { m_hand.clear(); }
const std::vector<Card>& Player::getHand() const { return m_hand; }
std::string Player::getName() const { return m_name; }
void Player::setMoney(int amount) { m_money = amount; }
int Player::getMoney() const { return m_money; }
void Player::placeBet(int amount) { m_money -= amount; }
void Player::winMoney(int amount) { m_money += amount; }

int Player::getHandValue() const {
    int value = 0, aces = 0;
    for (const auto& card : m_hand) {
        int v = card.getValue();
        if (v == 11) aces++;
        value += v;
    }
    while (value > 21 && aces > 0) { value -= 10; aces--; }
    return value;
}

HumanPlayer::HumanPlayer(const std::string& name) : Player(name), m_pendingAction("") {}
std::string HumanPlayer::getAction(const std::vector<std::string>& options) {
    while (m_pendingAction.empty()) SDL_Delay(10);
    std::string action = m_pendingAction;
    m_pendingAction.clear();
    return action;
}
void HumanPlayer::setPendingAction(const std::string& action) { m_pendingAction = action; }

BotPlayer::BotPlayer(const std::string& name) : Player(name) {}
std::string BotPlayer::getAction(const std::vector<std::string>& options) {
    if (options.size() >= 2 && options[0] == "hit" && options[1] == "stand")
        return (getHandValue() < 17) ? "hit" : "stand";
    return options[0];
}