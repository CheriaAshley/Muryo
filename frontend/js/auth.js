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
function updateAdminEntry() {
    const user = getCurrentUser();
    const adminEntry = document.getElementById("adminEntry");

    if (!adminEntry) return;

    if (user === null) {
        adminEntry.style.display = "none";
        return;
    }

    fetch(`http://127.0.0.1:8080/admin/me?user_id=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            if (data.success && data.is_admin) {
                localStorage.setItem("is_admin", "1");
                localStorage.setItem("admin_level", data.level);
                adminEntry.style.display = "block";
            } else {
                localStorage.removeItem("is_admin");
                localStorage.removeItem("admin_level");
                adminEntry.style.display = "none";
            }
        })
        .catch(error => {
            console.error("管理员身份查询失败:", error);
            adminEntry.style.display = "none";
        });
}

function goAdminCenter() {
    if (!checkLogin()) return;

    const isAdmin = localStorage.getItem("is_admin");

    if (isAdmin !== "1") {
        alert("你还不是管理员哦~");
        return;
    }

    window.location.href = "admin.html";
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
    window.location.href = "myexchange.html";
}

function goTodoApply() {
    if (!checkLogin()) return;
    window.location.href = "todoapply.html";
}