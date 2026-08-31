/*
 * equity_cli.cpp — terminal equity calculator (replaces the old HTTP backend).
 *
 * Goal: read hero/villain hole cards (+ optional board) from the command line
 * or stdin, call the engine, and print exact equity to the terminal.
 *
 * Salvage reference: the old backend/src/main.cpp (see git history from before
 * the migration commit) has the request-parsing + engine-call flow to adapt
 * from JSON/HTTP into plain terminal I/O.
 *
 * TODO (you implement):
 *   1. Parse argv into an EquityRequest (hero.cards, villain.cards, community).
 *   2. Call get_equity_multithr(request)  (Phase 3: exact enumeration).
 *   3. Print heroWinPct / villainWinPct / tiePct / runtimeMs.
 */

#include <iostream>

#include "engine.h"

int main(int /*argc*/, char** /*argv*/) {
    std::cout << "equity_cli: not implemented yet.\n";
    std::cout << "Usage (planned): equity_cli <heroCards> <villainCards> [board]\n";
    return 0;
} /* main() */
