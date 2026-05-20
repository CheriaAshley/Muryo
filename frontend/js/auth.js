function getCurrentUser() {
    const user_id = localStorage.getItem("user_id");
    const user_name = localStorage.getItem("user_name");

    if (
        !user_id ||
        !user_name ||
        user_id === "undefined" ||
        user_id === "null" ||
        user_name === "undefined" ||
        user_name === "null"
    ) {
        return null;
    }

    return {
        user_id: user_id,
        user_name: user_name
    };
}

function checkLogin() {
    const user = getCurrentUser();

    if (user === null) {
        alert("请先登录");
        window.location.href = "login.html";
        return false;
    }

    return true;
}

function logout() {
    localStorage.removeItem("user_id");
    localStorage.removeItem("user_name");
    alert("已退出登录");
    window.location.href = "login.html";
}

function updateLoginState() {
    const user = getCurrentUser();

    const welcomeText = document.getElementById("welcomeText");
    const loginBtn = document.getElementById("loginBtn");
    const logoutBtn = document.getElementById("logoutBtn");

    if (user === null) {
        if (welcomeText) {
            welcomeText.innerText = "";
        }

        if (loginBtn) {
            loginBtn.style.display = "inline-block";
        }

        if (logoutBtn) {
            logoutBtn.style.display = "none";
        }
    } else {
        if (welcomeText) {
            welcomeText.innerText = "你好，" + user.user_name;
        }

        if (loginBtn) {
            loginBtn.style.display = "none";
        }

        if (logoutBtn) {
            logoutBtn.style.display = "inline-block";
        }
    }
}

function goLogin() {
    window.location.href = "login.html";
}

function goHome() {
    window.location.href = "index.html";
}

function goMyPage() {
    if (!checkLogin()) return;
    window.location.href = "mypage.html";
}

function goMyApply() {
    if (!checkLogin()) return;
    window.location.href = "myapply.html";
}

function goMyExchange() {
    if (!checkLogin()) return;
    window.location.href = "mytrade.html";
}

function goTodoApply() {
    if (!checkLogin()) return;
    window.location.href = "todoapply.html";
}