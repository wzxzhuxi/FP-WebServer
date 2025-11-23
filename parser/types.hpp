#pragma once
#include <algorithm>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace http::parser {

enum class Method {
  Get,
  Post,
  Head,
  Put,
  Delete,
  Options,
  Trace,
  Connect,
  Patch
};

enum class Version {
  Http10,
  Http11,
};

struct RequestLine {
  Method method;
  std::string uri;
  Version version;
};

using Headers = std::unordered_map<std::string, std::string>;

struct HttpRequest {
  RequestLine request_line;
  Headers headers;
  std::vector<uint8_t> body;

  std::optional<std::string_view> header(std::string_view key) const {
    auto it = headers.find(std::string(key));
    return (it != headers.end()) ? std::optional<std::string_view>{it->second}
                                 : std::nullopt;
  }

  size_t content_length() const {
    auto is_valid_number = [](std::string_view s) -> bool {
      return !s.empty() && s[0] != '-' &&
             std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
    };

    return header("Content-Length")
        .transform([](auto v) { return std::string(v); })
        .and_then([&](const std::string& s) -> std::optional<size_t> {
          if (!is_valid_number(s)) {
            return std::nullopt;
          }
          try {
            return std::stoul(s);
          } catch (...) {
            return std::nullopt;
          }
        })
        .value_or(0);
  }
};

enum class ParseError {
  InvalidMethod,
  InvalidUri,
  InvalidVersion,
  InvalidHeader,
  IncompleteRequest,
  MalformedRequest
};

template <typename T> using ParseResult = std::expected<T, ParseError>;

} // namespace http::parser
