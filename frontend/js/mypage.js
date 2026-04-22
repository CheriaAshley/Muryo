window.onload = function () {
    if (!checkLogin()) return;

    updateLoginButtons();
    loadProfile();
    loadMyItems();
};

function updateLoginButtons() {
    const user = getCurrentUser();

    const loginBtn = document.getElementById("loginBtn");
    const logoutBtn = document.getElementById("logoutBtn");
    const welcomeText = document.getElementById("welcomeText");

    if (user.user_id && user.user_name) {
        if (loginBtn) loginBtn.style.display = "none";
        if (logoutBtn) logoutBtn.style.display = "inline-block";
        if (welcomeText) welcomeText.innerText = "欢迎你，" + user.user_name;
    } else {
        if (loginBtn) loginBtn.style.display = "inline-block";
        if (logoutBtn) logoutBtn.style.display = "none";
        if (welcomeText) welcomeText.innerText = "";
    }
}

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

            profileCard.innerHTML = `
                <h2>个人信息</h2>
                <p><strong>用户ID：</strong>${data.user_id}</p>
                <p><strong>用户名：</strong>${data.user_name || "暂无"}</p>
                <p><strong>联系方式：</strong>${data.contact || "暂无"}</p>
                <p><strong>个人介绍：</strong>${data.introduction || "这个人很神秘，还没有留下介绍~"}</p>
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

    console.log("当前登录用户：", user);

    if (!user || !user.user_id) {
        myItemList.innerHTML = `<p class="empty-text">未获取到登录用户信息，请重新登录</p>`;
        return;
    }

    const url = `http://127.0.0.1:8080/items/my?owner=${encodeURIComponent(user.user_id)}`;
    console.log("请求我的制品地址：", url);

    fetch(url)
        .then(response => response.json())
        .then(data => {
            console.log("我的制品接口返回：", data);
            myItemList.innerHTML = "";

            if (!Array.isArray(data)) {
                myItemList.innerHTML = `
                    <p class="empty-text">${data.message || "加载失败"}</p>
                `;
                return;
            }

            const items = data;

            if (items.length === 0) {
                myItemList.innerHTML = `
                    <p class="empty-text">咪还没有发布制品~快去和同好分享美味家产叭！</p>
                `;
                return;
            }

            items.forEach(item => {
                const card = document.createElement("div");
                card.className = "item-card";

                const imageHtml = item.image_url
                    ? `<img src="${item.image_url}" alt="${item.item_name}" class="item-image">`
                    : `<div class="item-image placeholder">暂无图片</div>`;

                card.innerHTML = `
                    ${imageHtml}
                    <div class="item-info">
                        <h3>${item.item_name || "默认名称"}</h3>
                        <p><span>角色：</span>${item.role || "暂无"}</p>
                        <p><span>类型：</span>${item.type || "暂无"}</p>
                        <p><span>剩余数量：</span>${item.quantity ?? 0}</p>
                        <p><span>介绍：</span>${item.intro || "暂无介绍"}</p>
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
            myItemList.innerHTML = `<p class="empty-text">加载失败，请检查接口返回是否是合法JSON</p>`;
        });
}