#include "ReviewController.h"
#include "DbManager.h"
#include "AuthHelper.h"

void ReviewController::addReview(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    CurrentUser user = AuthHelper::getUser(req);
    if (!user.isLoggedIn() || !user.isBuyer()) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Only logged-in buyers can review products.";
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
    int rating = body.get("rating", 0).asInt();
    std::string comment = body.get("comment", "").asString();

    if (productId <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Invalid product specified.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (rating < 1 || rating > 5) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Rating must be between 1 and 5 stars.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    // Check if buyer has already reviewed this product
    auto existing = DbManager::instance().query(
        "SELECT review_id FROM reviews WHERE product_id = ? AND buyer_id = ?",
        {std::to_string(productId), std::to_string(user.id)}
    );

    if (!existing.empty()) {
        // Update existing review
        DbManager::instance().execute(
            "UPDATE reviews SET rating = ?, comment = ?, created_at = CURRENT_TIMESTAMP WHERE review_id = ?",
            {std::to_string(rating), comment, existing[0]["review_id"].asString()}
        );

        Json::Value ret;
        ret["success"] = true;
        ret["message"] = "Your review has been updated!";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        return;
    }

    // Insert new review
    int64_t reviewId = DbManager::instance().execute(
        "INSERT INTO reviews (product_id, buyer_id, rating, comment) VALUES (?, ?, ?, ?)",
        {std::to_string(productId), std::to_string(user.id), std::to_string(rating), comment}
    );

    if (reviewId <= 0) {
        Json::Value ret;
        ret["success"] = false;
        ret["message"] = "Failed to submit review.";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    Json::Value ret;
    ret["success"] = true;
    ret["message"] = "Review submitted successfully! Thank you for your feedback.";
    ret["review_id"] = static_cast<Json::Int64>(reviewId);

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

void ReviewController::getProductReviews(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId) {
    std::string sql = 
        "SELECT r.review_id, r.product_id, r.buyer_id, r.rating, r.comment, r.created_at, "
        "       u.name AS buyer_name "
        "FROM reviews r "
        "JOIN users u ON r.buyer_id = u.id "
        "WHERE r.product_id = ? "
        "ORDER BY r.review_id DESC;";

    Json::Value reviews = DbManager::instance().query(sql, {std::to_string(productId)});

    double totalRating = 0.0;
    for (const auto& rev : reviews) {
        totalRating += rev["rating"].asDouble();
    }
    double avgRating = reviews.empty() ? 0.0 : (totalRating / reviews.size());

    Json::Value ret;
    ret["success"] = true;
    ret["reviews"] = reviews;
    ret["count"] = reviews.size();
    ret["average_rating"] = avgRating;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
