let adminLevel = 0;

const BASE_URL = "http://127.0.0.1:8080";

document.addEventListener("DOMContentLoaded", function () {
    bindStaticEvents();

    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadAdminInfo();
});

/**
 * 绑定页面一开始就存在的按钮事件
 */
function bindStaticEvents() {
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

    const adminContent = document.getElementById("adminContent");

    if (adminContent) {
        adminContent.addEventListener("click", handleAdminContentClick);
    }
}

/**
 * 简化 addEventListener
 */
function bindClick(id, handler) {
    const element = document.getElementById(id);

    if (!element) return;

    element.addEventListener("click", handler);
}

/**
 * 让 li 支持键盘 Enter / 空格触发
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
 * 处理 adminContent 里面动态生成的按钮
 * 因为这些按钮是 innerHTML 后生成的，所以用事件委托
 */
function handleAdminContentClick(event) {
    const button = event.target.closest("[data-action]");

    if (!button) return;

    const action = button.dataset.action;

    if (action === "submitDeleteApply") {
        submitDeleteApply();
    }
    else if (action === "submitBanApply") {
        submitBanApply();
    }
    else if (action === "loadAdminApplyList") {
        loadAdminApplyList();
    }
    else if (action === "recoverUser") {
        recoverUser();
    }
    else if (action === "makeUserAdmin") {
        makeUserAdmin();
    }
    else if (action === "submitChangeAdminLevel") {
        submitChangeAdminLevel();
    }
    else if (action === "removeAdmin") {
        removeAdmin();
    }
    else if (action === "handleAdminApply") {
        const applyId = button.dataset.applyId;
        const handleAction = button.dataset.handleAction;

        handleAdminApply(applyId, handleAction);
    }
}

function loadAdminInfo() {
    const user = getCurrentUser();
    const adminInfo = document.getElementById("adminInfo");
    const adminContent = document.getElementById("adminContent");

    if (!user || !user.user_id) {
        alert("请先登录后再访问管理员中心");
        window.location.href = "login.html";
        return;
    }

    fetch(`${BASE_URL}/admin/me?user_id=${encodeURIComponent(user.user_id)}`)
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (!data.success || !data.is_admin) {
                alert(data.message || "你还不是管理员哦~");
                window.location.href = "index.html";
                return;
            }

            adminLevel = Number(data.level);
            adminInfo.textContent = "当前管理员等级：Level " + adminLevel;

            renderAdminContent(adminLevel);
        })
        .catch(function (error) {
            console.error("加载管理员身份失败:", error);
            adminInfo.textContent = "管理员身份加载失败";
            adminContent.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function renderAdminContent(level) {
    const adminContent = document.getElementById("adminContent");

    let html = `<div class="admin-card">`;

    if (level === 1) {
        html += `
            <h2>一级管理员功能</h2>
            <p>你可以申请封禁违规制品，等待高级管理员审核。</p>

            <div class="admin-form">
                <h3>申请封禁制品</h3>
                <input type="number" id="deleteItemId" placeholder="请输入制品ID">
                <textarea id="deleteReason" placeholder="请输入封禁原因"></textarea>
                <button data-action="submitDeleteApply">提交封禁制品申请</button>
            </div>
        `;
    }
    else if (level === 2) {
        html += `
            <h2>二级管理员功能</h2>
            <p>可以申请封禁用户，也可以审核制品封禁申请。</p>

            <div class="admin-form">
                <h3>直接封禁制品</h3>
                <input type="number" id="deleteItemId" placeholder="请输入制品ID">
                <textarea id="deleteReason" placeholder="请输入封禁原因"></textarea>
                <button data-action="submitDeleteApply">直接封禁制品</button>
            </div>

            <div class="admin-form">
                <h3>申请封禁用户</h3>
                <input type="number" id="banUserId" placeholder="请输入用户ID">
                <textarea id="banReason" placeholder="请输入封禁原因"></textarea>
                <button data-action="submitBanApply">提交封禁用户申请</button>
            </div>

            <div class="admin-list">
                <button data-action="loadAdminApplyList">查看待审核申请</button>
                <div id="adminApplyList"></div>
            </div>
        `;
    }
    else if (level === 3) {
        html += `
            <h2>三级管理员功能（最高权限）</h2>
            <p>你可以直接封禁、审核所有申请，以及调度人员权限和恢复账号。</p>

            <div class="admin-grid">
                <div class="admin-form">
                    <h3>直接封禁制品</h3>
                    <input type="number" id="deleteItemId" placeholder="请输入制品ID">
                    <textarea id="deleteReason" placeholder="请输入封禁原因"></textarea>
                    <button data-action="submitDeleteApply">直接封禁制品</button>
                </div>

                <div class="admin-form">
                    <h3>直接封禁用户</h3>
                    <input type="number" id="banUserId" placeholder="请输入用户ID">
                    <textarea id="banReason" placeholder="请输入封禁原因"></textarea>
                    <button data-action="submitBanApply">直接封禁用户</button>
                </div>

                <div class="admin-form">
                    <h3>普通用户变管理员</h3>
                    <input type="number" id="makeAdminUserId" placeholder="请输入用户ID">
                    <input type="number" id="makeAdminLevel" min="1" max="3" placeholder="管理员等级(1-3)">
                    <button data-action="makeUserAdmin">设为管理员</button>
                </div>

                <div class="admin-form">
                    <h3>修改管理员等级</h3>
                    <input type="number" id="changeAdminUserId" placeholder="请输入管理员ID">
                    <input type="number" id="changeAdminLevelInput" min="1" max="3" placeholder="新等级(1-3)">
                    <button data-action="submitChangeAdminLevel">修改等级</button>
                </div>

                <div class="admin-form">
                    <h3>恢复封禁用户</h3>
                    <input type="number" id="recoverUserId" placeholder="请输入被封禁用户ID">
                    <button data-action="recoverUser">恢复用户</button>
                </div>

                <div class="admin-form">
                    <h3>管理员变普通用户</h3>
                    <input type="number" id="removeAdminUserId" placeholder="请输入要降级的管理员ID">
                    <button data-action="removeAdmin" class="danger-btn">取消管理员身份</button>
                </div>
            </div>

            <div class="admin-list">
                <button data-action="loadAdminApplyList">处理全局待审核申请</button>
                <div id="adminApplyList"></div>
            </div>
        `;
    }
    else {
        html += `<p>管理员等级异常，请检查数据库。</p>`;
    }

    html += `</div>`;

    adminContent.innerHTML = html;
}

function submitDeleteApply() {
    const user = getCurrentUser();
    const itemId = getInputValue("deleteItemId");
    const reason = getInputValue("deleteReason");

    if (!itemId) {
        alert("缺少制品编号");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("item_id", itemId);
    params.append("reason", reason);

    postForm("/admin/apply/ban_item", params, "封禁制品失败")
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                clearInput("deleteItemId");
                clearInput("deleteReason");
            }
        });
}

