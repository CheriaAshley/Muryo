window.onload = function () {
    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadProfile();
    loadMyItems();
    loadMyCollections(); 
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
            window.currentUser = profile;
            
            profileCard.innerHTML = `
            <h2>个人信息</h2>
            <div class="profile-details">
                <p><strong>用户ID：</strong>${profile.user_id}</p>
                <p><strong>用户名：</strong>${profile.user_name || "暂无"}</p>
                <p><strong>联系方式：</strong>${profile.contact || "暂无"}</p>
                <p><strong>个人介绍：</strong>${profile.introduction || profile.intro || "这个人很神秘，还没有留下介绍~"}</p>
            </div>
            <button class="edit-btn" onclick="openEditModal()">修改个人信息</button>
        `;
        })
        .catch(error => {
            console.error("加载个人信息失败:", error);
            profileCard.innerHTML = `<h2>个人信息</h2><p>加载失败，请检查后端是否启动</p>`;
        });
}

function openEditModal() {
    const modal = document.getElementById("editProfileModal");
    modal.style.display = "flex";
    if (window.currentUser) {
        document.getElementById("editUserName").value = window.currentUser.user_name || "";
        document.getElementById("editContact").value = window.currentUser.contact || "";
        document.getElementById("editIntro").value = window.currentUser.introduction || window.currentUser.intro || "";
    }
}

function closeEditModal() {
    document.getElementById("editProfileModal").style.display = "none";
}

async function saveProfile() {
    const user = getCurrentUser(); 
    const params = new URLSearchParams();
    
    params.append("user_id", user.user_id);
    params.append("user_name", document.getElementById("editUserName").value);
    params.append("contact", document.getElementById("editContact").value);
    params.append("intro", document.getElementById("editIntro").value);
    params.append("password", document.getElementById("editPassword").value);

    try {
        const response = await fetch("http://127.0.0.1:8080/user/update", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: params.toString()
        });
        const data = await response.json();
        if (data.success) {
            alert("更新成功！");
            location.reload(); 
        } else {
            alert("更新失败: " + data.message);
        }
    } catch (e) {
        alert("服务器好像开小差了，请稍后再试~");
    }
}

