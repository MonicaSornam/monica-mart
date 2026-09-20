/**
 * MONICA MART — Seller Dashboard & Inventory Management
 * Developer: Monica Sornam (College Capstone Project)
 */

let sellerProducts = [];
let editingProductId = null;

document.addEventListener('DOMContentLoaded', () => {
    if (document.getElementById('seller-dashboard-container')) {
        initSellerDashboard();
    }
});

async function initSellerDashboard() {
    const user = AppState.getUser();
    if (!user || user.role !== 'SELLER') {
        showToast('Unauthorized. Seller access required.', 'error');
        setTimeout(() => window.location.href = 'login.html', 1000);
        return;
    }

    // Set Welcome Banner
    const welcomeEl = document.getElementById('seller-welcome-name');
    if (welcomeEl) welcomeEl.textContent = user.name;

    // Load Seller Products & Metrics
    await loadSellerProducts();
    await loadSellerOrders();

    // Setup Add Product Form Listener
    setupAddProductModal();
}

// --- Load Seller Products & Stats ---
async function loadSellerProducts() {
    const tableBody = document.getElementById('seller-products-table-body');
    if (!tableBody) return;

    tableBody.innerHTML = `
        <tr><td colspan="7" class="text-center text-muted" style="padding: 30px;">Loading your inventory...</td></tr>
    `;

    const res = await apiRequest('/seller/products');
    if (!res.ok || !res.data.success) {
        tableBody.innerHTML = `
            <tr><td colspan="7" class="text-center text-muted" style="padding: 30px;">Failed to load your products.</td></tr>
        `;
        return;
    }

    sellerProducts = res.data.products || [];

    // Update Stat Card
    const totalProdEl = document.getElementById('stat-total-products');
    if (totalProdEl) totalProdEl.textContent = sellerProducts.length;

    if (sellerProducts.length === 0) {
        tableBody.innerHTML = `
            <tr><td colspan="7" class="text-center text-muted" style="padding: 30px;">
                You haven't listed any products yet. Click <strong>"+ Add Product"</strong> above to start selling!
            </td></tr>
        `;
        return;
    }

    tableBody.innerHTML = sellerProducts.map(p => `
        <tr>
            <td><strong>#${p.product_id}</strong></td>
            <td>
                <div style="display: flex; align-items: center; gap: 10px;">
                    <img src="${escapeHtml(p.image || 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80')}" 
                         alt="" style="width: 44px; height: 44px; object-fit: cover; border-radius: var(--radius-sm); border: 1px solid var(--border);">
                    <div>
                        <div class="bold" style="color: var(--text-main);">${escapeHtml(p.product_name)}</div>
                        <div style="font-size: 0.78rem; color: var(--text-light);">${p.description ? escapeHtml(p.description.substring(0, 45)) + '...' : ''}</div>
                    </div>
                </div>
            </td>
            <td><span class="badge badge-rose">${escapeHtml(p.category)}</span></td>
            <td class="bold" style="color: var(--lavender-dark);">${formatPrice(p.price)}</td>
            <td>
                <span class="badge ${p.quantity > 0 ? 'badge-delivered' : 'badge-cancelled'}">
                    ${p.quantity} in stock
                </span>
            </td>
            <td>★ ${p.rating || '0.0'} (${p.review_count || 0})</td>
            <td>
                <div style="display: flex; gap: 6px;">
                    <button class="btn btn-sm btn-outline" onclick="openEditProductModal(${p.product_id})">✏️ Edit</button>
                    <button class="btn btn-sm btn-danger" onclick="deleteSellerProduct(${p.product_id})">🗑️ Delete</button>
                </div>
            </td>
        </tr>
    `).join('');
}

// --- Load Orders for Seller ---
async function loadSellerOrders() {
    const tableBody = document.getElementById('seller-orders-table-body');
    if (!tableBody) return;

    tableBody.innerHTML = `
        <tr><td colspan="7" class="text-center text-muted" style="padding: 30px;">Loading orders...</td></tr>
    `;

    const res = await apiRequest('/seller/orders');
    if (!res.ok || !res.data.success) {
        tableBody.innerHTML = `
            <tr><td colspan="7" class="text-center text-muted" style="padding: 30px;">Failed to load orders.</td></tr>
        `;
        return;
    }

    const orders = res.data.orders || [];

    // Update Stats
    const totalOrdersEl = document.getElementById('stat-total-orders');
    const totalRevEl = document.getElementById('stat-total-revenue');
    if (totalOrdersEl) totalOrdersEl.textContent = res.data.count || 0;
    if (totalRevEl) totalRevEl.textContent = formatPrice(res.data.total_revenue || 0);

    if (orders.length === 0) {
        tableBody.innerHTML = `
            <tr><td colspan="7" class="text-center text-muted" style="padding: 30px;">No purchase orders received yet.</td></tr>
        `;
        return;
    }

    tableBody.innerHTML = orders.map(ord => `
        <tr>
            <td><strong>#MM-${ord.order_id}</strong></td>
            <td>${escapeHtml(ord.product_name)}</td>
            <td>${escapeHtml(ord.buyer_name || 'Customer')}</td>
            <td>${ord.quantity} unit(s)</td>
            <td class="bold" style="color: var(--lavender-dark);">${formatPrice(ord.line_total)}</td>
            <td>
                <span class="badge badge-${(ord.order_status || 'pending').toLowerCase()}">
                    ${ord.order_status}
                </span>
            </td>
            <td>
                <select class="form-control" style="padding: 4px 8px; font-size: 0.82rem; width: auto;" 
                        onchange="updateOrderStatus(${ord.order_id}, this.value)">
                    <option value="" disabled selected>Update Status</option>
                    <option value="Confirmed">Confirmed</option>
                    <option value="Shipped">Shipped</option>
                    <option value="Delivered">Delivered</option>
                    <option value="Cancelled">Cancelled</option>
                </select>
            </td>
        </tr>
    `).join('');
}

