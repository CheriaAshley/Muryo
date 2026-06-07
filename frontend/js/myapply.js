document.addEventListener("DOMContentLoaded", function () {
    // 初始化页面
    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadMyApplications();

    // 绑定弹窗按钮事件
    const closeModalBtn = document.getElementById("closeEditModal");
    if (closeModalBtn) {
        closeModalBtn.addEventListener("click", closeEditModal);
    }

    const submitEditBtn = document.getElementById("submitEdit");
    if (submitEditBtn) {
        submitEditBtn.addEventListener("click", submitEdit);
    }

    // 使用事件委托处理 apply-list 里的按钮点击
    const applyList = document.getElementById("applyList");
    if (applyList) {
        applyList.addEventListener("click", function (event) {
            const editBtn = event.target.closest(".btn-edit");
            const cancelBtn = event.target.closest(".btn-cancel");

            if (editBtn) {
                // 从按钮 dataset 读取 exchange_id、date、location
                const exchangeId = editBtn.dataset.exchangeId;
                const oldDate = editBtn.dataset.oldDate || "";
                const oldLocation = editBtn.dataset.oldLocation || "";
                openEditModal(exchangeId, oldDate, oldLocation);
                return;
            }

            if (cancelBtn) {
                const exchangeId = cancelBtn.dataset.exchangeId;
                if (exchangeId) cancelApplication(exchangeId);
                return;
            }
        });
    }
});

async function loadMyApplications() {
    const user = getCurrentUser();
    const applyList = document.getElementById("applyList");
    const applyCountText = document.getElementById("applyCountText");

    if (!applyList || !applyCountText) return;

    if (!user || !user.user_id) {
        applyList.innerHTML = `<div class="empty-box"><p>请先登录后查看我的申请</p></div>`;
        applyCountText.innerText = "共 0 条申请";
        return;
    }

    applyList.innerHTML = `<p class="loading-text">正在加载我的申请...</p>`;
    applyCountText.innerText = "正在统计...";

    try {
        const response = await fetch(`http://127.0.0.1:8080/exchange/outgoing?uto=${encodeURIComponent(user.user_id)}`);
        if (!response.ok) throw new Error("服务器响应异常");

        const data = await response.json();

        if (!data.success) {
            applyList.innerHTML = `<div class="empty-box"><p>${data.message || "加载失败"}</p></div>`;
            applyCountText.innerText = "共 0 条申请";
            return;
        }

        const applications = Array.isArray(data.data) ? data.data : [];
        applyCountText.innerText = "共 " + applications.length + " 条申请";

        if (applications.length === 0) {
            applyList.innerHTML = `<div class="empty-box"><p class="empty-text">${data.message || "咪还没有提交过任何申请！"}</p></div>`;
            return;
        }

        let html = "";

        applications.forEach(app => {
            const statusInfo = getStatusInfo(app.status, app.status_text);
            const displayDate = (app.date && app.date !== 0 && app.date !== '0') ? app.date : "待确定";
            const displayLocation = (app.location && app.location !== '待确定') ? escapeHtml(app.location) : "待确定";

            let actionBtnHtml = "";

            // 待处理(0)或已同意待交换(2) 可以编辑
            if (app.status == 0 || app.status == 2) {
                actionBtnHtml += `<button class="btn-edit" data-exchange-id="${app.exchange_id}" data-old-date="${app.date || ''}" data-old-location="${app.location || ''}">编辑时间地点</button>`;
            }

            if (app.status == 0) {
                actionBtnHtml += `<button class="btn-cancel" data-exchange-id="${app.exchange_id}">取消申请</button>`;
            }

            html += `
                <div class="apply-card ${statusInfo.cardClass}">
                    <div class="status-badge ${statusInfo.badgeClass}">${escapeHtml(statusInfo.text)}</div>

                    <h3>${escapeHtml(app.item_name || "未知制品")}</h3>

                    <p><span>申请明细编号：</span>${app.detail_id}</p>
                    <p><span>交换编号：</span>${app.exchange_id}</p>
                    <p><span>制品编号：</span>${app.item_id}</p>
                    <p><span>申请对象：</span>${escapeHtml(app.target_user_name || "未知用户")}</p>
                    <p><span>申请数量：</span>${app.apply_quantity}</p>
                    <p><span>制品余量：</span>${app.left_quantity}</p>
                    <p><span>交换日期：</span><strong style="color:#d4a373;">${displayDate}</strong></p>
                    <p><span>交换地点：</span><strong style="color:#d4a373;">${displayLocation}</strong></p>
                    <p><span>对方联系方式：</span><strong style="color:#e07a5f;">${escapeHtml(app.target_user_phone || "同意后可见")}</strong></p>
                    
                    <div class="card-actions">
                        ${actionBtnHtml}
                    </div>
                </div>
            `;
        });

        applyList.innerHTML = html;

    } catch (error) {
        console.error("加载我的申请失败：", error);
        applyList.innerHTML = `<div class="empty-box"><p>服务器连接失败，请检查后端是否启动哦~</p></div>`;
        applyCountText.innerText = "共 0 条申请";
    }
}

