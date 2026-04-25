#include "lexer.hpp"
#include "impl/parser_impl.hpp"
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
  expect_true(parser.parser_symbols_use_text_table_for_testing(
                  &lexer.text_table()),
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
      parser.parser_symbol_tables().find_identifier("Value");
  const c4c::Parser::SymbolId var_symbol =
      parser.parser_symbol_tables().find_identifier("counter");

  expect_true(typedef_symbol != c4c::Parser::kInvalidSymbol,
              "typedef wrapper should intern a valid SymbolId");
  expect_true(parser.parser_symbol_tables().is_typedef(typedef_symbol),
              "typedef wrapper should store membership by SymbolId");
  expect_true(parser.parser_symbol_tables().user_typedefs.count(
                  typedef_symbol) == 1,
              "user typedef wrapper should track user typedef membership by SymbolId");
  expect_true(parser.has_typedef_name("Value"),
              "string-facing typedef lookup should still work");
  expect_true(parser.has_typedef_type("Value"),
              "typedef type wrapper should still resolve through string lookup");
  expect_true(parser.find_typedef_type("Value") != nullptr &&
                  parser.find_typedef_type("Value")->base == c4c::TB_INT,
              "typedef type wrapper should recover the stored TypeSpec");
  expect_true(parser.parser_symbol_tables().lookup_typedef_type(
                  typedef_symbol) != nullptr &&
                  parser.parser_symbol_tables()
                          .lookup_typedef_type(typedef_symbol)
                          ->base == c4c::TB_INT,
              "symbol-keyed typedef map should recover the stored TypeSpec");

  expect_true(var_symbol != c4c::Parser::kInvalidSymbol,
              "var-type wrapper should intern a valid SymbolId");
  expect_true(parser.parser_symbol_tables().var_types.count(var_symbol) == 1,
              "var-type wrapper should store var bindings by SymbolId");
  expect_true(parser.has_var_type("counter"),
              "string-facing var-type lookup should still work");
  expect_true(parser.find_var_type("counter") != nullptr &&
                  parser.find_var_type("counter")->base == c4c::TB_DOUBLE,
              "var-type wrapper should recover the stored TypeSpec");
  const c4c::QualifiedNameKey counter_key =
      parser.intern_semantic_name_key("counter");
  expect_true(parser.has_structured_var_type(counter_key),
              "var-type wrapper should also populate structured value storage");
  expect_true(parser.find_structured_var_type(counter_key) != nullptr &&
                  parser.find_structured_var_type(counter_key)->base ==
                      c4c::TB_DOUBLE,
              "structured value storage should recover the stored TypeSpec");
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
      parser.parser_symbol_tables().typedefs.size();
  const auto snapshot = parser.save_state();
  const c4c::QualifiedNameKey scratch_key =
      parser.intern_semantic_name_key("scratch");
  parser.register_typedef_binding("Temp", temp_ts, true);
  parser.register_var_type_binding("scratch", temp_ts);
  parser.restore_state(snapshot);

  expect_true(parser.has_typedef_name("Keep"),
              "restore_state should preserve typedefs that existed before the snapshot");
  expect_true(!parser.has_typedef_name("Temp"),
              "restore_state should roll back typedefs added after the snapshot");
  expect_true(!parser.has_var_type("scratch"),
              "restore_state should roll back var bindings added after the snapshot");
  expect_true(!parser.has_structured_var_type(scratch_key),
              "restore_state should roll back structured var bindings added after the snapshot");
  expect_eq_int(
      static_cast<int>(parser.parser_symbol_tables().typedefs.size()),
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
      static_cast<int>(parser.parser_symbol_count_for_testing());

  parser.register_typedef_binding("ns::Type", typedef_ts, true);
  parser.register_var_type_binding("ns::value", var_ts);

  expect_true(parser.has_typedef_name("ns::Type"),
              "qualified typedef membership should remain lookupable");
  expect_true(parser.has_typedef_type("ns::Type"),
              "qualified typedef types should remain lookupable");
  expect_true(parser.find_typedef_type("ns::Type") != nullptr &&
                  parser.find_typedef_type("ns::Type")->base == c4c::TB_INT,
              "qualified typedef type lookup should recover the stored TypeSpec");
  const c4c::QualifiedNameKey value_key =
      parser.intern_semantic_name_key("ns::value");
  expect_true(parser.has_structured_var_type(value_key),
              "qualified value bindings should populate structured storage");
  expect_true(parser.find_structured_var_type(value_key) != nullptr &&
                  parser.find_structured_var_type(value_key)->base ==
                      c4c::TB_DOUBLE,
              "qualified structured value lookup should recover the stored TypeSpec");
  expect_true(parser.has_var_type("ns::value"),
              "qualified value bindings should remain string-lookupable");
  expect_true(parser.find_var_type("ns::value") != nullptr &&
                  parser.find_var_type("ns::value")->base == c4c::TB_DOUBLE,
              "qualified value lookup should recover the stored TypeSpec");
  expect_true(parser.parser_symbol_tables().find_identifier(
                  "ns::Type") ==
                  c4c::Parser::kInvalidSymbol,
              "qualified typedef names should not intern composed strings");
  expect_true(parser.parser_symbol_tables().find_identifier(
                  "ns::value") ==
                  c4c::Parser::kInvalidSymbol,
              "qualified value names should not intern composed strings");
  expect_eq_int(static_cast<int>(parser.parser_symbol_count_for_testing()),
                symbol_count_before,
                "qualified bindings should not change the atom-symbol table size");

#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const auto snapshot = parser.save_state();
  const c4c::QualifiedNameKey scratch_key =
      parser.intern_semantic_name_key("ns::scratch");

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
  expect_true(!parser.has_structured_var_type(scratch_key),
              "restore_state should roll back qualified values from structured storage");
#endif
}

void test_parser_structured_value_registration_keeps_legacy_lookup() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

  c4c::TypeSpec value_ts{};
  value_ts.array_size = -1;
  value_ts.inner_rank = -1;
  value_ts.base = c4c::TB_LONG;

  const c4c::QualifiedNameKey value_key =
      parser.intern_semantic_name_key("ns::registered");
  parser.register_structured_var_type_binding(value_key, value_ts);

  expect_true(parser.has_structured_var_type(value_key),
              "structured value registration should populate structured storage");
  expect_true(parser.find_structured_var_type(value_key) != nullptr &&
                  parser.find_structured_var_type(value_key)->base ==
                      c4c::TB_LONG,
              "structured value registration should recover the stored TypeSpec");
  expect_true(parser.has_var_type("ns::registered"),
              "structured value registration should keep the legacy string table populated");
  expect_true(parser.find_var_type("ns::registered") != nullptr &&
                  parser.find_var_type("ns::registered")->base ==
                      c4c::TB_LONG,
              "legacy string lookup should agree with structured value storage");

#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const auto snapshot = parser.save_state();
  const c4c::QualifiedNameKey temp_key =
      parser.intern_semantic_name_key("ns::temporary_registered");
  parser.register_structured_var_type_binding(temp_key, value_ts);
  parser.restore_state(snapshot);

  expect_true(!parser.has_structured_var_type(temp_key),
              "restore_state should roll back direct structured value bindings");
  expect_true(!parser.has_var_type("ns::temporary_registered"),
              "restore_state should roll back the legacy mirror for direct structured value bindings");
#endif
}

