/**
 * MONICA MART — Admin Governance Dashboard Controller
 * Developer: Monica Sornam (College Capstone Project)
 */

let pendingDeleteProductId = null;

document.addEventListener('DOMContentLoaded', () => {
    if (document.getElementById('admin-dashboard-container')) {
        initAdminDashboard();
    }
});

async function initAdminDashboard() {
    const user = AppState.getUser();
    if (!user || user.role !== 'ADMIN') {
        showToast('Unauthorized. Administrator privileges required.', 'error');
        setTimeout(() => window.location.href = 'login.html', 1000);
        return;
    }

    // Load Stats
    await loadAdminStats();

    // Load Sections
    await loadAdminUsers();
    await loadAdminProducts();
    await loadAdminOrders();

    // Setup Modals
    setupAdminDeleteModal();
}

// --- Platform Stats ---
async function loadAdminStats() {
    const res = await apiRequest('/admin/stats');
    if (res.ok && res.data.success) {
        const stats = res.data.stats;
        document.getElementById('stat-admin-users').textContent = stats.total_users || 0;
        document.getElementById('stat-admin-products').textContent = stats.total_products || 0;
        document.getElementById('stat-admin-orders').textContent = stats.total_orders || 0;
        document.getElementById('stat-admin-volume').textContent = formatPrice(stats.total_revenue || 0);
    }
}

// --- Users List ---
async function loadAdminUsers() {
    const tableBody = document.getElementById('admin-users-table-body');
    if (!tableBody) return;

    tableBody.innerHTML = `<tr><td colspan="5" class="text-center text-muted" style="padding: 24px;">Loading users...</td></tr>`;

    const res = await apiRequest('/admin/users');
    if (!res.ok || !res.data.success) {
        tableBody.innerHTML = `<tr><td colspan="5" class="text-center text-muted" style="padding: 24px;">Failed to load users.</td></tr>`;
        return;
    }

    const users = res.data.users || [];
    tableBody.innerHTML = users.map(u => {
        let badgeColor = 'badge-lavender';
        if (u.role === 'ADMIN') badgeColor = 'badge-cancelled';
        else if (u.role === 'SELLER') badgeColor = 'badge-rose';

        return `
            <tr>
                <td><strong>#${u.id}</strong></td>
                <td><strong>${escapeHtml(u.name)}</strong></td>
                <td>${escapeHtml(u.email)}</td>
                <td><span class="badge ${badgeColor}">${u.role}</span></td>
                <td style="color: var(--text-muted); font-size: 0.85rem;">
                    ${u.created_at ? new Date(u.created_at).toLocaleDateString() : 'N/A'}
                </td>
            </tr>
        `;
    }).join('');
}

// --- Products Moderation List ---
async function loadAdminProducts() {
    const tableBody = document.getElementById('admin-products-table-body');
    if (!tableBody) return;

    tableBody.innerHTML = `<tr><td colspan="7" class="text-center text-muted" style="padding: 24px;">Loading products...</td></tr>`;

    const res = await apiRequest('/admin/products');
    if (!res.ok || !res.data.success) {
        tableBody.innerHTML = `<tr><td colspan="7" class="text-center text-muted" style="padding: 24px;">Failed to load products.</td></tr>`;
        return;
    }

    const products = res.data.products || [];
    tableBody.innerHTML = products.map(p => `
        <tr>
            <td><strong>#${p.product_id}</strong></td>
            <td>
                <div style="display: flex; align-items: center; gap: 10px;">
                    <img src="${escapeHtml(p.image || 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80')}" 
                         alt="" style="width: 38px; height: 38px; object-fit: cover; border-radius: var(--radius-sm); border: 1px solid var(--border);">
                    <strong>${escapeHtml(p.product_name)}</strong>
                </div>
            </td>
            <td><span class="badge badge-rose">${escapeHtml(p.category)}</span></td>
            <td>${escapeHtml(p.seller_name || 'Seller')}</td>
            <td class="bold" style="color: var(--lavender-dark);">${formatPrice(p.price)}</td>
            <td>${p.quantity}</td>
            <td>
                <button class="btn btn-sm btn-danger" onclick="promptAdminRemoveProduct(${p.product_id}, '${escapeHtml(p.product_name)}')">
                    Remove
                </button>
            </td>
        </tr>
    `).join('');
}

// --- Orders Audit List ---
async function loadAdminOrders() {
    const tableBody = document.getElementById('admin-orders-table-body');
    if (!tableBody) return;

    tableBody.innerHTML = `<tr><td colspan="6" class="text-center text-muted" style="padding: 24px;">Loading all orders...</td></tr>`;

    const res = await apiRequest('/admin/orders');
    if (!res.ok || !res.data.success) {
        tableBody.innerHTML = `<tr><td colspan="6" class="text-center text-muted" style="padding: 24px;">Failed to load orders.</td></tr>`;
        return;
    }

    const orders = res.data.orders || [];
    tableBody.innerHTML = orders.map(ord => `
        <tr>
            <td><strong>#MM-${ord.order_id}</strong></td>
            <td>
                <div>${escapeHtml(ord.buyer_name || 'Customer')}</div>
                <div style="font-size: 0.78rem; color: var(--text-light);">${escapeHtml(ord.buyer_email || '')}</div>
            </td>
            <td style="font-size: 0.85rem; color: var(--text-muted);">
                ${ord.created_at ? new Date(ord.created_at).toLocaleDateString() : 'Recent'}
            </td>
            <td>${(ord.items || []).length} item(s)</td>
            <td class="bold" style="color: var(--lavender-dark);">${formatPrice(ord.total_amount)}</td>
            <td>
                <span class="badge badge-${(ord.status || 'pending').toLowerCase()}">${ord.status}</span>
            </td>
        </tr>
    `).join('');
}

// --- Product Removal Confirmation Modal ---
function setupAdminDeleteModal() {
    const modal = document.getElementById('admin-delete-modal');
    const cancelBtn = document.getElementById('admin-cancel-delete-btn');
    const confirmBtn = document.getElementById('admin-confirm-delete-btn');

    if (cancelBtn && modal) {
        cancelBtn.onclick = () => {
            modal.classList.remove('active');
            pendingDeleteProductId = null;
        };
    }

    if (confirmBtn && modal) {
        confirmBtn.onclick = async () => {
            if (!pendingDeleteProductId) return;

            confirmBtn.disabled = true;
            confirmBtn.textContent = 'Removing...';

            const res = await apiRequest(`/admin/products/${pendingDeleteProductId}`, 'DELETE');
            confirmBtn.disabled = false;
            confirmBtn.textContent = 'Remove Product';
            modal.classList.remove('active');

            if (res.ok && res.data.success) {
                showToast('Inappropriate product has been removed from marketplace.', 'success');
                loadAdminProducts();
                loadAdminStats();
            } else {
                showToast(res.data.message || 'Failed to delete product.', 'error');
            }
            pendingDeleteProductId = null;
        };
    }
}

function promptAdminRemoveProduct(productId, productName) {
    pendingDeleteProductId = productId;
    const nameEl = document.getElementById('delete-prod-name');
    if (nameEl) nameEl.textContent = productName;

    const modal = document.getElementById('admin-delete-modal');
    if (modal) modal.classList.add('active');
}
