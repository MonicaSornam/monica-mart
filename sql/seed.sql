-- ============================================================
-- MONICA MART - Database Seed Data
-- College Capstone Project
-- Developer: Monica Sornam
-- ============================================================

-- 1. CATEGORIES
INSERT OR IGNORE INTO categories (category_id, category_name) VALUES
(1, 'Electronics'),
(2, 'Fashion'),
(3, 'Home'),
(4, 'Beauty'),
(5, 'Accessories');

-- 2. USERS
-- Admin Account (Password: Admin@123)
-- SHA-256: e86f78a8a3caf0b60d8e74e5942aa6d86dc150cd3c03338aef25b7d2d7e3acc7
INSERT OR IGNORE INTO users (id, name, email, password, role) VALUES 
(1, 'Admin Monica', 'admin@monicamart.com', 'e86f78a8a3caf0b60d8e74e5942aa6d86dc150cd3c03338aef25b7d2d7e3acc7', 'ADMIN');

-- Seller Accounts (Password: Password@123)
-- SHA-256: ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f
INSERT OR IGNORE INTO users (id, name, email, password, role) VALUES 
(2, 'TechZone Electronics', 'tech_seller@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'SELLER'),
(3, 'Elite Fashion Hub', 'fashion_seller@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'SELLER');

-- Buyer Accounts (Password: Password@123)
INSERT OR IGNORE INTO users (id, name, email, password, role) VALUES 
(4, 'John Doe', 'buyer1@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'BUYER'),
(5, 'Jane Smith', 'buyer2@monicamart.com', 'ff7bd97b1a7789ddd2775122fd6817f3173672da9f802ceec57f284325bf589f', 'BUYER');

-- 3. CARTS FOR BUYERS
INSERT OR IGNORE INTO cart (cart_id, buyer_id) VALUES
(1, 4),
(2, 5);

-- 4. PRODUCTS
-- Sample Products specified in the Capstone Specification:
-- Laptop ₹45000, Mobile ₹18000, Headphones ₹999, Smart Watch ₹1999, Handbag ₹1499
INSERT OR IGNORE INTO products (product_id, seller_id, product_name, description, price, category, quantity, image) VALUES
(1, 2, 'Laptop Pro 15 Slim', 'High performance Intel Core i7 processor with 16GB RAM, 512GB NVMe SSD, and 15.6-inch FHD anti-glare display. Ideal for students, coding, and multitasking.', 45000.0, 'Electronics', 12, 'https://images.unsplash.com/photo-1496181133206-80ce9b88a853?w=600&auto=format&fit=crop&q=80'),
(2, 2, 'Mobile Neo 5G Smartphone', 'Flagship 5G smartphone with 64MP AI triple camera, 5000mAh all-day battery, 128GB storage, and 120Hz AMOLED super smooth display.', 18000.0, 'Electronics', 25, 'https://images.unsplash.com/photo-1511707171634-5f897ff02aa9?w=600&auto=format&fit=crop&q=80'),
(3, 2, 'Headphones Wireless Bluetooth', 'Premium over-ear wireless Bluetooth headphones with active noise cancellation, deep bass, built-in mic, and 30-hour playback.', 999.0, 'Electronics', 40, 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=600&auto=format&fit=crop&q=80'),
(4, 2, 'Smart Watch Active 2', 'Waterproof smart fitness watch with heart rate monitor, sleep tracking, SpO2 sensor, and customizable pastel watch faces.', 1999.0, 'Accessories', 30, 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80'),
(5, 3, 'Handbag Designer Vegan Leather', 'Handcrafted pastel vegan leather handbag featuring golden metallic zippers, spacious interior compartments, and detachable shoulder strap.', 1499.0, 'Fashion', 18, 'https://images.unsplash.com/photo-1584917865442-de89df76afd3?w=600&auto=format&fit=crop&q=80'),
(6, 3, 'Performance Running Shoes', 'Lightweight, breathable athletic running shoes with responsive shock-absorbing foam sole for maximum daily comfort.', 2499.0, 'Fashion', 22, 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?w=600&auto=format&fit=crop&q=80'),
(7, 3, 'Organic Herbal Skincare Kit', 'All-natural herbal facial cleanser, hydrating toner, and vitamin C glow serum suitable for all sensitive skin types.', 899.0, 'Beauty', 35, 'https://images.unsplash.com/photo-1556228720-195a672e8a03?w=600&auto=format&fit=crop&q=80'),
(8, 2, 'Minimalist LED Desk Lamp', 'Touch-controlled dimmable eye-caring desk lamp with 3 pastel light temperature modes, flexible neck, and USB charging port.', 649.0, 'Home', 50, 'https://images.unsplash.com/photo-1534972195531-a756b1126f24?w=600&auto=format&fit=crop&q=80');

-- 5. REVIEWS
INSERT OR IGNORE INTO reviews (review_id, product_id, buyer_id, rating, comment) VALUES
(1, 1, 4, 5, 'Super fast laptop for coding and college projects! Battery life lasts full day.'),
(2, 3, 5, 4, 'Very comfortable headphones with rich sound and clear bass. Great value!'),
(3, 4, 4, 5, 'Best budget smartwatch I have bought. Beautiful display and accurate step tracker.'),
(4, 6, 5, 5, 'Extremely comfortable running shoes, lightweight and great cushioning.');

-- 6. INITIAL DEMO ORDERS
INSERT OR IGNORE INTO orders (order_id, buyer_id, total_amount, status, shipping_address) VALUES
(1, 4, 1999.0, 'Delivered', '123 College Road, Chennai, Tamil Nadu 600001'),
(2, 5, 2499.0, 'Pending', '45 Green Park Avenue, Coimbatore, Tamil Nadu 641001');

INSERT OR IGNORE INTO order_items (order_item_id, order_id, product_id, seller_id, quantity, price_per_unit) VALUES
(1, 1, 4, 2, 1, 1999.0),
(2, 2, 6, 3, 1, 2499.0);
