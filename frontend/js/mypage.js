const BASE_URL = "http://127.0.0.1:8080";
let currentApplyItem = null;
let currentUser = null;

document.addEventListener("DOMContentLoaded", () => {
    bindMypageEvents();

    if (!checkLogin()) return;

    updateLoginState();
    updateAdminEntry();
    loadProfile();
    loadMyItems();
    loadMyCollections();
});

/**
 * 统一绑定事件
 */
function bindMypageEvents() {
    // 顶栏
    bindClick("loginBtn", goLogin);
    bindClick("logoutBtn", logout);

    // 侧边栏
    bindClick("homeNav", goHome);
    bindClick("myPageNav", goMyPage);
    bindClick("myApplyNav", goMyApply);
    bindClick("myExchangeNav", goMyExchange);
    bindClick("todoApplyNav", goTodoApply);
    bindClick("adminEntry", goAdminCenter);

    // 侧栏支持键盘
    ["homeNav","myPageNav","myApplyNav","myExchangeNav","todoApplyNav","adminEntry"].forEach(id=>{
        const el = document.getElementById(id);
        if(el){
            el.addEventListener("keydown", (e)=>{
                if(e.key === "Enter" || e.key === " ") el.click();
            });
        }
    });

    // 弹窗关闭
    bindClick("closePublishModalBtn", closePublishModal);
    bindClick("openPublishModalBtn", openPublishModal);
    bindClick("closeEditProfileBtn", closeEditModal);
    bindClick("closeApplyModalBtn", closeApplyModal);

    // 表单提交
    const publishForm = document.getElementById("publishForm");
    if(publishForm) publishForm.addEventListener("submit", e=>{
        e.preventDefault();
        publishItem();
    });

    const editProfileForm = document.getElementById("editProfileForm");
    if(editProfileForm) editProfileForm.addEventListener("submit", e=>{
        e.preventDefault();
        saveProfile();
    });

    const applyForm = document.getElementById("applyForm");
    if(applyForm) applyForm.addEventListener("submit", e=>{
        e.preventDefault();
        submitApply();
    });

    // 动态删除按钮、取消收藏、点击卡片等用事件委托
    const myItemList = document.getElementById("myItemList");
    if(myItemList){
        myItemList.addEventListener("click", handleMyItemListClick);
    }

    const myCollectionList = document.getElementById("myCollectionList");
    if(myCollectionList){
        myCollectionList.addEventListener("click", handleMyCollectionClick);
    }
}

/**
 * 简化点击事件绑定
 */
function bindClick(id, handler){
    const el = document.getElementById(id);
    if(el) el.addEventListener("click", handler);
}

/**
 * 删除、点击卡片事件委托
 */
function handleMyItemListClick(event){
    const delBtn = event.target.closest(".delete-item-btn");
    if(delBtn){
        const itemId = delBtn.dataset.itemId;
        deleteItem(event, itemId);
        return;
    }
    const card = event.target.closest(".item-card");
    if(card){
        const itemId = card.dataset.itemId;
        if(itemId) window.location.href = `detail.html?item_id=${itemId}`;
    }
}

function handleMyCollectionClick(event){
    const cancelFavBtn = event.target.closest(".cancel-fav-btn");
    if(cancelFavBtn){
        const itemId = cancelFavBtn.dataset.itemId;
        removeFavorite(event, itemId);
        return;
    }
    const card = event.target.closest(".item-card");
    if(card){
        const itemId = card.dataset.itemId;
        const ownerId = card.dataset.ownerId;
        const ownerName = card.dataset.ownerName;
        const itemName = card.dataset.itemName;
        const maxQty = card.dataset.maxQuantity;
        openApplyModal(itemId, ownerId, ownerName, itemName, maxQty);
    }
}