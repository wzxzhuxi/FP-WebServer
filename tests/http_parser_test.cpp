#include "http_parser.hpp"
#include "types.hpp"
#include <gtest/gtest.h>

using namespace http::parser;

// Test parse_method
TEST(HttpParserTest, ParseMethod_GET) {
  auto parser = parse_method();
  auto result = parser("GET /path");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, Method::Get);
  EXPECT_EQ(result->second, " /path");
}

TEST(HttpParserTest, ParseMethod_POST) {
  auto parser = parse_method();
  auto result = parser("POST /api");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, Method::Post);
}

TEST(HttpParserTest, ParseMethod_AllMethods) {
  const std::vector<std::pair<std::string, Method>> test_cases = {
    {"GET", Method::Get},
    {"POST", Method::Post},
    {"HEAD", Method::Head},
    {"PUT", Method::Put},
    {"DELETE", Method::Delete},
    {"OPTIONS", Method::Options},
    {"TRACE", Method::Trace},
    {"CONNECT", Method::Connect},
    {"PATCH", Method::Patch},
  };

  for (const auto& [method_str, expected] : test_cases) {
    auto result = parse_method()(method_str + " /");
    ASSERT_TRUE(result.has_value()) << "Failed to parse: " << method_str;
    EXPECT_EQ(result->first, expected);
  }
}

TEST(HttpParserTest, ParseMethod_Invalid) {
  auto parser = parse_method();
  auto result = parser("INVALID /path");

  ASSERT_FALSE(result.has_value());
}

TEST(HttpParserTest, ParseMethod_LowerCase) {
  auto parser = parse_method();
  auto result = parser("get /path");

  ASSERT_FALSE(result.has_value());
}

TEST(HttpParserTest, ParseMethod_Partial) {
  auto parser = parse_method();
  auto result = parser("GE /path");

  ASSERT_FALSE(result.has_value());
}

