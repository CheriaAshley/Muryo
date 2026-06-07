const BASE_URL = "http://127.0.0.1:8080";

document.addEventListener("DOMContentLoaded", function () {
    bindRegisterEvents();
});

function bindRegisterEvents() {
    const registerForm = document.getElementById("registerForm");
    const usernameInput = document.getElementById("username");
    const passwordInput = document.getElementById("password");
    const confirmPasswordInput = document.getElementById("confirmPassword");

    if (registerForm) {
        registerForm.addEventListener("submit", function (event) {
            event.preventDefault();
            register();
        });
    }

    if (usernameInput && passwordInput) {
        usernameInput.addEventListener("keydown", function (event) {
            if (event.key === "Enter") {
                event.preventDefault();
                passwordInput.focus();
            }
        });
    }

    if (passwordInput && confirmPasswordInput) {
        passwordInput.addEventListener("keydown", function (event) {
            if (event.key === "Enter") {
                event.preventDefault();
                confirmPasswordInput.focus();
            }
        });
    }

    if (confirmPasswordInput && registerForm) {
        confirmPasswordInput.addEventListener("keydown", function (event) {
            if (event.key === "Enter") {
                event.preventDefault();

                registerForm.dispatchEvent(new Event("submit", {
                    cancelable: true
                }));
            }
        });
    }
}

function register() {
    const usernameInput = document.getElementById("username");
    const passwordInput = document.getElementById("password");
    const confirmPasswordInput = document.getElementById("confirmPassword");
    const msg = document.getElementById("msg");
    const registerBtn = document.getElementById("registerBtn");

    if (!usernameInput || !passwordInput || !confirmPasswordInput || !msg) {
        console.error("注册页面缺少必要的 DOM 元素");
        return;
    }

    const username = usernameInput.value.trim();
    const password = passwordInput.value.trim();
    const confirmPassword = confirmPasswordInput.value.trim();

    msg.innerText = "";

    if (!username || !password) {
        msg.innerText = "用户名和密码不能为空哦~";
        return;
    }

    if (!confirmPassword) {
        msg.innerText = "咪，请再次输入密码哦~";
        return;
    }

    if (password !== confirmPassword) {
        msg.innerText = "两次输入的密码不一致哦~";
        return;
    }

    if (registerBtn) {
        registerBtn.disabled = true;
        registerBtn.innerText = "注册中...";
    }

    const params = new URLSearchParams();
    params.append("user_name", username);
    params.append("password", password);

    fetch(`${BASE_URL}/register`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
        .then(function (response) {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }

            return response.json();
        })
        .then(function (data) {
            if (data.success) {
                localStorage.removeItem("user_id");
                localStorage.removeItem("user_name");

                msg.innerText = data.message || "注册成功~欢迎加入Muryo~";

                setTimeout(function () {
                    window.location.replace("login.html");
                }, 1000);
            }
            else {
                msg.innerText = data.message || "注册失败，用户名可能已经存在~";
            }
        })
        .catch(function (error) {
            console.error("请求失败:", error);
            msg.innerText = "服务器连接失败，请检查后端是否启动哦~";
        })
        .finally(function () {
            if (registerBtn) {
                registerBtn.disabled = false;
                registerBtn.innerText = "注册";
            }
        });
}