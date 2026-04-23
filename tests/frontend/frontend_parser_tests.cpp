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

void test_link_name_table_reuses_text_table_storage() {
  c4c::TextTable texts;
  c4c::LinkNameTable link_names(&texts);

  const c4c::LinkNameId alpha = link_names.intern("Alpha");
  const c4c::TextId alpha_text = texts.intern("Alpha");
  const c4c::LinkNameId alpha_again = link_names.intern(alpha_text);

  expect_true(alpha != c4c::kInvalidLinkName,
              "link-name interning should produce a valid LinkNameId");
  expect_true(alpha == alpha_again,
              "interning the same text through text ids should reuse one LinkNameId");
  expect_true(link_names.find("Alpha") == alpha,
              "string lookup should recover the existing LinkNameId");
  expect_true(link_names.find(alpha_text) == alpha,
              "text-id lookup should recover the existing LinkNameId");
  expect_true(link_names.text_id(alpha) == alpha_text,
              "LinkNameTable should map semantic ids back to the shared TextId");
  expect_eq(link_names.spelling(alpha), "Alpha",
            "LinkNameTable spelling should resolve through the shared TextTable");
  expect_true(link_names.size() == 1,
              "one unique link-name spelling should occupy one semantic id slot");
  expect_true(texts.size() == 1,
              "LinkNameTable should reuse existing TextTable storage");

  const c4c::LinkNameId missing = link_names.find("Missing");
  expect_true(missing == c4c::kInvalidLinkName,
              "missing link-name lookups should stay invalid without creating entries");
  expect_true(link_names.text_id(c4c::kInvalidLinkName) == c4c::kInvalidText,
              "invalid LinkNameId should not resolve to a valid TextId");
}

void test_parser_reuses_symbol_ids_for_repeated_identifier_text_ids() {
  c4c::Lexer lexer("typedef int Value;\nValue other;\nValue third;\n");
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table());

  const c4c::Parser::SymbolId first = parser.symbol_id_for_token(tokens[2]);
  const c4c::Parser::SymbolId second = parser.symbol_id_for_token(tokens[4]);
  const c4c::Parser::SymbolId third = parser.symbol_id_for_token(tokens[7]);

  expect_true(first != c4c::Parser::kInvalidSymbol,
              "identifier tokens should intern to a valid SymbolId");
  expect_true(first == second && second == third,
              "repeated identifier text ids should reuse one SymbolId");
  expect_eq(parser.symbol_spelling(first), "Value",
            "parser symbol table should recover identifier spelling");
  expect_true(parser.shared_lookup_state_.parser_symbols.texts_ ==
                  &lexer.text_table(),
              "parser symbol table should reuse the shared lexer text table");
}

void test_parser_missing_text_id_is_rejected() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);
  c4c::Token token{};
  token.kind = c4c::TokenKind::Identifier;
  token.line = 1;
  token.column = 1;

  bool threw = false;
  try {
    (void)parser.token_spelling(token);
  } catch (const std::runtime_error&) {
    threw = true;
  }

  expect_true(threw,
              "tokens without a valid text_id should fail fast instead of falling back");
}