void test_parser_last_using_alias_name_prefers_text_id_storage() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  parser.set_last_using_alias_name(parser.intern_semantic_name_key("ns::Alias"));
  expect_true(parser.has_last_using_alias_name_text_id_for_testing(),
              "using-alias bookkeeping should retain a valid TextId");
  expect_eq(parser.last_using_alias_name_text(), "Alias",
            "using-alias bookkeeping should resolve through the parser text table");

  parser.replace_last_using_alias_name_fallback_for_testing("corrupted");
  expect_eq(parser.last_using_alias_name_text(), "Alias",
            "using-alias bookkeeping should prefer the TextId carrier over raw string storage");

  parser.clear_last_using_alias_name();
  expect_true(!parser.has_last_using_alias_name_text_id_for_testing(),
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
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
  });

  expect_true(parser.token_at_for_testing(0).text_id != c4c::kInvalidText,
              "injected identifier tokens should receive a stable text id");
  expect_eq(parser.token_spelling(parser.token_at_for_testing(0)), "ns",
            "parser token spelling helper should recover injected identifier spelling");

  c4c::Parser::QualifiedNameRef qn;
  expect_true(parser.peek_qualified_name(&qn, /*allow_global=*/true),
              "qualified-name peeking should accept helper-built injected tokens");
  expect_eq(qn.spelled(), "ns::Value",
            "qualified-name peeking should preserve helper-built injected spelling");

  const c4c::Parser::SymbolId symbol = parser.symbol_id_for_token(parser.token_at_for_testing(2));
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
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
  expect_true(parser.is_clearly_value_template_arg(nullptr, 0),
              "value-like template lookahead should read parser-owned token spelling");
}

void test_parser_capture_template_arg_expr_uses_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Token seed{};
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
      parser.make_injected_token(seed, c4c::TokenKind::Plus, "+"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "1"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });
  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.capture_template_arg_expr(0, &arg),
              "template arg expression capture should succeed for injected tokens");
  expect_true(arg.nttp_name != nullptr,
              "captured template arg expression should preserve text in nttp_name");
  expect_eq(arg.nttp_name, "$expr:Value+1",
            "template arg expression capture should use parser-owned token spelling");
  expect_eq_int(parser.token_cursor_for_testing(), 3,
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
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
  expect_true(parser.is_type_start(),
              "type-start classification should use parser-owned identifier spelling");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ConceptName"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
  const c4c::TextId concept_text_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ConceptName");
  parser.register_concept_name_for_testing(concept_text_id);
  expect_true(!parser.looks_like_unresolved_identifier_type_head(0),
              "identifier-type probes should use parser-owned spelling when excluding concept names");

  const c4c::TextId ns_text_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context_id =
      parser.ensure_named_namespace_context(0, ns_text_id, "ns");
  const c4c::TextId qualified_concept_text_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ScopedConcept");
  parser.register_concept_name_in_context(ns_context_id,
                                          qualified_concept_text_id,
                                          "ScopedConcept");
  expect_true(parser.is_concept_name("ns::ScopedConcept"),
              "qualified concept lookup should use structured concept keys");
}

void test_parser_exception_specs_and_attributes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "noexcept"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });
  parser.skip_attributes();
  expect_eq_int(parser.token_cursor_for_testing(), 4,
                "attribute skipping should consume identifier-spelled noexcept clauses");
  expect_eq(parser.token_spelling(parser.cur()), "after",
            "attribute skipping should leave the next token in place");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "noexcept"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "throw"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });
  parser.skip_exception_spec();
  expect_eq_int(parser.token_cursor_for_testing(), 8,
                "exception-spec skipping should consume identifier-spelled noexcept/throw clauses");
  expect_eq(parser.token_spelling(parser.cur()), "after",
            "exception-spec skipping should leave the next token in place");

  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwAttribute, "__attribute__"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "aligned"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "32"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
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

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "__underlying_type"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });
  (void)parser.parse_base_type();
  expect_eq_int(parser.token_cursor_for_testing(), 4,
                "builtin transform parsing should use parser-owned builtin spellings");
  expect_eq(parser.token_spelling(parser.cur()), "after",
            "builtin transform parsing should leave the next token in place");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypeof, "typeof"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "nullptr"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
  c4c::TypeSpec nullptr_ts = parser.parse_base_type();
  expect_true(nullptr_ts.base == c4c::TB_VOID && nullptr_ts.ptr_level == 1,
              "typeof-like nullptr probes should use parser-owned spellings");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypeof, "typeof"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
  c4c::TypeSpec value_operand_ts = parser.parse_base_type();
  expect_true(value_operand_ts.base == c4c::TB_DOUBLE,
              "typeof-like identifier operand probes should use parser-owned spellings");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypeof, "typeof"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::FloatLit, "1.0f"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
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

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "__float128"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "gnu_value"),
  });
  c4c::TypeSpec gnu_floatn_ts = parser.parse_base_type();
  expect_true(gnu_floatn_ts.base == c4c::TB_LONGDOUBLE,
              "parse_base_type should map GNU __float128 spelling onto the fixed-width float path");
  expect_eq(parser.token_spelling(parser.cur()), "gnu_value",
            "__float128 parsing should leave the declarator token in place");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "_Float128"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
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

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "TypeAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
  parser.clear_last_resolved_typedef();
  c4c::TypeSpec alias_ts = parser.parse_base_type();
  expect_true(alias_ts.base == c4c::TB_INT,
              "parse_base_type should resolve typedef names via parser-owned spelling");
  expect_eq(parser.last_resolved_typedef_text(), "TypeAlias",
            "typedef resolution should preserve the parser-owned identifier spelling");

  c4c::TypeSpec local_alias_ts{};
  local_alias_ts.array_size = -1;
  local_alias_ts.inner_rank = -1;
  local_alias_ts.base = c4c::TB_DOUBLE;
  const c4c::TextId local_alias_text = texts.intern("LocalAlias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(local_alias_text, local_alias_ts);

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "LocalAlias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
  parser.clear_last_resolved_typedef();
  c4c::TypeSpec local_alias_result = parser.parse_base_type();
  expect_true(local_alias_result.base == c4c::TB_DOUBLE,
              "parse_base_type should resolve scope-local typedef names through the lexical scope facade");
  expect_eq(parser.last_resolved_typedef_text(), "LocalAlias",
            "scope-local typedef resolution should preserve the parser-owned identifier spelling");
  expect_eq(parser.token_spelling(parser.cur()), "value",
            "scope-local typedef resolution should leave the declarator token in place");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ForwardDecl"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });
  c4c::TypeSpec unresolved_ts = parser.parse_base_type();
  expect_true(unresolved_ts.tag != nullptr,
              "unresolved identifier fallback should produce a placeholder type tag");
  expect_eq(unresolved_ts.tag, "ForwardDecl",
            "unresolved identifier fallback should use parser-owned spelling");
}

