/*==============================================================*/
/* DBMS name:      MySQL 5.0                                    */
/* Created on:     2026/4/11 15:20:18                           */
/*==============================================================*/


drop table if exists Admin;

drop table if exists Exchange;

drop table if exists Exdetail;

drop table if exists Item;

drop table if exists user;

/*==============================================================*/
/* Table: Admin                                                 */
/*==============================================================*/
create table Admin
(
   admin_id             int not null,
   admin_name           varchar(50) not null,
   password             varchar(100) not null,
   primary key (admin_id)
);

alter table Admin comment '此处用于说明管理员基本信息以及可能拥有的权限：
信息：
1.用户名
2.用户密码
                          ';

/*==============================================================*/
/* Table: Exchange                                              */
/*==============================================================*/
create table Exchange
(
   exchange_id          int not null,
   user_id              int,
   use_user_id          int,
   admin_id             int,
   date                 date not null,
   location             varchar(255) not null,
   status               int not null,
   primary key (exchange_id)
);

alter table Exchange comment '此处说明交换功能（系统核心）
1.交换编号（类似于订单编号方便用户和管理员精准锁定某一条记录）
';

/*==============================================================*/
/* Table: Exdetail                                              */
/*==============================================================*/
create table Exdetail
(
   quantity             int not null,
   detail_id            int not null,
   exchange_id          int,
   primary key (detail_id)
);

alter table Exdetail comment '此处表示交换明细
1.交换制品
2.交换数量
3.明细id
4.交换id';

/*==============================================================*/
/* Table: Item                                                  */
/*==============================================================*/
create table Item
(
   item_id              int not null,
   user_id              int,
   detail_id            int,
   admin_id             int,
   item_name            varchar(50) not null,
   role                 varchar(50) not null,
   type                 varchar(50) not null,
   quantity             int not null,
   stasus               int not null,
   primary key (item_id)
);

alter table Item comment '此处用于说明制品相关信息
1.制品角色
2.制品形式
3.制品数量
4.';

/*==============================================================*/
/* Table: user                                                  */
/*==============================================================*/
create table user
(
   user_id              int not null,
   admin_id             int,
   user_name            varchar(50) not null,
   password             varchar(100) not null,
   contact              varchar(100),
   status               int not null,
   introduction         varchar(500),
   primary key (user_id)
);

alter table user comment '此处说明用户端的基本信息即属性：
1.用户ID（区分用户的唯一标识、登录凭证）
2.用户名（可以';

alter table Exchange add constraint FK_check foreign key (admin_id)
      references Admin (admin_id) on delete restrict on update restrict;

alter table Exchange add constraint FK_ufrom foreign key (use_user_id)
      references user (user_id) on delete restrict on update restrict;

alter table Exchange add constraint FK_uto foreign key (user_id)
      references user (user_id) on delete restrict on update restrict;

alter table Exdetail add constraint FK_contains foreign key (exchange_id)
      references Exchange (exchange_id) on delete restrict on update restrict;

alter table Item add constraint FK_belongs foreign key (detail_id)
      references Exdetail (detail_id) on delete restrict on update restrict;

alter table Item add constraint FK_manage_item foreign key (admin_id)
      references Admin (admin_id) on delete restrict on update restrict;

alter table Item add constraint FK_owner foreign key (user_id)
      references user (user_id) on delete restrict on update restrict;

alter table user add constraint FK_manage foreign key (admin_id)
      references Admin (admin_id) on delete restrict on update restrict;