void test_parser_string_wrappers_use_symbol_id_keyed_name_tables() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

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
      parser.shared_lookup_state_.parser_name_tables.find_identifier("Value");
  const c4c::Parser::SymbolId var_symbol =
      parser.shared_lookup_state_.parser_name_tables.find_identifier("counter");

  expect_true(typedef_symbol != c4c::Parser::kInvalidSymbol,
              "typedef wrapper should intern a valid SymbolId");
  expect_true(parser.shared_lookup_state_.parser_name_tables.is_typedef(
                  typedef_symbol),
              "typedef wrapper should store membership by SymbolId");
  expect_true(parser.shared_lookup_state_.parser_name_tables.user_typedefs.count(
                  typedef_symbol) == 1,
              "user typedef wrapper should track user typedef membership by SymbolId");
  expect_true(parser.has_typedef_name("Value"),
              "string-facing typedef lookup should still work");
  expect_true(parser.has_typedef_type("Value"),
              "typedef type wrapper should still resolve through string lookup");
  expect_true(parser.find_typedef_type("Value") != nullptr &&
                  parser.find_typedef_type("Value")->base == c4c::TB_INT,
              "typedef type wrapper should recover the stored TypeSpec");
  expect_true(parser.shared_lookup_state_.parser_name_tables.lookup_typedef_type(
                  typedef_symbol) != nullptr &&
                  parser.shared_lookup_state_.parser_name_tables
                          .lookup_typedef_type(typedef_symbol)
                          ->base ==
                      c4c::TB_INT,
              "symbol-keyed typedef map should recover the stored TypeSpec");

  expect_true(var_symbol != c4c::Parser::kInvalidSymbol,
              "var-type wrapper should intern a valid SymbolId");
  expect_true(parser.shared_lookup_state_.parser_name_tables.var_types.count(
                  var_symbol) == 1,
              "var-type wrapper should store var bindings by SymbolId");
  expect_true(parser.has_var_type("counter"),
              "string-facing var-type lookup should still work");
  expect_true(parser.find_var_type("counter") != nullptr &&
                  parser.find_var_type("counter")->base == c4c::TB_DOUBLE,
              "var-type wrapper should recover the stored TypeSpec");
}

void test_parser_heavy_snapshot_restores_symbol_id_keyed_tables() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

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
  const size_t typedef_count_before =
      parser.shared_lookup_state_.parser_name_tables.typedefs.size();
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
  expect_eq_int(
      static_cast<int>(parser.shared_lookup_state_.parser_name_tables.typedefs.size()),
                static_cast<int>(typedef_count_before),
                "heavy snapshot restore should reset typedef membership to the snapshot");
#else
  expect_true(parser.has_typedef_name("Keep"),
              "test fixture should preserve the baseline typedef when heavy snapshots are off");
#endif
}

void test_parser_keeps_qualified_bindings_string_keyed() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

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
      static_cast<int>(parser.shared_lookup_state_.parser_symbols.size());

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
  expect_true(parser.shared_lookup_state_.parser_name_tables.find_identifier(
                  "ns::Type") ==
                  c4c::Parser::kInvalidSymbol,
              "qualified typedef names should not intern composed strings");
  expect_true(parser.shared_lookup_state_.parser_name_tables.find_identifier(
                  "ns::value") ==
                  c4c::Parser::kInvalidSymbol,
              "qualified value names should not intern composed strings");
  expect_eq_int(static_cast<int>(parser.shared_lookup_state_.parser_symbols.size()),
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

void test_parser_last_using_alias_name_prefers_text_id_storage() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  parser.set_last_using_alias_name("ns::Alias");
  expect_true(parser.active_context_state_.last_using_alias_name_text_id !=
                  c4c::kInvalidText,
              "using-alias bookkeeping should retain a valid TextId");
  expect_eq(parser.last_using_alias_name_text(), "ns::Alias",
            "using-alias bookkeeping should resolve through the parser text table");

  parser.active_context_state_.last_using_alias_name = "corrupted";
  expect_eq(parser.last_using_alias_name_text(), "ns::Alias",
            "using-alias bookkeeping should prefer the TextId carrier over raw string storage");

  parser.clear_last_using_alias_name();
  expect_true(parser.active_context_state_.last_using_alias_name_text_id ==
                  c4c::kInvalidText,
              "clearing using-alias bookkeeping should drop the TextId carrier");
  expect_true(parser.last_using_alias_name_text().empty(),
              "clearing using-alias bookkeeping should leave no visible alias text");
}

