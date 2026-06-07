let currentItem = null;
let ownerItems = [];
let isFavorite = false;

const BASE_URL = "http://127.0.0.1:8080";

document.addEventListener("DOMContentLoaded", function () {
    bindDetailEvents();

    updateLoginState();
    updateAdminEntry();
    loadItemDetail();
});

/**
 * 绑定详情页事件
 * 静态按钮直接绑定，动态生成的按钮用事件委托
 */
function bindDetailEvents() {
    const backHomeBtn = document.getElementById("backHomeBtn");

    if (backHomeBtn) {
        backHomeBtn.addEventListener("click", goHome);
    }

    const detailContainer = document.getElementById("detailContainer");

    if (detailContainer) {
        detailContainer.addEventListener("click", handleDetailContainerClick);
    }
}

/**
 * 处理 detailContainer 里面动态生成的按钮点击事件
 */
function handleDetailContainerClick(event) {
    const button = event.target.closest("[data-action]");

    if (!button) return;

    const action = button.dataset.action;

    if (action === "applyExchange") {
        applyExchange();
    }
    else if (action === "toggleFavorite") {
        toggleFavorite();
    }
    else if (action === "goHome") {
        goHome();
    }
}

function loadItemDetail() {
    const params = new URLSearchParams(window.location.search);
    const itemId = params.get("item_id");

    const detailContainer = document.getElementById("detailContainer");

    if (!detailContainer) return;

    if (!itemId) {
        detailContainer.innerHTML = "<p>缺少item_id参数</p>";
        return;
    }

    detailContainer.innerHTML = '<p class="loading-text">正在加载详情...</p>';

    fetch(`${BASE_URL}/items/detail?item_id=${encodeURIComponent(itemId)}`)
        .then(function (response) {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }

            return response.json();
        })
        .then(function (item) {
            if (!item.success) {
                detailContainer.innerHTML = `<p>${escapeHtml(item.message || "查询失败")}</p>`;
                return;
            }

            currentItem = item.data;

            renderItemDetail(currentItem);
            loadOwnerItems(currentItem.owner);
            checkFavorite();
        })
        .catch(function (error) {
            console.error("加载详情失败:", error);
            detailContainer.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function renderItemDetail(data) {
    const detailContainer = document.getElementById("detailContainer");

    if (!detailContainer) return;

    const imgUrl =
        data.img_url && data.img_url.trim() !== ""
            ? data.img_url
            : "upload/default_item.png";

    detailContainer.innerHTML = `
        <div class="detail-card">

            <div class="detail-img_url-box">
                <img class="detail-img_url" src="${escapeHtml(imgUrl)}" alt="制品图片">
            </div>

            <div class="detail-info">

                <h1>${escapeHtml(data.item_name || "默认名称")}</h1>

                <p><span>厨子：</span>${escapeHtml(data.owner_name || "未知用户")}</p>

                <p><span>角色：</span>${escapeHtml(data.role || "默认角色")}</p>

                <p><span>类型：</span>${escapeHtml(data.type || "默认类型")}</p>

                <p><span>剩余数量：</span>${escapeHtml(data.quantity || 0)}</p>

                <p><span>介绍：</span>${escapeHtml(data.intro || "暂无介绍")}</p>

                <div class="apply-box">

                    <label for="applyQuantity">申请数量：</label>

                    <input
                        type="number"
                        id="applyQuantity"
                        min="1"
                        max="${escapeHtml(data.quantity || 1)}"
                        value="1"
                    >

                </div>

                <div class="detail-actions">

                    <button
                        class="apply-btn"
                        data-action="applyExchange"
                    >
                        申请交换
                    </button>

                    <button
                        id="favoriteBtn"
                        class="favorite-btn"
                        data-action="toggleFavorite"
                        title="收藏"
                    >
                        ♡
                    </button>

                    <button
                        class="back-btn"
                        data-action="goHome"
                    >
                        返回首页
                    </button>

                </div>

            </div>

        </div>

        <div class="owner-items-section">
            <h2>这个厨子的可交换制品</h2>
            <p class="owner-items-tip">可以一次勾选多个制品一起申请交换哦~</p>

            <div id="ownerItemsList" class="owner-items-list">
                <p class="loading-text">正在加载这个厨子的其他制品...</p>
            </div>
        </div>
    `;
}

function loadOwnerItems(ownerId) {
    const ownerItemsList = document.getElementById("ownerItemsList");

    if (!ownerItemsList) return;

    if (!ownerId) {
        ownerItemsList.innerHTML = "<p>没有找到作者编号</p>";
        return;
    }

    fetch(`${BASE_URL}/items/by-owner?owner=${encodeURIComponent(ownerId)}`)
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (!data.success) {
                ownerItemsList.innerHTML = `<p>${escapeHtml(data.message || "加载失败")}</p>`;
                return;
            }

            ownerItems = Array.isArray(data.data) ? data.data : [];
            renderOwnerItems(ownerItems);
        })
        .catch(function (error) {
            console.error("加载作者制品失败:", error);
            ownerItemsList.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function renderOwnerItems(items) {
    const ownerItemsList = document.getElementById("ownerItemsList");

    if (!ownerItemsList) return;

    if (!items || items.length === 0) {
        ownerItemsList.innerHTML = "<p>这个厨子暂无可交换制品</p>";
        return;
    }

    let html = "";

    items.forEach(function (item) {
        const imgUrl =
            item.img_url && item.img_url.trim() !== ""
                ? item.img_url
                : "upload/default_item.png";

        const checked =
            currentItem && Number(item.item_id) === Number(currentItem.item_id)
                ? "checked"
                : "";

        html += `
            <div class="owner-item-card">
                <input
                    type="checkbox"
                    class="choose-item"
                    value="${escapeHtml(item.item_id)}"
                    ${checked}
                >

                <img class="owner-item-img_url" src="${escapeHtml(imgUrl)}" alt="制品图片">

                <div class="owner-item-info">
                    <h3>${escapeHtml(item.item_name || "默认名称")}</h3>
                    <p>角色：${escapeHtml(item.role || "暂无")}</p>
                    <p>类型：${escapeHtml(item.type || "暂无")}</p>
                    <p>剩余：${escapeHtml(item.quantity || 0)}</p>
                    <p>介绍：${escapeHtml(item.intro || "暂无介绍")}</p>

                    <label>
                        申请数量：
                        <input
                            type="number"
                            class="choose-quantity"
                            data-item-id="${escapeHtml(item.item_id)}"
                            min="1"
                            max="${escapeHtml(item.quantity || 1)}"
                            value="1"
                        >
                    </label>
                </div>
            </div>
        `;
    });

    ownerItemsList.innerHTML = html;
}

function applyExchange() {
    const user = getCurrentUser();

    if (user === null) {
        alert("请先登录");
        window.location.href = "login.html";
        return;
    }

    if (currentItem === null) {
        alert("制品信息还没有加载完成");
        return;
    }

    if (Number(user.user_id) === Number(currentItem.owner)) {
        alert("咪不可以和自己交换哦~快去寻找同好叭！");
        return;
    }

    const checkedItems = document.querySelectorAll(".choose-item:checked");

    if (checkedItems.length === 0) {
        alert("至少要选择一个制品哟~");
        return;
    }

    const itemIds = [];
    const quantities = [];

    for (let i = 0; i < checkedItems.length; i++) {
        const itemId = checkedItems[i].value;
        const quantityInput = document.querySelector(`.choose-quantity[data-item-id="${itemId}"]`);

        if (!quantityInput) {
            alert("没有找到申请数量输入框");
            return;
        }

        const quantity = Number(quantityInput.value);

        const item = ownerItems.find(function (one) {
            return Number(one.item_id) === Number(itemId);
        });

        if (!item) {
            alert("没有找到选中的制品信息");
            return;
        }

        if (quantity <= 0) {
            alert("申请数量必须大于0！");
            return;
        }

        if (quantity > Number(item.quantity)) {
            alert("申请数量不能超过剩余数量哦~");
            return;
        }

        itemIds.push(itemId);
        quantities.push(quantity);
    }

    const params = new URLSearchParams();
    params.append("ufrom", currentItem.owner);
    params.append("uto", user.user_id);
    params.append("item_ids", itemIds.join(","));
    params.append("quantities", quantities.join(","));

    fetch(`${BASE_URL}/exchange/apply`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            alert(data.message || "操作完成");

            if (data.success) {
                window.location.href = "myapply.html";
            }
        })
        .catch(function (error) {
            console.error("申请交换失败:", error);
            alert("服务器连接失败，请检查后端是否启动哦~");
        });
}

function checkFavorite() {
    const user = getCurrentUser();

    if (user === null || currentItem === null) {
        updateFavoriteButton(false);
        return;
    }

    fetch(`${BASE_URL}/favorite/check?user_id=${encodeURIComponent(user.user_id)}&item_id=${encodeURIComponent(currentItem.item_id)}`)
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (data.success) {
                isFavorite = data.is_favorite;
                updateFavoriteButton(isFavorite);
            }
            else {
                isFavorite = false;
                updateFavoriteButton(false);
            }
        })
        .catch(function (error) {
            console.error("查询收藏状态失败:", error);
            isFavorite = false;
            updateFavoriteButton(false);
        });
}

