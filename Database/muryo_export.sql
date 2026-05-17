CREATE DATABASE IF NOT EXISTS muryo
DEFAULT CHARACTER SET utf8mb4
-- 比较不区分大小写
COLLATE utf8mb4_unicode_ci;

USE muryo;

SET FOREIGN_KEY_CHECKS = 0;
-- 删除旧对象
-- 删除视图
DROP VIEW IF EXISTS view_exchange_detail;
-- 删除触发器
DROP TRIGGER IF EXISTS trg_exchange_after_update;
DROP TRIGGER IF EXISTS trg_exdetail_before_insert;
DROP TRIGGER IF EXISTS trg_exdetail_after_insert;
-- 删除存储过程
DROP PROCEDURE IF EXISTS proc_handle_exchange;
-- 删除表
DROP TABLE IF EXISTS admin_apply;
DROP TABLE IF EXISTS admin;
DROP TABLE IF EXISTS exdetail;
DROP TABLE IF EXISTS exchange;
DROP TABLE IF EXISTS item;
DROP TABLE IF EXISTS user;

SET FOREIGN_KEY_CHECKS = 1;

-- 用户表
CREATE TABLE user (
  user_id INT NOT NULL AUTO_INCREMENT,
  user_name VARCHAR(50) NOT NULL UNIQUE,
  password VARCHAR(100) NOT NULL,
  contact VARCHAR(100) DEFAULT 'Meowryo还没有填写哟~',
  status INT DEFAULT 1,
  intro VARCHAR(500) DEFAULT '这个Meowryo还没有填写自我介绍哟~',
  create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO user(user_id, user_name, password)
VALUES (1, 'system', '123456');

-- 管理员表是用户的子实体
CREATE TABLE admin (
  user_id INT NOT NULL,
  level INT DEFAULT 1,
  PRIMARY KEY (user_id),
  CONSTRAINT fk_admin_user
  FOREIGN KEY (user_id) REFERENCES user(user_id)
  ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 管理员操作申请表
CREATE TABLE admin_apply (
  apply_id INT NOT NULL AUTO_INCREMENT,
  admin_id INT NOT NULL,
  apply_type VARCHAR(30) NOT NULL,
  target_id INT NOT NULL,
  reason VARCHAR(255) DEFAULT '无',
  status INT DEFAULT 0,
  create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
  handle_admin_id INT DEFAULT NULL,
  handle_time DATETIME DEFAULT NULL,
  PRIMARY KEY (apply_id),
  KEY idx_admin_id (admin_id),
  KEY idx_handle_admin_id (handle_admin_id),
  CONSTRAINT fk_admin_apply_admin
  FOREIGN KEY (admin_id) REFERENCES admin(user_id),
  CONSTRAINT fk_admin_apply_handle_admin
  FOREIGN KEY (handle_admin_id) REFERENCES admin(user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 制品表
CREATE TABLE item (
  item_id INT NOT NULL AUTO_INCREMENT,
  owner INT NOT NULL DEFAULT 1,-- 需要建立一个id为1的默认用户，作为系统默认制品的拥有者
  item_name VARCHAR(50) DEFAULT '默认名称',
  role VARCHAR(50) DEFAULT '默认角色',
  type VARCHAR(50) DEFAULT '默认类型',
  quantity INT DEFAULT 0,
  status INT DEFAULT 0,
  create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
  img_url VARCHAR(255) DEFAULT 'upload/default_item.png',
  intro VARCHAR(255) DEFAULT '暂无介绍',
  PRIMARY KEY (item_id),
  KEY idx_owner (owner),
  CONSTRAINT fk_item_owner
  FOREIGN KEY (owner) REFERENCES user(user_id)
  ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 交换申请主表
-- uto 表示申请人，ufrom 表示被申请人
CREATE TABLE exchange (
  exchange_id INT NOT NULL AUTO_INCREMENT,
  ufrom INT NOT NULL DEFAULT 1,
  uto INT NOT NULL DEFAULT 1,
  date INT DEFAULT 0,
  location VARCHAR(255) DEFAULT '待确定',
  status INT DEFAULT 0,
  create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (exchange_id),
  KEY idx_uto (uto),
  KEY idx_ufrom (ufrom),
  CONSTRAINT fk_exchange_uto
  FOREIGN KEY (uto) REFERENCES user(user_id)
  ON DELETE RESTRICT ON UPDATE RESTRICT,
  CONSTRAINT fk_exchange_ufrom
  FOREIGN KEY (ufrom) REFERENCES user(user_id)
  ON DELETE RESTRICT ON UPDATE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 交换详情表，一条交换申请可以包含多个制品
CREATE TABLE exdetail (
  detail_id INT NOT NULL AUTO_INCREMENT,
  quantity INT NOT NULL DEFAULT 1,
  exchange_id INT NOT NULL,
  item_id INT NOT NULL,
  PRIMARY KEY (detail_id),
  KEY idx_exchange_id (exchange_id),
  KEY idx_item_id (item_id),
  CONSTRAINT fk_exdetail_exchange
  FOREIGN KEY (exchange_id) REFERENCES exchange(exchange_id),
  CONSTRAINT fk_exdetail_item
  FOREIGN KEY (item_id) REFERENCES item(item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

DELIMITER $$

-- 插入交换详情前检查数量、状态和库存
CREATE TRIGGER trg_exdetail_before_insert
BEFORE INSERT ON exdetail
FOR EACH ROW
BEGIN
    DECLARE now_quantity INT;
    DECLARE now_status INT;

    IF NEW.quantity IS NULL OR NEW.quantity <= 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = '申请数量必须大于0';
    END IF;

    SELECT quantity, status
    INTO now_quantity, now_status
    FROM item
    WHERE item_id = NEW.item_id
    FOR UPDATE;

    IF now_status <> 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = '该制品当前不可交换';
    END IF;

    IF now_quantity < NEW.quantity THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = '制品库存不足，不能申请交换';
    END IF;
END$$

-- 插入交换详情后自动扣减库存
CREATE TRIGGER trg_exdetail_after_insert
AFTER INSERT ON exdetail
FOR EACH ROW
BEGIN
    UPDATE item
    SET quantity = quantity - NEW.quantity
    WHERE item_id = NEW.item_id;
END$$

-- 拒绝或取消申请后自动恢复库存
CREATE TRIGGER trg_exchange_after_update
AFTER UPDATE ON exchange
FOR EACH ROW
BEGIN
    IF (OLD.status = 0 AND NEW.status = 1)
       OR (OLD.status = 0 AND NEW.status = 4)
       OR (OLD.status = 2 AND NEW.status = 4) THEN

        UPDATE item i
        JOIN exdetail d ON i.item_id = d.item_id
        SET i.quantity = i.quantity + d.quantity
        WHERE d.exchange_id = NEW.exchange_id;

    END IF;
END$$

-- 处理交换申请：同意、拒绝、完成、取消
CREATE PROCEDURE proc_handle_exchange(
    IN p_exchange_id INT,
    IN p_action VARCHAR(20),
    IN p_user_id INT
)
BEGIN
    DECLARE v_ufrom INT DEFAULT 0;
    DECLARE v_uto INT DEFAULT 0;
    DECLARE v_status INT DEFAULT -1;
    DECLARE v_count INT DEFAULT 0;

    START TRANSACTION;

    SELECT COUNT(*) INTO v_count
    FROM exchange
    WHERE exchange_id = p_exchange_id;

    IF v_count = 0 THEN
        ROLLBACK;
        SELECT 0 AS success, '没有找到对应的交换记录！' AS message;

    ELSE
        SELECT ufrom, uto, status
        INTO v_ufrom, v_uto, v_status
        FROM exchange
        WHERE exchange_id = p_exchange_id
        FOR UPDATE;

        IF p_action = 'agree' THEN
            IF v_ufrom <> p_user_id THEN
                ROLLBACK;
                SELECT 0 AS success, '只能操作自己收到的交换申请！' AS message;
            ELSEIF v_status <> 0 THEN
                ROLLBACK;
                SELECT 0 AS success, '当前状态不能同意申请！' AS message;
            ELSE
                UPDATE exchange
                SET status = 2
                WHERE exchange_id = p_exchange_id;

                COMMIT;
                SELECT 1 AS success, '已经同意交换申请！' AS message;
            END IF;

        ELSEIF p_action = 'reject' THEN
            IF v_ufrom <> p_user_id THEN
                ROLLBACK;
                SELECT 0 AS success, '只能操作自己收到的交换申请！' AS message;
            ELSEIF v_status <> 0 THEN
                ROLLBACK;
                SELECT 0 AS success, '当前状态不能拒绝申请！' AS message;
            ELSE
                UPDATE exchange
                SET status = 1
                WHERE exchange_id = p_exchange_id;

                COMMIT;
                SELECT 1 AS success, '已经拒绝该申请！' AS message;
            END IF;

        ELSEIF p_action = 'complete' THEN
            IF v_ufrom <> p_user_id THEN
                ROLLBACK;
                SELECT 0 AS success, '只能由被申请者确认完成交换！' AS message;
            ELSEIF v_status <> 2 THEN
                ROLLBACK;
                SELECT 0 AS success, '当前状态不能完成交换！' AS message;
            ELSE
                UPDATE exchange
                SET status = 3
                WHERE exchange_id = p_exchange_id;

                COMMIT;
                SELECT 1 AS success, '此次交换已完成！' AS message;
            END IF;

        ELSEIF p_action = 'cancel' THEN
            IF p_user_id <> v_uto AND p_user_id <> v_ufrom THEN
                ROLLBACK;
                SELECT 0 AS success, '只能由申请相关用户取消交换！' AS message;
            ELSEIF v_status <> 0 AND v_status <> 2 THEN
                ROLLBACK;
                SELECT 0 AS success, '当前状态不能取消交换！' AS message;
            ELSE
                UPDATE exchange
                SET status = 4
                WHERE exchange_id = p_exchange_id;

                COMMIT;
                SELECT 1 AS success, '已经取消该交换申请！' AS message;
            END IF;

        ELSE
            ROLLBACK;
            SELECT 0 AS success, '无效的操作！' AS message;
        END IF;
    END IF;
END$$

DELIMITER ;

-- 交换详情视图，方便前端查询申请人、被申请人和制品信息
CREATE VIEW view_exchange_detail AS
SELECT
    e.exchange_id,
    e.uto AS apply_user_id,
    u_apply.user_name AS apply_user_name,
    e.ufrom AS target_user_id,
    u_target.user_name AS target_user_name,
    d.detail_id,
    d.item_id,
    i.item_name,
    d.quantity,
    e.status,
    CASE
        WHEN e.status = 0 THEN '待处理'
        WHEN e.status = 1 THEN '已拒绝'
        WHEN e.status = 2 THEN '待交换'
        WHEN e.status = 3 THEN '已完成'
        WHEN e.status = 4 THEN '已取消'
        ELSE '未知状态'
    END AS status_text
FROM exchange e
JOIN exdetail d ON e.exchange_id = d.exchange_id
JOIN item i ON d.item_id = i.item_id
JOIN user u_apply ON e.uto = u_apply.user_id
JOIN user u_target ON e.ufrom = u_target.user_id;