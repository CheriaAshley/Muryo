function login() {
    const username = document.getElementById("username").value.trim();
    const password = document.getElementById("password").value.trim();
    const msg = document.getElementById("msg");
    const loginBtn = document.getElementById("loginBtn");

    msg.innerText = "";

    if (!username || !password) {
        msg.innerText = "用户名和密码不能为空哦~";
        return;
    }

    if (loginBtn) {
        loginBtn.disabled = true;
        loginBtn.innerText = "登录中...";
    }

    fetch("http://127.0.0.1:8080/login", {
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
            // 统一保存登录信息
            localStorage.setItem("user_id", data.user_id);
            localStorage.setItem("user_name", data.user_name);

            alert(data.message || "登录成功！欢迎来到Muryo！");
            window.location.replace("index.html");
        } else {
            msg.innerText = data.message || "登录失败，咪请检查账号密码或是否注册~";
        }
    })
    .catch(error => {
        console.error("请求失败:", error);
        msg.innerText = "服务器连接失败，请检查后端是否启动哦~";
    })
    .finally(() => {
        if (loginBtn) {
            loginBtn.disabled = false;
            loginBtn.innerText = "登录";
        }    
    });
}