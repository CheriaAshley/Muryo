#include <drogon/drogon.h>

int main() {
    drogon::app().addListener("127.0.0.1", 8080);

    drogon::app().registerHandler(
        "/ping",
        [](const drogon::HttpRequestPtr&,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                Json::Value json;
                json["status"] = "ok";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
                callback(resp);
        },
        { drogon::Get }
    );

    drogon::app().registerHandler(
        "/test",
        [](const drogon::HttpRequestPtr&,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
                Json::Value json;
                json["msg"] = "MURYO backend running";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
                callback(resp);
        },
        { drogon::Get }
    );

    drogon::app().run();
}