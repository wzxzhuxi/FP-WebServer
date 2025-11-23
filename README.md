# Function TinyWebServer

基于 C++23 的函数式 HTTP 服务器实现，使用 parser combinators 和不可变数据结构构建。

## 项目状态

这是一个**正在开发中**的项目，当前实现了完整的 HTTP 请求解析器和路由系统，但**缺少网络层**（Socket I/O）。

### 已完成 ✅

- **HTTP/1.1 请求解析器**（基于 parser combinators）
- **函数式路由器**（完全不可变、类型安全）
- **中间件系统**（高阶函数组合）
- **路径匹配**（支持路径参数和通配符）
- **HTTP 响应构建器**（Builder 模式）

### 未完成 ❌

- **TCP 网络层**（Socket、Epoll、事件循环）
- **完整 HTTP/1.1 协议**（chunked 编码、keep-alive、pipeline）
- **异步 Handler**（AsyncHandler 类型已定义但未实现）

## 技术栈

- **语言**: C++23
- **编译器**: GCC 13+ / Clang 16+
- **构建系统**: CMake 3.20+
- **测试框架**: Google Test
- **标准库特性**: `std::expected`, `std::string_view`, `std::function`

## 架构

```
HTTP 请求处理流程：
原始字节流
    ↓
[01 Parser] → HttpRequest 结构体
    ↓
[02 Router] → 匹配 Handler
    ↓
[03 Execute] → HttpResponse 响应
    ↓
(❌ 缺少网络层发送响应)
```

### 组件

#### 1. Parser Combinator 库 (`parser/`)

函数式 HTTP/1.1 解析器，使用组合子技术构建。

```cpp
// 解析 HTTP 请求
auto result = http::parser::parse_http_request(
    "GET /api/users HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "\r\n"
);

if (result) {
    auto [request, remaining] = *result;
    std::cout << request.request_line.uri;  // "/api/users"
}
```

**特性**:
- ✅ 解析 HTTP 方法（GET, POST, PUT, DELETE 等）
- ✅ 解析 URI、HTTP 版本、请求头、请求体
- ✅ 零拷贝解析（`std::string_view`）
- ✅ 类型安全错误处理（`std::expected<T, ParseError>`）
- ✅ 组合子组合（sequence, choice, many, map）

详见 [parser/README.md](parser/README.md)

#### 2. 路由与中间件系统 (`router/`)

完全不可变的路由器，支持路径匹配和中间件组合。

```cpp
// 构建路由器
Router router = Router{}
    .get("/", [](const HttpRequest& req) {
        return HttpResponse{}.status(200).body("Hello");
    })
    .get("/users/:id", [](const HttpRequest& req) {
        return HttpResponse{}.status(200);
    });

// 匹配请求
auto match = router.find(request);
if (match) {
    HttpResponse response = match->handler(request);
    std::string id = match->params["id"];  // 路径参数
}
```

**特性**:
- ✅ 不可变路由器（每次添加路由返回新实例）
- ✅ 路径参数匹配（`/users/:id`）
- ✅ 通配符匹配（`/static/*path`）
- ✅ 中间件组合（高阶函数）
- ✅ Builder 模式响应构建
- ✅ 类型安全（`[[nodiscard]]` 防止忽略返回值）

详见 [router/README.md](router/README.md)

## 快速开始

### 编译

```bash
# 安装依赖（Arch Linux）
sudo pacman -S gcc cmake gtest

# 构建项目
mkdir build && cd build
cmake ..
make

# 运行测试
./server_test

# 运行示例
./router_example
```

### 最小示例

```cpp
#include "parser/http_parser.hpp"
#include "router/router.hpp"

int main() {
    using namespace http;

    // 构建路由器
    router::Router router = router::Router{}
        .get("/", [](const parser::HttpRequest& req) {
            return router::HttpResponse{}
                .status(200)
                .header("Content-Type", "text/plain")
                .body("Hello, World!");
        });

    // 解析请求
    std::string_view raw_request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    auto parse_result = parser::parse_http_request(raw_request);
    if (!parse_result) {
        std::cerr << "Parse error\n";
        return 1;
    }

    auto [request, _] = *parse_result;

    // 路由匹配
    auto match = router.find(request);
    if (!match) {
        std::cerr << "Route not found\n";
        return 1;
    }

    // 执行 Handler
    auto response = match->handler(request);

    // 序列化响应
    std::cout << response.to_string();
    // HTTP/1.1 200 OK
    // Content-Type: text/plain
    //
    // Hello, World!

    return 0;
}
```

### 中间件示例

```cpp
#include "router/middleware.hpp"

using namespace http::router;
using namespace http::router::middleware;

// 自定义认证中间件
auto auth_middleware = [](Handler next) -> Handler {
    return [next](const parser::HttpRequest& req) -> HttpResponse {
        auto token = req.header("Authorization");
        if (!token || *token != "Bearer secret") {
            return HttpResponse{}.status(401).body("Unauthorized");
        }
        return next(req);
    };
};

// 组合中间件
Handler protected_handler = compose(
    {logging(), auth_middleware},
    [](const parser::HttpRequest& req) {
        return HttpResponse{}.status(200).body("Protected data");
    }
);

Router router = Router{}.get("/api/protected", protected_handler);
```

## 设计原则

### 函数式编程

- **不可变性**: 所有数据结构默认不可变
- **纯函数**: Handler 是纯函数（相同输入 → 相同输出）
- **高阶函数**: 中间件是包装 Handler 的函数
- **函数组合**: 复杂行为由简单函数组合而成
- **类型安全**: 使用 `std::expected` 而非异常

