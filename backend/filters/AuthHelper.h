#ifndef AUTH_HELPER_H
#define AUTH_HELPER_H

#include <drogon/drogon.h>
#include <string>
#include "DbManager.h"

struct CurrentUser {
    int64_t id = 0;
    std::string name;
    std::string email;
    std::string role;

    bool isLoggedIn() const { return id > 0; }
    bool isBuyer() const { return role == "BUYER"; }
    bool isSeller() const { return role == "SELLER"; }
    bool isAdmin() const { return role == "ADMIN"; }
};

class AuthHelper {
public:
    static CurrentUser getUser(const drogon::HttpRequestPtr& req) {
        CurrentUser user;

        // 1. Check Drogon HTTP Session
        auto session = req->session();
        if (session && session->find("user_id")) {
            user.id = session->get<int64_t>("user_id");
            user.name = session->get<std::string>("user_name");
            user.email = session->get<std::string>("user_email");
            user.role = session->get<std::string>("user_role");
            return user;
        }

        // 2. Fallback: Check X-User-Id header (useful for API clients & fetch requests)
        std::string headerUserId = req->getHeader("X-User-Id");
        if (!headerUserId.empty()) {
            auto rows = DbManager::instance().query(
                "SELECT id, name, email, role FROM users WHERE id = ?", {headerUserId});
            if (!rows.empty()) {
                user.id = rows[0]["id"].asInt64();
                user.name = rows[0]["name"].asString();
                user.email = rows[0]["email"].asString();
                user.role = rows[0]["role"].asString();
                return user;
            }
        }

        return user;
    }
};

#endif // AUTH_HELPER_H
