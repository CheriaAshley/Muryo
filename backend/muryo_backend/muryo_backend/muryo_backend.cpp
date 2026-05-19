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

// 解决 JSON 特殊字符问题（前期纯手拼JSON的产物暂留）
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
    svr.Post("/items/publish", [](const Request& req, Response& res) {
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

    //申请交换
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

        for (int i = 0; i < item_ids.size(); i++) {
            int item_id;
            int quantity;
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
    
    //处理申请
    svr.Post("/exchange/handle", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string exchange_idstr = req.get_param_value("exchange_id");
        string action = req.get_param_value("action");
        string user_idstr = req.get_param_value("user_id");

        if (exchange_idstr.empty() || action.empty() || user_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "交换ID、操作或用户ID参数不能为空";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        int exchange_id;
        int user_id;

        try {
            exchange_id = stoi(exchange_idstr);
            user_id = stoi(user_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "交换ID或用户ID参数格式不正确";
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (exchange_id <= 0 || user_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "参数编号不合法";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (action != "agree" && action != "reject" && action != "complete"&& action != "cancel") {
            response_json["success"] = false;
            response_json["message"] = "无效的操作！";
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
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

        string sql = "CALL proc_handle_exchange(";
        sql += to_string(exchange_id) ;
        sql += ", '";
        sql += action;
        sql += "', ";
        sql += to_string(user_id);
        sql += ")";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "存储过程执行失败！";
            
            mysql_close(conn);
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        
        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = "存储过程没有返回结果！";
            
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
            response_json["success"] = false;
            response_json["message"] = "读取存储过程结果失败！";
            
            mysql_close(conn);
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
                );
            return;
        }

        int success = row[0] ? atoi(row[0]) : 0;
        string message = row[1] ? row[1] : "操作完成";

        mysql_free_result(result);

        while (mysql_next_result(conn) == 0) {
            MYSQL_RES* extra_result = mysql_store_result(conn);
            if (extra_result != NULL) {
                mysql_free_result(extra_result);
            }
        }

        response_json["success"] = (success == 1);
        response_json["message"] = message;

        mysql_close(conn);
        
        res.set_content(
            response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );
    });
    
    //查看自己发出的申请
    svr.Get("/exchange/outgoing", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string utostr = req.get_param_value("uto");
        
        if (utostr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "咪，请先登录后再查看申请~";
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        int uto;

        try {
            uto = stoi(utostr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "请输入正确的用户ID哟~";
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        string sql =
            "SELECT exchange_id, detail_id, item_id, item_name, "
            "apply_quantity, left_quantity, status, status_text, "
            "target_user_id, target_user_name "
            "FROM view_exchange_detail "
            "WHERE apply_user_id = " + to_string(uto) + " "
            "ORDER BY "
            "CASE "
            "WHEN status = 0 THEN 0 "
            "WHEN status = 2 THEN 1 "
            "ELSE 2 "
            "END, "
            "CASE "
            "WHEN status IN (1,3,4) THEN detail_id "
            "ELSE 0 "
            "END DESC, "
            "exchange_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询失败: ") + mysql_error(conn);
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        
        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取结果失败: ") + mysql_error(conn);
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row;
        Value data(arrayValue);

        while ((row = mysql_fetch_row(result)) != NULL) {
            Value item;

            item["exchange_id"] = row[0] ? atoi(row[0]) : 0;
            item["detail_id"] = row[1] ? atoi(row[1]) : 0;
            item["item_id"] = row[2] ? atoi(row[2]) : 0;
            item["item_name"] = row[3] ? row[3] : "";
            item["apply_quantity"] = row[4] ? atoi(row[4]) : 0;
            item["left_quantity"] = row[5] ? atoi(row[5]) : 0;
            item["status"] = row[6] ? atoi(row[6]) : -1;
            item["status_text"] = row[7] ? row[7] : "";
            item["target_user_id"] = row[8] ? atoi(row[8]) : 0;
            item["target_user_name"] = row[9] ? row[9] : "";

            data.append(item);
        }

        response_json["success"] = true;
        response_json["message"] = data.size() == 0 ? "还没有提交过任何申请" : "查询成功";
        response_json["data"] = data;

        res.set_content(
            response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );

        mysql_free_result(result);
        mysql_close(conn);
    });

    //查看所有制品列表
    svr.Get("/items", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            response_json["data"] = Value(arrayValue);
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        string sql =
            "SELECT * "
            "FROM view_item_detail "
            "WHERE status = 0 AND quantity > 0 "
            "ORDER BY item_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "查询失败";
            response_json["data"] = Value(arrayValue);
     
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        
        if (result == NULL) {

            response_json["success"] = false;
            response_json["message"] = "请求结果失败";
            response_json["data"] = Value(arrayValue);
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row;
        Value data(arrayValue);

        while ((row = mysql_fetch_row(result))!= NULL) {
            Value item;

            item["item_id"] = row[0] ? atoi(row[0]) : 0;
            item["owner"] = row[1] ? atoi(row[1]) : 0;
            item["owner_name"] = row[2] ? row[2] : "";
            item["item_name"] = row[3] ? row[3] : "";
            item["role"] = row[4] ? row[4] : "";
            item["type"] = row[5] ? row[5] : "";
            item["quantity"] = row[6] ? atoi(row[6]) : 0;
            item["status"] = row[7] ? atoi(row[7]) : 0;
            item["img_url"] = row[8] ? row[8] : "";
            item["intro"] = row[9] ? row[9] : "";

            data.append(item);
        }

        response_json["success"] = true;
        response_json["message"] = data.size() == 0 ? "暂时还没有制品哦~" : "查询成功";
        response_json["data"] = data;

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(
            response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );
    });
    
    //查看我的制品
    svr.Get("/items/my", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string ownerstr = req.get_param_value("owner");
        
        if (ownerstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "请先登录后再查看我的制品";
            response_json["data"] = Value(arrayValue);

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }
        
        int owner;

        try {
            owner = stoi(ownerstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "请输入有效的用户ID";
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        if (owner <= 0) {
            response_json["success"] = false;
            response_json["message"] = "用户ID不合法";
            response_json["data"] = Value(arrayValue);

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            response_json["data"] = Value(arrayValue);

            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            return;
        }

        string sql =
        "SELECT * "
        "FROM view_item_detail "
        "WHERE status <> 2 AND owner = " + to_string(owner) + " "
        "ORDER BY item_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询失败: ") + mysql_error(conn);
            response_json["data"] = Value(arrayValue);
            
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
            response_json["message"] = string("结果获取失败: ") + mysql_error(conn);
            response_json["data"] = Value(arrayValue);
            
            res.set_content(
                response_json.toStyledString(),
                "application/json; charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row;
        Value data(arrayValue);

        while ((row = mysql_fetch_row(result))!= NULL) {
            Value item;

            item["item_id"] = row[0] ? atoi(row[0]) : 0;
            item["owner"] = row[1] ? atoi(row[1]) : 0;
            item["owner_name"] = row[2] ? row[2] : "";
            item["item_name"] = row[3] ? row[3] : "";
            item["role"] = row[4] ? row[4] : "";
            item["type"] = row[5] ? row[5] : "";
            item["quantity"] = row[6] ? atoi(row[6]) : 0;
            item["status"] = row[7] ? atoi(row[7]) : 0;
            item["img_url"] = row[8] ? row[8] : "";
            item["intro"] = row[9] ? row[9] : "";

            data.append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = data.size() == 0 ? "咪还没有发布制品哦~" : "查询成功";
        response_json["data"] = data;

        res.set_content(
            response_json.toStyledString(), 
            "application/json; charset=UTF-8"
        );
    });

    //查看制品详情
    svr.Get("/items/detail", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string item_idstr = req.get_param_value("item_id");
        
        if (item_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少item_id参数";
            response_json["data"] = Value(objectValue);
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        int item_id;

        try {
            item_id = stoi(item_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "请输入有效的制品ID";
            response_json["data"] = Value(objectValue);

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (item_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "制品ID不合法";
            response_json["data"] = Value(objectValue);

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败";
            response_json["data"] = Value(objectValue);
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        string sql =
            "SELECT * "
            "FROM view_item_detail "
            "WHERE item_id = " + to_string(item_id) + " "
            "LIMIT 1";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "查询失败";
            response_json["data"] = Value(objectValue);
  
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            string err = mysql_error(conn);
            response_json["success"] = false;
            response_json["message"] = "获取结果失败";
            response_json["data"] = Value(objectValue);
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            response_json["success"] = false;
            response_json["message"] = "没有找到该制品";
            response_json["data"] = Value(objectValue);
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            mysql_free_result(result);
            mysql_close(conn);
            return;
        }

        Value item;

        item["item_id"] = row[0] ? atoi(row[0]) : 0;
        item["owner"] = row[1] ? atoi(row[1]) : 0;
        item["owner_name"] = row[2] ? row[2] : "";
        item["item_name"] = row[3] ? row[3] : "";
        item["role"] = row[4] ? row[4] : "";
        item["type"] = row[5] ? row[5] : "";
        item["quantity"] = row[6] ? atoi(row[6]) : 0;
        item["status"] = row[7] ? atoi(row[7]) : 0;
        item["img_url"] = row[8] ? row[8] : "";
        item["intro"] = row[9] ? row[9] : "";

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "查询成功";
        response_json["data"] = item;
        
        res.set_content(
            response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );
    });
    
    //用户删除制品
    svr.Post("/items/delete", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string item_idstr = req.get_param_value("item_id");
        string ownerstr = req.get_param_value("owner");

        if (item_idstr.empty() || ownerstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少制品编号或用户编号";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        int item_id, owner;

        try {
            item_id = stoi(item_idstr);
            owner = stoi(ownerstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "制品编号或用户编号无效";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (item_id <= 0 || owner <= 0) {
            response_json["success"] = false;
            response_json["message"] = "制品编号或用户编号不合法";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
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

        string sql_select =
            "SELECT owner, status "
            "FROM item "
            "WHERE item_id = " + to_string(item_id);

        if (mysql_query(conn, sql_select.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "查询制品失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = "查询结果失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);
            
            response_json["success"] = false;
            response_json["message"] = "没有找到该制品";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        int real_owner = row[0] ? atoi(row[0]) : 0;
        int status = row[1] ? atoi(row[1]) : -1;

        mysql_free_result(result);

        if (real_owner != owner) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "咪没有权限删除别人的制品~";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        if (status == 1) {
            mysql_close(conn);
            response_json["success"] = false;
            response_json["message"] = "该制品已经删除过了~";
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
        }

        string sql_update =
            "UPDATE item "
            "SET status = 1 "
            "WHERE item_id = " + to_string(item_id) + " "
            "AND owner = " + to_string(owner) + " "
            "AND status <> 1";

        if (mysql_query(conn, sql_update.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "制品删除失败";

            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        if (mysql_affected_rows(conn) == 0) {
            response_json["success"] = false;
            response_json["message"] = "没有删除到任何制品";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "制品删除成功，期待咪的下一次产粮……";
        
        res.set_content(
            response_json.toStyledString(),
            "application/json;charset=UTF-8"
        );
    });

    //查看个人信息
    svr.Get("/user/profile", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string user_id_str = req.get_param_value("user_id");
        
        if (user_id_str.empty()) {
            response_json["success"] = false;
            response_json["message"] = "请先登录后再查看个人信息";

            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }

        int user_id;

        try {
            user_id = stoi(user_id_str);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "用户ID无效";
            
            res.set_content(
                response_json.toStyledString(), 
                "application/json;charset=UTF-8"
            );
            return;
        }
        if (user_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "用户ID不合法";

            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            return;
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

        string sql =
            "SELECT user_id, user_name, contact, introduction "
            "FROM `user` "
            "WHERE user_id = " + to_string(user_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "查询失败";
            
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = "获取结果失败";
            res.set_content(
                response_json.toStyledString(),
                "application/json;charset=UTF-8"
            );
            mysql_close(conn);
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "未找到该用户";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        response_json["success"] = true;
        response_json["message"] = "查询成功";
        response_json["data"]["user_id"] = row[0] ? atoi(row[0]) : 0;
        response_json["data"]["user_name"] = row[1] ? row[1] : "";
        response_json["data"]["contact"] = row[2] ? row[2] : "";
        response_json["data"]["introduction"] = row[3] ? row[3] : "";

        mysql_free_result(result);
        mysql_close(conn);

        res.set_content(
            response_json.toStyledString(), 
            "application/json;charset=UTF-8"
        );
    });

    //查看待办申请
    svr.Get("/exchange/todo", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;
        response_json["data"] = Value(arrayValue);

        string ufromstr = req.get_param_value("ufrom");
        if (ufromstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "请先登录后再查看待办申请";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

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
        if (ufrom <= 0) {
            response_json["success"] = false;
            response_json["message"] = "用户ID不合法";

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
            "detail_id, "
            "exchange_id, "
            "item_id, "
            "item_name, "
            "apply_quantity, "
            "left_quantity, "
            "status, "
            "status_text, "
            "apply_user_id, "
            "apply_user_name "
            "FROM view_exchange_detail "
            "WHERE target_user_id = " + to_string(ufrom) + " "
            "AND status IN (0, 2) "
            "ORDER BY "
            "CASE WHEN status = 0 THEN 0 ELSE 1 END ASC, "
            "detail_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询失败: ") + mysql_error(conn);
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取结果失败: ") + mysql_error(conn);
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)) != NULL) {
            Value item;

            item["detail_id"] = row[0] ? atoi(row[0]) : 0;
            item["exchange_id"] = row[1] ? atoi(row[1]) : 0;
            item["item_id"] = row[2] ? atoi(row[2]) : 0;
            item["item_name"] = row[3] ? row[3] : "暂无名称";
            item["apply_quantity"] = row[4] ? atoi(row[4]) : 0;
            item["left_quantity"] = row[5] ? atoi(row[5]) : 0;
            item["status"] = row[6] ? atoi(row[6]) : -1;
            item["status_text"] = row[7] ? row[7] : "";
            item["applicant_id"] = row[8] ? atoi(row[8]) : 0;
            item["applicant_name"] = row[9] ? row[9] : "未知用户";

            response_json["data"].append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] =response_json["data"].size() == 0 ?"暂时没有待办申请哦~" :"获取待办申请成功";

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    //查看我收到的交换申请
    svr.Get("/exchange/incoming", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        response_json["data"] = Value(arrayValue);
        
        string ufromstr = req.get_param_value("ufrom");
        if (ufromstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "请先登录后再查看收到的申请";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int ufrom;

        try {
            ufrom = stoi(ufromstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "用户编号无效！";
  
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }
        if (ufrom <= 0) {
            response_json["success"] = false;
            response_json["message"] = "用户编号不合法";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL* conn = connect_db();
        if (conn == NULL) {
            response_json["success"] = false;
            response_json["message"] = "数据库连接失败！";
    
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "SELECT "
            "exchange_id, "
            "apply_user_id, "
            "apply_user_name, "
            "target_user_id, "
            "target_user_name, "
            "detail_id, "
            "item_id, "
            "item_name, "
            "apply_quantity, "
            "left_quantity, "
            "status, "
            "status_text "
            "FROM view_exchange_detail "
            "WHERE target_user_id = " + to_string(ufrom) + " "
            "ORDER BY "
            "CASE "
            "WHEN status = 0 THEN 0 "
            "WHEN status = 2 THEN 1 "
            "ELSE 2 "
            "END, "
            "CASE "
            "WHEN status IN (1,3,4) THEN detail_id "
            "ELSE 0 "
            "END DESC, "
            "exchange_id DESC";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = "视图查询失败！";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = "查询结果获取失败！";
   
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            mysql_close(conn);
            return;
        }

        Value data(arrayValue);
        
        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result)) != NULL) {
            Value item;

            item["exchange_id"] = row[0] ? atoi(row[0]) : 0;
            item["apply_user_id"] = row[1] ? atoi(row[1]) : 0;
            item["apply_user_name"] = row[2] ? row[2] : "";
            item["target_user_id"] = row[3] ? atoi(row[3]) : 0;
            item["target_user_name"] = row[4] ? row[4] : "";
            item["detail_id"] = row[5] ? atoi(row[5]) : 0;
            item["item_id"] = row[6] ? atoi(row[6]) : 0;
            item["item_name"] = row[7] ? row[7] : "";
            item["apply_quantity"] = row[8] ? atoi(row[8]) : 0;
            item["left_quantity"] = row[9] ? atoi(row[9]) : 0;
            item["status"] = row[10] ? atoi(row[10]) : -1;
            item["status_text"] = row[11] ? row[11] : "";

            data.append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] =response_json["data"].size() == 0 ?"暂时没有收到交换申请" :"视图查询成功";

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

   
    /*
        下面是管理员相关部分
        由于是2.0版本的新功能，且原项目只打算做1.0版本
        所以管理员相关功能的代码质量可能会比较粗糙
        且有大量AI代劳（主要是完善手搓时简陋的代码逻辑和补全一些边界检查），
        请多包涵~(>_<)
    */
    
    //管理员封禁制品
    svr.Post("/admin/apply/ban_item", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string item_idstr = req.get_param_value("item_id");
        string reason = req.get_param_value("reason");

        if (admin_idstr.empty() || item_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少管理员编号或制品编号";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

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
        if (admin_id <= 0 || item_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号或制品编号不合法";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (reason.empty()) {
            reason = "管理员申请封禁违规制品";
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
            response_json["message"] = "你不是管理员，不能提交封禁申请";
            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string check_sql =
            "SELECT status FROM item WHERE item_id = " + to_string(item_id);

        if (mysql_query(conn, check_sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询制品失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取制品结果失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "没有找到该制品";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int item_status = row[0] ? atoi(row[0]) : -1;
        mysql_free_result(result);

        if (item_status == 2) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "该制品已经被封禁";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (level == 3) {
            string sql = "UPDATE item SET status = 2 WHERE item_id = " + to_string(item_id);

            if (mysql_query(conn, sql.c_str())) {
                response_json["success"] = false;
                response_json["message"] = "三级管理员直接封禁失败";
            }
            else if (mysql_affected_rows(conn) == 0) {
                response_json["success"] = false;
                response_json["message"] = "没有封禁到任何制品";
            }
            else {
                response_json["success"] = true;
                response_json["message"] = "三级管理员已直接封禁制品";
            }

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

         char reason_escape[1024];

        mysql_real_escape_string(
            conn,
            reason_escape,
            reason.c_str(),
            reason.length()
        );

        string sql =
            "INSERT INTO admin_apply(admin_id, apply_type, target_id, reason, status) VALUES("
            + to_string(admin_id) + ", 'ban_item', "
            + to_string(item_id) + ", '"
            + reason_escape + "', 0)";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("提交封禁申请失败: ") + mysql_error(conn);
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "封禁申请已提交，等待高级管理员审核";
        }

        mysql_close(conn);
        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 封禁用户
    svr.Post("/admin/apply/ban_user", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string user_idstr = req.get_param_value("user_id");
        string reason = req.get_param_value("reason");

        if (admin_idstr.empty() || user_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少管理员编号或用户编号";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

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

        if (admin_id <= 0 || user_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号或用户编号不合法";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (admin_id == user_id) {
            response_json["success"] = false;
            response_json["message"] = "不能封禁自己";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (reason.empty()) {
            reason = "管理员申请封禁违规用户";
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
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有二级及以上管理员可以提交封禁申请";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string check_sql =
            "SELECT status FROM `user` WHERE user_id = " + to_string(user_id);

        if (mysql_query(conn, check_sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询用户失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取用户结果失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "没有找到该用户";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int user_status = row[0] ? atoi(row[0]) : -1;
        mysql_free_result(result);

        if (user_status == 0) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "该用户已经被封禁";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int target_level = get_admin_level(conn, user_id);

        if (target_level >= level) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "不能封禁同级或更高级管理员";
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (level == 3) {
            string sql =
                "UPDATE `user` SET status = 0 WHERE user_id = "
                + to_string(user_id);

            if (mysql_query(conn, sql.c_str())) {
                response_json["success"] = false;
                response_json["message"] = string("三级管理员直接封禁失败: ") + mysql_error(conn);
            }
            else if (mysql_affected_rows(conn) == 0) {
                response_json["success"] = false;
                response_json["message"] = "没有封禁到任何用户";
            }
            else {
                response_json["success"] = true;
                response_json["message"] = "三级管理员已直接封禁用户";
            }

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        char reason_escape[1024];

        mysql_real_escape_string(
            conn,
            reason_escape,
            reason.c_str(),
            reason.length()
        );

        string sql =
            "INSERT INTO admin_apply(admin_id, apply_type, target_id, reason, status) VALUES("
            + to_string(admin_id) + ", 'ban_user', "
            + to_string(user_id) + ", '"
            + reason_escape + "', 0)";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("提交封禁申请失败: ") + mysql_error(conn);
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "封禁申请已提交，等待三级管理员审核";
        }

        mysql_close(conn);

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 查看待审核申请
    svr.Get("/admin/apply/list", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;
        Value data(arrayValue);

        string admin_idstr = req.get_param_value("admin_id");

        if (admin_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少管理员编号";
            response_json["data"] = data;

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

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

        if (admin_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号不合法";
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
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "权限不足";
            response_json["data"] = data;

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询失败: ") + mysql_error(conn);
            response_json["data"] = data;

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取结果失败: ") + mysql_error(conn);
            response_json["data"] = data;

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result)) != NULL) {
            Value item;

            item["apply_id"] = row[0] ? atoi(row[0]) : 0;
            item["admin_id"] = row[1] ? atoi(row[1]) : 0;
            item["apply_type"] = row[2] ? row[2] : "";
            item["target_id"] = row[3] ? atoi(row[3]) : 0;
            item["reason"] = row[4] ? row[4] : "";
            item["status"] = row[5] ? atoi(row[5]) : 0;
            item["create_time"] = row[6] ? row[6] : "";

            data.append(item);
        }

        mysql_free_result(result);
        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = data.size() == 0 ? "暂无待审核申请" : "查询成功";
        response_json["data"] = data;

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 处理审核申请
    svr.Post("/admin/apply/handle", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string apply_idstr = req.get_param_value("apply_id");
        string action = req.get_param_value("action");

        if (admin_idstr.empty() || apply_idstr.empty() || action.empty()) {
            response_json["success"] = false;
            response_json["message"] = "参数不能为空";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

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

        if (admin_id <= 0 || apply_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号或申请编号不合法";

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

        if (level < 2) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "权限不足，不能审核申请";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (mysql_query(conn, "START TRANSACTION")) {
            response_json["success"] = false;
            response_json["message"] = string("事务开启失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "SELECT apply_type, target_id, status "
            "FROM admin_apply "
            "WHERE apply_id = " + to_string(apply_id) + " "
            "FOR UPDATE";

        if (mysql_query(conn, sql.c_str())) {
            mysql_query(conn, "ROLLBACK");

            response_json["success"] = false;
            response_json["message"] = string("查询申请失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            mysql_query(conn, "ROLLBACK");

            response_json["success"] = false;
            response_json["message"] = string("获取申请结果失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            mysql_free_result(result);
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "申请不存在";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string apply_type = row[0] ? row[0] : "";
        int target_id = row[1] ? atoi(row[1]) : 0;
        int status = row[2] ? atoi(row[2]) : -1;

        mysql_free_result(result);

        if (status != 0) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "该申请已经被处理";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (apply_type == "delete_item" && level < 2) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有二级及以上管理员可以审核删除申请";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (apply_type == "ban_user" && level < 3) {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以审核封禁申请";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (apply_type != "delete_item" && apply_type != "ban_user") {
            mysql_query(conn, "ROLLBACK");
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "未知申请类型";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (action == "agree") {
            if (apply_type == "delete_item") {
                sql =
                    "UPDATE item "
                    "SET status = 2 "
                    "WHERE item_id = " + to_string(target_id);
            }
            else {
                sql =
                    "UPDATE `user` "
                    "SET status = 0 "
                    "WHERE user_id = " + to_string(target_id);
            }

            if (mysql_query(conn, sql.c_str())) {
                mysql_query(conn, "ROLLBACK");

                response_json["success"] = false;
                response_json["message"] = string("执行申请操作失败: ") + mysql_error(conn);

                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            if (mysql_affected_rows(conn) == 0) {
                mysql_query(conn, "ROLLBACK");

                response_json["success"] = false;
                response_json["message"] = "没有更新到目标对象，可能目标已经不存在或状态未变化";

                mysql_close(conn);
                res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
                return;
            }

            sql =
                "UPDATE admin_apply "
                "SET status = 1, handle_admin_id = "
                + to_string(admin_id) +
                ", handle_time = NOW() "
                "WHERE apply_id = " + to_string(apply_id);
        }
        else {
            sql =
                "UPDATE admin_apply "
                "SET status = 2, handle_admin_id = "
                + to_string(admin_id) +
                ", handle_time = NOW() "
                "WHERE apply_id = " + to_string(apply_id);
        }

        if (mysql_query(conn, sql.c_str())) {
            mysql_query(conn, "ROLLBACK");

            response_json["success"] = false;
            response_json["message"] = string("更新申请状态失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (mysql_query(conn, "COMMIT")) {
            mysql_query(conn, "ROLLBACK");

            response_json["success"] = false;
            response_json["message"] = string("事务提交失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;

        if (action == "agree") {
            response_json["message"] = "申请已通过并执行成功";
        }
        else {
            response_json["message"] = "申请已拒绝";
        }

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 恢复封禁用户
    svr.Post("/admin/user/recover", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string user_idstr = req.get_param_value("user_id");

        if (admin_idstr.empty() || user_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少管理员编号或用户编号";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

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

        if (admin_id <= 0 || user_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号或用户编号不合法";

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

        if (get_admin_level(conn, admin_id) < 3) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以恢复用户";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string check_sql =
            "SELECT status FROM `user` WHERE user_id = " + to_string(user_id);

        if (mysql_query(conn, check_sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询用户失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取用户结果失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "没有找到该用户";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int user_status = row[0] ? atoi(row[0]) : -1;

        mysql_free_result(result);

        if (user_status == 1) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "该用户目前是正常状态，不需要恢复";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "UPDATE `user` SET status = 1 WHERE user_id = "
            + to_string(user_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("恢复失败: ") + mysql_error(conn);
        }
        else if (mysql_affected_rows(conn) == 0) {
            response_json["success"] = false;
            response_json["message"] = "没有恢复到任何用户";
        }
        else {
            response_json["success"] = true;
            response_json["message"] = "用户已恢复";
        }

        mysql_close(conn);

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 修改管理员等级
    svr.Post("/admin/change_level", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string target_idstr = req.get_param_value("target_id");
        string levelstr = req.get_param_value("level");

        if (admin_idstr.empty() || target_idstr.empty() || levelstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "参数不能为空";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int admin_id;
        int target_id;
        int new_level;

        try {
            admin_id = stoi(admin_idstr);
            target_id = stoi(target_idstr);
            new_level = stoi(levelstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (admin_id <= 0 || target_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号不合法";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (new_level < 1 || new_level > 3) {
            response_json["success"] = false;
            response_json["message"] = "管理员等级只能是1到3";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (admin_id == target_id) {
            response_json["success"] = false;
            response_json["message"] = "不能修改自己的管理员等级";

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

        int admin_level = get_admin_level(conn, admin_id);

        if (admin_level < 3) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以修改管理员等级";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int target_level = get_admin_level(conn, target_id);

        if (target_level < 1) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "目标用户不是管理员，不能修改管理员等级";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (target_level == new_level) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "目标管理员已经是该等级";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "UPDATE admin "
            "SET level = " + to_string(new_level) + " "
            "WHERE user_id = " + to_string(target_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("修改失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (mysql_affected_rows(conn) == 0) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "没有修改到任何管理员";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "管理员等级修改成功";

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 让管理员变成普通用户
    svr.Post("/admin/remove_admin", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string target_idstr = req.get_param_value("target_id");

        if (admin_idstr.empty() || target_idstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "缺少管理员编号或目标用户编号";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int admin_id;
        int target_id;

        try {
            admin_id = stoi(admin_idstr);
            target_id = stoi(target_idstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (admin_id <= 0 || target_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号或目标用户编号不合法";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (admin_id == target_id) {
            response_json["success"] = false;
            response_json["message"] = "不能取消自己的管理员身份";

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

        int admin_level = get_admin_level(conn, admin_id);

        if (admin_level < 3) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以取消管理员身份";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int target_level = get_admin_level(conn, target_id);

        if (target_level < 1) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "目标用户不是管理员";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "DELETE FROM admin "
            "WHERE user_id = " + to_string(target_id);

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("取消管理员身份失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (mysql_affected_rows(conn) == 0) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "没有修改到任何管理员";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "该管理员已变为普通用户";

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });

    // 让普通用户成为管理员
    svr.Post("/admin/add_admin", [](const Request& req, Response& res) {
        set_cors(res);

        Value response_json;

        string admin_idstr = req.get_param_value("admin_id");
        string target_idstr = req.get_param_value("target_id");
        string levelstr = req.get_param_value("level");

        if (admin_idstr.empty() || target_idstr.empty() || levelstr.empty()) {
            response_json["success"] = false;
            response_json["message"] = "参数不能为空";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int admin_id;
        int target_id;
        int level;

        try {
            admin_id = stoi(admin_idstr);
            target_id = stoi(target_idstr);
            level = stoi(levelstr);
        }
        catch (...) {
            response_json["success"] = false;
            response_json["message"] = "参数错误";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (admin_id <= 0 || target_id <= 0) {
            response_json["success"] = false;
            response_json["message"] = "管理员编号或目标用户编号不合法";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        if (level < 1 || level > 3) {
            response_json["success"] = false;
            response_json["message"] = "管理员等级只能是1到3";

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

        int admin_level = get_admin_level(conn, admin_id);

        if (admin_level < 3) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "只有三级管理员可以添加管理员";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int target_level = get_admin_level(conn, target_id);

        if (target_level >= 1) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "该用户已经是管理员";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string check_sql =
            "SELECT user_id, status FROM `user` WHERE user_id = "
            + to_string(target_id);

        if (mysql_query(conn, check_sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("查询用户失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (result == NULL) {
            response_json["success"] = false;
            response_json["message"] = string("获取用户结果失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row == NULL) {
            mysql_free_result(result);
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "目标用户不存在";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        int user_status = row[1] ? atoi(row[1]) : -1;
        mysql_free_result(result);

        if (user_status == 0) {
            mysql_close(conn);

            response_json["success"] = false;
            response_json["message"] = "封禁用户不能成为管理员";

            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        string sql =
            "INSERT INTO admin(user_id, level) VALUES("
            + to_string(target_id) + ", "
            + to_string(level) + ")";

        if (mysql_query(conn, sql.c_str())) {
            response_json["success"] = false;
            response_json["message"] = string("添加管理员失败: ") + mysql_error(conn);

            mysql_close(conn);
            res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
            return;
        }

        mysql_close(conn);

        response_json["success"] = true;
        response_json["message"] = "用户已成为管理员";

        res.set_content(response_json.toStyledString(), "application/json;charset=UTF-8");
    });
    
    cout << "Muryo Server running at http://127.0.0.1:8080;" << endl;
    svr.listen("127.0.0.1", 8080);

    return 0;
}