#include "combinaor.hpp"
#include "types.hpp"
#include <gtest/gtest.h>

using namespace http::parser;
using namespace http::parser::combinator;

// Test one_char
TEST(CombinatorTest, OneChar_Success) {
  auto parser = one_char();
  auto result = parser("abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, 'a');
  EXPECT_EQ(result->second, "bc");
}

TEST(CombinatorTest, OneChar_EmptyInput) {
  auto parser = one_char();
  auto result = parser("");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ParseError::IncompleteRequest);
}

// Test satisfy
TEST(CombinatorTest, Satisfy_Success) {
  auto parser = satisfy([](char c) { return std::isdigit(c); });
  auto result = parser("123");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, '1');
  EXPECT_EQ(result->second, "23");
}

TEST(CombinatorTest, Satisfy_Failure) {
  auto parser = satisfy([](char c) { return std::isdigit(c); });
  auto result = parser("abc");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ParseError::MalformedRequest);
}

// Test string
TEST(CombinatorTest, String_Success) {
  auto parser = string("GET");
  auto result = parser("GET /index.html");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "GET");
  EXPECT_EQ(result->second, " /index.html");
}

TEST(CombinatorTest, String_Failure) {
  auto parser = string("POST");
  auto result = parser("GET /index.html");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ParseError::MalformedRequest);
}

TEST(CombinatorTest, String_PartialMatch) {
  auto parser = string("GETPOST");
  auto result = parser("GET");

  ASSERT_FALSE(result.has_value());
}

// Test sequence
TEST(CombinatorTest, Sequence_Success) {
  auto p1 = string("GET");
  auto p2 = string(" ");
  auto parser = sequence(p1, p2);
  auto result = parser("GET /path");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.first, "GET");
  EXPECT_EQ(result->first.second, " ");
  EXPECT_EQ(result->second, "/path");
}

TEST(CombinatorTest, Sequence_FirstFails) {
  auto p1 = string("POST");
  auto p2 = string(" ");
  auto parser = sequence(p1, p2);
  auto result = parser("GET /path");

  ASSERT_FALSE(result.has_value());
}

TEST(CombinatorTest, Sequence_SecondFails) {
  auto p1 = string("GET");
  auto p2 = string("X");
  auto parser = sequence(p1, p2);
  auto result = parser("GET /path");

  ASSERT_FALSE(result.has_value());
}

// Test choice (binary version)
TEST(CombinatorTest, Choice_FirstSucceeds) {
  auto p1 = string("GET");
  auto p2 = string("POST");
  auto parser = choice(p1, p2);
  auto result = parser("GET /path");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "GET");
}

TEST(CombinatorTest, Choice_SecondSucceeds) {
  auto p1 = string("GET");
  auto p2 = string("POST");
  auto parser = choice(p1, p2);
  auto result = parser("POST /data");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "POST");
}

TEST(CombinatorTest, Choice_BothFail) {
  auto p1 = string("GET");
  auto p2 = string("POST");
  auto parser = choice(p1, p2);
  auto result = parser("DELETE /resource");

  ASSERT_FALSE(result.has_value());
}

// Test map
TEST(CombinatorTest, Map_Success) {
  auto parser = map(string("42"), [](auto) { return 42; });
  auto result = parser("42 is the answer");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, 42);
  EXPECT_EQ(result->second, " is the answer");
}

TEST(CombinatorTest, Map_Failure) {
  auto parser = map(string("42"), [](auto) { return 42; });
  auto result = parser("not a number");

  ASSERT_FALSE(result.has_value());
}

// Test many
TEST(CombinatorTest, Many_MultipleMatches) {
  auto parser = many(satisfy([](char c) { return std::isdigit(c); }));
  auto result = parser("123abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.size(), 3);
  EXPECT_EQ(result->first[0], '1');
  EXPECT_EQ(result->first[1], '2');
  EXPECT_EQ(result->first[2], '3');
  EXPECT_EQ(result->second, "abc");
}

TEST(CombinatorTest, Many_NoMatches) {
  auto parser = many(satisfy([](char c) { return std::isdigit(c); }));
  auto result = parser("abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.size(), 0);
  EXPECT_EQ(result->second, "abc");
}

TEST(CombinatorTest, Many_EmptyInput) {
  auto parser = many(satisfy([](char c) { return std::isdigit(c); }));
  auto result = parser("");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.size(), 0);
}

// Test many1
TEST(CombinatorTest, Many1_MultipleMatches) {
  auto parser = many1(satisfy([](char c) { return std::isdigit(c); }));
  auto result = parser("123abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.size(), 3);
  EXPECT_EQ(result->second, "abc");
}

TEST(CombinatorTest, Many1_SingleMatch) {
  auto parser = many1(satisfy([](char c) { return std::isdigit(c); }));
  auto result = parser("1abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first.size(), 1);
  EXPECT_EQ(result->first[0], '1');
}

TEST(CombinatorTest, Many1_NoMatches) {
  auto parser = many1(satisfy([](char c) { return std::isdigit(c); }));
  auto result = parser("abc");

  ASSERT_FALSE(result.has_value());
}

// Test spaces
TEST(CombinatorTest, Spaces_MultipleSpaces) {
  auto parser = spaces();
  auto result = parser("   abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->second, "abc");
}

TEST(CombinatorTest, Spaces_TabsAndNewlines) {
  auto parser = spaces();
  auto result = parser("\t\n\r abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->second, "abc");
}

TEST(CombinatorTest, Spaces_NoSpaces) {
  auto parser = spaces();
  auto result = parser("abc");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->second, "abc");
}

// Test take_until
TEST(CombinatorTest, TakeUntil_Success) {
  auto parser = take_until(':');
  auto result = parser("key:value");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "key");
  EXPECT_EQ(result->second, ":value");
}

TEST(CombinatorTest, TakeUntil_NotFound) {
  auto parser = take_until(':');
  auto result = parser("no colon here");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), ParseError::IncompleteRequest);
}

TEST(CombinatorTest, TakeUntil_EmptyBefore) {
  auto parser = take_until(':');
  auto result = parser(":value");

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "");
  EXPECT_EQ(result->second, ":value");
}

// Complex combinator composition
TEST(CombinatorTest, ComplexComposition_KeyValuePair) {
  auto key_parser = take_until(':');
  auto colon = string(":");
  auto value_parser = take_until('\n');

  auto result1 = key_parser("Host:localhost\n");
  ASSERT_TRUE(result1.has_value());
  auto [key, rest1] = *result1;

  auto result2 = colon(rest1);
  ASSERT_TRUE(result2.has_value());
  auto [_, rest2] = *result2;

  auto result3 = value_parser(rest2);
  ASSERT_TRUE(result3.has_value());
  auto [value, rest3] = *result3;

  EXPECT_EQ(key, "Host");
  EXPECT_EQ(value, "localhost");
}
