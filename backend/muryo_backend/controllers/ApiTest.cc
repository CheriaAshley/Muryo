#include "ApiTest.h"

void ApiTest::test(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    Json::Value json;
    json["msg"] = "MURYO backend running";

    auto resp = HttpResponse::newHttpJsonResponse(json);
    callback(resp);
}

void ApiTest::ping(const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    Json::Value json;
    json["status"] = "ok";

    auto resp = HttpResponse::newHttpJsonResponse(json);
    callback(resp);
}