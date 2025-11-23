#pragma once
#include "matcher.hpp"
#include "types.hpp"
#include <map>
#include <memory>

namespace http::router {
class Router {
  using RouteKey = std::pair<parser::Method, std::string>;
  using RouteTable = std::map<RouteKey, std::pair<PathPattern, Handler>>;

  std::shared_ptr<const RouteTable> routes_;

public:
  Router() : routes_(std::make_shared<RouteTable>()) {}

  [[nodiscard]] Router route(parser::Method method, std::string_view pattern,
                             Handler handler) const {
    auto new_routes = std::make_shared<RouteTable>(*routes_);

    RouteKey key{method, std::string(pattern)};
    (*new_routes)[key] = {PathPattern(pattern), std::move(handler)};

    Router new_router;
    new_router.routes_ = new_routes;
    return new_router;
  }

  [[nodiscard]] Router get(std::string_view pattern, Handler handler) const {
    return route(parser::Method::Get, pattern, std::move(handler));
  }

  [[nodiscard]] Router post(std::string_view pattern, Handler handler) const {
    return route(parser::Method::Post, pattern, std::move(handler));
  }

  [[nodiscard]] Router put(std::string_view pattern, Handler handler) const {
    return route(parser::Method::Put, pattern, std::move(handler));
  }

  [[nodiscard]] Router delete_(std::string_view pattern, Handler handler) const {
    return route(parser::Method::Delete, pattern, std::move(handler));
  }

  [[nodiscard]] std::optional<RouteMatch> find(const parser::HttpRequest &req) const {
    for (const auto &[key, value] : *routes_) {
      const auto &[method, _] = key;
      const auto &[pattern, handler] = value;

      if (method != req.request_line.method) {
        continue;
      }

      if (auto params = pattern.match(req.request_line.uri)) {
        return RouteMatch{handler, *params};
      }
    }
    return std::nullopt;
  }

  HttpResponse handle(const parser::HttpRequest &req) const {
    auto match = find(req);

    if (!match) {
      return HttpResponse::not_found().with_text("Route not found");
    }
    try {
      return match->handler(req);
    } catch (const std::exception &e) {
      return HttpResponse::internal_server_error().with_text(
          std::string("Handler error: ") + e.what());
    }
  }
};
} // namespace http::router
