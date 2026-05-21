window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    updateLoginState();
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