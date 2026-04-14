// muryo_backend.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
/*
#include <iostream>
#include "httplib.h"

int main() {
    httplib::Server svr;

    svr.Get("/test", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"msg\":\"MURYO backend running\"}", "application/json");
        });

    std::cout << "Server started at " << std::endl;

    svr.listen("127.0.0.1", 8080);

    return 0;
}*/