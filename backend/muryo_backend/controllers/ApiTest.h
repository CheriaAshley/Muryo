#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ApiTest : public drogon::HttpController<ApiTest>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ApiTest::test, "/test", Get);
    ADD_METHOD_TO(ApiTest::ping, "/ping", Get);
    METHOD_LIST_END

        void test(const HttpRequestPtr& req,
            std::function<void(const HttpResponsePtr&)>&& callback);

    void ping(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback);
};
