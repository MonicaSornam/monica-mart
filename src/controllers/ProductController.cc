#include "ProductController.h"
#include "DbManager.h"
#include "AuthHelper.h"
#include <sstream>

void ProductController::getProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    std::string search = req->getParameter("search");
    std::string category = req->getParameter("category");

    std::string sql = 
        "SELECT p.product_id, p.seller_id, p.product_name, p.description, p.price, "
        "       p.category, p.quantity, p.image, p.created_at, u.name AS seller_name, "
        "       ROUND(COALESCE(AVG(r.rating), 0), 1) AS rating, "
        "       COUNT(r.review_id) AS review_count "
        "FROM products p "
        "JOIN users u ON p.seller_id = u.id "
        "LEFT JOIN reviews r ON p.product_id = r.product_id ";

    std::vector<std::string> params;
    std::vector<std::string> whereClauses;

    if (!search.empty()) {
        whereClauses.push_back("(p.product_name LIKE ? OR p.description LIKE ?)");
        params.push_back("%" + search + "%");
        params.push_back("%" + search + "%");
    }

    if (!category.empty() && category != "All") {
        whereClauses.push_back("p.category = ?");
        params.push_back(category);
    }

    if (!whereClauses.empty()) {
        sql += " WHERE ";
        for (size_t i = 0; i < whereClauses.size(); ++i) {
            sql += whereClauses[i];
            if (i + 1 < whereClauses.size()) sql += " AND ";
        }
    }

    sql += " GROUP BY p.product_id ORDER BY p.product_id DESC;";

    Json::Value products = DbManager::instance().query(sql, params);

    Json::Value ret;
    ret["success"] = true;
    ret["count"] = products.size();
    ret["products"] = products;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void ProductController::getProductById(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId) {
    std::string sql = 
        "SELECT p.product_id, p.seller_id, p.product_name, p.description, p.price, "
        "       p.category, p.quantity, p.image, p.created_at, u.name AS seller_name, "
        "       ROUND(COALESCE(AVG(r.rating), 0), 1) AS rating, "
        "       COUNT(r.review_id) AS review_count "
        "FROM products p "
        "JOIN users u ON p.seller_id = u.id "
        "LEFT JOIN reviews r ON p.product_id = r.product_id "
        "WHERE p.product_id = ? "
        "GROUP BY p.product_id;";

    auto rows = DbManager::instance().query(sql, {std::to_string(productId)});

    if (rows.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Product not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    Json::Value product = rows[0];

    // Fetch reviews for this product
    std::string reviewsSql = 
        "SELECT r.review_id, r.buyer_id, u.name AS buyer_name, r.rating, r.comment, r.created_at "
        "FROM reviews r "
        "JOIN users u ON r.buyer_id = u.id "
        "WHERE r.product_id = ? "
        "ORDER BY r.review_id DESC;";
    product["reviews"] = DbManager::instance().query(reviewsSql, {std::to_string(productId)});

    Json::Value ret;
    ret["success"] = true;
    ret["product"] = product;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void ProductController::createProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isSeller()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized access. Only registered sellers can add products.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
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
    std::string name = body.get("product_name", "").asString();
    std::string desc = body.get("description", "").asString();
    double price = body.get("price", 0.0).asDouble();
    std::string category = body.get("category", "").asString();
    int quantity = body.get("quantity", 0).asInt();
    std::string image = body.get("image", "").asString();

    if (name.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Product name is required.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (price <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Price must be a valid positive number.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (quantity < 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Quantity cannot be negative.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (category.empty()) {
        category = "General";
    }

    if (image.empty()) {
        image = "https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80";
    }

    int64_t newId = DbManager::instance().execute(
        "INSERT INTO products (seller_id, product_name, description, price, category, quantity, image) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)",
        {std::to_string(user.id), name, desc, std::to_string(price), category, std::to_string(quantity), image}
    );

    if (newId <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Failed to save product to database.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Product added successfully!";
    ret["product_id"] = static_cast<Json::Int64>(newId);

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void ProductController::updateProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isSeller()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized access. Only sellers can edit products.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    // Verify ownership: Seller A must NOT edit Seller B's product!
    auto existing = DbManager::instance().query(
        "SELECT product_id, seller_id FROM products WHERE product_id = ?",
        {std::to_string(productId)}
    );

    if (existing.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Product not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    if (existing[0]["seller_id"].asInt64() != user.id) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized: You can only edit your own products.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
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
    std::string name = body.get("product_name", "").asString();
    std::string desc = body.get("description", "").asString();
    double price = body.get("price", 0.0).asDouble();
    std::string category = body.get("category", "").asString();
    int quantity = body.get("quantity", 0).asInt();
    std::string image = body.get("image", "").asString();

    if (name.empty() || price <= 0 || quantity < 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid product details. Ensure name is provided, price > 0, and quantity >= 0.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    DbManager::instance().execute(
        "UPDATE products SET product_name = ?, description = ?, price = ?, category = ?, quantity = ?, image = ? "
        "WHERE product_id = ? AND seller_id = ?",
        {name, desc, std::to_string(price), category, std::to_string(quantity), image, std::to_string(productId), std::to_string(user.id)}
    );

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Product updated successfully!";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void ProductController::deleteProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized access.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    // Check ownership (Seller can delete their own, Admin can delete any)
    auto existing = DbManager::instance().query(
        "SELECT product_id, seller_id FROM products WHERE product_id = ?",
        {std::to_string(productId)}
    );

    if (existing.empty()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Product not found.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k404NotFound);
        callback(resp);
        return;
    }

    if (!user.isAdmin() && existing[0]["seller_id"].asInt64() != user.id) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Unauthorized: You can only delete your own products.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    DbManager::instance().execute("DELETE FROM products WHERE product_id = ?", {std::to_string(productId)});

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Product deleted successfully.";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