function getStatusInfo(status, statusText) {
    status = Number(status);

    if (status === 0) return { text: statusText || "待处理", badgeClass: "status-pending", cardClass: "pending" };
    if (status === 1) return { text: statusText || "已拒绝", badgeClass: "status-rejected", cardClass: "rejected" };
    if (status === 2) return { text: statusText || "已同意待交换", badgeClass: "status-accepted", cardClass: "accepted" };
    if (status === 3) return { text: statusText || "已完成", badgeClass: "status-finished", cardClass: "finished" };
    if (status === 4) return { text: statusText || "已取消", badgeClass: "status-discard", cardClass: "discard" };
    return { text: statusText || "未知状态", badgeClass: "status-pending", cardClass: "pending" };
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

// 取消申请功能
async function cancelApplication(exchangeId) {
    const user = getCurrentUser();
    if (!user || !user.user_id) {
        alert("请先登录！");
        return;
    }

    if (!confirm("确定要取消这个交换申请吗？此操作不可逆哦~")) return;

    const params = new URLSearchParams();
    params.append("exchange_id", exchangeId);
    params.append("action", "cancel"); 
    params.append("user_id", user.user_id);

    try {
        const response = await fetch("http://127.0.0.1:8080/exchange/handle", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params.toString()
        });
        
        const data = await response.json();
        
        if (data.success) {
            alert("申请已取消！");
            loadMyApplications(); 
        } else {
            alert("取消失败: " + (data.message || "未知错误"));
        }
    } catch (error) {
        console.error("取消申请请求失败:", error);
        alert("网络连接失败，请检查后端是否启动~");
    }
}

// 弹窗逻辑
function openEditModal(exchangeId, oldDate, oldLocation) {
    const modal = document.getElementById("editModal");
    if (!modal) return;

    document.getElementById("editExchangeId").value = exchangeId;
    document.getElementById("editDate").value = (oldDate && oldDate !== '0') ? oldDate : "";
    document.getElementById("editLocation").value = (oldLocation && oldLocation !== '待确定') ? oldLocation : "";

    modal.style.display = "flex"; // 或 block，根据你的 CSS
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

    const exchangeId = document.getElementById("editExchangeId").value;
    const newDate = document.getElementById("editDate").value;
    const newLocation = document.getElementById("editLocation").value;

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
        const response = await fetch("http://127.0.0.1:8080/exchange/update_info", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params.toString()
        });
        
        const data = await response.json();
        
        if (data.success) {
            alert("交换时间和地点修改成功！");
            closeEditModal();
            loadMyApplications();
        } else {
            alert("修改失败: " + (data.message || "未知错误"));
        }
    } catch (error) {
        console.error("修改时间地点请求失败:", error);
        alert("网络连接失败，请检查后端服务是否启动~");
    }
}