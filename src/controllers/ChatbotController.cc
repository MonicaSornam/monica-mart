#include "ChatbotController.h"
#include <algorithm>
#include <string>

namespace {
    std::string toLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return lower;
    }
}

void ChatbotController::handleChat(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("message")) {
        Json::Value ret;
        ret["success"] = false;
        ret["reply"] = "Please send a message to chat with Monica Mart Assistant.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string userMsg = toLower((*jsonPtr)["message"].asString());
    std::string botReply;

    // Rule-based NLP pattern matching
    if (userMsg.find("register") != std::string::npos || userMsg.find("sign up") != std::string::npos || userMsg.find("create account") != std::string::npos) {
        botReply = "Open the Register page from the navigation bar and choose whether you want to register as a Buyer or a Seller.";
    } else if (userMsg.find("add product") != std::string::npos || userMsg.find("new product") != std::string::npos || userMsg.find("sell") != std::string::npos) {
        botReply = "Login as a seller and open the Seller Dashboard. Click 'Add Product' to enter product details, price, category, stock, and image.";
    } else if (userMsg.find("checkout") != std::string::npos || userMsg.find("place order") != std::string::npos || userMsg.find("payment") != std::string::npos) {
        botReply = "Add products to your cart and select 'Proceed to Checkout'. Review your order summary and click 'Confirm Order'.";
    } else if (userMsg.find("order") != std::string::npos || userMsg.find("history") != std::string::npos || userMsg.find("track") != std::string::npos) {
        botReply = "Open Order History by clicking 'Orders' in the top navigation bar to view your past purchases and delivery status.";
    } else if (userMsg.find("search") != std::string::npos || userMsg.find("find") != std::string::npos || userMsg.find("browse") != std::string::npos) {
        botReply = "Open the Products page and use the search bar or category filters to discover products.";
    } else if (userMsg.find("cart") != std::string::npos) {
        botReply = "Click 'Cart' in the top navigation bar to view your selected items, update quantities, or proceed to checkout.";
    } else if (userMsg.find("contact") != std::string::npos || userMsg.find("support") != std::string::npos || userMsg.find("help") != std::string::npos) {
        botReply = "Please contact our customer support team at support@monicamart.com or call +91 98765 43210.";
    } else if (userMsg.find("hello") != std::string::npos || userMsg.find("hi") != std::string::npos || userMsg.find("hey") != std::string::npos) {
        botReply = "Hello! Welcome to Monica Mart! How can I assist your shopping experience today?";
    } else if (userMsg.find("admin") != std::string::npos) {
        botReply = "Administrators can log in to view platform statistics, oversee users, and manage products.";
    } else if (userMsg.find("seller") != std::string::npos) {
        botReply = "Sellers can manage their inventory, add new products, edit listings, and view sales orders from the Seller Dashboard.";
    } else {
        botReply = "Sorry, I can currently answer basic questions about registration, products, cart, checkout, and orders.";
    }

    Json::Value ret;
    ret["success"] = true;
    ret["reply"] = botReply;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
