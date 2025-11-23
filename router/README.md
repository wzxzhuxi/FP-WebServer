# HTTP 路由器 - 函数式路由与中间件系统

完全不可变、类型安全的 HTTP 路由器，支持路径匹配、中间件组合和函数式请求处理。

## 架构

路由器采用**持久化数据结构**设计，每次路由添加都返回新的 Router 实例，保证完全不可变。

### 核心概念

```cpp
// Handler：处理 HTTP 请求的纯函数
using Handler = std::function<HttpResponse(const HttpRequest&)>;

// Middleware：包装 Handler 的高阶函数
using Middleware = std::function<Handler(Handler)>;

// Router：不可变路由表
class Router {
    std::shared_ptr<const RouteTable> routes_;  // 共享只读路由表
};
```

### 文件结构

- **`types.hpp`**: HTTP 响应类型、Handler 和 Middleware 类型定义
- **`router.hpp`**: 不可变路由器实现
- **`matcher.hpp`**: 路径模式匹配（支持路径参数）
- **`middleware.hpp`**: 内置中间件（日志、CORS 等）

## 使用方法

### 基础路由

```cpp
#include "router/router.hpp"

using namespace http::router;
using namespace http::parser;

// 创建空路由器
Router router;

// 添加路由（每次返回新的 Router）
router = router
    .get("/", [](const HttpRequest& req) {
        return HttpResponse{}
            .status(200)
            .body("Hello World");
    })
    .get("/api/users", [](const HttpRequest& req) {
        return HttpResponse{}
            .status(200)
            .header("Content-Type", "application/json")
            .body(R"({"users": []})");
    })
    .post("/api/users", [](const HttpRequest& req) {
        // 创建用户
        return HttpResponse{}.status(201);
    });

// 匹配请求
HttpRequest req = /* 解析的请求 */;
auto match = router.find(req);

if (match) {
    HttpResponse response = match->handler(req);
    // 发送响应给客户端
}
```

### 路径参数

```cpp
router = router.get("/users/:id", [](const HttpRequest& req) {
    // 访问路径参数需要先 find() 获取 RouteMatch
    return HttpResponse{}.status(200);
});

// 使用
HttpRequest req;
req.request_line.uri = "/users/123";

auto match = router.find(req);
if (match) {
    std::string user_id = match->params["id"];  // "123"
    // 处理请求
}
```

### 通配符路径

```cpp
router = router.get("/static/*path", [](const HttpRequest& req) {
    return HttpResponse{}.status(200);
});

// 匹配示例
// /static/css/style.css  → params["path"] = "css/style.css"
// /static/js/app.js      → params["path"] = "js/app.js"
```

### 中间件组合

```cpp
#include "router/middleware.hpp"

using namespace http::router::middleware;

// 定义中间件
auto auth = [](Handler next) -> Handler {
    return [next](const HttpRequest& req) -> HttpResponse {
        auto token = req.header("Authorization");
        if (!token) {
            return HttpResponse{}.status(401).body("Unauthorized");
        }
        // 验证 token...
        return next(req);  // 继续到下一个处理器
    };
};

auto cors = [](Handler next) -> Handler {
    return [next](const HttpRequest& req) -> HttpResponse {
        auto response = next(req);
        return std::move(response)
            .header("Access-Control-Allow-Origin", "*")
            .header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    };
};

// 组合中间件
Handler final_handler = [](const HttpRequest& req) {
    return HttpResponse{}.status(200).body("Protected resource");
};

Handler protected_route = compose({logging(), cors, auth}, final_handler);

// 使用
router = router.get("/api/protected", protected_route);
```

### 内置中间件

```cpp
// 日志中间件
Handler h = logging()(my_handler);
// 输出: /api/users -> 200

// 自定义日志
Handler h2 = logging([](const HttpRequest& req, const HttpResponse& res) {
    std::cout << req.request_line.method << " "
              << req.request_line.uri << " -> "
              << res.status_code << '\n';
})(my_handler);
```

## HTTP 响应构建

### Builder 模式

```cpp
HttpResponse response = HttpResponse{}
    .status(200)
    .header("Content-Type", "application/json")
    .header("X-Custom-Header", "value")
    .body(R"({"message": "success"})");

// 序列化为 HTTP/1.1 格式
std::string http_response = response.to_string();
// HTTP/1.1 200 OK\r\n
// Content-Type: application/json\r\n
// X-Custom-Header: value\r\n
// \r\n
// {"message": "success"}
```

