#include "OrderController.h"
#include "DbManager.h"
#include "AuthHelper.h"

void OrderController::checkout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Please log in as a buyer to checkout.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    std::string shippingAddress = "Standard Delivery Address";
    auto jsonPtr = req->getJsonObject();
    if (jsonPtr && jsonPtr->isMember("shipping_address")) {
        shippingAddress = (*jsonPtr)["shipping_address"].asString();
        if (shippingAddress.empty()) shippingAddress = "Default Address";
    }

    // 1. Fetch Cart and Cart Items
    auto cartRows = DbManager::instance().query("SELECT cart_id FROM cart WHERE buyer_id = ?", {std::to_string(user.id)});
    if (cartRows.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Cart not found or empty.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    int64_t cartId = cartRows[0]["cart_id"].asInt64();

    std::string cartSql = 
        "SELECT ci.item_id, ci.product_id, ci.quantity, "
        "       p.product_name, p.price, p.seller_id, p.quantity AS stock "
        "FROM cart_items ci "
        "JOIN products p ON ci.product_id = p.product_id "
        "WHERE ci.cart_id = ?;";

    Json::Value items = DbManager::instance().query(cartSql, {std::to_string(cartId)});

    if (items.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Your cart is empty. Add products before checking out.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // 2. Validate Stock for all items
    double totalAmount = 0.0;
    for (const auto& item : items) {
        int requestedQty = item["quantity"].asInt();
        int stock = item["stock"].asInt();
        std::string productName = item["product_name"].asString();

        if (requestedQty > stock) {
            Json::Value ret;
            ret["success"] = false;
            ret["message"] = "Insufficient stock for \"" + productName + "\". Only " + 
                             std::to_string(stock) + " available.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        totalAmount += requestedQty * item["price"].asDouble();
    }

    // 3. Begin Transaction
    DbManager::instance().beginTransaction();

    // Create Order
    int64_t orderId = DbManager::instance().execute(
        "INSERT INTO orders (buyer_id, total_amount, status, shipping_address) VALUES (?, ?, 'Pending', ?)",
        {std::to_string(user.id), std::to_string(totalAmount), shippingAddress}
    );

    if (orderId <= 0) {
        DbManager::instance().rollback();
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Failed to create order record.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    // Insert Order Items and decrement stock
    for (const auto& item : items) {
        int productId = item["product_id"].asInt();
        int sellerId = item["seller_id"].asInt();
        int qty = item["quantity"].asInt();
        double price = item["price"].asDouble();

        DbManager::instance().execute(
            "INSERT INTO order_items (order_id, product_id, seller_id, quantity, price_per_unit) "
            "VALUES (?, ?, ?, ?, ?)",
            {std::to_string(orderId), std::to_string(productId), std::to_string(sellerId), std::to_string(qty), std::to_string(price)}
        );

        DbManager::instance().execute(
            "UPDATE products SET quantity = quantity - ? WHERE product_id = ?",
            {std::to_string(qty), std::to_string(productId)}
        );
    }

    // Clear Cart Items
    DbManager::instance().execute("DELETE FROM cart_items WHERE cart_id = ?", {std::to_string(cartId)});

    DbManager::instance().commit();

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Order Successful!";
    ret["order_id"] = static_cast<Json::Int64>(orderId);
    ret["total_amount"] = totalAmount;
    ret["status"] = "Pending";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void OrderController::getBuyerOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized. Please log in as a buyer.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    std::string sql = 
        "SELECT o.order_id, o.total_amount, o.status, o.shipping_address, o.created_at "
        "FROM orders o "
        "WHERE o.buyer_id = ? "
        "ORDER BY o.order_id DESC;";

    Json::Value orders = DbManager::instance().query(sql, {std::to_string(user.id)});

    for (auto& ord : orders) {
        int64_t orderId = ord["order_id"].asInt64();
        std::string itemsSql = 
            "SELECT oi.order_item_id, oi.product_id, oi.quantity, oi.price_per_unit, "
            "       p.product_name, p.image, u.name AS seller_name "
            "FROM order_items oi "
            "JOIN products p ON oi.product_id = p.product_id "
            "JOIN users u ON oi.seller_id = u.id "
            "WHERE oi.order_id = ?;";
        ord["items"] = DbManager::instance().query(itemsSql, {std::to_string(orderId)});
    }

    Json::Value ret;
    ret["success"] = true;
    ret["orders"] = orders;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void OrderController::getOrderDetails(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int orderId) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    std::string sql = 
        "SELECT o.order_id, o.buyer_id, o.total_amount, o.status, o.shipping_address, o.created_at, "
        "       u.name AS buyer_name, u.email AS buyer_email "
        "FROM orders o "
        "JOIN users u ON o.buyer_id = u.id "
        "WHERE o.order_id = ?;";

    auto rows = DbManager::instance().query(sql, {std::to_string(orderId)});
    if (rows.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Order not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    // Access check: only the buyer who made it, an involved seller, or admin can view
    if (!user.isAdmin() && user.isBuyer() && rows[0]["buyer_id"].asInt64() != user.id) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Forbidden.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    Json::Value order = rows[0];

    std::string itemsSql = 
        "SELECT oi.order_item_id, oi.product_id, oi.quantity, oi.price_per_unit, "
        "       p.product_name, p.image, u.name AS seller_name "
        "FROM order_items oi "
        "JOIN products p ON oi.product_id = p.product_id "
        "JOIN users u ON oi.seller_id = u.id "
        "WHERE oi.order_id = ?;";
    order["items"] = DbManager::instance().query(itemsSql, {std::to_string(orderId)});

    Json::Value ret;
    ret["success"] = true;
    ret["order"] = order;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
