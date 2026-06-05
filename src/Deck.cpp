#include "Deck.h"
#include <algorithm>
#include <chrono>

Deck::Deck() : m_rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    buildDeck();
    shuffle();
}

void Deck::buildDeck() {
    m_cards.clear();
    for (int s = 0; s < 4; ++s) {
        for (int r = 0; r < 13; ++r) {
            m_cards.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
}

void Deck::shuffle() {
    std::shuffle(m_cards.begin(), m_cards.end(), m_rng);
}

Card Deck::dealCard() {
    if (m_cards.empty()) {
        buildDeck();
        shuffle();
    }
    Card card = m_cards.back();
    m_cards.pop_back();
    return card;
}

void Deck::reset() {
    buildDeck();
    shuffle();
}

int Deck::remainingCards() const {
    return static_cast<int>(m_cards.size());
}
