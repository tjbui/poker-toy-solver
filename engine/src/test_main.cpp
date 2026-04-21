#include "engine.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::optional;
using std::string;
using std::vector;

void print_result(const string& label, const EquityResult& result) {
    cout << label << "\n";
    cout << "Hero win %: " << result.heroWinPct << "\n";
    cout << "Villain win %: " << result.villainWinPct << "\n";
    cout << "Tie %: " << result.tiePct << "\n";
    cout << "Hands: " << result.simulatedHands << "\n";
    cout << "Runtime ms: " << result.runtimeMs << "\n";
    cout << "Engine: " << result.engine << "\n";
    cout << "-----------------------------\n";
}

int main() {
    {
        EquityRequest req{};
        req.hero.mode = "exact";
        req.hero.cards = {string("S-A"), string("H-K")};

        req.villain.mode = "exact";
        req.villain.cards = {string("C-3"), string("D-3")};

        req.community = {std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        req.simulations = 1000;
        req.engine = "cpu";

        EquityResult result = run_equity_simulation_iterative(req);
        print_result("AK vs 33 preflop", result);
    }

    {
        EquityRequest req{};
        req.hero.mode = "exact";
        req.hero.cards = {string("S-A"), string("H-A")};

        req.villain.mode = "exact";
        req.villain.cards = {string("C-K"), string("D-K")};

        req.community = {std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        req.simulations = 1000;
        req.engine = "cpu";

        EquityResult result = run_equity_simulation_iterative(req);
        print_result("AA vs KK preflop", result);
    }

    {
        EquityRequest req{};
        req.hero.mode = "exact";
        req.hero.cards = {string("S-A"), string("S-K")};

        req.villain.mode = "exact";
        req.villain.cards = {string("H-Q"), string("H-J")};

        req.community = {
            string("S-T"),
            string("S-2"),
            string("S-3"),
            string("D-9"),
            string("C-4")
        };
        req.simulations = 1;
        req.engine = "cpu";

        EquityResult result = run_equity_simulation_iterative(req);
        print_result("Made flush board test", result);
    }

    {
        EquityRequest req{};
        req.hero.mode = "exact";
        req.hero.cards = {string("S-A"), string("H-A")};

        req.villain.mode = "exact";
        req.villain.cards = {string("C-A"), string("D-A")};

        req.community = {
            string("S-K"),
            string("H-K"),
            string("D-Q"),
            string("C-Q"),
            string("S-2")
        };
        req.simulations = 1;
        req.engine = "cpu";

        EquityResult result = run_equity_simulation_iterative(req);
        print_result("Tie test", result);
    }

    return 0;
}