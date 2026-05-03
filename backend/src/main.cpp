/* main.cpp - http server to process requests from frontend and call engine */

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "httplib.h"
#include "json.hpp"
#include "engine.h"

using json = nlohmann::json;

/*
 * Parse string into PlayerMode enum
 */
PlayerMode parse_player_mode(const std::string& mode) {
    if (mode == "exact") {
        return PlayerMode::Exact;
    }

    if (mode == "range") {
        return PlayerMode::Range;
    }

    throw std::invalid_argument("Invalid player mode: " + mode);
} /* parse_player_mode() */

/*
 * Template to work for all arrays of any fixed size N
 */
template <std::size_t N>
std::array<std::optional<std::string>, N> parse_nullable_string_array(const json& arr) {
    if (!arr.is_array()) {
        throw std::invalid_argument("Expected array");
    }

    if (arr.size() != N) {
        throw std::invalid_argument(
            "Expected array of size " + std::to_string(N)
        );
    }

    std::array<std::optional<std::string>, N> result{};

    for (std::size_t i = 0; i < N; i++) {
        if (arr.at(i).is_null()) {
            result[i] = std::nullopt;
        } else {
            result[i] = arr.at(i).get<std::string>();
        }
    }

    return result;
} /* parse_nullable_string_array() */

/*
 * Start HTTP Server and define POST request to calculate equity with iterative / multithreaded simulator
 */
int main() {
    httplib::Server server;

    server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "http://localhost:5173");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    server.Post("/api/equity", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "http://localhost:5173");

        try {
            json body = json::parse(req.body);

            EquityRequest request{};

            request.hero.mode = parse_player_mode(
                body.at("hero").at("mode").get<std::string>()
            );

            request.hero.cards = parse_nullable_string_array<2>(
                body.at("hero").at("cards")
            );

            request.hero.range = body.at("hero").at("range")
                .get<std::vector<std::string>>();

            request.villain.mode = parse_player_mode(
                body.at("villain").at("mode").get<std::string>()
            );

            request.villain.cards = parse_nullable_string_array<2>(
                body.at("villain").at("cards")
            );

            request.villain.range = body.at("villain").at("range")
                .get<std::vector<std::string>>();

            request.community = parse_nullable_string_array<5>(
                body.at("community")
            );

            request.simulations = body.at("simulations").get<int>();

            EquityResult result = get_equity_multithr(request);

            json response = {
                {"heroWinPct", result.heroWinPct},
                {"villainWinPct", result.villainWinPct},
                {"tiePct", result.tiePct},
                {"simulatedHands", result.simulatedHands},
                {"runtimeMs", result.runtimeMs}
            };

            res.set_content(response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            json error = {
                {"error", "Invalid request"},
                {"message", e.what()}
            };

            res.set_content(error.dump(), "application/json");
            res.status = 400;
        }
    });

    std::cout << "Server running at http://localhost:8080\n";
    server.listen("0.0.0.0", 8080);

    return 0;
} /* main() */