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

void test_parser_keeps_qualified_bindings_string_keyed() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena);

  c4c::Parser::QualifiedNameRef qn;
  qn.is_global_qualified = true;
  qn.qualifier_segments = {"ns", "inner"};
  qn.base_name = "Type";

  expect_true(!qn.is_unqualified_atom(),
              "qualified names should not be treated as source-atom symbols");
  expect_eq(qn.spelled(), "ns::inner::Type",
            "qualified lookup spelling should remain segment-joined text");
  expect_eq(qn.spelled(/*include_global_prefix=*/true), "::ns::inner::Type",
            "qualified spelling should preserve global qualification when requested");

  c4c::TypeSpec typedef_ts{};
  typedef_ts.array_size = -1;
  typedef_ts.inner_rank = -1;
  typedef_ts.base = c4c::TB_INT;

  c4c::TypeSpec var_ts{};
  var_ts.array_size = -1;
  var_ts.inner_rank = -1;
  var_ts.base = c4c::TB_DOUBLE;

  const int symbol_count_before =
      static_cast<int>(parser.parser_symbols_.size());

  parser.register_typedef_binding("ns::Type", typedef_ts, true);
  parser.register_var_type_binding("ns::value", var_ts);

  expect_true(parser.has_typedef_name("ns::Type"),
              "qualified typedef membership should remain lookupable");
  expect_true(parser.has_typedef_type("ns::Type"),
              "qualified typedef types should remain lookupable");
  expect_true(parser.find_typedef_type("ns::Type") != nullptr &&
                  parser.find_typedef_type("ns::Type")->base == c4c::TB_INT,
              "qualified typedef type lookup should recover the stored TypeSpec");
  expect_true(parser.has_var_type("ns::value"),
              "qualified value bindings should remain lookupable");
  expect_true(parser.find_var_type("ns::value") != nullptr &&
                  parser.find_var_type("ns::value")->base == c4c::TB_DOUBLE,
              "qualified value lookup should recover the stored TypeSpec");
  expect_true(parser.parser_name_tables_.find_identifier("ns::Type") ==
                  c4c::Parser::kInvalidSymbol,
              "qualified typedef names should not intern composed strings");
  expect_true(parser.parser_name_tables_.find_identifier("ns::value") ==
                  c4c::Parser::kInvalidSymbol,
              "qualified value names should not intern composed strings");
  expect_eq_int(static_cast<int>(parser.parser_symbols_.size()),
                symbol_count_before,
                "qualified bindings should not change the atom-symbol table size");

#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const auto snapshot = parser.save_state();

  c4c::TypeSpec temp_ts{};
  temp_ts.array_size = -1;
  temp_ts.inner_rank = -1;
  temp_ts.base = c4c::TB_FLOAT;

  parser.register_typedef_binding("ns::Temp", temp_ts, true);
  parser.register_var_type_binding("ns::scratch", temp_ts);
  parser.restore_state(snapshot);

  expect_true(!parser.has_typedef_type("ns::Temp"),
              "restore_state should roll back qualified typedefs from fallback storage");
  expect_true(!parser.has_var_type("ns::scratch"),
              "restore_state should roll back qualified values from fallback storage");
#endif
}

