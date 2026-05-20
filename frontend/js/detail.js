let currentItem = null;
let ownerItems = [];
let isFavorite = false;

window.onload = function () {
    loadItemDetail();
};

function loadItemDetail() {
    const params = new URLSearchParams(window.location.search);
    const itemId = params.get("item_id");

    const detailContainer = document.getElementById("detailContainer");

    if (!itemId) {
        detailContainer.innerHTML = "<p>缺少item_id参数</p>";
        return;
    }

    detailContainer.innerHTML = '<p class="loading-text">正在加载详情...</p>';

    fetch(`http://127.0.0.1:8080/items/detail?item_id=${itemId}`)
        .then(response => {
            if (!response.ok) {
                throw new Error("服务器响应异常");
            }
            return response.json();
        })
        .then(item => {
            if (!item.success) {
                detailContainer.innerHTML = `<p>${item.message || "查询失败"}</p>`;
                return;
            }

            currentItem =item.data;
    
            renderItemDetail(currentItem);
            loadOwnerItems(currentItem.owner);
            checkFavorite();
        })
        .catch(error => {
            console.error("加载详情失败:", error);
            detailContainer.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}
function renderItemDetail(data) {

    const detailContainer = document.getElementById("detailContainer");

    const img_url =
        data.img_url && data.img_url.trim() !== ""
            ? data.img_url
            : "upload/default_item.png";

    detailContainer.innerHTML = `
        <div class="detail-card">

            <div class="detail-img_url-box">
                <img class="detail-img_url" src="${img_url}" alt="制品图片">
            </div>

            <div class="detail-info">

                <h1>${data.item_name || "默认名称"}</h1>

                <p><span>厨子：</span>${data.owner_name || "未知用户"}</p>

                <p><span>角色：</span>${data.role || "默认角色"}</p>

                <p><span>类型：</span>${data.type || "默认类型"}</p>

                <p><span>剩余数量：</span>${data.quantity || 0}</p>

                <p><span>介绍：</span>${data.intro || "暂无介绍"}</p>

                <div class="apply-box">

                    <label for="applyQuantity">申请数量：</label>

                    <input
                        type="number"
                        id="applyQuantity"
                        min="1"
                        max="${data.quantity || 1}"
                        value="1"
                    >

                </div>

                <div class="detail-actions">

                    <button
                        class="apply-btn"
                        onclick="applyExchange()"
                    >
                        申请交换
                    </button>

                    <button
                        id="favoriteBtn"
                        class="favorite-btn"
                        onclick="toggleFavorite()"
                        title="收藏"
                    >
                        ♡
                    </button>

                    <button
                        class="back-btn"
                        onclick="goHome()"
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

    if (!ownerId) {
        ownerItemsList.innerHTML = "<p>没有找到作者编号</p>";
        return;
    }

    fetch(`http://127.0.0.1:8080/items/by-owner?owner=${encodeURIComponent(ownerId)}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                ownerItemsList.innerHTML = `<p>${data.message || "加载失败"}</p>`;
                return;
            }

            ownerItems = Array.isArray(data.data) ? data.data : [];
            renderOwnerItems(ownerItems);
        })
        .catch(error => {
            console.error("加载作者制品失败:", error);
            ownerItemsList.innerHTML = "<p>服务器连接失败，请检查后端是否启动哦~</p>";
        });
}

function renderOwnerItems(items) {
    const ownerItemsList = document.getElementById("ownerItemsList");

    if (!items || items.length === 0) {
        ownerItemsList.innerHTML = "<p>这个厨子暂无可交换制品</p>";
        return;
    }

    let html = "";

    items.forEach(item => {
        const img_url = item.img_url && item.img_url.trim() !== ""
            ? item.img_url
            : "upload/default_item.png";

        const checked = currentItem && Number(item.item_id) === Number(currentItem.item_id)
            ? "checked"
            : "";

        html += `
            <div class="owner-item-card">
                <input
                    type="checkbox"
                    class="choose-item"
                    value="${item.item_id}"
                    ${checked}
                >

                <img class="owner-item-img_url" src="${img_url}" alt="制品图片">

                <div class="owner-item-info">
                    <h3>${item.item_name || "默认名称"}</h3>
                    <p>角色：${item.role || "暂无"}</p>
                    <p>类型：${item.type || "暂无"}</p>
                    <p>剩余：${item.quantity || 0}</p>
                    <p>介绍：${item.intro || "暂无介绍"}</p>

                    <label>
                        申请数量：
                        <input
                            type="number"
                            class="choose-quantity"
                            data-item-id="${item.item_id}"
                            min="1"
                            max="${item.quantity || 1}"
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

        const item = ownerItems.find(one => Number(one.item_id) === Number(itemId));

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

    fetch("http://127.0.0.1:8080/exchange/apply", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");

        if (data.success) {
            window.location.href = "myapply.html";
        }
    })
    .catch(error => {
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

    fetch(`http://127.0.0.1:8080/favorite/check?user_id=${encodeURIComponent(user.user_id)}&item_id=${encodeURIComponent(currentItem.item_id)}`)
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                isFavorite = data.is_favorite;
                updateFavoriteButton(isFavorite);
            } else {
                isFavorite = false;
                updateFavoriteButton(false);
            }
        })
        .catch(error => {
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
    } else {
        addFavorite();
    }
}

function addFavorite() {
    const user = getCurrentUser();

    const params = new URLSearchParams();
    params.append("user_id", user.user_id);
    params.append("item_id", currentItem.item_id);

    fetch("http://127.0.0.1:8080/favorite/add", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");

        if (data.success) {
            isFavorite = true;
            updateFavoriteButton(true);
        }
    })
    .catch(error => {
        console.error("添加收藏失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}

function removeFavorite() {
    const user = getCurrentUser();

    const params = new URLSearchParams();
    params.append("user_id", user.user_id);
    params.append("item_id", currentItem.item_id);

    fetch("http://127.0.0.1:8080/favorite/remove", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");

        if (data.success) {
            isFavorite = false;
            updateFavoriteButton(false);
        }
    })
    .catch(error => {
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
    } else {
        favoriteBtn.innerText = "♡";
        favoriteBtn.classList.remove("active");
        favoriteBtn.title = "收藏";
    }
}