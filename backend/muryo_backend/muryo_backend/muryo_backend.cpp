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
vector<string> split(const string& s, char delimiter) {
    vector<string> result;
    string temp;
    stringstream ss(s);

    while (getline(ss, temp, delimiter)) {
        result.push_back(temp);
    }

    return result;
}
int main() {
    system("chcp 65001");
    Server svr;
    //主界面
    svr.Get("/", [](const Request&, Response& res) {
        res.set_content("欢迎来到Muryo主页~", "text/plain; charset=UTF-8");
        });
    //测试后端运行
    svr.Get("/test", [](const Request&, Response& res) {
        res.set_content("Muryo正在运行", "text/plain; charset=UTF-8");
        });
    //注册界面
    svr.Post("/register", [](const Request& req, Response& res) {
        string user_name = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        if (user_name.empty() || password.empty()) {
            res.set_content("用户名和密码不能为空哦~", "text/plain; charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain; charset=UTF-8");
            return;
        }
        //SQL代码生成
        string sql = "INSERT INTO user(user_name, password) VALUES('" + user_name + "','" + password + "')";

        int result = mysql_query(conn, sql.c_str());
        if (result != 0) {
            string err = mysql_error(conn);
            mysql_close(conn);
            res.set_content("注册失败: " + err, "text/plain; charset=UTF-8");
            return;
        }

        mysql_close(conn);
        res.set_content("注册成功~欢迎加入Muryo~", "text/plain; charset=UTF-8");
        });
    // 登录接口
    svr.Post("/login", [](const Request& req, Response& res) {
        string user_name = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain; charset=UTF-8");
            return;
        }

        //生成SQL语句
        string sql = "SELECT * FROM user WHERE user_name='" + user_name + "' AND password='" + password + "'";

        if (mysql_query(conn, sql.c_str())) {
            string err = "请求失败: ";
            err += mysql_error(conn);
            res.set_content(err, "text/plain; charset=UTF-8");
            mysql_close(conn);
            return;
        }

        // 获取查询结果
        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            res.set_content("查询失败", "text/plain; charset=UTF-8");
            mysql_close(conn);
            return;
        }

        // 判断有没有查到数据
        MYSQL_ROW row = mysql_fetch_row(result);

        if (row) {
            res.set_content("{\"success\":true,\"message\":\"登录成功！欢迎来到Muryo！\"}", "application/json");
        }
        else {
            res.set_content("{\"success\":false,\"message\":\"登录失败，咪请检查账号密码或是否注册~\"}", "application/json");
        }

        mysql_free_result(result);
        mysql_close(conn);
        });
    //发布制品接口
    svr.Post("/publish", [](const Request& req, Response& res) {

        string owner = req.get_param_value("owner");
        string item_name = req.get_param_value("item_name");
        string type = req.get_param_value("type");
        string intro = req.get_param_value("intro");
        string quantity = req.get_param_value("quantity");
        string role = req.get_param_value("role");
        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain;charset=UTF-8");
            return;
        }

        string sql = "INSERT INTO item(owner, item_name, type, intro, quantity,role) VALUES('"
            + owner + "','" + item_name + "','" + type + "','" + intro + "','" + quantity + "','"+role+"')";

        if (mysql_query(conn, sql.c_str())) {
            res.set_content("发布失败，请检查制品数量或用户状态", "text/plain;charset=UTF-8");
        }
        else {
            res.set_content("发布成功！快去和同好一起交流吧~", "text/plain;charset=UTF-8");
        }

        mysql_close(conn);
        });
    //申请交换
    svr.Post("/exchange/apply", [](const Request& req, Response& res) {
        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain;charset=UTF-8");
            return;
        }

        string ufrom = req.get_param_value("ufrom");
        string uto = req.get_param_value("uto");
        string item_idsstr = req.get_param_value("item_ids");      
        string quantitystr = req.get_param_value("quantities"); 
        
        
        if (ufrom.empty() || uto.empty() || item_idsstr.empty() || quantitystr.empty()) {
            res.set_content("参数不能为空", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        vector<string> item_ids = split(item_idsstr, ',');
        vector<string> quantity = split(quantitystr, ',');

  
        if (item_ids.size() != quantity.size()) {
            res.set_content("请检查制品和制品数量填写一致！", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        if (item_ids.empty()) {
            res.set_content("至少要申请一个制品哟~", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        mysql_query(conn, "START TRANSACTION");//事物

        
        string sql_exchange = "INSERT INTO exchange(ufrom, uto, status) VALUES('"
            + ufrom + "','" + uto + "','0')";

        if (mysql_query(conn, sql_exchange.c_str())) {
            cout << "请求失败: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK");
            res.set_content(string("申请失败: ") + mysql_error(conn), "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }
        int exchange_id = (int)mysql_insert_id(conn);

        for (int i = 0; i < item_ids.size(); i++) {
            string item_id = item_ids[i];
            string apply_quantity = quantity[i];

            if (item_id.empty() || apply_quantity.empty()) {
                mysql_query(conn, "ROLLBACK");
                res.set_content("制品编号和数量不能为空哦~", "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            int q = stoi(apply_quantity);
            if (q <= 0) {
                mysql_query(conn, "ROLLBACK");
                res.set_content("申请数量必须大于0哦~", "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
            string check_sql = "SELECT quantity, owner,status FROM item WHERE item_id = " + item_id;

            if (mysql_query(conn, check_sql.c_str())) {
                cout << "请求错误: " << mysql_error(conn) << endl;
                mysql_query(conn, "ROLLBACK");
                res.set_content(string("查询制品失败: ") + mysql_error(conn), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (result == NULL) {
                mysql_query(conn, "ROLLBACK");
                res.set_content("制品查询失败", "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            MYSQL_ROW row = mysql_fetch_row(result);
            if (row == NULL) {
                mysql_free_result(result);
                mysql_query(conn, "ROLLBACK");
                res.set_content(("请检查制品编号~，item_id = " + item_id).c_str(), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            int stock = stoi(row[0]);
            string owner = row[1];
            int status = stoi(row[2]);

            mysql_free_result(result);
            if (status!=0) {
                mysql_query(conn, "ROLLBACK");
                res.set_content(("item_id = " + item_id + " 这个制品不可以交换哟~").c_str(), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
            if (owner != ufrom) {
                mysql_query(conn, "ROLLBACK");
                res.set_content(("item_id = " + item_id + " 这个制品不属于咪地申请对象~").c_str(), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
            if (owner == uto) {
                mysql_query(conn, "ROLLBACK");
                res.set_content("咪不可以和自己交换哦~快去寻找同好叭！", "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            if (stock < q) {
                mysql_query(conn, "ROLLBACK");
                res.set_content(("咪来晚一步！item_id = " + item_id + "数量不足！").c_str(), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
            string count = "UPDATE item SET quantity="
                + to_string(stock-q) + " WHERE item_id=" + item_id ;

            if (mysql_query(conn, count.c_str())) {
                cout << "请求失败: " << mysql_error(conn) << endl;
                mysql_query(conn, "ROLLBACK");
                res.set_content(string("数量更新失败！ ") + mysql_error(conn), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            // 插入明细表
            string sql_detail = "INSERT INTO exdetail(exchange_id, item_id, quantity) VALUES("
                + to_string(exchange_id) + "," + item_id + "," + apply_quantity + ")";

            if (mysql_query(conn, sql_detail.c_str())) {
                cout << "请求失败: " << mysql_error(conn) << endl;
                mysql_query(conn, "ROLLBACK");
                res.set_content(string("插入明细失败: ") + mysql_error(conn), "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
        }

        mysql_query(conn, "COMMIT");
        res.set_content("恭喜咪，申请交换成功！", "text/plain;charset=UTF-8");
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
            res.set_content("请输入正确的用户ID哟~", "text/plain;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain;charset=UTF-8");
            return;
        }
        string sql = "SELECT e.detail_id,e.quantity,e.item_id,i.item_name,i.quantity AS 'left',a.status FROM exdetail e JOIN item i ON e.item_id = i.item_id JOIN exchange a ON e.exchange_id = a.exchange_id WHERE i.owner = " + ufromstr;

        if (mysql_query(conn, sql.c_str())) {
            cout << "请求失败: " << mysql_error(conn) << endl;
            res.set_content(string("查询失败: ") + mysql_error(conn), "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            cout << "获取结果失败: " << mysql_error(conn) << endl;
            res.set_content(string("查询失败: ") + mysql_error(conn), "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row;
        string response = "";
        while ((row = mysql_fetch_row(result)) != NULL) {
            response += "明细编号: ";
            response += row[0];
            response += "    交换状态:";
            response += row[5];
            response += "\n";
            response += "制品编号: ";
            response += row[2];
            response += "    制品名称";
            response += row[3]?row[3]:"NULL";
            response += "\n";
            response += "申请数量:";
            response += row[1];
            response += "    制品余量:";
            response += row[4];
            response += "\n";
            response += "~~~~~~~俺是分割线~~~~~~~\n";
        }
        if (response.empty()) {
            response = "还没有Meowryo和你交换~快去寻找同好叭！";
        }
        res.set_content(response, "text/plain;charset=UTF-8");
        mysql_free_result(result);
        mysql_close(conn);

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