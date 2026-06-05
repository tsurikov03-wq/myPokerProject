#ifndef CARD_H
#define CARD_H

#include <string>

enum class Suit { Clubs, Diamonds, Hearts, Spades };
enum class Rank { Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace };

class Card {
public:
    Card(Suit suit, Rank rank);

    Suit getSuit() const;
    Rank getRank() const;
    int getValue() const;
    int getPokerValue() const;
    bool isRed() const;
    std::string toString() const;
    std::string getTextureName() const;

private:
    Suit m_suit;
    Rank m_rank;
};

#endif
