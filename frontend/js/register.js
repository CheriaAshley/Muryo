function register() {
    const username = document.getElementById("username").value.trim();
    const password = document.getElementById("password").value.trim();
    const confirmPassword = document.getElementById("confirmPassword").value.trim();
    const msg = document.getElementById("msg");
    const registerBtn = document.getElementById("registerBtn");

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
    fetch("http://127.0.0.1:8080/register", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `user_name=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
    })
    .then(response => {
        if (!response.ok) {
            throw new Error("服务器响应异常");
        }
        return response.json();
    })
    .then(data => {
        if (data.success) {
            localStorage.removeItem("user_id");
            localStorage.removeItem("user_name");

            msg.innerText = data.message || "注册成功~欢迎加入Muryo~";
            setTimeout(() => {
                window.location.replace ( "login.html");
            }, 1000);
        } else {
            msg.innerText = data.message || "注册失败，用户名重复~";
        }
    })
    .catch(error => {
        console.error("请求失败:", error);
        msg.innerText = "服务器连接失败，请检查后端是否启动哦~";
    })
    .finally(() => {
        if (registerBtn) {
            registerBtn.disabled = false;
            registerBtn.innerText = "注册";
        }
    });
}