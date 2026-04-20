#include <iostream>
#include "httplib.h"

int main() {
    httplib::Server server;

    // GET test
    server.Get("/hello", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello World from C++!", "text/plain");
    });

    // POST test
    server.Post("/api/test", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "Received POST request\n";
        std::cout << "Body: " << req.body << std::endl;

        res.set_content("Hello from POST endpoint", "text/plain");
    });

    std::cout << "Server running at http://localhost:8080\n";
    server.listen("0.0.0.0", 8080);
}