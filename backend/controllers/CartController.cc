#include "CartController.h"
#include "DbManager.h"
#include "AuthHelper.h"

namespace {
    // Helper to get or create buyer cart_id
    int64_t getOrCreateCartId(int64_t buyerId) {
        auto rows = DbManager::instance().query("SELECT cart_id FROM cart WHERE buyer_id = ?", {std::to_string(buyerId)});
        if (!rows.empty()) {
            return rows[0]["cart_id"].asInt64();
        }
        return DbManager::instance().execute("INSERT INTO cart (buyer_id) VALUES (?)", {std::to_string(buyerId)});
    }
}

void CartController::getCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Please log in as a buyer to view your cart.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    int64_t cartId = getOrCreateCartId(user.id);

    std::string sql = 
        "SELECT ci.item_id, ci.cart_id, ci.product_id, ci.quantity, "
        "       p.product_name, p.price, p.image, p.category, p.quantity AS stock, "
        "       ROUND(ci.quantity * p.price, 2) AS subtotal "
        "FROM cart_items ci "
        "JOIN products p ON ci.product_id = p.product_id "
        "WHERE ci.cart_id = ? "
        "ORDER BY ci.item_id DESC;";

    Json::Value items = DbManager::instance().query(sql, {std::to_string(cartId)});

    double totalAmount = 0.0;
    int totalItems = 0;
    for (const auto& item : items) {
        totalAmount += item["subtotal"].asDouble();
        totalItems += item["quantity"].asInt();
    }

    Json::Value ret;
    ret["success"] = true;
    ret["cart_id"] = static_cast<Json::Int64>(cartId);
    ret["items"] = items;
    ret["total_amount"] = totalAmount;
    ret["total_items"] = totalItems;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void CartController::addToCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Please log in as a buyer to add items to the cart.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid JSON body.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const Json::Value& body = *jsonPtr;
    int productId = body.get("product_id", 0).asInt();
    int quantity = body.get("quantity", 1).asInt();

    if (productId <= 0 || quantity <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid product or quantity specified.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Check available stock
    auto prodRows = DbManager::instance().query(
        "SELECT product_id, product_name, quantity, price FROM products WHERE product_id = ?",
        {std::to_string(productId)}
    );

    if (prodRows.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Product not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    int availableStock = prodRows[0]["quantity"].asInt();
    if (availableStock <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Sorry, this product is currently out of stock.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    int64_t cartId = getOrCreateCartId(user.id);

    // Check if item already exists in cart
    auto existingItem = DbManager::instance().query(
        "SELECT item_id, quantity FROM cart_items WHERE cart_id = ? AND product_id = ?",
        {std::to_string(cartId), std::to_string(productId)}
    );

    if (!existingItem.empty()) {
        int currentCartQty = existingItem[0]["quantity"].asInt();
        int newQty = currentCartQty + quantity;

        if (newQty > availableStock) {
            Json::Value ret;
            ret["success"] = false;
            ret["message"] = "Insufficient stock. You already have " + std::to_string(currentCartQty) + 
                             " in cart, and only " + std::to_string(availableStock) + " are available.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        DbManager::instance().execute(
            "UPDATE cart_items SET quantity = ? WHERE item_id = ?",
            {std::to_string(newQty), existingItem[0]["item_id"].asString()}
        );
    } else {
        if (quantity > availableStock) {
            Json::Value ret;
            ret["success"] = false;
            ret["message"] = "Insufficient stock. Only " + std::to_string(availableStock) + " items available.";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
            return;
        }

        DbManager::instance().execute(
            "INSERT INTO cart_items (cart_id, product_id, quantity) VALUES (?, ?, ?)",
            {std::to_string(cartId), std::to_string(productId), std::to_string(quantity)}
        );
    }

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Product added to cart.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void CartController::updateCartItem(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int itemId) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid JSON body.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    int newQty = jsonPtr->get("quantity", 1).asInt();
    if (newQty <= 0) {
        // If 0, delete item
        DbManager::instance().execute("DELETE FROM cart_items WHERE item_id = ?", {std::to_string(itemId)});
        Json::Value ret;
        ret["success"] = true;
        ret["message"] = "Item removed from cart.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        return;
    }

    // Verify stock
    std::string sql = 
        "SELECT ci.item_id, p.quantity AS stock "
        "FROM cart_items ci "
        "JOIN products p ON ci.product_id = p.product_id "
        "WHERE ci.item_id = ?;";
    auto rows = DbManager::instance().query(sql, {std::to_string(itemId)});

    if (rows.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Cart item not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    int availableStock = rows[0]["stock"].asInt();
    if (newQty > availableStock) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Cannot exceed available stock (" + std::to_string(availableStock) + ").";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    DbManager::instance().execute("UPDATE cart_items SET quantity = ? WHERE item_id = ?", {std::to_string(newQty), std::to_string(itemId)});

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Cart updated successfully.";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void CartController::removeFromCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int itemId) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    DbManager::instance().execute("DELETE FROM cart_items WHERE item_id = ?", {std::to_string(itemId)});

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Item removed from cart.";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
