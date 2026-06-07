document.addEventListener("DOMContentLoaded", function () {
    const usernameInput = document.getElementById("username");
    const passwordInput = document.getElementById("password");
    const loginForm = document.getElementById("loginForm");

    //防止报错，如果页面上没有表单，就不绑定事件
    if (!loginForm) return;

    // 用户名回车 → 跳到密码框，也要先判断元素是否存在，防止报错
    if (usernameInput) {
        usernameInput.addEventListener("keydown", function(event) {
            if (event.key === "Enter") {
                event.preventDefault();
                passwordInput.focus();
            }
        });
    }

    // 密码回车 → 提交表单，也要先判断元素是否存在，防止报错
    if (passwordInput) {
        passwordInput.addEventListener("keydown", function(event) {
            if (event.key === "Enter") {
                event.preventDefault();
                loginForm.dispatchEvent(new Event("submit", { cancelable: true }));//主动触发事件，且允许取消默认行为，交给submit事件的监听器去处理登录逻辑
            }
        });
    }

    // 统一绑定表单提交
    if (loginForm) {
        loginForm.addEventListener("submit", function(event) {
            event.preventDefault();
            login();
        });
    }
});

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
        //encodeURIComponent对用户名和密码进行编码，防止特殊字符导致请求失败
        body: `user_name=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
    })
   .then(response => {
        if (!response.ok) {
            throw new Error("服务器响应异常");//跳转到catch块，显示连接失败的提示
        }
        return response.json();
    })
    .then(data => {
        if (data.success) {
            // 统一保存登录信息
            localStorage.setItem("user_id", data.user_id);
            localStorage.setItem("user_name", data.user_name);

            alert(data.message || "登录成功！欢迎来到Muryo！");
            window.location.replace("index.html");//当前页面跳转到index，不可以后退回登录页*主要是区别一下herf
        } else {
            msg.innerText = data.message || "登录失败，咪请检查账号密码或是否注册~";
        }
    })
    .catch(error => {
        console.error("请求失败:", error);
        msg.innerText = "服务器连接失败，请检查后端配置！";
    })
    //容易忘记，无论成功还是失败，都要恢复登录按钮的状态，防止用户被卡住
    .finally(() => {
        if (loginBtn) {
            loginBtn.disabled = false;
            loginBtn.innerText = "登录";
        }    
    });
}