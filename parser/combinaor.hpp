#pragma once
#include "types.hpp"
#include <expected>
#include <functional>
#include <string_view>
#include <variant>

namespace http::parser::combinator {

template <typename T>
using Parser =
    std::function<std::expected<std::pair<T, std::string_view>, ParseError>(
        std::string_view)>;

inline Parser<char> one_char() {
  return [](std::string_view input)
             -> std::expected<std::pair<char, std::string_view>, ParseError> {
    if (input.empty()) {
      return std::unexpected(ParseError::IncompleteRequest);
    }
    return std::pair{input[0], input.substr(1)};
  };
}

inline Parser<char> satisfy(std::function<bool(char)> predicate) {
  return [predicate](std::string_view input) 
    ->std::expected<std::pair<char, std::string_view>, ParseError> {
      return one_char()(input).and_then(
          [&](auto pair)
              -> std::expected<std::pair<char, std::string_view>, ParseError> {
            auto [ch, rest] = pair;
            if (predicate(ch)) {
              return std::pair{ch, rest};
            }
            return std::unexpected(ParseError::MalformedRequest);
          });
    };
  }

  inline Parser<std::string_view>
  string(std::string_view target) {
    return [target](std::string_view input)
               -> std::expected<std::pair<std::string_view, std::string_view>,
                                ParseError> {
      if (input.starts_with(target)) {
        return std::pair{target, input.substr(target.size())};
      }
      return std::unexpected(ParseError::MalformedRequest);
    };
  }

  template <typename A, typename B>
  Parser<std::pair<A, B>> sequence(Parser<A> pa, Parser<B> pb) {
    return [pa = std::move(pa), pb = std::move(pb)](std::string_view input) {
      return pa(input).and_then([&](auto first_result) {
        auto [a, rest1] = first_result;
        return pb(rest1).transform([&](auto second_result) {
          auto [b, rest2] = second_result;
          return std::pair{std::pair{std::move(a), std::move(b)}, rest2};
        });
      });
    };
  }

  template <typename T> Parser<T> choice(Parser<T> p1, Parser<T> p2) {
    return [p1 = std::move(p1), p2 = std::move(p2)](std::string_view input) {
      auto r = p1(input);
      if (r) {
        return r;
      }
      return p2(input);
    };
  }

template<typename T>
Parser<T> choice(std::vector<Parser<T>> parsers) {
  return [parsers = std::move(parsers)](std::string_view input)
             -> std::expected<std::pair<T, std::string_view>, ParseError> {
    for (const auto &p : parsers) {
      auto result = p(input);
      if (result) {
        return result;
      }
    }
    return std::unexpected(ParseError::MalformedRequest);
  };
}

template <typename A, typename F> auto map(Parser<A> pa, F f) {
  using B = decltype(f(std::declval<A>()));
  return Parser<B>(
      [pa = std::move(pa), f = std::move(f)](std::string_view input)
          -> std::expected<std::pair<B, std::string_view>, ParseError> {
        return pa(input).transform([&](auto pair) {
          auto [a, rest] = pair;
          return std::pair{f(std::move(a)), rest};
        });
      });
}

template <typename T> Parser<std::vector<T>> many(Parser<T> p) {
  return [p = std::move(p)](std::string_view input)
             -> std::expected<std::pair<std::vector<T>, std::string_view>,
                              ParseError> {
    std::vector<T> results;
    auto current_input = input;

    while (true) {
      auto result = p(current_input);
      if (!result) {
        break;
      }
      auto [value, rest] = *result;
      results.push_back(std::move(value));
      current_input = rest;
    }

    return std::pair{std::move(results), current_input};
  };
}

template <typename T> Parser<std::vector<T>> many1(Parser<T> p) {
  return [p = std::move(p)](std::string_view input) {
    return p(input).and_then([&](auto first_result) {
      auto [first, rest1] = first_result;
      auto many_result = many(p)(rest1);

      return many_result.transform([&](auto many_pair) {
        auto [rest_vec, final_rest] = many_pair;
        std::vector<T> all_results;
        all_results.push_back(std::move(first));
        all_results.insert(all_results.end(), rest_vec.begin(), rest_vec.end());
        return std::pair{std::move(all_results), final_rest};
      });
    });
  };
}

inline Parser<std::monostate> spaces() {
  return [](std::string_view input)
             -> std::expected<std::pair<std::monostate, std::string_view>,
                              ParseError> {
    auto it = std::find_if_not(input.begin(), input.end(),
                               [](unsigned char c) { return std::isspace(c); });
    auto count = std::distance(input.begin(), it);
    return std::pair{std::monostate{}, input.substr(count)};
  };
}

inline Parser<std::string_view> take_until(char delimiter) {
  return [delimiter](std::string_view input)
             -> std::expected<std::pair<std::string_view, std::string_view>,
                              ParseError> {
    auto pos = input.find(delimiter);
    if (pos == std::string_view::npos) {
      return std::unexpected(ParseError::IncompleteRequest);
    }
    return std::pair{input.substr(0, pos), input.substr(pos)};
  };
}

} // namespace http::parser::combinator