void test_parser_parse_qualified_name_populates_atom_symbol_ids() {
  c4c::Lexer lexer("::ns::inner::Type value;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

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

void test_parser_apply_qualified_name_preserves_text_ids_on_ast_nodes() {
  c4c::Lexer lexer("::ns::inner::Type value;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  const c4c::Parser::QualifiedNameRef qn = parser.parse_qualified_name();
  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 1);
  parser.apply_qualified_name(ref, qn);

  expect_true(ref->unqualified_text_id == qn.base_text_id,
              "AST qualified-name handoff should keep the parser TextId for the base name");
  expect_true(ref->qualifier_text_ids != nullptr,
              "AST qualified-name handoff should allocate parallel qualifier TextIds");
  expect_true(ref->qualifier_text_ids[0] == qn.qualifier_text_ids[0] &&
                  ref->qualifier_text_ids[1] == qn.qualifier_text_ids[1],
              "AST qualified-name handoff should preserve parser TextIds for qualifier segments");
  expect_eq(lexer.text_table().lookup(ref->qualifier_text_ids[0]), "ns",
            "AST qualifier TextIds should resolve through the shared parser text table");
  expect_eq(lexer.text_table().lookup(ref->qualifier_text_ids[1]), "inner",
            "AST qualifier TextIds should resolve through the shared parser text table");
}

void test_parser_injected_token_helpers_preserve_qualified_name_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

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
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Token seed{};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
  expect_true(parser.is_clearly_value_template_arg(nullptr, 0),
              "value-like template lookahead should read parser-owned token spelling");
}

void test_parser_capture_template_arg_expr_uses_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Token seed{};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
      parser.make_injected_token(seed, c4c::TokenKind::Plus, "+"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "1"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  };
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

void test_parser_type_start_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec typedef_ts{};
  typedef_ts.array_size = -1;
  typedef_ts.inner_rank = -1;
  typedef_ts.base = c4c::TB_INT;
  parser.register_typedef_binding("TypeAlias", typedef_ts, true);

  c4c::Token seed{};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
  expect_true(parser.is_type_start(),
              "type-start classification should use parser-owned identifier spelling");

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ConceptName"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
  parser.binding_state_.concept_names.insert("ConceptName");
  expect_true(!parser.looks_like_unresolved_identifier_type_head(0),
              "identifier-type probes should use parser-owned spelling when excluding concept names");
}

void test_parser_exception_specs_and_attributes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "noexcept"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  };
  parser.skip_attributes();
  expect_eq_int(parser.pos_, 4,
                "attribute skipping should consume identifier-spelled noexcept clauses");
  expect_eq(parser.token_spelling(parser.cur()), "after",
            "attribute skipping should leave the next token in place");

  parser.pos_ = 0;
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "noexcept"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "throw"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  };
  parser.skip_exception_spec();
  expect_eq_int(parser.pos_, 8,
                "exception-spec skipping should consume identifier-spelled noexcept/throw clauses");
  expect_eq(parser.token_spelling(parser.cur()), "after",
            "exception-spec skipping should leave the next token in place");

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  parser.pos_ = 0;
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::KwAttribute, "__attribute__"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "aligned"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "32"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  parser.parse_attributes(&ts);
  expect_eq_int(ts.align_bytes, 32,
                "GNU attribute parsing should use parser-owned attribute spellings");
}

void test_parser_typeof_like_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::TypeSpec typedef_ts{};
  typedef_ts.array_size = -1;
  typedef_ts.inner_rank = -1;
  typedef_ts.base = c4c::TB_INT;
  parser.register_typedef_binding("TypeAlias", typedef_ts, true);

  c4c::TypeSpec value_ts{};
  value_ts.array_size = -1;
  value_ts.inner_rank = -1;
  value_ts.base = c4c::TB_DOUBLE;
  parser.register_var_type_binding("value", value_ts);

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "__underlying_type"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  };
  parser.pos_ = 0;
  (void)parser.parse_base_type();
  expect_eq_int(parser.pos_, 4,
                "builtin transform parsing should use parser-owned builtin spellings");
  expect_eq(parser.token_spelling(parser.cur()), "after",
            "builtin transform parsing should leave the next token in place");

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::KwTypeof, "typeof"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "nullptr"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  parser.pos_ = 0;
  c4c::TypeSpec nullptr_ts = parser.parse_base_type();
  expect_true(nullptr_ts.base == c4c::TB_VOID && nullptr_ts.ptr_level == 1,
              "typeof-like nullptr probes should use parser-owned spellings");

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::KwTypeof, "typeof"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  parser.pos_ = 0;
  c4c::TypeSpec value_operand_ts = parser.parse_base_type();
  expect_true(value_operand_ts.base == c4c::TB_DOUBLE,
              "typeof-like identifier operand probes should use parser-owned spellings");

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::KwTypeof, "typeof"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::FloatLit, "1.0f"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  parser.pos_ = 0;
  c4c::TypeSpec float_operand_ts = parser.parse_base_type();
  expect_true(float_operand_ts.base == c4c::TB_FLOAT,
              "typeof-like float literal suffix probes should use parser-owned spellings");
}