void test_parser_parse_qualified_name_populates_atom_symbol_ids() {
  c4c::Lexer lexer("::ns::inner::Type value;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, c4c::SourceProfile::CppSubset);

  const c4c::Parser::QualifiedNameRef qn = parser.parse_qualified_name();

  expect_true(qn.is_global_qualified,
              "parsed qualified names should preserve leading global qualification");
  expect_eq(qn.spelled(/*include_global_prefix=*/true), "::ns::inner::Type",
            "parsed qualified names should preserve the joined lookup spelling");
  expect_eq_int(static_cast<int>(qn.qualifier_segments.size()), 2,
                "qualified names should keep two qualifier segments");
  expect_eq_int(static_cast<int>(qn.qualifier_symbol_ids.size()), 2,
                "qualified names should carry one atom id per qualifier segment");
  expect_eq(qn.qualifier_segments[0], "ns",
            "first qualifier segment should preserve its source spelling");
  expect_eq(qn.qualifier_segments[1], "inner",
            "second qualifier segment should preserve its source spelling");
  expect_true(qn.qualifier_symbol_ids[0] != c4c::Parser::kInvalidSymbol,
              "first qualifier segment should intern to a valid SymbolId");
  expect_true(qn.qualifier_symbol_ids[1] != c4c::Parser::kInvalidSymbol,
              "second qualifier segment should intern to a valid SymbolId");
  expect_true(qn.base_symbol_id != c4c::Parser::kInvalidSymbol,
              "base segment should intern to a valid SymbolId");
  expect_eq(parser.symbol_spelling(qn.qualifier_symbol_ids[0]), "ns",
            "first qualifier atom id should resolve back to its spelling");
  expect_eq(parser.symbol_spelling(qn.qualifier_symbol_ids[1]), "inner",
            "second qualifier atom id should resolve back to its spelling");
  expect_eq(parser.symbol_spelling(qn.base_symbol_id), "Type",
            "base atom id should resolve back to its spelling");
}

void test_parser_injected_token_helpers_preserve_qualified_name_spelling() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena, c4c::SourceProfile::CppSubset);

  c4c::Token seed{};
  seed.line = 7;
  seed.column = 3;
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
  };

  expect_true(parser.tokens_[0].text_id != c4c::kInvalidText,
              "injected identifier tokens should receive a stable text id");
  expect_eq(parser.token_spelling(parser.tokens_[0]), "ns",
            "parser token spelling helper should recover injected identifier spelling");

  c4c::Parser::QualifiedNameRef qn;
  expect_true(parser.peek_qualified_name(&qn, /*allow_global=*/true),
              "qualified-name peeking should accept helper-built injected tokens");
  expect_eq(qn.spelled(), "ns::Value",
            "qualified-name peeking should preserve helper-built injected spelling");

  const c4c::Parser::SymbolId symbol = parser.symbol_id_for_token(parser.tokens_[2]);
  expect_true(symbol != c4c::Parser::kInvalidSymbol,
              "helper-built injected identifier tokens should intern to a SymbolId");
  expect_eq(parser.symbol_spelling(symbol), "Value",
            "helper-built injected identifier symbols should preserve spelling");
}

void test_parser_value_like_template_lookahead_uses_token_spelling() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena, c4c::SourceProfile::CppSubset);

  c4c::Token seed{};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  parser.tokens_[5].lexeme = "bridge_only_value";
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  expect_true(parser.is_clearly_value_template_arg(nullptr, 0),
              "value-like template lookahead should read parser-owned token spelling");
}

void test_parser_capture_template_arg_expr_uses_token_spelling() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena, c4c::SourceProfile::CppSubset);

  c4c::Token seed{};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
      parser.make_injected_token(seed, c4c::TokenKind::Plus, "+"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "1"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  };
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  parser.tokens_[0].lexeme = "bridge_only_value";
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.capture_template_arg_expr(0, &arg),
              "template arg expression capture should succeed for injected tokens");
  expect_true(arg.nttp_name != nullptr,
              "captured template arg expression should preserve text in nttp_name");
  expect_eq(arg.nttp_name, "$expr:Value+1",
            "template arg expression capture should use parser-owned token spelling");
  expect_eq_int(parser.pos_, 3,
                "template arg expression capture should stop before template close");
}

}  // namespace

int main() {
  test_parser_reuses_symbol_ids_for_repeated_identifier_text_ids();
  test_parser_symbol_helper_falls_back_to_lexeme_without_text_id();
  test_parser_string_wrappers_use_symbol_id_keyed_name_tables();
  test_parser_heavy_snapshot_restores_symbol_id_keyed_tables();
  test_parser_keeps_qualified_bindings_string_keyed();
  test_parser_parse_qualified_name_populates_atom_symbol_ids();
  test_parser_injected_token_helpers_preserve_qualified_name_spelling();
  test_parser_value_like_template_lookahead_uses_token_spelling();
  test_parser_capture_template_arg_expr_uses_token_spelling();

  std::cout << "PASS: frontend_parser_tests\n";
  return 0;
}
