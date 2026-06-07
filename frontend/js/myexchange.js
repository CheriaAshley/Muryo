const BASE_URL = "http://127.0.0.1:8080";

document.addEventListener("DOMContentLoaded", function () {
    bindMyExchangeEvents();

    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadMyExchange();
});

/**
 * 统一绑定页面事件
 */
function bindMyExchangeEvents() {
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

    bindClick("closeEditModal", closeEditModal);
    bindClick("submitEditBtn", submitEdit);

    const tradeList = document.getElementById("tradeList");

    if (tradeList) {
        tradeList.addEventListener("click", handleTradeListClick);
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
 * 让 li 菜单支持 Enter / 空格触发
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
 * 处理动态生成的交换卡片按钮
 */
function handleTradeListClick(event) {
    const editBtn = event.target.closest(".btn-edit");

    if (!editBtn) return;

    const exchangeId = editBtn.dataset.exchangeId;
    const oldDate = editBtn.dataset.oldDate || "";
    const oldLocation = editBtn.dataset.oldLocation || "";

    openEditModal(exchangeId, oldDate, oldLocation);
}

function loadMyExchange() {
    const user = getCurrentUser();
    const tradeList = document.getElementById("tradeList");
    const tradeCountText = document.getElementById("tradeCountText");

    if (!tradeList) return;

    if (!user || !user.user_id) {
        tradeList.innerHTML = `
            <div class="empty-box">
                <p class="empty-text">请先登录后查看我的交换</p>
            </div>
        `;

        if (tradeCountText) {
            tradeCountText.innerText = "共 0 条申请";
        }

        return;
    }

    tradeList.innerHTML = `<p class="loading-text">正在加载我的交换...</p>`;

    if (tradeCountText) {
        tradeCountText.innerText = "正在统计...";
    }

    fetch(`${BASE_URL}/exchange/incoming?ufrom=${encodeURIComponent(user.user_id)}`)
        .then(function (response) {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }

            return response.json();
        })
        .then(function (data) {
            if (!data.success) {
                tradeList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">${escapeHtml(data.message || "加载失败")}</p>
                    </div>
                `;

                if (tradeCountText) {
                    tradeCountText.innerText = "共 0 条申请";
                }

                return;
            }

            const list = Array.isArray(data.data) ? data.data : [];

            if (tradeCountText) {
                tradeCountText.innerText = "共 " + list.length + " 条申请";
            }

            if (list.length === 0) {
                tradeList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">${escapeHtml(data.message || "暂时没有收到交换申请")}</p>
                    </div>
                `;

                return;
            }

            renderTradeList(list);
        })
        .catch(function (error) {
            console.error("加载我的交换失败：", error);

            tradeList.innerHTML = `
                <div class="empty-box">
                    <p class="empty-text">服务器连接失败，请检查后端是否启动哦~</p>
                </div>
            `;

            if (tradeCountText) {
                tradeCountText.innerText = "共 0 条申请";
            }
        });
}

function renderTradeList(list) {
    const tradeList = document.getElementById("tradeList");

    if (!tradeList) return;

    let html = "";

    list.forEach(function (item) {
        const statusInfo = getStatusInfo(item.status, item.status_text);

        const displayDate =
            item.date && item.date !== 0 && item.date !== "0"
                ? item.date
                : "待确定";

        const displayLocation =
            item.location && item.location !== "待确定"
                ? escapeHtml(item.location)
                : "待确定";

        let actionBtnHtml = "";

        if (item.status == 0 || item.status == 2) {
            actionBtnHtml += `
                <button
                    class="btn-edit"
                    data-exchange-id="${escapeHtml(item.exchange_id)}"
                    data-old-date="${escapeHtml(item.date || "")}"
                    data-old-location="${escapeHtml(item.location || "")}"
                >
                    编辑时间地点
                </button>
            `;
        }

        html += `
            <div class="trade-card ${escapeHtml(statusInfo.cardClass)}">
                <div class="status-badge ${escapeHtml(statusInfo.badgeClass)}">
                    ${escapeHtml(statusInfo.text)}
                </div>

                <img
                    class="trade-img_url"
                    src="${escapeHtml(item.img_url || "upload/default_item.png")}"
                    alt="制品图片"
                >

                <h3>${escapeHtml(item.item_name || "未知制品")}</h3>

                <p><span>明细编号：</span>${escapeHtml(item.detail_id)}</p>
                <p><span>交换编号：</span>${escapeHtml(item.exchange_id)}</p>
                <p><span>申请人：</span>${escapeHtml(item.apply_user_name || "未知用户")}</p>
                <p><span>制品编号：</span>${escapeHtml(item.item_id)}</p>
                <p><span>申请数量：</span>${escapeHtml(item.apply_quantity)}</p>
                <p><span>当前余量：</span>${escapeHtml(item.left_quantity)}</p>
                <p><span>交换日期：</span><strong class="important-date">${displayDate}</strong></p>
                <p><span>交换地点：</span><strong class="important-date">${displayLocation}</strong></p>
                <p><span>对方联系方式：</span><strong class="important-phone">${escapeHtml(item.apply_user_phone || "同意后可见")}</strong></p>

                <div class="card-actions">
                    ${actionBtnHtml}
                </div>
            </div>
        `;
    });

    tradeList.innerHTML = html;
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

    if (status === 1) {
        return {
            text: statusText || "已拒绝",
            cardClass: "rejected",
            badgeClass: "status-rejected"
        };
    }

    if (status === 2) {
        return {
            text: statusText || "已同意未交换",
            cardClass: "accepted",
            badgeClass: "status-accepted"
        };
    }

    if (status === 3) {
        return {
            text: statusText || "已完成",
            cardClass: "finished",
            badgeClass: "status-finished"
        };
    }

    if (status === 4) {
        return {
            text: statusText || "已取消",
            cardClass: "discard",
            badgeClass: "status-discard"
        };
    }

    return {
        text: statusText || "未知状态",
        cardClass: "",
        badgeClass: ""
    };
}

function openEditModal(exchangeId, oldDate, oldLocation) {
    const modal = document.getElementById("editModal");
    const editExchangeId = document.getElementById("editExchangeId");
    const editDate = document.getElementById("editDate");
    const editLocation = document.getElementById("editLocation");

    if (!modal || !editExchangeId || !editDate || !editLocation) return;

    editExchangeId.value = exchangeId;

    if (oldDate && oldDate !== "0") {
        editDate.value = oldDate;
    } else {
        editDate.value = "";
    }

    if (oldLocation && oldLocation !== "待确定") {
        editLocation.value = oldLocation;
    } else {
        editLocation.value = "";
    }

    modal.style.display = "flex";
}

function closeEditModal() {
    const modal = document.getElementById("editModal");

    if (!modal) return;

    modal.style.display = "none";
}

async function submitEdit() {
    const user = getCurrentUser();

    if (!user || !user.user_id) {
        alert("请先登录！");
        return;
    }

    const exchangeId = document.getElementById("editExchangeId").value.trim();
    const newDate = document.getElementById("editDate").value.trim();
    const newLocation = document.getElementById("editLocation").value.trim();

    if (!exchangeId) {
        alert("缺少交换编号");
        return;
    }

    if (!newDate || !newLocation) {
        alert("交换时间和交换地点都必须填写哦！");
        return;
    }

    const params = new URLSearchParams();
    params.append("exchange_id", exchangeId);
    params.append("user_id", user.user_id);
    params.append("date", newDate);
    params.append("location", newLocation);

    try {
        const response = await fetch(`${BASE_URL}/exchange/update_info`, {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: params.toString()
        });

        const data = await response.json();

        if (data.success) {
            alert("交换时间和地点修改成功！");
            closeEditModal();
            loadMyExchange();
        } else {
            alert("修改失败: " + (data.message || "未知错误"));
        }
    } catch (error) {
        console.error("修改时间地点请求失败:", error);
        alert("网络连接失败，请检查后端服务是否启动~");
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