void test_parser_is_type_start_uses_local_visible_typedef_scope() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_DOUBLE;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });

  expect_true(parser.is_type_start(),
              "type-head probes should consult parser-local visible typedef bindings before spelling-based fallback");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
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
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "1"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
  parser.clear_last_resolved_typedef();

  c4c::Node* expr = c4c::parse_unary(parser);

  expect_true(expr != nullptr && expr->kind == c4c::NK_CAST,
              "local visible typedef casts should parse through the scope-local typedef facade");
  expect_true(expr->type.base == c4c::TB_INT,
              "local visible typedef casts should recover the bound typedef type");
  expect_true(expr->left != nullptr && expr->left->kind == c4c::NK_INT_LIT &&
                  expr->left->ival == 1,
              "local visible typedef casts should keep the cast operand intact");
  expect_eq(parser.last_resolved_typedef_text(), "Alias",
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
  parser.register_struct_definition_for_testing("Box", owner);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_STRUCT;
  alias_ts.tag = arena.strdup("Box");

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  });

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
      parser.parser_symbol_tables().intern_identifier("Alias");
  parser.parser_symbol_tables().user_typedefs.insert(alias_symbol);

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
  c4c::begin_record_body_context(parser, "Widget", "Alias", &saved_struct_tag,
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
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("Target"), "corrupted");

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
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("Target"), "corrupted");

  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type("Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "visible typedef aliases should resolve scope-local target types through the visible facade");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_visible_type_alias_uses_token_text_id_scope_lookup() {
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
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("Target"), "corrupted");

  const c4c::TypeSpec* visible_alias =
      parser.find_visible_typedef_type(alias_text, "Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "token TextId visible typedef probes should consult the local lexical scope first");
  expect_eq(parser.resolve_visible_type_name(alias_text, "Alias"), "Target",
            "token TextId visible typedef probes should still resolve through the visible alias facade");

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
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("ns::Target"),
      "corrupted");

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

  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "using-import fixture should intern the alias text");
  expect_true(parser.replace_using_value_alias_compatibility_name_for_testing(
                  0, alias_text, "corrupted"),
              "using-import fixture should record a value-alias entry");
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

  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "global using-import fixture should intern the alias text");
  expect_true(parser.replace_using_value_alias_compatibility_name_for_testing(
                  0, alias_text, "corrupted"),
              "global using-import fixture should record a value-alias entry");
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
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("Target"), "corrupted");

  const c4c::TypeSpec* visible_alias = parser.find_visible_var_type("Alias");
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "visible value aliases should resolve scope-local target types through the visible facade");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible value scope");
}

void test_parser_register_local_bindings_keep_flat_tables_empty() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  c4c::TypeSpec value_ts{};
  value_ts.array_size = -1;
  value_ts.inner_rank = -1;
  value_ts.base = c4c::TB_DOUBLE;

  parser.push_local_binding_scope();
  parser.register_typedef_binding("Alias", alias_ts, true);
  parser.register_var_type_binding("value", value_ts);

  expect_true(parser.find_typedef_type("Alias") == nullptr,
              "local typedef registration should keep the flat typedef table empty");
  expect_true(parser.find_visible_typedef_type("Alias") != nullptr &&
                  parser.find_visible_typedef_type("Alias")->base == c4c::TB_INT,
              "local typedef registration should resolve through lexical scope storage");
  expect_true(parser.find_var_type("value") == nullptr,
              "local value registration should keep the flat value table empty");
  expect_true(parser.find_visible_var_type("value") != nullptr &&
                  parser.find_visible_var_type("value")->base == c4c::TB_DOUBLE,
              "local value registration should resolve through lexical scope storage");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local binding scope");
  expect_true(parser.find_visible_typedef_type("Alias") == nullptr,
              "popping the local scope should remove lexical typedef visibility");
  expect_true(parser.find_visible_var_type("value") == nullptr,
              "popping the local scope should remove lexical value visibility");
}

