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

void expect_eq_int(int actual, int expected, const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::to_string(expected) +
         "\nActual: " + std::to_string(actual));
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

void test_parser_string_wrappers_use_symbol_id_keyed_name_tables() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena);

  c4c::TypeSpec typedef_ts{};
  typedef_ts.array_size = -1;
  typedef_ts.inner_rank = -1;
  typedef_ts.base = c4c::TB_INT;

  c4c::TypeSpec var_ts{};
  var_ts.array_size = -1;
  var_ts.inner_rank = -1;
  var_ts.base = c4c::TB_DOUBLE;

  parser.register_typedef_binding("Value", typedef_ts, true);
  parser.register_var_type_binding("counter", var_ts);

  const c4c::Parser::SymbolId typedef_symbol =
      parser.parser_name_tables_.find_identifier("Value");
  const c4c::Parser::SymbolId var_symbol =
      parser.parser_name_tables_.find_identifier("counter");

  expect_true(typedef_symbol != c4c::Parser::kInvalidSymbol,
              "typedef wrapper should intern a valid SymbolId");
  expect_true(parser.parser_name_tables_.is_typedef(typedef_symbol),
              "typedef wrapper should store membership by SymbolId");
  expect_true(parser.parser_name_tables_.user_typedefs.count(typedef_symbol) == 1,
              "user typedef wrapper should track user typedef membership by SymbolId");
  expect_true(parser.has_typedef_name("Value"),
              "string-facing typedef lookup should still work");
  expect_true(parser.has_typedef_type("Value"),
              "typedef type wrapper should still resolve through string lookup");
  expect_true(parser.find_typedef_type("Value") != nullptr &&
                  parser.find_typedef_type("Value")->base == c4c::TB_INT,
              "typedef type wrapper should recover the stored TypeSpec");
  expect_true(parser.parser_name_tables_.lookup_typedef_type(typedef_symbol) != nullptr &&
                  parser.parser_name_tables_.lookup_typedef_type(typedef_symbol)->base ==
                      c4c::TB_INT,
              "symbol-keyed typedef map should recover the stored TypeSpec");

  expect_true(var_symbol != c4c::Parser::kInvalidSymbol,
              "var-type wrapper should intern a valid SymbolId");
  expect_true(parser.parser_name_tables_.var_types.count(var_symbol) == 1,
              "var-type wrapper should store var bindings by SymbolId");
  expect_true(parser.has_var_type("counter"),
              "string-facing var-type lookup should still work");
  expect_true(parser.find_var_type("counter") != nullptr &&
                  parser.find_var_type("counter")->base == c4c::TB_DOUBLE,
              "var-type wrapper should recover the stored TypeSpec");
}

void test_parser_heavy_snapshot_restores_symbol_id_keyed_tables() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena);

  c4c::TypeSpec keep_ts{};
  keep_ts.array_size = -1;
  keep_ts.inner_rank = -1;
  keep_ts.base = c4c::TB_INT;

  c4c::TypeSpec temp_ts{};
  temp_ts.array_size = -1;
  temp_ts.inner_rank = -1;
  temp_ts.base = c4c::TB_FLOAT;

  parser.register_typedef_binding("Keep", keep_ts, false);
#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const size_t typedef_count_before = parser.parser_name_tables_.typedefs.size();
  const auto snapshot = parser.save_state();
  parser.register_typedef_binding("Temp", temp_ts, true);
  parser.register_var_type_binding("scratch", temp_ts);
  parser.restore_state(snapshot);

  expect_true(parser.has_typedef_name("Keep"),
              "restore_state should preserve typedefs that existed before the snapshot");
  expect_true(!parser.has_typedef_name("Temp"),
              "restore_state should roll back typedefs added after the snapshot");
  expect_true(!parser.has_var_type("scratch"),
              "restore_state should roll back var bindings added after the snapshot");
  expect_eq_int(static_cast<int>(parser.parser_name_tables_.typedefs.size()),
                static_cast<int>(typedef_count_before),
                "heavy snapshot restore should reset typedef membership to the snapshot");
#else
  expect_true(parser.has_typedef_name("Keep"),
              "test fixture should preserve the baseline typedef when heavy snapshots are off");
#endif
}

}  // namespace

int main() {
  test_parser_reuses_symbol_ids_for_repeated_identifier_text_ids();
  test_parser_symbol_helper_falls_back_to_lexeme_without_text_id();
  test_parser_string_wrappers_use_symbol_id_keyed_name_tables();
  test_parser_heavy_snapshot_restores_symbol_id_keyed_tables();

  std::cout << "PASS: frontend_parser_tests\n";
  return 0;
}
