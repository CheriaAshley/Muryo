#include <iostream>
#include <string>
#include "httplib.h"//HTTP服务器库
#include <mysql.h>//MySQl数据库接口

using namespace httplib;//必须使用和std不同
using namespace std;
MYSQL* connect_db() {//链接数据库并返回连接对象
    MYSQL* conn = mysql_init(NULL);//创建初始化连接结构返回地址
    if (conn == NULL) {
        cout << "mysql_init failed" << endl;//这一步和密码等权限问题无关
        return NULL;
    }
    conn = mysql_real_connect(//连接数据库
        conn,
        "127.0.0.1",      // host
        "muryo_user",     // user
        "muryo123",        // password
        "muryo",          // database
        3306,             // port
        NULL,
        0
    );

    if (conn == NULL) {
        cout << "mysql_real_connect failed: " << mysql_error(conn) << endl;
        return NULL;
    }

    // 设置字符集，防止中文乱码
    mysql_set_character_set(conn, "utf8");

    return conn;
}
int main() {
    Server svr;
    //主界面
    svr.Get("/", [](const Request&, Response& res) {
        res.set_content("Muryo home page", "text/plain; charset=UTF-8");
        });
    //测试后端运行
    svr.Get("/test", [](const Request&, Response& res) {
        res.set_content("Muryo backend is running", "text/plain; charset=UTF-8");
        });
    //注册界面
    svr.Post("/register", [](const Request& req, Response& res) {
        string username = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        if (username.empty() || password.empty()) {
            res.set_content("username or password is empty", "text/plain; charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("database connection failed", "text/plain; charset=UTF-8");
            return;
        }
        //SQL代码生成
        string sql = "INSERT INTO user(user_name, password) VALUES('" + username + "','" + password + "')";

        int result = mysql_query(conn, sql.c_str());
        if (result != 0) {
            string err = mysql_error(conn);
            mysql_close(conn);
            res.set_content("register failed: " + err, "text/plain; charset=UTF-8");
            return;
        }

        mysql_close(conn);
        res.set_content("register success", "text/plain; charset=UTF-8");
        });
    // 登录接口
    svr.Post("/login", [](const Request& req, Response& res) {
        string username = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("Database connection failed", "text/plain; charset=UTF-8");
            return;
        }

        //生成SQL语句
        string sql = "SELECT * FROM user WHERE user_name='" + username + "' AND password='" + password + "'";

        if (mysql_query(conn, sql.c_str())) {
            string err = "Query failed: ";
            err += mysql_error(conn);
            res.set_content(err, "text/plain; charset=UTF-8");
            mysql_close(conn);
            return;
        }

        // 获取查询结果
        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            res.set_content("Failed to get result", "text/plain; charset=UTF-8");
            mysql_close(conn);
            return;
        }

        // 判断有没有查到数据
        MYSQL_ROW row = mysql_fetch_row(result);

        if (row) {
            res.set_content("Login success", "text/plain; charset=UTF-8");
        }
        else {
            res.set_content("Username or password is incorrect", "text/plain; charset=UTF-8");
        }

        mysql_free_result(result);
        mysql_close(conn);
        });
    //发布制品接口
    svr.Post("/publish", [](const Request& req, Response& res) {

        string user_id = req.get_param_value("user_id");
        string name = req.get_param_value("name");//制品名称
        string type = req.get_param_value("type");
        string intro = req.get_param_value("intro");
        string quantity = req.get_param_value("quantity");

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        string sql = "INSERT INTO item(owner, item_name, type, intro, quantity) VALUES('"
            + user_id + "','" + name + "','" + type + "','" + intro + "','" + quantity + "')";

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("Publish failed", "text/plain");
        }
        else {
            res.set_content("Publish success", "text/plain");
        }

        mysql_close(conn);
        });
    //申请交换
    svr.Post("/exchange/apply", [](const Request& req, Response& res) {
        string item_idstr = req.get_param_value("item_id");
        string utostr = req.get_param_value("uto");

        int item_id, uto;
        //检测无效字符
        try {
            item_id = stoi(item_idstr);
            uto = stoi(utostr);
        }
        catch (...) {
            res.set_content("item_id or uto invalid", "text/plain");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        // 查询制品信息
        string sql_select = "SELECT owner, quantity, status FROM item WHERE item_id = " + to_string(item_id);
        if (mysql_query(conn, sql_select.c_str())) {
            res.set_content("Query item failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("Store result failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {
            res.set_content("Item not found", "text/plain");
            mysql_free_result(result);
            mysql_close(conn);
            return;
        }

        int ufrom = stoi(row[0]);
        int quantity = stoi(row[1]);
        int status = stoi(row[2]);

        mysql_free_result(result);

        // 不能和自己交换
        if (ufrom == uto) {
            res.set_content("You cannot exchange your own item", "text/plain");
            mysql_close(conn);
            return;
        }

        // 数量大于0才可以交换
        if (quantity <= 0) {
            res.set_content("Meowryo来晚一步~制品已经换完啦", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        // 状态必须可交换(0可以，1不行）
        if (status != 0) {
            res.set_content("这个制品不可以交换~", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        //  插入交换记录
        string sql_insert = "INSERT INTO exchange(item_id, ufrom, requester_id, exchange_status) VALUES("
            + to_string(item_id) + ","
            + to_string(ufrom) + ","
            + to_string(uto) + ",0)";

        if (mysql_query(conn, sql_insert.c_str())) {
            res.set_content("Apply exchange failed", "text/plain");
        }
        else {
            res.set_content("Apply exchange success", "text/plain");
        }

        mysql_close(conn);
        });
    //查看收到的申请
    svr.Get("/exchange/incoming", [](const Request& req, Response& res) {
        string ufromstr = req.get_param_value("ufrom");
        int ufrom;

        try {
            ufrom = stoi(ufromstr);
        }
        catch (...) {
            res.set_content("Invalid ufrom", "text/plain");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        string sql = "SELECT e.exchange_id, e.item_id, i.item_name, e.ufrom, e.status "
            "FROM exchange e "
            "JOIN item i ON e.item_id = i.item_id "
            "WHERE e.ufrom = " + to_string(ufrom);

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("Query incoming exchanges failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("Store result failed", "text/plain");
            mysql_close(conn);
            return;
        }

        string output = "";
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            output += "exchange_id: " + string(row[0]) + "\n";
            output += "item_id: " + string(row[1]) + "\n";
            output += "item_name: " + string(row[2]) + "\n";
            output += "ufrom: " + string(row[3]) + "\n";
            output += "status: " + string(row[4]) + "\n";
            output += "-------------------\n";
        }

        if (output.empty()) {
            output = "还没有Meowryo和你交换哦~快去寻找同好叭~  ";
        }
        mysql_free_result(result);
        mysql_close(conn);
        res.set_content(output, "text/plain");
        });
    //处理申请
    svr.Post("/exchange/handle", [](const Request& req, Response& res) {
        string exchange_idstr = req.get_param_value("exchange_id");
        string action = req.get_param_value("action");

        int exchange_id;
        try {
            exchange_id = stoi(exchange_idstr);
        }
        catch (...) {
            res.set_content("Invalid exchange_id", "text/plain");
            return;
        }

        if (action != "agree" && action != "reject") {
            res.set_content("Invalid action", "text/plain");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        //  先查这条申请是否存在，以及当前状态
        string sql_select = "SELECT item_id, status FROM exchange WHERE exchange_id = " + to_string(exchange_id);

        if (mysql_query(conn, sql_select.c_str())) {
            res.set_content("Query exchange record failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("Store result failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);
            res.set_content("Exchange record not found", "text/plain");
            return;
        }

        int item_id = stoi(row[0]);
        int status = stoi(row[1]);

        mysql_free_result(result);

        if (status != 0) {
            mysql_close(conn);
            res.set_content("This exchange has already been handled", "text/plain");
            return;
        }

        // 更新交换申请状态
        int newstatus = (action == "agree") ? 1 : 2;
        string sql_update_exchange = "UPDATE exchange SET status = " + to_string(newstatus) +
            " WHERE exchange_id = " + to_string(exchange_id);

        if (mysql_query(conn, sql_update_exchange.c_str())) {
            res.set_content("Update exchange status failed", "text/plain");
            mysql_close(conn);
            return;
        }

        //  如果同意，把 item 状态改掉
        if (action == "agree") {
            string sql_update_item = "UPDATE item SET status = 0 WHERE item_id = " + to_string(item_id);

            if (mysql_query(conn, sql_update_item.c_str())) {
                res.set_content("Exchange agreed, but item status update failed", "text/plain");
                mysql_close(conn);
                return;
            }
        }

        mysql_close(conn);

        if (action == "agree") {
            res.set_content("Exchange request agreed", "text/plain");
        }
        else {
            res.set_content("Exchange request rejected", "text/plain");
        }
        });
    //查看自己发出的申请
    svr.Get("/exchange/outgoing", [](const Request& req, Response& res) {
        string utostr = req.get_param_value("uto");
        int uto;

        try {
            uto = stoi(utostr);
        }
        catch (...) {
            res.set_content("Invalid requester_id", "text/plain");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        string sql = "SELECT e.exchange_id, e.item_id, i.item_name, e.ufrom, e.status"
            "FROM exchange e "
            "JOIN item i ON e.item_id = i.item_id "
            "WHERE e.uto = " + to_string(uto);

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("Query outgoing exchanges failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("Store result failed", "text/plain");
            mysql_close(conn);
            return;
        }

        string output = "";
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            output += "exchange_id: " + string(row[0]) + "\n";
            output += "item_id: " + string(row[1]) + "\n";
            output += "item_name: " + string(row[2]) + "\n";
            output += "ufrom: " + string(row[3]) + "\n";
            output += "status: " + string(row[4]) + "\n";
            output += "-------------------\n";
        }

        if (output.empty()) {
            output = "No outgoing exchange requests";
        }

        mysql_free_result(result);
        mysql_close(conn);
        res.set_content(output, "text/plain");
        });
    //查看所有制品列表
    svr.Get("/items", [](const Request& req, Response& res) {

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        string sql = "SELECT *"
            "FROM item WHERE status = 1 AND quantity > 0 ";

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("Query items failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("Store result failed", "text/plain");
            mysql_close(conn);
            return;
        }

        string output = "";
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            output += "item_id: " + string(row[0]) + "\n";
            output += "owner: " + string(row[1]) + "\n";
            output += "name: " + string(row[2]) + "\n";
            output += "role: " + string(row[3]) + "\n";
            output += "type: " + string(row[4]) + "\n";
            output += "count: " + string(row[5]) + "\n";
            output += "image: " + string(row[7]) + "\n";
            output += "intro: " + string(row[8]) + "\n";
            output += "----------------------\n";
        }

        if (output.empty()) {
            output = "No items available";
        }

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(output, "text/plain");
        });
    //查看我的制品
    svr.Get("/my/items", [](const Request& req, Response& res) {

        string user_idstr = req.get_param_value("user_id");
        int user_id;

        try {
            user_id = stoi(user_idstr);
        }
        catch (...) {
            res.set_content("Invalid user_id", "text/plain");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        string sql = "SELECT * "
            "FROM item WHERE owner = " + to_string(user_id);

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("Query my items failed", "text/plain");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("Store result failed", "text/plain");
            mysql_close(conn);
            return;
        }

        string output = "";
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            output += "item_id: " + string(row[0]) + "\n";
            output += "owner: " + string(row[1]) + "\n";
            output += "name: " + string(row[2]) + "\n";
            output += "role: " + string(row[3]) + "\n";
            output += "type: " + string(row[4]) + "\n";
            output += "count: " + string(row[5]) + "\n";
            output += "image: " + string(row[7]) + "\n";
            output += "intro: " + string(row[8]) + "\n";
            output += "----------------------\n";
        }

        if (output.empty()) {
            output = "No items found";
        }

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(output, "text/plain");
        });
    //删除制品
    svr.Post("/item/delete", [](const Request& req, Response& res) {

        string item_idstr = req.get_param_value("item_id");
        int item_id;

        try {
            item_id = stoi(item_idstr);
        }
        catch (...) {
            res.set_content("Invalid item_id", "text/plain");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("DB connection failed", "text/plain");
            return;
        }

        string sql = "DELETE FROM item WHERE item_id = " + to_string(item_id);

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("Delete item failed", "text/plain");
        }
        else {
            res.set_content("Delete item success", "text/plain");
        }

        mysql_close(conn);
        });
    cout << "Server running at http://127.0.0.1:8080" << endl;
    svr.listen("127.0.0.1", 8080);

    return 0;
}