/**
 * MONICA MART — Main JavaScript Utilities & Shared State
 * Developer: Monica Sornam (College Capstone Project)
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
    },
    logout() {
        this.setUser(null);
        showToast('You have been logged out successfully.', 'success');
        setTimeout(() => {
            window.location.href = 'login.html';
        }, 600);
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
        return { ok: false, status: 500, data: { success: false, message: 'Server communication error.' } };
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

// --- Dynamic Navigation by Role ---
async function updateNavigation() {
    const nav = document.getElementById('main-nav');
    if (!nav) return;

    const user = AppState.getUser();
    const currentPath = window.location.pathname.toLowerCase();

    let linksHtml = '';

    if (!user) {
        // GUEST
        linksHtml = `
            <li><a href="index.html" class="${currentPath.endsWith('index.html') || currentPath.endsWith('/') ? 'active' : ''}">Home</a></li>
            <li><a href="products.html" class="${currentPath.endsWith('products.html') ? 'active' : ''}">Products</a></li>
            <li><a href="login.html" class="${currentPath.endsWith('login.html') ? 'active' : ''}">Login</a></li>
            <li><a href="register.html" class="btn btn-sm btn-primary" style="color: #fff;">Register</a></li>
        `;
    } else if (user.role === 'BUYER') {
        // BUYER
        linksHtml = `
            <li><a href="index.html" class="${currentPath.endsWith('index.html') || currentPath.endsWith('/') ? 'active' : ''}">Home</a></li>
            <li><a href="products.html" class="${currentPath.endsWith('products.html') || currentPath.endsWith('product.html') ? 'active' : ''}">Products</a></li>
            <li><a href="cart.html" class="${currentPath.endsWith('cart.html') ? 'active' : ''}">
                Cart <span class="nav-badge" id="nav-cart-count">0</span>
            </a></li>
            <li><a href="orders.html" class="${currentPath.endsWith('orders.html') ? 'active' : ''}">Orders</a></li>
            <li>
                <span class="nav-user-tag">👤 ${escapeHtml(user.name)}</span>
            </li>
            <li><a href="#" onclick="AppState.logout(); return false;" class="btn btn-sm btn-outline">Logout</a></li>
        `;
    } else if (user.role === 'SELLER') {
        // SELLER
        linksHtml = `
            <li><a href="seller.html" class="${currentPath.endsWith('seller.html') || currentPath.endsWith('seller-dashboard.html') ? 'active' : ''}">Dashboard</a></li>
            <li><a href="products.html" class="${currentPath.endsWith('products.html') ? 'active' : ''}">Products</a></li>
            <li><a href="seller.html#seller-orders-tab" class="">Orders</a></li>
            <li>
                <span class="nav-user-tag">🏪 ${escapeHtml(user.name)}</span>
            </li>
            <li><a href="#" onclick="AppState.logout(); return false;" class="btn btn-sm btn-outline">Logout</a></li>
        `;
    } else if (user.role === 'ADMIN') {
        // ADMIN
        linksHtml = `
            <li><a href="admin.html" class="${currentPath.endsWith('admin.html') || currentPath.endsWith('admin-dashboard.html') ? 'active' : ''}">Admin Dashboard</a></li>
            <li><a href="products.html" class="${currentPath.endsWith('products.html') ? 'active' : ''}">Marketplace</a></li>
            <li>
                <span class="nav-user-tag">🛡️ Admin</span>
            </li>
            <li><a href="#" onclick="AppState.logout(); return false;" class="btn btn-sm btn-outline">Logout</a></li>
        `;
    }

    nav.innerHTML = linksHtml;

    // Mobile nav toggle
    const toggle = document.getElementById('mobile-nav-toggle');
    if (toggle) {
        toggle.onclick = () => {
            nav.classList.toggle('active');
        };
    }

    // Refresh cart badge if buyer
    if (user && user.role === 'BUYER') {
        updateCartBadge();
    }
}

// --- Cart Badge Count ---
async function updateCartBadge() {
    const badge = document.getElementById('nav-cart-count');
    if (!badge) return;
    const res = await apiRequest('/cart');
    if (res.ok && res.data.success) {
        badge.textContent = res.data.total_items || 0;
    }
}

// --- HTML Sanitizer Helper ---
function escapeHtml(str) {
    if (!str) return '';
    return String(str)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#039;');
}

document.addEventListener('DOMContentLoaded', () => {
    updateNavigation();
});
