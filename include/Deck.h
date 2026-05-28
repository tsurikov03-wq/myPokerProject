#ifndef DECK_H
#define DECK_H

#include "Card.h"
#include <vector>
#include <random>

class Deck {
public:
    Deck();
    void shuffle();
    Card dealCard();
    void reset();
    int remainingCards() const;
private:
    std::vector<Card> m_cards;
    std::mt19937 m_rng;
    void buildDeck();
};

#endif