window.onload = function () {
    if (!checkLogin()) return;

    updateLoginButtons();
    loadMyApplications();
};

function updateLoginButtons() {
    const user = getCurrentUser();

    const loginBtn = document.getElementById("loginBtn");
    const logoutBtn = document.getElementById("logoutBtn");
    const welcomeText = document.getElementById("welcomeText");

    if (user.user_id && user.user_name) {
        if (loginBtn) loginBtn.style.display = "none";
        if (logoutBtn) logoutBtn.style.display = "inline-block";
        if (welcomeText) welcomeText.innerText = "欢迎你，" + user.user_name;
    } else {
        if (loginBtn) loginBtn.style.display = "inline-block";
        if (logoutBtn) logoutBtn.style.display = "none";
        if (welcomeText) welcomeText.innerText = "";
    }
}

function loadMyApplications() {
    const user = getCurrentUser();
    const applyList = document.getElementById("applyList");

    if (!user || !user.user_id) {
        applyList.innerHTML = `<div class="empty-box"><p>请先登录后查看我的申请</p></div>`;
        return;
    }

    fetch(`http://127.0.0.1:8080/exchange/outgoing?uto=${user.user_id}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                applyList.innerHTML = `<div class="empty-box"><p>${data.message}</p></div>`;
                return;
            }

            const applications = data.data || [];

            if (applications.length === 0) {
                applyList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">你还没有提交过任何申请，快去寻找心仪的制品吧！</p>
                    </div>
                `;
                return;
            }

            let html = "";

            applications.forEach(app => {
                const statusInfo = getStatusInfo(app.status);

                html += `
                    <div class="apply-card ${statusInfo.cardClass}">
                        <div class="status-badge ${statusInfo.badgeClass}">${statusInfo.text}</div>
                        <h3>${escapeHtml(app.item_name || "未知制品")}</h3>
                        <p><span>申请明细编号：</span>${app.detail_id}</p>
                        <p><span>交换编号：</span>${app.exchange_id}</p>
                        <p><span>制品编号：</span>${app.item_id}</p>
                        <p><span>申请数量：</span>${app.apply_quantity}</p>
                        <p><span>制品余量：</span>${app.left_quantity}</p>
                        <p><span>状态码：</span>${app.status}</p>
                    </div>
                `;
            });

            applyList.innerHTML = html;
        })
        .catch(error => {
            console.error("加载我的申请失败：", error);
            applyList.innerHTML = `
                <div class="empty-box">
                    <p>加载失败，请检查后端是否启动，或接口地址是否正确。</p>
                </div>
            `;
        });
}

function getStatusInfo(status) {
    status = Number(status);

    if (status === 0) {
        return {
            text: "待处理",
            badgeClass: "status-pending",
            cardClass: "pending"
        };
    } else if (status === 1) {
        return {
            text: "已拒绝",
            badgeClass: "status-rejected",
            cardClass: "rejected"
        };
    } else if (status === 2) {
        return {
            text: "已同意待交换",
            badgeClass: "status-accepted",
            cardClass: "accepted"
        };
    } else if (status === 3) {
        return {
            text: "已完成",
            badgeClass: "status-finished",
            cardClass: "finished"
    };
    } else if (status === 4) {
        return {
            text: "已取消",
            badgeClass: "status-discard",
            cardClass: "discard"
        };
    } else {
        return {
            text: "未知状态",
            badgeClass: "status-pending",
            cardClass: "pending"
        };
    }
}

function escapeHtml(str) {
    if (str === null || str === undefined) return "";
    return String(str)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#39;");
}

function goHome() {
    window.location.href = "index.html";
}

function goLogin() {
    window.location.href = "login.html";
}

function goMyPage() {
    window.location.href = "mypage.html";
}

function goMyExchange() {
    window.location.href = "myexchange.html";
}

function goMyTrade() {
    window.location.href = "mytrade.html";
}

function goTodoApply() {
    window.location.href = "todoapply.html";
}