window.onload = function () {
    loadItemDetail();
};

function loadItemDetail() {
    const params = new URLSearchParams(window.location.search);
    const itemId = params.get("item_id");

    const detailContainer = document.getElementById("detailContainer");

    if (!itemId) {
        detailContainer.innerHTML = "<p>没有找到制品ID</p>";
        return;
    }

    fetch(`http://127.0.0.1:8080/item/detail?item_id=${itemId}`)
        .then(response => {
            if (!response.ok) {
                throw new Error("网络响应失败");
            }
            return response.json();
        })
        .then(item => {
            if (!item.success) {
                detailContainer.innerHTML = `<p>${item.message}</p>`;
                return;
            }

            const data = item.data;

            detailContainer.innerHTML = `
                <h1>${data.item_name || "默认名称"}</h1>
                <p><span>厨子：</span>${data.owner_name || "未知用户"}</p>
                <p><span>角色：</span>${data.role || "默认角色"}</p>
                <p><span>类型：</span>${data.type || "默认类型"}</p>
                <p><span>剩余数量：</span>${data.quantity || 0}</p>
                <p><span>介绍：</span>${data.intro || "暂无介绍"}</p>
                <button onclick="goBack()">返回首页</button>
            `;
        })
        .catch(error => {
            console.error("加载详情失败:", error);
            detailContainer.innerHTML = "<p>详情加载失败</p>";
        });
}

function goBack() {
    window.location.href = "index.html";
}