function register() {
    const username = document.getElementById("username").value.trim();
    const password = document.getElementById("password").value.trim();
    const confirmPassword = document.getElementById("confirmPassword").value.trim();
    const msg = document.getElementById("msg");

    msg.innerText = "";

    if (!username || !password || !confirmPassword) {
        msg.innerText = "请把信息填写完整哦~";
        return;
    }

    if (password !== confirmPassword) {
        msg.innerText = "两次输入的密码不一致哦~";
        return;
    }

    fetch("http://127.0.0.1:8080/register", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `user_name=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            msg.innerText = data.message || "注册成功~";
            setTimeout(() => {
                window.location.replace ( "login.html");
            }, 1000);
        } else {
            msg.innerText = data.message || "注册失败，请稍后再试~";
        }
    })
    .catch(error => {
        console.error("请求失败:", error);
        msg.innerText = "Muryo出错啦！可能是服务器没开或跨域配置有问题 T . T";
    });
}