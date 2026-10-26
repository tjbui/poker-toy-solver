#pragma once

/*
 * kuhn.h -- Kuhn poker as a CFR game.
 *
 * Deck J, Q, K. Both players ante 1 and get one card; the third is never seen.
 * Actions are always 0 = pass (check / fold) and 1 = bet (bet / call), and the
 * history is one char per action:
 *   pp   check, check        -> showdown for 1
 *   bp   bet, fold           -> player 0 wins 1
 *   bb   bet, call           -> showdown for 2
 *   pbp  check, bet, fold    -> player 1 wins 1
 *   pbb  check, bet, call    -> showdown for 2
 * Known equilibrium (tests/test_kuhn.cpp checks it): player 0's value is -1/18.
 */

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace solver {

struct KuhnGame {
    enum Action { kPass = 0, kBet = 1 };

    struct State {
        std::array<int, 2> cards{-1, -1};   // 0 = J, 1 = Q, 2 = K; -1 = not dealt yet
        std::string history;                // 'p' / 'b' per action so far
    };

    State initial() const { return State{}; }

    // The only chance event is the deal, before any action.
    bool is_chance(const State& s) const { return s.cards[0] < 0; }

    // All 6 ordered deals of two different cards, each with probability 1/6.
    std::vector<std::pair<State, double>> chance_outcomes(const State& s) const {
        std::vector<std::pair<State, double>> out;
        for (int c0 = 0; c0 < 3; c0++) {
            for (int c1 = 0; c1 < 3; c1++) {
                if (c0 == c1) continue;
                State dealt = s;
                dealt.cards = {c0, c1};
                out.push_back({dealt, 1.0 / 6.0});
            }
        }
        return out;
    }

    // pp, or any bet that has been answered (…bp folded, …bb called).
    bool is_terminal(const State& s) const {
        const std::string& h = s.history;
        if (h == "pp") return true;
        if (h.size() < 2) return false;
        const std::string last2 = h.substr(h.size() - 2);
        return last2 == "bp" || last2 == "bb";
    }

    // Value to player 0 at a terminal history.
    double payoff(const State& s) const {
        const std::string& h = s.history;
        if (h.size() >= 2 && h.substr(h.size() - 2) == "bp") {
            const int folder = static_cast<int>(h.size() - 1) % 2;   // who passed facing the bet
            return folder == 1 ? 1.0 : -1.0;
        }
        const double stake = h.find('b') == std::string::npos ? 1.0 : 2.0;   // ante (+ bet)
        return s.cards[0] > s.cards[1] ? stake : -stake;
    }

    int current_player(const State& s) const { return static_cast<int>(s.history.size()) % 2; }

    int num_actions(const State&) const { return 2; }

    State apply(const State& s, int action) const {
        State next = s;
        next.history += action == kBet ? 'b' : 'p';
        return next;
    }

    // What the acting player sees: their own card + the betting so far, e.g. "Qpb".
    std::string infoset_key(const State& s) const {
        return std::string(1, "JQK"[s.cards[current_player(s)]]) + s.history;
    }

    std::string action_name(int action) const { return action == kBet ? "bet" : "pass"; }
};

} // namespace solver