### 状态码辅助方法

```cpp
// 常用状态码
response.status(200);  // OK
response.status(201);  // Created
response.status(400);  // Bad Request
response.status(404);  // Not Found
response.status(500);  // Internal Server Error

// 状态文本自动生成
response.status_text(200);  // "OK"
response.status_text(404);  // "Not Found"
```

## 路由匹配

### PathPattern

```cpp
#include "router/matcher.hpp"

// 创建路径模式
PathPattern pattern("/users/:id/posts/:post_id");

// 匹配路径
auto result = pattern.match("/users/123/posts/456");

if (result) {
    std::cout << (*result)["id"];       // "123"
    std::cout << (*result)["post_id"];  // "456"
}

// 不匹配
pattern.match("/users/123");           // std::nullopt
pattern.match("/users/123/comments");  // std::nullopt
```

### 支持的模式

| 模式 | 示例 | 匹配 | 参数 |
|------|------|------|------|
| 静态路径 | `/api/users` | `/api/users` | - |
| 路径参数 | `/users/:id` | `/users/123` | `id=123` |
| 多参数 | `/users/:id/posts/:pid` | `/users/1/posts/2` | `id=1, pid=2` |
| 通配符 | `/static/*path` | `/static/css/a.css` | `path=css/a.css` |

## 设计原则

### 不可变性

```cpp
// ❌ 错误：Router 没有修改方法
Router router;
router.add_route("/path", handler);  // 编译错误

// ✅ 正确：每次返回新实例
Router router;
router = router.route(Method::GET, "/path", handler);

// 或者链式调用
router = Router{}
    .get("/a", handler_a)
    .post("/b", handler_b)
    .put("/c", handler_c);
```

### 类型安全

```cpp
// Handler 类型强制约束
Handler h1 = [](const HttpRequest& req) -> HttpResponse {
    return HttpResponse{}.status(200);
};  // ✅ 正确

Handler h2 = [](int x) { return x + 1; };  // ❌ 编译错误

// [[nodiscard]] 防止忽略返回值
router.get("/path", handler);  // ⚠️ 编译警告：返回值被忽略
router = router.get("/path", handler);  // ✅ 正确
```

### 高阶函数

中间件是包装 Handler 的函数：

```cpp
// Middleware 签名
using Middleware = std::function<Handler(Handler)>;

// 实现示例
Middleware timing = [](Handler next) -> Handler {
    return [next](const HttpRequest& req) -> HttpResponse {
        auto start = std::chrono::steady_clock::now();
        auto response = next(req);
        auto end = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        ).count();

        std::cout << "Request took " << duration << "ms\n";
        return response;
    };
};
```

### 组合优于继承

使用函数组合而非类继承：

```cpp
// ❌ OOP 方式（我们不这样做）
class BaseMiddleware {
    virtual HttpResponse handle(HttpRequest) = 0;
};
class LoggingMiddleware : public BaseMiddleware { ... };
class AuthMiddleware : public BaseMiddleware { ... };

// ✅ 函数式方式（我们的做法）
Handler final_handler = compose({
    logging(),
    auth,
    rate_limit,
    cache
}, base_handler);
```

## 实现细节

### 不可变数据结构

```cpp
class Router {
    // 使用 shared_ptr<const T> 实现持久化数据结构
    std::shared_ptr<const RouteTable> routes_;

public:
    Router route(Method m, std::string_view p, Handler h) const {
        // 复制路由表（写时复制）
        auto new_routes = std::make_shared<RouteTable>(*routes_);
        new_routes->emplace_back(Route{m, PathPattern(p), std::move(h)});
        return Router{new_routes};  // 返回新实例
    }
};
```

### 中间件组合

```cpp
// 从右到左组合中间件
inline Handler compose(std::vector<Middleware> middlewares,
                       Handler final_handler) {
    return std::accumulate(
        middlewares.rbegin(),
        middlewares.rend(),
        std::move(final_handler),
        [](Handler h, const Middleware& m) {
            return m(std::move(h));
        }
    );
}

// 执行顺序：
// compose({A, B, C}, handler)
// → C(B(A(handler)))
// → 请求流: A → B → C → handler → C → B → A
```

### 路径匹配算法

