#include "AdminController.h"
#include "DbManager.h"
#include "AuthHelper.h"

void AdminController::getAllUsers(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isAdmin()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Admin privileges required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    // Do NOT select passwords!
    std::string sql = "SELECT id, name, email, role, created_at FROM users ORDER BY id ASC;";
    Json::Value users = DbManager::instance().query(sql);

    Json::Value ret;
    ret["success"] = true;
    ret["count"] = users.size();
    ret["users"] = users;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AdminController::getAllProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isAdmin()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Admin privileges required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    std::string sql = 
        "SELECT p.product_id, p.seller_id, p.product_name, p.description, p.price, "
        "       p.category, p.quantity, p.image, p.created_at, u.name AS seller_name "
        "FROM products p "
        "JOIN users u ON p.seller_id = u.id "
        "ORDER BY p.product_id DESC;";

    Json::Value products = DbManager::instance().query(sql);

    Json::Value ret;
    ret["success"] = true;
    ret["count"] = products.size();
    ret["products"] = products;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AdminController::getAllOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isAdmin()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Admin privileges required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    std::string sql = 
        "SELECT o.order_id, o.buyer_id, o.total_amount, o.status, o.shipping_address, o.created_at, "
        "       u.name AS buyer_name, u.email AS buyer_email "
        "FROM orders o "
        "JOIN users u ON o.buyer_id = u.id "
        "ORDER BY o.order_id DESC;";

    Json::Value orders = DbManager::instance().query(sql);

    for (auto& ord : orders) {
        int64_t orderId = ord["order_id"].asInt64();
        std::string itemsSql = 
            "SELECT oi.order_item_id, oi.product_id, oi.quantity, oi.price_per_unit, "
            "       p.product_name, u.name AS seller_name "
            "FROM order_items oi "
            "JOIN products p ON oi.product_id = p.product_id "
            "JOIN users u ON oi.seller_id = u.id "
            "WHERE oi.order_id = ?;";
        ord["items"] = DbManager::instance().query(itemsSql, {std::to_string(orderId)});
    }

    Json::Value ret;
    ret["success"] = true;
    ret["count"] = orders.size();
    ret["orders"] = orders;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AdminController::deleteProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isAdmin()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Admin privileges required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto existing = DbManager::instance().query("SELECT product_id FROM products WHERE product_id = ?", {std::to_string(productId)});
    if (existing.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Product not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    DbManager::instance().execute("DELETE FROM products WHERE product_id = ?", {std::to_string(productId)});

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Product removed successfully by administrator.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void AdminController::getDashboardStats(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isAdmin()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto uRows = DbManager::instance().query("SELECT count(*) AS total_users FROM users;");
    auto pRows = DbManager::instance().query("SELECT count(*) AS total_products FROM products;");
    auto oRows = DbManager::instance().query("SELECT count(*) AS total_orders, COALESCE(SUM(total_amount), 0) AS total_revenue FROM orders;");

    Json::Value stats;
    stats["total_users"] = uRows.empty() ? 0 : uRows[0]["total_users"].asInt();
    stats["total_products"] = pRows.empty() ? 0 : pRows[0]["total_products"].asInt();
    stats["total_orders"] = oRows.empty() ? 0 : oRows[0]["total_orders"].asInt();
    stats["total_revenue"] = oRows.empty() ? 0.0 : oRows[0]["total_revenue"].asDouble();

    Json::Value ret;
    ret["success"] = true;
    ret["stats"] = stats;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
