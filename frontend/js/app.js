/**
 * MONICA MART - Main Application JavaScript
 * Full-Stack Client Controller & Utilities
 * Author: Monica Sornam (College Capstone Project)
 */

const API_BASE = '/api';

// --- State Management ---
const AppState = {
    getUser() {
        try {
            const userStr = localStorage.getItem('monica_mart_user');
            return userStr ? JSON.parse(userStr) : null;
        } catch (e) {
            return null;
        }
    },
    setUser(user) {
        if (user) {
            localStorage.setItem('monica_mart_user', JSON.stringify(user));
        } else {
            localStorage.removeItem('monica_mart_user');
        }
    },
    isLoggedIn() {
        return !!this.getUser();
    },
    getRole() {
        const user = this.getUser();
        return user ? user.role : null;
    }
};

// --- HTTP Request Wrapper ---
async function apiRequest(endpoint, method = 'GET', data = null) {
    const headers = {
        'Content-Type': 'application/json'
    };

    const user = AppState.getUser();
    if (user && user.id) {
        headers['X-User-Id'] = user.id.toString();
    }

    const options = {
        method,
        headers,
        credentials: 'same-origin'
    };

    if (data && (method === 'POST' || method === 'PUT')) {
        options.body = JSON.stringify(data);
    }

    try {
        const response = await fetch(API_BASE + endpoint, options);
        const result = await response.json();
        return { ok: response.ok, status: response.status, data: result };
    } catch (err) {
        console.error('API Request error:', err);
        return { ok: false, status: 500, data: { success: false, message: 'Network or server error. Please try again.' } };
    }
}

// --- Toast Notifications ---
function showToast(message, type = 'success') {
    let container = document.getElementById('toast-container');
    if (!container) {
        container = document.createElement('div');
        container.id = 'toast-container';
        document.body.appendChild(container);
    }

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `<span>${message}</span>`;
    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(100%)';
        toast.style.transition = 'all 0.3s ease';
        setTimeout(() => toast.remove(), 300);
    }, 3500);
}

// --- Currency Formatter ---
function formatPrice(amount) {
    return '₹' + Number(amount).toLocaleString('en-IN', { minimumFractionDigits: 0, maximumFractionDigits: 2 });
}

// --- Dynamic Navigation & Auth Sync ---
async function updateNavigation() {
    const nav = document.getElementById('main-nav');
    if (!nav) return;

    const user = AppState.getUser();
    const currentPath = window.location.pathname;

    let navHtml = `
        <li><a href="index.html" class="${currentPath.endsWith('index.html') || currentPath === '/' ? 'active' : ''}">Home</a></li>
        <li><a href="products.html" class="${currentPath.endsWith('products.html') ? 'active' : ''}">Products</a></li>
    `;

    if (user) {
        if (user.role === 'BUYER') {
            navHtml += `
                <li><a href="cart.html" class="${currentPath.endsWith('cart.html') ? 'active' : ''}">Cart <span id="nav-cart-badge" class="cart-nav-badge">0</span></a></li>
                <li><a href="orders.html" class="${currentPath.endsWith('orders.html') ? 'active' : ''}">Orders</a></li>
                <li><span class="user-menu-btn">👤 ${user.name}</span></li>
                <li><a href="javascript:void(0)" onclick="handleLogout()" class="btn btn-sm btn-outline">Logout</a></li>
            `;
            refreshCartCount();
        } else if (user.role === 'SELLER') {
            navHtml += `
                <li><a href="seller-dashboard.html" class="${currentPath.endsWith('seller-dashboard.html') ? 'active' : ''}">Dashboard</a></li>
                <li><a href="add-product.html" class="${currentPath.endsWith('add-product.html') ? 'active' : ''}">+ Add Product</a></li>
                <li><span class="user-menu-btn">🏪 ${user.name} (Seller)</span></li>
                <li><a href="javascript:void(0)" onclick="handleLogout()" class="btn btn-sm btn-outline">Logout</a></li>
            `;
        } else if (user.role === 'ADMIN') {
            navHtml += `
                <li><a href="admin-dashboard.html" class="${currentPath.endsWith('admin-dashboard.html') ? 'active' : ''}">Admin Panel</a></li>
                <li><span class="user-menu-btn">🛡️ ${user.name}</span></li>
                <li><a href="javascript:void(0)" onclick="handleLogout()" class="btn btn-sm btn-outline">Logout</a></li>
            `;
        }
    } else {
        navHtml += `
            <li><a href="login.html" class="${currentPath.endsWith('login.html') ? 'active' : ''}">Login</a></li>
            <li><a href="register.html" class="btn btn-sm btn-primary ${currentPath.endsWith('register.html') ? 'active' : ''}">Register</a></li>
        `;
    }

    nav.innerHTML = navHtml;
}

