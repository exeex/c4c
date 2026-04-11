#include "lexer.hpp"
#include "parser.hpp"

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

void test_parser_reuses_symbol_ids_for_repeated_identifier_text_ids() {
  c4c::Lexer lexer("typedef int Value;\nValue other;\nValue third;\n");
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena);

  const c4c::Parser::SymbolId first = parser.symbol_id_for_token(tokens[2]);
  const c4c::Parser::SymbolId second = parser.symbol_id_for_token(tokens[4]);
  const c4c::Parser::SymbolId third = parser.symbol_id_for_token(tokens[7]);

  expect_true(first != c4c::Parser::kInvalidSymbol,
              "identifier tokens should intern to a valid SymbolId");
  expect_true(first == second && second == third,
              "repeated identifier text ids should reuse one SymbolId");
  expect_eq(parser.symbol_spelling(first), "Value",
            "parser symbol table should recover identifier spelling");
  expect_true(parser.parser_text_ids_.size() == 1,
              "parser should cache one parser-owned text id for one token text id");
}

void test_parser_symbol_helper_falls_back_to_lexeme_without_text_id() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena);
  const c4c::Token token{c4c::TokenKind::Identifier, "Fallback", "", 1, 1,
                         c4c::kInvalidText, c4c::kInvalidFile};

  const c4c::Parser::SymbolId symbol = parser.symbol_id_for_token(token);

  expect_true(symbol != c4c::Parser::kInvalidSymbol,
              "identifier fallback path should intern from lexeme when text_id is absent");
  expect_eq(parser.symbol_spelling(symbol), "Fallback",
            "fallback symbol path should preserve the identifier spelling");
}

}  // namespace

int main() {
  test_parser_reuses_symbol_ids_for_repeated_identifier_text_ids();
  test_parser_symbol_helper_falls_back_to_lexeme_without_text_id();

  std::cout << "PASS: frontend_parser_tests\n";
  return 0;
}