void test_parser_if_condition_decl_uses_local_visible_typedef_scope() {
  c4c::Lexer lexer("if (Alias value = 0) { }\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::Node* stmt = c4c::parse_stmt(parser);
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_BLOCK,
              "if-condition declarations should parse as a scoped block when they introduce a declaration");
  expect_eq_int(stmt->n_children, 2,
                "if-condition declaration blocks should contain the declaration and the wrapped if statement");
  expect_true(stmt->children[0] != nullptr &&
                  stmt->children[0]->kind == c4c::NK_DECL,
              "if-condition declarations should materialize a declaration node in the scoped block");
  expect_true(stmt->children[1] != nullptr &&
                  stmt->children[1]->kind == c4c::NK_IF,
              "if-condition declaration blocks should wrap the parsed if statement");
  expect_true(stmt->children[0]->type.base == c4c::TB_INT,
              "if-condition declaration types should resolve through the bound local typedef");
  expect_eq(stmt->children[0]->name, "value",
            "if-condition declarations should preserve the declarator spelling");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_if_condition_decl_scope_does_not_leak_bindings() {
  c4c::Lexer lexer("if (Alias value = 0) value = 1; else value = 2;\n"
                   "value = 3;\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::Node* stmt = c4c::parse_stmt(parser);
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_BLOCK,
              "if-condition declaration scope should still parse as a synthetic block");
  expect_true(stmt->children[1] != nullptr &&
                  stmt->children[1]->kind == c4c::NK_IF,
              "if-condition declaration scope should keep the wrapped if node");
  expect_true(parser.find_var_type("value") == nullptr,
              "if-condition declarations should not leak through the flat var table");
  expect_true(parser.find_visible_var_type("value") == nullptr,
              "if-condition declaration scope should pop after the statement finishes");

  c4c::Node* trailing = c4c::parse_stmt(parser);
  expect_true(trailing != nullptr,
              "parsing should continue after the if-condition declaration scope ends");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_for_init_decl_uses_loop_lifetime_local_scope() {
  c4c::Lexer lexer("for (Alias value = 0; value < 2; ++value) { value = 1; }\n"
                   "value = 3;\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::Node* stmt = c4c::parse_stmt(parser);
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_FOR,
              "for init declarations should parse as a for-statement node");
  expect_true(stmt->init != nullptr && stmt->init->kind == c4c::NK_DECL,
              "for init declarations should materialize a declaration node");
  expect_true(stmt->body != nullptr && stmt->body->kind == c4c::NK_BLOCK,
              "for init declarations should keep the loop body attached");
  expect_true(parser.find_visible_var_type("value") == nullptr,
              "for init declaration bindings should not leak after the loop");

  c4c::Node* trailing = c4c::parse_stmt(parser);
  expect_true(trailing != nullptr,
              "parsing should continue after the for init declaration scope ends");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_while_condition_decl_uses_loop_lifetime_local_scope() {
  c4c::Lexer lexer("while (Alias value = 1) { value = 2; }\n"
                   "value = 3;\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::Node* stmt = c4c::parse_stmt(parser);
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_BLOCK,
              "while-condition declarations should parse as a synthetic block");
  expect_eq_int(stmt->n_children, 2,
                "while-condition declaration blocks should contain the declaration and the wrapped while statement");
  expect_true(stmt->children[0] != nullptr &&
                  stmt->children[0]->kind == c4c::NK_DECL,
              "while-condition declarations should materialize a declaration node");
  expect_true(stmt->children[1] != nullptr &&
                  stmt->children[1]->kind == c4c::NK_WHILE,
              "while-condition declaration blocks should wrap the parsed while statement");
  expect_true(stmt->children[1]->body != nullptr &&
                  stmt->children[1]->body->kind == c4c::NK_BLOCK,
              "while-condition declarations should keep the loop body attached");
  expect_true(parser.find_visible_var_type("value") == nullptr,
              "while-condition declaration bindings should not leak after the loop");

  c4c::Node* trailing = c4c::parse_stmt(parser);
  expect_true(trailing != nullptr,
              "parsing should continue after the while-condition declaration scope ends");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_range_for_decl_uses_loop_lifetime_local_scope() {
  c4c::Lexer lexer("for (Alias value : items) { value = 1; }\n"
                   "value = 3;\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::Node* stmt = c4c::parse_stmt(parser);
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_RANGE_FOR,
              "range-for declarations should parse as a range-for node");
  expect_true(stmt->init != nullptr && stmt->init->kind == c4c::NK_DECL,
              "range-for declarations should materialize a declaration node");
  expect_true(stmt->body != nullptr && stmt->body->kind == c4c::NK_BLOCK,
              "range-for declarations should keep the loop body attached");
  expect_true(parser.find_visible_var_type("value") == nullptr,
              "range-for declaration bindings should not leak after the loop");

  c4c::Node* trailing = c4c::parse_stmt(parser);
  expect_true(trailing != nullptr,
              "parsing should continue after the range-for declaration scope ends");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_switch_condition_decl_uses_case_scope_without_leaking() {
  c4c::Lexer lexer("switch (Alias value = 1) { default: value = 2; }\n"
                   "value = 3;\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);

  c4c::Node* stmt = c4c::parse_stmt(parser);
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_BLOCK,
              "switch-condition declarations should parse as a synthetic block");
  expect_eq_int(stmt->n_children, 2,
                "switch-condition declaration blocks should contain the declaration and the wrapped switch statement");
  expect_true(stmt->children[0] != nullptr &&
                  stmt->children[0]->kind == c4c::NK_DECL,
              "switch-condition declarations should materialize a declaration node");
  expect_true(stmt->children[1] != nullptr &&
                  stmt->children[1]->kind == c4c::NK_SWITCH,
              "switch-condition declaration blocks should wrap the parsed switch statement");
  expect_true(stmt->children[1]->body != nullptr &&
                  stmt->children[1]->body->kind == c4c::NK_BLOCK,
              "switch-condition declarations should keep the switch body attached");
  expect_true(parser.find_visible_var_type("value") == nullptr,
              "switch-condition declaration bindings should not leak after the statement finishes");

  c4c::Node* trailing = c4c::parse_stmt(parser);
  expect_true(trailing != nullptr,
              "parsing should continue after the switch-condition declaration scope ends");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_top_level_typedef_uses_unresolved_identifier_type_head_fallback() {
  c4c::Lexer lexer("typedef ForwardDecl Alias;\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* decl = parse_top_level(parser);
  expect_true(decl == nullptr,
              "top-level typedef fallback should stay bookkeeping-only after registering the alias");

  const c4c::TypeSpec* alias_ts = parser.find_typedef_type("Alias");
  expect_true(alias_ts != nullptr && alias_ts->base == c4c::TB_TYPEDEF,
              "top-level typedef fallback should register the alias in the parser typedef table");
  expect_eq(alias_ts->tag, "ForwardDecl",
            "registered top-level typedef aliases should keep the unresolved placeholder base");
}

void test_parser_block_local_bindings_do_not_leak_into_later_ctor_init_probes() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "int main() {\n"
                   "  {\n"
                   "    typedef int Alias;\n"
                   "    int source = 7;\n"
                   "  }\n"
                   "  Box alias_copy(Alias(other));\n"
                   "  Box value(source(other));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "block-local leak regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the block-local leak regression should contain the record definition and main");

  c4c::Node* main_fn = program->children[1];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the block-local leak regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 3,
                "main should retain the inner block plus both post-block ambiguous declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_BLOCK,
              "the inner scope should remain an explicit block");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_EMPTY,
              "out-of-scope typedef names should not survive into later ctor-init probes");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_EMPTY,
              "out-of-scope value names should not survive into later ctor-init probes");
}

void test_parser_local_ctor_init_probe_balances_unresolved_param_and_value_expr_shapes() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "int main() {\n"
                   "  Box copy(Value other);\n"
                   "  Box value(source & other);\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "balanced ctor-init probe regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the regression program should contain the record definition and main");

  c4c::Node* main_fn = program->children[1];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the regression program should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 2,
                "main should retain both ambiguous local declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_EMPTY,
              "unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_DECL,
              "value-expression direct-init forms should stay declarations");
  expect_eq(main_fn->body->children[1]->name, "value",
            "value-expression direct-init forms should keep their declarator spelling");
}

void test_parser_local_ctor_init_probe_balances_parenthesized_param_and_value_call_shapes() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "int main() {\n"
                   "  int source;\n"
                   "  Box copy(Value(other));\n"
                   "  Box value(source(other));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "parenthesized ctor-init probe regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the parenthesized regression program should contain the record definition and main");

  c4c::Node* main_fn = program->children[1];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the parenthesized regression program should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 3,
                "main should retain the local source declaration and both ambiguous declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the local source declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_EMPTY,
              "parenthesized unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_DECL,
              "known visible value call-like forms should stay declarations");
  expect_eq(main_fn->body->children[2]->name, "value",
            "known visible value call-like forms should keep their declarator spelling");
}

