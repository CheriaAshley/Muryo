#include <iostream>
#include <string>
#include "httplib.h"//HTTP服务器库
#include <mysql.h>//MySQl数据库接口
#include <json/json.h>
#include <windows.h>
#include <sstream>//字符串流，用于分割字符串（暂留）

using namespace httplib;//必须使用和std不同
using namespace std;
using namespace Json;

//连接数据库并返回连接对象
MYSQL* connect_db() {
    MYSQL* conn = mysql_init(NULL);//创建初始化连接结构返回地址
    if (conn == NULL) {
        cout << "mysql_init failed" << endl;//这一步和密码等权限问题无关
        return NULL;
    }
    //连接数据库
    conn = mysql_real_connect(
        conn,
        "127.0.0.1",      // host
        "muryo_user",     // user，改为你的数据库用户名
        "muryo123",        // password，改为你的数据库密码
        "muryo",          // database
        3306,             // port
        NULL,
        0
    );

    if (conn == NULL) {
        cout << "mysql_real_connect failed: " << mysql_error(conn) << endl;
        return NULL;
    }

    // 设置字符集，防止中文乱码（为了和数据库保持一致所以改为utf8mb4）
    mysql_set_character_set(conn, "utf8mb4");

    return conn;
}

//前期纯后端时的小函数，用于一次提交多个交换申请分割字符串（暂留）
vector<string> split(const string& s, char delimiter) {
    vector<string> result;
    string temp;
    stringstream ss(s);

    while (getline(ss, temp, delimiter)) {
        result.push_back(temp);
    }

    return result;
}
//解决跨域
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

//查询管理员等级
int get_admin_level(MYSQL* conn, int user_id) {
    string sql = "SELECT level FROM admin WHERE user_id = " + to_string(user_id);

    if (mysql_query(conn, sql.c_str())) {
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == NULL) {
        mysql_free_result(result);
        return 0;
    }

    int level = atoi(row[0]);

    mysql_free_result(result);
    return level;
}

