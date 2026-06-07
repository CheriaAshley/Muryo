const BASE_URL = "http://127.0.0.1:8080";

/**
 * 获取当前登录用户信息
 */
const getCurrentUser = () => {
    const user_id = localStorage.getItem("user_id");
    const user_name = localStorage.getItem("user_name");

    if (!user_id || !user_name || user_id === "undefined" || user_id === "null" || user_name === "undefined" || user_name === "null") {
        return null;
    }

    return { user_id, user_name };
};

/**
 * 判断用户是否登录，如果未登录会自动跳转登录页
 */
const checkLogin = () => {
    const user = getCurrentUser();
    if (!user) {
        alert("请先登录");
        window.location.href = "login.html";
        return false;
    }
    return true;
};

/**
 * 登出当前用户
 */
const logout = () => {
    localStorage.removeItem("user_id");
    localStorage.removeItem("user_name");
    localStorage.removeItem("is_admin");
    localStorage.removeItem("admin_level");

    alert("已退出登录");
    window.location.href = "login.html";
};

/**
 * 更新页面顶部登录状态
 */
const updateLoginState = () => {
    const user = getCurrentUser();
    const welcomeText = document.getElementById("welcomeText");
    const loginBtn = document.getElementById("loginBtn");
    const logoutBtn = document.getElementById("logoutBtn");

    if (!user) {
        if (welcomeText) welcomeText.innerText = "";
        if (loginBtn) loginBtn.style.display = "inline-block";
        if (logoutBtn) logoutBtn.style.display = "none";
    } else {
        if (welcomeText) welcomeText.innerText = `你好，${user.user_name}`;
        if (loginBtn) loginBtn.style.display = "none";
        if (logoutBtn) logoutBtn.style.display = "inline-block";
    }
};

/**
 * 查询管理员身份并显示入口
 */
const updateAdminEntry = () => {
    const user = getCurrentUser();
    const adminEntry = document.getElementById("adminEntry");
    if (!adminEntry) return;

    if (!user) {
        adminEntry.style.display = "none";
        return;
    }

    fetch(`${BASE_URL}/admin/me?user_id=${encodeURIComponent(user.user_id)}`)
        .then(res => res.json())
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
        .catch(err => {
            console.error("管理员身份查询失败:", err);
            adminEntry.style.display = "none";
        });
};

/**
 * 页面跳转函数
 */
const goAdminCenter = () => {
    if (!checkLogin()) return;
    const isAdmin = localStorage.getItem("is_admin");
    if (isAdmin !== "1") {
        alert("你还不是管理员哦~");
        return;
    }
    window.location.href = "admin.html";
};

const goLogin = () => window.location.href = "login.html";
const goHome = () => window.location.href = "index.html";
const goMyPage = () => checkLogin() && (window.location.href = "mypage.html");
const goMyApply = () => checkLogin() && (window.location.href = "myapply.html");
const goMyExchange = () => checkLogin() && (window.location.href = "myexchange.html");
const goTodoApply = () => checkLogin() && (window.location.href = "todoapply.html");