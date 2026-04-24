window.onload = function () {
    if (!checkLogin()) return;

    updateLoginButtons();
    loadMyTrade();
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

function loadMyTrade() {
    const user = getCurrentUser();
    const tradeList = document.getElementById("tradeList");

    if (!user.user_id) {
        tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">请先登录后查看我的交换~</p></div>`;
        return;
    }

    fetch(`http://127.0.0.1:8080/exchange/incoming?ufrom=${user.user_id}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">${data.message}</p></div>`;
                return;
            }

            const list = data.data;

            if (!list || list.length === 0) {
                tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">还没有人向你发起交换申请哦~</p></div>`;
                return;
            }

            let html = "";

            list.forEach(item => {
                const statusInfo = getStatusInfo(item.status);

                html += `
                    <div class="trade-card ${statusInfo.cardClass}">
                        <div class="status-badge ${statusInfo.badgeClass}">
                            ${statusInfo.text}
                        </div>
                        <h3>${item.item_name || "未知制品"}</h3>
                        <p><span>明细编号：</span>${item.detail_id}</p>
                        <p><span>交换编号：</span>${item.exchange_id}</p>
                        <p><span>申请人ID：</span>${item.applicant_id}</p>
                        <p><span>申请人昵称：</span>${item.applicant_name || "未知用户"}</p>
                        <p><span>制品编号：</span>${item.item_id}</p>
                        <p><span>申请数量：</span>${item.apply_quantity}</p>
                        <p><span>当前余量：</span>${item.left_quantity}</p>
                    </div>
                `;
            });

            tradeList.innerHTML = html;
        })
        .catch(error => {
            console.error("加载我的交换失败：", error);
            tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">加载失败，请检查后端是否启动或接口是否正确~</p></div>`;
        });
}

function getStatusInfo(status) {
    switch (Number(status)) {
        case 0:
            return {
                text: "待处理",
                cardClass: "pending",
                badgeClass: "status-pending"
            };
        case 1:
            return {
                text: "已拒绝",
                cardClass: "rejected",
                badgeClass: "status-rejected"
            };
        case 2:
            return {
                text: "已同意未交换",
                cardClass: "accepted",
                badgeClass: "status-accepted"
            };
        case 3:
            return {
                text: "已完成",
                cardClass: "finished",
                badgeClass: "status-finished"
            };
        case 4:
            return {
                text: "已取消",
                cardClass: "discard",
                badgeClass: "status-discard"
            };
        default:
            return {
                text: "未知状态",
                cardClass: "",
                badgeClass: ""
            };
    }
}

/* 页面跳转函数 */
function goHome() {
    window.location.href = "index.html";
}

function goLogin() {
    window.location.href = "login.html";
}

function goMyPage() {
    window.location.href = "mypage.html";
}

function goMyApply() {
    window.location.href = "myapply.html";
}

function goMyTrade() {
    window.location.href = "mytrade.html";
}

function goTodoApply() {
    window.location.href = "todoapply.html";
}