int main() {
    //中文乱码问题
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    Server svr;

    //解决跨域
    svr.Options(R"(.*)", [](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    //测试后端运行
    svr.Get("/test", [](const Request&, Response& res) {

        set_cors(res);

        Value response_json;

        response_json["success"] = true;
        response_json["message"] = "Muryo后端正在运行";
        response_json["data"]["server"] = "Muryo";
        response_json["data"]["status"] = "running";

        res.set_content(
            response_json.toStyledString(),
            "application/json;charset=UTF-8"
        );
    });

    //注册
    svr.Post("/register", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string user_name = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        if (user_name.empty() || password.empty()) {
            response_json["success"] = false;
            response_json["message"] = "用户名和密码不能为空哦~";

            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        /*
        以下是AI的方案，虽然有防止SQL注入的函数，但还是加了一层转义，毕竟安全第一
        *其实那个防注入是初期防止手拼JSON出错的
        现在懒得删了万一有用呢（）
        */
        char user_name_escape[512];
        char password_escape[512];

        mysql_real_escape_string(
            conn,
            user_name_escape,
            user_name.c_str(),
            user_name.length()
        );

        mysql_real_escape_string(
            conn,
            password_escape,
            password.c_str(),
            password.length()
        );

        string sql = "INSERT INTO user(user_name, password) VALUES('" ;
        sql+= user_name_escape;
        sql+= "','"; 
        sql+= password_escape;
        sql+= "')";

        int result = mysql_query(conn, sql.c_str());

        if (result != 0) {
            string err = mysql_error(conn);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "注册失败: " + err;

            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "注册成功~欢迎加入Muryo~";

        res.set_content(
            response_json.toStyledString(),
            "application/json; charset=UTF-8"
        );
    });
    
    //登录
    svr.Post("/login", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string user_name = req.get_param_value("user_name");
        string password = req.get_param_value("password");

        if (user_name.empty() || password.empty()) {
            response_json["success"] = false;
            response_json["message"] = "用户名和密码不能为空哦~";

            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        char user_name_escape[512];
        char password_escape[512];

        mysql_real_escape_string(
            conn,
            user_name_escape,
            user_name.c_str(),
            user_name.length()
        );

        mysql_real_escape_string(
            conn,
            password_escape,
            password.c_str(),
            password.length()
        );

        string sql = "SELECT user_id, user_name FROM user WHERE user_name='" ;
        sql+= user_name_escape;
        sql+= "' AND password='";
        sql+= password_escape;
        sql+= "'";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "请求失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );

            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = "查询失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row) {
            response_json["success"] = true;
            response_json["message"] = "登录成功！欢迎来到Muryo！";
            response_json["user_id"] = row[0] ? atoi(row[0]) : 0;
            response_json["user_name"] = row[1] ? row[1] : "";
        }
        else {
            response_json["success"] = false;
            response_json["message"] = "登录失败，咪请检查账号密码或是否注册~";
        }

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(
            response_json.toStyledString(),
            "application/json; charset=UTF-8"
        );
    });

    //发布制品
    svr.Post("/publish", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string owner = req.get_param_value("owner");
        string item_name = req.get_param_value("item_name");
        string role = req.get_param_value("role");
        string type = req.get_param_value("type");
        string quantity = req.get_param_value("quantity");
        string img_url = req.get_param_value("img_url");
        string intro = req.get_param_value("intro");
        
        if (owner.empty()) {
            response_json["success"] = false;
            response_json["message"] = "请先登录后再发布制品";

            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (item_name.empty() || role.empty() || type.empty()) {
            response_json["success"] = false;
            response_json["message"] = "咪，制品名称、角色和类型不能为空~";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (quantity.empty()) {
            quantity = "0";
        }

        int quantity_num;

        try {
            quantity_num = stoi(quantity);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "咪，数量只能是数字哦~";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (quantity_num <= 0) {
            response_json["success"] = false;
            response_json["message"] = "咪，数量必须大于0哦~";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (intro.empty()) {
            intro = "暂无介绍";
        }

        if (img_url.empty()) {
            img_url = "upload/default_item.png";
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        char owner_escape[64];
        char item_name_escape[512];
        char role_escape[512];
        char type_escape[512];
        char intro_escape[1024];
        char img_url_escape[1024];

        mysql_real_escape_string(conn, owner_escape, owner.c_str(), owner.length());
        mysql_real_escape_string(conn, item_name_escape, item_name.c_str(), item_name.length());
        mysql_real_escape_string(conn, role_escape, role.c_str(), role.length());
        mysql_real_escape_string(conn, type_escape, type.c_str(), type.length());
        mysql_real_escape_string(conn, intro_escape, intro.c_str(), intro.length());
        mysql_real_escape_string(conn, img_url_escape, img_url.c_str(), img_url.length());

        string sql = "INSERT INTO item(owner, item_name, role, type, quantity, img_url, intro) VALUES(";
        sql += owner_escape;
        sql += ",'";
        sql += item_name_escape;
        sql += "','";
        sql += role_escape;
        sql += "','";
        sql += type_escape;
        sql += "',";
        sql += to_string(quantity_num);
        sql += ",'";
        sql += img_url_escape;
        sql += "','";
        sql += intro_escape;
        sql += "')";

        if (mysql_query(conn, sql.c_str())) {
            string err = mysql_error(conn);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "发布失败，请检查发布信息或用户状态";
            response_json["error"] = err;

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }
       
        response_json["success"] = true;
        response_json["message"] = "发布成功！快去和同好交换吧~";

        res.set_content(
            response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );

        mysql_close(conn);
    });

    // 申请交换
    svr.Post("/exchange/apply", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string ufrom_str = req.get_param_value("ufrom");
        string uto_str = req.get_param_value("uto");
        string item_ids_str = req.get_param_value("item_ids");
        string quantities_str = req.get_param_value("quantities");

        if (ufrom_str.empty() || uto_str.empty() || item_ids_str.empty() || quantities_str.empty()) {
            response_json["success"] = false;
            response_json["message"] = "请检查申请人、被申请人、制品编号和数量是否填写";
        
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        int ufrom;
        int uto;

        try {
            ufrom = stoi(ufrom_str);
            uto = stoi(uto_str);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "用户编号格式错误";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (ufrom == uto) {
            response_json["success"] = false;
            response_json["message"] = "咪不可以和自己交换哦~快去寻找同好叭！";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }


        vector<string> item_ids = split(item_ids_str, ',');
        vector<string> quantities = split(quantities_str, ',');

        if (item_ids.size() != quantities.size()) {
            response_json["success"] = false;
            response_json["message"] = "请检查制品和制品数量填写是否一致！";

            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (item_ids.empty()) {
            response_json["success"] = false;
            response_json["message"] = "至少要申请一个制品哟~";
           
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        vector<int> item_id_list;
        vector<int> quantity_list;

        try {
            item_id = stoi(item_ids[i]);
            quantity = stoi(quantities[i]);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "制品编号和申请数量必须是数字";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (item_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "制品编号不合法";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (quantity <= 0) {
            response_json["success"] = false;
            response_json["message"] = "申请数量必须大于0！";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        for (int j = 0; j < item_id_list.size(); j++) {
            if (item_id_list[j] == item_id) {
                response_json["success"] = false;
                response_json["message"] = "同一个制品不要重复选择哦";
                
                res.set_content(
                    response_json.toStyledString(),
                    "application/json;charset=UTF-8"
                );
                return;
            }
        }

        item_id_list.push_back(item_id);
        quantity_list.push_back(quantity);

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (mysql_query(conn, "START TRANSACTION")) {
            response_json["success"] = false;
            response_json["message"] = "事务开启失败！";
            mysql_close(conn);
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        for (int i = 0; i < item_id_list.size(); i++) {
            string check_sql = "SELECT owner, status FROM item WHERE item_id = "
                + to_string(item_id_list[i]);

            if (mysql_query(conn, check_sql.c_str())) {
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = string("查询制品失败: ") + mysql_error(conn);
                mysql_close(conn);
                
                res.set_content(
                    response_json.toStyledString(), 
                    "application/json;charset=UTF-8"
                );
                return;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (result == NULL) {
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = "制品查询失败";
                mysql_close(conn);
                
                res.set_content(
                    response_json.toStyledString(),
                    "application/json;charset=UTF-8"
                );
                return;
            }

            MYSQL_ROW row = mysql_fetch_row(result);

            if (row == NULL) {
                mysql_free_result(result);
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = "没有找到制品，item_id = " + to_string(item_id_list[i]);
                mysql_close(conn);
                
                res.set_content(
                    response_json.toStyledString(),
                    "application/json;charset=UTF-8"
                );
                return;
            }

            int owner = row[0] ? atoi(row[0]) : 0;
            int status = row[1] ? atoi(row[1]) : -1;

            mysql_free_result(result);

            if (owner != ufrom) {
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = "item_id = " + to_string(item_id_list[i]) + " 这个制品不属于申请对象";
                mysql_close(conn);
                
                res.set_content(
                    response_json.toStyledString(),
                    "application/json;charset=UTF-8"
                );
                return;
            }

            if (status != 0) {
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = "item_id = " + to_string(item_id_list[i]) + " 这个制品当前不可交换";
                mysql_close(conn);
                
                res.set_content(response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
                return;
            }
        }

        string sql_exchange = "INSERT INTO `exchange`(ufrom, uto, status) VALUES(";
        sql_exchange += to_string(ufrom);
        sql_exchange += ",";
        sql_exchange += to_string(uto) ;
        sql_exchange += ",0)";

        if (mysql_query(conn, sql_exchange.c_str())) {
            mysql_query(conn, "ROLLBACK");
            response_json["success"] = false;
            response_json["message"] = string("创建交换申请失败: ") + mysql_error(conn);
            mysql_close(conn);
            
            res.set_content(response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );
            return;
        }

        int exchange_id = (int)mysql_insert_id(conn);

        for (int i = 0; i < item_id_list.size(); i++) {
            string sql_detail = "INSERT INTO exdetail(exchange_id, item_id, quantity) VALUES(";
            sql_detail += to_string(exchange_id) + ",";
            sql_detail += to_string(item_id_list[i]) + ",";
            sql_detail += to_string(quantity_list[i]) + ")";

            if (mysql_query(conn, sql_detail.c_str())) {
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = string("申请失败: ") + mysql_error(conn);
                mysql_close(conn);
                
                res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
                );
                return;
            }
        }

        if (mysql_query(conn, "COMMIT")) {
            mysql_query(conn, "ROLLBACK");
            response_json["success"] = false;
            response_json["message"] = "事务提交失败";
            mysql_close(conn);
            
            res.set_content(
            response_json.toStyledString(),
            "application/json;charset=UTF-8"
            );
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "恭喜咪，申请交换成功！";
        response_json["exchange_id"] = exchange_id;
        
        res.set_content(
            response_json.toStyledString(),
            "application/json;charset=UTF-8"
        );
    });
    
    // 查看收到的申请（我的交换）
    svr.Get("/exchange/incoming", [](const Request& req, Response& res) {
            set_cors(res);

            Json::Value response_json;
            response_json["data"] = Json::Value(Json::arrayValue);

            string ufromstr = req.get_param_value("ufrom");
            int ufrom;

            try {
                ufrom = stoi(ufromstr);
            }
            catch (...) {
                response_json["success"] = false;
                response_json["message"] = "请输入正确的用户ID哟~";
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            MYSQL* conn = connect_db();
            if (conn == NULL) {
                response_json["success"] = false;
                response_json["message"] = "数据库连接失败";
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            string sql =
                "SELECT "
                "e.detail_id, "
                "e.exchange_id, "
                "e.item_id, "
                "i.item_name, "
                "e.quantity AS apply_quantity, "
                "i.quantity AS left_quantity, "
                "a.status, "
                "a.uto AS applicant_id, "
                "u.user_name AS applicant_name "
                "FROM exdetail e "
                "JOIN item i ON e.item_id = i.item_id "
                "JOIN exchange a ON e.exchange_id = a.exchange_id "
                "LEFT JOIN user u ON a.uto = u.user_id "
                "WHERE i.owner = " + to_string(ufrom) + " "
                "ORDER BY "
                "CASE "
                "WHEN a.status = 0 THEN 0 "
                "WHEN a.status = 2 THEN 1 "
                "ELSE 2 "
                "END ASC, "
                "CASE "
                "WHEN a.status IN (1, 3, 4) THEN e.detail_id "
                "ELSE NULL "
                "END DESC, "
                "e.detail_id ASC";

            if (mysql_query(conn, sql.c_str())) {
                cout << "查询失败: " << mysql_error(conn) << endl;
                response_json["success"] = false;
                response_json["message"] = string("查询失败: ") + mysql_error(conn);
                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            MYSQL_RES* result = mysql_store_result(conn);
            if (result == NULL) {
                cout << "获取结果失败: " << mysql_error(conn) << endl;
                response_json["success"] = false;
                response_json["message"] = string("获取结果失败: ") + mysql_error(conn);
                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)) != NULL) {
                Json::Value item;

                item["detail_id"] = row[0] ? stoi(row[0]) : 0;
                item["exchange_id"] = row[1] ? stoi(row[1]) : 0;
                item["item_id"] = row[2] ? stoi(row[2]) : 0;
                item["item_name"] = row[3] ? row[3] : "暂无名称";
                item["apply_quantity"] = row[4] ? stoi(row[4]) : 0;
                item["left_quantity"] = row[5] ? stoi(row[5]) : 0;
                item["status"] = row[6] ? stoi(row[6]) : -1;
                item["applicant_id"] = row[7] ? stoi(row[7]) : 0;
                item["applicant_name"] = row[8] ? row[8] : "未知用户";

                response_json["data"].append(item);
            }

            if (response_json["data"].size() == 0) {
                response_json["success"] = true;
                response_json["message"] = "还没有人向你发起交换申请哦~";
            }
            else {
                response_json["success"] = true;
                response_json["message"] = "获取我的交换成功";
            }

            mysql_free_result(result);
            mysql_close(conn);

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });
    
    // 处理申请：使用存储过程完成更新操作
    svr.Post("/exchange/handle", [](const Request& req, Response& res) {
                set_cors(res);

                Json::Value response_json;

                string exchange_idstr = req.get_param_value("exchange_id");
                string action = req.get_param_value("action");
                string ufromstr = req.get_param_value("ufrom");

                int exchange_id;
                int request_ufrom;

                try {
                    exchange_id = stoi(exchange_idstr);
                    request_ufrom = stoi(ufromstr);
                }
                catch (...) {
                    response_json["success"] = false;
                    response_json["message"] = "无效的参数！";
                    res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                    return;
                }

                if (action != "agree" && action != "reject" && action != "complete") {
                    response_json["success"] = false;
                    response_json["message"] = "无效的操作！";
                    res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                    return;
                }

                MYSQL* conn = connect_db();
                if (conn == NULL) {
                    response_json["success"] = false;
                    response_json["message"] = "数据库连接失败";
                    res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                    return;
                }

                string sql = "CALL proc_handle_exchange("
                    + to_string(exchange_id) + ", '"
                    + action + "', "
                    + to_string(request_ufrom) + ")";

                if (mysql_query(conn, sql.c_str())) {
                    response_json["success"] = false;
                    response_json["message"] = "存储过程执行失败！";
                    mysql_close(conn);
                    res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                    return;
                }

                MYSQL_RES* result = mysql_store_result(conn);
                if (result == NULL) {
                    response_json["success"] = false;
                    response_json["message"] = "存储过程没有返回结果！";
                    mysql_close(conn);
                    res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                    return;
                }

                MYSQL_ROW row = mysql_fetch_row(result);
                if (row == NULL) {
                    mysql_free_result(result);
                    response_json["success"] = false;
                    response_json["message"] = "读取存储过程结果失败！";
                    mysql_close(conn);
                    res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                    return;
                }

                int success = atoi(row[0]);
                string message = row[1] ? row[1] : "操作完成";

                mysql_free_result(result);

                response_json["success"] = (success == 1);
                response_json["message"] = message;

                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
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
        set_cors(res);
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
    // 查看待办申请
    svr.Get("/exchange/todo", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;
        response_json["data"] = Json::Value(Json::arrayValue);

        string ufromstr = req.get_param_value("ufrom");
        int ufrom;

        try {
            ufrom = stoi(ufromstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "请输入正确的用户ID哟~";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "SELECT "
            "e.detail_id, "
            "e.exchange_id, "
            "e.item_id, "
            "i.item_name, "
            "e.quantity AS apply_quantity, "
            "i.quantity AS left_quantity, "
            "a.status, "
            "a.uto AS applicant_id, "
            "u.user_name AS applicant_name "
            "FROM exdetail e "
            "JOIN item i ON e.item_id = i.item_id "
            "JOIN exchange a ON e.exchange_id = a.exchange_id "
            "LEFT JOIN user u ON a.uto = u.user_id "
            "WHERE i.owner = " + to_string(ufrom) + " "
            "AND (a.status = 0 OR a.status = 2) "
            "ORDER BY "
            "CASE WHEN a.status = 0 THEN 0 ELSE 1 END ASC, "
            "e.detail_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            cout << "查询待办申请失败: " << mysql_error(conn) << endl;
            response_json["success"] = false;
            response_json["message"] = string("查询失败: ") + mysql_error(conn);
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            cout << "获取待办申请结果失败: " << mysql_error(conn) << endl;
            response_json["success"] = false;
            response_json["message"] = string("获取结果失败: ") + mysql_error(conn);
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)) != NULL) {
            Json::Value item;

            item["detail_id"] = row[0] ? stoi(row[0]) : 0;
            item["exchange_id"] = row[1] ? stoi(row[1]) : 0;
            item["item_id"] = row[2] ? stoi(row[2]) : 0;
            item["item_name"] = row[3] ? row[3] : "暂无名称";
            item["apply_quantity"] = row[4] ? stoi(row[4]) : 0;
            item["left_quantity"] = row[5] ? stoi(row[5]) : 0;
            item["status"] = row[6] ? stoi(row[6]) : -1;
            item["applicant_id"] = row[7] ? stoi(row[7]) : 0;
            item["applicant_name"] = row[8] ? row[8] : "未知用户";

            response_json["data"].append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        if (response_json["data"].size() == 0) {
            response_json["message"] = "暂时没有待办申请哦~";
        }
        else {
            response_json["message"] = "获取待办申请成功";
        }

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    // 含有视图的查询操作：查看我收到的交换申请详情
    svr.Get("/exchange/view/incoming", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        string ufromstr = req.get_param_value("ufrom");
        int ufrom;

        try {
            ufrom = stoi(ufromstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "用户编号无效！";
            response_json["data"] = Json::Value(Json::arrayValue);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败！";
            response_json["data"] = Json::Value(Json::arrayValue);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "SELECT exchange_id, uto, apply_user_name, ufrom, target_user_name, "
            "detail_id, item_id, item_name, quantity, status, status_text "
            "FROM view_exchange_detail "
            "WHERE ufrom = " + to_string(ufrom) + " "
            "ORDER BY "
            "CASE "
            "WHEN status = 0 THEN 0 "
            "WHEN status = 2 THEN 1 "
            "ELSE 2 "
            "END, detail_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "视图查询失败！";
            response_json["data"] = Json::Value(Json::arrayValue);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = "查询结果获取失败！";
            response_json["data"] = Json::Value(Json::arrayValue);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        Json::Value data(Json::arrayValue);
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            Json::Value item;

            item["exchange_id"] = row[0] ? row[0] : "";
            item["uto"] = row[1] ? row[1] : "";
            item["apply_user_name"] = row[2] ? row[2] : "";
            item["ufrom"] = row[3] ? row[3] : "";
            item["target_user_name"] = row[4] ? row[4] : "";
            item["detail_id"] = row[5] ? row[5] : "";
            item["item_id"] = row[6] ? row[6] : "";
            item["item_name"] = row[7] ? row[7] : "";
            item["quantity"] = row[8] ? row[8] : "";
            item["status"] = row[9] ? row[9] : "";
            item["status_text"] = row[10] ? row[10] : "";

            data.append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "视图查询成功！";
        response_json["data"] = data;

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });

    /*下面是管理员相关部分*/
    //删除制品
    svr.Post("/admin/apply/delete_item", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string item_idstr = req.get_param_value("item_id");
        string reason = req.get_param_value("reason");

        int admin_id;
        int item_id;

        try {
            admin_id = stoi(admin_idstr);
            item_id = stoi(item_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int level = get_admin_level(conn, admin_id);

        if (level < 1) {
            response_json["success"] = false;
            response_json["message"] = "你不是管理员，不能提交删除申请";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (level == 3) {
            string sql = "UPDATE item SET status = 2 WHERE item_id = " + to_string(item_id);

            if (mysql_query(conn, sql.c_str())) {
                response_json["success"] = false;
                response_json["message"] = "三级管理员直接删除失败";
            }
            else {
                response_json["success"] = true;
                response_json["message"] = "三级管理员已直接删除制品";
            }

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "INSERT INTO admin_apply(admin_id, apply_type, target_id, reason, status) VALUES("
            + to_string(admin_id) + ", 'delete_item', "
            + to_string(item_id) + ", '"
            + reason + "', 0)";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "提交删除申请失败";
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "删除申请已提交，等待高级管理员审核";
        }

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    //封禁用户
    svr.Post("/admin/apply/ban_user", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string user_idstr = req.get_param_value("user_id");
        string reason = req.get_param_value("reason");

        int admin_id;
        int user_id;

        try {
            admin_id = stoi(admin_idstr);
            user_id = stoi(user_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int level = get_admin_level(conn, admin_id);

        if (level < 2) {
            response_json["success"] = false;
            response_json["message"] = "只有二级及以上管理员可以提交封禁申请";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (level == 3) {
            string sql = "UPDATE user SET ban_status = 1 WHERE user_id = " + to_string(user_id);

            if (mysql_query(conn, sql.c_str())) {
                response_json["success"] = false;
                response_json["message"] = "三级管理员直接封禁失败";
            }
            else {
                response_json["success"] = true;
                response_json["message"] = "三级管理员已直接封禁用户";
            }

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "INSERT INTO admin_apply(admin_id, apply_type, target_id, reason, status) VALUES("
            + to_string(admin_id) + ", 'ban_user', "
            + to_string(user_id) + ", '"
            + reason + "', 0)";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "提交封禁申请失败";
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "封禁申请已提交，等待三级管理员审核";
        }

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    //查看待审核申请
    svr.Get("/admin/apply/list", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;
        Json::Value data(Json::arrayValue);

        string admin_idstr = req.get_param_value("admin_id");
        int admin_id;

        try {
            admin_id = stoi(admin_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";
            response_json["data"] = data;
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();

        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            response_json["data"] = data;
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int level = get_admin_level(conn, admin_id);

        string sql;

        if (level == 2) {
            sql =
                "SELECT apply_id, admin_id, apply_type, target_id, reason, status, create_time "
                "FROM admin_apply "
                "WHERE status = 0 AND apply_type = 'delete_item' "
                "ORDER BY apply_id DESC";
        }
        else if (level == 3) {
            sql =
                "SELECT apply_id, admin_id, apply_type, target_id, reason, status, create_time "
                "FROM admin_apply "
                "WHERE status = 0 "
                "ORDER BY apply_id DESC";
        }
        else {
            response_json["success"] = false;
            response_json["message"] = "权限不足";
            response_json["data"] = data;
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "查询失败";
            response_json["data"] = data;
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result))) {
            Json::Value item;
            item["apply_id"] = atoi(row[0]);
            item["admin_id"] = atoi(row[1]);
            item["apply_type"] = row[2];
            item["target_id"] = atoi(row[3]);
            item["reason"] = row[4] ? row[4] : "";
            item["status"] = atoi(row[5]);
            item["create_time"] = row[6] ? row[6] : "";
            data.append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "查询成功";
        response_json["data"] = data;

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    //处理审核申请
    svr.Post("/admin/apply/handle", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string apply_idstr = req.get_param_value("apply_id");
        string action = req.get_param_value("action");

        int admin_id;
        int apply_id;

        try {
            admin_id = stoi(admin_idstr);
            apply_id = stoi(apply_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (action != "agree" && action != "reject") {
            response_json["success"] = false;
            response_json["message"] = "操作类型错误";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();

        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int level = get_admin_level(conn, admin_id);

        string sql =
            "SELECT apply_type, target_id, status FROM admin_apply "
            "WHERE apply_id = " + to_string(apply_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "查询申请失败";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            response_json["success"] = false;
            response_json["message"] = "申请不存在";
            mysql_free_result(result);
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string apply_type = row[0];
        int target_id = atoi(row[1]);
        int status = atoi(row[2]);

        mysql_free_result(result);

        if (status != 0) {
            response_json["success"] = false;
            response_json["message"] = "该申请已经被处理";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (apply_type == "delete_item" && level < 2) {
            response_json["success"] = false;
            response_json["message"] = "只有二级及以上管理员可以审核删除申请";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (apply_type == "ban_user" && level < 3) {
            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以审核封禁申请";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        mysql_query(conn, "START TRANSACTION");

        if (action == "agree") {
            if (apply_type == "delete_item") {
                sql = "UPDATE item SET status = 2 WHERE item_id = " + to_string(target_id);
            }
            else if (apply_type == "ban_user") {
                sql = "UPDATE user SET ban_status = 1 WHERE user_id = " + to_string(target_id);
            }

            if (mysql_query(conn, sql.c_str())) {
                mysql_query(conn, "ROLLBACK");
                response_json["success"] = false;
                response_json["message"] = "执行申请操作失败";
                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            sql =
                "UPDATE admin_apply SET status = 1, handle_admin_id = "
                + to_string(admin_id)
                + ", handle_time = NOW() WHERE apply_id = "
                + to_string(apply_id);
        }
        else {
            sql =
                "UPDATE admin_apply SET status = 2, handle_admin_id = "
                + to_string(admin_id)
                + ", handle_time = NOW() WHERE apply_id = "
                + to_string(apply_id);
        }

        if (mysql_query(conn, sql.c_str())) {
            mysql_query(conn, "ROLLBACK");
            response_json["success"] = false;
            response_json["message"] = "更新申请状态失败";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        mysql_query(conn, "COMMIT");

        response_json["success"] = true;
        response_json["message"] = "申请处理成功";

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    //恢复封禁用户
    svr.Post("/admin/user/recover", [](const Request& req, Response& res) {
            set_cors(res);

            Json::Value response_json;

            int admin_id = stoi(req.get_param_value("admin_id"));
            int user_id = stoi(req.get_param_value("user_id"));

            MYSQL* conn = connect_db();

            if (get_admin_level(conn, admin_id) < 3) {
                response_json["success"] = false;
                response_json["message"] = "只有三级管理员可以恢复用户";
                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            string sql = "UPDATE user SET ban_status = 0 WHERE user_id = " + to_string(user_id);

            if (mysql_query(conn, sql.c_str())) {
                response_json["success"] = false;
                response_json["message"] = "恢复失败";
            }
            else {
                response_json["success"] = true;
                response_json["message"] = "用户已恢复";
            }

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            });
    //修改管理员等级
    svr.Post("/admin/change_level", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        int admin_id = stoi(req.get_param_value("admin_id"));
        int target_id = stoi(req.get_param_value("target_id"));
        int new_level = stoi(req.get_param_value("level"));

        MYSQL* conn = connect_db();

        if (get_admin_level(conn, admin_id) < 3) {
            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以修改管理员等级";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (new_level < 1 || new_level > 3) {
            response_json["success"] = false;
            response_json["message"] = "管理员等级只能是1到3";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "UPDATE admin SET level = "
            + to_string(new_level)
            + " WHERE user_id = "
            + to_string(target_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "修改失败";
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "管理员等级修改成功";
        }

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    //让管理员变成普通用户
    svr.Post("/admin/remove_admin", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        int admin_id = stoi(req.get_param_value("admin_id"));
        int target_id = stoi(req.get_param_value("target_id"));

        MYSQL* conn = connect_db();

        if (get_admin_level(conn, admin_id) < 3) {
            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以取消管理员身份";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql = "DELETE FROM admin WHERE user_id = " + to_string(target_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "取消管理员身份失败";
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "该管理员已变为普通用户";
        }

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    //让普通用户成为管理员
    svr.Post("/admin/add_admin", [](const Request& req, Response& res) {
        set_cors(res);

        Json::Value response_json;

        int admin_id = stoi(req.get_param_value("admin_id"));
        int target_id = stoi(req.get_param_value("target_id"));
        int level = stoi(req.get_param_value("level"));

        MYSQL* conn = connect_db();

        if (get_admin_level(conn, admin_id) < 3) {
            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以添加管理员";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (level < 1 || level > 3) {
            response_json["success"] = false;
            response_json["message"] = "管理员等级只能是1到3";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "INSERT INTO admin(user_id, level) VALUES("
            + to_string(target_id) + ", "
            + to_string(level) + ")";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "添加管理员失败，可能该用户已经是管理员";
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "用户已成为管理员";
        }

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
        });
    
    cout << "Server running at http://127.0.0.1:8080;" << endl;
    svr.listen("127.0.0.1", 8080);

    return 0;
}