### 务实主义

- **高层抽象**: API 层使用纯函数式风格
- **底层实现**: 内部可使用循环（避免栈溢出）
- **性能**: 标准库算法优于手写循环
- **安全**: `<cctype>` 函数安全转换为 `unsigned char`

### 代码质量

- ✅ 所有测试通过（67 个单元测试）
- ✅ clang-tidy 静态分析通过
- ✅ 无编译警告（`-Wall -Wextra -Werror`）
- ✅ 完整的命名空间限定（无 `using namespace`）
- ✅ C++23 标准

## 项目结构

```
function-tinywebserver/
├── parser/                  # HTTP 解析器
│   ├── types.hpp           # HTTP 领域类型
│   ├── combinaor.hpp       # 组合子库
│   ├── http_parser.hpp     # HTTP 解析器
│   └── README.md           # 解析器文档
├── router/                  # 路由系统
│   ├── types.hpp           # 响应类型和 Handler
│   ├── router.hpp          # 不可变路由器
│   ├── matcher.hpp         # 路径匹配
│   ├── middleware.hpp      # 中间件
│   └── README.md           # 路由器文档
├── http/                    # (未实现) 网络层
│   ├── http_cnn.hpp        # (空文件)
│   └── http_cnn.cpp        # (空文件)
├── test/                    # 测试
│   └── test.cpp            # 67 个单元测试
├── example/                 # 示例
│   └── router_usage.cpp    # 路由使用示例
├── main.cpp                 # 简单解析测试
├── CMakeLists.txt          # 构建配置
└── README.md               # 本文件
```

## 路线图

### 阶段 1: 核心协议 ✅ (已完成)

- [x] Parser combinator 库
- [x] HTTP/1.1 请求解析
- [x] 路由器和路径匹配
- [x] 中间件系统
- [x] HTTP 响应构建

### 阶段 2: 网络层 (进行中)

- [ ] TCP Socket 封装
- [ ] Epoll 事件循环
- [ ] 连接管理
- [ ] 请求/响应收发

### 阶段 3: 完整 HTTP/1.1 (计划中)

- [ ] `Transfer-Encoding: chunked`
- [ ] `Connection: keep-alive`
- [ ] HTTP 管道化（Pipelining）
- [ ] `Content-Length` 验证

### 阶段 4: 高级特性 (未来)

- [ ] 异步 Handler
- [ ] 线程池
- [ ] 静态文件服务
- [ ] 日志系统
- [ ] 性能优化（路由查找）

## 性能

### 当前性能

- **解析速度**: 未测试（需要网络层才能基准测试）
- **路由查找**: O(n) 线性搜索（n = 路由数量）
- **内存**: 零拷贝解析，最小化分配

### 优化方向

- 路由查找: 前缀树（Trie）或基数树（Radix Tree）
- 解析器: SIMD 加速字符串匹配
- 中间件: 编译期组合消除运行时开销

## 局限性

### 功能局限

- ❌ 无网络层（无法运行完整服务器）
- ❌ 不支持 HTTP/2, HTTP/3
- ❌ 不支持 WebSocket
- ❌ 不支持 HTTPS（无 TLS）

### 协议局限

- ❌ 不支持分块传输编码
- ❌ 不验证 Content-Length
- ❌ 不处理持久连接

### 性能局限

- 单线程（可扩展为线程池）
- 路由查找 O(n)（可优化为树结构）
- 同步 I/O（可扩展为异步）

## 测试

```bash
# 运行所有测试
./build/server_test

# [==========] Running 67 tests from 4 test suites.
# [----------] 28 tests from CombinatorTest
# [----------] 39 tests from HttpParserTest
# [----------] All tests passed.
```

### 测试覆盖

- ✅ 28 个组合子测试
- ✅ 39 个 HTTP 解析器测试
- ❌ 路由器测试（计划添加）
- ❌ 中间件测试（计划添加）

## 贡献

### 代码风格

- 完整命名空间限定（禁止 `using namespace`）
- 函数式优先（不可变、纯函数）
- 使用标准库算法（`std::find_if`, `std::accumulate`）
- `const` 默认，`mutable` 需理由
- 所有 Handler 返回值必须使用（`[[nodiscard]]`）

### 提交前检查

```bash
# 编译通过
cmake --build build

# 测试通过
./build/server_test

# 静态分析
clang-tidy parser/*.hpp router/*.hpp

# 格式化
clang-format -i **/*.hpp **/*.cpp
```

## 许可证

MIT License

## 参考资料

### Parser Combinators

- [Monadic Parser Combinators](https://www.cs.nott.ac.uk/~pszgmh/pearl.pdf) - Graham Hutton
- [Parsec](https://hackage.haskell.org/package/parsec) - Haskell 解析器库

### HTTP 协议

- [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) - HTTP Semantics
- [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) - HTTP/1.1

### C++23

- [std::expected](https://en.cppreference.com/w/cpp/utility/expected)
- [std::string_view](https://en.cppreference.com/w/cpp/string/basic_string_view)

### 函数式编程

- [Functional Programming in C++](https://www.manning.com/books/functional-programming-in-c-plus-plus) - Ivan Čukić
- [Immutable Data Structures](https://en.wikipedia.org/wiki/Persistent_data_structure)

## 作者

Function TinyWebServer - 一个函数式编程实验项目

**"Talk is cheap. Show me the code."**
