#ifndef PRODUCT_CONTROLLER_H
#define PRODUCT_CONTROLLER_H

#include <drogon/HttpController.h>

class ProductController : public drogon::HttpController<ProductController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProductController::getProducts, "/api/products", drogon::Get);
    ADD_METHOD_TO(ProductController::getProductById, "/api/products/{1}", drogon::Get);
    ADD_METHOD_TO(ProductController::createProduct, "/api/products", drogon::Post);
    ADD_METHOD_TO(ProductController::updateProduct, "/api/products/{1}", drogon::Put);
    ADD_METHOD_TO(ProductController::deleteProduct, "/api/products/{1}", drogon::Delete);
    METHOD_LIST_END

    void getProducts(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getProductById(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
    void createProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void updateProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
    void deleteProduct(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
};

#endif // PRODUCT_CONTROLLER_H
