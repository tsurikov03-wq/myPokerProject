#ifndef PLAYER_H
#define PLAYER_H

#include "Card.h"
#include <vector>
#include <string>

class Player {
public:
    Player(const std::string& name);
    virtual ~Player() = default;
    virtual std::string getAction(const std::vector<std::string>& options) = 0;
    virtual bool isBot() const { return false; }
    void addCard(const Card& card);
    void clearHand();
    const std::vector<Card>& getHand() const;
    int getHandValue() const;
    std::string getName() const;
    void setMoney(int amount);
    int getMoney() const;
    void placeBet(int amount);
    void winMoney(int amount);
protected:
    std::string m_name;
    std::vector<Card> m_hand;
    int m_money;
};

class HumanPlayer : public Player {
public:
    HumanPlayer(const std::string& name);
    std::string getAction(const std::vector<std::string>& options) override;
    void setPendingAction(const std::string& action);
    std::string m_pendingAction;
};

class BotPlayer : public Player {
public:
    BotPlayer(const std::string& name);
    std::string getAction(const std::vector<std::string>& options) override;
    bool isBot() const override { return true; }
};

#endif