void test_parser_parse_base_type_identifier_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "__float128"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "gnu_value"),
  };
  parser.pos_ = 0;
  c4c::TypeSpec gnu_floatn_ts = parser.parse_base_type();
  expect_true(gnu_floatn_ts.base == c4c::TB_LONGDOUBLE,
              "parse_base_type should map GNU __float128 spelling onto the fixed-width float path");
  expect_eq(parser.token_spelling(parser.cur()), "gnu_value",
            "__float128 parsing should leave the declarator token in place");

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "_Float128"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
  parser.pos_ = 0;
  c4c::TypeSpec floatn_ts = parser.parse_base_type();
  expect_true(floatn_ts.base == c4c::TB_LONGDOUBLE,
              "parse_base_type should use parser-owned spelling for fixed-width float keywords");
  expect_eq(parser.token_spelling(parser.cur()), "value",
            "fixed-width float keyword parsing should leave the declarator token in place");

  c4c::TypeSpec typedef_ts{};
  typedef_ts.array_size = -1;
  typedef_ts.inner_rank = -1;
  typedef_ts.base = c4c::TB_INT;
  parser.register_typedef_binding("TypeAlias", typedef_ts, true);

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
  parser.pos_ = 0;
  parser.clear_last_resolved_typedef();
  c4c::TypeSpec alias_ts = parser.parse_base_type();
  expect_true(alias_ts.base == c4c::TB_INT,
              "parse_base_type should resolve typedef names via parser-owned spelling");
  expect_eq(parser.active_context_state_.last_resolved_typedef, "TypeAlias",
            "typedef resolution should preserve the parser-owned identifier spelling");

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ForwardDecl"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };
  parser.pos_ = 0;
  c4c::TypeSpec unresolved_ts = parser.parse_base_type();
  expect_true(unresolved_ts.tag != nullptr,
              "unresolved identifier fallback should produce a placeholder type tag");
  expect_eq(unresolved_ts.tag, "ForwardDecl",
            "unresolved identifier fallback should use parser-owned spelling");
}

