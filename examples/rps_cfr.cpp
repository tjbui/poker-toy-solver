#include <iostream>
#include <array>
#include <numeric>

enum RPSIndex {
    ROCK,
    PAPER,
    SCISSORS
};

// A strategy / regret list: one slot per action [Rock, Paper, Scissors].
using Strategy = std::array<double, 3>;

/*
 * Get the EV of playing rock (arr[ROCK]), paper (arr[PAPER]) and scissors (arr[SCISSORS])
 * given the opponent's strategy.
 */
Strategy getEV(const Strategy &oppStr) {
    return Strategy
        {
            oppStr[SCISSORS] - oppStr[PAPER],   // EV(Rock)
            oppStr[ROCK]     - oppStr[SCISSORS], // EV(Paper)
            oppStr[PAPER]    - oppStr[ROCK]      // EV(Scissors)
        };
}

/*
 * Regret matching: clip negatives to 0, normalize the rest.
 * If nothing is positive, play uniform [1/3, 1/3, 1/3].
 */
Strategy regretMatch(const Strategy &regretSum) {
    Strategy curStr{};
    double normalizer = 0.0;
    for (int a = 0; a < 3; a++) {
        curStr[a] = regretSum[a] > 0.0 ? regretSum[a] : 0.0;
        normalizer += curStr[a];
    }
    for (int a = 0; a < 3; a++) {
        curStr[a] = normalizer > 0.0 ? curStr[a] / normalizer : 1.0 / 3.0;
    }
    return curStr;
}

// Average a strategySum pile into a final strategy (divide by its total).
Strategy average(const Strategy &strategySum) {
    Strategy avg{};
    double total = std::accumulate(strategySum.begin(), strategySum.end(), 0.0);
    for (int a = 0; a < 3; a++) {
        avg[a] = total > 0.0 ? strategySum[a] / total : 1.0 / 3.0;
    }
    return avg;
}

int main() {
    // Two SEPARATE piles per player: regretSum drives the next move,
    // strategySum is averaged at the end for the GTO answer.
    Strategy regretSumA{}, strategySumA{};
    Strategy regretSumB{}, strategySumB{};

    // Seed different starting strategies so we can watch the current strategy
    // chase, then settle to Nash on average. regretSum still starts at 0.
    Strategy curA{1.0, 0.0, 0.0}; // A starts pure Rock
    Strategy curB{0.0, 1.0, 0.0}; // B starts pure Paper

    for (int i = 0; i < 100000; i++) {
        // 1. current strategies from regret matching
        //    (iteration 0 uses the seeds above; after that, regret matching)
        if (i > 0) {
            curA = regretMatch(regretSumA);
            curB = regretMatch(regretSumB);
        }

        // 2. add current strategies to the strategy-sum piles
        for (int a = 0; a < 3; a++) {
            strategySumA[a] += curA[a];
            strategySumB[a] += curB[a];
        }

        // Print A's CURRENT strategy vs its running AVERAGE at a few checkpoints.
        // Watch: current keeps bouncing, average slides toward [.33, .33, .33].
        if (i == 0 || i == 9 || i == 99 || i == 999 || i == 9999 || i == 99999) {
            Strategy avgSoFar = average(strategySumA);
            std::cout << "iter " << i + 1
                      << " | A current: "
                      << curA[ROCK] << " " << curA[PAPER] << " " << curA[SCISSORS]
                      << " | A average: "
                      << avgSoFar[ROCK] << " " << avgSoFar[PAPER] << " " << avgSoFar[SCISSORS]
                      << "\n";
        }

        // 3. EV of each action vs the OTHER player's current strategy
        Strategy evA = getEV(curB);
        Strategy evB = getEV(curA);

        // 4. val = EV of the mix I actually played; regret = EV - val
        double valA = 0.0, valB = 0.0;
        for (int a = 0; a < 3; a++) {
            valA += curA[a] * evA[a];
            valB += curB[a] * evB[a];
        }
        for (int a = 0; a < 3; a++) {
            regretSumA[a] += evA[a] - valA;
            regretSumB[a] += evB[a] - valB;
        }
    }

    Strategy avgA = average(strategySumA);
    Strategy avgB = average(strategySumB);

    std::cout << "A (GTO): " << avgA[ROCK] << " " << avgA[PAPER] << " " << avgA[SCISSORS] << "\n";
    std::cout << "B (GTO): " << avgB[ROCK] << " " << avgB[PAPER] << " " << avgB[SCISSORS] << "\n";

    return 0;
}
