#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "httplib.h"
#include "json.hpp"
#include "engine.h"

using json = nlohmann::json;

static std::vector<std::optional<std::string>> parse_nullable_string_array(const json& arr) {
    std::vector<std::optional<std::string>> result;
    result.reserve(arr.size());

    for (const auto& item : arr) {
        if (item.is_null()) {
            result.push_back(std::nullopt);
        } else {
            result.push_back(item.get<std::string>());
        }
    }

    return result;
}

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

            request.hero.mode = body.at("hero").at("mode").get<std::string>();
            request.hero.cards = parse_nullable_string_array(body.at("hero").at("cards"));
            request.hero.range = body.at("hero").at("range").get<std::vector<std::string>>();

            request.villain.mode = body.at("villain").at("mode").get<std::string>();
            request.villain.cards = parse_nullable_string_array(body.at("villain").at("cards"));
            request.villain.range = body.at("villain").at("range").get<std::vector<std::string>>();

            request.community = parse_nullable_string_array(body.at("community"));
            request.simulations = body.at("simulations").get<int>();

            EquityResult result = run_equity_simulation_iterative(request);

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
}