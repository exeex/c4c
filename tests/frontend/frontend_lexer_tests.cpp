#include "lexer.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

void test_lexer_populates_text_and_file_ids_for_basic_tokens() {
  c4c::Lexer lexer("int value = value;\n");
  const std::vector<c4c::Token> tokens = lexer.scan_all();

  expect_true(tokens.size() == 6,
              "basic lexer test should produce five tokens plus EOF");
  expect_true(tokens[0].kind == c4c::TokenKind::KwInt,
              "first token should be KW_int");
  expect_true(tokens[1].kind == c4c::TokenKind::Identifier,
              "second token should be an identifier");
  expect_true(tokens[1].text_id != c4c::kInvalidText,
              "identifier token should carry a text_id");
  expect_true(tokens[0].file_id != c4c::kInvalidFile,
              "tokens without a line marker should still carry a stable file_id");
  expect_true(tokens[1].text_id == tokens[3].text_id,
              "repeated identifier spellings should reuse one text_id");
  expect_eq(lexer.text_table().lookup(tokens[1].text_id), "value",
            "text table should resolve identifier spellings by text_id");
  expect_eq(lexer.file_table().lookup(tokens[0].file_id), "",
            "default file identity should preserve the legacy empty file string");
  expect_true(tokens.back().text_id == c4c::kInvalidText,
              "EOF should keep the invalid text_id sentinel");
  expect_true(tokens.back().file_id == tokens[0].file_id,
              "EOF should keep the current file identity");
}

void test_lexer_tracks_line_marker_files_and_prefixed_literal_spellings() {
  c4c::Lexer lexer("# 7 \"alpha.c\"\nfoo foo\n# 9 \"beta.c\"\nL\"x\" foo\n");
  const std::vector<c4c::Token> tokens = lexer.scan_all();

  expect_true(tokens.size() == 5,
              "line-marker lexer test should produce four tokens plus EOF");
  expect_true(tokens[0].kind == c4c::TokenKind::Identifier &&
                  tokens[1].kind == c4c::TokenKind::Identifier,
              "line-marker test should start with two identifiers");
  expect_true(tokens[2].kind == c4c::TokenKind::StrLit,
              "prefixed literal should stay classified as STR_LIT");
  expect_true(tokens[0].text_id == tokens[1].text_id,
              "repeated identifiers in one file should reuse the same text_id");
  expect_true(tokens[0].text_id == tokens[3].text_id,
              "identifier text ids should stay stable across line markers");
  expect_eq(lexer.file_table().lookup(tokens[0].file_id), "alpha.c",
            "first line marker should set the logical file");
  expect_eq(lexer.file_table().lookup(tokens[3].file_id), "beta.c",
            "later line marker should update the logical file");
  expect_eq(lexer.text_table().lookup(tokens[2].text_id), "L\"x\"",
            "prefixed string literals should intern the full raw spelling");
}

}  // namespace

int main() {
  test_lexer_populates_text_and_file_ids_for_basic_tokens();
  test_lexer_tracks_line_marker_files_and_prefixed_literal_spellings();

  std::cout << "PASS: frontend_lexer_tests\n";
  return 0;
}
