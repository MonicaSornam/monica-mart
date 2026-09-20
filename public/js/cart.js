/**
 * MONICA MART — Shopping Cart Management
 * Developer: Monica Sornam (College Capstone Project)
 */

document.addEventListener('DOMContentLoaded', () => {
    if (document.getElementById('cart-container')) {
        initCartPage();
    }
});

async function initCartPage() {
    const user = AppState.getUser();
    if (!user) {
        showToast('Please log in as a buyer to access your shopping cart.', 'warning');
        setTimeout(() => window.location.href = 'login.html', 1000);
        return;
    }
    if (user.role !== 'BUYER') {
        showToast('Only buyer accounts have a shopping cart.', 'error');
        setTimeout(() => window.location.href = 'products.html', 1200);
        return;
    }

    await loadCartData();
}

async function loadCartData() {
    const container = document.getElementById('cart-container');
    if (!container) return;

    container.innerHTML = `
        <div style="text-align: center; padding: 50px;">
            <p class="text-muted">Loading your shopping cart...</p>
        </div>
    `;

    const res = await apiRequest('/cart');
    if (!res.ok || !res.data.success) {
        container.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-icon">🛍️</div>
                <h3>Failed to load cart</h3>
                <p>There was an issue fetching your cart. Please try again.</p>
                <button class="btn btn-peach" onclick="loadCartData()">Retry</button>
            </div>
        `;
        return;
    }

    const cart = res.data;
    const items = cart.items || [];

    updateCartBadge();

    if (items.length === 0) {
        container.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-icon">🛒</div>
                <h3>Your cart is empty</h3>
                <p>Looks like you haven't added anything to your cart yet. Explore our curated pastel collections!</p>
                <a href="products.html" class="btn btn-peach">Shop Now 🛍️</a>
            </div>
        `;
        return;
    }

    // Render Cart Layout (Items Table + Summary Card)
    container.innerHTML = `
        <div class="cart-layout">
            <div class="data-card">
                <h3 style="font-size: 1.3rem; color: var(--lavender-dark); margin-bottom: 18px;">
                    Cart Items (${cart.total_items} items)
                </h3>
                <div class="table-responsive">
                    <table class="custom-table">
                        <thead>
                            <tr>
                                <th>Product</th>
                                <th>Price</th>
                                <th style="text-align: center;">Quantity</th>
                                <th>Subtotal</th>
                                <th>Action</th>
                            </tr>
                        </thead>
                        <tbody>
                            ${items.map(item => createCartRowHtml(item)).join('')}
                        </tbody>
                    </table>
                </div>
            </div>

            <div class="cart-summary-card">
                <h3>Order Summary</h3>
                <div class="summary-row">
                    <span>Total Items:</span>
                    <strong>${cart.total_items}</strong>
                </div>
                <div class="summary-row">
                    <span>Subtotal:</span>
                    <span>${formatPrice(cart.total_amount)}</span>
                </div>
                <div class="summary-row">
                    <span>Estimated Shipping:</span>
                    <span style="color: var(--status-delivered); font-weight: 700;">FREE</span>
                </div>
                <div class="summary-row summary-total">
                    <span>Total Amount:</span>
                    <span>${formatPrice(cart.total_amount)}</span>
                </div>
                <button class="btn btn-primary btn-lg" style="width: 100%; margin-top: 24px;" onclick="window.location.href='checkout.html'">
                    Proceed to Checkout 💳
                </button>
                <a href="products.html" class="btn btn-outline" style="width: 100%; margin-top: 10px;">
                    Continue Shopping
                </a>
            </div>
        </div>
    `;
}

function createCartRowHtml(item) {
    const placeholderImg = 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80';
    const imageSrc = item.image ? escapeHtml(item.image) : placeholderImg;

    return `
        <tr>
            <td>
                <div style="display: flex; align-items: center; gap: 14px;">
                    <img src="${imageSrc}" alt="${escapeHtml(item.product_name)}" style="width: 54px; height: 54px; object-fit: cover; border-radius: var(--radius-sm); border: 1px solid var(--border);" onerror="this.src='${placeholderImg}'">
                    <div>
                        <a href="product.html?id=${item.product_id}" class="bold" style="color: var(--text-main);">
                            ${escapeHtml(item.product_name)}
                        </a>
                        <div style="font-size: 0.8rem; color: var(--text-muted);">${escapeHtml(item.category || '')}</div>
                    </div>
                </div>
            </td>
            <td>${formatPrice(item.price)}</td>
            <td style="text-align: center;">
                <div class="qty-control" style="margin: 0 auto;">
                    <button class="qty-btn" onclick="updateCartItemQty(${item.item_id}, ${item.quantity - 1}, ${item.stock})">−</button>
                    <span class="qty-display">${item.quantity}</span>
                    <button class="qty-btn" onclick="updateCartItemQty(${item.item_id}, ${item.quantity + 1}, ${item.stock})">+</button>
                </div>
                <div style="font-size: 0.72rem; color: var(--text-light); margin-top: 4px;">Max: ${item.stock}</div>
            </td>
            <td class="bold" style="color: var(--lavender-dark);">${formatPrice(item.subtotal)}</td>
            <td>
                <button class="btn btn-sm btn-outline" style="color: var(--status-cancelled); border-color: var(--status-cancelled);" onclick="removeCartItem(${item.item_id})">
                    ✕ Remove
                </button>
            </td>
        </tr>
    `;
}

async function updateCartItemQty(itemId, newQty, stock) {
    if (newQty <= 0) {
        await removeCartItem(itemId);
        return;
    }

    if (newQty > stock) {
        showToast(`Cannot add more than warehouse stock (${stock} available).`, 'warning');
        return;
    }

    const res = await apiRequest(`/cart/${itemId}`, 'PUT', { quantity: newQty });
    if (res.ok && res.data.success) {
        showToast('Cart updated.', 'success');
        loadCartData();
    } else {
        showToast(res.data.message || 'Failed to update quantity.', 'error');
    }
}

async function removeCartItem(itemId) {
    if (!confirm('Are you sure you want to remove this item from your cart?')) return;

    const res = await apiRequest(`/cart/${itemId}`, 'DELETE');
    if (res.ok && res.data.success) {
        showToast('Item removed from cart.', 'success');
        loadCartData();
    } else {
        showToast(res.data.message || 'Failed to remove item.', 'error');
    }
}
