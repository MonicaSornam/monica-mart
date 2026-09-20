#ifndef ORDER_CONTROLLER_H
#define ORDER_CONTROLLER_H

#include <drogon/HttpController.h>

class OrderController : public drogon::HttpController<OrderController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderController::checkout, "/api/orders", drogon::Post);
    ADD_METHOD_TO(OrderController::getBuyerOrders, "/api/orders", drogon::Get);
    ADD_METHOD_TO(OrderController::getOrderDetails, "/api/orders/{1}", drogon::Get);
    ADD_METHOD_TO(OrderController::updateOrderStatus, "/api/orders/{1}/status", drogon::Put);
    METHOD_LIST_END

    void checkout(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getBuyerOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getOrderDetails(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int orderId);
    void updateOrderStatus(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int orderId);
};

#endif // ORDER_CONTROLLER_H
