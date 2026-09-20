# MONICA MART — College Capstone Viva Voce & Technical Guide

**Project Title:** MONICA MART — Full-Stack Multi-Seller E-Commerce Marketplace  
**Student Developer:** Monica Sornam  
**Target Degree:** College Capstone Project Evaluation  
**Backend:** C++20 + Drogon Web Framework  
**Database:** SQLite3 / PostgreSQL  
**Frontend:** Vanilla HTML5, CSS3, JavaScript  
**Theme:** Soft Pastel Identity (Lavender, Dusty Rose, Soft Pink, Peach)  
**Build System:** CMake & Docker  

---

## 1. Executive Summary & Objective

**MONICA MART** is an individual college capstone project simulating an enterprise e-commerce platform (similar to Amazon / Flipkart) with a manageable, high-performance scope.

It unites three distinct actors:
1. **Buyers**: Discover products via live keyword search and category filtering, inspect real-time stock, manage a shopping cart with boundary limits, checkout seamlessly, track order lifecycle, and leave star ratings and comments.
2. **Sellers**: Onboard independently, manage an exclusive catalog (add, edit, delete with validations), review sales revenue, and fulfill purchase orders.
3. **Admin**: Oversee platform health, inspect registered accounts, monitor gross marketplace merchandise value, and moderate inappropriate product listings with confirmation safeguards.
4. **AI Shopping Assistant**: An in-app assistant that guides users through registration, inventory queries, cart operations, checkout procedures, and order tracking.

---

## 2. Technology Stack & Technical Rationale

| Layer | Technology | Rationale for College Viva |
| :--- | :--- | :--- |
| **Language** | **C++20** | Demonstrates deep systems programming capability, memory safety, RAII idioms, and modern standards (`std::filesystem`, concepts, lambdas). |
| **Backend** | **Drogon Framework** | Ultra-high throughput, event-driven, non-blocking asynchronous HTTP framework written in C++. Outperforms standard Python/Node backends in benchmark latency and resource utilization. |
| **Database** | **SQLite3 / PostgreSQL** | Self-contained, zero-configuration relational database engine with full ACID compliance, foreign key cascade rules, and transaction rollback protection. |
| **Build System** | **CMake (>=3.16)** | Industry standard cross-platform build orchestration tool compiling Drogon controllers, models, and shared libraries. |
| **Frontend** | **Vanilla HTML5/CSS3/JS** | Zero heavy Node.js/npm dependencies. Delivers lightning-fast page loads, accessible semantic HTML, and straightforward code readability for viva reviewers. |
| **Design** | **Strict Pastel Palette** | Uses `#AE9CB7` (Lavender), `#D6B0BF` (Dusty Rose), `#F4C4C3` (Soft Pink), and `#FBD8C6` (Peach) with readable charcoal typography for an elegant, professional boutique identity. |

---

## 3. Architecture & Data Flow

```
[Browser Client]
       │
       ▼ (HTTP REST / JSON)
[Drogon C++20 Web Server :8080]
       │
       ├── Pre-Routing Advice (CORS, Preflight OPTIONS, Security Headers)
       │
       ├── HttpControllers:
       │     ├── AuthController    (/api/register, /api/login, /api/logout, /api/me)
       │     ├── ProductController (/api/products, /api/products/{id})
       │     ├── CartController    (/api/cart, /api/cart/add, /api/cart/update, /api/cart/remove)
       │     ├── OrderController   (/api/orders, /api/orders/{id}, /api/orders/{id}/status)
       │     ├── ReviewController  (/api/reviews, /api/products/{id}/reviews)
       │     ├── SellerController  (/api/seller/products, /api/seller/orders)
       │     ├── AdminController   (/api/admin/users, /api/admin/products, /api/admin/orders, /api/admin/stats)
       │     └── ChatbotController (/api/chatbot)
       │
       ├── AuthHelper (Role verification: isBuyer(), isSeller(), isAdmin())
       │
       ▼
[DbManager Singleton]
       │
       ├── Parameterized SQL Execution (SQLite3 C-API)
       ├── SHA-256 Cryptographic Password Hashing
       ├── Database Transaction Manager (BEGIN, COMMIT, ROLLBACK)
       │
       ▼
[(monica_mart.db) Relational SQLite3 Engine]
```

---

## 4. Top 10 Viva Questions & Model Answers

### Q1: Why did you choose C++20 and Drogon instead of conventional Node.js or Django?
> **Answer:** "Most student e-commerce projects rely on Node.js or Python. Choosing modern C++20 and Drogon demonstrates deep understanding of systems architecture, memory management, and asynchronous non-blocking event loops (Epoll/IOCP via Trantor). Drogon ranks among the fastest web frameworks in the world on TechEmpower benchmarks, consuming a fraction of the RAM required by interpreted languages while handling thousands of concurrent requests."

