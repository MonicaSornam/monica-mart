#ifndef CHATBOT_CONTROLLER_H
#define CHATBOT_CONTROLLER_H

#include <drogon/HttpController.h>

class ChatbotController : public drogon::HttpController<ChatbotController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ChatbotController::handleChat, "/api/chatbot", drogon::Post);
    METHOD_LIST_END

    void handleChat(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // CHATBOT_CONTROLLER_H
