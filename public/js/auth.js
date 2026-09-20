/**
 * MONICA MART — Authentication Controller (Login & Registration)
 * Developer: Monica Sornam (College Capstone Project)
 */

document.addEventListener('DOMContentLoaded', () => {
    // 1. Handle Login Form
    const loginForm = document.getElementById('login-form');
    if (loginForm) {
        loginForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const submitBtn = loginForm.querySelector('button[type="submit"]');
            submitBtn.disabled = true;
            submitBtn.textContent = 'Logging in...';

            const email = document.getElementById('login-email').value.trim();
            const password = document.getElementById('login-password').value;

            if (!email || !password) {
                showToast('Please provide both email and password.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Log In';
                return;
            }

            const res = await apiRequest('/login', 'POST', { email, password });

            if (res.ok && res.data.success) {
                AppState.setUser(res.data.user);
                showToast(`Welcome back, ${res.data.user.name}!`, 'success');

                setTimeout(() => {
                    const role = res.data.user.role;
                    if (role === 'ADMIN') {
                        window.location.href = 'admin.html';
                    } else if (role === 'SELLER') {
                        window.location.href = 'seller.html';
                    } else {
                        window.location.href = 'products.html';
                    }
                }, 800);
            } else {
                showToast(res.data.message || 'Invalid email or password.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Log In';
            }
        });
    }

    // 2. Handle Register Form
    const registerForm = document.getElementById('register-form');
    if (registerForm) {
        // Role selector cards toggle
        const roleOptions = document.querySelectorAll('.role-option');
        roleOptions.forEach(opt => {
            opt.addEventListener('click', () => {
                roleOptions.forEach(o => o.classList.remove('selected'));
                opt.classList.add('selected');
                const radio = opt.querySelector('input[type="radio"]');
                if (radio) radio.checked = true;
            });
        });

        registerForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const submitBtn = registerForm.querySelector('button[type="submit"]');
            submitBtn.disabled = true;
            submitBtn.textContent = 'Creating Account...';

            const name = document.getElementById('reg-name').value.trim();
            const email = document.getElementById('reg-email').value.trim();
            const password = document.getElementById('reg-password').value;
            const roleRadio = document.querySelector('input[name="role"]:checked');
            const role = roleRadio ? roleRadio.value : 'BUYER';

            // Frontend Validations
            if (!name || name.length < 2) {
                showToast('Please enter your full name.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Register Now';
                return;
            }

            const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
            if (!emailRegex.test(email)) {
                showToast('Please enter a valid email address.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Register Now';
                return;
            }

            if (!password || password.length < 6) {
                showToast('Password must be at least 6 characters long.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Register Now';
                return;
            }

            const res = await apiRequest('/register', 'POST', { name, email, password, role });

            if (res.ok && res.data.success) {
                AppState.setUser(res.data.user);
                showToast('Registration successful! Welcome to Monica Mart.', 'success');

                setTimeout(() => {
                    if (res.data.user.role === 'SELLER') {
                        window.location.href = 'seller.html';
                    } else {
                        window.location.href = 'products.html';
                    }
                }, 800);
            } else {
                showToast(res.data.message || 'Registration failed. Email might already be taken.', 'error');
                submitBtn.disabled = false;
                submitBtn.textContent = 'Register Now';
            }
        });
    }
});

// Quick fill helper for evaluation and testing demo
function fillDemoAccount(role) {
    const emailInput = document.getElementById('login-email');
    const passwordInput = document.getElementById('login-password');
    if (!emailInput || !passwordInput) return;

    if (role === 'admin') {
        emailInput.value = 'admin@monicamart.com';
        passwordInput.value = 'Admin@123';
    } else if (role === 'seller') {
        emailInput.value = 'tech_seller@monicamart.com';
        passwordInput.value = 'Password@123';
    } else if (role === 'buyer') {
        emailInput.value = 'buyer1@monicamart.com';
        passwordInput.value = 'Password@123';
    }
    showToast(`Filled credentials for demo ${role.toUpperCase()}`, 'warning');
}
