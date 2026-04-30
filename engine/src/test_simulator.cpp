#include "engine.h"

#include <optional>
#include <string>
#include <vector>

using std::optional;
using std::string;
using std::vector;

int main() {
    EquityRequest req{};

    // Change this scenario as needed while debugging
    req.hero.mode = "exact";
    req.hero.cards = {string("S-A"), string("H-K")};

    req.villain.mode = "exact";
    req.villain.cards = {string("C-3"), string("D-3")};

    req.community = {
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };

    req.simulations = 100000;

    req.engine = "cpu";

    // This will trigger your debug prints inside engine
    run_equity_simulation_iterative(req);

    return 0;
}