// --- Refresh Cart Item Count for Buyer ---
async function refreshCartCount() {
    const user = AppState.getUser();
    if (!user || user.role !== 'BUYER') return;

    const res = await apiRequest('/cart');
    if (res.ok && res.data.success) {
        const badge = document.getElementById('nav-cart-badge');
        if (badge) {
            badge.textContent = res.data.total_items || 0;
        }
    }
}

// --- Global Logout Handler ---
async function handleLogout() {
    await apiRequest('/logout', 'POST');
    AppState.setUser(null);
    showToast('Logged out successfully.', 'info');
    setTimeout(() => {
        window.location.href = 'login.html';
    }, 600);
}

// --- Quick Add To Cart from Catalog ---
async function quickAddToCart(productId) {
    const user = AppState.getUser();
    if (!user) {
        showToast('Please log in to add items to your cart.', 'warning');
        setTimeout(() => window.location.href = 'login.html', 1200);
        return;
    }
    if (user.role !== 'BUYER') {
        showToast('Only buyer accounts can add items to the cart.', 'warning');
        return;
    }

    const res = await apiRequest('/cart', 'POST', { product_id: productId, quantity: 1 });
    if (res.ok && res.data.success) {
        showToast(res.data.message || 'Product added to cart!', 'success');
        refreshCartCount();
    } else {
        showToast(res.data.message || 'Failed to add product to cart.', 'error');
    }
}

// --- Floating AI Chatbot Initialization ---
function initChatbot() {
    if (document.getElementById('chatbot-bubble')) return;

    // 1. Create Bubble
    const bubble = document.createElement('button');
    bubble.id = 'chatbot-bubble';
    bubble.className = 'chatbot-bubble';
    bubble.innerHTML = '💬';
    bubble.title = 'Ask Monica Mart Assistant';

    // 2. Create Window
    const win = document.createElement('div');
    win.id = 'chatbot-window';
    win.className = 'chatbot-window hidden';
    win.innerHTML = `
        <div class="chatbot-header">
            <span>✨ Monica Mart Assistant</span>
            <button class="chatbot-close-btn" onclick="toggleChatbot()">✕</button>
        </div>
        <div class="chatbot-messages" id="chatbot-messages">
            <div class="chat-msg bot">
                Hello! 👋 I am your Monica Mart shopping assistant. How can I help you today?
            </div>
        </div>
        <div class="chatbot-suggestions">
            <span class="chat-chip" onclick="sendQuickChat('How can I register?')">Register</span>
            <span class="chat-chip" onclick="sendQuickChat('How can I add a product?')">Add Product</span>
            <span class="chat-chip" onclick="sendQuickChat('How can I checkout?')">Checkout</span>
            <span class="chat-chip" onclick="sendQuickChat('Where can I see my orders?')">Orders</span>
        </div>
        <div class="chatbot-input-row">
            <input type="text" id="chatbot-input" class="chatbot-input" placeholder="Ask a question..." onkeydown="if(event.key==='Enter') sendChatMessage()">
            <button class="chatbot-send-btn" onclick="sendChatMessage()">➤</button>
        </div>
    `;

    document.body.appendChild(bubble);
    document.body.appendChild(win);

    bubble.onclick = toggleChatbot;
}

function toggleChatbot() {
    const win = document.getElementById('chatbot-window');
    if (!win) return;
    win.classList.toggle('hidden');
    if (!win.classList.contains('hidden')) {
        document.getElementById('chatbot-input').focus();
    }
}

function sendQuickChat(text) {
    const input = document.getElementById('chatbot-input');
    input.value = text;
    sendChatMessage();
}

async function sendChatMessage() {
    const input = document.getElementById('chatbot-input');
    const msg = input.value.trim();
    if (!msg) return;

    const msgContainer = document.getElementById('chatbot-messages');

    // Add user message
    const userDiv = document.createElement('div');
    userDiv.className = 'chat-msg user';
    userDiv.textContent = msg;
    msgContainer.appendChild(userDiv);
    input.value = '';
    msgContainer.scrollTop = msgContainer.scrollHeight;

    // Send to backend
    const res = await apiRequest('/chatbot', 'POST', { message: msg });
    const botDiv = document.createElement('div');
    botDiv.className = 'chat-msg bot';
    if (res.ok && res.data.success) {
        botDiv.textContent = res.data.reply;
    } else {
        botDiv.textContent = res.data.reply || "Sorry, I am having trouble connecting to the server. Please try again.";
    }
    msgContainer.appendChild(botDiv);
    msgContainer.scrollTop = msgContainer.scrollHeight;
}

// --- Initialize Global Features on DOM Ready ---
document.addEventListener('DOMContentLoaded', () => {
    updateNavigation();
    initChatbot();

    // Mobile nav toggle
    const toggleBtn = document.getElementById('mobile-nav-toggle');
    if (toggleBtn) {
        toggleBtn.addEventListener('click', () => {
            const nav = document.getElementById('main-nav');
            if (nav) nav.classList.toggle('show');
        });
    }
});
