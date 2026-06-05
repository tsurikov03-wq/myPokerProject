#include "Utils.h"
#include <map>
#include <algorithm>

int evaluateHandRank(const std::vector<Card>& hand) {
    if (hand.size() < 5) return 0;


    std::map<int, int> valCounts;
    std::map<Suit, int> suitCounts;
    std::vector<int> values;

    for (const auto& c : hand) {
        int v = c.getPokerValue();
        valCounts[v]++;
        suitCounts[c.getSuit()]++;
        values.push_back(v);
    }

    std::sort(values.begin(), values.end(), std::greater<int>());

    bool isFlush = false;
    for (auto const& [suit, count] : suitCounts) {
        if (count == 5) isFlush = true;
    }


    bool isStraight = false;
    int straightHigh = 0;
    if (valCounts.size() == 5) {
        if (values[0] - values[4] == 4) {
            isStraight = true;
            straightHigh = values[0];
        }

        if (values[0] == 14 && values[1] == 5 && values[2] == 4 && values[3] == 3 && values[4] == 2) {
            isStraight = true;
            straightHigh = 5;
        }
    }


    struct Freq { int val; int count; };
    std::vector<Freq> freqs;
    for (auto const& [val, count] : valCounts) {
        freqs.push_back({ val, count });
    }
    std::sort(freqs.begin(), freqs.end(), [](const Freq& a, const Freq& b) {
        if (a.count != b.count) return a.count > b.count;
        return a.val > b.val;
    });


    int category = 0;
    if (isStraight && isFlush) category = 8;
    else if (freqs[0].count == 4) category = 7;
    else if (freqs[0].count == 3 && freqs[1].count == 2) category = 6;
    else if (isFlush) category = 5;
    else if (isStraight) category = 4;
    else if (freqs[0].count == 3) category = 3;
    else if (freqs[0].count == 2 && freqs[1].count == 2) category = 2;
    else if (freqs[0].count == 2) category = 1;
    else category = 0;


    int score = category;
    if (isStraight) {
        score = (score << 4) + straightHigh;
    } else {
        for (const auto& f : freqs) {
            score = (score << 4) + f.val;
        }
    }
    return score;
}