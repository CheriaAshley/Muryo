const BASE_URL = "http://127.0.0.1:8080";

document.addEventListener("DOMContentLoaded", function () {
    bindTodoApplyEvents();

    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadTodoApply();
});

/**
 * 统一绑定页面事件
 */
function bindTodoApplyEvents() {
    bindClick("loginBtn", goLogin);
    bindClick("logoutBtn", logout);

    bindClick("homeNav", goHome);
    bindClick("myPageNav", goMyPage);
    bindClick("myApplyNav", goMyApply);
    bindClick("myExchangeNav", goMyExchange);
    bindClick("todoApplyNav", goTodoApply);
    bindClick("adminEntry", goAdminCenter);

    bindKeyboardClick("homeNav");
    bindKeyboardClick("myPageNav");
    bindKeyboardClick("myApplyNav");
    bindKeyboardClick("myExchangeNav");
    bindKeyboardClick("todoApplyNav");
    bindKeyboardClick("adminEntry");

    const todoList = document.getElementById("todoList");

    if (todoList) {
        todoList.addEventListener("click", handleTodoListClick);
    }
}

/**
 * 简化点击事件绑定
 */
function bindClick(id, handler) {
    const element = document.getElementById(id);

    if (!element) return;

    element.addEventListener("click", handler);
}

/**
 * 让 li 菜单支持键盘 Enter / 空格触发
 */
function bindKeyboardClick(id) {
    const element = document.getElementById(id);

    if (!element) return;

    element.addEventListener("keydown", function (event) {
        if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            element.click();
        }
    });
}

/**
 * 处理动态生成的待办按钮
 */
function handleTodoListClick(event) {
    const button = event.target.closest("[data-action]");

    if (!button) return;

    const exchangeId = button.dataset.exchangeId;
    const action = button.dataset.action;

    if (!exchangeId || !action) {
        alert("缺少操作参数");
        return;
    }

    handleExchange(exchangeId, action);
}

function loadTodoApply() {
    const user = getCurrentUser();
    const todoList = document.getElementById("todoList");
    const todoCountText = document.getElementById("todoCountText");

    if (!todoList) return;

    if (!user || !user.user_id) {
        todoList.innerHTML = `
            <div class="empty-box">
                <p class="empty-text">请先登录后查看待办申请</p>
            </div>
        `;

        if (todoCountText) {
            todoCountText.innerText = "共 0 条待办";
        }

        return;
    }

    todoList.innerHTML = `<p class="loading-text">正在加载待办申请...</p>`;

    if (todoCountText) {
        todoCountText.innerText = "正在统计...";
    }

    fetch(`${BASE_URL}/exchange/todo?ufrom=${encodeURIComponent(user.user_id)}`)
        .then(function (response) {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }

            return response.json();
        })
        .then(function (data) {
            if (!data.success) {
                todoList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">${escapeHtml(data.message || "加载失败")}</p>
                    </div>
                `;

                if (todoCountText) {
                    todoCountText.innerText = "共 0 条待办";
                }

                return;
            }

            const list = Array.isArray(data.data) ? data.data : [];

            if (todoCountText) {
                todoCountText.innerText = "共 " + list.length + " 条待办";
            }

            if (list.length === 0) {
                todoList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">${escapeHtml(data.message || "暂时没有待办申请哦~")}</p>
                    </div>
                `;

                return;
            }

            renderTodoList(list);
        })
        .catch(function (error) {
            console.error("加载待办申请失败：", error);

            todoList.innerHTML = `
                <div class="empty-box">
                    <p class="empty-text">服务器连接失败，请检查后端是否启动哦~</p>
                </div>
            `;

            if (todoCountText) {
                todoCountText.innerText = "共 0 条待办";
            }
        });
}

function renderTodoList(list) {
    const todoList = document.getElementById("todoList");

    if (!todoList) return;

    let html = "";

    list.forEach(function (item) {
        const statusInfo = getStatusInfo(item.status, item.status_text);

        html += `
            <div class="apply-card ${escapeHtml(statusInfo.cardClass)}">
                <div class="status-badge ${escapeHtml(statusInfo.badgeClass)}">
                    ${escapeHtml(statusInfo.text)}
                </div>

                <h3>${escapeHtml(item.item_name || "未知制品")}</h3>

                <p><span>明细编号：</span>${escapeHtml(item.detail_id)}</p>
                <p><span>交换编号：</span>${escapeHtml(item.exchange_id)}</p>
                <p><span>申请人ID：</span>${escapeHtml(item.applicant_id)}</p>
                <p><span>申请人昵称：</span>${escapeHtml(item.applicant_name || "未知用户")}</p>
                <p><span>制品编号：</span>${escapeHtml(item.item_id)}</p>
                <p><span>申请数量：</span>${escapeHtml(item.apply_quantity)}</p>
                <p><span>当前余量：</span>${escapeHtml(item.left_quantity)}</p>

                <div class="card-actions">
                    ${renderActionButtons(item)}
                </div>
            </div>
        `;
    });

    todoList.innerHTML = html;
}

function renderActionButtons(item) {
    const status = Number(item.status);
    const exchangeId = escapeHtml(item.exchange_id);

    if (status === 0) {
        return `
            <button
                class="action-btn btn-agree"
                data-action="agree"
                data-exchange-id="${exchangeId}"
            >
                同意
            </button>

            <button
                class="action-btn btn-reject"
                data-action="reject"
                data-exchange-id="${exchangeId}"
            >
                拒绝
            </button>
        `;
    }

    if (status === 2) {
        return `
            <button
                class="action-btn btn-complete"
                data-action="complete"
                data-exchange-id="${exchangeId}"
            >
                已完成
            </button>

            <button
                class="action-btn btn-cancel"
                data-action="cancel"
                data-exchange-id="${exchangeId}"
            >
                取消
            </button>
        `;
    }

    return "";
}

function handleExchange(exchangeId, action) {
    const user = getCurrentUser();

    if (!user || !user.user_id) {
        alert("请先登录！");
        return;
    }

    if (action === "cancel" && !confirm("确定要取消这个交换吗？")) {
        return;
    }

    if (action === "reject" && !confirm("确定要拒绝这个交换申请吗？")) {
        return;
    }

    if (action === "complete" && !confirm("确定这个交换已经完成了吗？")) {
        return;
    }

    const params = new URLSearchParams();
    params.append("exchange_id", exchangeId);
    params.append("action", action);
    params.append("user_id", user.user_id);

    fetch(`${BASE_URL}/exchange/handle`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
        .then(function (response) {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }

            return response.json();
        })
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                loadTodoApply();
            }
        })
        .catch(function (error) {
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