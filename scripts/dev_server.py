#!/usr/bin/env python3
"""
============================================================
MONICA MART - Local Development & Verification Server
Student Name: Monica Sornam
Project: College Capstone Project

This local runner provides a zero-setup local HTTP server
that serves the frontend/public web application and executes
the exact same SQLite REST API endpoints as the C++ Drogon backend.
Enables immediate Windows testing before or alongside Docker!
============================================================
"""

import http.server
import socketserver
import json
import sqlite3
import hashlib
import os
import urllib.parse

PORT = 8080
DB_FILE = 'monica_mart.db'
SCHEMA_FILE = os.path.join('sql', 'schema.sql')
SEED_FILE = os.path.join('sql', 'seed.sql')
FALLBACK_SQL = os.path.join('database', 'database.sql')

PUBLIC_DIR = os.path.abspath('public') if os.path.exists('public') else os.path.abspath('frontend')

def get_db():
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON;")
    return conn

def init_db():
    if not os.path.exists(DB_FILE) or os.path.getsize(DB_FILE) == 0:
        print(f"[DB] Initializing {DB_FILE}...")
        conn = get_db()
        if os.path.exists(SCHEMA_FILE) and os.path.exists(SEED_FILE):
            with open(SCHEMA_FILE, 'r', encoding='utf-8') as f:
                conn.executescript(f.read())
            with open(SEED_FILE, 'r', encoding='utf-8') as f:
                conn.executescript(f.read())
            print("[DB] Loaded schema and seed files from sql/!")
        elif os.path.exists(FALLBACK_SQL):
            with open(FALLBACK_SQL, 'r', encoding='utf-8') as f:
                conn.executescript(f.read())
            print("[DB] Loaded database from database/database.sql!")
        conn.close()
        print("[DB] Database initialized with seed accounts and sample products!")

def sha256_hash(text):
    return hashlib.sha256(text.encode('utf-8')).hexdigest()

class MonicaMartHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=PUBLIC_DIR, **kwargs)

    def send_json(self, data, status=200):
        body = json.dumps(data).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, X-User-Id, Authorization')
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, X-User-Id, Authorization')
        self.end_headers()

    def get_current_user(self):
        user_id = self.headers.get('X-User-Id')
        if not user_id:
            return None
        conn = get_db()
        user = conn.execute("SELECT id, name, email, role FROM users WHERE id = ?", (user_id,)).fetchone()
        conn.close()
        return dict(user) if user else None

    def read_json_body(self):
        length = int(self.headers.get('Content-Length', 0))
        if length == 0:
            return {}
        raw = self.rfile.read(length)
        return json.loads(raw.decode('utf-8'))

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        if not path.startswith('/api/'):
            return super().do_GET()

        conn = get_db()

        # 1. Products List
        if path == '/api/products':
            search = query.get('search', [''])[0]
            category = query.get('category', [''])[0]

            sql = """
                SELECT p.product_id, p.seller_id, p.product_name, p.description, p.price,
                       p.category, p.quantity, p.image, p.created_at, u.name AS seller_name,
                       ROUND(COALESCE(AVG(r.rating), 0), 1) AS rating,
                       COUNT(r.review_id) AS review_count
                FROM products p
                JOIN users u ON p.seller_id = u.id
                LEFT JOIN reviews r ON p.product_id = r.product_id
            """
            params = []
            where = []
            if search:
                where.append("(p.product_name LIKE ? OR p.description LIKE ?)")
                params.extend([f"%{search}%", f"%{search}%"])
            if category and category != 'All':
                where.append("p.category = ?")
                params.append(category)

            if where:
                sql += " WHERE " + " AND ".join(where)

            sql += " GROUP BY p.product_id ORDER BY p.product_id DESC"
            rows = conn.execute(sql, params).fetchall()
            products = [dict(r) for r in rows]
            conn.close()
            return self.send_json({"success": True, "count": len(products), "products": products})

        # 2. Product by ID
        if path.startswith('/api/products/'):
            parts = path.strip('/').split('/')
            if len(parts) == 3 and parts[2].isdigit():
                pid = parts[2]
                p = conn.execute("""
                    SELECT p.product_id, p.seller_id, p.product_name, p.description, p.price,
                           p.category, p.quantity, p.image, p.created_at, u.name AS seller_name,
                           ROUND(COALESCE(AVG(r.rating), 0), 1) AS rating,
                           COUNT(r.review_id) AS review_count
                    FROM products p
                    JOIN users u ON p.seller_id = u.id
                    LEFT JOIN reviews r ON p.product_id = r.product_id
                    WHERE p.product_id = ?
                    GROUP BY p.product_id
                """, (pid,)).fetchone()
                if not p:
                    conn.close()
                    return self.send_json({"success": False, "message": "Product not found."}, 404)
                
                prod = dict(p)
                revs = conn.execute("""
                    SELECT r.review_id, r.buyer_id, u.name AS buyer_name, r.rating, r.comment, r.created_at
                    FROM reviews r
                    JOIN users u ON r.buyer_id = u.id
                    WHERE r.product_id = ?
                    ORDER BY r.review_id DESC
                """, (pid,)).fetchall()
                prod['reviews'] = [dict(r) for r in revs]
                conn.close()
                return self.send_json({"success": True, "product": prod})

            # Product Reviews endpoint: /api/products/<id>/reviews
            if len(parts) == 4 and parts[2].isdigit() and parts[3] == 'reviews':
                pid = parts[2]
                revs = conn.execute("""
                    SELECT r.review_id, r.buyer_id, u.name AS buyer_name, r.rating, r.comment, r.created_at
                    FROM reviews r
                    JOIN users u ON r.buyer_id = u.id
                    WHERE r.product_id = ?
                    ORDER BY r.review_id DESC
                """, (pid,)).fetchall()
                conn.close()
                return self.send_json({"success": True, "reviews": [dict(r) for r in revs]})

        # 3. Cart View
        if path == '/api/cart':
            user = self.get_current_user()
            if not user or user['role'] != 'BUYER':
                conn.close()
                return self.send_json({"success": False, "message": "Login as buyer required."}, 401)

            cart = conn.execute("SELECT cart_id FROM cart WHERE buyer_id = ?", (user['id'],)).fetchone()
            if not cart:
                conn.close()
                return self.send_json({"success": True, "cart_id": None, "items": [], "total_amount": 0, "total_items": 0})

            items = conn.execute("""
                SELECT ci.item_id, ci.cart_id, ci.product_id, ci.quantity,
                       p.product_name, p.price, p.image, p.category, p.quantity AS stock,
                       ROUND(ci.quantity * p.price, 2) AS subtotal
                FROM cart_items ci
                JOIN products p ON ci.product_id = p.product_id
                WHERE ci.cart_id = ?
                ORDER BY ci.item_id DESC
            """, (cart['cart_id'],)).fetchall()

            item_list = [dict(i) for i in items]
            total_amt = sum(i['subtotal'] for i in item_list)
            total_qty = sum(i['quantity'] for i in item_list)
            conn.close()
            return self.send_json({
                "success": True,
                "cart_id": cart['cart_id'],
                "items": item_list,
                "total_amount": total_amt,
                "total_items": total_qty
            })

        # 4. Buyer Orders
        if path == '/api/orders':
            user = self.get_current_user()
            if not user or user['role'] != 'BUYER':
                conn.close()
                return self.send_json({"success": False, "message": "Login as buyer required."}, 401)

            orders = conn.execute("SELECT order_id, total_amount, status, shipping_address, created_at FROM orders WHERE buyer_id = ? ORDER BY order_id DESC", (user['id'],)).fetchall()
            order_list = []
            for o in orders:
                od = dict(o)
                items = conn.execute("""
                    SELECT oi.order_item_id, oi.product_id, oi.quantity, oi.price_per_unit,
                           p.product_name, p.image, u.name AS seller_name
                    FROM order_items oi
                    JOIN products p ON oi.product_id = p.product_id
                    JOIN users u ON oi.seller_id = u.id
                    WHERE oi.order_id = ?
                """, (od['order_id'],)).fetchall()
                od['items'] = [dict(i) for i in items]
                order_list.append(od)
            conn.close()
            return self.send_json({"success": True, "orders": order_list})

        # Order Details: /api/orders/<id>
        if path.startswith('/api/orders/'):
            parts = path.strip('/').split('/')
            if len(parts) == 3 and parts[2].isdigit():
                oid = parts[2]
                user = self.get_current_user()
                if not user:
                    conn.close()
                    return self.send_json({"success": False, "message": "Login required."}, 401)
                
                ord_row = conn.execute("SELECT * FROM orders WHERE order_id = ?", (oid,)).fetchone()
                if not ord_row:
                    conn.close()
                    return self.send_json({"success": False, "message": "Order not found."}, 404)
                
                od = dict(ord_row)
                items = conn.execute("""
                    SELECT oi.order_item_id, oi.product_id, oi.quantity, oi.price_per_unit,
                           p.product_name, p.image, u.name AS seller_name
                    FROM order_items oi
                    JOIN products p ON oi.product_id = p.product_id
                    JOIN users u ON oi.seller_id = u.id
                    WHERE oi.order_id = ?
                """, (oid,)).fetchall()
                od['items'] = [dict(i) for i in items]
                conn.close()
                return self.send_json({"success": True, "order": od})

        # 5. Seller Products
        if path == '/api/seller/products':
            user = self.get_current_user()
            if not user or user['role'] != 'SELLER':
                conn.close()
                return self.send_json({"success": False, "message": "Seller access required."}, 403)
            
            rows = conn.execute("""
                SELECT p.product_id, p.product_name, p.description, p.price, p.category,
                       p.quantity, p.image, p.created_at,
                       ROUND(COALESCE(AVG(r.rating), 0), 1) AS rating,
                       COUNT(r.review_id) AS review_count
                FROM products p
                LEFT JOIN reviews r ON p.product_id = r.product_id
                WHERE p.seller_id = ?
                GROUP BY p.product_id
                ORDER BY p.product_id DESC
            """, (user['id'],)).fetchall()
            conn.close()
            return self.send_json({"success": True, "count": len(rows), "products": [dict(r) for r in rows]})

        # 6. Seller Orders
        if path == '/api/seller/orders':
            user = self.get_current_user()
            if not user or user['role'] != 'SELLER':
                conn.close()
                return self.send_json({"success": False, "message": "Seller access required."}, 403)
            
            rows = conn.execute("""
                SELECT oi.order_item_id, oi.order_id, oi.product_id, oi.quantity, oi.price_per_unit,
                       (oi.quantity * oi.price_per_unit) AS line_total,
                       p.product_name, p.image,
                       o.status AS order_status, o.shipping_address, o.created_at,
                       u.name AS buyer_name, u.email AS buyer_email
                FROM order_items oi
                JOIN orders o ON oi.order_id = o.order_id
                JOIN products p ON oi.product_id = p.product_id
                JOIN users u ON o.buyer_id = u.id
                WHERE oi.seller_id = ?
                ORDER BY oi.order_item_id DESC
            """, (user['id'],)).fetchall()
            orders = [dict(r) for r in rows]
            tot_rev = sum(r['line_total'] for r in orders)
            units = sum(r['quantity'] for r in orders)
            conn.close()
            return self.send_json({"success": True, "count": len(orders), "orders": orders, "total_revenue": tot_rev, "total_units_sold": units})

        # 7. Admin Stats
        if path == '/api/admin/stats':
            user = self.get_current_user()
            if not user or user['role'] != 'ADMIN':
                conn.close()
                return self.send_json({"success": False, "message": "Admin access required."}, 403)
            
            u = conn.execute("SELECT count(*) FROM users").fetchone()[0]
            p = conn.execute("SELECT count(*) FROM products").fetchone()[0]
            o_row = conn.execute("SELECT count(*), COALESCE(SUM(total_amount), 0) FROM orders").fetchone()
            conn.close()
            return self.send_json({"success": True, "stats": {
                "total_users": u, "total_products": p, "total_orders": o_row[0], "total_revenue": o_row[1]
            }})

        # 8. Admin Users
        if path == '/api/admin/users':
            user = self.get_current_user()
            if not user or user['role'] != 'ADMIN':
                conn.close()
                return self.send_json({"success": False, "message": "Admin access required."}, 403)
            users = conn.execute("SELECT id, name, email, role, created_at FROM users ORDER BY id ASC").fetchall()
            conn.close()
            return self.send_json({"success": True, "users": [dict(u) for u in users]})

        # 9. Admin Products
        if path == '/api/admin/products':
            user = self.get_current_user()
            if not user or user['role'] != 'ADMIN':
                conn.close()
                return self.send_json({"success": False, "message": "Admin access required."}, 403)
            rows = conn.execute("""
                SELECT p.product_id, p.seller_id, p.product_name, p.description, p.price,
                       p.category, p.quantity, p.image, p.created_at, u.name AS seller_name
                FROM products p
                JOIN users u ON p.seller_id = u.id
                ORDER BY p.product_id DESC
            """).fetchall()
            conn.close()
            return self.send_json({"success": True, "products": [dict(r) for r in rows]})

        # 10. Admin Orders
        if path == '/api/admin/orders':
            user = self.get_current_user()
            if not user or user['role'] != 'ADMIN':
                conn.close()
                return self.send_json({"success": False, "message": "Admin access required."}, 403)
            orders = conn.execute("""
                SELECT o.order_id, o.buyer_id, o.total_amount, o.status, o.shipping_address, o.created_at,
                       u.name AS buyer_name, u.email AS buyer_email
                FROM orders o
                JOIN users u ON o.buyer_id = u.id
                ORDER BY o.order_id DESC
            """).fetchall()
            ord_list = []
            for o in orders:
                od = dict(o)
                items = conn.execute("""
                    SELECT oi.order_item_id, oi.product_id, oi.quantity, oi.price_per_unit,
                           p.product_name, u.name AS seller_name
                    FROM order_items oi
                    JOIN products p ON oi.product_id = p.product_id
                    JOIN users u ON oi.seller_id = u.id
                    WHERE oi.order_id = ?
                """, (od['order_id'],)).fetchall()
                od['items'] = [dict(i) for i in items]
                ord_list.append(od)
            conn.close()
            return self.send_json({"success": True, "orders": ord_list})

        conn.close()
        return self.send_json({"success": False, "message": "Endpoint not found."}, 404)

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        body = self.read_json_body()
        conn = get_db()

        # Registration
        if path == '/api/register':
            name = body.get('name', '').strip()
            email = body.get('email', '').strip()
            password = body.get('password', '')
            role = body.get('role', '')

            if not name or not email or not password or not role:
                conn.close()
                return self.send_json({"success": False, "message": "All fields are required."}, 400)

            if role not in ('BUYER', 'SELLER'):
                conn.close()
                return self.send_json({"success": False, "message": "Invalid role."}, 400)

            exist = conn.execute("SELECT id FROM users WHERE LOWER(email) = LOWER(?)", (email,)).fetchone()
            if exist:
                conn.close()
                return self.send_json({"success": False, "message": "Email already exists."}, 409)

            pw_hash = sha256_hash(password)
            cursor = conn.cursor()
            cursor.execute("INSERT INTO users (name, email, password, role) VALUES (?, ?, ?, ?)", (name, email, pw_hash, role))
            new_id = cursor.lastrowid
            if role == 'BUYER':
                conn.execute("INSERT OR IGNORE INTO cart (buyer_id) VALUES (?)", (new_id,))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Registration successful! You can now log in.", "user": {"id": new_id, "name": name, "email": email, "role": role}}, 201)

        # Login
        if path == '/api/login':
            email = body.get('email', '').strip()
            password = body.get('password', '')
            pw_hash = sha256_hash(password)

            user = conn.execute("SELECT id, name, email, password, role FROM users WHERE LOWER(email) = LOWER(?)", (email,)).fetchone()
            if not user or user['password'] != pw_hash:
                conn.close()
                return self.send_json({"success": False, "message": "Invalid email or password."}, 401)

            res_user = {"id": user['id'], "name": user['name'], "email": user['email'], "role": user['role']}
            conn.close()
            return self.send_json({"success": True, "message": f"Welcome back, {user['name']}!", "user": res_user})

        # Logout
        if path == '/api/logout':
            conn.close()
            return self.send_json({"success": True, "message": "Logged out successfully."})

        # Add Product (Seller)
        if path == '/api/products':
            user = self.get_current_user()
            if not user or user['role'] != 'SELLER':
                conn.close()
                return self.send_json({"success": False, "message": "Seller access required."}, 403)
            
            name = body.get('product_name', '').strip()
            cat = body.get('category', 'General')
            price = float(body.get('price', 0))
            qty = int(body.get('quantity', 0))
            img = body.get('image', '').strip() or 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600'
            desc = body.get('description', '').strip()

            if not name or price <= 0 or qty < 0:
                conn.close()
                return self.send_json({"success": False, "message": "Invalid product parameters."}, 400)

            cursor = conn.cursor()
            cursor.execute("""
                INSERT INTO products (seller_id, product_name, description, price, category, quantity, image)
                VALUES (?, ?, ?, ?, ?, ?, ?)
            """, (user['id'], name, desc, price, cat, qty, img))
            conn.commit()
            pid = cursor.lastrowid
            conn.close()
            return self.send_json({"success": True, "message": "Product added successfully!", "product_id": pid}, 201)

        # Add to Cart (Buyer) - supports both /api/cart and /api/cart/add
        if path in ('/api/cart', '/api/cart/add'):
            user = self.get_current_user()
            if not user or user['role'] != 'BUYER':
                conn.close()
                return self.send_json({"success": False, "message": "Buyer access required."}, 401)

            pid = int(body.get('product_id', 0))
            qty = int(body.get('quantity', 1))

            prod = conn.execute("SELECT quantity, price FROM products WHERE product_id = ?", (pid,)).fetchone()
            if not prod:
                conn.close()
                return self.send_json({"success": False, "message": "Product not found."}, 404)

            stock = prod['quantity']
            if stock <= 0:
                conn.close()
                return self.send_json({"success": False, "message": "Product is out of stock."}, 400)

            cart = conn.execute("SELECT cart_id FROM cart WHERE buyer_id = ?", (user['id'],)).fetchone()
            cart_id = cart['cart_id'] if cart else None
            if not cart_id:
                c = conn.cursor()
                c.execute("INSERT INTO cart (buyer_id) VALUES (?)", (user['id'],))
                conn.commit()
                cart_id = c.lastrowid

            exist = conn.execute("SELECT item_id, quantity FROM cart_items WHERE cart_id = ? AND product_id = ?", (cart_id, pid)).fetchone()
            if exist:
                new_q = exist['quantity'] + qty
                if new_q > stock:
                    conn.close()
                    return self.send_json({"success": False, "message": f"Insufficient stock. Only {stock} available."}, 400)
                conn.execute("UPDATE cart_items SET quantity = ? WHERE item_id = ?", (new_q, exist['item_id']))
            else:
                if qty > stock:
                    conn.close()
                    return self.send_json({"success": False, "message": f"Insufficient stock. Only {stock} available."}, 400)
                conn.execute("INSERT INTO cart_items (cart_id, product_id, quantity) VALUES (?, ?, ?)", (cart_id, pid, qty))
            
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Product added to cart!"})

        # Checkout & Order Placement (Buyer)
        if path == '/api/orders':
            user = self.get_current_user()
            if not user or user['role'] != 'BUYER':
                conn.close()
                return self.send_json({"success": False, "message": "Buyer access required."}, 401)

            cart = conn.execute("SELECT cart_id FROM cart WHERE buyer_id = ?", (user['id'],)).fetchone()
            if not cart:
                conn.close()
                return self.send_json({"success": False, "message": "Cart not found."}, 400)

            items = conn.execute("""
                SELECT ci.product_id, ci.quantity, p.price, p.quantity AS stock, p.seller_id
                FROM cart_items ci
                JOIN products p ON ci.product_id = p.product_id
                WHERE ci.cart_id = ?
            """, (cart['cart_id'],)).fetchall()

            if not items:
                conn.close()
                return self.send_json({"success": False, "message": "Cart is empty."}, 400)

            # Stock check
            total = 0.0
            for it in items:
                if it['quantity'] > it['stock']:
                    conn.close()
                    return self.send_json({"success": False, "message": f"Stock boundary exceeded for Product #{it['product_id']}."}, 400)
                total += it['quantity'] * it['price']

            addr = body.get('shipping_address', 'Default Delivery Address').strip()
            cursor = conn.cursor()
            cursor.execute("INSERT INTO orders (buyer_id, total_amount, status, shipping_address) VALUES (?, ?, 'Pending', ?)",
                           (user['id'], total, addr))
            oid = cursor.lastrowid

            for it in items:
                cursor.execute("""
                    INSERT INTO order_items (order_id, product_id, seller_id, quantity, price_per_unit)
                    VALUES (?, ?, ?, ?, ?)
                """, (oid, it['product_id'], it['seller_id'], it['quantity'], it['price']))
                cursor.execute("UPDATE products SET quantity = quantity - ? WHERE product_id = ?",
                               (it['quantity'], it['product_id']))

            cursor.execute("DELETE FROM cart_items WHERE cart_id = ?", (cart['cart_id'],))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Order Successful!", "order_id": oid, "total_amount": total}, 201)

        # Reviews
        if path == '/api/reviews':
            user = self.get_current_user()
            if not user or user['role'] != 'BUYER':
                conn.close()
                return self.send_json({"success": False, "message": "Buyer access required."}, 401)

            pid = int(body.get('product_id', 0))
            rating = int(body.get('rating', 5))
            comment = body.get('comment', '').strip()

            if rating < 1 or rating > 5:
                conn.close()
                return self.send_json({"success": False, "message": "Rating must be between 1 and 5."}, 400)

            exist = conn.execute("SELECT review_id FROM reviews WHERE product_id = ? AND buyer_id = ?", (pid, user['id'])).fetchone()
            if exist:
                conn.execute("UPDATE reviews SET rating = ?, comment = ?, created_at = CURRENT_TIMESTAMP WHERE review_id = ?",
                             (rating, comment, exist['review_id']))
            else:
                conn.execute("INSERT INTO reviews (product_id, buyer_id, rating, comment) VALUES (?, ?, ?, ?)",
                             (pid, user['id'], rating, comment))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Review submitted successfully! Thank you for your feedback."})

        # Rule-Based Chatbot
        if path == '/api/chatbot':
            msg = body.get('message', '').lower()
            if "search" in msg or "find" in msg:
                reply = "Open the Products page and use the search bar to find products by name."
            elif "add" in msg and "cart" in msg:
                reply = "Click '+ Add to Cart' on any product card, or open product details to pick your quantity."
            elif "checkout" in msg or "place order" in msg:
                reply = "Open your Cart and click 'Proceed to Checkout'. Enter your address and click 'Confirm Order'."
            elif "order" in msg or "history" in msg or "track" in msg:
                reply = "Open the Orders page from the top navigation to see past orders and delivery statuses."
            elif "categor" in msg:
                reply = "Monica Mart has 5 categories: Electronics, Fashion, Home, Beauty, and Accessories."
            elif "review" in msg or "rating" in msg or "star" in msg:
                reply = "Navigate to any product details page, scroll down to reviews, select 1-5 stars and submit your feedback."
            elif "remove" in msg or ("delete" in msg and "cart" in msg):
                reply = "In your cart, click '✕ Remove' or reduce the quantity to zero to remove the item."
            elif "register" in msg or "sign up" in msg or "seller" in msg:
                reply = "Click 'Register' in the top bar, pick 'Buyer' or 'Seller', and create your account!"
            else:
                reply = "Hello! I am Monica Mart's shopping assistant. Ask me how to search, add to cart, checkout, or track orders!"
            
            conn.close()
            return self.send_json({"success": True, "reply": reply, "response": reply})

        conn.close()
        return self.send_json({"success": False, "message": "Endpoint not found."}, 404)

    def do_PUT(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        body = self.read_json_body()
        conn = get_db()

        # Update Order Status (Seller or Admin)
        if path.startswith('/api/orders/') and path.endswith('/status'):
            parts = path.strip('/').split('/')
            oid = parts[2]
            user = self.get_current_user()
            if not user or user['role'] not in ('SELLER', 'ADMIN'):
                conn.close()
                return self.send_json({"success": False, "message": "Seller or Admin access required."}, 403)
            
            new_status = body.get('status', '').strip()
            valid = ('Pending', 'Confirmed', 'Processing', 'Shipped', 'Delivered', 'Cancelled')
            if new_status not in valid:
                conn.close()
                return self.send_json({"success": False, "message": f"Invalid status. Must be one of {valid}."}, 400)

            conn.execute("UPDATE orders SET status = ? WHERE order_id = ?", (new_status, oid))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": f"Order #{oid} status updated to {new_status}."})

        # Update Product (Seller)
        if path.startswith('/api/products/'):
            user = self.get_current_user()
            pid = path.split('/')[-1]
            if not user or user['role'] != 'SELLER':
                conn.close()
                return self.send_json({"success": False, "message": "Unauthorized."}, 403)

            exist = conn.execute("SELECT seller_id FROM products WHERE product_id = ?", (pid,)).fetchone()
            if not exist or exist['seller_id'] != user['id']:
                conn.close()
                return self.send_json({"success": False, "message": "Unauthorized to edit this product."}, 403)

            name = body.get('product_name')
            desc = body.get('description')
            price = body.get('price')
            cat = body.get('category')
            qty = body.get('quantity')
            img = body.get('image')

            conn.execute("""
                UPDATE products SET product_name = ?, description = ?, price = ?, category = ?, quantity = ?, image = ?
                WHERE product_id = ? AND seller_id = ?
            """, (name, desc, price, cat, qty, img, pid, user['id']))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Product updated successfully!"})

        # Update Cart Item Quantity - supports /api/cart/<id> and /api/cart/update/<id>
        if path.startswith('/api/cart/'):
            user = self.get_current_user()
            parts = path.strip('/').split('/')
            item_id = parts[-1]
            if not user or user['role'] != 'BUYER':
                conn.close()
                return self.send_json({"success": False, "message": "Buyer access required."}, 401)

            new_qty = int(body.get('quantity', 1))
            if new_qty <= 0:
                conn.execute("DELETE FROM cart_items WHERE item_id = ?", (item_id,))
                conn.commit()
                conn.close()
                return self.send_json({"success": True, "message": "Item removed from cart."})

            row = conn.execute("SELECT ci.quantity, p.quantity AS stock FROM cart_items ci JOIN products p ON ci.product_id = p.product_id WHERE ci.item_id = ?", (item_id,)).fetchone()
            if not row or new_qty > row['stock']:
                conn.close()
                return self.send_json({"success": False, "message": "Stock boundary exceeded."}, 400)

            conn.execute("UPDATE cart_items SET quantity = ? WHERE item_id = ?", (new_qty, item_id))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Cart updated successfully."})

        conn.close()
        return self.send_json({"success": False, "message": "Endpoint not found."}, 404)

    def do_DELETE(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        conn = get_db()
        user = self.get_current_user()

        # Delete from Cart - supports /api/cart/<id> and /api/cart/remove/<id>
        if path.startswith('/api/cart/'):
            parts = path.strip('/').split('/')
            item_id = parts[-1]
            conn.execute("DELETE FROM cart_items WHERE item_id = ?", (item_id,))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Item removed from cart."})

        # Seller Delete Product
        if path.startswith('/api/products/'):
            pid = path.split('/')[-1]
            if not user:
                conn.close()
                return self.send_json({"success": False, "message": "Unauthorized."}, 401)

            if user['role'] == 'SELLER':
                conn.execute("DELETE FROM products WHERE product_id = ? AND seller_id = ?", (pid, user['id']))
            elif user['role'] == 'ADMIN':
                conn.execute("DELETE FROM products WHERE product_id = ?", (pid,))
            else:
                conn.close()
                return self.send_json({"success": False, "message": "Forbidden."}, 403)

            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Product deleted successfully."})

        # Admin Delete Inappropriate Product
        if path.startswith('/api/admin/products/'):
            pid = path.split('/')[-1]
            if not user or user['role'] != 'ADMIN':
                conn.close()
                return self.send_json({"success": False, "message": "Admin privileges required."}, 403)

            conn.execute("DELETE FROM products WHERE product_id = ?", (pid,))
            conn.commit()
            conn.close()
            return self.send_json({"success": True, "message": "Product removed successfully by administrator."})

        conn.close()
        return self.send_json({"success": False, "message": "Endpoint not found."}, 404)

def run():
    init_db()
    with socketserver.TCPServer(("", PORT), MonicaMartHandler) as httpd:
        print("=" * 65)
        print("             MONICA MART — SERVER RUNNING")
        print(f"             URL: http://localhost:{PORT}")
        print("=" * 65)
        print("Press Ctrl+C to stop the server.")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")

if __name__ == '__main__':
    run()