// Test parse_version
TEST(HttpParserTest, ParseVersion_HTTP10) {
  auto parser = parse_version();
  auto result = parser("HTTP/1.0\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, Version::Http10);
  EXPECT_EQ(result->second, "\r\n");
}

TEST(HttpParserTest, ParseVersion_HTTP11) {
  auto parser = parse_version();
  auto result = parser("HTTP/1.1\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, Version::Http11);
}

TEST(HttpParserTest, ParseVersion_Invalid) {
  auto parser = parse_version();

  EXPECT_FALSE(parse_version()("HTTP/2.0").has_value());
  EXPECT_FALSE(parse_version()("http/1.1").has_value());
  EXPECT_FALSE(parse_version()("HTTP/1").has_value());
}

// Test parse_request_line
TEST(HttpParserTest, ParseRequestLine_Simple) {
  auto parser = parse_request_line();
  auto result = parser("GET /index.html HTTP/1.1\r\n");

  ASSERT_TRUE(result.has_value());
  auto [request_line, rest] = *result;

  EXPECT_EQ(request_line.method, Method::Get);
  EXPECT_EQ(request_line.uri, "/index.html");
  EXPECT_EQ(request_line.version, Version::Http11);
  EXPECT_EQ(rest, "");
}

TEST(HttpParserTest, ParseRequestLine_POST) {
  auto result = parse_request_line()("POST /api/users HTTP/1.0\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.method, Method::Post);
  EXPECT_EQ(result->first.uri, "/api/users");
  EXPECT_EQ(result->first.version, Version::Http10);
}

TEST(HttpParserTest, ParseRequestLine_RootPath) {
  auto result = parse_request_line()("GET / HTTP/1.1\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.uri, "/");
}

TEST(HttpParserTest, ParseRequestLine_MissingCRLF) {
  auto result = parse_request_line()("GET /path HTTP/1.1");

  ASSERT_FALSE(result.has_value());
}

TEST(HttpParserTest, ParseRequestLine_ExtraSpaces) {
  auto result = parse_request_line()("GET  /path HTTP/1.1\r\n");

  ASSERT_FALSE(result.has_value());
}

TEST(HttpParserTest, ParseRequestLine_MissingSpace) {
  auto result = parse_request_line()("GET/path HTTP/1.1\r\n");

  ASSERT_FALSE(result.has_value());
}

// Test parse_header
TEST(HttpParserTest, ParseHeader_Simple) {
  auto parser = parse_header();
  auto result = parser("Host: localhost\r\n");

  ASSERT_TRUE(result.has_value());
  auto [header, rest] = *result;

  EXPECT_EQ(header.first, "Host");
  EXPECT_EQ(header.second, "localhost");
  EXPECT_EQ(rest, "");
}

TEST(HttpParserTest, ParseHeader_MultipleSpaces) {
  auto result = parse_header()("Content-Type:   text/html\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.first, "Content-Type");
  EXPECT_EQ(result->first.second, "text/html");
}

TEST(HttpParserTest, ParseHeader_NoSpace) {
  auto result = parse_header()("Content-Length:42\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.first, "Content-Length");
  EXPECT_EQ(result->first.second, "42");
}

TEST(HttpParserTest, ParseHeader_MissingColon) {
  auto result = parse_header()("InvalidHeader\r\n");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ParseError::InvalidHeader);
}

TEST(HttpParserTest, ParseHeader_MissingCRLF) {
  auto result = parse_header()("Host: localhost");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ParseError::InvalidHeader);
}

TEST(HttpParserTest, ParseHeader_EmptyValue) {
  auto result = parse_header()("X-Custom:\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.first, "X-Custom");
  EXPECT_EQ(result->first.second, "");
}

// Test parse_headers
TEST(HttpParserTest, ParseHeaders_Single) {
  auto parser = parse_headers();
  auto result = parser("Host: localhost\r\n\r\n");

  ASSERT_TRUE(result.has_value());
  auto [headers, rest] = *result;

  EXPECT_EQ(headers.size(), 1);
  EXPECT_EQ(headers["Host"], "localhost");
  EXPECT_EQ(rest, "");
}

TEST(HttpParserTest, ParseHeaders_Multiple) {
  std::string input =
    "Host: example.com\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 1234\r\n"
    "\r\n";

  auto result = parse_headers()(input);

  ASSERT_TRUE(result.has_value());
  auto [headers, rest] = *result;

  EXPECT_EQ(headers.size(), 3);
  EXPECT_EQ(headers["Host"], "example.com");
  EXPECT_EQ(headers["Content-Type"], "text/html");
  EXPECT_EQ(headers["Content-Length"], "1234");
}

TEST(HttpParserTest, ParseHeaders_Empty) {
  auto result = parse_headers()("\r\n");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.size(), 0);
}

TEST(HttpParserTest, ParseHeaders_WithBody) {
  std::string input =
    "Content-Length: 11\r\n"
    "\r\n"
    "Hello World";

  auto result = parse_headers()(input);

  ASSERT_TRUE(result.has_value());
  auto [headers, rest] = *result;

  EXPECT_EQ(headers.size(), 1);
  EXPECT_EQ(headers["Content-Length"], "11");
  EXPECT_EQ(rest, "Hello World");
}

// Test HttpRequest helper methods
TEST(HttpRequestTest, Header_Found) {
  HttpRequest req;
  req.headers["Content-Type"] = "application/json";

  auto value = req.header("Content-Type");
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "application/json");
}

TEST(HttpRequestTest, Header_NotFound) {
  HttpRequest req;

  auto value = req.header("Missing-Header");
  EXPECT_FALSE(value.has_value());
}

TEST(HttpRequestTest, ContentLength_Valid) {
  HttpRequest req;
  req.headers["Content-Length"] = "42";

  EXPECT_EQ(req.content_length(), 42);
}

TEST(HttpRequestTest, ContentLength_Missing) {
  HttpRequest req;

  EXPECT_EQ(req.content_length(), 0);
}

TEST(HttpRequestTest, ContentLength_Invalid) {
  HttpRequest req;
  req.headers["Content-Length"] = "not-a-number";

  EXPECT_EQ(req.content_length(), 0);
}

TEST(HttpRequestTest, ContentLength_Negative) {
  HttpRequest req;
  req.headers["Content-Length"] = "-10";

  // stoul will throw on negative numbers
  EXPECT_EQ(req.content_length(), 0);
}

// Integration tests
TEST(HttpParserIntegrationTest, CompleteGETRequest) {
  std::string request =
    "GET /index.html HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: TestClient/1.0\r\n"
    "Accept: text/html\r\n"
    "\r\n";

  auto result = parse_http_request(request);

  ASSERT_TRUE(result.has_value());
  auto& req = *result;

  EXPECT_EQ(req.request_line.method, Method::Get);
  EXPECT_EQ(req.request_line.uri, "/index.html");
  EXPECT_EQ(req.request_line.version, Version::Http11);

  EXPECT_EQ(req.headers.size(), 3);
  EXPECT_EQ(req.headers["Host"], "www.example.com");
  EXPECT_EQ(req.headers["User-Agent"], "TestClient/1.0");
  EXPECT_EQ(req.headers["Accept"], "text/html");

  EXPECT_EQ(req.body.size(), 0);
}

TEST(HttpParserIntegrationTest, CompletePOSTRequest) {
  std::string request =
    "POST /api/data HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 18\r\n"
    "\r\n"
    "{\"key\":\"value\"}";

  auto result = parse_http_request(request);

  ASSERT_TRUE(result.has_value());
  auto& req = *result;

  EXPECT_EQ(req.request_line.method, Method::Post);
  EXPECT_EQ(req.request_line.uri, "/api/data");
  EXPECT_EQ(req.content_length(), 18);

  std::string body_str(req.body.begin(), req.body.end());
  EXPECT_EQ(body_str, "{\"key\":\"value\"}");
}

TEST(HttpParserIntegrationTest, MinimalRequest) {
  std::string request = "GET / HTTP/1.0\r\n\r\n";

  auto result = parse_http_request(request);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->request_line.method, Method::Get);
  EXPECT_EQ(result->request_line.uri, "/");
  EXPECT_EQ(result->headers.size(), 0);
}

TEST(HttpParserIntegrationTest, MalformedRequest_NoHeaders) {
  std::string request = "GET / HTTP/1.1";

  auto result = parse_http_request(request);

  ASSERT_FALSE(result.has_value());
}

TEST(HttpParserIntegrationTest, MalformedRequest_InvalidMethod) {
  std::string request = "INVALID / HTTP/1.1\r\n\r\n";

  auto result = parse_http_request(request);

  ASSERT_FALSE(result.has_value());
}

// Edge cases
TEST(HttpParserEdgeCaseTest, VeryLongURL) {
  std::string long_url(8000, 'a');
  std::string request = "GET /" + long_url + " HTTP/1.1\r\n\r\n";

  auto result = parse_http_request(request);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->request_line.uri.size(), long_url.size() + 1);
}

TEST(HttpParserEdgeCaseTest, ManyHeaders) {
  std::string request = "GET / HTTP/1.1\r\n";
  for (int i = 0; i < 100; ++i) {
    request += "Header" + std::to_string(i) + ": Value" + std::to_string(i) + "\r\n";
  }
  request += "\r\n";

  auto result = parse_http_request(request);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->headers.size(), 100);
}

TEST(HttpParserEdgeCaseTest, LargeBody) {
  std::string body(10000, 'X');
  std::string request =
    "POST /upload HTTP/1.1\r\n"
    "Content-Length: 10000\r\n"
    "\r\n" + body;

  auto result = parse_http_request(request);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->body.size(), 10000);
}
