#ifndef PLAYER_H
#define PLAYER_H

#include "Card.h"
#include <string>
#include <vector>

class Player {
public:
    Player(const std::string& name, int startingMoney = 1000);
    virtual ~Player() = default;

    virtual std::string getAction(const std::vector<std::string>& options) = 0;
    virtual bool isBot() const { return false; }

    void addCard(const Card& card);
    void clearHand();
    const std::vector<Card>& getHand() const;
    int getHandValue() const;
    bool isBust() const { return getHandValue() > 21; }

    std::string getName() const;
    int getMoney() const;
    void setMoney(int amount);
    void placeBet(int amount);
    void winMoney(int amount);
    bool canAfford(int amount) const { return m_money >= amount; }

    int m_currentBet = 0;
    bool m_hasFolded = false;
    bool m_isAllIn = false;
    bool m_hasActed = false;

protected:
    std::string m_name;
    std::vector<Card> m_hand;
    int m_money;
};

class HumanPlayer : public Player {
public:
    HumanPlayer(const std::string& name, int startingMoney = 1000);
    std::string getAction(const std::vector<std::string>& options) override;
    void setPendingAction(const std::string& action);
    std::string m_pendingAction;
};

class BotPlayer : public Player {
public:
    BotPlayer(const std::string& name, int startingMoney = 1000);
    std::string getAction(const std::vector<std::string>& options) override;
    bool isBot() const override { return true; }
};

#endif