void test_parser_local_ctor_init_probe_preserves_visible_head_handoff_boundary() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "int source(int value) { return value; }\n"
                   "namespace ns {\n"
                   "int sink(int value) { return value; }\n"
                   "}\n"
                   "int main() {\n"
                   "  int payload = 7;\n"
                   "  Box copy(Value(other));\n"
                   "  Box value(source(payload));\n"
                   "  Box qualified_copy(ns::Value(other));\n"
                   "  Box qualified_value(ns::sink(payload));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "visible-head handoff regression should parse as a program");
  expect_eq_int(program->n_children, 4,
                "the visible-head handoff regression should contain the record, helper, namespace, and main");

  c4c::Node* main_fn = program->children[3];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the visible-head handoff regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 5,
                "main should retain the payload declaration and the focused handoff declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the payload declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_EMPTY,
              "visible type heads should remain function declarations at the handoff boundary");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_DECL,
              "visible value heads should stay ctor-init declarations at the handoff boundary");
  expect_eq(main_fn->body->children[2]->name, "value",
            "visible value handoff declarations should keep their declarator spelling");
  expect_true(main_fn->body->children[3] != nullptr &&
                  main_fn->body->children[3]->kind == c4c::NK_EMPTY,
              "qualified visible type heads should remain function declarations at the handoff boundary");
  expect_true(main_fn->body->children[4] != nullptr &&
                  main_fn->body->children[4]->kind == c4c::NK_DECL,
              "qualified visible value heads should stay ctor-init declarations at the handoff boundary");
  expect_eq(main_fn->body->children[4]->name, "qualified_value",
            "qualified visible value handoff declarations should keep their declarator spelling");
}

void test_parser_local_ctor_init_probe_balances_grouped_pointer_param_and_value_call_shapes() {
  c4c::Lexer lexer("struct Box { Box(int*); };\n"
                   "int deref(int* p) { return p ? *p : 0; }\n"
                   "int main() {\n"
                   "  int payload = 7;\n"
                   "  int* source = &payload;\n"
                   "  Box copy(Value((*other)));\n"
                   "  Box value(source((*other)));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "grouped pointer ctor-init probe regression should parse as a program");
  expect_eq_int(program->n_children, 3,
                "the grouped-pointer regression program should contain the record, helper, and main");

  c4c::Node* main_fn = program->children[2];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the grouped-pointer regression program should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 4,
                "main should retain the local payload/source declarations and both ambiguous declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the payload declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_DECL,
              "the source pointer declaration should remain a declaration");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_EMPTY,
              "grouped pointer unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[3] != nullptr &&
                  main_fn->body->children[3]->kind == c4c::NK_DECL,
              "known visible value grouped-call forms should stay declarations");
  expect_eq(main_fn->body->children[3]->name, "value",
            "known visible value grouped-call forms should keep their declarator spelling");
}

void test_parser_local_ctor_init_probe_balances_template_param_and_value_call_shapes() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "template<typename T>\n"
                   "int source(T value) { return value; }\n"
                   "int main() {\n"
                   "  int payload = 7;\n"
                   "  Box copy(Value<int>(other));\n"
                   "  Box value(source<int>(payload));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "template ctor-init probe regression should parse as a program");
  expect_eq_int(program->n_children, 3,
                "the template regression program should contain the record, helper, and main");

  c4c::Node* main_fn = program->children[2];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the template regression program should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 3,
                "main should retain the payload declaration and both ambiguous declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the payload declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_EMPTY,
              "template-headed unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_DECL,
              "known visible value template-call forms should stay declarations");
  expect_eq(main_fn->body->children[2]->name, "value",
            "known visible value template-call forms should keep their declarator spelling");
}

void test_parser_local_ctor_init_probe_balances_qualified_param_and_value_call_shapes() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "namespace ns {\n"
                   "template<typename T>\n"
                   "int source(T value) { return value; }\n"
                   "int sink(int value) { return value; }\n"
                   "}\n"
                   "int main() {\n"
                   "  int payload = 7;\n"
                   "  Box copy(ns::Value(other));\n"
                   "  Box templ_copy(ns::Value<int>(other));\n"
                   "  Box value(ns::sink(payload));\n"
                   "  Box templ_value(ns::source<int>(payload));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "qualified ctor-init probe regression should parse as a program");
  expect_eq_int(program->n_children, 3,
                "the qualified regression program should contain the record, namespace contents, and main");

  c4c::Node* main_fn = program->children[2];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the qualified regression program should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 5,
                "main should retain the payload declaration plus both qualified ambiguous pairs");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the payload declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_EMPTY,
              "qualified unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_EMPTY,
              "qualified template-headed unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[3] != nullptr &&
                  main_fn->body->children[3]->kind == c4c::NK_DECL,
              "qualified visible value call-like forms should stay declarations");
  expect_eq(main_fn->body->children[3]->name, "value",
            "qualified visible value call-like forms should keep their declarator spelling");
  expect_true(main_fn->body->children[4] != nullptr &&
                  main_fn->body->children[4]->kind == c4c::NK_DECL,
              "qualified visible value template-call forms should stay declarations");
  expect_eq(main_fn->body->children[4]->name, "templ_value",
            "qualified visible value template-call forms should keep their declarator spelling");
}

