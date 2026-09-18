#include "AuthController.h"
#include "DbManager.h"
#include "AuthHelper.h"

void AuthController::registerUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid JSON payload in request.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const Json::Value& body = *jsonPtr;
    std::string name = body.get("name", "").asString();
    std::string email = body.get("email", "").asString();
    std::string password = body.get("password", "").asString();
    std::string role = body.get("role", "").asString();

    // 1. Validation
    if (name.empty() || email.empty() || password.empty() || role.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "All fields (name, email, password, role) are required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (role != "BUYER" && role != "SELLER") {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid role selected. Must be BUYER or SELLER.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // 2. Check for duplicate email
    auto existing = DbManager::instance().query("SELECT id FROM users WHERE LOWER(email) = LOWER(?)", {email});
    if (!existing.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Email already exists. Please choose a different email or log in.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k409Conflict);
        callback(resp);
        return;
    }

    // 3. Hash password using SHA-256 and insert user
    std::string passwordHash = DbManager::hashPassword(password);
    int64_t newUserId = DbManager::instance().execute(
        "INSERT INTO users (name, email, password, role) VALUES (?, ?, ?, ?)",
        {name, email, passwordHash, role}
    );

    if (newUserId <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Registration failed due to a database error.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // 4. If BUYER, create cart entry
    if (role == "BUYER") {
        DbManager::instance().execute("INSERT OR IGNORE INTO cart (buyer_id) VALUES (?)", {std::to_string(newUserId)});
    }

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Registration successful! You can now log in.";
    ret["user"]["id"] = static_cast<Json::Int64>(newUserId);
    ret["user"]["name"] = name;
    ret["user"]["email"] = email;
    ret["user"]["role"] = role;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void AuthController::login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid JSON payload in request.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const Json::Value& body = *jsonPtr;
    std::string email = body.get("email", "").asString();
    std::string password = body.get("password", "").asString();

    if (email.empty() || password.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Email and password are required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Hash entered password to compare
    std::string passwordHash = DbManager::hashPassword(password);
    auto rows = DbManager::instance().query(
        "SELECT id, name, email, password, role FROM users WHERE LOWER(email) = LOWER(?)",
        {email}
    );

    if (rows.empty() || rows[0]["password"].asString() != passwordHash) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid email or password.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    int64_t userId = rows[0]["id"].asInt64();
    std::string name = rows[0]["name"].asString();
    std::string role = rows[0]["role"].asString();

    // Set Drogon Session
    auto session = req->session();
    if (session) {
        session->insert("user_id", userId);
        session->insert("user_name", name);
        session->insert("user_email", email);
        session->insert("user_role", role);
    }

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Login successful. Welcome back, " + name + "!";
    ret["user"]["id"] = static_cast<Json::Int64>(userId);
    ret["user"]["name"] = name;
    ret["user"]["email"] = email;
    ret["user"]["role"] = role;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthController::logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto session = req->session();
    if (session) {
        session->erase("user_id");
        session->erase("user_name");
        session->erase("user_email");
        session->erase("user_role");
    }

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Logged out successfully.";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AuthController::getMe(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Not logged in.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    Json::Value ret;
    ret["success"] = true;
    ret["user"]["id"] = static_cast<Json::Int64>(user.id);
    ret["user"]["name"] = user.name;
    ret["user"]["email"] = user.email;
    ret["user"]["role"] = user.role;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
