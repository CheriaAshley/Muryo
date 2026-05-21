window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    updateLoginState();
    updateAdminEntry();
    loadTodoApply();
};

function loadTodoApply() {
    const user = getCurrentUser();
    const todoList = document.getElementById("todoList");
    const todoCountText = document.getElementById("todoCountText");

    todoList.innerHTML = `<p class="loading-text">正在加载待办申请...</p>`;
    if (todoCountText) todoCountText.innerText = "正在统计...";

    fetch(`http://127.0.0.1:8080/exchange/todo?ufrom=${user.user_id}`)
        .then(response => {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }
            return response.json();
        })
        .then(data => {
            if (!data.success) {
                todoList.innerHTML = `<div class="empty-box"><p class="empty-text">${data.message || "加载失败"}</p></div>`;
                if (todoCountText) todoCountText.innerText = "共 0 条待办";
                return;
            }

            const list = Array.isArray(data.data) ? data.data : [];

             if (todoCountText) {
                todoCountText.innerText = "共 " + list.length + " 条待办";
            }

            if (list.length === 0) {
                todoList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">${data.message || "暂时没有待办申请哦~"}</p>
                    </div>
                `;
                return;
            }

            let html = "";

            list.forEach(item => {
                const statusInfo = getStatusInfo(item.status, item.status_text);

                 html += `
                    <div class="apply-card ${statusInfo.cardClass}">
                        <div class="status-badge ${statusInfo.badgeClass}">
                            ${escapeHtml(statusInfo.text)}
                        </div>

                        <h3>${escapeHtml(item.item_name || "未知制品")}</h3>

                        <p><span>明细编号：</span>${item.detail_id}</p>
                        <p><span>交换编号：</span>${item.exchange_id}</p>
                        <p><span>申请人ID：</span>${item.applicant_id}</p>
                        <p><span>申请人昵称：</span>${escapeHtml(item.applicant_name || "未知用户")}</p>
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
            todoList.innerHTML = `
                <div class="empty-box">
                    <p class="empty-text">服务器连接失败，请检查后端是否启动哦~</p>
                </div>
            `;
            if (todoCountText) todoCountText.innerText = "共 0 条待办";
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

    const params = new URLSearchParams();
    params.append("exchange_id", exchangeId);
    params.append("action", action);
    params.append("user_id", user.user_id);

    fetch("http://127.0.0.1:8080/exchange/handle", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
    .then(response => {
        if (!response.ok) {
            throw new Error("服务器响应异常");
        }
        return response.json();
    })
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            loadTodoApply();
        }
    })
    .catch(error => {
        console.error("处理申请失败：", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function getStatusInfo(status, statusText) {
    status = Number(status);

    if (status === 0) {
        return {
            text: statusText || "待处理",
            cardClass: "pending",
            badgeClass: "status-pending"
        };
    }

    if (status === 2) {
        return {
            text: statusText || "待交换",
            cardClass: "accepted",
            badgeClass: "status-accepted"
        };
    }

    return {
        text: statusText || "未知状态",
        cardClass: "",
        badgeClass: ""
    };
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