void test_parser_local_ctor_init_probe_balances_qualified_member_access_value_shapes() {
  c4c::Lexer lexer("struct Box { Box(int); };\n"
                   "namespace ns {\n"
                   "struct Payload {\n"
                   "  int value;\n"
                   "};\n"
                   "Payload payload;\n"
                   "}\n"
                   "int main() {\n"
                   "  Box copy(ns::Value(other));\n"
                   "  Box value(ns::payload.value);\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "qualified member-access ctor-init regression should parse as a program");
  expect_eq_int(program->n_children, 4,
                "the qualified member-access regression should contain the record, namespace payload items, and main");

  c4c::Node* main_fn = program->children[3];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the qualified member-access regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 2,
                "main should retain both ambiguous qualified constructor-style declarations");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_EMPTY,
              "qualified unresolved named-parameter forms should remain function declarations");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_DECL,
              "qualified visible member-access forms should stay declarations");
  expect_eq(main_fn->body->children[1]->name, "value",
            "qualified visible member-access forms should keep their declarator spelling");
}

void test_parser_out_of_class_ctor_body_keeps_parameter_scope_for_ctor_init_probe() {
  c4c::Lexer lexer("struct Box {\n"
                   "  Box(int source);\n"
                   "};\n"
                   "Box::Box(int source) {\n"
                   "  Box value(source(other));\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "out-of-class constructor body regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the out-of-class constructor body regression should contain the record and constructor definition");

  c4c::Node* ctor_fn = program->children[1];
  expect_true(ctor_fn != nullptr && ctor_fn->kind == c4c::NK_FUNCTION,
              "the out-of-class constructor body regression should include a parsed constructor");
  expect_true(ctor_fn->body != nullptr && ctor_fn->body->kind == c4c::NK_BLOCK,
              "the constructor definition should keep its block body");
  expect_eq_int(ctor_fn->body->n_children, 1,
                "the constructor body should retain the ambiguous value-like declaration");
  expect_true(ctor_fn->body->children[0] != nullptr &&
                  ctor_fn->body->children[0]->kind == c4c::NK_DECL,
              "constructor parameters should remain visible while parsing the body");
  expect_eq(ctor_fn->body->children[0]->name, "value",
            "the constructor body should keep the declarator spelling for the value-like declaration");
}

void test_parser_stmt_disambiguates_global_qualified_template_call_as_expr() {
  c4c::Lexer lexer("namespace api {\n"
                   "template<typename T>\n"
                   "int source(T value) { return value; }\n"
                   "}\n"
                   "int main() {\n"
                   "  int payload = 7;\n"
                   "  ::api::source<int>(payload);\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "global-qualified template call regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the global-qualified template regression should contain the namespace contents and main");

  c4c::Node* main_fn = program->children[1];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the global-qualified template regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 2,
                "main should retain the local payload declaration and the expression statement");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the payload declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_EXPR_STMT,
              "global-qualified template calls should stay expression statements");
}

void test_parser_stmt_disambiguates_global_qualified_operator_call_as_expr() {
  c4c::Lexer lexer("struct BaseImpl {\n"
                   "  BaseImpl& operator=(const BaseImpl&) { return *this; }\n"
                   "};\n"
                   "int main() {\n"
                   "  BaseImpl lhs;\n"
                   "  BaseImpl rhs;\n"
                   "  ::BaseImpl::operator=(lhs, rhs);\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "global-qualified operator regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the global-qualified operator regression should contain the record and main");

  c4c::Node* main_fn = program->children[1];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the global-qualified operator regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 3,
                "main should retain the local declarations and the operator expression statement");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_DECL,
              "the lhs declaration should remain a declaration");
  expect_true(main_fn->body->children[1] != nullptr &&
                  main_fn->body->children[1]->kind == c4c::NK_DECL,
              "the rhs declaration should remain a declaration");
  expect_true(main_fn->body->children[2] != nullptr &&
                  main_fn->body->children[2]->kind == c4c::NK_EXPR_STMT,
              "global-qualified operator calls should stay expression statements");
}

void test_parser_stmt_prefers_expression_for_member_access_after_visible_type_head() {
  c4c::Lexer lexer("typedef int allocator;\n"
                   "int main() {\n"
                   "  allocator.construct(1);\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "member-access after a visible type head should parse as a program");
  expect_eq_int(program->n_children, 1,
                "the member-access regression should retain the parsed main function");

  c4c::Node* main_fn = program->children[0];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the member-access regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 1,
                "main should retain the member-access expression statement");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_EXPR_STMT,
              "member-access after a visible type head should stay an expression statement");
}

void test_parser_stmt_disambiguates_qualified_visible_value_member_access_as_expr() {
  c4c::Lexer lexer("namespace api {\n"
                   "struct Payload {\n"
                   "  int value;\n"
                   "};\n"
                   "Payload payload;\n"
                   "}\n"
                   "int main() {\n"
                   "  return api::payload.value;\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "qualified visible-value member access regression should parse as a program");
  expect_eq_int(program->n_children, 3,
                "the qualified visible-value member access regression should contain the namespace and main");

  c4c::Node* main_fn = program->children[2];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the qualified visible-value member access regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 1,
                "main should retain the return statement");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_RETURN,
              "qualified visible-value member access should remain on the expression path");
}

