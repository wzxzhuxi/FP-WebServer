# HTTP 解析器 - Parser Combinator 库

基于 C++23 的类型安全、函数式 HTTP/1.1 请求解析器，使用 parser combinator 技术构建。

## 架构

这个解析器基于 **parser combinators（解析器组合子）** - 一种函数式编程方法，通过组合简单、可复用的构建块来创建复杂的解析器。

### 核心概念

```cpp
// Parser<T> 是一个函数，消费输入并返回：
// - 成功：(解析值, 剩余输入)
// - 失败：ParseError
using Parser<T> = std::function<
    std::expected<std::pair<T, std::string_view>, ParseError>(std::string_view)
>;
```

### 文件结构

- **`types.hpp`**: HTTP 领域类型（Method, HttpRequest, Headers, ParseError）
- **`combinaor.hpp`**: 核心组合子原语（sequence, choice, many, map）
- **`http_parser.hpp`**: 通过组合原语构建的 HTTP 专用解析器

## 使用方法

### 解析完整的 HTTP 请求

```cpp
#include "parser/http_parser.hpp"

std::string_view raw_request =
    "GET /api/users/123 HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Authorization: Bearer token123\r\n"
    "\r\n";

auto result = http::parser::parse_http_request(raw_request);

if (result) {
    auto [request, remaining] = *result;

    // 访问解析后的数据
    std::cout << request.request_line.method;  // Method::GET
    std::cout << request.request_line.uri;     // "/api/users/123"
    std::cout << request.request_line.version; // Version::HTTP_1_1

    auto host = request.header("Host");        // "example.com"
    auto length = request.content_length();    // std::nullopt (无 body)
} else {
    std::cerr << "解析错误: " << static_cast<int>(result.error()) << '\n';
}
```

### 使用单独的解析器

```cpp
#include "parser/http_parser.hpp"

// 解析 HTTP 方法
auto method_result = http::parser::parse_method()("GET /");
// → std::expected<std::pair<Method::GET, std::string_view{" /"}>, ParseError>

// 解析请求行
auto line_result = http::parser::parse_request_line()(
    "POST /submit HTTP/1.1\r\n"
);
// → RequestLine{Method::POST, "/submit", Version::HTTP_1_1}

// 解析请求头
auto headers_result = http::parser::parse_headers()(
    "Content-Type: application/json\r\n"
    "Content-Length: 42\r\n"
    "\r\n"
);
// → std::unordered_map<std::string, std::string>
```

## 组合子原语

### 基础解析器

```cpp
// 解析单个字符
auto p = combinator::one_char();
p("hello") // → ('h', "ello")

// 解析满足谓词的字符
auto digit = combinator::satisfy([](unsigned char c) {
    return std::isdigit(c);
});
digit("42") // → ('4', "2")

// 解析精确字符串
auto get = combinator::string("GET");
get("GET /") // → ("GET", " /")
```

### 组合操作

```cpp
// 按顺序尝试解析器（第一个成功的获胜）
auto method = combinator::choice<Method>({
    parse_get(),
    parse_post(),
    parse_put()
});

// 解析零个或多个匹配
auto headers = combinator::many(parse_header());

// 转换解析结果
auto to_upper = combinator::map(
    combinator::string("get"),
    [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }
);
to_upper("get") // → ("GET", "")
```

### 工具函数

```cpp
// 跳过空白字符
auto ws = combinator::spaces();
ws("   hello") // → (monostate, "hello")

// 读取直到分隔符
auto until_crlf = combinator::take_until("\r\n");
until_crlf("data\r\nmore") // → ("data", "\r\nmore")
```

## HTTP 类型

### Method（HTTP 方法）

```cpp
enum class Method {
    GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, CONNECT, TRACE
};
```

### Version（HTTP 版本）

```cpp
enum class Version {
    HTTP_1_0,  // "HTTP/1.0"
    HTTP_1_1   // "HTTP/1.1"
};
```

### HttpRequest（HTTP 请求）

```cpp
struct HttpRequest {
    RequestLine request_line;
    std::unordered_map<std::string, std::string> headers;
    std::vector<uint8_t> body;

    // 辅助方法
    std::optional<std::string_view> header(std::string_view key) const;
    std::optional<size_t> content_length() const;
};
```

