#include <iostream>
#include <mysql.h>

int main() {
    MYSQL* conn = mysql_init(nullptr);
    if (conn == nullptr) {
        std::cout << "mysql_init failed" << std::endl;
        return 1;
    }

    if (mysql_real_connect(
        conn,
        "127.0.0.1",
        "muryo_user",
        "muryo123",
        "muryo",
        3306,
        nullptr,
        0
    ) == nullptr) {
        std::cout << "connect failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return 1;
    }

    if (mysql_query(conn, "SELECT 1")) {
        std::cout << "query failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return 1;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == nullptr) {
        std::cout << "store result failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return 1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    if (row != nullptr) {
        std::cout << "OK: " << row[0] << std::endl;
    }
    else {
        std::cout << "no result" << std::endl;
    }

    mysql_free_result(res);
    mysql_close(conn);

    return 0;
}