function submitBanApply() {
    const user = getCurrentUser();
    const userId = getInputValue("banUserId");
    const reason = getInputValue("banReason");

    if (!userId) {
        alert("缺少用户编号");
        return;
    }

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("user_id", userId);
    params.append("reason", reason);

    postForm("/admin/apply/ban_user", params, "封禁用户失败")
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                clearInput("banUserId");
                clearInput("banReason");
            }
        });
}

function loadAdminApplyList() {
    const user = getCurrentUser();
    const listBox = document.getElementById("adminApplyList");

    if (!listBox) return;

    listBox.innerHTML = "<p>正在加载待审核申请...</p>";

    fetch(`${BASE_URL}/admin/apply/list?admin_id=${encodeURIComponent(user.user_id)}`)
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (!data.success) {
                listBox.innerHTML = `<p>${escapeHtml(data.message || "查询失败")}</p>`;
                return;
            }

            const list = Array.isArray(data.data) ? data.data : [];

            if (list.length === 0) {
                listBox.innerHTML = `<p>暂无待审核申请</p>`;
                return;
            }

            let html = "<h3>待审核列表</h3>";

            list.forEach(function (app) {
                html += `
                    <div class="admin-apply-card">
                        <p><strong>申请编号：</strong>${escapeHtml(app.apply_id)}</p>
                        <p><strong>申请管理员：</strong>${escapeHtml(app.admin_id)}</p>
                        <p><strong>申请类型：</strong>${escapeHtml(getApplyTypeText(app.apply_type))}</p>
                        <p><strong>目标ID：</strong>${escapeHtml(app.target_id)}</p>
                        <p><strong>原因：</strong>${escapeHtml(app.reason || "暂无")}</p>

                        <div class="card-actions">
                            ${renderAdminButtons(app)}
                        </div>
                    </div>
                `;
            });

            listBox.innerHTML = html;
        })
        .catch(function (error) {
            console.error("加载待审核申请失败:", error);
            listBox.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function renderAdminButtons(app) {
    if (app.apply_type !== "ban_item" && app.apply_type !== "ban_user") {
        return `
            <button 
                data-action="handleAdminApply" 
                data-apply-id="${escapeHtml(app.apply_id)}" 
                data-handle-action="reject" 
                class="btn-reject">
                驳回未知申请
            </button>
        `;
    }

    return `
        <button 
            data-action="handleAdminApply" 
            data-apply-id="${escapeHtml(app.apply_id)}" 
            data-handle-action="agree" 
            class="btn-agree">
            同意
        </button>

        <button 
            data-action="handleAdminApply" 
            data-apply-id="${escapeHtml(app.apply_id)}" 
            data-handle-action="reject" 
            class="btn-reject">
            拒绝
        </button>
    `;
}

function handleAdminApply(applyId, action) {
    const user = getCurrentUser();

    if (!applyId || !action) {
        alert("缺少审核参数");
        return;
    }

    const confirmText = action === "agree"
        ? "确定要通过并执行此申请吗？"
        : "确定要驳回此申请吗？";

    if (!confirm(confirmText)) return;

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("apply_id", applyId);
    params.append("action", action);

    postForm("/admin/apply/handle", params, "审核失败")
        .then(function (data) {
            console.log("后端返回:", data);

            if (data.success) {
                alert(data.message || "操作成功！");
            }
            else {
                alert("操作被拒绝：" + (data.message || "未知原因"));
            }

            loadAdminApplyList();
        });
}

function recoverUser() {
    const user = getCurrentUser();
    const userId = getInputValue("recoverUserId");

    if (!userId) {
        alert("缺少被恢复用户编号");
        return;
    }

    if (!confirm("确定要为该账号解封吗？")) return;

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("user_id", userId);

    postForm("/admin/user/recover", params, "恢复用户失败")
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                clearInput("recoverUserId");
            }
        });
}

