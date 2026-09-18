#include "SellerController.h"
#include "DbManager.h"
#include "AuthHelper.h"

void SellerController::getSellerProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isSeller()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Only logged-in sellers can access seller products.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    std::string sql = 
        "SELECT p.product_id, p.product_name, p.description, p.price, p.category, "
        "       p.quantity, p.image, p.created_at, "
        "       ROUND(COALESCE(AVG(r.rating), 0), 1) AS rating, "
        "       COUNT(r.review_id) AS review_count "
        "FROM products p "
        "LEFT JOIN reviews r ON p.product_id = r.product_id "
        "WHERE p.seller_id = ? "
        "GROUP BY p.product_id "
        "ORDER BY p.product_id DESC;";

    Json::Value products = DbManager::instance().query(sql, {std::to_string(user.id)});

    Json::Value ret;
    ret["success"] = true;
    ret["count"] = products.size();
    ret["products"] = products;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void SellerController::getSellerOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isSeller()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Only logged-in sellers can access seller orders.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    std::string sql = 
        "SELECT oi.order_item_id, oi.order_id, oi.product_id, oi.quantity, oi.price_per_unit, "
        "       (oi.quantity * oi.price_per_unit) AS line_total, "
        "       p.product_name, p.image, "
        "       o.status AS order_status, o.shipping_address, o.created_at, "
        "       u.name AS buyer_name, u.email AS buyer_email "
        "FROM order_items oi "
        "JOIN orders o ON oi.order_id = o.order_id "
        "JOIN products p ON oi.product_id = p.product_id "
        "JOIN users u ON o.buyer_id = u.id "
        "WHERE oi.seller_id = ? "
        "ORDER BY oi.order_item_id DESC;";

    Json::Value orders = DbManager::instance().query(sql, {std::to_string(user.id)});

    double totalRevenue = 0.0;
    int totalUnitsSold = 0;
    for (const auto& ord : orders) {
        totalRevenue += ord["line_total"].asDouble();
        totalUnitsSold += ord["quantity"].asInt();
    }

    Json::Value ret;
    ret["success"] = true;
    ret["count"] = orders.size();
    ret["orders"] = orders;
    ret["total_revenue"] = totalRevenue;
    ret["total_units_sold"] = totalUnitsSold;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
