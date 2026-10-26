#pragma once

/*
 * rps.h -- Rock-Paper-Scissors as a CFR game (a smoke test for the core).
 *
 * Written as a tree: player 0 moves, then player 1 moves WITHOUT seeing it,
 * so player 1 has a single infoset no matter what player 0 did -- that's how
 * simultaneous moves look in a game tree.
 *
 * rock_beats_scissors sets what winning with rock against scissors pays
 * (everything else pays 1). At 1 the equilibrium is [1/3, 1/3, 1/3], which is
 * also where CFR starts, so it's a weak test; at 2 it's [1/4, 1/2, 1/4].
 */

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace solver {

struct RpsGame {
    enum Action { kRock = 0, kPaper = 1, kScissors = 2 };

    double rock_beats_scissors = 1.0;

    struct State {
        std::array<int, 2> moves{-1, -1};
        int count = 0;                      // moves made so far
    };

    State initial() const { return State{}; }

    bool is_chance(const State&) const { return false; }
    std::vector<std::pair<State, double>> chance_outcomes(const State&) const { return {}; }

    bool is_terminal(const State& s) const { return s.count == 2; }

    // Value to player 0.
    double payoff(const State& s) const {
        return value(s.moves[0], s.moves[1]);
    }

    int current_player(const State& s) const { return s.count; }

    int num_actions(const State&) const { return 3; }

    State apply(const State& s, int action) const {
        State next = s;
        next.moves[s.count] = action;
        next.count++;
        return next;
    }

    // Player 1 never sees player 0's move, so each player has one infoset.
    std::string infoset_key(const State& s) const { return s.count == 0 ? "P1" : "P2"; }

    std::string action_name(int action) const {
        return action == kRock ? "rock" : action == kPaper ? "paper" : "scissors";
    }

private:
    // What `mine` wins (+) or loses (-) against `theirs`.
    double value(int mine, int theirs) const {
        if (mine == theirs) return 0.0;
        if (beats(mine, theirs)) return win_amount(mine, theirs);
        return -win_amount(theirs, mine);
    }

    static bool beats(int a, int b) {
        return (a == kRock && b == kScissors) || (a == kPaper && b == kRock) ||
               (a == kScissors && b == kPaper);
    }

    double win_amount(int winner, int loser) const {
        return winner == kRock && loser == kScissors ? rock_beats_scissors : 1.0;
    }
};

} // namespace solver
