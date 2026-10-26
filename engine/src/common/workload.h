#pragma once

/*
 * workload.h -- turn an EquityRequest into the work every backend runs.
 *
 * Host-only: parsing, range expansion, matchup building. Both the CPU and the
 * GPU backends start from the same Workload, which is what lets them return
 * identical counts.
 */

#include <array>
#include <cstdint>
#include <vector>

#include "equity/equity.h"
#include "hand_eval.h"

namespace equity {

// One concrete hero-vs-villain deal: four distinct hole cards. Uploaded to the
// GPU as-is, so it must stay 4 packed bytes.
struct Matchup {
    Card h1, h2;   // hero
    Card v1, v2;   // villain
};

/*
 * A runout is one way to deal the missing community cards; every matchup has
 * the same number of them (runoutsPerMatchup). The total work is every
 * (matchup, runout) pair. Backends that split the work numbers it as one list,
 * index = matchup * runoutsPerMatchup + runout, where runout r is the r-th
 * lexicographic k-subset of the cards not used by that matchup or the known board.
 */
struct Workload {
    std::vector<Matchup> matchups;
    std::array<Card, 5> known{};  // known board cards; the first knownCount are valid
    int knownCount = 0;
    int k = 0;                    // unknown board cards to draw: 5 - knownCount
    int deckSize = 0;             // cards left per matchup: 48 - knownCount
    std::uint64_t runoutsPerMatchup = 0;  // C(deckSize, k), the same for every matchup
    std::uint64_t total = 0;              // matchups.size() * runoutsPerMatchup
};

// What a backend returns: raw counts over the whole work space.
struct EquityCounts {
    std::uint64_t heroWins = 0;
    std::uint64_t villainWins = 0;
    std::uint64_t ties = 0;

    std::uint64_t total() const { return heroWins + villainWins + ties; }

    EquityCounts& operator+=(const EquityCounts& o) {
        heroWins += o.heroWins;
        villainWins += o.villainWins;
        ties += o.ties;
        return *this;
    }
};

/*
 * Parse and expand the request into a Workload.
 * Throws std::invalid_argument on bad cards/tokens, card conflicts, or when
 * no valid matchup is left.
 */
Workload prepare_workload(const EquityRequest& request);

} // namespace equity
