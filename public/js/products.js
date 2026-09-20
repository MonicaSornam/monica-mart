/**
 * MONICA MART — Product Catalog, Search, Filter & Detail Views
 * Developer: Monica Sornam (College Capstone Project)
 */

let allProducts = [];
let currentCategory = '';
let currentSearch = '';
let currentSort = 'default';

document.addEventListener('DOMContentLoaded', () => {
    // 1. If on Products Listing Page
    if (document.getElementById('products-grid')) {
        initProductsCatalog();
    }

    // 2. If on Product Details Page
    if (document.getElementById('product-detail-container')) {
        initProductDetailPage();
    }

    // 3. If on Home Page with featured grid
    if (document.getElementById('featured-products-grid')) {
        initFeaturedProducts();
    }
});

// --- Catalog Initialization ---
async function initProductsCatalog() {
    // Parse URL params for initial category or search
    const urlParams = new URLSearchParams(window.location.search);
    const catParam = urlParams.get('category');
    const searchParam = urlParams.get('search');

    if (catParam) currentCategory = catParam;
    if (searchParam) currentSearch = searchParam;

    const searchInput = document.getElementById('search-input');
    if (searchInput && currentSearch) {
        searchInput.value = currentSearch;
    }

    // Setup Category Pill Listeners
    const pills = document.querySelectorAll('.category-pill');
    pills.forEach(pill => {
        const cat = pill.getAttribute('data-category');
        if (cat === currentCategory || (!currentCategory && cat === '')) {
            pill.classList.add('active');
        } else {
            pill.classList.remove('active');
        }

        pill.addEventListener('click', () => {
            pills.forEach(p => p.classList.remove('active'));
            pill.classList.add('active');
            currentCategory = cat;
            fetchAndRenderProducts();
        });
    });

    // Setup Search Listener
    if (searchInput) {
        let debounceTimer;
        searchInput.addEventListener('input', (e) => {
            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(() => {
                currentSearch = e.target.value.trim();
                fetchAndRenderProducts();
            }, 300);
        });
    }

    // Setup Sort Listener
    const sortSelect = document.getElementById('sort-select');
    if (sortSelect) {
        sortSelect.addEventListener('change', (e) => {
            currentSort = e.target.value;
            renderProductsList();
        });
    }

    await fetchAndRenderProducts();
}

// --- Fetch Products from API ---
async function fetchAndRenderProducts() {
    const grid = document.getElementById('products-grid');
    if (!grid) return;

    grid.innerHTML = `
        <div style="grid-column: 1/-1; text-align: center; padding: 40px;">
            <p class="text-muted">Loading products from Monica Mart...</p>
        </div>
    `;

    let url = '/products';
    const params = [];
    if (currentSearch) params.push(`search=${encodeURIComponent(currentSearch)}`);
    if (currentCategory) params.push(`category=${encodeURIComponent(currentCategory)}`);
    if (params.length > 0) url += `?${params.join('&')}`;

    const res = await apiRequest(url);
    if (res.ok && res.data.success) {
        allProducts = res.data.products || [];
        renderProductsList();
    } else {
        grid.innerHTML = `
            <div style="grid-column: 1/-1; text-align: center; padding: 40px;">
                <p class="text-muted">Failed to load products. Please refresh.</p>
            </div>
        `;
    }
}

// --- Render Filtered & Sorted Products ---
function renderProductsList() {
    const grid = document.getElementById('products-grid');
    if (!grid) return;

    let items = [...allProducts];

    // Sorting
    if (currentSort === 'price-asc') {
        items.sort((a, b) => a.price - b.price);
    } else if (currentSort === 'price-desc') {
        items.sort((a, b) => b.price - a.price);
    } else if (currentSort === 'rating-desc') {
        items.sort((a, b) => (b.rating || 0) - (a.rating || 0));
    }

    if (items.length === 0) {
        grid.innerHTML = `
            <div style="grid-column: 1/-1;">
                <div class="empty-state">
                    <div class="empty-state-icon">🛍️</div>
                    <h3>No products found</h3>
                    <p>We couldn't find any products matching your criteria. Try adjusting your search or category filter.</p>
                    <button class="btn btn-peach" onclick="resetFilters()">Clear Filters</button>
                </div>
            </div>
        `;
        return;
    }

    grid.innerHTML = items.map(p => createProductCardHtml(p)).join('');
}

// --- Reset Filters Helper ---
function resetFilters() {
    currentCategory = '';
    currentSearch = '';
    const searchInput = document.getElementById('search-input');
    if (searchInput) searchInput.value = '';

    const pills = document.querySelectorAll('.category-pill');
    pills.forEach(p => {
        if (p.getAttribute('data-category') === '') p.classList.add('active');
        else p.classList.remove('active');
    });

    fetchAndRenderProducts();
}

