#pragma once
#include "combinaor.hpp"
#include "types.hpp"
#include <cctype>

namespace http::parser {

  // using namespace combinaor;

  inline combinator::Parser<http::parser::Method> parse_method() {
    return combinator::choice<http::parser::Method>(
        {combinator::map(combinator::string("GET"), [](auto) { return http::parser::Method::Get; }),
         combinator::map(combinator::string("POST"), [](auto) { return http::parser::Method::Post; }),
         combinator::map(combinator::string("HEAD"), [](auto) { return http::parser::Method::Head; }),
         combinator::map(combinator::string("PUT"), [](auto) { return http::parser::Method::Put; }),
         combinator::map(combinator::string("DELETE"),
             [](auto) { return http::parser::Method::Delete; }),
         combinator::map(combinator::string("OPTIONS"),
             [](auto) { return http::parser::Method::Options; }),
         combinator::map(combinator::string("TRACE"), [](auto) { return http::parser::Method::Trace; }),
         combinator::map(combinator::string("CONNECT"),
             [](auto) { return http::parser::Method::Connect; }),
         combinator::map(combinator::string("PATCH"),
             [](auto) { return http::parser::Method::Patch; })});
  }

  inline combinator::Parser<std::string> parse_uri() {
    return [](std::string_view input)
               -> std::expected<std::pair<std::string, std::string_view>,
                                http::parser::ParseError> {
      auto it = std::find_if(input.begin(), input.end(),
                            [](unsigned char c) { return std::isspace(c); });
      auto count = std::distance(input.begin(), it);

      if (count == 0) {
        return std::unexpected(http::parser::ParseError::InvalidUri);
      }

      auto uri = std::string(input.substr(0, count));
      return std::pair{std::move(uri), input.substr(count)};
    };
  }

  inline combinator::Parser<http::parser::Version> parse_version() {
    return combinator::choice<http::parser::Version>(
        {combinator::map(combinator::string("HTTP/1.0"), [](auto) { return http::parser::Version::Http10; }),
         combinator::map(combinator::string("HTTP/1.1"), [](auto) { return http::parser::Version::Http11; })});
  }

  inline combinator::Parser<std::monostate> crlf() {
    return combinator::map(combinator::string("\r\n"), [](auto) { return std::monostate{}; });
  }

  inline combinator::Parser<std::monostate> sp() {
    return combinator::map(combinator::string(" "), [](auto) { return std::monostate{}; });
  }

  inline combinator::Parser<http::parser::RequestLine> parse_request_line() {
    return [](std::string_view input)
               -> std::expected<
                   std::pair<http::parser::RequestLine, std::string_view>,
                   http::parser::ParseError> {
      auto method_result = parse_method()(input);
      if (!method_result) {
        return std::unexpected(method_result.error());
      }
      auto [method, rest1] = *method_result;

      auto sp1_result = sp()(rest1);
      if (!sp1_result) {
        return std::unexpected(sp1_result.error());
      }
      auto [sp1, rest2] = *sp1_result;

      auto uri_result = parse_uri()(rest2);
      if (!uri_result) {
        return std::unexpected(uri_result.error());
      }
      auto [uri, rest3] = *uri_result;

      auto sp2_result = sp()(rest3);
      if (!sp2_result) {
        return std::unexpected(sp2_result.error());
      }
      auto [sp2, rest4] = *sp2_result;

      auto version_result = parse_version()(rest4);
      if (!version_result) {
        return std::unexpected(version_result.error());
      }
      auto [version, rest5] = *version_result;

      auto crlf_result = crlf()(rest5);
      if (!crlf_result) {
        return std::unexpected(crlf_result.error());
      }
      auto [crlf, rest6] = *crlf_result;

      return std::pair{http::parser::RequestLine{method, uri, version}, rest6};
    };
  }

  inline combinator::Parser<std::pair<std::string, std::string>> parse_header() {
    return
        [](std::string_view input)
            -> std::expected<std::pair<std::pair<std::string, std::string>, std::string_view>,
                             http::parser::ParseError> {
          auto colon_pos = input.find(':');
          if (colon_pos == std::string_view::npos) {
            return std::unexpected(http::parser::ParseError::InvalidHeader);
          }
          auto key = std::string(input.substr(0, colon_pos));
          auto rest = input.substr(colon_pos + 1);
          auto it = std::find_if_not(rest.begin(), rest.end(),
                                     [](unsigned char c) { return std::isspace(c) && c != '\r'; });
          auto skipped = std::distance(rest.begin(), it);
          rest = rest.substr(skipped);
          auto crlf_pos = rest.find("\r\n");
          if (crlf_pos == std::string_view::npos) {
            return std::unexpected(http::parser::ParseError::InvalidHeader);
          }
          auto value = std::string(rest.substr(0, crlf_pos));
          rest = rest.substr(crlf_pos + 2);

          return std::pair{std::pair{std::move(key), std::move(value)}, rest};
        };
  }

  inline combinator::Parser<http::parser::Headers> parse_headers() {
    return [](std::string_view input)
               -> std::expected<std::pair<Headers, std::string_view>,
                                http::parser::ParseError> {
      Headers headers;
      auto current_input = input;

      while (true) {
        if (current_input.starts_with("\r\n")) {
          return std::pair{std::move(headers), current_input.substr(2)};
        }
        auto header_result = parse_header()(current_input);
        if (!header_result) {
          return std::unexpected(header_result.error());
        }
        auto [header_pair, rest] = *header_result;
        headers.insert(std::move(header_pair));
        current_input = rest;
      }
    };
  }

  inline ParseResult<http::parser::HttpRequest> parse_http_request(std::string_view input) {
    auto request_line_result = parse_request_line()(input);
    if (!request_line_result) {
      return std::unexpected(request_line_result.error());
    }
    auto [request_line, rest1] = *request_line_result;

    auto headers_result = parse_headers()(rest1);
    if (!headers_result) {
      return std::unexpected(headers_result.error());
    }
    auto [headers, rest2] = *headers_result;

    std::vector<uint8_t> body(rest2.begin(), rest2.end());

    return HttpRequest{std::move(request_line), std::move(headers),
                       std::move(body)};
  }

} // namespace http::parser
