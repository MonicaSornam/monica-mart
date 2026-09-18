#ifndef SELLER_CONTROLLER_H
#define SELLER_CONTROLLER_H

#include <drogon/HttpController.h>

class SellerController : public drogon::HttpController<SellerController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SellerController::getSellerProducts, "/api/seller/products", drogon::Get);
    ADD_METHOD_TO(SellerController::getSellerOrders, "/api/seller/orders", drogon::Get);
    METHOD_LIST_END

    void getSellerProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getSellerOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // SELLER_CONTROLLER_H
