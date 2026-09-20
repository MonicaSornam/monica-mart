#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::registerUser, "/api/register", drogon::Post);
    ADD_METHOD_TO(AuthController::login, "/api/login", drogon::Post);
    ADD_METHOD_TO(AuthController::logout, "/api/logout", drogon::Post);
    ADD_METHOD_TO(AuthController::getMe, "/api/me", drogon::Get);
    METHOD_LIST_END

    void registerUser(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void logout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getMe(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // AUTH_CONTROLLER_H
