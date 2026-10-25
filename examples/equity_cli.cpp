/*
 * equity_cli.cpp -- terminal equity calculator.
 *
 * Usage:
 *   equity_cli [--backend auto|cpu-single|cpu-multi|gpu] [--gpu] HERO VILLAIN [BOARD]
 *
 *   HERO / VILLAIN
 *     exact hand : "AhKd"
 *     range      : comma-separated tokens, e.g. "AA,KK,AKs,QJo"
 *                  (a single class like "AKs" is also a range)
 *   BOARD (optional)
 *     0, 3, 4, or 5 cards, concatenated: "Ah7d2c"  (commas allowed: "Ah,7d,2c")
 *   --backend  which backend to run (default auto: GPU for big jobs if available)
 *   --gpu      shorthand for --backend gpu
 *
 * Examples:
 *   equity_cli AhKd 2c2d
 *   equity_cli AhKd 2c2d 7h8s9c
 *   equity_cli --gpu "AA,KK" "AKs,AKo" 2c7d9s
 */

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "equity/equity.h"

using namespace equity;

static PlayerRequest parse_player(const std::string& arg) {
    PlayerRequest p{};

    // A bare 4-char token with no comma is an exact hand ("AhKd").
    // Anything else (has a comma, or a 2-/3-char class) is a range.
    if (arg.size() == 4 && arg.find(',') == std::string::npos) {
        p.mode = PlayerMode::Exact;
        p.cards[0] = arg.substr(0, 2);
        p.cards[1] = arg.substr(2, 2);
        return p;
    }

    p.mode = PlayerMode::Range;
    std::string tok;
    for (char c : arg) {
        if (c == ',') {
            if (!tok.empty()) p.range.push_back(tok);
            tok.clear();
        } else {
            tok.push_back(c);
        }
    }
    if (!tok.empty()) p.range.push_back(tok);
    return p;
}

static std::array<std::optional<std::string>, 5> parse_board(const std::string& arg) {
    std::string cleaned;
    for (char c : arg) {
        if (c != ',') cleaned.push_back(c);
    }
    if (cleaned.size() % 2 != 0) {
        throw std::invalid_argument("Board must be whole cards (2 chars each)");
    }

    std::array<std::optional<std::string>, 5> community{};
    std::size_t slot = 0;
    for (std::size_t i = 0; i < cleaned.size(); i += 2) {
        if (slot >= 5) {
            throw std::invalid_argument("Board can have at most 5 cards");
        }
        community[slot++] = cleaned.substr(i, 2);
    }
    return community;
}

static Backend parse_backend(const std::string& name) {
    for (Backend b : {Backend::Auto, Backend::CpuSingle, Backend::CpuMulti, Backend::Gpu}) {
        if (name == backend_name(b)) return b;
    }
    throw std::invalid_argument("Unknown backend: " + name +
                                " (expected auto, cpu-single, cpu-multi or gpu)");
}

static void usage() {
    std::cout <<
        "Usage: equity_cli [--backend auto|cpu-single|cpu-multi|gpu] [--gpu] HERO VILLAIN [BOARD]\n"
        "  HERO/VILLAIN : exact \"AhKd\"  or  range \"AA,KK,AKs\"\n"
        "  BOARD        : 0/3/4/5 cards, e.g. \"7h8s9c\"\n"
        "  --backend    : default auto (GPU for big jobs when available)\n"
        "  --gpu        : shorthand for --backend gpu\n"
        "Example: equity_cli AhKd 2c2d 7h8s9c\n";
}

int main(int argc, char** argv) {
    try {
        EquityOptions options;
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            std::string a = argv[i];
            if (a == "--gpu") {
                options.backend = Backend::Gpu;
            } else if (a == "--backend") {
                if (i + 1 >= argc) throw std::invalid_argument("--backend needs a value");
                options.backend = parse_backend(argv[++i]);
            } else {
                args.push_back(a);
            }
        }

        if (args.size() < 2) {
            usage();
            return 1;
        }

        EquityRequest request{};
        request.hero = parse_player(args[0]);
        request.villain = parse_player(args[1]);
        if (args.size() >= 3) {
            request.community = parse_board(args[2]);
        }

        EquityResult r = compute_equity(request, options);

        std::cout.setf(std::ios::fixed);
        std::cout.precision(2);
        std::cout << "Hero:    " << r.heroWinPct << " %\n";
        std::cout << "Villain: " << r.villainWinPct << " %\n";
        std::cout << "Tie:     " << r.tiePct << " %\n";
        std::cout << "Runouts: " << r.evaluatedRunouts << "\n";
        std::cout << "Backend: " << backend_name(r.backend) << "\n";
        std::cout << "Time:    " << r.runtimeMs << " ms\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
} /* main() */
