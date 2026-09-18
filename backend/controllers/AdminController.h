#ifndef ADMIN_CONTROLLER_H
#define ADMIN_CONTROLLER_H

#include <drogon/HttpController.h>

class AdminController : public drogon::HttpController<AdminController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AdminController::getAllUsers, "/api/admin/users", drogon::Get);
    ADD_METHOD_TO(AdminController::getAllProducts, "/api/admin/products", drogon::Get);
    ADD_METHOD_TO(AdminController::getAllOrders, "/api/admin/orders", drogon::Get);
    ADD_METHOD_TO(AdminController::deleteProduct, "/api/admin/products/{1}", drogon::Delete);
    ADD_METHOD_TO(AdminController::getDashboardStats, "/api/admin/stats", drogon::Get);
    METHOD_LIST_END

    void getAllUsers(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getAllProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getAllOrders(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void deleteProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
    void getDashboardStats(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

#endif // ADMIN_CONTROLLER_H
