let allItems = [];

const BASE_URL = "http://127.0.0.1:8080";

document.addEventListener("DOMContentLoaded", function () {
    bindIndexEvents();

    updateLoginState();
    updateAdminEntry();
    loadItems();
});

/**
 * 绑定首页按钮、搜索框事件
 */
function bindIndexEvents() {
    const searchInput = document.getElementById("searchInput");
    const searchBtn = document.getElementById("searchBtn");
    const resetBtn = document.getElementById("resetBtn");

    if (searchInput) {
        searchInput.addEventListener("keydown", function (event) {
            if (event.key === "Enter") {
                searchItems();
            }
        });
    }

    if (searchBtn) {
        searchBtn.addEventListener("click", searchItems);
    }

    if (resetBtn) {
        resetBtn.addEventListener("click", resetSearch);
    }

    const itemList = document.getElementById("itemList");

    if (itemList) {
        // 使用事件委托处理所有 item-card 点击
        itemList.addEventListener("click", function (event) {
            const card = event.target.closest(".item-card");
            if (!card) return;

            const itemId = card.dataset.itemId;
            if (!itemId) return;

            window.location.href = "detail.html?item_id=" + encodeURIComponent(itemId);
        });
    }

    // 侧边栏导航事件
    const navIds = ["homeNav", "myPageNav", "myApplyNav", "myExchangeNav", "todoApplyNav", "adminEntry"];
    const navHandlers = [goHome, goMyPage, goMyApply, goMyExchange, goTodoApply, goAdminCenter];

    navIds.forEach((id, idx) => {
        const el = document.getElementById(id);
        if (el) {
            el.addEventListener("click", navHandlers[idx]);
            el.addEventListener("keydown", function (e) {
                if (e.key === "Enter" || e.key === " ") {
                    e.preventDefault();
                    navHandlers[idx]();
                }
            });
        }
    });
}

function loadItems() {
    const itemList = document.getElementById("itemList");
    const itemCountText = document.getElementById("itemCountText");

    if (itemList) itemList.innerHTML = '<p class="loading-text">正在加载制品...</p>';
    if (itemCountText) itemCountText.innerText = "正在统计制品数量...";

    fetch(`${BASE_URL}/items`)
        .then(response => {
            if (!response.ok) throw new Error("服务器响应异常");
            return response.json();
        })
        .then(data => {
            if (!data.success) {
                allItems = [];
                renderItems([], data.message || "查询失败");
                return;
            }

            allItems = Array.isArray(data.data) ? data.data : [];
            renderItems(allItems, data.message || "查询成功");
        })
        .catch(error => {
            console.error("加载制品失败:", error);
            allItems = [];
            renderItems([], "服务器连接失败，请检查后端是否启动哦~");
        });
}

function renderItems(items, message) {
    const itemList = document.getElementById("itemList");
    const itemCountText = document.getElementById("itemCountText");

    if (!itemList) return;

    itemList.innerHTML = "";

    if (!items || items.length === 0) {
        itemList.innerHTML = `<p class="empty-text">${message || "暂时还没有制品哦~"}</p>`;
        if (itemCountText) itemCountText.innerText = "当前共展示 0 个制品";
        return;
    }

    if (itemCountText) itemCountText.innerText = "当前共展示 " + items.length + " 个制品";

    items.forEach(item => {
        const img_url = item.img_url && item.img_url.trim() !== ""
            ? item.img_url
            : "upload/default_item.png";

        const card = document.createElement("div");
        card.className = "item-card";
        card.dataset.itemId = item.item_id;

        card.innerHTML = `
            <img class="item-img_url" src="${img_url}" alt="制品图片">

            <div class="item-info">
                <h3>${item.item_name || "默认名称"}</h3>
                <p>厨子：${item.owner_name || "未知用户"}</p>
                <p>角色：${item.role || "默认角色"}</p>
                <p>类型：${item.type || "默认类型"}</p>
                <p>剩余数量：${item.quantity || 0}</p>
                <p class="item-intro">介绍：${item.intro || "暂无介绍"}</p>
            </div>
        `;

        itemList.appendChild(card);
    });
}

function searchItems() {
    const keywordInput = document.getElementById("searchInput");
    const keyword = keywordInput ? keywordInput.value.trim().toLowerCase() : "";

    if (!keyword) {
        renderItems(allItems, "查询成功");
        return;
    }

    const result = allItems.filter(item => {
        const text =
            (item.item_name || "") + " " +
            (item.owner_name || "") + " " +
            (item.role || "") + " " +
            (item.type || "") + " " +
            (item.intro || "");

        return text.toLowerCase().includes(keyword);
    });

    renderItems(result, "没有找到相关制品哦~");

    const itemCountText = document.getElementById("itemCountText");
    if (itemCountText) itemCountText.innerText = "搜索结果：" + result.length + " 个制品";
}

function resetSearch() {
    const searchInput = document.getElementById("searchInput");

    if (searchInput) searchInput.value = "";

    renderItems(allItems, "查询成功");
}