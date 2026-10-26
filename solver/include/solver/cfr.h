#pragma once

/*
 * cfr.h -- vanilla Counterfactual Regret Minimization, written once for any game.
 *
 * CfrTrainer<Game> works with any two-player zero-sum Game type that provides
 * (see kuhn.h for a full example):
 *   State                      the game's own small value type
 *   State initial()            the root (before any chance event)
 *   bool  is_chance(s)         + chance_outcomes(s) -> {child, probability} pairs
 *   bool  is_terminal(s)       + payoff(s) -> value to player 0
 *   int   current_player(s)    0 or 1
 *   int   num_actions(s)       + apply(s, a) -> child state
 *   std::string infoset_key(s) what the acting player can see (their card + history)
 *
 * Every value here is from player 0's point of view; player 1's is the negative.
 * It's a template (not a virtual base class) so each game keeps its own State
 * struct, copied down the recursion with no heap allocation, and calls inline.
 */

#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace solver {

// Regret matching: clip negatives to 0, normalize; uniform if nothing is positive.
inline std::vector<double> regret_matching(const std::vector<double>& regretSum) {
    const int n = static_cast<int>(regretSum.size());
    std::vector<double> strategy(n);
    double normalizer = 0.0;
    for (int a = 0; a < n; a++) {
        strategy[a] = regretSum[a] > 0.0 ? regretSum[a] : 0.0;
        normalizer += strategy[a];
    }
    for (int a = 0; a < n; a++) {
        strategy[a] = normalizer > 0.0 ? strategy[a] / normalizer : 1.0 / n;
    }
    return strategy;
}

// Average a strategySum pile into a final strategy; uniform if it's empty.
inline std::vector<double> average(const std::vector<double>& strategySum) {
    const int n = static_cast<int>(strategySum.size());
    double total = 0.0;
    for (double x : strategySum) total += x;
    std::vector<double> avg(n);
    for (int a = 0; a < n; a++) {
        avg[a] = total > 0.0 ? strategySum[a] / total : 1.0 / n;
    }
    return avg;
}

// One information set: everything CFR remembers about one decision.
struct InfoNode {
    std::vector<double> regretSum;    // drives the next current strategy
    std::vector<double> strategySum;  // averaged at the end: the answer
    std::vector<double> strategy;     // current strategy, fixed for one iteration

    explicit InfoNode(int num_actions)
        : regretSum(num_actions, 0.0),
          strategySum(num_actions, 0.0),
          strategy(num_actions, 1.0 / num_actions) {}
};

template <class Game>
class CfrTrainer {
public:
    using State = typename Game::State;

    explicit CfrTrainer(Game game = Game{}) : game_(std::move(game)) {}

    // Run more iterations. Each one walks the whole game tree once, then every
    // infoset's next current strategy comes from its updated regrets.
    void train(int iterations) {
        for (int it = 0; it < iterations; it++) {
            cfr(game_.initial(), {1.0, 1.0}, 1.0);
            for (auto& [key, node] : infosets_) {
                node.strategy = regret_matching(node.regretSum);
            }
        }
        iterations_ += iterations;
    }

    // The answer for one infoset: its average strategy. Throws if never visited.
    std::vector<double> average_strategy(const std::string& key) const {
        return average(infosets_.at(key).strategySum);
    }

    // Player 0's expected value when both players use their average strategies.
    double expected_value() const { return value_of(game_.initial()); }

    const std::unordered_map<std::string, InfoNode>& infosets() const { return infosets_; }
    const Game& game() const { return game_; }
    long long iterations() const { return iterations_; }

private:
    /*
     * One CFR visit. reach[p] = probability that player p's OWN actions lead
     * here; chance = probability of the chance events (the deal) on the way.
     * Returns this node's value to player 0 under the current strategies, and
     * updates the acting player's regrets and strategy sum on the way back up.
     */
    double cfr(const State& s, std::array<double, 2> reach, double chance) {
        if (game_.is_terminal(s)) {
            return game_.payoff(s);
        }
        if (game_.is_chance(s)) {
            double value = 0.0;
            for (const auto& [child, p] : game_.chance_outcomes(s)) {
                value += p * cfr(child, reach, chance * p);
            }
            return value;
        }

        const int player = game_.current_player(s);
        const int n = game_.num_actions(s);
        // unordered_map references stay valid when children insert new infosets.
        InfoNode& node = infosets_.try_emplace(game_.infoset_key(s), n).first->second;

        // Value of each action, and of the mix actually played.
        std::vector<double> action_value(n);
        double node_value = 0.0;
        for (int a = 0; a < n; a++) {
            std::array<double, 2> next = reach;
            next[player] *= node.strategy[a];
            action_value[a] = cfr(game_.apply(s, a), next, chance);
            node_value += node.strategy[a] * action_value[a];
        }

        // Regret is weighted by how often the OPPONENT and the deal bring us here
        // (counterfactual); the strategy sum by how often WE do.
        const double sign = player == 0 ? 1.0 : -1.0;     // player 1's view is the negative
        const double opponent_reach = reach[1 - player] * chance;
        for (int a = 0; a < n; a++) {
            node.regretSum[a] += opponent_reach * sign * (action_value[a] - node_value);
            node.strategySum[a] += reach[player] * node.strategy[a];
        }
        return node_value;
    }

    // Value to player 0 with both players on their average strategies (no updates).
    double value_of(const State& s) const {
        if (game_.is_terminal(s)) {
            return game_.payoff(s);
        }
        if (game_.is_chance(s)) {
            double value = 0.0;
            for (const auto& [child, p] : game_.chance_outcomes(s)) {
                value += p * value_of(child);
            }
            return value;
        }
        const std::vector<double> strategy = average_strategy(game_.infoset_key(s));
        double value = 0.0;
        for (int a = 0; a < game_.num_actions(s); a++) {
            value += strategy[a] * value_of(game_.apply(s, a));
        }
        return value;
    }

    Game game_;
    std::unordered_map<std::string, InfoNode> infosets_;
    long long iterations_ = 0;
};

} // namespace solver
