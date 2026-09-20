/**
 * MONICA MART — Orders & Checkout Controller
 * Developer: Monica Sornam (College Capstone Project)
 */

document.addEventListener('DOMContentLoaded', () => {
    // 1. If on Checkout Page
    if (document.getElementById('checkout-container')) {
        initCheckoutPage();
    }

    // 2. If on Orders History Page
    if (document.getElementById('orders-container')) {
        initOrdersPage();
    }
});

// --- Checkout Page Controller ---
async function initCheckoutPage() {
    const user = AppState.getUser();
    if (!user) {
        showToast('Please log in as a buyer to complete checkout.', 'warning');
        setTimeout(() => window.location.href = 'login.html', 1000);
        return;
    }
    if (user.role !== 'BUYER') {
        showToast('Only buyer accounts can place orders.', 'error');
        setTimeout(() => window.location.href = 'products.html', 1200);
        return;
    }

    // Populate user details in checkout form
    const buyerNameEl = document.getElementById('checkout-buyer-name');
    const buyerEmailEl = document.getElementById('checkout-buyer-email');
    if (buyerNameEl) buyerNameEl.value = user.name;
    if (buyerEmailEl) buyerEmailEl.value = user.email;

    // Load Cart Items into Checkout Summary
    const cartRes = await apiRequest('/cart');
    if (!cartRes.ok || !cartRes.data.success || (cartRes.data.items || []).length === 0) {
        showToast('Your cart is empty. Add products before checking out.', 'warning');
        setTimeout(() => window.location.href = 'products.html', 1200);
        return;
    }

    const cart = cartRes.data;
    const items = cart.items || [];
    const summaryList = document.getElementById('checkout-items-list');
    const subtotalEl = document.getElementById('checkout-subtotal');
    const totalEl = document.getElementById('checkout-total');

    if (summaryList) {
        summaryList.innerHTML = items.map(item => `
            <div style="display: flex; justify-content: space-between; margin-bottom: 12px; font-size: 0.92rem;">
                <div>
                    <strong>${escapeHtml(item.product_name)}</strong>
                    <div style="color: var(--text-muted); font-size: 0.8rem;">Qty: ${item.quantity} × ${formatPrice(item.price)}</div>
                </div>
                <div class="bold" style="color: var(--lavender-dark);">${formatPrice(item.subtotal)}</div>
            </div>
        `).join('');
    }

    if (subtotalEl) subtotalEl.textContent = formatPrice(cart.total_amount);
    if (totalEl) totalEl.textContent = formatPrice(cart.total_amount);

    // Setup Confirm Order Button Listener
    const checkoutForm = document.getElementById('checkout-form');
    if (checkoutForm) {
        checkoutForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const confirmBtn = document.getElementById('confirm-order-btn');
            confirmBtn.disabled = true;
            confirmBtn.textContent = 'Processing Order...';

            const shippingAddress = document.getElementById('checkout-address').value.trim();
            if (!shippingAddress || shippingAddress.length < 5) {
                showToast('Please provide a complete shipping address.', 'error');
                confirmBtn.disabled = false;
                confirmBtn.textContent = 'Confirm Order 💳';
                return;
            }

            // Post Order
            const orderRes = await apiRequest('/orders', 'POST', {
                shipping_address: shippingAddress
            });

            if (orderRes.ok && orderRes.data.success) {
                const orderId = orderRes.data.order_id;
                showOrderSuccessModal(orderId, orderRes.data.total_amount);
                updateCartBadge();
            } else {
                showToast(orderRes.data.message || 'Order confirmation failed. Please check stock.', 'error');
                confirmBtn.disabled = false;
                confirmBtn.textContent = 'Confirm Order 💳';
            }
        });
    }
}

// --- Order Success Confirmation Display ---
function showOrderSuccessModal(orderId, totalAmount) {
    const container = document.getElementById('checkout-container');
    if (!container) return;

    container.innerHTML = `
        <div class="empty-state" style="background: var(--surface); border: 2px solid var(--status-delivered); padding: 50px 30px;">
            <div class="empty-state-icon" style="color: var(--status-delivered); font-size: 4rem;">🎉</div>
            <h2 style="font-size: 2rem; color: var(--lavender-dark); margin-bottom: 10px;">Order Successful!</h2>
            <p style="font-size: 1.1rem; color: var(--text-main); margin-bottom: 6px;">
                Thank you for your purchase at <strong>MONICA MART</strong>!
            </p>
            <div style="background: var(--lavender-subtle); padding: 14px 24px; border-radius: var(--radius); display: inline-block; margin: 16px 0; border: 1px solid var(--lavender);">
                <span style="font-size: 0.9rem; color: var(--text-muted);">Your Order ID:</span>
                <div style="font-size: 1.6rem; font-weight: 800; color: var(--lavender-dark);">#MM-${orderId}</div>
                <div style="font-size: 0.95rem; font-weight: 700; color: var(--text-main); margin-top: 4px;">Total Paid: ${formatPrice(totalAmount)}</div>
            </div>
            <p class="text-muted" style="max-width: 500px; margin: 0 auto 26px;">
                A confirmation has been saved to your account. You can track your order status in real time.
            </p>
            <div style="display: flex; gap: 14px; justify-content: center; flex-wrap: wrap;">
                <a href="orders.html" class="btn btn-primary btn-lg">View Order History 📦</a>
                <a href="products.html" class="btn btn-peach btn-lg">Continue Shopping 🛍️</a>
            </div>
        </div>
    `;
}

