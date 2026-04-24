window.onload = function () {
    if (!checkLogin()) return;

    updateLoginButtons();
    loadTodoApply();
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

function loadTodoApply() {
    const user = getCurrentUser();
    const todoList = document.getElementById("todoList");

    if (!user.user_id) {
        todoList.innerHTML = `<div class="empty-box"><p class="empty-text">请先登录后查看待办申请~</p></div>`;
        return;
    }

    fetch(`http://127.0.0.1:8080/exchange/todo?ufrom=${user.user_id}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                todoList.innerHTML = `<div class="empty-box"><p class="empty-text">${data.message}</p></div>`;
                return;
            }

            const list = data.data;

            if (!list || list.length === 0) {
                todoList.innerHTML = `<div class="empty-box"><p class="empty-text">暂时没有待处理或待交换的申请哦~</p></div>`;
                return;
            }

            let html = "";

            list.forEach(item => {
                const statusInfo = getStatusInfo(item.status);

                html += `
                    <div class="apply-card ${statusInfo.cardClass}">
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
                        <div class="card-actions">
                            ${renderActionButtons(item)}
                        </div>
                    </div>
                `;
            });

            todoList.innerHTML = html;
        })
        .catch(error => {
            console.error("加载待办申请失败：", error);
            todoList.innerHTML = `<div class="empty-box"><p class="empty-text">加载失败，请检查后端是否启动~</p></div>`;
        });
}

function renderActionButtons(item) {
    const status = Number(item.status);

    if (status === 0) {
        return `
            <button class="action-btn btn-agree" onclick="handleExchange(${item.exchange_id}, 'agree')">同意</button>
            <button class="action-btn btn-reject" onclick="handleExchange(${item.exchange_id}, 'reject')">拒绝</button>
        `;
    }

    if (status === 2) {
        return `
            <button class="action-btn btn-complete" onclick="handleExchange(${item.exchange_id}, 'complete')">已完成</button>
        `;
    }

    return "";
}

function handleExchange(exchangeId, action) {
    const user = getCurrentUser();

    fetch("http://127.0.0.1:8080/exchange/handle", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `exchange_id=${exchangeId}&action=${action}&ufrom=${user.user_id}`
    })
        .then(response => response.json())
        .then(data => {
            alert(data.message);
            if (data.success) {
                loadTodoApply();
            }
        })
        .catch(error => {
            console.error("处理申请失败：", error);
            alert("处理失败，请检查后端是否启动~");
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
        case 2:
            return {
                text: "待交换",
                cardClass: "accepted",
                badgeClass: "status-accepted"
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