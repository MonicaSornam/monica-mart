#ifndef CART_CONTROLLER_H
#define CART_CONTROLLER_H

#include <drogon/HttpController.h>

class CartController : public drogon::HttpController<CartController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CartController::getCart, "/api/cart", drogon::Get);
    ADD_METHOD_TO(CartController::addToCart, "/api/cart", drogon::Post);
    ADD_METHOD_TO(CartController::addToCart, "/api/cart/add", drogon::Post);
    ADD_METHOD_TO(CartController::updateCartItem, "/api/cart/{1}", drogon::Put);
    ADD_METHOD_TO(CartController::updateCartItem, "/api/cart/update/{1}", drogon::Put);
    ADD_METHOD_TO(CartController::removeFromCart, "/api/cart/{1}", drogon::Delete);
    ADD_METHOD_TO(CartController::removeFromCart, "/api/cart/remove/{1}", drogon::Delete);
    METHOD_LIST_END

    void getCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void addToCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void updateCartItem(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int itemId);
    void removeFromCart(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int itemId);
};

#endif // CART_CONTROLLER_H
