#include "Card.h"

Card::Card(Suit suit, Rank rank) : m_suit(suit), m_rank(rank) {}
Suit Card::getSuit() const { return m_suit; }
Rank Card::getRank() const { return m_rank; }

int Card::getValue() const {
    if (m_rank == Rank::Ace) return 11;
    if (m_rank >= Rank::Jack) return 10;
    return static_cast<int>(m_rank) + 2;
}

std::string Card::toString() const {
    std::string rankStr;
    switch (m_rank) {
        case Rank::Two:   rankStr = "2"; break;
        case Rank::Three: rankStr = "3"; break;
        case Rank::Four:  rankStr = "4"; break;
        case Rank::Five:  rankStr = "5"; break;
        case Rank::Six:   rankStr = "6"; break;
        case Rank::Seven: rankStr = "7"; break;
        case Rank::Eight: rankStr = "8"; break;
        case Rank::Nine:  rankStr = "9"; break;
        case Rank::Ten:   rankStr = "10"; break;
        case Rank::Jack:  rankStr = "J"; break;
        case Rank::Queen: rankStr = "Q"; break;
        case Rank::King:  rankStr = "K"; break;
        case Rank::Ace:   rankStr = "A"; break;
    }
    std::string suitStr;
    switch (m_suit) {
        case Suit::Clubs:    suitStr = "♣"; break;
        case Suit::Diamonds: suitStr = "♦"; break;
        case Suit::Hearts:   suitStr = "♥"; break;
        case Suit::Spades:   suitStr = "♠"; break;
    }
    return rankStr + suitStr;
}