void test_parser_local_visible_typedef_cast_uses_scope_lookup() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "1"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  parser.pos_ = 0;
  parser.clear_last_resolved_typedef();

  c4c::Node* expr = parser.parse_unary();

  expect_true(expr != nullptr && expr->kind == c4c::NK_CAST,
              "local visible typedef casts should parse through the scope-local typedef facade");
  expect_true(expr->type.base == c4c::TB_INT,
              "local visible typedef casts should recover the bound typedef type");
  expect_true(expr->left != nullptr && expr->left->kind == c4c::NK_INT_LIT &&
                  expr->left->ival == 1,
              "local visible typedef casts should keep the cast operand intact");
  expect_eq(parser.active_context_state_.last_resolved_typedef, "Alias",
            "local visible typedef casts should preserve the visible typedef spelling");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_decode_type_ref_text_uses_local_visible_scope_lookup() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::TypeSpec decoded{};
  expect_true(parser.decode_type_ref_text("Alias", &decoded),
              "type-ref decoding should consult parser-local visible typedef bindings");
  expect_true(decoded.base == c4c::TB_INT,
              "type-ref decoding should recover the bound local visible typedef type");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_dependent_typename_uses_local_visible_owner_alias() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::Node* owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  owner->name = arena.strdup("Box");
  owner->member_typedef_names = arena.alloc_array<const char*>(1);
  owner->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  owner->n_member_typedefs = 1;
  owner->member_typedef_names[0] = arena.strdup("type");
  owner->member_typedef_types[0].array_size = -1;
  owner->member_typedef_types[0].inner_rank = -1;
  owner->member_typedef_types[0].base = c4c::TB_INT;
  parser.definition_state_.struct_tag_def_map["Box"] = owner;

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_STRUCT;
  alias_ts.tag = arena.strdup("Box");

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  };
  parser.pos_ = 0;

  std::string resolved_name;
  expect_true(parser.parse_dependent_typename_specifier(&resolved_name),
              "dependent typename parsing should accept a scope-local owner alias");

  expect_eq(resolved_name, "Alias::type",
            "dependent typename parsing should preserve the cached alias-member spelling");
  expect_true(parser.find_typedef_type(resolved_name) != nullptr &&
                  parser.find_typedef_type(resolved_name)->base == c4c::TB_INT,
              "dependent typename parsing should resolve a scope-local owner alias through the visible typedef facade");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_is_typedef_name_uses_local_visible_scope_lookup() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  expect_true(parser.is_typedef_name("Alias"),
              "typedef-name probes should treat parser-local visible bindings as typedefs");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_conflicting_user_typedef_binding_uses_local_visible_scope_lookup() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::C);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  c4c::TypeSpec different_ts = alias_ts;
  different_ts.base = c4c::TB_FLOAT;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);
  const c4c::Parser::SymbolId alias_symbol =
      parser.shared_lookup_state_.parser_name_tables.intern_identifier("Alias");
  parser.shared_lookup_state_.parser_name_tables.user_typedefs.insert(alias_symbol);

  expect_true(!parser.has_conflicting_user_typedef_binding("Alias", alias_ts),
              "typedef conflict checks should accept matching scope-local visible typedefs");
  expect_true(parser.has_conflicting_user_typedef_binding("Alias", different_ts),
              "typedef conflict checks should reject incompatible scope-local visible typedefs");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_record_body_context_keeps_visible_template_origin_lookup_local() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  expect_true(parser.find_typedef_type("Alias") == nullptr,
              "local visible typedef fixtures should not populate the flat typedef table");

  std::string saved_struct_tag;
  std::string struct_source_name;
  parser.begin_record_body_context("Widget", "Alias", &saved_struct_tag,
                                   &struct_source_name);

  expect_eq(struct_source_name, "Alias",
            "record body setup should preserve the template origin spelling");
  expect_true(parser.find_typedef_type("Alias") == nullptr,
              "record body setup should not synthesize a flat typedef binding when a visible local alias already exists");
  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type("Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "record body setup should continue resolving the template origin through the visible typedef facade");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_visible_type_alias_uses_scope_local_typedef_facade() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  const c4c::TextId target_text = texts.intern("Target");
  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(target_text, target_ts);
  parser.namespace_state_.using_value_aliases[0][alias_text] = {
      parser.intern_semantic_name_key("Target"), "corrupted"};

  expect_eq(parser.resolve_visible_type_name("Alias"), "Target",
            "unqualified value aliases should resolve through the visible typedef facade when the target is scope-local");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_visible_type_alias_resolves_scope_local_target_type() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  const c4c::TextId target_text = texts.intern("Target");
  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(target_text, target_ts);
  parser.namespace_state_.using_value_aliases[0][alias_text] = {
      parser.intern_semantic_name_key("Target"), "corrupted"};

  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type("Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "visible typedef aliases should resolve scope-local target types through the visible facade");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_visible_type_alias_keeps_qualified_target_resolution() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.register_typedef_binding("ns::Target", target_ts, true);
  parser.namespace_state_.using_value_aliases[0][alias_text] = {
      parser.intern_semantic_name_key("ns::Target"), "corrupted"};

  expect_eq(parser.resolve_visible_type_name("Alias"), "ns::Target",
            "qualified value aliases should keep namespace-qualified typedef resolution intact");
  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type("Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "qualified value aliases should still resolve through the existing namespace-visible path");
}

void test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_TYPEDEF;
  alias_ts.tag = arena.strdup("Target");

  const c4c::TextId target_text = texts.intern("Target");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(target_text, target_ts);

  const c4c::TypeSpec resolved = parser.resolve_typedef_type_chain(alias_ts);
  expect_true(resolved.base == c4c::TB_INT,
              "typedef-chain resolution should re-probe local visible typedef targets before flat lookup");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_using_value_import_keeps_structured_target_key() {
  c4c::Lexer lexer("using ns::Target;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;
  parser.register_var_type_binding("ns::Target", target_ts);

  (void)parser.parse_top_level();

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "using-import fixture should intern the alias text");
  auto alias_it = parser.namespace_state_.using_value_aliases[0].find(alias_text);
  expect_true(alias_it != parser.namespace_state_.using_value_aliases[0].end(),
              "using-import fixture should record a value-alias entry");
  alias_it->second.compatibility_name = "corrupted";
  expect_eq(parser.resolve_visible_value_name("Target"), "ns::Target",
            "using-import lookup should prefer the structured target key over the compatibility bridge");
}

void test_parser_global_using_value_import_keeps_global_target_resolution() {
  c4c::Lexer lexer("using ::Target;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;
  parser.register_var_type_binding("Target", target_ts);

  (void)parser.parse_top_level();

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "global using-import fixture should intern the alias text");
  auto alias_it = parser.namespace_state_.using_value_aliases[0].find(alias_text);
  expect_true(alias_it != parser.namespace_state_.using_value_aliases[0].end(),
              "global using-import fixture should record a value-alias entry");
  alias_it->second.compatibility_name = "corrupted";
  expect_eq(parser.resolve_visible_value_name("Target"), "Target",
            "global using-import lookup should keep the global target spelling instead of introducing a leading scope bridge");
}

void test_parser_visible_value_alias_resolves_scope_local_target_type() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  const c4c::TextId target_text = texts.intern("Target");
  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_value(target_text, target_ts);
  parser.namespace_state_.using_value_aliases[0][alias_text] = {
      parser.intern_semantic_name_key("Target"), "corrupted"};

  const c4c::TypeSpec* visible_alias = parser.find_visible_var_type("Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "visible value aliases should resolve scope-local target types through the visible facade");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible value scope");
}

void test_parser_template_member_suffix_probe_uses_token_spelling() {
  c4c::Lexer lexer(
      "template<int N>\n"
      "struct Trait { typedef int type; };\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  (void)parser.parse_top_level();
  expect_true(parser.find_template_struct_primary("Trait") != nullptr,
              "template struct fixture should register before injected suffix probing");

  c4c::Token seed{};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  };
  parser.pos_ = 0;
  const c4c::TypeSpec member_ts = parser.parse_base_type();
  expect_true(member_ts.base == c4c::TB_INT,
              "template-member suffix probes should use parser-owned spelling");
}

void test_parser_template_type_arg_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.push_template_scope(
      c4c::Parser::TemplateScopeKind::FreeFunctionTemplate,
      {{.name = "T", .is_nttp = false}});
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  };
  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.try_parse_template_type_arg(&arg),
              "template type-argument probes should use parser-owned spelling");
  expect_true(!arg.is_value,
              "template scope type parameters should stay classified as type arguments");
  expect_true(arg.type.base == c4c::TB_TYPEDEF,
              "template scope type parameters should parse as placeholder typedef types");
  expect_eq(arg.type.tag, "T",
            "template type-argument parsing should preserve the parser-owned spelling");
  expect_eq_int(parser.pos_, 1,
                "template type-argument parsing should stop before the template close");
  parser.pop_template_scope();
}

