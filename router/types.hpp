#pragma once
#include "../parser/types.hpp"
#include <expected>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace http::router {

struct HttpResponse {
  int status_code;
  std::string status_text;
  std::unordered_map<std::string, std::string> headers;
  std::vector<uint8_t> body;

  static HttpResponse ok() { return {200, "OK", {}, {}}; }

  static HttpResponse not_found() { return {404, "Not Found", {}, {}}; }

  static HttpResponse bad_request() { return {400, "Bad Request", {}, {}}; }

  static HttpResponse internal_server_error() {
    return {500, "Internal Server Error", {}, {}};
  }

  HttpResponse with_header(std::string key, std::string value) && {
    headers.insert({std::move(key), std::move(value)});
    return std::move(*this);
  }

  HttpResponse with_body(std::vector<uint8_t> body) && {
    this->body = std::move(body);
    return std::move(*this);
  }

  HttpResponse with_text(std::string text) && {
    body = std::vector<uint8_t>(text.begin(), text.end());
    headers["Content-Type"] = "text/plain";
    return std::move(*this);
  }

  HttpResponse with_html(std::string html) && {
    body = std::vector<uint8_t>(html.begin(), html.end());
    headers["Content-Type"] = "text/html";
    return std::move(*this);
  }

  HttpResponse with_json(std::string json) && {
    body = std::vector<uint8_t>(json.begin(), json.end());
    headers["Content-Type"] = "application/json";
    return std::move(*this);
  }
};

using Handler = std::function<HttpResponse(const parser::HttpRequest &)>;

using AsyncHandler = std::function<std::future<HttpResponse>(const parser::HttpRequest &)>;

enum class RouterError { NotFound, MethodNotAllowed, InternalError };

struct RouteMatch {
  Handler handler;
  std::unordered_map<std::string, std::string> params;
};

} // namespace http::router
