window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    updateLoginState();
    updateAdminEntry();
    loadMyApplications();
};

function loadMyApplications() {
    const user = getCurrentUser();
    const applyList = document.getElementById("applyList");
    const applyCountText = document.getElementById("applyCountText");

    if (!user || !user.user_id) {
        applyList.innerHTML = `<div class="empty-box"><p>请先登录后查看我的申请</p></div>`;
        if (applyCountText) applyCountText.innerText = "共 0 条申请";
        return;
    }

    applyList.innerHTML = `<p class="loading-text">正在加载我的申请...</p>`;
    if (applyCountText) applyCountText.innerText = "正在统计...";

    fetch(`http://127.0.0.1:8080/exchange/outgoing?uto=${user.user_id}`)
        .then(response => {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }
            return response.json();
        })
        .then(data => {
            if (!data.success) {
                applyList.innerHTML = `<div class="empty-box"><p>${data.message || "加载失败"}</p></div>`;
                if (applyCountText) applyCountText.innerText = "共 0 条申请";
                return;
            }

            const applications = Array.isArray(data.data) ? data.data : [];

            if (applyCountText) {
                applyCountText.innerText = "共 " + applications.length + " 条申请";
            }

            if (applications.length === 0) {
                applyList.innerHTML = `
                    <div class="empty-box">
                        <p class="empty-text">${data.message || "咪还没有提交过任何申请！"}</p>
                    </div>
                `;
                return;
            }

            let html = "";

            applications.forEach(app => {
                const statusInfo = getStatusInfo(app.status, app.status_text);

                html += `
                    <div class="apply-card ${statusInfo.cardClass}">
                        <div class="status-badge ${statusInfo.badgeClass}">
                            ${escapeHtml(statusInfo.text)}
                        </div>

                        <h3>${escapeHtml(app.item_name || "未知制品")}</h3>

                        <p><span>申请明细编号：</span>${app.detail_id}</p>
                        <p><span>交换编号：</span>${app.exchange_id}</p>
                        <p><span>制品编号：</span>${app.item_id}</p>
                        <p><span>申请对象：</span>${escapeHtml(app.target_user_name || "未知用户")}</p>
                        <p><span>申请数量：</span>${app.apply_quantity}</p>
                        <p><span>制品余量：</span>${app.left_quantity}</p>
                    </div>
                `;
            });

            applyList.innerHTML = html;
        })
        .catch(error => {
            console.error("加载我的申请失败：", error);
            applyList.innerHTML = `
                <div class="empty-box">
                    <p>服务器连接失败，请检查后端是否启动哦~</p>
                </div>
            `;
            if (applyCountText) {
                applyCountText.innerText = "共 0 条申请";
            }
        });
}

function getStatusInfo(status, statusText) {
    status = Number(status);

    if (status === 0) {
        return {
            text: statusText || "待处理",
            badgeClass: "status-pending",
            cardClass: "pending"
        };
    } 
    if (status === 1) {
        return {
            text: statusText || "已拒绝",
            badgeClass: "status-rejected",
            cardClass: "rejected"
        };
    }
    if (status === 2) {
        return {
            text: statusText || "已同意待交换",
            badgeClass: "status-accepted",
            cardClass: "accepted"
        };
    }
    if (status === 3) {
        return {
            text: statusText || "已完成",
            badgeClass: "status-finished",
            cardClass: "finished"
    };
    } 
    if (status === 4) {
        return {
            text: statusText || "已取消",
            badgeClass: "status-discard",
            cardClass: "discard"
        };
    }
    return {
        text: statusText || "未知状态",
        badgeClass: "status-pending",
        cardClass: "pending"
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

const imageInput = document.getElementById("itemImage");

if (imageInput) {

    imageInput.addEventListener("change", function () {

        const nameText = document.getElementById("selectedImageName");

        if (this.files.length > 0) {
            nameText.innerText =
                "已选择图片：" + this.files[0].name;
        } else {
            nameText.innerText = "";
        }

    });
}