// --- Status Update by Seller ---
async function updateOrderStatus(orderId, newStatus) {
    if (!newStatus) return;

    const res = await apiRequest(`/orders/${orderId}/status`, 'PUT', { status: newStatus });
    if (res.ok && res.data.success) {
        showToast(`Order #MM-${orderId} status changed to ${newStatus}!`, 'success');
        loadSellerOrders();
    } else {
        showToast(res.data.message || 'Failed to update order status.', 'error');
    }
}

// --- Setup Add / Edit Product Modal ---
function setupAddProductModal() {
    const modalBackdrop = document.getElementById('product-modal');
    const openBtn = document.getElementById('open-add-product-btn');
    const closeBtn = document.getElementById('close-product-modal-btn');
    const form = document.getElementById('product-form');

    if (openBtn && modalBackdrop) {
        openBtn.onclick = () => {
            editingProductId = null;
            document.getElementById('modal-title').textContent = 'Add New Product';
            form.reset();
            modalBackdrop.classList.add('active');
        };
    }

    if (closeBtn && modalBackdrop) {
        closeBtn.onclick = () => {
            modalBackdrop.classList.remove('active');
        };
    }

    if (form) {
        form.onsubmit = async (e) => {
            e.preventDefault();

            const name = document.getElementById('prod-name').value.trim();
            const desc = document.getElementById('prod-desc').value.trim();
            const price = parseFloat(document.getElementById('prod-price').value);
            const category = document.getElementById('prod-category').value;
            const stock = parseInt(document.getElementById('prod-stock').value, 10);
            const image = document.getElementById('prod-image').value.trim();

            // Validations
            if (!name) {
                showToast('Product name cannot be empty.', 'error');
                return;
            }
            if (isNaN(price) || price < 0) {
                showToast('Please enter a valid non-negative price.', 'error');
                return;
            }
            if (isNaN(stock) || stock < 0) {
                showToast('Please enter a valid non-negative stock quantity.', 'error');
                return;
            }
            if (!category) {
                showToast('Please select a category.', 'error');
                return;
            }

            const payload = {
                product_name: name,
                description: desc,
                price: price,
                category: category,
                quantity: stock,
                image: image || 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80'
            };

            let res;
            if (editingProductId) {
                // Update
                res = await apiRequest(`/products/${editingProductId}`, 'PUT', payload);
            } else {
                // Create
                res = await apiRequest('/products', 'POST', payload);
            }

            if (res.ok && res.data.success) {
                showToast(editingProductId ? 'Product updated successfully!' : 'Product added successfully!', 'success');
                modalBackdrop.classList.remove('active');
                loadSellerProducts();
            } else {
                showToast(res.data.message || 'Operation failed.', 'error');
            }
        };
    }
}

// --- Open Edit Product Modal ---
function openEditProductModal(productId) {
    const product = sellerProducts.find(p => p.product_id === productId);
    if (!product) return;

    editingProductId = productId;
    document.getElementById('modal-title').textContent = 'Edit Product #' + productId;
    document.getElementById('prod-name').value = product.product_name;
    document.getElementById('prod-desc').value = product.description || '';
    document.getElementById('prod-price').value = product.price;
    document.getElementById('prod-category').value = product.category;
    document.getElementById('prod-stock').value = product.quantity;
    document.getElementById('prod-image').value = product.image || '';

    const modalBackdrop = document.getElementById('product-modal');
    if (modalBackdrop) modalBackdrop.classList.add('active');
}

// --- Delete Product ---
async function deleteSellerProduct(productId) {
    if (!confirm(`Are you sure you want to delete Product #${productId}? This action cannot be undone.`)) {
        return;
    }

    const res = await apiRequest(`/products/${productId}`, 'DELETE');
    if (res.ok && res.data.success) {
        showToast('Product deleted from your catalog.', 'success');
        loadSellerProducts();
    } else {
        showToast(res.data.message || 'Failed to delete product.', 'error');
    }
}