function makeUserAdmin() {
    const user = getCurrentUser();
    const targetId = getInputValue("makeAdminUserId");
    const level = getInputValue("makeAdminLevel");

    if (!targetId || !level) {
        alert("用户ID和等级不能为空");
        return;
    }

    if (!isValidAdminLevel(level)) {
        alert("管理员等级只能是 1、2、3");
        return;
    }

    if (!confirm(`确定要将用户 ${targetId} 设为 Level ${level} 管理员吗？`)) return;

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("target_id", targetId);
    params.append("level", level);

    postForm("/admin/add_admin", params, "添加管理员失败")
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                clearInput("makeAdminUserId");
                clearInput("makeAdminLevel");
            }
        });
}

function submitChangeAdminLevel() {
    const user = getCurrentUser();
    const targetId = getInputValue("changeAdminUserId");
    const level = getInputValue("changeAdminLevelInput");

    if (!targetId || !level) {
        alert("管理员ID和新等级不能为空");
        return;
    }

    if (!isValidAdminLevel(level)) {
        alert("管理员等级只能是 1、2、3");
        return;
    }

    if (!confirm(`确定要将管理员 ${targetId} 的等级修改为 Level ${level} 吗？`)) return;

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("target_id", targetId);
    params.append("level", level);

    postForm("/admin/change_level", params, "修改管理员等级失败")
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                clearInput("changeAdminUserId");
                clearInput("changeAdminLevelInput");
            }
        });
}

function removeAdmin() {
    const user = getCurrentUser();
    const targetId = getInputValue("removeAdminUserId");

    if (!targetId) {
        alert("缺少目标管理员编号");
        return;
    }

    if (!confirm("高危操作：确定要彻底取消该账号的管理员身份吗？")) return;

    const params = new URLSearchParams();
    params.append("admin_id", user.user_id);
    params.append("target_id", targetId);

    postForm("/admin/remove_admin", params, "取消管理员身份失败")
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                clearInput("removeAdminUserId");
            }
        });
}

function postForm(path, params, errorMessage) {
    return fetch(`${BASE_URL}${path}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
        .then(function (response) {
            return response.json();
        })
        .catch(function (error) {
            console.error(errorMessage + ":", error);
            alert("服务器连接失败，请检查后端是否启动哦~");

            return {
                success: false,
                message: "服务器连接失败"
            };
        });
}

function getInputValue(id) {
    const element = document.getElementById(id);

    if (!element) return "";

    return element.value.trim();
}

function clearInput(id) {
    const element = document.getElementById(id);

    if (!element) return;

    element.value = "";
}

function isValidAdminLevel(level) {
    const num = Number(level);

    return num === 1 || num === 2 || num === 3;
}

function getApplyTypeText(type) {
    if (type === "ban_item") return "封禁制品申请";
    if (type === "ban_user") return "封禁用户申请";

    return "未知申请类型";
}

/**
 * 防止后端返回的文字里带 HTML，被 innerHTML 当成标签执行
 */
function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}