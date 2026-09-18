#include <drogon/drogon.h>
#include <iostream>
#include "DbManager.h"

int main(int argc, char* argv[]) {
    std::cout << "============================================================" << std::endl;
    std::cout << "                    MONICA MART                           " << std::endl;
    std::cout << "        Multi-Seller E-Commerce Shopping Website          " << std::endl;
    std::cout << "             College Capstone Project                      " << std::endl;
    std::cout << "              Developer: Monica Sornam                     " << std::endl;
    std::cout << "============================================================" << std::endl;

    // 1. Initialize SQLite Database & Seeds
    std::string dbFile = "monica_mart.db";
    std::string schemaFile = "database/database.sql";
    
    if (!DbManager::instance().init(dbFile, schemaFile)) {
        std::cerr << "[ERROR] Failed to initialize SQLite database! Exiting..." << std::endl;
        return 1;
    }
    std::cout << "[SUCCESS] SQLite database connected and ready." << std::endl;

    // 2. Configure Drogon Application
    try {
        drogon::app().loadConfigFile("config.json");
    } catch (...) {
        std::cout << "[INFO] config.json not found in working dir, using built-in defaults." << std::endl;
        drogon::app().addListener("0.0.0.0", 8080);
        drogon::app().setDocumentRoot("./frontend");
        drogon::app().setHomePage("index.html");
        drogon::app().enableSession(7200);
    }

    // Enable CORS for frontend API requests
    drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr& req,
                                              drogon::AdviceCallback&& acb,
                                              drogon::AdviceChainCallback&& accb) {
        if (req->method() == drogon::HttpMethod::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            acb(resp);
            return;
        }
        accb();
    });

    // Add CORS headers to all responses
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr& req,
                                               const drogon::HttpResponsePtr& resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    std::cout << "[SERVER] Starting Drogon C++20 Server on http://0.0.0.0:8080" << std::endl;
    std::cout << "[SERVER] Web interface: http://localhost:8080" << std::endl;

    // 3. Start Drogon Event Loop
    drogon::app().run();

    return 0;
}
