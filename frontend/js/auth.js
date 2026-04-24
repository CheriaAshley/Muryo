function getCurrentUser() {
    return {
        user_id: localStorage.getItem("user_id"),
        user_name: localStorage.getItem("user_name")
    };
}

function checkLogin() {
    const user = getCurrentUser();

    if (!user.user_id || !user.user_name) {
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