// --- Order History Page Controller ---
async function initOrdersPage() {
    const user = AppState.getUser();
    if (!user) {
        showToast('Please log in as a buyer to view your orders.', 'warning');
        setTimeout(() => window.location.href = 'login.html', 1000);
        return;
    }
    if (user.role !== 'BUYER') {
        showToast('Only buyer accounts have personal order histories.', 'error');
        setTimeout(() => window.location.href = 'products.html', 1200);
        return;
    }

    const container = document.getElementById('orders-container');
    if (!container) return;

    container.innerHTML = `
        <div style="text-align: center; padding: 40px;">
            <p class="text-muted">Loading your past orders...</p>
        </div>
    `;

    const res = await apiRequest('/orders');
    if (!res.ok || !res.data.success) {
        container.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-icon">📦</div>
                <h3>Failed to load orders</h3>
                <p>Could not retrieve your orders. Please refresh the page.</p>
                <button class="btn btn-peach" onclick="initOrdersPage()">Retry</button>
            </div>
        `;
        return;
    }

    const orders = res.data.orders || [];

    if (orders.length === 0) {
        container.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-icon">📦</div>
                <h3>No Orders Placed Yet</h3>
                <p>You have not placed any orders yet. Discover our curated collection and place your first order!</p>
                <a href="products.html" class="btn btn-peach">Start Shopping 🛍️</a>
            </div>
        `;
        return;
    }

    container.innerHTML = `
        <div style="display: flex; flex-direction: column; gap: 24px;">
            ${orders.map(order => createOrderCardHtml(order)).join('')}
        </div>
    `;
}

function createOrderCardHtml(order) {
    const statusClass = `badge-${(order.status || 'pending').toLowerCase()}`;
    const dateFormatted = order.created_at ? new Date(order.created_at).toLocaleString() : 'Recent';
    const items = order.items || [];

    return `
        <div class="data-card">
            <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 12px; margin-bottom: 16px; padding-bottom: 14px; border-bottom: 1.5px solid var(--border);">
                <div>
                    <span style="font-size: 0.85rem; color: var(--text-muted);">Order ID:</span>
                    <strong style="font-size: 1.15rem; color: var(--lavender-dark); margin-left: 6px;">#MM-${order.order_id}</strong>
                    <span style="margin: 0 10px; color: var(--border);">|</span>
                    <span style="font-size: 0.85rem; color: var(--text-muted);">${dateFormatted}</span>
                </div>
                <div>
                    <span class="badge ${statusClass}" style="font-size: 0.85rem; padding: 5px 14px;">${order.status}</span>
                </div>
            </div>

            <div style="display: flex; flex-direction: column; gap: 12px; margin-bottom: 18px;">
                ${items.map(item => `
                    <div style="display: flex; justify-content: space-between; align-items: center; background: #FAF7F9; padding: 10px 14px; border-radius: var(--radius-sm); border: 1px solid var(--border);">
                        <div>
                            <strong>${escapeHtml(item.product_name)}</strong>
                            <div style="font-size: 0.8rem; color: var(--text-muted);">
                                Seller: ${escapeHtml(item.seller_name || 'Verified Seller')} | Quantity: ${item.quantity}
                            </div>
                        </div>
                        <div class="bold" style="color: var(--lavender-dark);">
                            ${formatPrice(item.price_per_unit * item.quantity)}
                        </div>
                    </div>
                `).join('')}
            </div>

            <div style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 12px; padding-top: 14px; border-top: 1px solid var(--border);">
                <div style="font-size: 0.85rem; color: var(--text-muted);">
                    📍 Shipping To: <span style="color: var(--text-main); font-weight: 500;">${escapeHtml(order.shipping_address || 'Default Address')}</span>
                </div>
                <div style="font-size: 1.1rem;">
                    Total: <strong style="color: var(--lavender-dark); font-size: 1.25rem;">${formatPrice(order.total_amount)}</strong>
                </div>
            </div>
        </div>
    `;
}
