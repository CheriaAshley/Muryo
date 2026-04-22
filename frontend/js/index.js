window.onload = function () {
    updateLoginButtons();
    loadItems();
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

function loadItems() {
    fetch("http://127.0.0.1:8080/items")
        .then(response => response.json())
        .then(data => {
            const itemList = document.getElementById("itemList");
            itemList.innerHTML = "";

            if (!Array.isArray(data) || data.length === 0) {
                itemList.innerHTML = "<p>目前暂无制品</p>";
                return;
            }

            data.forEach(item => {
                const card = document.createElement("div");
                card.className = "item-card";

                card.innerHTML = `
                    <h3>${item.item_name || "默认名称"}</h3>
                    <p>厨子：${item.owner_name || "未知用户"}</p>
                    <p>角色：${item.role || "默认角色"}</p>
                    <p>类型：${item.type || "默认类型"}</p>
                    <p>剩余数量：${item.quantity || 0}</p>
                    <p>介绍：${item.intro || "暂无介绍"}</p>
                `;

                card.onclick = function () {
                    window.location.href = `detail.html?item_id=${item.item_id}`;
                };

                itemList.appendChild(card);
            });
        })
        .catch(error => {
            console.error("加载制品失败:", error);
            document.getElementById("itemList").innerHTML = "<p>加载失败，请检查后端是否启动</p>";
        });
}

function searchItems() {
    const keyword = document.getElementById("searchInput").value.trim().toLowerCase();
    const cards = document.querySelectorAll(".item-card");

    cards.forEach(card => {
        const text = card.innerText.toLowerCase();
        if (text.includes(keyword)) {
            card.style.display = "block";
        } else {
            card.style.display = "none";
        }
    });
}