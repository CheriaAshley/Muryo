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
    cout << "Server running at http://127.0.0.1:8080" << endl;
    svr.listen("127.0.0.1", 8080);

    return 0;
}