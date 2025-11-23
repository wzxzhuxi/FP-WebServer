#pragma once
#include "types.hpp"
#include <optional>
#include <regex>

namespace http::router {

class PathPattern {
  std::string pattern_;
  std::regex regex_;
  std::vector<std::string> param_names_;

public:
  PathPattern() = default;

  explicit PathPattern(std::string_view pattern) : pattern_(pattern) {
    std::string regex_str = "^";
    std::string current_segment;
    bool in_param = false;

    for (size_t i = 0; i < pattern.size(); ++i) {
      const char c = pattern[i];

      if (c == ':') {
        in_param = true;
        current_segment.clear();
      } else if (c == '/' || i == pattern.size() - 1) {
        if (in_param) {
          if (i == pattern.size() - 1 && c != '/') {
            current_segment += c;
          }
          param_names_.push_back(current_segment);
          regex_str += "([^/]+)";
          in_param = false;
        }
        if (c == '/') {
          regex_str += '/';
        } else if (c == '*') {
          param_names_.push_back("wildcard");
          regex_str += "(.*)";
          break;
        } else {
          if (in_param) {
            current_segment += c;
          } else {
            if (c == '.' || c == '?' || c == '+') {
              regex_str += '\\';
            }
            regex_str += c;
          }
        }
      }
    }
    regex_str += "$";
    regex_ = std::regex(regex_str);
  }
  [[nodiscard]] std::optional<std::unordered_map<std::string, std::string>> match(std::string_view path) const {
    std::smatch matches;
    const std::string path_str(path);

    if(std::regex_match(path_str, matches, regex_)) {
      std::unordered_map<std::string, std::string> params;
      for (size_t i = 1; i < matches.size(); ++i) {
        if (i - 1 < param_names_.size()) {
          params[param_names_[i - 1]] = matches[i].str();
        }
      }
      return params;
    }
    return std::nullopt;
  }
  [[nodiscard]] const std::string& pattern() const { return pattern_; }
};

} // namespace http::router

// namespace PathPattern
