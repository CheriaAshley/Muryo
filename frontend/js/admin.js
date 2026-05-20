let adminLevel = 0;

window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    loadAdminInfo();
};

function loadAdminInfo() {
    const user = getCurrentUser();
    const adminInfo = document.getElementById("adminInfo");
    const adminContent = document.getElementById("adminContent");

    fetch(`http://127.0.0.1:8080/admin/me?user_id=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success || !data.is_admin) {
                alert(data.message || "你还不是管理员哦~");
                window.location.href = "index.html";
                return;
            }

            adminLevel = Number(data.level);
            adminInfo.innerText = "当前管理员等级：Level " + adminLevel;

            renderAdminContent(adminLevel);
        })
        .catch(error => {
            console.error("加载管理员身份失败:", error);
            adminInfo.innerText = "管理员身份加载失败";
            adminContent.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function renderAdminContent(level) {
    const adminContent = document.getElementById("adminContent");

    if (level === 1) {
        adminContent.innerHTML = `
            <div class="admin-card">
                <h2>一级管理员功能</h2>
                <p>可以申请封禁违规制品，等待高级管理员审核。</p>

                <div class="admin-form">
                    <input type="number" id="deleteItemId" placeholder="请输入制品ID">
                    <textarea id="deleteReason" placeholder="请输入封禁原因"></textarea>
                    <button onclick="submitDeleteApply()">提交封禁制品申请</button>
                </div>
            </div>
        `;
        return;
    }

    if (level === 2) {
        adminContent.innerHTML = `
            <div class="admin-card">
                <h2>二级管理员功能</h2>
                <p>可以申请封禁用户，也可以审核一级管理员提交的制品封禁申请。</p>

                <div class="admin-form">
                    <h3>申请封禁用户</h3>
                    <input type="number" id="banUserId" placeholder="请输入用户ID">
                    <textarea id="banReason" placeholder="请输入封禁原因"></textarea>
                    <button onclick="submitBanApply()">提交封禁用户申请</button>
                </div>

                <div class="admin-list">
                    <button onclick="loadAdminApplyList()">查看待审核申请</button>
                    <div id="adminApplyList"></div>
                </div>
            </div>
        `;
        return;
    }

    if (level === 3) {
        adminContent.innerHTML = `
            <div class="admin-card">
                <h2>三级管理员功能</h2>
                <p>可以直接封禁制品、封禁用户、审核所有申请、恢复用户、调整管理员身份。</p>

                <div class="admin-form">
                    <h3>直接封禁制品</h3>
                    <input type="number" id="deleteItemId" placeholder="请输入制品ID">
                    <textarea id="deleteReason" placeholder="请输入封禁原因"></textarea>
                    <button onclick="submitDeleteApply()">直接封禁制品</button>
                </div>

                <div class="admin-form">
                    <h3>直接封禁用户</h3>
                    <input type="number" id="banUserId" placeholder="请输入用户ID">
                    <textarea id="banReason" placeholder="请输入封禁原因"></textarea>
                    <button onclick="submitBanApply()">直接封禁用户</button>
                </div>

                <div class="admin-form">
                    <h3>恢复封禁用户</h3>
                    <input type="number" id="recoverUserId" placeholder="请输入用户ID">
                    <button onclick="recoverUser()">恢复用户</button>
                </div>

                <div class="admin-form">
                    <h3>普通用户变管理员</h3>
                    <input type="number" id="makeAdminUserId" placeholder="请输入用户ID">
                    <input type="number" id="makeAdminLevel" min="1" max="3" placeholder="管理员等级1-3">
                    <button onclick="makeUserAdmin()">设为管理员</button>
                </div>

                <div class="admin-form">
                    <h3>修改管理员等级</h3>
                    <input type="number" id="changeAdminUserId" placeholder="请输入管理员用户ID">
                    <input type="number" id="changeAdminLevel" min="1" max="3" placeholder="新等级1-3">
                    <button onclick="changeAdminLevel()">修改等级</button>
                </div>

                <div class="admin-form">
                    <h3>管理员变普通用户</h3>
                    <input type="number" id="removeAdminUserId" placeholder="请输入管理员用户ID">
                    <button onclick="removeAdmin()">取消管理员身份</button>
                </div>

                <div class="admin-list">
                    <button onclick="loadAdminApplyList()">查看待审核申请</button>
                    <div id="adminApplyList"></div>
                </div>
            </div>
        `;
        return;
    }

    adminContent.innerHTML = "<p>管理员等级异常，请检查数据库。</p>";
}

function submitDeleteApply() {
    const user = getCurrentUser();
    const itemId = document.getElementById("deleteItemId").value.trim();
    const reason = document.getElementById("deleteReason").value.trim();

    if (!itemId) {
        alert("缺少制品编号");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("item_id", itemId);
    params.append("reason", reason);

    fetch("http://127.0.0.1:8080/admin/apply/ban_item", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            document.getElementById("deleteItemId").value = "";
            document.getElementById("deleteReason").value = "";
        }
    })
    .catch(error => {
        console.error("封禁制品失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function submitBanApply() {
    const user = getCurrentUser();
    const userId = document.getElementById("banUserId").value.trim();
    const reason = document.getElementById("banReason").value.trim();

    if (!userId) {
        alert("缺少用户编号");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("user_id", userId);
    params.append("reason", reason);

    fetch("http://127.0.0.1:8080/admin/apply/ban_user", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            document.getElementById("banUserId").value = "";
            document.getElementById("banReason").value = "";
        }
    })
    .catch(error => {
        console.error("封禁用户失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function loadAdminApplyList() {
    const user = getCurrentUser();
    const listBox = document.getElementById("adminApplyList");

    listBox.innerHTML = "<p>正在加载待审核申请...</p>";

    fetch(`http://127.0.0.1:8080/admin/apply/list?admin_id=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                listBox.innerHTML = `<p>${data.message || "查询失败"}</p>`;
                return;
            }

            const list = Array.isArray(data.data) ? data.data : [];

            if (list.length === 0) {
                listBox.innerHTML = `<p>${data.message || "暂无待审核申请"}</p>`;
                return;
            }

            let html = "<h3>待审核申请</h3>";

            list.forEach(app => {
                html += `
                    <div class="admin-apply-card">
                        <p><strong>申请编号：</strong>${app.apply_id}</p>
                        <p><strong>申请管理员：</strong>${app.admin_id}</p>
                        <p><strong>申请类型：</strong>${getApplyTypeText(app.apply_type)}</p>
                        <p><strong>目标编号：</strong>${app.target_id}</p>
                        <p><strong>原因：</strong>${app.reason || "暂无原因"}</p>
                        <p><strong>提交时间：</strong>${app.create_time || "暂无"}</p>

                        <button onclick="handleAdminApply(${app.apply_id}, 'agree')">同意</button>
                        <button onclick="handleAdminApply(${app.apply_id}, 'reject')">拒绝</button>
                    </div>
                `;
            });

            listBox.innerHTML = html;
        })
        .catch(error => {
            console.error("加载审核申请失败:", error);
            listBox.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function handleAdminApply(applyId, action) {
    const user = getCurrentUser();

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("apply_id", applyId);
    params.append("action", action);

    fetch("http://127.0.0.1:8080/admin/apply/handle", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            loadAdminApplyList();
        }
    })
    .catch(error => {
        console.error("审核失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function recoverUser() {
    const user = getCurrentUser();
    const userId = document.getElementById("recoverUserId").value.trim();

    if (!userId) {
        alert("缺少用户编号");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("user_id", userId);

    fetch("http://127.0.0.1:8080/admin/user/recover", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            document.getElementById("recoverUserId").value = "";
        }
    })
    .catch(error => {
        console.error("恢复用户失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function makeUserAdmin() {
    const user = getCurrentUser();
    const targetId = document.getElementById("makeAdminUserId").value.trim();
    const level = document.getElementById("makeAdminLevel").value.trim();

    if (!targetId || !level) {
        alert("参数不能为空");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("target_id", targetId);
    params.append("level", level);

    fetch("http://127.0.0.1:8080/admin/add_admin", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            document.getElementById("makeAdminUserId").value = "";
            document.getElementById("makeAdminLevel").value = "";
        }
    })
    .catch(error => {
        console.error("添加管理员失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function changeAdminLevel() {
    const user = getCurrentUser();
    const targetId = document.getElementById("changeAdminUserId").value.trim();
    const level = document.getElementById("changeAdminLevel").value.trim();

    if (!targetId || !level) {
        alert("参数不能为空");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("target_id", targetId);
    params.append("level", level);

    fetch("http://127.0.0.1:8080/admin/change_level", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            document.getElementById("changeAdminUserId").value = "";
            document.getElementById("changeAdminLevel").value = "";
        }
    })
    .catch(error => {
        console.error("修改管理员等级失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function removeAdmin() {
    const user = getCurrentUser();
    const targetId = document.getElementById("removeAdminUserId").value.trim();

    if (!targetId) {
        alert("缺少目标用户编号");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("target_id", targetId);

    fetch("http://127.0.0.1:8080/admin/remove_admin", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            document.getElementById("removeAdminUserId").value = "";
        }
    })
    .catch(error => {
        console.error("取消管理员身份失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function getApplyTypeText(type) {
    if (type === "ban_item") return "封禁制品申请";
    if (type === "ban_user") return "封禁用户申请";
    return "未知申请";
}