void test_parser_template_type_arg_uses_visible_scope_local_alias() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  const c4c::TextId target_text = texts.intern("Target");
  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(target_text, target_ts);
  parser.namespace_state_.using_value_aliases[0][alias_text] = {
      parser.intern_semantic_name_key("Target"), "corrupted"};

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  };
  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.try_parse_template_type_arg(&arg),
              "template type-argument probes should treat visible lexical aliases as type heads");
  expect_true(!arg.is_value,
              "visible lexical aliases should stay classified as type arguments");
  expect_true(arg.type.base == c4c::TB_TYPEDEF,
              "visible lexical aliases should parse as typedef-like type arguments");
  expect_eq(arg.type.tag, "Alias",
            "template type-argument parsing should preserve the alias spelling");
  expect_eq_int(parser.pos_, 1,
                "visible lexical alias type-argument parsing should stop before the template close");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_alias_template_value_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.template_state_.alias_template_info["Alias"] = {};
  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  expect_true(!parser.is_clearly_value_template_arg(nullptr, 0),
              "direct alias-template probes should use parser-owned spelling");

  c4c::TextTable resolved_texts;
  c4c::FileTable resolved_files;
  c4c::Parser resolved_parser({}, arena, &resolved_texts, &resolved_files,
                              c4c::SourceProfile::CppSubset);
  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;
  const c4c::TextId alias_text = resolved_texts.intern("Alias");
  resolved_parser.register_typedef_binding("ns::Alias", alias_ts, true);
  resolved_parser.namespace_state_.using_value_aliases[0][alias_text] = {
      resolved_parser.intern_semantic_name_key("ns::Alias"), "corrupted"};
  resolved_parser.template_state_.alias_template_info["ns::Alias"] = {};
  resolved_parser.tokens_ = {
      resolved_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };
  expect_true(!resolved_parser.is_clearly_value_template_arg(nullptr, 0),
              "resolved alias-template probes should use parser-owned spelling");
}