function loadMyItems() {
    const user = getCurrentUser();
    const myItemList = document.getElementById("myItemList");
    const myItemCountText = document.getElementById("myItemCountText");

    myItemList.innerHTML = `<p class="loading-text">正在加载我的制品...</p>`;
    if (myItemCountText) myItemCountText.innerText = "正在统计...";

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
            if (myItemCountText) myItemCountText.innerText = "共 " + items.length + " 个制品";

            if (items.length === 0) {
                myItemList.innerHTML = `<p class="empty-text">${data.message || "咪还没有发布制品哦~"}</p>`;
                return;
            }

            items.forEach(item => {
                const card = document.createElement("div");
                card.className = "item-card";
                const img_url = item.img_url && item.img_url.trim() !== "" ? item.img_url : "upload/default_item.png";
                    
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

function loadMyCollections() {
    const user = getCurrentUser();
    const collectionList = document.getElementById("myCollectionList");
    const collectionCountText = document.getElementById("myCollectionCountText");

    collectionList.innerHTML = `<p class="loading-text">正在加载我的收藏...</p>`;
    if (collectionCountText) collectionCountText.innerText = "正在统计...";

    fetch(`http://127.0.0.1:8080/favorite/list?user_id=${encodeURIComponent(user.user_id)}`)
        .then(response => response.json())
        .then(data => {
            collectionList.innerHTML = "";
            if (!data.success) {
                collectionList.innerHTML = `<p class="empty-text">${data.message || "加载失败"}</p>`;
                if (collectionCountText) collectionCountText.innerText = "共 0 个收藏";
                return;
            }

            const items = Array.isArray(data.data) ? data.data : [];
            if (collectionCountText) collectionCountText.innerText = "共 " + items.length + " 个收藏";

            if (items.length === 0) {
                collectionList.innerHTML = `<p class="empty-text">你还没有收藏任何制品哦~快去制品广场逛逛吧！</p>`;
                return;
            }

            items.forEach(item => {
                const card = document.createElement("div");
                card.className = "item-card";
                const img_url = item.img_url && item.img_url.trim() !== "" ? item.img_url : "upload/default_item.png";
                    
                card.innerHTML = `
                    <img src="${img_url}" alt="${item.item_name || "制品图片"}" class="item-img_url">
                    <div class="item-info">
                        <h3>${item.item_name || "默认名称"}</h3>
                        <p><span>拥有者：</span>${item.owner_name || "未知"}</p>
                        <p><span>角色：</span>${item.role || "暂无"}</p>
                        <p><span>类型：</span>${item.type || "暂无"}</p>
                        <p><span>剩余数量：</span>${item.quantity || 0}</p>
                        
                        <button class="cancel-fav-btn" onclick="removeFavorite(event, ${item.item_id})">
                            取消收藏
                        </button>
                    </div>
                `;

                card.onclick = function () {
                    openApplyModal(item.item_id, item.owner, item.owner_name, item.item_name, item.quantity);
                };
                collectionList.appendChild(card);
            });
        })
        .catch(error => {
            console.error("加载我的收藏失败:", error);
            collectionList.innerHTML = `<p class="empty-text">服务器连接失败，请检查后端是否启动哦~</p>`;
            if (collectionCountText) collectionCountText.innerText = "共 0 个收藏";
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
    const imgUrl = document.getElementById("itemImage").value.trim(); 
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
    params.append("img_url", imgUrl); 
    params.append("intro", intro);

    fetch("http://127.0.0.1:8080/items/publish", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
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
        headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
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

let currentApplyItem = null;

function openApplyModal(itemId, ownerId, ownerName, itemName, maxQuantity) {
    currentApplyItem = { item_id: itemId, owner: ownerId, max_quantity: maxQuantity };

    document.getElementById("applyOwnerName").innerText = ownerName || "神秘用户";
    document.getElementById("applyItemName").innerText = itemName;
    document.getElementById("applyMaxQty").innerText = maxQuantity;
    
    document.getElementById("applyQuantity").value = 1;
    document.getElementById("applyQuantity").max = maxQuantity;

    document.getElementById("applyModal").style.display = "flex";
}

function closeApplyModal() {
    document.getElementById("applyModal").style.display = "none";
    currentApplyItem = null;
}

function submitApply() {
    if (!currentApplyItem) return;

    const user = getCurrentUser();
    if (!user || !user.user_id) {
        alert("请先登录才能申请交换哦~");
        return;
    }

    const quantity = parseInt(document.getElementById("applyQuantity").value);

    if (quantity <= 0 || quantity > currentApplyItem.max_quantity) {
        alert(`申请数量必须大于0，且不能超过当前余量 (${currentApplyItem.max_quantity}) 哦~`);
        return;
    }

    const applyBtn = document.getElementById("submitApplyBtn");
    if (applyBtn) {
        applyBtn.disabled = true;
        applyBtn.innerText = "提交中...";
    }

    const params = new URLSearchParams();
    params.append("ufrom", currentApplyItem.owner); 
    params.append("uto", user.user_id); 
    params.append("item_ids", currentApplyItem.item_id);
    params.append("quantities", quantity); 

    fetch("http://127.0.0.1:8080/exchange/apply", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "申请结果返回");
        if (data.success) {
            closeApplyModal();
            loadMyCollections(); 
        }
    })
    .catch(error => {
        console.error("申请交换失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    })
    .finally(() => {
        if (applyBtn) {
            applyBtn.disabled = false;
            applyBtn.innerText = "发送申请";
        }
    });
}

function removeFavorite(event, itemId) {
    event.stopPropagation(); 

    const user = getCurrentUser();
    if (!user || !user.user_id) {
        alert("请先登录哦~");
        return;
    }

    if (!confirm("确定要取消收藏这个制品吗？")) {
        return;
    }

    const params = new URLSearchParams();
    params.append("user_id", user.user_id);
    params.append("item_id", itemId);

    fetch("http://127.0.0.1:8080/favorite/remove", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
        body: params.toString()
    })
    .then(response => response.json())
    .then(data => {
        alert(data.message || "操作完成");
        if (data.success) {
            loadMyCollections();
        }
    })
    .catch(error => {
        console.error("取消收藏失败:", error);
        alert("服务器连接失败，请检查后端是否启动哦~");
    });
}