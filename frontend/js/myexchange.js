window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadMyExchange();
};

function loadMyExchange() {
    const user = getCurrentUser();
    const tradeList = document.getElementById("tradeList");
    const tradeCountText = document.getElementById("tradeCountText");

    tradeList.innerHTML = `<p class="loading-text">正在加载我的交换...</p>`;
    if (tradeCountText) tradeCountText.innerText = "正在统计...";

    fetch(`http://127.0.0.1:8080/exchange/incoming?ufrom=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">${data.message || "加载失败"}</p></div>`;
                if (tradeCountText) tradeCountText.innerText = "共 0 条申请";
                return;
            }

            const list = Array.isArray(data.data) ? data.data : [];

            if (tradeCountText) {
                tradeCountText.innerText = "共 " + list.length + " 条申请";
            }

            if (list.length === 0) {
                tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">${data.message || "暂时没有收到交换申请"}</p></div>`;
                return;
            }

            let html = "";

            list.forEach(item => {
                const statusInfo = getStatusInfo(item.status, item.status_text);

                // --- 新增：处理时间和地点的显示 ---
                let displayDate = (item.date && item.date !== 0 && item.date !== '0') ? item.date : "待确定";
                let displayLocation = (item.location && item.location !== '待确定') ? escapeHtml(item.location) : "待确定";

                // --- 新增：编辑按钮 ---
                let actionBtnHtml = "";
                // 如果是待处理(0)或已同意待交换(2)，都可以编辑时间地点
                if (item.status == 0 || item.status == 2) {
                    actionBtnHtml += `<button class="btn-edit" onclick="openEditModal(${item.exchange_id}, '${item.date || ''}', '${item.location || ''}')">编辑时间地点</button>`;
                }

                html += `
                    <div class="trade-card ${statusInfo.cardClass}">
                        <div class="status-badge ${statusInfo.badgeClass}">
                            ${escapeHtml(statusInfo.text)}
                        </div>

                        <img class="trade-img_url" src="${escapeHtml(item.img_url || 'upload/default_item.png')}" alt="制品图片">
                        <h3>${escapeHtml(item.item_name || "未知制品")}</h3>
                        <p><span>明细编号：</span>${item.detail_id}</p>
                        <p><span>交换编号：</span>${item.exchange_id}</p>
                        <p><span>申请人：</span>${escapeHtml(item.apply_user_name || "未知用户")}</p>
                        <p><span>制品编号：</span>${item.item_id}</p>
                        <p><span>申请数量：</span>${item.apply_quantity}</p>
                        <p><span>当前余量：</span>${item.left_quantity}</p>
                        <p><span>交换日期：</span><strong style="color:#d4a373;">${displayDate}</strong></p>
                        <p><span>交换地点：</span><strong style="color:#d4a373;">${displayLocation}</strong></p>
                        <p><span>对方联系方式：</span><strong style="color:#e07a5f;">${escapeHtml(item.apply_user_phone || "同意后可见")}</strong></p>
                        
                        <div class="card-actions">
                            ${actionBtnHtml}
                        </div>
                    </div>
                `;
            });

            tradeList.innerHTML = html;
        })
        .catch(error => {
            console.error("加载我的交换失败：", error);
            tradeList.innerHTML = `<div class="empty-box"><p class="empty-text">服务器连接失败，请检查后端是否启动哦~</p></div>`;
            if (tradeCountText) tradeCountText.innerText = "共 0 条申请";
        });
}

function getStatusInfo(status, statusText) {
    status = Number(status);

    if (status === 0) return { text: statusText || "待处理", cardClass: "pending", badgeClass: "status-pending" };
    if (status === 1) return { text: statusText || "已拒绝", cardClass: "rejected", badgeClass: "status-rejected" };
    if (status === 2) return { text: statusText || "已同意未交换", cardClass: "accepted", badgeClass: "status-accepted" };
    if (status === 3) return { text: statusText || "已完成", cardClass: "finished", badgeClass: "status-finished" };
    if (status === 4) return { text: statusText || "已取消", cardClass: "discard", badgeClass: "status-discard" };

    return { text: statusText || "未知状态", cardClass: "", badgeClass: "" };
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

/* =========================================
   新增：编辑交换时间和地点的弹窗逻辑
   ========================================= */

// 1. 打开弹窗并回显数据
function openEditModal(exchangeId, oldDate, oldLocation) {
    document.getElementById("editExchangeId").value = exchangeId;
    
    document.getElementById("editDate").value = (oldDate && oldDate !== '0') ? oldDate : "";
    document.getElementById("editLocation").value = (oldLocation && oldLocation !== '待确定') ? oldLocation : "";
    
    document.getElementById("editModal").style.display = "flex"; 
}

// 2. 关闭弹窗
function closeEditModal() {
    document.getElementById("editModal").style.display = "none";
}

// 3. 提交修改请求
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
            loadMyExchange(); // 修改成功后刷新列表
        } else {
            alert("修改失败: " + (data.message || "未知错误"));
        }
    } catch (error) {
        console.error("修改时间地点请求失败:", error);
        alert("网络连接失败，请检查后端服务是否启动~");
    }
}