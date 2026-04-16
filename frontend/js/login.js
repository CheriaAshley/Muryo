function login() {
    const username = document.getElementById("username").value;
    const password = document.getElementById("password").value;

    if (!username || !password) {
        alert("咪请输入用户名和密码");
        return;
    }

    fetch("http://127.0.0.1:8080/login", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({
            username: username,
            password: password
        })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            alert("登录成功！");

            // 保存登录状态
            localStorage.setItem("username", username);

            // 跳转首页
            window.location.href = "index.html";
        } else {
            alert(data.message);
        }
    })
    .catch(error => {
        console.error("请求失败:", error);
        alert("Muryo出错啦！服务器连接失败T . T");
    });
}