function toggleFavorite() {
    const user = getCurrentUser();

    if (user === null) {
        alert("请先登录后再收藏");
        window.location.href = "login.html";
        return;
    }

    if (currentItem === null) {
        alert("制品信息还没有加载完成");
        return;
    }

    if (isFavorite) {
        removeFavorite();
    }
    else {
        addFavorite();
    }
}

function addFavorite() {
    const user = getCurrentUser();

    const params = new URLSearchParams();
    params.append("user_id", user.user_id);
    params.append("item_id", currentItem.item_id);

    fetch(`${BASE_URL}/favorite/add`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (data.success) {
                isFavorite = true;
                updateFavoriteButton(true);
            }
            else {
                alert(data.message || "收藏失败");
            }
        })
        .catch(function (error) {
            console.error("添加收藏失败:", error);
            alert("服务器连接失败，请检查后端是否启动哦~");
        });
}

function removeFavorite() {
    const user = getCurrentUser();

    const params = new URLSearchParams();
    params.append("user_id", user.user_id);
    params.append("item_id", currentItem.item_id);

    fetch(`${BASE_URL}/favorite/remove`, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
        .then(function (response) {
            return response.json();
        })
        .then(function (data) {
            if (data.success) {
                isFavorite = false;
                updateFavoriteButton(false);
            }
            else {
                alert(data.message || "取消收藏失败");
            }
        })
        .catch(function (error) {
            console.error("取消收藏失败:", error);
            alert("服务器连接失败，请检查后端是否启动哦~");
        });
}

function updateFavoriteButton(favoriteState) {
    const favoriteBtn = document.getElementById("favoriteBtn");

    if (!favoriteBtn) return;

    if (favoriteState) {
        favoriteBtn.innerText = "♥";
        favoriteBtn.classList.add("active");
        favoriteBtn.title = "取消收藏";
    }
    else {
        favoriteBtn.innerText = "♡";
        favoriteBtn.classList.remove("active");
        favoriteBtn.title = "收藏";
    }
}

/**
 * 防止后端返回的文字里带 HTML，被 innerHTML 当成标签执行
 * 这是一个简单的 XSS 防护
 */
function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}