// --- Product Card Component ---
function createProductCardHtml(product) {
    const stars = product.rating ? '★'.repeat(Math.round(product.rating)) + '☆'.repeat(5 - Math.round(product.rating)) : '★★★★★';
    const ratingText = product.rating ? `${product.rating} (${product.review_count || 0})` : 'New';
    const inStock = product.quantity > 0;
    const placeholderImg = 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80';
    const imageSrc = product.image ? escapeHtml(product.image) : placeholderImg;

    return `
        <div class="product-card">
            <div class="product-image-wrap">
                <span class="product-badge-category">${escapeHtml(product.category)}</span>
                <a href="product.html?id=${product.product_id}">
                    <img src="${imageSrc}" alt="${escapeHtml(product.product_name)}" onerror="this.src='${placeholderImg}'">
                </a>
            </div>
            <div class="product-body">
                <h3 class="product-title">
                    <a href="product.html?id=${product.product_id}">${escapeHtml(product.product_name)}</a>
                </h3>
                <div class="product-rating">
                    <span>${stars}</span>
                    <span class="count">${ratingText}</span>
                </div>
                <div class="product-footer">
                    <div>
                        <div class="product-price">${formatPrice(product.price)}</div>
                        <div class="product-stock ${inStock ? '' : 'text-muted'}">
                            ${inStock ? `${product.quantity} in stock` : '<span style="color: var(--status-cancelled); font-weight: bold;">Out of Stock</span>'}
                        </div>
                    </div>
                </div>
                <div class="product-actions">
                    <a href="product.html?id=${product.product_id}" class="btn btn-sm btn-outline">Details</a>
                    <button class="btn btn-sm btn-primary" onclick="quickAddToCart(${product.product_id})" ${inStock ? '' : 'disabled'}>
                        + Add to Cart
                    </button>
                </div>
            </div>
        </div>
    `;
}

// --- Quick Add to Cart from Card ---
async function quickAddToCart(productId) {
    const user = AppState.getUser();
    if (!user) {
        showToast('Please log in as a buyer to add items to your cart.', 'warning');
        setTimeout(() => window.location.href = 'login.html', 1000);
        return;
    }
    if (user.role !== 'BUYER') {
        showToast('Only buyer accounts can add items to cart.', 'error');
        return;
    }

    const res = await apiRequest('/cart', 'POST', { product_id: productId, quantity: 1 });
    if (res.ok && res.data.success) {
        showToast('Product added to your cart! 🛍️', 'success');
        updateCartBadge();
    } else {
        showToast(res.data.message || 'Could not add to cart.', 'error');
    }
}

// --- Featured Products on Home Page ---
async function initFeaturedProducts() {
    const grid = document.getElementById('featured-products-grid');
    if (!grid) return;

    const res = await apiRequest('/products');
    if (res.ok && res.data.success) {
        const featured = (res.data.products || []).slice(0, 4);
        if (featured.length > 0) {
            grid.innerHTML = featured.map(p => createProductCardHtml(p)).join('');
        } else {
            grid.innerHTML = '<p class="text-muted text-center" style="grid-column: 1/-1;">No featured products found.</p>';
        }
    }
}

// --- Product Details Page Controller ---
async function initProductDetailPage() {
    const urlParams = new URLSearchParams(window.location.search);
    const productId = urlParams.get('id');

    if (!productId) {
        window.location.href = 'products.html';
        return;
    }

    const res = await apiRequest(`/products/${productId}`);
    if (!res.ok || !res.data.success) {
        showToast('Product not found.', 'error');
        setTimeout(() => window.location.href = 'products.html', 1200);
        return;
    }

    const product = res.data.product;
    renderProductDetails(product);
    loadProductReviews(productId);
    setupReviewSubmission(productId);
}

