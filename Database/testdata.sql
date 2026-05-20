/*
 * 该SQL脚本用于初始化测试数据，覆盖了用户、管理员、制品、交换申请等多个表。
 * 包含正常数据和边界情况的数据，以便进行全面的功能测试。
 * 注意：执行该脚本会清空相关表中的旧数据，并重置自增ID，请确保在测试环境中使用。
 * 该脚本可能存在缺陷，例如未考虑某些边界情况或数据一致性问题，测试过程中请注意观察并记录任何异常情况，以便后续修正。
 */
USE muryo;

-- 清空旧测试数据
SET FOREIGN_KEY_CHECKS = 0;

DELETE FROM exdetail;
DELETE FROM exchange;
DELETE FROM admin_apply;
DELETE FROM admin;
DELETE FROM item;
DELETE FROM user;

ALTER TABLE exdetail AUTO_INCREMENT = 1;
ALTER TABLE exchange AUTO_INCREMENT = 1;
ALTER TABLE admin_apply AUTO_INCREMENT = 1;
ALTER TABLE admin AUTO_INCREMENT = 1;
ALTER TABLE item AUTO_INCREMENT = 1;
ALTER TABLE user AUTO_INCREMENT = 1;

SET FOREIGN_KEY_CHECKS = 1;

INSERT INTO user(user_id, user_name, password)
VALUES (1, 'system', '123456')
ON DUPLICATE KEY UPDATE
user_name = VALUES(user_name),
password = VALUES(password);

INSERT INTO admin(user_id, level)
VALUES (1, 3)
ON DUPLICATE KEY UPDATE
level = 3;

-- 插入用户数据（默认用户ID为1的用户已在建立数据库时创建，这里插入更多用户用于测试）
INSERT INTO user(user_id, user_name, password, contact, status, intro) VALUES
(2, 'mika', '123456', 'QQ:10001', 1, '喜欢明日方舟和偶像番，常收集吧唧和透卡'),
(3, 'yuki', '123456', 'QQ:10002', 1, '主收原神、崩铁相关制品'),
(4, 'sora', '123456', 'QQ:10003', 1, '喜欢日常番和轻百合作品'),
(5, 'nana', '123456', 'QQ:10004', 1, '主要交换立牌、色纸和明信片'),
(6, 'haru', '123456', 'QQ:10005', 1, '喜欢收藏角色小卡和拍立得'),
(7, 'rin', '123456', 'QQ:10006', 1, '刚注册的新用户，还没有发布制品'),
(8, 'admin_one', '123456', 'admin1@muryo.com', 1, '一级管理员账号'),
(9, 'admin_two', '123456', 'admin2@muryo.com', 1, '二级管理员账号'),
(10, 'admin_three', '123456', 'admin3@muryo.com', 1, '三级管理员账号'),
(11, 'banned_user', '123456', 'QQ:10007', 0, '该账号用于测试封禁状态'),
(12, 'no_item_user', '123456', 'QQ:10008', 1, '该账号用于测试没有制品的情况');

-- 插入管理员数据
INSERT INTO admin(user_id, level) VALUES
(8, 1),
(9, 2),
(10, 3);

-- 插入制品数据
INSERT INTO item(item_id, owner, item_name, role, type, quantity, status, img_url, intro) VALUES
(1, 1, '默认制品', '默认角色', '默认类型', 999, 0, 'upload/default_item.png', '系统默认制品'),

(2, 2, '拉普兰德音律联觉吧唧', '拉普兰德', '吧唧', 8, 0, 'upload/default_item.png', '58mm吧唧，成色良好'),
(3, 2, '德克萨斯透卡', '德克萨斯', '透卡', 5, 0, 'upload/default_item.png', '适合同角色交换'),
(4, 2, '能天使亚克力立牌', '能天使', '立牌', 3, 0, 'upload/default_item.png', '桌面立牌，保护膜未撕'),
(5, 2, '空弦明信片', '空弦', '明信片', 0, 0, 'upload/default_item.png', '用于测试库存为0'),
(6, 2, '旧版陈晖洁色纸', '陈晖洁', '色纸', 1, 2, 'upload/default_item.png', '用于测试已删除制品'),

(7, 3, '芙宁娜镭射票', '芙宁娜', '票根', 10, 0, 'upload/default_item.png', '希望交换同系列角色'),
(8, 3, '那维莱特吧唧', '那维莱特', '吧唧', 6, 0, 'upload/default_item.png', '可多个一起换'),
(9, 3, '流萤小卡', '流萤', '小卡', 9, 0, 'upload/default_item.png', '崩铁角色小卡'),
(10, 3, '镜流亚克力挂件', '镜流', '挂件', 2, 0, 'upload/default_item.png', '挂件未拆封'),

