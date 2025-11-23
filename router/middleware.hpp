#pragma once
#include "types.hpp"
#include <functional>
#include <iostream>
#include <numeric>

namespace http::router::middleware {

  using Middleware = std::function<Handler(Handler)>;

  inline Middleware logging() {
    return [](Handler next) -> Handler {
      return [next = std::move(next)](const parser::HttpRequest &req) -> HttpResponse {
        std::cout << "[" << static_cast<int>(req.request_line.method) << "]"
                  << req.request_line.uri << '\n';

        auto response = next(req);

        std::cout << " -> " << response.status_code << '\n';

        return response;
      };
    };
  }

  inline Middleware require_auth(std::function<bool(const parser::HttpRequest &)> check) {
    return [check = std::move(check)](Handler next) -> Handler {
      return
          [next = std::move(next), check](const parser::HttpRequest &req) -> HttpResponse {
            if (!check(req)) {
              return HttpResponse{401, "Unauthorized", {}, {}}.with_text(
                  "Authentication required");
            }
            return next(req);
          };
    };
  }

  inline Middleware cors() {
    return [](Handler next) -> Handler {
      return [next = std::move(next)](const parser::HttpRequest &req) -> HttpResponse {
        auto response = next(req);

        return std::move(response)
            .with_header("Access-Control-Allow-Origin", "*")
            .with_header("Access-Control-Allow-Methods",
                         "GET, POST, PUT, DELETE");
      };
    };
  }

  inline Handler compose(std::vector<Middleware> middlewares,
                         Handler final_handler) {
    return std::accumulate(
      middlewares.rbegin(),
      middlewares.rend(),
      std::move(final_handler),
      [](Handler h, const Middleware& m) { return m(std::move(h)); }
    );
  }

} // namespace http::router::middleware