// --- Render Product Details View ---
function renderProductDetails(product) {
    document.title = `${product.product_name} — MONICA MART`;

    const imgContainer = document.getElementById('detail-img-container');
    const titleEl = document.getElementById('detail-title');
    const priceEl = document.getElementById('detail-price');
    const categoryEl = document.getElementById('detail-category');
    const sellerEl = document.getElementById('detail-seller');
    const descEl = document.getElementById('detail-desc');
    const stockEl = document.getElementById('detail-stock');
    const addToCartBtn = document.getElementById('detail-add-cart-btn');

    const placeholderImg = 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=600&auto=format&fit=crop&q=80';
    const imageSrc = product.image ? escapeHtml(product.image) : placeholderImg;

    if (imgContainer) {
        imgContainer.innerHTML = `<img src="${imageSrc}" alt="${escapeHtml(product.product_name)}" onerror="this.src='${placeholderImg}'">`;
    }
    if (titleEl) titleEl.textContent = product.product_name;
    if (priceEl) priceEl.textContent = formatPrice(product.price);
    if (categoryEl) categoryEl.textContent = product.category;
    if (sellerEl) sellerEl.textContent = product.seller_name || 'Verified Seller';
    if (descEl) descEl.textContent = product.description || 'No description provided for this product.';

    const inStock = product.quantity > 0;
    if (stockEl) {
        stockEl.innerHTML = inStock 
            ? `<span class="badge badge-delivered">In Stock (${product.quantity} units available)</span>` 
            : `<span class="badge badge-cancelled">Out of Stock</span>`;
    }

    // Quantity controls
    let selectedQty = 1;
    const qtyDisplay = document.getElementById('detail-qty-val');
    const decBtn = document.getElementById('detail-qty-dec');
    const incBtn = document.getElementById('detail-qty-inc');

    if (decBtn && incBtn && qtyDisplay) {
        decBtn.onclick = () => {
            if (selectedQty > 1) {
                selectedQty--;
                qtyDisplay.textContent = selectedQty;
            }
        };
        incBtn.onclick = () => {
            if (selectedQty < product.quantity) {
                selectedQty++;
                qtyDisplay.textContent = selectedQty;
            } else {
                showToast(`Maximum available stock is ${product.quantity}.`, 'warning');
            }
        };
    }

    if (addToCartBtn) {
        if (!inStock) {
            addToCartBtn.disabled = true;
            addToCartBtn.textContent = 'Out of Stock';
        } else {
            addToCartBtn.onclick = async () => {
                const user = AppState.getUser();
                if (!user) {
                    showToast('Please log in as a buyer to add items to cart.', 'warning');
                    setTimeout(() => window.location.href = 'login.html', 1000);
                    return;
                }
                if (user.role !== 'BUYER') {
                    showToast('Only buyer accounts can purchase products.', 'error');
                    return;
                }

                addToCartBtn.disabled = true;
                addToCartBtn.textContent = 'Adding...';

                const addRes = await apiRequest('/cart', 'POST', {
                    product_id: product.product_id,
                    quantity: selectedQty
                });

                addToCartBtn.disabled = false;
                addToCartBtn.textContent = 'Add to Cart 🛍️';

                if (addRes.ok && addRes.data.success) {
                    showToast(`Added ${selectedQty} item(s) to your cart!`, 'success');
                    updateCartBadge();
                } else {
                    showToast(addRes.data.message || 'Could not add to cart.', 'error');
                }
            };
        }
    }
}

// --- Load Reviews for Product ---
async function loadProductReviews(productId) {
    const list = document.getElementById('reviews-list');
    if (!list) return;

    const res = await apiRequest(`/products/${productId}/reviews`);
    if (res.ok && res.data.success) {
        const reviews = res.data.reviews || [];
        if (reviews.length === 0) {
            list.innerHTML = `
                <div style="grid-column: 1/-1; text-align: center; padding: 24px;">
                    <p class="text-muted">No reviews yet for this product. Be the first to share your experience!</p>
                </div>
            `;
            return;
        }

        list.innerHTML = reviews.map(r => {
            const stars = '★'.repeat(r.rating) + '☆'.repeat(5 - r.rating);
            return `
                <div class="review-card">
                    <div class="review-header">
                        <span class="reviewer-name">${escapeHtml(r.buyer_name || 'Verified Buyer')}</span>
                        <span class="review-stars">${stars}</span>
                    </div>
                    <p class="review-comment">${escapeHtml(r.comment)}</p>
                    <div style="font-size: 0.78rem; color: var(--text-light); margin-top: 8px;">
                        ${r.created_at ? new Date(r.created_at).toLocaleDateString() : 'Recent'}
                    </div>
                </div>
            `;
        }).join('');
    }
}

// --- Setup Review Submission Form ---
function setupReviewSubmission(productId) {
    const reviewForm = document.getElementById('submit-review-form');
    if (!reviewForm) return;

    const user = AppState.getUser();
    const reviewCard = document.getElementById('submit-review-card');

    if (!user || user.role !== 'BUYER') {
        if (reviewCard) {
            reviewCard.innerHTML = `
                <div class="empty-state" style="padding: 24px;">
                    <p class="text-muted">Please <a href="login.html" class="bold" style="color: var(--lavender-dark);">log in as a buyer</a> to submit a product review.</p>
                </div>
            `;
        }
        return;
    }

    reviewForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const rating = parseInt(document.getElementById('review-rating').value, 10);
        const comment = document.getElementById('review-comment').value.trim();

        if (!rating || rating < 1 || rating > 5) {
            showToast('Please select a star rating from 1 to 5.', 'error');
            return;
        }

        if (!comment || comment.length < 4) {
            showToast('Please write a brief comment sharing your experience.', 'error');
            return;
        }

        const submitBtn = reviewForm.querySelector('button[type="submit"]');
        submitBtn.disabled = true;

        const res = await apiRequest('/reviews', 'POST', {
            product_id: parseInt(productId, 10),
            rating,
            comment
        });

        submitBtn.disabled = false;

        if (res.ok && res.data.success) {
            showToast('Thank you! Your review has been submitted.', 'success');
            document.getElementById('review-comment').value = '';
            loadProductReviews(productId);
        } else {
            showToast(res.data.message || 'Failed to submit review.', 'error');
        }
    });
}