### 错误处理

```cpp
enum class ParseError {
    UnexpectedEof,    // 意外的输入结束
    InvalidMethod,    // 无效的 HTTP 方法
    InvalidUri,       // 无效的 URI
    InvalidVersion,   // 无效的 HTTP 版本
    InvalidHeader,    // 无效的请求头
    ExpectedCrlf      // 期望 CRLF 但未找到
};

// 所有解析器都返回 std::expected<T, ParseError>
auto result = parse_http_request(input);
if (!result) {
    switch (result.error()) {
        case ParseError::InvalidMethod:
            // 处理错误
            break;
    }
}
```

## 设计原则

### 函数式组合

复杂的解析器通过组合简单解析器构建：

```cpp
// parse_request_line() 由以下部分构建：
parse_method()        // "GET"
  → 跳过空格          // " "
  → parse_uri()       // "/path"
  → 跳过空格          // " "
  → parse_version()   // "HTTP/1.1"
  → 期望 "\r\n"
```

### 不可变性

- 所有解析器都是纯函数（相同输入 → 相同输出）
- 输入永不改变（使用 `std::string_view`）
- 解析器返回新状态而不是修改现有状态

### 类型安全

```cpp
// 编译期类型检查
Parser<Method> method_parser;        // 只能返回 Method
Parser<std::string> uri_parser;      // 只能返回 std::string

// 无异常 - 错误即值
std::expected<HttpRequest, ParseError> result = parse_http_request(input);
```

### 零拷贝解析

尽可能使用 `std::string_view` 避免内存分配：

```cpp
// 请求头存储对原始输入的引用（当安全时）
std::optional<std::string_view> header(std::string_view key) const;
```

## 实现细节

### 安全性

所有 `<cctype>` 函数都安全地转换为 `unsigned char`：

```cpp
// ❌ 错误 - 当 char 为负数时有未定义行为
std::isspace(c)

// ✅ 正确 - 始终安全
[](unsigned char c) { return std::isspace(c); }
```

### 性能

- 使用 `std::find_if` / `std::find_if_not` 而非手动循环
- 尽可能零拷贝（string_view）
- 无递归（对大输入栈安全）

### 务实的函数式编程

- 高层 API：纯函数、不可变数据
- 底层实现：标准库算法（内部可能使用循环）
- C++ 无尾递归优化 → 循环对生产环境更安全

## 局限性

当前解析器处理**基础 HTTP/1.1** 但不支持：

- ❌ `Transfer-Encoding: chunked`（分块传输编码）
- ❌ `Connection: keep-alive` 验证
- ❌ 管道化请求（Pipelined requests）
- ❌ Content-Length body 验证（解析请求头但不验证 body 长度）

对于简单的 web 服务器，这些局限是可接受的。生产级服务器需要这些特性。

## 测试

```bash
# 运行解析器测试
./build/server_test --gtest_filter=*ParserTest*

# 所有 39 个 HTTP 解析器测试通过
# 所有 28 个组合子测试通过
```

## 示例：构建自定义解析器

```cpp
#include "parser/combinaor.hpp"

// 解析电子邮件地址
combinator::Parser<std::string> parse_email() {
    return [](std::string_view input)
        -> std::expected<std::pair<std::string, std::string_view>, ParseError> {

        auto at_pos = input.find('@');
        if (at_pos == std::string_view::npos) {
            return std::unexpected(ParseError::InvalidUri);
        }

        auto local = input.substr(0, at_pos);
        auto domain_start = at_pos + 1;
        auto space_pos = input.find(' ', domain_start);
        auto domain = input.substr(domain_start, space_pos - domain_start);

        std::string email = std::string(local) + "@" + std::string(domain);
        return std::pair{email, input.substr(space_pos)};
    };
}

// 使用
auto result = parse_email()("user@example.com next");
// → ("user@example.com", " next")
```

## 参考资料

- **Parser Combinators**: 源自 Haskell Parsec 的函数式解析技术
- **std::expected**: C++23 单子式错误处理（类似 Rust 的 Result<T, E>）
- **HTTP/1.1 RFC**: [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html)