void test_parser_stmt_disambiguates_qualified_visible_value_member_access_assignment_as_expr() {
  c4c::Lexer lexer("namespace api {\n"
                   "struct Payload {\n"
                   "  int value;\n"
                   "};\n"
                   "Payload payload;\n"
                   "}\n"
                   "int main() {\n"
                   "  api::payload.value = 9;\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "qualified visible-value member assignment regression should parse as a program");
  expect_eq_int(program->n_children, 3,
                "the qualified visible-value member assignment regression should contain the namespace and main");

  c4c::Node* main_fn = program->children[2];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the qualified visible-value member assignment regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 1,
                "main should retain the member assignment statement");
  expect_true(main_fn->body->children[0] != nullptr &&
                  main_fn->body->children[0]->kind == c4c::NK_EXPR_STMT,
              "qualified visible-value member assignments should remain on the expression path");
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

  (void)parse_top_level(parser);
  expect_true(parser.find_template_struct_primary("Trait") != nullptr,
              "template struct fixture should register before injected suffix probing");

  c4c::Token seed{};
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  });
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
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });
  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.try_parse_template_type_arg(&arg),
              "template type-argument probes should use parser-owned spelling");
  expect_true(!arg.is_value,
              "template scope type parameters should stay classified as type arguments");
  expect_true(arg.type.base == c4c::TB_TYPEDEF,
              "template scope type parameters should parse as placeholder typedef types");
  expect_eq(arg.type.tag, "T",
            "template type-argument parsing should preserve the parser-owned spelling");
  expect_eq_int(parser.token_cursor_for_testing(), 1,
                "template type-argument parsing should stop before the template close");
  parser.pop_template_scope();
}

void test_parser_template_scope_type_param_prefers_text_id_over_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId param_text = texts.intern("T");
  const c4c::TextId other_text = texts.intern("Other");
  parser.push_template_scope(
      c4c::Parser::TemplateScopeKind::FreeFunctionTemplate,
      {{.name_text_id = param_text, .name = "corrupted"}});

  expect_true(parser.is_template_scope_type_param(param_text, "corrupted"),
              "template-scope lookup should match the semantic TextId even when spelling is stale");
  expect_true(!parser.is_template_scope_type_param(other_text, "T"),
              "template-scope lookup should not fall back to spelling when a TextId is already available");
  parser.pop_template_scope();

  parser.push_template_scope(
      c4c::Parser::TemplateScopeKind::FreeFunctionTemplate,
      {{.name = "T"}});
  expect_true(parser.is_template_scope_type_param("T"),
              "spelling-only template-scope lookups should still work when no TextId is available");
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
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("Target"), "corrupted");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });
  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.try_parse_template_type_arg(&arg),
              "template type-argument probes should treat visible lexical aliases as type heads");
  expect_true(!arg.is_value,
              "visible lexical aliases should stay classified as type arguments");
  expect_true(arg.type.base == c4c::TB_INT,
              "visible lexical aliases should resolve to the bound scope-local type");
  expect_true(arg.type.tag == nullptr,
              "visible lexical alias type-argument parsing should not fabricate a flat typedef tag");
  expect_eq_int(parser.token_cursor_for_testing(), 1,
                "visible lexical alias type-argument parsing should stop before the template close");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_synthesized_typedef_binding_unregisters_by_text_id() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId synth_text_id = texts.intern("SynthParam");
  parser.register_synthesized_typedef_binding(synth_text_id, "corrupted");
  expect_true(parser.is_typedef_name(synth_text_id, "SynthParam"),
              "synthesized typedef registration should prefer the semantic TextId");

  parser.unregister_typedef_binding(synth_text_id, "still_corrupted");
  expect_true(!parser.is_typedef_name(synth_text_id, "SynthParam"),
              "synthesized typedef cleanup should remove the semantic TextId binding");
}

void test_parser_template_type_arg_prefers_local_visible_typedef_text_id() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::TypeSpec global_alias_ts{};
  global_alias_ts.array_size = -1;
  global_alias_ts.inner_rank = -1;
  global_alias_ts.base = c4c::TB_CHAR;
  parser.register_typedef_binding("Alias", global_alias_ts, true);

  c4c::TypeSpec local_alias_ts{};
  local_alias_ts.array_size = -1;
  local_alias_ts.inner_rank = -1;
  local_alias_ts.base = c4c::TB_DOUBLE;

  const c4c::TextId alias_text = texts.intern("Alias");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, local_alias_ts);
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });

  c4c::Parser::TemplateArgParseResult arg{};
  expect_true(parser.try_parse_template_type_arg(&arg),
              "template type-argument probes should prefer the local visible typedef TextId path before spelling-based fallback");
  expect_true(!arg.is_value,
              "local visible typedef template arguments should stay classified as type arguments");
  expect_true(arg.type.base == c4c::TB_DOUBLE,
              "local visible typedef template arguments should resolve to the local binding");
  expect_eq_int(parser.token_cursor_for_testing(), 1,
                "local visible typedef template arguments should stop before the template close");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_deferred_nttp_builtin_trait_uses_visible_scope_local_alias() {
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

  const std::vector<c4c::Token> toks = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "__is_same"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Comma, ","),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  };

  long long value = 0;
  expect_true(parser.eval_deferred_nttp_expr_tokens("Trait", toks, {}, {}, &value),
              "deferred NTTP builtin traits should resolve scope-local aliases through token TextId lookup");
  expect_eq_int(value, 1,
                "deferred NTTP builtin traits should treat the scope-local alias as the bound type");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_deferred_nttp_member_lookup_uses_visible_scope_local_aliases() {
  c4c::Lexer lexer(
      "template<typename T>\n"
      "struct Trait { static constexpr int value = 7; };\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  (void)parse_top_level(parser);
  expect_true(parser.find_template_struct_primary("Trait") != nullptr,
              "template member lookup fixture should register the template primary");

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  c4c::TypeSpec trait_alias_ts{};
  trait_alias_ts.array_size = -1;
  trait_alias_ts.inner_rank = -1;
  trait_alias_ts.base = c4c::TB_STRUCT;
  trait_alias_ts.tag = arena.strdup("Trait");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId trait_alias_text = lexer.text_table().intern("AliasTrait");
  parser.push_local_binding_scope();
  parser.bind_local_typedef(alias_text, alias_ts);
  parser.bind_local_typedef(trait_alias_text, trait_alias_ts);

  const std::vector<c4c::Token> toks = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "AliasTrait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };

  long long value = 0;
  expect_true(parser.eval_deferred_nttp_expr_tokens("Trait", toks, {}, {}, &value),
              "deferred NTTP member lookup should resolve scope-local template aliases and template arguments through token TextId lookup");
  expect_eq_int(value, 7,
                "deferred NTTP member lookup should preserve the instantiated static member value");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_alias_template_value_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(
          parser.current_namespace_context_id(),
          parser.find_parser_text_id("Alias"), "Alias"),
      {});
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
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
  resolved_parser.register_using_value_alias_for_testing(
      0, alias_text, resolved_parser.intern_semantic_name_key("ns::Alias"),
      "corrupted");
  resolved_parser.register_alias_template_info_for_testing(
      resolved_parser.alias_template_key_in_context(
          resolved_parser.current_namespace_context_id(),
          resolved_parser.find_parser_text_id("ns::Alias"), "ns::Alias"),
      {});
  resolved_parser.replace_token_stream_for_testing({
      resolved_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::IntLit, "0"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      resolved_parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });
  expect_true(!resolved_parser.is_clearly_value_template_arg(nullptr, 0),
              "resolved alias-template probes should use parser-owned spelling");
}

