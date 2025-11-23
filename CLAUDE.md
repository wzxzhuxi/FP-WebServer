# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a functional programming-based HTTP server implementation in modern C++. The codebase uses parser combinators and functional composition to parse HTTP/1.x requests.

## Architecture

### Parser Combinator System (`parser/`)

The core parsing architecture is built on functional parser combinators using `std::expected` for error handling:

- **`types.hpp`**: Defines HTTP domain types (`Method`, `Version`, `RequestLine`, `HttpRequest`, `Headers`) and error types (`ParseError`). Uses `std::expected<T, ParseError>` as the result type for all parsing operations.

- **`combinaor.hpp`**: Core combinator library implementing monadic parser composition:
  - `Parser<T>` is a function type: `std::function<std::expected<std::pair<T, std::string_view>, ParseError>(std::string_view)>`
  - Parsers consume input and return either `(parsed_value, remaining_input)` or an error
  - Fundamental combinators: `one_char()`, `satisfy()`, `string()`
  - Composition operators: `sequence()`, `choice()`, `map()`
  - Repetition: `many()`, `many1()`
  - Utilities: `spaces()`, `take_until()`

- **`http_parser.hpp`**: HTTP-specific parsers built by composing primitives:
  - `parse_method()`: Parses HTTP methods using `choice()` over string literals
  - `parse_url()`: Extracts URL from request line (current implementation is overly simplistic)
  - `parse_version()`: Parses HTTP/1.0 or HTTP/1.1
  - `parse_request_line()`: Manually sequences method, URL, version parsing (should use `sequence()` combinator but doesn't)
  - `parse_header()`: Parses individual headers (key: value\r\n)
  - `parse_headers()`: Collects all headers until blank line
  - `parse_http_request()`: Top-level parser composing request line + headers + body

### HTTP Connection Layer (`http/`)

Currently empty stub files. This is where connection handling, response generation, and server socket logic will live.

## Code Quality Issues

This codebase has several bugs and design problems:

1. **Syntax errors in `combinaor.hpp`**:
   - Line 70: `Parser<T> choice<std::vector<Parser<T>> parsers)` - malformed template syntax
   - Line 72: `ParserError` instead of `ParseError` (inconsistent naming)
   - Line 132: Same `ParserError` typo in `spaces()`

2. **Syntax errors in `http_parser.hpp`**:
   - Line 5: `namespace http : parser` should be `namespace http::parser`
   - Lines 100-102: Broken return type syntax with misplaced angle bracket
   - Line 143: `parse_http_request()` function signature missing `std::string_view input` parameter

3. **Semantic issues**:
   - `parse_url()` only accepts alphanumeric characters - this breaks for URLs with `/`, `?`, `#`, etc.
   - `parse_request_line()` manually sequences parsers instead of using the `sequence()` combinator that was defined for this purpose
   - Empty files `http_cnn.cpp` and `http_cnn.hpp` suggest incomplete implementation

4. **No build system**: No Makefile, CMakeLists.txt, or compilation instructions

## Development Workflow

Since there's no build system yet, compilation will require:
- C++23 support (for `std::expected`)
- Include paths for the `parser/` directory
- Example: `g++ -std=c++23 -I. -Iparser/ <source_files>`

## Functional Programming Patterns Used

- Monadic composition via `std::expected` and its `and_then()`, `transform()` methods
- Parser combinators as higher-order functions
- Immutable parsing - parsers return new state rather than mutating input
- Type safety through `std::variant`, `std::optional`, `std::expected`
- Declarative parsing - describe the grammar, not the parsing algorithm

## What Needs Fixing First

1. Fix all syntax errors in `combinaor.hpp` and `http_parser.hpp`
2. Implement proper URL parsing (handle `/`, query strings, fragments, etc.)
3. Refactor `parse_request_line()` to actually use the `sequence()` combinator
4. Add a build system (Makefile or CMakeLists.txt)
5. Implement the HTTP connection handling in `http/`
6. Add tests for parser combinators