(11, 4, '后藤一里拍立得', '后藤一里', '拍立得', 4, 0, 'upload/default_item.png', '孤独摇滚相关'),
(12, 4, '喜多郁代吧唧', '喜多郁代', '吧唧', 7, 0, 'upload/default_item.png', '可交换同作品角色'),
(13, 4, '山田凉色纸', '山田凉', '色纸', 2, 0, 'upload/default_item.png', '轻微压痕'),
(14, 4, '伊地知虹夏贴纸', '伊地知虹夏', '贴纸', 12, 0, 'upload/default_item.png', '贴纸套装'),

(15, 5, '初音未来立牌', '初音未来', '立牌', 5, 0, 'upload/default_item.png', '透明底座'),
(16, 5, '镜音铃吧唧', '镜音铃', '吧唧', 8, 0, 'upload/default_item.png', '可同价交换'),
(17, 5, '巡音流歌明信片', '巡音流歌', '明信片', 6, 0, 'upload/default_item.png', '保存较好'),
(18, 5, 'KAITO小卡', 'KAITO', '小卡', 3, 1, 'upload/default_item.png', '用于测试下架状态'),

(19, 6, '木之本樱色纸', '木之本樱', '色纸', 4, 0, 'upload/default_item.png', '魔卡少女樱相关'),
(20, 6, '大道寺知世吧唧', '大道寺知世', '吧唧', 5, 0, 'upload/default_item.png', '支持互换'),
(21, 6, '李小狼透卡', '李小狼', '透卡', 7, 0, 'upload/default_item.png', '透卡无明显划痕'),
(22, 6, '小樱贴纸包', '木之本樱', '贴纸', 15, 0, 'upload/default_item.png', '一包多张');

-- 插入管理员申请数据
INSERT INTO admin_apply(apply_id, admin_id, apply_type, target_id, reason, status, handle_admin_id, handle_time) VALUES
(1, 8, 'delete_item', 6, '制品信息不完整，申请删除', 0, NULL, NULL),
(2, 8, 'delete_item', 18, '该制品疑似重复发布', 1, 10, '2026-05-01 10:20:00'),
(3, 9, 'ban_user', 11, '用户多次发布无效信息', 1, 10, '2026-05-02 14:30:00'),
(4, 9, 'ban_user', 7, '申请封禁该用户，但证据不足', 2, 10, '2026-05-03 16:45:00'),
(5, 10, 'restore_user', 11, '用户申诉后恢复账号', 0, NULL, NULL),
(6, 10, 'promote_admin', 5, '该用户活跃度较高，申请成为一级管理员', 0, NULL, NULL);

-- 插入交换申请主表
-- status: 0待处理 1已拒绝 2待交换 3已完成 4已取消
INSERT INTO exchange(exchange_id, ufrom, uto, date, location, status) VALUES
(1, 2, 3, 0, '待确定', 0),
(2, 2, 4, 0, '待确定', 0),
(3, 3, 2, 0, '教学楼A区', 2),
(4, 3, 5, 0, '图书馆门口', 3),
(5, 4, 2, 0, '食堂一楼', 1),
(6, 4, 6, 0, '待确定', 4),
(7, 5, 3, 0, '宿舍楼下', 2),
(8, 5, 4, 0, '社团活动室', 3),
(9, 6, 5, 0, '校门口', 1),
(10, 6, 2, 0, '待确定', 0),
(11, 2, 5, 0, '待确定', 4),
(12, 3, 6, 0, '图书馆二楼', 3);

-- 插入交换详情数据
INSERT INTO exdetail(detail_id, quantity, exchange_id, item_id) VALUES
(1, 1, 1, 2),
(2, 2, 1, 3),
(3, 1, 2, 4),
(4, 2, 3, 7),
(5, 1, 3, 8),
(6, 3, 4, 9),
(7, 1, 4, 10),
(8, 1, 5, 11),
(9, 2, 5, 12),
(10, 1, 6, 13),
(11, 3, 7, 15),
(12, 2, 7, 16),
(13, 1, 8, 17),
(14, 2, 9, 19),
(15, 1, 9, 20),
(16, 2, 10, 21),
(17, 1, 10, 22),
(18, 1, 11, 2),
(19, 1, 12, 8),
(20, 2, 12, 9);

-- 取消状态的申请如果已经扣过库存，这里手动恢复库存
UPDATE item i
JOIN exdetail d ON i.item_id = d.item_id
JOIN exchange e ON d.exchange_id = e.exchange_id
SET i.quantity = i.quantity + d.quantity
WHERE e.status = 4;