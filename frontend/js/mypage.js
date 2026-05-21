window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadProfile();
    loadMyItems();
};

function loadProfile() {
    const user = getCurrentUser();
    const profileCard = document.getElementById("profileCard");

    fetch(`http://127.0.0.1:8080/user/profile?user_id=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                profileCard.innerHTML = `<h2>个人信息</h2><p>${data.message || "加载失败"}</p>`;
                return;
            }

            const profile = data.data;

            profileCard.innerHTML = `
            <h2>个人信息</h2>
            <p><strong>用户ID：</strong>${profile.user_id}</p>
            <p><strong>用户名：</strong>${profile.user_name || "暂无"}</p>
            <p><strong>联系方式：</strong>${profile.contact || "暂无"}</p>
            <p><strong>个人介绍：</strong>${profile.introduction || profile.intro || "这个人很神秘，还没有留下介绍~"}</p>
        `;
        })
        .catch(error => {
            console.error("加载个人信息失败:", error);
            profileCard.innerHTML = `<h2>个人信息</h2><p>加载失败，请检查后端是否启动</p>`;
        });
}

function loadMyItems() {
    const user = getCurrentUser();
    const myItemList = document.getElementById("myItemList");
    const myItemCountText = document.getElementById("myItemCountText");

    myItemList.innerHTML = `<p class="loading-text">正在加载我的制品...</p>`;

    if (myItemCountText) {
        myItemCountText.innerText = "正在统计...";
    }

    fetch(`http://127.0.0.1:8080/items/my?owner=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            myItemList.innerHTML = "";

            if (!data.success) {
                myItemList.innerHTML = `<p class="empty-text">${data.message || "加载失败"}</p>`;
                if (myItemCountText) myItemCountText.innerText = "共 0 个制品";
                return;
            }

            const items = Array.isArray(data.data) ? data.data : [];

            if (myItemCountText) {
                myItemCountText.innerText = "共 " + items.length + " 个制品";
            }

            if (items.length === 0) {
                myItemList.innerHTML = `<p class="empty-text">${data.message || "咪还没有发布制品哦~"}</p>`;
                return;
            }


            items.forEach(item => {
                const card = document.createElement("div");
                card.className = "item-card";

                const img_url = item.img_url && item.img_url.trim() !== ""
                    ? item.img_url
                    : "upload/default_item.png";
                    
                card.innerHTML = `
                    <img src="${img_url}" alt="${item.item_name || "制品图片"}" class="item-img_url">

                    <div class="item-info">
                        <h3>${item.item_name || "默认名称"}</h3>
                        <p><span>角色：</span>${item.role || "暂无"}</p>
                        <p><span>类型：</span>${item.type || "暂无"}</p>
                        <p><span>剩余数量：</span>${item.quantity || 0}</p>
                        <p><span>介绍：</span>${item.intro || "暂无介绍"}</p>

                        <button class="delete-item-btn" onclick="deleteItem(event, ${item.item_id})">
                            删除制品
                        </button>
                    </div>
                `;

                card.onclick = function () {
                    window.location.href = `detail.html?item_id=${item.item_id}`;
                };

                myItemList.appendChild(card);
            });
        })
        .catch(error => {
            console.error("加载我的制品失败:", error);
             myItemList.innerHTML = `<p class="empty-text">服务器连接失败，请检查后端是否启动哦~</p>`;
            if (myItemCountText) myItemCountText.innerText = "共 0 个制品";
        });
}

function openPublishModal() {
    document.getElementById("publishModal").style.display = "flex";
}

function closePublishModal() {
    document.getElementById("publishModal").style.display = "none";
    document.getElementById("publishForm").reset();
}

function publishItem() {
    const user = getCurrentUser();

    if (!user || !user.user_id) {
        alert("请先登录哦~");
        return;
    }

    const itemName = document.getElementById("itemName").value.trim();
    const role = document.getElementById("role").value.trim();
    const type = document.getElementById("type").value.trim();
    const quantity = document.getElementById("quantity").value.trim();
    // 忽略图片输入，后端直接使用默认图片
    const intro = document.getElementById("intro").value.trim();
    const publishBtn = document.getElementById("publishBtn");

    if (!itemName || !role || !type) {
        alert("咪，制品名称、角色和类型不能为空~");
        return;
    }

    if (!quantity || Number(quantity) <= 0) {
        alert("咪，数量必须大于0哦~");
        return;
    }

    if (publishBtn) {
        publishBtn.disabled = true;
        publishBtn.innerText = "发布中...";
    }

    const params = new URLSearchParams();
    params.append("owner", user.user_id);
    params.append("item_name", itemName);
    params.append("role", role);
    params.append("type", type);
    params.append("quantity", quantity);
    params.append("intro", intro);

    fetch("http://127.0.0.1:8080/items/publish", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"
        },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "发布完成");

        if (data.success) {
            closePublishModal();
            loadMyItems();
        }
    })
    .catch(error => {
        console.error("发布制品失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    })
    .finally(() => {
        if (publishBtn) {
            publishBtn.disabled = false;
            publishBtn.innerText = "确认发布";
        }
    });
}

function deleteItem(event, itemId) {
    event.stopPropagation();

    const user = getCurrentUser();

    if (!user || !user.user_id) {
        alert("请先登录哦~");
        return;
    }

    if (!confirm("确定要删除这个制品吗？删除后无法恢复哦~")) {
        return;
    }

    const params = new URLSearchParams();
    params.append("item_id", itemId);
    params.append("owner", user.user_id);

    fetch("http://127.0.0.1:8080/items/delete", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"
        },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");

        if (data.success) {
            loadMyItems();
        }
    })
    .catch(error => {
        console.error("删除制品失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}