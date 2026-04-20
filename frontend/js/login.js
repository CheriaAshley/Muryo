function login() {
    const username = document.getElementById("username").value;
    const password = document.getElementById("password").value;
    const msg = document.getElementById("msg");

    msg.innerText = "";

    if (!username || !password) {
        msg.innerText = "咪请输入用户名和密码";
        return;
    }

    fetch("http://127.0.0.1:8080/login", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `user_name=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            sessionStorage.setItem("user_id", data.user_id);
            alert("登录成功！");
            localStorage.setItem("username", username);
            window.location.replace ( "index.html");
        } else {
            msg.innerText = data.message || "用户名或密码错误";
        }
    })
    .catch(error => {
        console.error("请求失败:", error);
        msg.innerText = "Muryo出错啦！服务器连接失败 T . T";
    });
}