void test_parser_typename_template_parameter_probe_uses_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "typename"),
  };
  expect_true(parser.classify_typename_template_parameter() ==
                  c4c::Parser::TypenameTemplateParamKind::TypeParameter,
              "typename template-parameter probe should use parser-owned spelling");
}

void test_parser_post_pointer_qualifier_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "_Nullable"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "restrict"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  };
  parser.consume_declarator_post_pointer_qualifiers();

  expect_eq(parser.token_spelling(parser.cur()), "after",
            "post-pointer qualifier probes should use parser-owned spelling");
}

void test_parser_qualified_declarator_name_uses_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.tokens_ = {
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "inner"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
  };
  std::string qualified_name;
  expect_true(parser.parse_qualified_declarator_name(&qualified_name),
              "qualified declarator names should parse from injected token spelling");
  expect_eq(qualified_name, "::ns::inner::Value",
            "qualified declarator names should materialize parser-owned spelling");
}

}  // namespace

int main() {
  test_link_name_table_reuses_text_table_storage();
  test_parser_reuses_symbol_ids_for_repeated_identifier_text_ids();
  test_parser_missing_text_id_is_rejected();
  test_parser_string_wrappers_use_symbol_id_keyed_name_tables();
  test_parser_heavy_snapshot_restores_symbol_id_keyed_tables();
  test_parser_keeps_qualified_bindings_string_keyed();
  test_parser_last_using_alias_name_prefers_text_id_storage();
  test_parser_parse_qualified_name_populates_atom_symbol_ids();
  test_parser_apply_qualified_name_preserves_text_ids_on_ast_nodes();
  test_parser_injected_token_helpers_preserve_qualified_name_spelling();
  test_parser_value_like_template_lookahead_uses_token_spelling();
  test_parser_capture_template_arg_expr_uses_token_spelling();
  test_parser_type_start_probes_use_token_spelling();
  test_parser_exception_specs_and_attributes_use_token_spelling();
  test_parser_typeof_like_probes_use_token_spelling();
  test_parser_parse_base_type_identifier_probes_use_token_spelling();
  test_parser_local_visible_typedef_cast_uses_scope_lookup();
  test_parser_decode_type_ref_text_uses_local_visible_scope_lookup();
  test_parser_dependent_typename_uses_local_visible_owner_alias();
  test_parser_is_typedef_name_uses_local_visible_scope_lookup();
  test_parser_conflicting_user_typedef_binding_uses_local_visible_scope_lookup();
  test_parser_record_body_context_keeps_visible_template_origin_lookup_local();
  test_parser_visible_type_alias_uses_scope_local_typedef_facade();
  test_parser_visible_type_alias_resolves_scope_local_target_type();
  test_parser_visible_type_alias_keeps_qualified_target_resolution();
  test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup();
  test_parser_using_value_import_keeps_structured_target_key();
  test_parser_global_using_value_import_keeps_global_target_resolution();
  test_parser_visible_value_alias_resolves_scope_local_target_type();
  test_parser_template_member_suffix_probe_uses_token_spelling();
  test_parser_template_type_arg_probes_use_token_spelling();
  test_parser_template_type_arg_uses_visible_scope_local_alias();
  test_parser_alias_template_value_probes_use_token_spelling();
  test_parser_typename_template_parameter_probe_uses_token_spelling();
  test_parser_post_pointer_qualifier_probes_use_token_spelling();
  test_parser_qualified_declarator_name_uses_token_spelling();

  std::cout << "PASS: frontend_parser_tests\n";
  return 0;
}
