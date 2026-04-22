#include <iostream>
#include <string>
#include "httplib.h"//HTTP服务器库
#include <mysql.h>//MySQl数据库接口
#include <json/json.h>


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
void set_cors(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}
// 解决 JSON 特殊字符问题
string escape_json(const string& input) {
    string output;
    output.reserve(input.size() + 10);

    for (char c : input) {
        switch (c) {
        case '\"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            // 控制字符过滤
            if (static_cast<unsigned char>(c) < 0x20) {
                // 跳过不可见控制字符
            }
            else {
                output += c;
            }
            break;
        }
    }

    return output;
}

int main() {
    system("chcp 65001");
    Server svr;
    //解决跨域问题。
    svr.Options(R"(.*)", [](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
        });
    
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
        set_cors(res);

        string user_name = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        if (user_name.empty() || password.empty()) {
            res.set_content(
                "{\"success\":false,\"message\":\"用户名和密码不能为空哦~\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"数据库连接失败\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        string sql = "INSERT INTO user(user_name, password) VALUES('" + user_name + "','" + password + "')";

        int result = mysql_query(conn, sql.c_str());
        if (result != 0) {
            string err = mysql_error(conn);
            mysql_close(conn);

            string json = "{\"success\":false,\"message\":\"注册失败: " + err + "\"}";
            res.set_content(json, "application/json; charset=UTF-8");
            return;
        }

        mysql_close(conn);

        res.set_content(
            "{\"success\":true,\"message\":\"注册成功~欢迎加入Muryo~\"}",
            "application/json; charset=UTF-8"
        );
        });
    // 登录接口
    svr.Post("/login", [](const Request& req, Response& res) {
        set_cors(res);

        string user_name = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        if (user_name.empty() || password.empty()) {
            res.set_content(
                "{\"success\":false,\"message\":\"用户名和密码不能为空\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"数据库连接失败\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        string sql = "SELECT user_id, user_name FROM user WHERE user_name='" + user_name + "' AND password='" + password + "'";

        if (mysql_query(conn, sql.c_str())) {
            res.set_content(
                "{\"success\":false,\"message\":\"请求失败\"}",
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"查询失败\"}",
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row) {
            string user_id = row[0] ? row[0] : "";
            string user_name_db = row[1] ? row[1] : "";

            string json = "{";
            json += "\"success\":true,";
            json += "\"message\":\"登录成功！欢迎来到Muryo！\",";
            json += "\"user_id\":" + user_id + ",";
            json += "\"user_name\":\"" + user_name_db + "\"";
            json += "}";

            res.set_content(json, "application/json; charset=UTF-8");
        }
        else {
            res.set_content(
                "{\"success\":false,\"message\":\"登录失败，咪请检查账号密码或是否注册~\"}",
                "application/json; charset=UTF-8"
            );
        }

        mysql_free_result(result);
        mysql_close(conn);
        });
    //发布制品接口
    svr.Post("/publish", [](const Request& req, Response& res) {
        set_cors(res);
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
        string ufromstr = req.get_param_value("ufrom");
        int exchange_id;

        try {
            exchange_id = stoi(exchange_idstr);
        }
        catch (...) {
            res.set_content("无效的交换编号！", "text/plain;charset=UTF-8");
            return;
        }

        if (action != "agree" && action != "reject") {
            res.set_content("无效的操作！", "text/plain;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain;charset=UTF-8");
            return;
        }

        // 开启事务
        if (mysql_query(conn, "START TRANSACTION")) {
            res.set_content("事务开启失败！", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        string sql_select = "SELECT ufrom, status FROM exchange WHERE exchange_id = " + to_string(exchange_id);

        if (mysql_query(conn, sql_select.c_str())) {
            mysql_query(conn, "ROLLBACK");
            res.set_content("请求失败！", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            mysql_query(conn, "ROLLBACK");
            res.set_content("结果查询失败！", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {
            mysql_free_result(result);
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);
            res.set_content("没有找到对应的交换记录！", "text/plain;charset=UTF-8");
            return;
        }

        int ufrom = stoi(row[0]);   
        int status = stoi(row[1]);

        mysql_free_result(result);
        if (ufrom != stoi(ufromstr)) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);
            res.set_content("咪只能操作自己收到的交换申请~", "text/plain;charset=UTF-8");
            return;
        }
        if (status == 1) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);
            res.set_content("咪已经拒绝过这条申请啦~", "text/plain;charset=UTF-8");
            return;
        }
        if (status == 2) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);
            res.set_content("咪已经同意申请，等待交换~", "text/plain;charset=UTF-8");
            return;
        }
        if (status == 3) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);
            res.set_content("此次交换已经完成啦~", "text/plain;charset=UTF-8");
            return;
        }
        if (status == 4) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);
            res.set_content("咪已经取消了这次交换~", "text/plain;charset=UTF-8");
            return;
        }

        int newstatus = (action == "agree") ? 2 : 1;
        string sql_update_exchange = "UPDATE exchange SET status = " + to_string(newstatus) +
            " WHERE exchange_id = " + to_string(exchange_id);

        if (mysql_query(conn, sql_update_exchange.c_str())) {
            mysql_query(conn, "ROLLBACK");
            res.set_content("数据更新失败！", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }
        if (action == "reject") {
            string sql_de = "SELECT item_id, quantity FROM exdetail WHERE exchange_id = " + exchange_idstr;

            if (mysql_query(conn, sql_de.c_str())) {
                mysql_query(conn, "ROLLBACK");
                res.set_content("明细查询失败！", "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
            MYSQL_RES* result1 = mysql_store_result(conn);
            if (result1 == NULL) {
                mysql_query(conn, "ROLLBACK");
                res.set_content("明细结果获取失败！", "text/plain;charset=UTF-8");
                mysql_close(conn);
                return;
            }
            MYSQL_ROW row1;
            while ((row1 = mysql_fetch_row(result1)) != NULL) {
                string item_id = row1[0];
                string count = row1[1];
                string sql_op = "UPDATE item SET quantity = quantity + " + count +
                    " WHERE item_id = " + item_id;

                if (mysql_query(conn, sql_op.c_str())) {
                    mysql_free_result(result1);
                    mysql_query(conn, "ROLLBACK");
                    res.set_content("数量更新失败！", "text/plain;charset=UTF-8");
                    mysql_close(conn);
                    return;
                }
            }
            mysql_free_result(result1);
        }
        if (mysql_query(conn, "COMMIT")) {
            mysql_query(conn, "ROLLBACK");
            res.set_content("事务提交失败！", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }
        mysql_close(conn);
        if (action == "agree") {
            res.set_content("咪已经同意交换申请，快去交换吧~", "text/plain;charset=UTF-8");
        }
        else {
            res.set_content("咪已经拒绝该申请~", "text/plain;charset=UTF-8");
        }
        });
        // 查看自己发出的申请
        svr.Get("/exchange/outgoing", [](const Request& req, Response& res) {
            set_cors(res);

            Json::Value response_json;

            string utostr = req.get_param_value("uto");
            int uto;

            try {
                uto = stoi(utostr);
            }
            catch (...) {
                response_json["success"] = false;
                response_json["message"] = "请输入正确的用户ID哟~";
                response_json["data"] = Json::Value(Json::arrayValue);

                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            MYSQL* conn = connect_db();
            if (conn == NULL) {
                response_json["success"] = false;
                response_json["message"] = "数据库连接失败";
                response_json["data"] = Json::Value(Json::arrayValue);

                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            string sql =
                "SELECT "
                "a.exchange_id, "
                "e.detail_id, "
                "e.item_id, "
                "i.item_name, "
                "e.quantity AS apply_quantity, "
                "i.quantity AS left_quantity, "
                "a.status "
                "FROM exdetail e "
                "JOIN item i ON e.item_id = i.item_id "
                "JOIN exchange a ON e.exchange_id = a.exchange_id "
                "WHERE a.uto = " + to_string(uto) + " "
                "ORDER BY "
                "CASE "
                "WHEN a.status = 0 THEN 0 "
                "WHEN a.status = 2 THEN 1 "
                "ELSE 2 "
                "END, "
                "CASE "
                "WHEN a.status IN (1,3,4) THEN e.detail_id "
                "ELSE 0 "
                "END DESC, "
                "a.exchange_id DESC";

            if (mysql_query(conn, sql.c_str())) {
                response_json["success"] = false;
                response_json["message"] = string("查询失败: ") + mysql_error(conn);
                response_json["data"] = Json::Value(Json::arrayValue);

                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (result == NULL) {
                response_json["success"] = false;
                response_json["message"] = string("获取结果失败: ") + mysql_error(conn);
                response_json["data"] = Json::Value(Json::arrayValue);

                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                mysql_close(conn);
                return;
            }

            MYSQL_ROW row;
            Json::Value data(Json::arrayValue);

            while ((row = mysql_fetch_row(result)) != NULL) {
                Json::Value item;

                item["exchange_id"] = row[0] ? atoi(row[0]) : 0;
                item["detail_id"] = row[1] ? atoi(row[1]) : 0;
                item["item_id"] = row[2] ? atoi(row[2]) : 0;
                item["item_name"] = row[3] ? row[3] : "";
                item["apply_quantity"] = row[4] ? atoi(row[4]) : 0;
                item["left_quantity"] = row[5] ? atoi(row[5]) : 0;
                item["status"] = row[6] ? atoi(row[6]) : -1;

                data.append(item);
            }

            response_json["success"] = true;
            response_json["message"] = data.size() == 0 ? "还没有提交过任何申请" : "查询成功";
            response_json["data"] = data;

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");

            mysql_free_result(result);
            mysql_close(conn);
            });
    
    // 查看所有制品列表
    svr.Get("/items", [](const Request& req, Response& res) {
        set_cors(res);

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"数据库连接失败\"}",
                "application/json;charset=UTF-8"
            );
            return;
        }

        // 联表查用户名，不再只返回 owner 的数字 ID
        string sql =
            "SELECT "
            "i.item_id, "
            "IFNULL(NULLIF(u.user_name, ''), '未知用户') AS owner_name, "
            "IFNULL(NULLIF(i.item_name, ''), '默认名称') AS item_name, "
            "IFNULL(NULLIF(i.role, ''), '默认角色') AS role, "
            "IFNULL(NULLIF(i.type, ''), '默认类型') AS type, "
            "i.quantity, "
            "IFNULL(NULLIF(i.image_url, ''), '') AS image_url, "
            "IFNULL(NULLIF(i.intro, ''), '暂无介绍') AS intro "
            "FROM item i "
            "LEFT JOIN `user` u ON i.owner = u.user_id "
            "WHERE i.status = 0 AND i.quantity > 0";

        if (mysql_query(conn, sql.c_str())) {
            string err = mysql_error(conn);
            err = escape_json(err);

            string json =
                "{\"success\":false,\"message\":\"请求失败\",\"error\":\"" + err + "\"}";
            res.set_content(json, "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            string err = mysql_error(conn);
            err = escape_json(err);

            string json =
                "{\"success\":false,\"message\":\"请求结果失败\",\"error\":\"" + err + "\"}";
            res.set_content(json, "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row;
        string json = "[";

        while ((row = mysql_fetch_row(result))) {
            string item_id = row[0] ? row[0] : "";
            string owner_name = row[1] ? row[1] : "";
            string item_name = row[2] ? row[2] : "";
            string role = row[3] ? row[3] : "";
            string type = row[4] ? row[4] : "";
            string quantity = row[5] ? row[5] : "0";
            string image_url = row[6] ? row[6] : "";
            string intro = row[7] ? row[7] : "";

            json += "{";
            json += "\"item_id\":\"" + escape_json(item_id) + "\",";
            json += "\"owner_name\":\"" + escape_json(owner_name) + "\",";
            json += "\"item_name\":\"" + escape_json(item_name) + "\",";
            json += "\"role\":\"" + escape_json(role) + "\",";
            json += "\"type\":\"" + escape_json(type) + "\",";
            json += "\"quantity\":\"" + escape_json(quantity) + "\",";
            json += "\"image_url\":\"" + escape_json(image_url) + "\",";
            json += "\"intro\":\"" + escape_json(intro) + "\"";
            json += "},";
        }

        if (json.back() == ',') {
            json.pop_back();
        }
        json += "]";

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(json, "application/json;charset=UTF-8");
        });
    // 查看我的制品
    svr.Get("/items/my", [](const Request& req, Response& res) {
        set_cors(res);

        if (!req.has_param("owner")) {
            res.set_content(
                "{\"success\":false,\"message\":\"缺少owner参数\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        string ownerstr = req.get_param_value("owner");
        int owner;

        try {
            owner = stoi(ownerstr);
        }
        catch (...) {
            res.set_content(
                "{\"success\":false,\"message\":\"请输入有效的用户ID\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"数据库连接失败\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        string sql = "SELECT item_id, owner, item_name, role, type, quantity, status, image_url, intro "
            "FROM item WHERE status <> 2 AND owner = " + to_string(owner);

        if (mysql_query(conn, sql.c_str())) {
            cout << "SQL执行失败: " << mysql_error(conn) << endl;
            res.set_content(
                "{\"success\":false,\"message\":\"请求失败\"}",
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            cout << "结果获取失败: " << mysql_error(conn) << endl;
            res.set_content(
                "{\"success\":false,\"message\":\"拉取结果失败\"}",
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        string json = "[";
        MYSQL_ROW row;
        bool first = true;

        while ((row = mysql_fetch_row(result))) {
            if (!first) json += ",";
            first = false;

            string item_name = row[2] ? row[2] : "";
            string role = row[3] ? row[3] : "";
            string type = row[4] ? row[4] : "";
            string image_url = row[7] ? row[7] : "";
            string intro = row[8] ? row[8] : "";

            json += "{";
            json += "\"item_id\":" + string(row[0] ? row[0] : "0") + ",";
            json += "\"owner\":" + string(row[1] ? row[1] : "0") + ",";
            json += "\"item_name\":\"" + item_name + "\",";
            json += "\"role\":\"" + role + "\",";
            json += "\"type\":\"" + type + "\",";
            json += "\"quantity\":" + string(row[5] ? row[5] : "0") + ",";
            json += "\"status\":" + string(row[6] ? row[6] : "0") + ",";
            json += "\"image_url\":\"" + image_url + "\",";
            json += "\"intro\":\"" + intro + "\"";
            json += "}";
        }

        json += "]";

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(json, "application/json; charset=UTF-8");
        });
    //查看制品详情
    svr.Get("/item/detail", [](const Request& req, Response& res) {
        set_cors(res);

        string item_id = req.get_param_value("item_id");
        if (item_id.empty()) {
            res.set_content(
                "{\"success\":false,\"message\":\"缺少item_id参数\"}",
                "application/json;charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"数据库连接失败\"}",
                "application/json;charset=UTF-8"
            );
            return;
        }

        string sql =
            "SELECT "
            "i.item_id, "
            "IFNULL(NULLIF(u.user_name, ''), '未知用户') AS owner_name, "
            "IFNULL(NULLIF(i.item_name, ''), '默认名称') AS item_name, "
            "IFNULL(NULLIF(i.role, ''), '默认角色') AS role, "
            "IFNULL(NULLIF(i.type, ''), '默认类型') AS type, "
            "i.quantity, "
            "IFNULL(NULLIF(i.image_url, ''), '') AS item_img, "
            "IFNULL(NULLIF(i.intro, ''), '暂无介绍') AS intro "
            "FROM item i "
            "LEFT JOIN `user` u ON i.owner = u.user_id "
            "WHERE i.item_id = " + item_id + " LIMIT 1";

        if (mysql_query(conn, sql.c_str())) {
            string err = mysql_error(conn);
            string json = "{\"success\":false,\"message\":\"查询失败\",\"error\":\"" + err + "\"}";
            res.set_content(json, "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            string err = mysql_error(conn);
            string json = "{\"success\":false,\"message\":\"获取结果失败\",\"error\":\"" + err + "\"}";
            res.set_content(json, "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"没有找到该制品\"}",
                "application/json;charset=UTF-8"
            );
            mysql_free_result(result);
            mysql_close(conn);
            return;
        }

        string json = "{";
        json += "\"success\":true,";
        json += "\"data\":{";
        json += "\"item_id\":\"" + string(row[0] ? row[0] : "") + "\",";
        json += "\"owner_name\":\"" + string(row[1] ? row[1] : "") + "\",";
        json += "\"item_name\":\"" + string(row[2] ? row[2] : "") + "\",";
        json += "\"role\":\"" + string(row[3] ? row[3] : "") + "\",";
        json += "\"type\":\"" + string(row[4] ? row[4] : "") + "\",";
        json += "\"quantity\":\"" + string(row[5] ? row[5] : "") + "\",";
        json += "\"item_img\":\"" + string(row[6] ? row[6] : "") + "\",";
        json += "\"intro\":\"" + string(row[7] ? row[7] : "") + "\"";
        json += "}}";

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(json, "application/json;charset=UTF-8");
        });
    // 删除制品
    svr.Post("/items/delete", [](const Request& req, Response& res) {

        string item_idstr = req.get_param_value("item_id");
        string ownerstr = req.get_param_value("owner");

        int item_id, owner;

        try {
            item_id = stoi(item_idstr);
        }
        catch (...) {
            res.set_content("制品编号无效", "text/plain;charset=UTF-8");
            return;
        }

        try {
            owner = stoi(ownerstr);
        }
        catch (...) {
            res.set_content("用户编号无效", "text/plain;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content("数据库连接失败", "text/plain;charset=UTF-8");
            return;
        }

        string sql_select = "SELECT owner, status FROM item WHERE item_id = " + to_string(item_id);

        if (mysql_query(conn, sql_select.c_str())) {
            res.set_content("查询制品失败", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content("查询结果失败", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);
            res.set_content("没有找到该制品", "text/plain;charset=UTF-8");
            return;
        }

        int real_owner = stoi(row[0]);
        int status = stoi(row[1]);

        mysql_free_result(result);

        if (real_owner != owner) {
            mysql_close(conn);
            res.set_content("咪没有权限删除别人的制品~", "text/plain;charset=UTF-8");
            return;
        }

        if (status == 2) {
            mysql_close(conn);
            res.set_content("该制品已经删除过了~", "text/plain;charset=UTF-8");
            return;
        }

        string sql_update = "UPDATE item SET status = 2 WHERE item_id = " + to_string(item_id);

        if (mysql_query(conn, sql_update.c_str())) {
            res.set_content("制品删除失败", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        if (mysql_affected_rows(conn) == 0) {
            res.set_content("没有删除到任何制品", "text/plain;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        mysql_close(conn);
        res.set_content("制品删除成功，期待咪的下一次产粮……", "text/plain;charset=UTF-8");
        });
    // 查看个人信息
    svr.Get("/user/profile", [](const Request& req, Response& res) {
        set_cors(res);

        string user_id_str = req.get_param_value("user_id");
        int user_id;

        try {
            user_id = stoi(user_id_str);
        }
        catch (...) {
            res.set_content(
                "{\"success\":false,\"message\":\"user_id无效\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"数据库连接失败\"}",
                "application/json; charset=UTF-8"
            );
            return;
        }

        string sql = "SELECT user_id, user_name, contact, introduction FROM user WHERE user_id = " + to_string(user_id);

        if (mysql_query(conn, sql.c_str())) {
            res.set_content(
                "{\"success\":false,\"message\":\"查询失败\"}",
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            res.set_content(
                "{\"success\":false,\"message\":\"获取结果失败\"}",
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row) {
            string json = "{";
            json += "\"success\":true,";
            json += "\"user_id\":" + string(row[0] ? row[0] : "0") + ",";
            json += "\"user_name\":\"" + string(row[1] ? row[1] : "") + "\",";
            json += "\"contact\":\"" + string(row[2] ? row[2] : "") + "\",";
            json += "\"introduction\":\"" + string(row[3] ? row[3] : "") + "\"";
            json += "}";

            res.set_content(json, "application/json; charset=UTF-8");
        }
        else {
            res.set_content(
                "{\"success\":false,\"message\":\"未找到该用户\"}",
                "application/json; charset=UTF-8"
            );
        }

        mysql_free_result(result);
        mysql_close(conn);
        });
    cout << "Server running at http://127.0.0.1:8080;" << endl;
    svr.listen("127.0.0.1", 8080);

    return 0;
}