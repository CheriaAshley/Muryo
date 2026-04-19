window.onload = function () {
    loadItems();
};

function loadItems() {
    fetch("http://127.0.0.1:8080/items")
        .then(response => response.json())
        .then(data => {
            const itemList = document.getElementById("itemList");
            itemList.innerHTML = "";

            if (data.length === 0) {
                itemList.innerHTML = "<p>目前暂无制品</p>";
                return;
            }

            data.forEach(item => {
                const card = document.createElement("div");
                card.className = "item-card";

                card.innerHTML = `
                    <h3>${item.item_name}</h3>
                    <p>厨子：${item.owner}</p>
                    <p>角色：${item.item_role}</p>
                    <p>类型：${item.item_type}</p>
                    <p>剩余数量：${item.quantity}</p>
                    <p>介绍：${item.description || "暂无介绍"}</p>
                `;

                itemList.appendChild(card);
            });
        })
        .catch(error => {
            console.error("加载制品失败:", error);
            document.getElementById("itemList").innerHTML = "<p>加载失败，请检查后端是否启动</p>";
        });
}