```cpp
// PathPattern 构造时将模式编译为正则表达式
PathPattern::PathPattern(std::string_view pattern) {
    std::string regex_pattern = "^";

    for (size_t i = 0; i < pattern.size(); ++i) {
        const char c = pattern[i];

        if (c == ':') {
            // 路径参数 :name → 捕获组 (?<name>[^/]+)
            auto end = pattern.find('/', i);
            auto name = pattern.substr(i + 1, end - i - 1);
            param_names_.push_back(std::string(name));
            regex_pattern += "([^/]+)";
            i = (end == std::string_view::npos) ? pattern.size() : end - 1;
        } else if (c == '*') {
            // 通配符 *name → 捕获组 (?<name>.*)
            auto name = pattern.substr(i + 1);
            param_names_.push_back(std::string(name));
            regex_pattern += "(.*)";
            break;
        } else {
            regex_pattern += c;
        }
    }

    regex_pattern += "$";
    regex_ = std::regex(regex_pattern);
}
```

## 性能考虑

### 路由查找复杂度

- 线性搜索：O(n) 其中 n = 路由数量
- 对于小型应用（< 100 路由）性能足够
- 生产环境可优化为前缀树（Trie）或基数树（Radix Tree）

### 不可变性开销

```cpp
// 每次添加路由都复制整个路由表
router = router.get("/path", handler);  // O(n) 复制

// 优化：批量构建
Router router = Router{}
    .get("/a", h1)
    .get("/b", h2)
    .get("/c", h3);  // 3 次复制

// 更好的做法：一次构建后不再修改
static const Router router = build_routes();  // 只构建一次
```

### 中间件性能

```cpp
// 中间件按注册顺序执行
Handler h = compose({
    logging(),      // 第 1 层包装
    cors,           // 第 2 层包装
    auth,           // 第 3 层包装
    rate_limit      // 第 4 层包装
}, final_handler);

// 每次请求都经过所有中间件层
// 如果某个中间件拒绝请求（如 auth 失败），后续中间件不执行
```

## 使用示例

### 完整的 REST API

```cpp
Router build_api_router() {
    return Router{}
        // 用户管理
        .get("/api/users", [](const HttpRequest& req) {
            // 返回所有用户
            return HttpResponse{}.status(200).body("[]");
        })
        .post("/api/users", [](const HttpRequest& req) {
            // 创建用户
            return HttpResponse{}.status(201);
        })
        .get("/api/users/:id", [](const HttpRequest& req) {
            // 获取单个用户（需要从 RouteMatch 提取 id）
            return HttpResponse{}.status(200);
        })
        .put("/api/users/:id", [](const HttpRequest& req) {
            // 更新用户
            return HttpResponse{}.status(200);
        })
        .delete_("/api/users/:id", [](const HttpRequest& req) {
            // 删除用户
            return HttpResponse{}.status(204);
        })

        // 静态文件
        .get("/static/*path", [](const HttpRequest& req) {
            // 服务静态文件
            return HttpResponse{}.status(200);
        });
}
```

### 带中间件的应用

```cpp
#include "router/router.hpp"
#include "router/middleware.hpp"

Router build_app() {
    using namespace http::router;
    using namespace http::router::middleware;

    // 公共路由（无需认证）
    Handler public_handler = compose(
        {logging()},
        [](const HttpRequest& req) {
            return HttpResponse{}.status(200).body("Public");
        }
    );

    // 私有路由（需要认证）
    auto auth_middleware = [](Handler next) -> Handler {
        return [next](const HttpRequest& req) -> HttpResponse {
            if (!req.header("Authorization")) {
                return HttpResponse{}.status(401);
            }
            return next(req);
        };
    };

    Handler private_handler = compose(
        {logging(), auth_middleware},
        [](const HttpRequest& req) {
            return HttpResponse{}.status(200).body("Private");
        }
    );

    return Router{}
        .get("/", public_handler)
        .get("/api/protected", private_handler);
}
```

## 局限性

### 当前不支持

- ❌ 异步 Handler（AsyncHandler 类型已定义但未实现）
- ❌ WebSocket 升级
- ❌ HTTP/2 服务器推送
- ❌ 路由优先级排序
- ❌ 路由组（分组路由）

### 可改进项

- 路由查找算法（从 O(n) 到 O(log n)）
- 写时复制优化（copy-on-write）
- 路径参数类型验证（当前都是字符串）

## 测试

```bash
# 运行示例
./build/router_example

# 输出：
# [0]/ -> 200 Status: 200
```

## 参考资料

- **不可变数据结构**: 持久化数据结构的函数式实现
- **Middleware Pattern**: 源自 Node.js Express / Koa
- **Builder Pattern**: Fluent API 设计
- **Higher-Order Functions**: 函数式编程核心概念
