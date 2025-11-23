// example/router_usage.cpp
#include "../router/router.hpp"
#include "../router/middleware.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace http::router;
using namespace http::parser;

// Handler: 首页
HttpResponse index_handler(const HttpRequest& req) {
    return HttpResponse::ok()
        .with_html("<h1>Welcome</h1>");
}

// Handler: 静态文件
HttpResponse static_file_handler(const HttpRequest& req) {
    std::string filepath = "./static" + req.request_line.uri;

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return HttpResponse::not_found()
            .with_text("File not found");
    }

    std::vector<uint8_t> content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return HttpResponse::ok()
        .with_body(std::move(content));
}

// Handler: 用户详情（带路径参数）
HttpResponse user_handler(const HttpRequest& req) {
    return HttpResponse::ok()
        .with_html("<h1>User Details</h1>");
}

int main() {
    // 构建路由器（函数式链式调用）
    auto router = Router()
        .get("/", index_handler)
        .get("/user/:id", user_handler)
        .get("/static/*path", static_file_handler);

    // 添加中间件
    auto with_middleware = middleware::compose(
        {
            middleware::logging(),
            middleware::cors()
        },
        [&router](const HttpRequest& req) {
            return router.handle(req);
        }
    );

    // 测试
    HttpRequest test_req{
        RequestLine{Method::Get, "/", Version::Http11},
        {},
        {}
    };

    auto response = with_middleware(test_req);
    std::cout << "Status: " << response.status_code << std::endl;

    return 0;
}
