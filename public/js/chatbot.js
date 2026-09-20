/**
 * MONICA MART — AI Shopping Assistant Chatbot
 * Developer: Monica Sornam (College Capstone Project)
 */

document.addEventListener('DOMContentLoaded', () => {
    initChatbotWidget();
});

function initChatbotWidget() {
    // 1. Mount Chatbot Markup if not present
    if (!document.getElementById('chatbot-widget-root')) {
        const root = document.createElement('div');
        root.id = 'chatbot-widget-root';
        root.innerHTML = `
            <!-- Floating Launcher Button -->
            <button class="chatbot-launcher" id="chatbot-launcher-btn" title="Ask Monica Mart AI Assistant">
                ✨
            </button>

            <!-- Chatbot Window -->
            <div class="chatbot-window" id="chatbot-window-box">
                <div class="chatbot-header">
                    <div class="chatbot-header-info">
                        <span style="font-size: 1.3rem;">🌸</span>
                        <div>
                            <h4>Monica Mart Assistant</h4>
                            <div class="chatbot-status">
                                <span class="status-dot"></span> Online | C++20 Smart Engine
                            </div>
                        </div>
                    </div>
                    <button class="chatbot-close" id="chatbot-close-btn" title="Close">✕</button>
                </div>

                <div class="chatbot-messages" id="chatbot-messages-list">
                    <div class="chat-bubble bot">
                        Hello! 👋 I am <strong>Monica Mart's Shopping Assistant</strong>.<br>
                        How can I assist your shopping experience today?
                    </div>
                </div>

                <!-- Suggested Quick Prompts -->
                <div class="chatbot-suggestions">
                    <button class="suggestion-chip" onclick="askQuickPrompt('How can I search for a product?')">🔍 Search product</button>
                    <button class="suggestion-chip" onclick="askQuickPrompt('How do I add a product to cart?')">🛒 Add to cart</button>
                    <button class="suggestion-chip" onclick="askQuickPrompt('How do I checkout?')">💳 Checkout</button>
                    <button class="suggestion-chip" onclick="askQuickPrompt('Where can I see my orders?')">📦 Track orders</button>
                    <button class="suggestion-chip" onclick="askQuickPrompt('What categories are available?')">🏷️ Categories</button>
                    <button class="suggestion-chip" onclick="askQuickPrompt('How can I give a review?')">⭐ Give review</button>
                    <button class="suggestion-chip" onclick="askQuickPrompt('How can I remove an item from cart?')">🗑️ Remove item</button>
                </div>

                <form class="chatbot-input-area" id="chatbot-input-form">
                    <input type="text" id="chatbot-input-field" placeholder="Ask a question..." autocomplete="off">
                    <button type="submit" class="chatbot-send" title="Send message">➤</button>
                </form>
            </div>
        `;
        document.body.appendChild(root);
    }

    // 2. Setup Events
    const launcher = document.getElementById('chatbot-launcher-btn');
    const windowBox = document.getElementById('chatbot-window-box');
    const closeBtn = document.getElementById('chatbot-close-btn');
    const form = document.getElementById('chatbot-input-form');
    const inputField = document.getElementById('chatbot-input-field');

    if (launcher && windowBox) {
        launcher.onclick = () => {
            windowBox.classList.toggle('active');
            if (windowBox.classList.contains('active')) {
                inputField.focus();
            }
        };
    }

    if (closeBtn && windowBox) {
        closeBtn.onclick = () => {
            windowBox.classList.remove('active');
        };
    }

    if (form) {
        form.onsubmit = async (e) => {
            e.preventDefault();
            const text = inputField.value.trim();
            if (!text) return;

            inputField.value = '';
            await processUserChat(text);
        };
    }
}

// --- Quick Prompt Trigger ---
async function askQuickPrompt(promptText) {
    await processUserChat(promptText);
}

// --- Chat Processor ---
async function processUserChat(userMessage) {
    const list = document.getElementById('chatbot-messages-list');
    if (!list) return;

    // 1. Add User Bubble
    const userBubble = document.createElement('div');
    userBubble.className = 'chat-bubble user';
    userBubble.textContent = userMessage;
    list.appendChild(userBubble);
    list.scrollTop = list.scrollHeight;

    // 2. Loading Indicator Bubble
    const botLoading = document.createElement('div');
    botLoading.className = 'chat-bubble bot';
    botLoading.textContent = 'Thinking... 💭';
    list.appendChild(botLoading);
    list.scrollTop = list.scrollHeight;

    // 3. Request from backend or rule-based fallback
    let botReply = '';

    try {
        const res = await apiRequest('/chatbot', 'POST', { message: userMessage });
        if (res.ok && res.data.success && res.data.response) {
            botReply = res.data.response;
        } else {
            botReply = generateSmartLocalResponse(userMessage);
        }
    } catch (e) {
        botReply = generateSmartLocalResponse(userMessage);
    }

    // Replace Loading with actual reply
    botLoading.innerHTML = botReply;
    list.scrollTop = list.scrollHeight;
}