### Q2: How does the system handle concurrent purchases when stock is low?
> **Answer:** "During checkout in `OrderController.cc` (and `dev_server.py`), operations are wrapped in an atomic database transaction (`BEGIN TRANSACTION` ... `COMMIT`). Before creating the order, every cart item's requested quantity is compared against the warehouse stock (`quantity`). If requested quantity exceeds stock, the transaction is immediately rejected (`ROLLBACK`), preventing overselling."

### Q3: How do you prevent SQL Injection attacks?
> **Answer:** "All database interactions in `DbManager` use parameterized queries via the SQLite3 C API (`sqlite3_prepare_v2` and `sqlite3_bind_*`). User inputs are never concatenated directly into SQL statements, neutralizing SQL injection vectors."

### Q4: How is authentication and role authorization enforced?
> **Answer:** "Passwords are never stored in plaintext; they are hashed using cryptographic SHA-256 with deterministic salting. The backend enforces Role-Based Access Control (RBAC):
- Buyers cannot access `/api/seller/*` or `/api/admin/*`.
- Sellers can only edit or delete products matching their own `seller_id`.
- Only users with role `ADMIN` can access user lists or moderate listings."

### Q5: How does the AI Shopping Assistant function without external paid API keys?
> **Answer:** "To ensure zero deployment friction and 100% offline reliability during viva evaluation, the AI assistant utilizes an intelligent intent and keyword matching engine embedded in both C++ (`ChatbotController`) and JavaScript (`chatbot.js`). It accurately detects customer intent (e.g. search queries, cart questions, checkout flow, seller help, returns) and responds with formatted markdown answers instantly."

### Q6: Can you explain the database schema and foreign key relationships?
> **Answer:** "The database consists of 8 normalized relational tables:
- `users`: Core identity table with unique email and role constraints.
- `products`: Linked to `users(id)` via `seller_id` (`ON DELETE CASCADE`).
- `cart`: 1-to-1 relationship with `buyer_id`.
- `cart_items`: Junction table linking `cart_id` and `product_id`.
- `orders`: Records total amount, buyer ID, and delivery address.
- `order_items`: Captures product price snapshot and seller ID at the moment of checkout.
- `reviews`: Composite unique constraint `UNIQUE(product_id, buyer_id)` ensuring each buyer can only review a product once."

### Q7: Why did you redesign the website with a pastel palette?
> **Answer:** "The website was updated to a calm, modern, and boutique pastel palette:
- **Lavender (#AE9CB7):** Primary visual identity, headers, section accents, and borders.
- **Dusty Rose (#D6B0BF):** Secondary actions, product cards, hover states, and badges.
- **Soft Pink (#F4C4C3):** Card backgrounds, soft UI highlights, and review areas.
- **Peach (#FBD8C6):** Hero accents, empty states, and footer highlights.
- **White & Charcoal (#3C3140):** High contrast content surfaces ensuring full WCAG accessibility."

### Q8: What happens during checkout if the cart is empty?
> **Answer:** "Both client-side (`cart.js`) and server-side (`OrderController.cc`) validate the cart. If the cart has zero items, the checkout button is disabled on the frontend, and the backend returns a `400 Bad Request` with message 'Your cart is empty. Add products before checking out.'"

### Q9: Can Seller A edit or delete products created by Seller B?
> **Answer:** "No. In `ProductController::updateProduct` and `deleteProduct`, the database query verifies `WHERE product_id = ? AND seller_id = ?`. If a seller attempts to modify an item belonging to another seller, the server rejects the request with a `403 Forbidden` error."

### Q10: How can this project be built and demonstrated on different environments?
> **Answer:** 
> 1. **Zero-Setup Local Dev Mode (Windows/Mac/Linux):**
>    ```bash
>    python scripts/dev_server.py
>    ```
> 2. **C++20 Native Drogon Build:**
>    ```bash
>    mkdir build && cd build
>    cmake ..
>    cmake --build .
>    ./MonicaMart
>    ```
> 3. **Docker Multi-Stage Container:**
>    ```bash
>    docker build -t monica-mart .
>    docker run -p 8080:8080 monica-mart
>    ```
> 4. **Automated Test Suite Verification:**
>    ```bash
>    python scripts/test_suite.py
>    ```

---

## 5. Pre-Configured Test Credentials

| Role | Email | Password | Access / Purpose |
| :--- | :--- | :--- | :--- |
| **Admin** | `admin@monicamart.com` | `Admin@123` | Platform Governance, Product Moderation, User Audit |
| **Seller** | `tech_seller@monicamart.com` | `Password@123` | Electronics Storefront, Inventory CRUD, Order Tracking |
| **Seller** | `fashion_seller@monicamart.com` | `Password@123` | Fashion & Boutique Storefront |
| **Buyer** | `buyer1@monicamart.com` | `Password@123` | Shopping Cart, Checkout, Order History, Star Reviews |
| **Buyer** | `buyer2@monicamart.com` | `Password@123` | Secondary Buyer Account for Multi-User Testing |