void test_parser_alias_template_info_prefers_structured_key_over_recovery() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId alias_text = texts.intern("Alias");
  const c4c::QualifiedNameKey alias_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), alias_text, "Alias");
  c4c::ParserAliasTemplateInfo info;
  info.param_names = {"T"};
  info.aliased_type.array_size = -1;
  info.aliased_type.inner_rank = -1;
  info.aliased_type.base = c4c::TB_INT;
  parser.register_alias_template_info_for_testing(alias_key, info);
  parser.register_using_value_alias_for_testing(
      0, alias_text, parser.intern_semantic_name_key("Bridge"), "Bridge");

  const c4c::ParserAliasTemplateInfo* found =
      parser.find_alias_template_info_in_context(
          parser.current_namespace_context_id(), alias_text, "Alias");
  expect_true(found != nullptr && found->aliased_type.base == c4c::TB_INT,
              "alias-template info lookup should prefer the structured key before any rendered-name recovery");
}

void test_parser_typename_template_parameter_probe_uses_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "typename"),
  });
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

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "_Nullable"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "restrict"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });
  c4c::consume_declarator_post_pointer_qualifiers(parser);

  expect_eq(parser.token_spelling(parser.cur()), "after",
            "post-pointer qualifier probes should use parser-owned spelling");
}

void test_parser_qualified_declarator_name_uses_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "inner"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Value"),
  });
  std::string qualified_name;
  expect_true(c4c::parse_qualified_declarator_name(parser, &qualified_name),
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
  test_parser_structured_value_registration_keeps_legacy_lookup();
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
  test_parser_is_type_start_uses_local_visible_typedef_scope();
  test_parser_local_visible_typedef_cast_uses_scope_lookup();
  test_parser_decode_type_ref_text_uses_local_visible_scope_lookup();
  test_parser_dependent_typename_uses_local_visible_owner_alias();
  test_parser_is_typedef_name_uses_local_visible_scope_lookup();
  test_parser_conflicting_user_typedef_binding_uses_local_visible_scope_lookup();
  test_parser_record_body_context_keeps_visible_template_origin_lookup_local();
  test_parser_visible_type_alias_uses_scope_local_typedef_facade();
  test_parser_visible_type_alias_resolves_scope_local_target_type();
  test_parser_visible_type_alias_uses_token_text_id_scope_lookup();
  test_parser_visible_type_alias_keeps_qualified_target_resolution();
  test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup();
  test_parser_using_value_import_keeps_structured_target_key();
  test_parser_global_using_value_import_keeps_global_target_resolution();
  test_parser_visible_value_alias_resolves_scope_local_target_type();
  test_parser_register_local_bindings_keep_flat_tables_empty();
  test_parser_if_condition_decl_uses_local_visible_typedef_scope();
  test_parser_if_condition_decl_scope_does_not_leak_bindings();
  test_parser_for_init_decl_uses_loop_lifetime_local_scope();
  test_parser_while_condition_decl_uses_loop_lifetime_local_scope();
  test_parser_range_for_decl_uses_loop_lifetime_local_scope();
  test_parser_switch_condition_decl_uses_case_scope_without_leaking();
  test_parser_top_level_typedef_uses_unresolved_identifier_type_head_fallback();
  test_parser_block_local_bindings_do_not_leak_into_later_ctor_init_probes();
  test_parser_local_ctor_init_probe_balances_unresolved_param_and_value_expr_shapes();
  test_parser_local_ctor_init_probe_balances_parenthesized_param_and_value_call_shapes();
  test_parser_local_ctor_init_probe_preserves_visible_head_handoff_boundary();
  test_parser_local_ctor_init_probe_balances_grouped_pointer_param_and_value_call_shapes();
  test_parser_local_ctor_init_probe_balances_template_param_and_value_call_shapes();
  test_parser_local_ctor_init_probe_balances_qualified_param_and_value_call_shapes();
  test_parser_local_ctor_init_probe_balances_qualified_member_access_value_shapes();
  test_parser_out_of_class_ctor_body_keeps_parameter_scope_for_ctor_init_probe();
  test_parser_stmt_disambiguates_global_qualified_template_call_as_expr();
  test_parser_stmt_disambiguates_global_qualified_operator_call_as_expr();
  test_parser_stmt_prefers_expression_for_member_access_after_visible_type_head();
  test_parser_stmt_disambiguates_qualified_visible_value_member_access_as_expr();
  test_parser_stmt_disambiguates_qualified_visible_value_member_access_assignment_as_expr();
  test_parser_template_member_suffix_probe_uses_token_spelling();
  test_parser_template_type_arg_probes_use_token_spelling();
  test_parser_template_scope_type_param_prefers_text_id_over_spelling();
  test_parser_template_type_arg_uses_visible_scope_local_alias();
  test_parser_synthesized_typedef_binding_unregisters_by_text_id();
  test_parser_template_type_arg_prefers_local_visible_typedef_text_id();
  test_parser_deferred_nttp_builtin_trait_uses_visible_scope_local_alias();
  test_parser_deferred_nttp_member_lookup_uses_visible_scope_local_aliases();
  test_parser_alias_template_value_probes_use_token_spelling();
  test_parser_alias_template_info_prefers_structured_key_over_recovery();
  test_parser_typename_template_parameter_probe_uses_token_spelling();
  test_parser_post_pointer_qualifier_probes_use_token_spelling();
  test_parser_qualified_declarator_name_uses_token_spelling();

  std::cout << "PASS: frontend_parser_tests\n";
  return 0;
}