// --- Rule-Based Intent Matching Engine (For Zero-Latency & Offline Viva) ---
function generateSmartLocalResponse(input) {
    const q = input.toLowerCase();

    if (q.includes('search') || q.includes('find')) {
        return '🔍 <strong>Searching for products:</strong><br>Navigate to the <a href="products.html" style="text-decoration: underline;">Products</a> page and type any keyword (e.g. <em>"Laptop"</em>, <em>"Watch"</em>, <em>"Shoes"</em>) into the search bar. The search is case-insensitive and updates instantly!';
    }
    
    if (q.includes('add') && (q.includes('cart') || q.includes('item'))) {
        return '🛒 <strong>Adding to Cart:</strong><br>You can click the <strong>"+ Add to Cart"</strong> button on any product card, or open the product details page, select your quantity, and click <strong>"Add to Cart"</strong>. Make sure you are logged in as a Buyer!';
    }

    if (q.includes('checkout') || q.includes('buy') || q.includes('place order')) {
        return '💳 <strong>Checkout Flow:</strong><br>1. Open your <a href="cart.html" style="text-decoration: underline;">Cart</a>.<br>2. Click <strong>"Proceed to Checkout"</strong>.<br>3. Verify your delivery address.<br>4. Click <strong>"Confirm Order"</strong>. Your order ID will be generated right away!';
    }

    if (q.includes('order') || q.includes('track') || q.includes('status') || q.includes('history')) {
        return '📦 <strong>Order Tracking:</strong><br>You can view all past orders, delivery addresses, items, and current status (Pending, Confirmed, Shipped, Delivered) on the <a href="orders.html" style="text-decoration: underline;">Orders Page</a>.';
    }

    if (q.includes('categor') || q.includes('type')) {
        return '🏷️ <strong>Available Categories:</strong><br>Monica Mart offers 5 pastel-curated categories:<br>• <strong>Electronics</strong><br>• <strong>Fashion</strong><br>• <strong>Home</strong><br>• <strong>Beauty</strong><br>• <strong>Accessories</strong><br>Filter them via the pills on the Products page!';
    }

    if (q.includes('review') || q.includes('rating') || q.includes('star') || q.includes('comment')) {
        return '⭐ <strong>Giving Product Reviews:</strong><br>Open any product details page, scroll down to the <strong>"Write a Review"</strong> section, pick a 1 to 5 star rating, type your feedback, and click Submit!';
    }

    if (q.includes('remove') || (q.includes('delete') && q.includes('cart'))) {
        return '🗑️ <strong>Removing Items from Cart:</strong><br>Open your <a href="cart.html" style="text-decoration: underline;">Shopping Cart</a> and click the <strong>"✕ Remove"</strong> button next to the product, or click the minus (–) button until quantity reaches zero.';
    }

    if (q.includes('seller') || q.includes('vendor') || q.includes('sell')) {
        return '🏪 <strong>Seller Registration:</strong><br>You can register as a seller via <a href="register.html" style="text-decoration: underline;">Register</a> by choosing "Seller Account". Sellers get their own private dashboard to add, edit, and delete products, plus view buyer orders!';
    }

    if (q.includes('tech') || q.includes('c++') || q.includes('backend') || q.includes('monica') || q.includes('viva')) {
        return '🎓 <strong>Capstone Technical Information:</strong><br>• <strong>Developer:</strong> Monica Sornam<br>• <strong>Backend:</strong> C++20 + Drogon Web Framework<br>• <strong>Database:</strong> SQLite3 / PostgreSQL<br>• <strong>Frontend:</strong> Vanilla HTML5, CSS3, JavaScript<br>• <strong>Theme:</strong> Lavender, Dusty Rose, Soft Pink, Peach';
    }

    // Default Fallback
    return '👋 I am here to help you shop! You can ask me how to search products, add items to your cart, complete checkout, track orders, or write customer reviews.';
}
