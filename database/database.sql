-- ============================================================
-- MONICA MART - SQLite Database Schema & Seed Data
-- Author: Monica Sornam (Capstone Project)
-- ============================================================

PRAGMA foreign_keys = ON;

-- 1. USERS TABLE
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    email TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    role TEXT NOT NULL CHECK(role IN ('BUYER', 'SELLER', 'ADMIN')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. PRODUCTS TABLE
CREATE TABLE IF NOT EXISTS products (
    product_id INTEGER PRIMARY KEY AUTOINCREMENT,
    seller_id INTEGER NOT NULL,
    product_name TEXT NOT NULL,
    description TEXT,
    price REAL NOT NULL CHECK(price >= 0),
    category TEXT NOT NULL,
    quantity INTEGER NOT NULL CHECK(quantity >= 0),
    image TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (seller_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 3. CART TABLE
CREATE TABLE IF NOT EXISTS cart (
    cart_id INTEGER PRIMARY KEY AUTOINCREMENT,
    buyer_id INTEGER NOT NULL UNIQUE,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (buyer_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 4. CART ITEMS TABLE
CREATE TABLE IF NOT EXISTS cart_items (
    item_id INTEGER PRIMARY KEY AUTOINCREMENT,
    cart_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK(quantity > 0),
    FOREIGN KEY (cart_id) REFERENCES cart(cart_id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE CASCADE,
    UNIQUE(cart_id, product_id)
);

-- 5. ORDERS TABLE
CREATE TABLE IF NOT EXISTS orders (
    order_id INTEGER PRIMARY KEY AUTOINCREMENT,
    buyer_id INTEGER NOT NULL,
    total_amount REAL NOT NULL,
    status TEXT NOT NULL DEFAULT 'Pending' CHECK(status IN ('Pending', 'Processing', 'Shipped', 'Delivered', 'Cancelled')),
    shipping_address TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (buyer_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 6. ORDER ITEMS TABLE
CREATE TABLE IF NOT EXISTS order_items (
    order_item_id INTEGER PRIMARY KEY AUTOINCREMENT,
    order_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    seller_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL CHECK(quantity > 0),
    price_per_unit REAL NOT NULL,
    FOREIGN KEY (order_id) REFERENCES orders(order_id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(product_id),
    FOREIGN KEY (seller_id) REFERENCES users(id)
);

-- 7. REVIEWS TABLE
CREATE TABLE IF NOT EXISTS reviews (
    review_id INTEGER PRIMARY KEY AUTOINCREMENT,
    product_id INTEGER NOT NULL,
    buyer_id INTEGER NOT NULL,
    rating INTEGER NOT NULL CHECK(rating >= 1 AND rating <= 5),
    comment TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE CASCADE,
    FOREIGN KEY (buyer_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE(product_id, buyer_id)
);

-- 8. WISHLIST TABLE (OPTIONAL BONUS)
CREATE TABLE IF NOT EXISTS wishlist (
    wishlist_id INTEGER PRIMARY KEY AUTOINCREMENT,
    buyer_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (buyer_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE CASCADE,
    UNIQUE(buyer_id, product_id)
);

-- ============================================================
-- SEED DATA (Pre-configured for demonstration and testing)
-- ============================================================

-- Predefined Admin (Password: Admin@123)
-- SHA-256: e86f78a8a3caf0b60d8e74e5942aa6d86dc150cd3c03338aef25b7d2d7e3acc7
INSERT OR IGNORE INTO users (id, name, email, password, role) VALUES 
(1, 'Admin Monica', 'admin@monicamart.com', 'e86f78a8a3caf0b60d8e74e5942aa6d86dc150cd3c03338aef25b7d2d7e3acc7', 'ADMIN');

-- Predefined Sellers (Password: Password@123)
-- SHA-256: ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f
INSERT OR IGNORE INTO users (id, name, email, password, role) VALUES 
(2, 'TechZone Electronics', 'tech_seller@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'SELLER'),
(3, 'Elite Fashion Hub', 'fashion_seller@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'SELLER');

-- Predefined Buyers (Password: Password@123)
INSERT OR IGNORE INTO users (id, name, email, password, role) VALUES 
(4, 'John Doe', 'buyer1@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'BUYER'),
(5, 'Jane Smith', 'buyer2@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'BUYER');

-- Predefined Sample Products
INSERT OR IGNORE INTO products (product_id, seller_id, product_name, description, price, category, quantity, image) VALUES
(1, 2, 'Ultra Slim Laptop Pro 15', 'High performance quad-core processor with 16GB RAM, 512GB NVMe SSD, and 15.6-inch FHD anti-glare display. Ideal for students and developers.', 45000.0, 'Electronics', 12, 'https://images.unsplash.com/photo-1496181133206-80ce9b88a853?w=600&auto=format&fit=crop&q=80'),
(2, 2, 'Smartphone Neo 5G', 'Flagship 5G smartphone with 64MP AI triple camera, 5000mAh all-day battery, and 120Hz AMOLED super smooth display.', 18000.0, 'Electronics', 25, 'https://images.unsplash.com/photo-1511707171634-5f897ff02aa9?w=600&auto=format&fit=crop&q=80'),
(3, 2, 'Wireless Noise-Cancelling Headphones', 'Premium over-ear wireless Bluetooth headphones with active noise cancellation, deep bass, and 30-hour battery life.', 999.0, 'Accessories', 40, 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=600&auto=format&fit=crop&q=80'),
(4, 2, 'Smart Fitness Watch Active 2', 'Waterproof smart fitness watch with heart rate monitor, sleep tracking, SpO2 sensor, and customizable watch faces.', 1999.0, 'Accessories', 30, 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80'),
(5, 3, 'Designer Leather Handbag', 'Handcrafted vegan leather handbag featuring golden metallic zippers, spacious interior compartments, and elegant shoulder strap.', 1499.0, 'Fashion', 18, 'https://images.unsplash.com/photo-1584917865442-de89df76afd3?w=600&auto=format&fit=crop&q=80'),
(6, 3, 'Performance Running Shoes', 'Lightweight, breathable athletic running shoes with responsive shock-absorbing foam sole for maximum comfort.', 2499.0, 'Fashion', 22, 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?w=600&auto=format&fit=crop&q=80'),
(7, 3, 'Organic Herbal Skincare Kit', 'All-natural herbal facial cleanser, hydrating toner, and vitamin C glow serum suitable for all skin types.', 899.0, 'Beauty', 35, 'https://images.unsplash.com/photo-1556228720-195a672e8a03?w=600&auto=format&fit=crop&q=80'),
(8, 2, 'Minimalist LED Desk Lamp', 'Touch-controlled dimmable eye-caring desk lamp with 3 color modes, flexible neck, and built-in USB charging port.', 649.0, 'Home', 50, 'https://images.unsplash.com/photo-1534972195531-a756b1126f24?w=600&auto=format&fit=crop&q=80');

-- Sample Initial Reviews
INSERT OR IGNORE INTO reviews (review_id, product_id, buyer_id, rating, comment) VALUES
(1, 1, 4, 5, 'Super fast laptop for coding and multitasking! Excellent battery life.'),
(2, 3, 5, 4, 'Very comfortable headphones with rich sound and good noise cancellation.'),
(3, 4, 4, 5, 'Best budget smartwatch I have bought. Battery easily lasts 5 days.'),
(4, 6, 5, 5, 'Extremely comfortable running shoes, lightweight and great cushioning.');

-- Sample Initial Orders (To verify Order History and Seller Orders immediately)
INSERT OR IGNORE INTO orders (order_id, buyer_id, total_amount, status, shipping_address) VALUES
(1, 4, 1999.0, 'Delivered', '123 College Road, Chennai, Tamil Nadu 600001'),
(2, 5, 2499.0, 'Pending', '45 Green Park Avenue, Coimbatore, Tamil Nadu 641001');

INSERT OR IGNORE INTO order_items (order_item_id, order_id, product_id, seller_id, quantity, price_per_unit) VALUES
(1, 1, 4, 2, 1, 1999.0),
(2, 2, 6, 3, 1, 2499.0);
