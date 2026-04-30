#include "lexer.hpp"
#include "impl/parser_impl.hpp"
#include "impl/types/types_helpers.hpp"
#include "parser.hpp"
#include "sema/consteval.hpp"
#include "sema/type_utils.hpp"
#include "sema/validate.hpp"

#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
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

c4c::TextId parser_test_text_id(c4c::Parser& parser,
                                std::string_view spelling) {
  return parser.parser_text_id_for_token(c4c::kInvalidText, spelling);
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

  const c4c::TextId value_text_id = parser_test_text_id(parser, "Value");
  const c4c::TextId counter_text_id = parser_test_text_id(parser, "counter");

  parser.register_typedef_binding(value_text_id, typedef_ts, true);
  parser.register_var_type_binding(counter_text_id, var_ts);

  const c4c::Parser::SymbolId typedef_symbol =
      parser.parser_symbol_tables().find_identifier(value_text_id);
  const c4c::Parser::SymbolId var_symbol =
      parser.parser_symbol_tables().find_identifier(counter_text_id);

  expect_true(typedef_symbol != c4c::Parser::kInvalidSymbol,
              "typedef wrapper should intern a valid SymbolId");
  expect_true(parser.parser_symbol_tables().is_typedef(typedef_symbol),
              "typedef wrapper should store membership by SymbolId");
  expect_true(parser.parser_symbol_tables().user_typedefs.count(
                  typedef_symbol) == 1,
              "user typedef wrapper should track user typedef membership by SymbolId");
  expect_true(parser.has_typedef_name(parser_test_text_id(parser, "Value")),
              "string-facing typedef lookup should still work");
  expect_true(parser.has_typedef_type(parser_test_text_id(parser, "Value")),
              "typedef type wrapper should still resolve through string lookup");
  expect_true(parser.find_typedef_type(parser_test_text_id(parser, "Value")) != nullptr &&
                  parser.find_typedef_type(parser_test_text_id(parser, "Value"))->base == c4c::TB_INT,
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
  expect_true(parser.find_var_type(parser_test_text_id(parser, "counter")) !=
                  nullptr,
              "TextId-facing var-type lookup should still work");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "counter")) != nullptr &&
                  parser.find_var_type(parser_test_text_id(parser, "counter"))->base == c4c::TB_DOUBLE,
              "var-type wrapper should recover the stored TypeSpec");
  const c4c::QualifiedNameKey counter_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "counter"));
  expect_true(parser.has_structured_var_type(counter_key),
              "var-type wrapper should also populate structured value storage");
  expect_true(parser.find_structured_var_type(counter_key) != nullptr &&
                  parser.find_structured_var_type(counter_key)->base ==
                      c4c::TB_DOUBLE,
              "structured value storage should recover the stored TypeSpec");
}

void test_parser_id_first_binding_helpers_prefer_text_ids() {
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
  var_ts.base = c4c::TB_LONG;

  const c4c::TextId typedef_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "IdFirstType");
  const c4c::TextId value_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "idFirstValue");
  const c4c::TextId ns_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "idFirstNs");
  const c4c::TextId fn_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "idFirstFn");
  const c4c::TextId qualified_fn_id =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "idFirstQualifiedFn");
  const int ns_context =
      parser.ensure_named_namespace_context(0, ns_id, "wrong_ns_fallback");

  parser.register_typedef_binding(typedef_id, typedef_ts, true);
  parser.register_var_type_binding(value_id, var_ts);
  parser.register_known_fn_name_in_context(ns_context, fn_id);
  parser.register_known_fn_name_in_context(ns_context, qualified_fn_id);

  expect_true(parser.has_typedef_type(parser_test_text_id(parser, "IdFirstType")),
              "ID-first typedef registration should use the TextId spelling");
  expect_true(!parser.has_typedef_type(parser_test_text_id(parser, "wrong_type_fallback")),
              "ID-first typedef registration should not prefer fallback spelling");
  expect_true(parser.find_var_type(value_id) != nullptr,
              "ID-first value registration should use the TextId spelling");
  expect_true(parser.find_var_type(
                  parser_test_text_id(parser, "wrong_value_fallback")) ==
                  nullptr,
              "ID-first value registration should not prefer fallback spelling");

  c4c::TypeSpec fallback_typedef_ts{};
  fallback_typedef_ts.array_size = -1;
  fallback_typedef_ts.inner_rank = -1;
  fallback_typedef_ts.base = c4c::TB_DOUBLE;

  c4c::TypeSpec fallback_var_ts{};
  fallback_var_ts.array_size = -1;
  fallback_var_ts.inner_rank = -1;
  fallback_var_ts.base = c4c::TB_FLOAT;

  const c4c::TextId lookup_typedef_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "IdFirstLookupType");
  const c4c::TextId lookup_value_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "idFirstLookupValue");
  parser.register_typedef_binding(lookup_typedef_id, typedef_ts, true);
  parser.register_typedef_binding(parser_test_text_id(parser, "typedefLookupBridge"), fallback_typedef_ts,
                                  true);
  parser.register_var_type_binding(lookup_value_id, var_ts);
  parser.register_var_type_binding(parser_test_text_id(parser, "valueLookupBridge"), fallback_var_ts);

  const c4c::TypeSpec* id_typedef =
      parser.find_typedef_type(lookup_typedef_id);
  expect_true(id_typedef != nullptr && id_typedef->base == c4c::TB_INT,
              "ID-first typedef lookup should prefer the TextId spelling over a mismatched fallback");
  const c4c::TypeSpec* visible_typedef =
      parser.find_visible_typedef_type(lookup_typedef_id);
  expect_true(visible_typedef != nullptr && visible_typedef->base == c4c::TB_INT,
              "visible typedef lookup should prefer the TextId-backed global binding before fallback spelling");
  const c4c::TextId missing_typedef_id =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "MissingTextIdTypedef");
  expect_true(parser.find_visible_typedef_type(missing_typedef_id) == nullptr,
              "visible typedef lookup should not promote fallback spelling when a valid TextId lookup misses");
  const c4c::QualifiedNameKey direct_typedef_key =
      parser.struct_typedef_key_in_context(0, lookup_typedef_id);
  const c4c::TypeSpec* direct_typedef =
      parser.find_typedef_type(direct_typedef_key);
  expect_true(direct_typedef == nullptr,
              "direct qualified-key typedef lookup should require explicitly structured storage");
  parser.register_structured_typedef_binding_in_context(0, lookup_typedef_id,
                                                        typedef_ts);
  direct_typedef = parser.find_typedef_type(direct_typedef_key);
  expect_true(direct_typedef != nullptr && direct_typedef->base == c4c::TB_INT,
              "direct qualified-key typedef lookup should use explicitly structured storage");
  const c4c::QualifiedNameKey stale_typedef_key =
      parser.struct_typedef_key_in_context(0, missing_typedef_id);
  expect_true(parser.find_typedef_type(stale_typedef_key) == nullptr,
              "direct qualified-key typedef lookup should not promote fallback spelling when a valid TextId lookup misses");
  const c4c::QualifiedNameKey invalid_key{};
  const c4c::TypeSpec* fallback_typedef =
      parser.find_typedef_type(invalid_key);
  expect_true(fallback_typedef == nullptr,
              "direct qualified-key typedef lookup should not use invalid-key fallback compatibility");
  c4c::Parser::VisibleNameResult resolved_type;
  expect_true(parser.lookup_type_in_context(0, lookup_typedef_id,
                                            &resolved_type) &&
                  parser.visible_name_spelling(resolved_type) ==
                      "IdFirstLookupType",
              "namespace-visible typedef lookup should report the TextId spelling over a mismatched fallback");
  resolved_type = {};
  expect_true(!parser.lookup_type_in_context(0, missing_typedef_id,
                                             &resolved_type),
              "namespace-visible typedef lookup should not promote fallback spelling when a valid TextId lookup misses");
  resolved_type = {};
  expect_true(!parser.lookup_type_in_context(0, c4c::kInvalidText,
                                             &resolved_type),
              "namespace-visible typedef lookup should reject TextId-less rendered fallback storage");
  const c4c::TypeSpec* id_var =
      parser.find_var_type(lookup_value_id);
  expect_true(id_var != nullptr && id_var->base == c4c::TB_LONG,
              "ID-first value lookup should prefer the TextId spelling over a mismatched fallback");
  const c4c::TypeSpec* visible_var =
      parser.find_visible_var_type(lookup_value_id);
  expect_true(visible_var != nullptr && visible_var->base == c4c::TB_LONG,
              "visible value lookup should prefer the TextId-backed global binding before fallback spelling");
  const c4c::TextId missing_value_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "MissingTextIdValue");
  expect_true(parser.find_visible_var_type(missing_value_id) == nullptr,
              "visible value lookup should not promote fallback spelling when a valid TextId lookup misses");
  const c4c::QualifiedNameKey direct_value_key =
      parser.known_fn_name_key_in_context(0, lookup_value_id);
  const c4c::TypeSpec* direct_var =
      parser.find_var_type(direct_value_key);
  expect_true(direct_var != nullptr && direct_var->base == c4c::TB_LONG,
              "direct qualified-key value lookup should prefer the TextId spelling over a mismatched fallback");
  const c4c::QualifiedNameKey stale_value_key =
      parser.known_fn_name_key_in_context(0, missing_value_id);
  expect_true(parser.find_var_type(stale_value_key) == nullptr,
              "direct qualified-key value lookup should not promote fallback spelling when a valid TextId lookup misses");
  const c4c::TypeSpec* fallback_var =
      parser.find_var_type(invalid_key);
  expect_true(fallback_var == nullptr,
              "direct qualified-key value lookup should not use invalid-key fallback compatibility");
  c4c::TypeSpec legacy_only_var_ts{};
  legacy_only_var_ts.array_size = -1;
  legacy_only_var_ts.inner_rank = -1;
  legacy_only_var_ts.base = c4c::TB_SHORT;
  const c4c::TextId legacy_only_value_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "legacyOnlyValue");
  const c4c::Parser::SymbolId legacy_only_symbol =
      parser.parser_symbol_tables().intern_identifier("legacyOnlyValue");
  parser.parser_symbol_tables().var_types[legacy_only_symbol] =
      legacy_only_var_ts;
  const c4c::QualifiedNameKey legacy_only_key =
      parser.known_fn_name_key_in_context(0, legacy_only_value_id);
  expect_true(parser.find_var_type(legacy_only_key) == nullptr,
              "direct qualified-key value lookup should not promote legacy-only rendered cache entries");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "legacyOnlyValue")) != nullptr &&
                  parser.find_var_type(parser_test_text_id(parser, "legacyOnlyValue"))->base ==
                      c4c::TB_SHORT,
              "string value lookup should preserve explicit legacy cache compatibility");
  expect_true(parser.find_var_type(invalid_key) == nullptr,
              "invalid-key value lookup should not preserve TextId-less fallback compatibility");
  expect_true(parser.find_visible_var_type(c4c::kInvalidText) == nullptr,
              "TextId-less visible value lookup should not preserve legacy cache compatibility");
  c4c::Parser::VisibleNameResult resolved_value;
  expect_true(parser.lookup_value_in_context(0, lookup_value_id,
                                             &resolved_value) &&
                  parser.visible_name_spelling(resolved_value) ==
                      "idFirstLookupValue",
              "namespace-visible value lookup should report the TextId spelling over a mismatched fallback");
  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(0, missing_value_id,
                                              &resolved_value),
              "namespace-visible value lookup should not promote fallback spelling when a valid TextId lookup misses");
  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(0, c4c::kInvalidText,
                                              &resolved_value),
              "namespace-visible value lookup should reject TextId-less rendered fallback storage");
  const c4c::TypeSpec* string_visible_var =
      parser.find_visible_var_type(parser_test_text_id(parser, "valueLookupBridge"));
  expect_true(string_visible_var != nullptr &&
                  string_visible_var->base == c4c::TB_FLOAT,
              "string visible value lookup should preserve TextId-less compatibility");

  parser.register_typedef_binding(parser_test_text_id(parser, "idFirstNs::namespaceTypeBridge"),
                                  fallback_typedef_ts, true);
  parser.register_var_type_binding(parser_test_text_id(parser, "idFirstNs::namespaceValueBridge"),
                                   fallback_var_ts);
  const c4c::TextId missing_namespace_type_id =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "MissingNamespaceType");
  const c4c::TextId missing_namespace_value_id =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "MissingNamespaceValue");
  resolved_type = {};
  expect_true(!parser.lookup_type_in_context(ns_context,
                                             missing_namespace_type_id,
                                             &resolved_type),
              "namespace-scoped typedef lookup should not promote fallback spelling when a valid TextId lookup misses");
  resolved_type = {};
  expect_true(!parser.lookup_type_in_context(ns_context, c4c::kInvalidText,
                                             &resolved_type),
              "namespace-scoped typedef lookup should reject TextId-less rendered fallback storage");
  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(ns_context,
                                              missing_namespace_value_id,
                                              &resolved_value),
              "namespace-scoped value lookup should not promote fallback spelling when a valid TextId lookup misses");
  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(ns_context, c4c::kInvalidText,
                                              &resolved_value),
              "namespace-scoped value lookup should reject TextId-less rendered fallback storage");

  const c4c::QualifiedNameKey value_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "idFirstValue"));
  expect_true(parser.has_structured_var_type(value_key),
              "ID-first value registration should populate structured value storage");
  expect_true(parser.find_structured_var_type(value_key) != nullptr &&
                  parser.find_structured_var_type(value_key)->base ==
                      c4c::TB_LONG,
              "structured value storage should agree with the ID-first registration");

  expect_true(parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "idFirstNs::idFirstFn"))),
              "ID-first known-function registration should use the TextId spelling");
  expect_true(!parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "idFirstNs::wrong_fn_fallback"))),
              "ID-first known-function registration should not prefer fallback spelling");
  expect_true(parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "idFirstNs::idFirstQualifiedFn"))),
              "ID-first qualified known-function registration should use the namespace context and TextId spelling");
  expect_true(!parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "wrongNs::wrong_qualified_fn_fallback"))),
              "ID-first qualified known-function registration should not promote rendered fallback spelling");
  parser.register_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "stringBridgeNs::stringBridgeFn")));
  expect_true(parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "stringBridgeNs::stringBridgeFn"))),
              "public string known-function lookup should preserve rendered bridge compatibility");
  parser.register_known_fn_name_in_context(
      0, parser_test_text_id(parser, "legacyKnownBridgeNs::legacyKnownBridgeFn"));
  expect_true(!parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "legacyKnownBridgeNs::legacyKnownBridgeFn"))),
              "context known-function registration should not recreate rendered qualified-name fallback routes");
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

  parser.register_typedef_binding(parser_test_text_id(parser, "Keep"), keep_ts, false);
#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const size_t typedef_count_before =
      parser.parser_symbol_tables().typedefs.size();
  const auto snapshot = parser.save_state();
  const c4c::QualifiedNameKey scratch_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "scratch"));
  parser.register_typedef_binding(parser_test_text_id(parser, "Temp"), temp_ts, true);
  parser.register_var_type_binding(parser_test_text_id(parser, "scratch"), temp_ts);
  parser.restore_state(snapshot);

  expect_true(parser.has_typedef_name(parser_test_text_id(parser, "Keep")),
              "restore_state should preserve typedefs that existed before the snapshot");
  expect_true(!parser.has_typedef_name(parser_test_text_id(parser, "Temp")),
              "restore_state should roll back typedefs added after the snapshot");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "scratch")) == nullptr,
              "restore_state should roll back var bindings added after the snapshot");
  expect_true(!parser.has_structured_var_type(scratch_key),
              "restore_state should roll back structured var bindings added after the snapshot");
  expect_eq_int(
      static_cast<int>(parser.parser_symbol_tables().typedefs.size()),
                static_cast<int>(typedef_count_before),
                "heavy snapshot restore should reset typedef membership to the snapshot");
#else
  expect_true(parser.has_typedef_name(parser_test_text_id(parser, "Keep")),
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

  const c4c::TextId qualified_type_text_id =
      parser_test_text_id(parser, "ns::Type");
  const c4c::TextId qualified_value_text_id =
      parser_test_text_id(parser, "ns::value");

  parser.register_typedef_binding(qualified_type_text_id, typedef_ts, true);
  parser.register_var_type_binding(qualified_value_text_id, var_ts);

  expect_true(parser.has_typedef_name(parser_test_text_id(parser, "ns::Type")),
              "qualified typedef membership should remain lookupable");
  expect_true(parser.has_typedef_type(parser_test_text_id(parser, "ns::Type")),
              "qualified typedef types should remain lookupable");
  expect_true(parser.find_typedef_type(parser_test_text_id(parser, "ns::Type")) != nullptr &&
                  parser.find_typedef_type(parser_test_text_id(parser, "ns::Type"))->base == c4c::TB_INT,
              "qualified typedef type lookup should recover the stored TypeSpec");
  const c4c::QualifiedNameKey value_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::value"));
  expect_true(parser.has_structured_var_type(value_key),
              "qualified value bindings should populate structured storage");
  expect_true(parser.find_structured_var_type(value_key) != nullptr &&
                  parser.find_structured_var_type(value_key)->base ==
                      c4c::TB_DOUBLE,
              "qualified structured value lookup should recover the stored TypeSpec");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "ns::value")) != nullptr,
              "qualified value bindings should remain string-lookupable");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "ns::value")) != nullptr &&
                  parser.find_var_type(parser_test_text_id(parser, "ns::value"))->base == c4c::TB_DOUBLE,
              "qualified value lookup should recover the stored TypeSpec");
  expect_true(parser.parser_symbol_tables().find_identifier(
                  qualified_type_text_id) ==
                  c4c::Parser::kInvalidSymbol,
              "qualified typedef names should not intern composed strings");
  expect_true(parser.parser_symbol_tables().find_identifier(
                  qualified_value_text_id) ==
                  c4c::Parser::kInvalidSymbol,
              "qualified value names should not intern composed strings");
  expect_eq_int(static_cast<int>(parser.parser_symbol_count_for_testing()),
                symbol_count_before,
                "qualified bindings should not change the atom-symbol table size");

#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const auto snapshot = parser.save_state();
  const c4c::QualifiedNameKey scratch_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::scratch"));

  c4c::TypeSpec temp_ts{};
  temp_ts.array_size = -1;
  temp_ts.inner_rank = -1;
  temp_ts.base = c4c::TB_FLOAT;

  parser.register_typedef_binding(parser_test_text_id(parser, "ns::Temp"), temp_ts, true);
  parser.register_var_type_binding(parser_test_text_id(parser, "ns::scratch"), temp_ts);
  parser.restore_state(snapshot);

  expect_true(!parser.has_typedef_type(parser_test_text_id(parser, "ns::Temp")),
              "restore_state should roll back qualified typedefs from fallback storage");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "ns::scratch")) == nullptr,
              "restore_state should roll back qualified values from fallback storage");
  expect_true(!parser.has_structured_var_type(scratch_key),
              "restore_state should roll back qualified values from structured storage");
#endif
}

void test_parser_structured_value_registration_avoids_string_bridge_and_legacy_mirror() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

  c4c::TypeSpec value_ts{};
  value_ts.array_size = -1;
  value_ts.inner_rank = -1;
  value_ts.base = c4c::TB_LONG;

  const c4c::QualifiedNameKey value_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::registered"));
  parser.register_structured_var_type_binding(value_key, value_ts);
  const c4c::QualifiedNameKey unqualified_value_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "registered"));
  parser.register_structured_var_type_binding(unqualified_value_key, value_ts);
  const c4c::TextId registered_text_id = parser.find_parser_text_id("registered");

  expect_true(parser.has_structured_var_type(value_key),
              "structured value registration should populate structured storage");
  expect_true(parser.find_structured_var_type(value_key) != nullptr &&
                  parser.find_structured_var_type(value_key)->base ==
                      c4c::TB_LONG,
              "structured value registration should recover the stored TypeSpec");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "ns::registered")) == nullptr,
              "string-facing value lookup should not bridge to structured-only storage");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "registered")) == nullptr,
              "unqualified string-facing value lookup should not bridge to structured-only storage");
  const c4c::Parser::SymbolId registered_symbol =
      parser.parser_symbol_tables().find_identifier(registered_text_id);
  expect_true(registered_symbol == c4c::Parser::kInvalidSymbol ||
                  parser.parser_symbol_tables().var_types.count(
                      registered_symbol) == 0,
              "direct structured value registration should not populate the legacy symbol cache");
  expect_true(parser.find_var_type(
                  parser.find_parser_text_id("registered")) ==
                  nullptr,
              "direct TextId value lookup should not observe a legacy mirror for structured-only registrations");

#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
  const auto snapshot = parser.save_state();
  const c4c::QualifiedNameKey temp_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::temporary_registered"));
  parser.register_structured_var_type_binding(temp_key, value_ts);
  parser.restore_state(snapshot);

  expect_true(!parser.has_structured_var_type(temp_key),
              "restore_state should roll back direct structured value bindings");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "ns::temporary_registered")) == nullptr,
              "restore_state should roll back the string bridge for direct structured value bindings");
#endif
}

void test_parser_last_using_alias_name_prefers_text_id_storage() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  parser.set_last_using_alias_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::Alias")));
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
  parser.register_typedef_binding(parser_test_text_id(parser, "TypeAlias"), typedef_ts, true);

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
                                          qualified_concept_text_id);
  const c4c::QualifiedNameKey qualified_concept_key =
      parser.known_fn_name_key_in_context(ns_context_id,
                                          qualified_concept_text_id);
  expect_true(parser.has_structured_concept_name(qualified_concept_key),
              "qualified concept lookup should use structured concept keys");
  c4c::Parser::VisibleNameResult resolved_concept;
  expect_true(parser.lookup_concept_in_context(ns_context_id,
                                               qualified_concept_text_id,
                                               &resolved_concept) &&
                  parser.visible_name_spelling(resolved_concept) ==
                      "ns::ScopedConcept",
              "qualified concept lookup should project structured concept keys");
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
  parser.register_typedef_binding(parser_test_text_id(parser, "TypeAlias"), typedef_ts, true);

  c4c::TypeSpec value_ts{};
  value_ts.array_size = -1;
  value_ts.inner_rank = -1;
  value_ts.base = c4c::TB_DOUBLE;
  parser.register_var_type_binding(parser_test_text_id(parser, "value"), value_ts);

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
  parser.register_typedef_binding(parser_test_text_id(parser, "TypeAlias"), typedef_ts, true);

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
  alias_ts.record_def = owner;

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
  expect_true(parser.find_typedef_type(parser.find_parser_text_id(resolved_name)) != nullptr &&
                  parser.find_typedef_type(parser.find_parser_text_id(resolved_name))->base == c4c::TB_INT,
              "dependent typename parsing should resolve a scope-local owner alias through the visible typedef facade");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_dependent_typename_owner_alias_prefers_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("RealBox");
  real_owner->member_typedef_names = arena.alloc_array<const char*>(1);
  real_owner->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_owner->n_member_typedefs = 1;
  real_owner->member_typedef_names[0] = arena.strdup("type");
  real_owner->member_typedef_types[0].array_size = -1;
  real_owner->member_typedef_types[0].inner_rank = -1;
  real_owner->member_typedef_types[0].base = c4c::TB_INT;

  c4c::Node* stale_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_owner->name = arena.strdup("StaleBox");
  stale_owner->n_member_typedefs = 0;
  parser.register_struct_definition_for_testing("StaleBox", stale_owner);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_STRUCT;
  alias_ts.tag = arena.strdup("StaleBox");
  alias_ts.record_def = real_owner;

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
              "dependent typename owner aliases should use record_def before stale rendered tags");

  expect_eq(resolved_name, "Alias::type",
            "dependent typename parsing should preserve alias-member spelling");
  const c4c::TypeSpec* resolved_type =
      parser.find_typedef_type(parser.find_parser_text_id(resolved_name));
  expect_true(resolved_type != nullptr && resolved_type->base == c4c::TB_INT,
              "record_def owner lookup should recover the real member typedef");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible typedef scope");
}

void test_parser_member_typedef_suffix_prefers_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("RealBox");
  real_owner->member_typedef_names = arena.alloc_array<const char*>(1);
  real_owner->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_owner->n_member_typedefs = 1;
  real_owner->member_typedef_names[0] = arena.strdup("type");
  real_owner->member_typedef_types[0].array_size = -1;
  real_owner->member_typedef_types[0].inner_rank = -1;
  real_owner->member_typedef_types[0].base = c4c::TB_INT;

  c4c::Node* stale_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_owner->name = arena.strdup("StaleBox");
  stale_owner->n_member_typedefs = 0;
  parser.register_struct_definition_for_testing("StaleBox", stale_owner);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_STRUCT;
  alias_ts.tag = arena.strdup("StaleBox");
  alias_ts.record_def = real_owner;
  parser.register_typedef_binding(parser_test_text_id(parser, "Alias"), alias_ts, true);

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  });

  const c4c::TypeSpec member_ts = parser.parse_base_type();
  expect_eq_int(static_cast<int>(member_ts.base), static_cast<int>(c4c::TB_INT),
                "member typedef suffix lookup should use typedef record_def before stale rendered tags");
}

void test_parser_member_typedef_suffix_uses_tagless_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("RealBox");
  real_owner->member_typedef_names = arena.alloc_array<const char*>(1);
  real_owner->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_owner->n_member_typedefs = 1;
  real_owner->member_typedef_names[0] = arena.strdup("type");
  real_owner->member_typedef_types[0].array_size = -1;
  real_owner->member_typedef_types[0].inner_rank = -1;
  real_owner->member_typedef_types[0].base = c4c::TB_INT;

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_STRUCT;
  alias_ts.record_def = real_owner;
  parser.register_typedef_binding(parser_test_text_id(parser, "Alias"), alias_ts, true);

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  });

  const c4c::TypeSpec member_ts = parser.parse_base_type();
  expect_eq_int(static_cast<int>(member_ts.base), static_cast<int>(c4c::TB_INT),
                "member typedef suffix lookup should use tagless record_def owners");
}

void test_parser_member_typedef_suffix_rejects_rendered_owner_fallbacks() {
  {
    c4c::Arena arena;
    c4c::TextTable texts;
    c4c::FileTable files;
    c4c::Parser parser({}, arena, &texts, &files,
                       c4c::SourceProfile::CppSubset);
    c4c::Token seed{};

    c4c::Node* rendered_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
    rendered_owner->name = arena.strdup("RenderedOnlyBox");
    rendered_owner->member_typedef_names = arena.alloc_array<const char*>(1);
    rendered_owner->member_typedef_types =
        arena.alloc_array<c4c::TypeSpec>(1);
    rendered_owner->n_member_typedefs = 1;
    rendered_owner->member_typedef_names[0] = arena.strdup("type");
    rendered_owner->member_typedef_types[0].array_size = -1;
    rendered_owner->member_typedef_types[0].inner_rank = -1;
    rendered_owner->member_typedef_types[0].base = c4c::TB_INT;
    parser.register_struct_definition_for_testing("RenderedOnlyBox",
                                                  rendered_owner);

    c4c::TypeSpec alias_ts{};
    alias_ts.array_size = -1;
    alias_ts.inner_rank = -1;
    alias_ts.base = c4c::TB_STRUCT;
    alias_ts.tag = arena.strdup("RenderedOnlyBox");
    parser.register_typedef_binding(parser_test_text_id(parser, "Alias"),
                                    alias_ts, true);

    parser.replace_token_stream_for_testing({
        parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
        parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
        parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
    });

    const c4c::TypeSpec member_ts = parser.parse_base_type();
    expect_true(member_ts.base == c4c::TB_STRUCT &&
                    member_ts.deferred_member_type_name != nullptr,
                "member typedef suffix lookup should reject rendered tag-map owner recovery");
  }

  {
    c4c::Arena arena;
    c4c::TextTable texts;
    c4c::FileTable files;
    c4c::Parser parser({}, arena, &texts, &files,
                       c4c::SourceProfile::CppSubset);
    c4c::Token seed{};

    c4c::Node* structured_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
    structured_owner->name = arena.strdup("OwnerTag");
    structured_owner->n_member_typedefs = 0;

    c4c::TypeSpec alias_ts{};
    alias_ts.array_size = -1;
    alias_ts.inner_rank = -1;
    alias_ts.base = c4c::TB_STRUCT;
    alias_ts.tag = arena.strdup("OwnerTag");
    alias_ts.record_def = structured_owner;
    parser.register_typedef_binding(parser_test_text_id(parser, "Alias"),
                                    alias_ts, true);

    c4c::TypeSpec rendered_member_ts{};
    rendered_member_ts.array_size = -1;
    rendered_member_ts.inner_rank = -1;
    rendered_member_ts.base = c4c::TB_LONG;
    parser.register_typedef_binding(
        parser_test_text_id(parser, "OwnerTag::type"), rendered_member_ts,
        true);

    parser.replace_token_stream_for_testing({
        parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
        parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
        parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
    });

    const c4c::TypeSpec member_ts = parser.parse_base_type();
    expect_true(member_ts.base == c4c::TB_STRUCT &&
                    member_ts.deferred_member_type_name != nullptr,
                "member typedef suffix lookup should reject rendered owner::member typedef storage");
  }
}

void test_parser_nested_dependent_typename_prefers_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::Node* real_nested = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_nested->name = arena.strdup("RealNested");
  real_nested->member_typedef_names = arena.alloc_array<const char*>(1);
  real_nested->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_nested->n_member_typedefs = 1;
  real_nested->member_typedef_names[0] = arena.strdup("type");
  real_nested->member_typedef_types[0].array_size = -1;
  real_nested->member_typedef_types[0].inner_rank = -1;
  real_nested->member_typedef_types[0].base = c4c::TB_LONG;

  c4c::Node* stale_nested = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_nested->name = arena.strdup("StaleNested");
  stale_nested->n_member_typedefs = 0;
  parser.register_struct_definition_for_testing("StaleNested", stale_nested);

  c4c::TypeSpec nested_field_type{};
  nested_field_type.array_size = -1;
  nested_field_type.inner_rank = -1;
  nested_field_type.base = c4c::TB_STRUCT;
  nested_field_type.tag = arena.strdup("StaleNested");
  nested_field_type.record_def = real_nested;

  c4c::Node* nested_field = parser.make_node(c4c::NK_DECL, 1);
  nested_field->name = arena.strdup("Nested");
  nested_field->type = nested_field_type;

  c4c::Node* root = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  root->name = arena.strdup("Root");
  root->unqualified_text_id = parser_test_text_id(parser, "Root");
  root->namespace_context_id = 0;
  root->n_fields = 1;
  root->fields = arena.alloc_array<c4c::Node*>(1);
  root->fields[0] = nested_field;
  parser.register_struct_definition_for_testing("Root", root);

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Root"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Nested"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  });

  std::string resolved_name;
  expect_true(parser.parse_dependent_typename_specifier(&resolved_name),
              "nested dependent typename owners should use field record_def before stale rendered tags");

  expect_eq(resolved_name, "Root::Nested::type",
            "nested dependent typename parsing should preserve nested spelling");
  const c4c::TypeSpec* resolved_type =
      parser.find_typedef_type(parser.find_parser_text_id(resolved_name));
  expect_true(resolved_type != nullptr && resolved_type->base == c4c::TB_LONG,
              "nested record_def owner lookup should recover the real nested member typedef");
}

void test_parser_nested_dependent_typename_uses_tagless_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  c4c::Node* real_nested = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_nested->name = arena.strdup("RealNested");
  real_nested->member_typedef_names = arena.alloc_array<const char*>(1);
  real_nested->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  real_nested->n_member_typedefs = 1;
  real_nested->member_typedef_names[0] = arena.strdup("type");
  real_nested->member_typedef_types[0].array_size = -1;
  real_nested->member_typedef_types[0].inner_rank = -1;
  real_nested->member_typedef_types[0].base = c4c::TB_LONG;

  c4c::TypeSpec nested_field_type{};
  nested_field_type.array_size = -1;
  nested_field_type.inner_rank = -1;
  nested_field_type.base = c4c::TB_STRUCT;
  nested_field_type.record_def = real_nested;

  c4c::Node* nested_field = parser.make_node(c4c::NK_DECL, 1);
  nested_field->name = arena.strdup("Nested");
  nested_field->type = nested_field_type;

  c4c::Node* root = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  root->name = arena.strdup("Root");
  root->unqualified_text_id = parser_test_text_id(parser, "Root");
  root->namespace_context_id = 0;
  root->n_fields = 1;
  root->fields = arena.alloc_array<c4c::Node*>(1);
  root->fields[0] = nested_field;
  parser.register_struct_definition_for_testing("Root", root);

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Root"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Nested"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
  });

  std::string resolved_name;
  expect_true(parser.parse_dependent_typename_specifier(&resolved_name),
              "nested dependent typename owners should use tagless field record_def");

  const c4c::TypeSpec* resolved_type =
      parser.find_typedef_type(parser.find_parser_text_id(resolved_name));
  expect_true(resolved_type != nullptr && resolved_type->base == c4c::TB_LONG,
              "tagless nested record_def owner lookup should recover the real member typedef");
}

void test_parser_record_ctor_probe_prefers_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("RealBox");
  real_owner->n_fields = 0;

  c4c::Node* stale_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_owner->name = arena.strdup("StaleBox");
  stale_owner->n_fields = -1;
  parser.register_struct_definition_for_testing("StaleBox", stale_owner);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_STRUCT;
  alias_ts.tag = arena.strdup("StaleBox");
  alias_ts.record_def = real_owner;
  parser.register_typedef_binding(parser_test_text_id(parser, "Alias"), alias_ts, true);

  c4c::TypeSpec probe{};
  probe.array_size = -1;
  probe.inner_rank = -1;
  probe.base = c4c::TB_TYPEDEF;
  probe.tag = arena.strdup("Alias");

  expect_true(parser.resolves_to_record_ctor_type(probe),
              "record constructor probes should use typedef record_def before rendered maps");

  c4c::TypeSpec tagless_probe = alias_ts;
  tagless_probe.tag = nullptr;
  expect_true(parser.resolves_to_record_ctor_type(tagless_probe),
              "record constructor probes should use tagless record_def owners");
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

  expect_true(parser.is_typedef_name(parser_test_text_id(parser, "Alias")),
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

  expect_true(!parser.has_conflicting_user_typedef_binding(alias_text, alias_ts),
              "typedef conflict checks should accept matching scope-local visible typedefs");
  expect_true(parser.has_conflicting_user_typedef_binding(alias_text, different_ts),
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

  expect_true(parser.find_typedef_type(parser_test_text_id(parser, "Alias")) == nullptr,
              "local visible typedef fixtures should not populate the flat typedef table");

  std::string saved_struct_tag;
  std::string struct_source_name;
  c4c::begin_record_body_context(parser, "Widget", "Alias", &saved_struct_tag,
                                 &struct_source_name);

  expect_eq(struct_source_name, "Alias",
            "record body setup should preserve the template origin spelling");
  expect_true(parser.find_typedef_type(parser_test_text_id(parser, "Alias")) == nullptr,
              "record body setup should not synthesize a flat typedef binding when a visible local alias already exists");
  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias"));
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
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "Target"));
  parser.register_known_fn_name(target_key);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "corrupted");

  expect_eq(parser.resolve_visible_type_name("Alias"), "Target",
            "unqualified value aliases should project the structured target through the visible typedef facade when the target key is known");

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
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "Target"));
  parser.register_known_fn_name(target_key);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "corrupted");

  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias"));
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
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "Target"));
  parser.register_known_fn_name(target_key);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "corrupted");

  const c4c::TypeSpec* visible_alias =
      parser.find_visible_typedef_type(alias_text);
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

  c4c::TypeSpec stale_rendered_ts{};
  stale_rendered_ts.array_size = -1;
  stale_rendered_ts.inner_rank = -1;
  stale_rendered_ts.base = c4c::TB_DOUBLE;

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::TextId target_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Target");
  const c4c::TextId alias_text = texts.intern("Alias");
  parser.register_structured_typedef_binding_in_context(ns_context, target_text,
                                                        target_ts);
  parser.register_typedef_binding(parser_test_text_id(parser, "ns::Target"),
                                  stale_rendered_ts, true);
  const c4c::QualifiedNameKey target_key =
      parser.struct_typedef_key_in_context(ns_context, target_text);
  parser.register_known_fn_name(target_key);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "corrupted");

  expect_eq(parser.resolve_visible_type_name("Alias"), "ns::Target",
            "qualified value aliases should keep namespace-qualified typedef resolution intact");
  const c4c::TypeSpec* visible_alias = parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias"));
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "qualified value aliases should use structured target metadata instead of the rendered typedef spelling");
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
  parser.register_var_type_binding(parser_test_text_id(parser, "ns::Target"), target_ts);

  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "using-import fixture should intern the alias text");
  expect_true(parser.replace_using_value_alias_compatibility_name_for_testing(
                  0, alias_text, "corrupted"),
              "using-import fixture should record a value-alias entry");
  expect_eq(parser.resolve_visible_value_name(alias_text, "Target"), "ns::Target",
            "using-import lookup should prefer the structured target key over the compatibility bridge");
}

void test_parser_using_value_import_prefers_structured_type_over_corrupt_rendered_name() {
  c4c::Lexer lexer("using ns::Target;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  c4c::TypeSpec bridge_ts{};
  bridge_ts.array_size = -1;
  bridge_ts.inner_rank = -1;
  bridge_ts.base = c4c::TB_DOUBLE;

  parser.register_structured_var_type_binding(
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::Target")), target_ts);
  parser.register_var_type_binding(parser_test_text_id(parser, "corrupted"), bridge_ts);

  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "using-import fixture should intern the alias text");
  expect_true(parser.replace_using_value_alias_compatibility_name_for_testing(
                  0, alias_text, "corrupted"),
              "using-import fixture should record a value-alias entry");

  const c4c::TypeSpec* visible_alias = parser.find_visible_var_type(parser_test_text_id(parser, "Target"));
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "using value imports should prefer the structured target type before rendered fallback names");
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
  parser.register_var_type_binding(parser_test_text_id(parser, "Target"), target_ts);

  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Target");
  expect_true(alias_text != c4c::kInvalidText,
              "global using-import fixture should intern the alias text");
  expect_true(parser.replace_using_value_alias_compatibility_name_for_testing(
                  0, alias_text, "corrupted"),
              "global using-import fixture should record a value-alias entry");
  expect_eq(parser.resolve_visible_value_name(alias_text, "Target"), "Target",
            "global using-import lookup should keep the global target spelling instead of introducing a leading scope bridge");
}

void test_parser_out_of_class_operator_registers_structured_global_key() {
  c4c::Lexer lexer("::Owner::operator bool();\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* fn = parse_top_level(parser);

  expect_true(fn != nullptr && fn->kind == c4c::NK_FUNCTION,
              "global-qualified out-of-class operator declarations should parse as functions");
  expect_eq(fn->name, "Owner::operator_bool",
            "out-of-class operator final spelling should keep the existing rendered display name");
  expect_true(parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "::Owner::operator_bool"))),
              "out-of-class operator registration should keep the structured global-qualified key");
  expect_true(!parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "Owner::operator_bool"))),
              "out-of-class operator registration should not fall back to stale non-global rendered spelling when structure is available");
}

void test_parser_out_of_class_constructor_registers_structured_global_key() {
  c4c::Lexer lexer("::Owner::Owner();\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* fn = parse_top_level(parser);

  expect_true(fn != nullptr && fn->kind == c4c::NK_FUNCTION,
              "global-qualified out-of-class constructor declarations should parse as functions");
  expect_eq(fn->name, "Owner::Owner",
            "out-of-class constructor final spelling should keep the existing rendered display name");
  expect_true(parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "::Owner::Owner"))),
              "out-of-class constructor registration should keep the structured global-qualified key");
  expect_true(!parser.has_known_fn_name(parser.intern_semantic_name_key(parser_test_text_id(parser, "Owner::Owner"))),
              "out-of-class constructor registration should not fall back to stale non-global rendered spelling when structure is available");
}

void test_parser_namespace_lookup_rejects_type_projection_bridges_and_demotes_value_bridges() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec legacy_type_ts{};
  legacy_type_ts.array_size = -1;
  legacy_type_ts.inner_rank = -1;
  legacy_type_ts.base = c4c::TB_INT;

  c4c::TypeSpec legacy_ts{};
  legacy_ts.array_size = -1;
  legacy_ts.inner_rank = -1;
  legacy_ts.base = c4c::TB_SHORT;

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::TextId outer_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "outer");
  const int outer_context =
      parser.ensure_named_namespace_context(0, outer_text, "outer");
  const c4c::TextId type_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "LegacyOnlyType");
  const c4c::TextId qualified_type_text =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "ns::LegacyOnlyType");
  parser.binding_state_.non_atom_typedef_types[qualified_type_text] =
      legacy_type_ts;

  const c4c::TextId value_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "LegacyOnlyValue");
  const c4c::TextId qualified_value_text =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "ns::LegacyOnlyValue");
  parser.binding_state_.non_atom_var_types[qualified_value_text] = legacy_ts;

  c4c::Parser::VisibleNameResult resolved_type;
  expect_true(!parser.lookup_type_in_context(ns_context, type_text,
                                             &resolved_type),
              "namespace type lookup should reject legacy rendered typedef storage");
  resolved_type = {};
  expect_true(!parser.lookup_type_in_context(ns_context, c4c::kInvalidText,
                                             &resolved_type),
              "namespace type lookup should reject TextId-less rendered typedef storage");

  c4c::Parser::VisibleNameResult resolved_value;
  expect_true(!parser.lookup_value_in_context(ns_context, value_text,
                                              &resolved_value),
              "namespace value lookup should not promote legacy-only rendered names when a valid TextId lookup misses structured storage");
  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(ns_context, c4c::kInvalidText,
                                              &resolved_value),
              "namespace value lookup should reject TextId-less rendered-name compatibility");

  c4c::Parser::QualifiedNameRef qn;
  qn.qualifier_segments.push_back("ns");
  qn.qualifier_text_ids.push_back(ns_text);
  qn.base_name = "LegacyOnlyType";
  qn.base_text_id = type_text;
  expect_true(!parser.resolve_qualified_type(qn),
              "qualified type resolution should reject legacy rendered typedef storage");
  qn.base_text_id = c4c::kInvalidText;
  expect_true(!parser.resolve_qualified_type(qn),
              "qualified type resolution should reject explicit TextId-less rendered typedef storage");

  qn.base_name = "LegacyOnlyValue";
  qn.base_text_id = value_text;
  expect_true(!parser.resolve_qualified_value(qn),
              "qualified value resolution should not promote legacy-only rendered names for valid TextIds");
  qn.base_text_id = c4c::kInvalidText;
  const c4c::Parser::VisibleNameResult qualified_value =
      parser.resolve_qualified_value(qn);
  expect_true(!static_cast<bool>(qualified_value),
              "qualified value resolution should reject explicit TextId-less compatibility");

  parser.namespace_state_.using_namespace_contexts[0].push_back(ns_context);
  resolved_type = {};
  expect_true(!parser.lookup_type_in_context(0, type_text,
                                             &resolved_type),
              "namespace-import type lookup should reject imported legacy rendered typedef storage");

  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(0, value_text,
                                              &resolved_value),
              "namespace-import value lookup should not promote imported legacy-only rendered names for valid TextIds");
  resolved_value = {};
  expect_true(!parser.lookup_value_in_context(0, c4c::kInvalidText,
                                              &resolved_value),
              "namespace-import value lookup should reject TextId-less imported compatibility");

  parser.register_structured_typedef_binding_in_context(ns_context, type_text,
                                                        legacy_type_ts);
  resolved_type = {};
  expect_true(parser.lookup_type_in_context(ns_context, type_text,
                                            &resolved_type) &&
                  parser.visible_name_spelling(resolved_type) ==
                      "ns::LegacyOnlyType",
              "namespace type lookup should keep structured typedef authority");
  resolved_type = {};
  expect_true(parser.lookup_type_in_context(0, type_text,
                                            &resolved_type) &&
                  parser.visible_name_spelling(resolved_type) ==
                      "LegacyOnlyType",
              "global visible type lookup should keep imported structured typedef authority");
  parser.namespace_state_.using_namespace_contexts[outer_context].push_back(ns_context);
  resolved_type = {};
  expect_true(parser.lookup_type_in_context(outer_context, type_text,
                                            &resolved_type) &&
                  parser.visible_name_spelling(resolved_type) ==
                      "ns::LegacyOnlyType",
              "non-global namespace-import type lookup should keep structured typedef authority");
}

void test_parser_qualified_type_parse_fallback_requires_structured_type() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_ts{};
  alias_ts.array_size = -1;
  alias_ts.inner_rank = -1;
  alias_ts.base = c4c::TB_INT;

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");

  c4c::Parser::QualifiedNameRef qn;
  qn.qualifier_segments.push_back("ns");
  qn.qualifier_text_ids.push_back(ns_text);
  qn.base_name = "Alias";
  qn.base_text_id = alias_text;

  c4c::QualifiedTypeProbe unresolved_probe =
      c4c::probe_qualified_type(parser, qn);
  expect_true(!unresolved_probe.has_resolved_typedef,
              "unresolved qualified type probes should not claim structured type authority");
  expect_true(!c4c::has_qualified_type_parse_fallback(unresolved_probe),
              "qualified type parse fallback should not accept rendered unresolved spelling");
  expect_true(!c4c::can_start_qualified_type_declaration(
                  parser, unresolved_probe, 3, c4c::TokenKind::Less),
              "qualified type declaration starts should not use rendered unresolved spelling");

  parser.register_typedef_binding(
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns::Alias"),
      alias_ts, true);
  unresolved_probe = c4c::probe_qualified_type(parser, qn);
  expect_true(!c4c::has_qualified_type_parse_fallback(unresolved_probe),
              "qualified type parse fallback should ignore legacy rendered typedef storage");

  c4c::TypeSpec enum_ts{};
  enum_ts.array_size = -1;
  enum_ts.inner_rank = -1;
  enum_ts.base = c4c::TB_ENUM;
  parser.register_typedef_binding(
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns::Alias"),
      enum_ts, true);
  unresolved_probe = c4c::probe_qualified_type(parser, qn);
  expect_true(!c4c::has_qualified_type_parse_fallback(unresolved_probe),
              "qualified type parse fallback should ignore legacy rendered enum storage");

  c4c::Lexer missing_tpl_lexer("ns::Missing<int> value;\n",
                               c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> missing_tpl_tokens =
      missing_tpl_lexer.scan_all();
  c4c::Arena missing_tpl_arena;
  c4c::Parser missing_tpl_parser(
      missing_tpl_tokens, missing_tpl_arena, &missing_tpl_lexer.text_table(),
      &missing_tpl_lexer.file_table(), c4c::SourceProfile::CppSubset);
  c4c::TypeSpec missing_tpl_ts{};
  expect_true(!c4c::try_parse_qualified_base_type(missing_tpl_parser,
                                                  &missing_tpl_ts),
              "unknown namespace-qualified template ids should not parse as types from spelling alone");

  parser.register_structured_typedef_binding_in_context(ns_context, alias_text,
                                                        alias_ts);
  const c4c::QualifiedTypeProbe structured_probe =
      c4c::probe_qualified_type(parser, qn);
  expect_true(structured_probe.has_resolved_typedef,
              "structured qualified typedef probes should keep type authority");
  expect_true(c4c::has_qualified_type_parse_fallback(structured_probe),
              "qualified type parse fallback should accept structured type probes");
}

void test_parser_qualified_functional_cast_owner_requires_structured_authority() {
  c4c::Arena legacy_arena;
  c4c::TextTable legacy_texts;
  c4c::FileTable legacy_files;
  c4c::Parser legacy_parser({}, legacy_arena, &legacy_texts, &legacy_files,
                            c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::TextId legacy_ns_text =
      parser_test_text_id(legacy_parser, "ns");
  legacy_parser.ensure_named_namespace_context(0, legacy_ns_text, "ns");
  const c4c::TextId legacy_t_text =
      parser_test_text_id(legacy_parser, "T");
  legacy_parser.push_template_scope(
      c4c::Parser::TemplateScopeKind::FreeFunctionTemplate,
      {{.name_text_id = legacy_t_text, .name = "T", .is_nttp = false}});
  c4c::Node* legacy_owner =
      legacy_parser.make_node(c4c::NK_STRUCT_DEF, 1);
  legacy_owner->name = legacy_arena.strdup("ns::LegacyOwner");
  legacy_owner->n_member_typedefs = 1;
  legacy_owner->member_typedef_names =
      legacy_arena.alloc_array<const char*>(1);
  legacy_owner->member_typedef_types =
      legacy_arena.alloc_array<c4c::TypeSpec>(1);
  legacy_owner->member_typedef_names[0] = legacy_arena.strdup("Member");
  legacy_owner->member_typedef_types[0].array_size = -1;
  legacy_owner->member_typedef_types[0].inner_rank = -1;
  legacy_owner->member_typedef_types[0].base = c4c::TB_INT;
  legacy_parser.definition_state_.defined_struct_tags.insert("ns::LegacyOwner");
  legacy_parser.register_struct_definition_for_testing("ns::LegacyOwner",
                                                       legacy_owner);
  c4c::TypeSpec legacy_owner_ts{};
  legacy_owner_ts.array_size = -1;
  legacy_owner_ts.inner_rank = -1;
  legacy_owner_ts.base = c4c::TB_STRUCT;
  legacy_owner_ts.tag = legacy_arena.strdup("ns::LegacyOwner");
  legacy_parser.register_typedef_binding(
      parser_test_text_id(legacy_parser, "ns::LegacyOwner"),
      legacy_owner_ts, true);
  legacy_parser.replace_token_stream_for_testing({
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "LegacyOwner"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Member"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
  });

  c4c::Node* legacy_expr = c4c::parse_unary(legacy_parser);
  expect_true(legacy_expr != nullptr && legacy_expr->kind != c4c::NK_CAST,
              "legacy rendered owner typedef/tag storage alone should not authorize a qualified functional-cast parse");

  c4c::TextTable structured_texts;
  c4c::FileTable structured_files;
  c4c::Arena structured_arena;
  c4c::Parser structured_parser({}, structured_arena, &structured_texts,
                                &structured_files,
                                c4c::SourceProfile::CppSubset);
  const c4c::TextId structured_ns_text =
      parser_test_text_id(structured_parser, "ns");
  const int structured_ns_context =
      structured_parser.ensure_named_namespace_context(
          0, structured_ns_text, "ns");
  const c4c::TextId owner_text =
      parser_test_text_id(structured_parser, "Owner");
  c4c::Node* structured_owner =
      structured_parser.make_node(c4c::NK_STRUCT_DEF, 1);
  structured_owner->name = structured_arena.strdup("Owner");
  structured_owner->unqualified_text_id = owner_text;
  structured_owner->namespace_context_id = structured_ns_context;
  structured_owner->n_member_typedefs = 1;
  structured_owner->member_typedef_names =
      structured_arena.alloc_array<const char*>(1);
  structured_owner->member_typedef_types =
      structured_arena.alloc_array<c4c::TypeSpec>(1);
  structured_owner->member_typedef_names[0] =
      structured_arena.strdup("Member");
  structured_owner->member_typedef_types[0].array_size = -1;
  structured_owner->member_typedef_types[0].inner_rank = -1;
  structured_owner->member_typedef_types[0].base = c4c::TB_INT;
  structured_parser.register_struct_definition_for_testing("Owner",
                                                           structured_owner);

  c4c::Parser::QualifiedNameRef owner_qn;
  owner_qn.qualifier_segments.push_back("ns");
  owner_qn.qualifier_text_ids.push_back(structured_ns_text);
  owner_qn.base_name = "Owner";
  owner_qn.base_text_id = owner_text;
  expect_true(c4c::qualified_type_owner_has_structured_authority(
                  structured_parser, owner_qn),
              "structured record metadata should authorize the qualified owner");
}

void test_parser_qualified_member_typedef_lookup_requires_structured_metadata() {
  c4c::Token seed{};

  c4c::Arena legacy_arena;
  c4c::TextTable legacy_texts;
  c4c::FileTable legacy_files;
  c4c::Parser legacy_parser({}, legacy_arena, &legacy_texts, &legacy_files,
                            c4c::SourceProfile::CppSubset);
  const c4c::TextId legacy_ns_text =
      parser_test_text_id(legacy_parser, "ns");
  legacy_parser.ensure_named_namespace_context(0, legacy_ns_text, "ns");
  c4c::Node* legacy_owner =
      legacy_parser.make_node(c4c::NK_STRUCT_DEF, 1);
  legacy_owner->name = legacy_arena.strdup("ns::LegacyOwner");
  legacy_owner->n_member_typedefs = 1;
  legacy_owner->member_typedef_names =
      legacy_arena.alloc_array<const char*>(1);
  legacy_owner->member_typedef_types =
      legacy_arena.alloc_array<c4c::TypeSpec>(1);
  legacy_owner->member_typedef_names[0] = legacy_arena.strdup("Member");
  legacy_owner->member_typedef_types[0].array_size = -1;
  legacy_owner->member_typedef_types[0].inner_rank = -1;
  legacy_owner->member_typedef_types[0].base = c4c::TB_INT;
  legacy_parser.register_struct_definition_for_testing("ns::LegacyOwner",
                                                       legacy_owner);
  c4c::TypeSpec legacy_owner_ts{};
  legacy_owner_ts.array_size = -1;
  legacy_owner_ts.inner_rank = -1;
  legacy_owner_ts.base = c4c::TB_STRUCT;
  legacy_owner_ts.tag = legacy_arena.strdup("ns::LegacyOwner");
  legacy_parser.register_typedef_binding(
      parser_test_text_id(legacy_parser, "ns::LegacyOwner"),
      legacy_owner_ts, true);
  legacy_parser.replace_token_stream_for_testing({
      legacy_parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "LegacyOwner"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      legacy_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Member"),
  });

  std::string legacy_name;
  expect_true(legacy_parser.parse_dependent_typename_specifier(&legacy_name),
              "dependent typename spelling should still be consumed");
  expect_true(legacy_parser.find_typedef_type(
                  legacy_parser.find_parser_text_id(legacy_name)) == nullptr,
              "legacy rendered owner typedef/tag storage alone should not resolve member typedefs");

  c4c::Arena rendered_member_arena;
  c4c::TextTable rendered_member_texts;
  c4c::FileTable rendered_member_files;
  c4c::Parser rendered_member_parser(
      {}, rendered_member_arena, &rendered_member_texts,
      &rendered_member_files, c4c::SourceProfile::CppSubset);
  const c4c::TextId rendered_ns_text =
      parser_test_text_id(rendered_member_parser, "ns");
  const int rendered_ns_context =
      rendered_member_parser.ensure_named_namespace_context(
          0, rendered_ns_text, "ns");
  const c4c::TextId rendered_owner_text =
      parser_test_text_id(rendered_member_parser, "Owner");
  c4c::Node* rendered_owner =
      rendered_member_parser.make_node(c4c::NK_STRUCT_DEF, 1);
  rendered_owner->name = rendered_member_arena.strdup("RenderedOwner");
  rendered_owner->unqualified_text_id = rendered_owner_text;
  rendered_owner->namespace_context_id = rendered_ns_context;
  rendered_owner->n_member_typedefs = 0;
  rendered_member_parser.register_struct_definition_for_testing("ns::Owner",
                                                                rendered_owner);
  c4c::TypeSpec rendered_member_ts{};
  rendered_member_ts.array_size = -1;
  rendered_member_ts.inner_rank = -1;
  rendered_member_ts.base = c4c::TB_LONG;
  rendered_member_parser.register_typedef_binding(
      parser_test_text_id(rendered_member_parser, "RenderedOwner::Member"),
      rendered_member_ts, true);
  rendered_member_parser.replace_token_stream_for_testing({
      rendered_member_parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      rendered_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      rendered_member_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      rendered_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Owner"),
      rendered_member_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      rendered_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Member"),
  });

  std::string rendered_member_name;
  expect_true(rendered_member_parser.parse_dependent_typename_specifier(
                  &rendered_member_name),
              "structured owners should still be consumable without member metadata");
  expect_true(rendered_member_parser.find_typedef_type(
                  rendered_member_parser.find_parser_text_id(
                      rendered_member_name)) == nullptr,
              "rendered owner::member typedef storage should not replace direct member typedef metadata");

  c4c::Arena structured_arena;
  c4c::TextTable structured_texts;
  c4c::FileTable structured_files;
  c4c::Parser structured_parser({}, structured_arena, &structured_texts,
                                &structured_files,
                                c4c::SourceProfile::CppSubset);
  const c4c::TextId structured_ns_text =
      parser_test_text_id(structured_parser, "ns");
  const int structured_ns_context =
      structured_parser.ensure_named_namespace_context(
          0, structured_ns_text, "ns");
  const c4c::TextId structured_owner_text =
      parser_test_text_id(structured_parser, "Owner");
  c4c::Node* structured_owner =
      structured_parser.make_node(c4c::NK_STRUCT_DEF, 1);
  structured_owner->name = structured_arena.strdup("ns::Owner");
  structured_owner->unqualified_text_id = structured_owner_text;
  structured_owner->namespace_context_id = structured_ns_context;
  structured_owner->n_member_typedefs = 1;
  structured_owner->member_typedef_names =
      structured_arena.alloc_array<const char*>(1);
  structured_owner->member_typedef_types =
      structured_arena.alloc_array<c4c::TypeSpec>(1);
  structured_owner->member_typedef_names[0] =
      structured_arena.strdup("Member");
  structured_owner->member_typedef_types[0].array_size = -1;
  structured_owner->member_typedef_types[0].inner_rank = -1;
  structured_owner->member_typedef_types[0].base = c4c::TB_INT;
  structured_parser.register_struct_definition_for_testing("ns::Owner",
                                                           structured_owner);
  structured_parser.replace_token_stream_for_testing({
      structured_parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      structured_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      structured_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      structured_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Owner"),
      structured_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      structured_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Member"),
  });

  std::string structured_name;
  expect_true(structured_parser.parse_dependent_typename_specifier(
                  &structured_name),
              "structured record metadata should authorize member typedef lookup");
  const c4c::TypeSpec* structured_type =
      structured_parser.find_typedef_type(
          structured_parser.find_parser_text_id(structured_name));
  expect_true(structured_type != nullptr && structured_type->base == c4c::TB_INT,
              "direct member typedef arrays should remain the member typedef authority");
}

void test_parser_record_body_member_typedef_writers_register_direct_keys() {
  c4c::Lexer lexer("namespace ns {\n"
                   "struct Owner {\n"
                   "  using UsingMember = int;\n"
                   "  typedef long TypedefMember;\n"
                   "};\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "record-body member typedef writer regression should parse");

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context =
      parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::TextId owner_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Owner");
  const c4c::TextId using_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "UsingMember");
  const c4c::TextId typedef_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "TypedefMember");

  c4c::TypeSpec stale_rendered_ts{};
  stale_rendered_ts.array_size = -1;
  stale_rendered_ts.inner_rank = -1;
  stale_rendered_ts.base = c4c::TB_DOUBLE;
  expect_true(parser.binding_state_.non_atom_typedef_types.count(
                  parser.parser_text_id_for_token(c4c::kInvalidText,
                                                  "ns::Owner::UsingMember")) ==
                  0,
              "record-body member typedef writer should not create rendered using-member storage");
  expect_true(parser.binding_state_.non_atom_typedef_types.count(
                  parser.parser_text_id_for_token(c4c::kInvalidText,
                                                  "ns::Owner::TypedefMember")) ==
                  0,
              "record-body member typedef writer should not create rendered typedef-member storage");
  parser.register_typedef_binding(
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "ns::Owner::UsingMember"),
      stale_rendered_ts, true);
  parser.register_typedef_binding(
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "ns::Owner::TypedefMember"),
      stale_rendered_ts, true);

  const c4c::QualifiedNameKey using_key =
      parser.record_member_typedef_key_in_context(ns_context, owner_text,
                                                  using_text);
  const c4c::QualifiedNameKey typedef_key =
      parser.record_member_typedef_key_in_context(ns_context, owner_text,
                                                  typedef_text);
  const c4c::TypeSpec* using_type = parser.find_typedef_type(using_key);
  const c4c::TypeSpec* typedef_type = parser.find_typedef_type(typedef_key);
  expect_true(using_type != nullptr && using_type->base == c4c::TB_INT,
              "record-body using member typedef writer should register a direct record/member key");
  expect_true(typedef_type != nullptr && typedef_type->base == c4c::TB_LONG,
              "record-body typedef member writer should register a direct record/member key");

  c4c::Parser::QualifiedNameRef using_qn;
  using_qn.qualifier_segments = {"ns", "Owner"};
  using_qn.qualifier_text_ids = {ns_text, owner_text};
  using_qn.base_name = "UsingMember";
  using_qn.base_text_id = using_text;
  const c4c::Parser::VisibleNameResult resolved_using =
      parser.resolve_qualified_type(using_qn);
  const c4c::TypeSpec* resolved_using_type =
      parser.find_typedef_type(resolved_using.key);
  expect_true(resolved_using_type != nullptr &&
                  resolved_using_type->base == c4c::TB_INT,
              "qualified member typedef reader should use the direct record/member key before stale rendered storage");
}

void test_parser_template_record_member_typedef_writer_registers_dependent_key() {
  c4c::Lexer lexer("namespace ns {\n"
                   "template <typename T>\n"
                   "struct Owner {\n"
                   "  using Member = T;\n"
                   "};\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "template record member typedef carrier fixture should parse");

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context =
      parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::TextId owner_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Owner");
  const c4c::TextId member_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Member");
  const c4c::TextId param_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "T");
  const c4c::QualifiedNameKey member_key =
      parser.record_member_typedef_key_in_context(ns_context, owner_text,
                                                  member_text);

  const c4c::TypeSpec* dependent_member =
      parser.find_dependent_record_member_typedef_type(member_key);
  expect_true(dependent_member != nullptr &&
                  dependent_member->base == c4c::TB_TYPEDEF &&
                  dependent_member->tag != nullptr &&
                  parser.find_parser_text_id(dependent_member->tag) ==
                      param_text,
              "template record member typedef writer should register a dependent record/member key");

  c4c::TypeSpec stale_rendered_ts{};
  stale_rendered_ts.array_size = -1;
  stale_rendered_ts.inner_rank = -1;
  stale_rendered_ts.base = c4c::TB_DOUBLE;
  parser.register_typedef_binding(
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "ns::Owner::Member"),
      stale_rendered_ts, true);

  const c4c::TypeSpec* resolved_member = parser.find_typedef_type(member_key);
  expect_true(resolved_member != nullptr &&
                  resolved_member->base == c4c::TB_TYPEDEF &&
                  resolved_member->tag != nullptr &&
                  parser.find_parser_text_id(resolved_member->tag) ==
                      param_text,
              "dependent record/member carrier should win over stale rendered typedef storage");

  c4c::Parser::QualifiedNameRef member_qn;
  member_qn.qualifier_segments = {"ns", "Owner"};
  member_qn.qualifier_text_ids = {ns_text, owner_text};
  member_qn.base_name = "Member";
  member_qn.base_text_id = member_text;
  const c4c::Parser::VisibleNameResult visible_member =
      parser.resolve_qualified_type(member_qn);
  const c4c::TypeSpec* visible_member_type =
      parser.find_typedef_type(visible_member.key);
  expect_true(visible_member_type != nullptr &&
                  visible_member_type->base == c4c::TB_TYPEDEF &&
                  visible_member_type->tag != nullptr &&
                  parser.find_parser_text_id(visible_member_type->tag) ==
                      param_text,
              "qualified type lookup should consume the dependent record/member carrier");
}

void test_parser_c_style_cast_member_typedef_uses_structured_metadata() {
  c4c::Lexer lexer("struct Box {\n"
                   "  using AliasL = int&;\n"
                   "  typedef int&& AliasR;\n"
                   "};\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "member typedef cast fixture should parse the record body");

  expect_true(parser.binding_state_.non_atom_typedef_types.count(
                  parser.parser_text_id_for_token(c4c::kInvalidText,
                                                  "Box::AliasL")) == 0,
              "record-body member typedef writer should not create rendered AliasL storage");
  expect_true(parser.binding_state_.non_atom_typedef_types.count(
                  parser.parser_text_id_for_token(c4c::kInvalidText,
                                                  "Box::AliasR")) == 0,
              "record-body member typedef writer should not create rendered AliasR storage");

  c4c::Token seed{};
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "AliasL"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "x"),
  });
  c4c::Node* lvalue_cast = c4c::parse_unary(parser);
  expect_true(lvalue_cast != nullptr && lvalue_cast->kind == c4c::NK_CAST,
              "C-style casts should parse member typedefs through structured record/member metadata");
  expect_true(lvalue_cast->type.base == c4c::TB_INT &&
                  lvalue_cast->type.is_lvalue_ref,
              "structured AliasL metadata should recover the lvalue-reference target type");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "AliasR"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "x"),
  });
  c4c::Node* rvalue_cast = c4c::parse_unary(parser);
  expect_true(rvalue_cast != nullptr && rvalue_cast->kind == c4c::NK_CAST,
              "C-style casts should parse typedef members after rendered mirrors are removed");
  expect_true(rvalue_cast->type.base == c4c::TB_INT &&
                  rvalue_cast->type.is_rvalue_ref,
              "structured AliasR metadata should recover the rvalue-reference target type");
}

void test_parser_template_instantiation_member_typedef_uses_concrete_key() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token box_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box");
  const c4c::Token payload_alias_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "PayloadAlias");
  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");

  c4c::TypeSpec param_ts{};
  param_ts.array_size = -1;
  param_ts.inner_rank = -1;
  param_ts.base = c4c::TB_TYPEDEF;
  param_ts.tag = arena.strdup("T");

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->unqualified_name = arena.strdup("Box");
  primary->unqualified_text_id = box_token.text_id;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->n_template_args = 0;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;
  primary->n_member_typedefs = 1;
  primary->member_typedef_names = arena.alloc_array<const char*>(1);
  primary->member_typedef_names[0] = arena.strdup("Alias");
  primary->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  primary->member_typedef_types[0] = param_ts;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), box_token.text_id, primary);
  const c4c::QualifiedNameKey box_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), box_token.text_id);
  parser.template_state_.template_struct_defs_by_key[box_key] = primary;
  c4c::TypeSpec box_alias{};
  box_alias.array_size = -1;
  box_alias.inner_rank = -1;
  box_alias.base = c4c::TB_STRUCT;
  box_alias.tag = arena.strdup("Box");
  parser.register_typedef_binding(box_token.text_id, box_alias, true);

  c4c::Node* payload = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  payload->name = arena.strdup("Payload");
  parser.register_struct_definition_for_testing("Payload", payload);
  c4c::TypeSpec payload_alias{};
  payload_alias.array_size = -1;
  payload_alias.inner_rank = -1;
  payload_alias.base = c4c::TB_STRUCT;
  payload_alias.tag = arena.strdup("Payload");
  payload_alias.record_def = payload;
  parser.register_typedef_binding(payload_alias_token.text_id, payload_alias,
                                  true);

  parser.replace_token_stream_for_testing({
      box_token,
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      payload_alias_token,
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });
  const c4c::TypeSpec box_ts = parser.parse_base_type();
  expect_true(box_ts.record_def != nullptr,
              "template member typedef carrier test should instantiate Box");

  c4c::Parser::TemplateArgParseResult concrete_arg{};
  concrete_arg.is_value = false;
  concrete_arg.type = payload_alias;
  const c4c::ParserTemplateState::TemplateInstantiationKey concrete_owner{
      box_key, c4c::make_template_instantiation_argument_keys({concrete_arg})};
  const c4c::TypeSpec* stored_member =
      parser.find_template_instantiation_member_typedef_type(concrete_owner,
                                                             alias_text);
  expect_true(stored_member != nullptr && stored_member->record_def == payload,
              "template instantiation member typedef writer should store by concrete owner key plus member TextId");

  c4c::TypeSpec stale_rendered_ts{};
  stale_rendered_ts.array_size = -1;
  stale_rendered_ts.inner_rank = -1;
  stale_rendered_ts.base = c4c::TB_DOUBLE;
  parser.register_typedef_binding(
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "Box_Payload::Alias"),
      stale_rendered_ts, true);
  box_ts.record_def->member_typedef_types[0] = stale_rendered_ts;

  parser.replace_token_stream_for_testing({
      box_token,
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      payload_alias_token,
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });
  const c4c::TypeSpec alias_ts = parser.parse_base_type();
  expect_true(alias_ts.record_def == payload,
              "template instantiation member typedef reader should prefer the concrete owner carrier over stale rendered scoped storage");
}

void test_parser_namespace_typedef_registration_stays_namespace_scoped() {
  c4c::Lexer lexer("namespace ns {\n"
                   "typedef int Alias;\n"
                   "}\n"
                   "ns::Alias value;\n"
                   "int main() {\n"
                   "  typedef int Local;\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "namespace typedef registration regression should parse as a program");

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  const c4c::TextId local_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Local");

  c4c::Parser::QualifiedNameRef qn;
  qn.qualifier_segments.push_back("ns");
  qn.qualifier_text_ids.push_back(ns_text);
  qn.base_name = "Alias";
  qn.base_text_id = alias_text;
  const c4c::QualifiedTypeProbe alias_probe =
      c4c::probe_qualified_type(parser, qn);
  expect_true(alias_probe.has_resolved_typedef,
              "parsed namespace-scope typedefs should register structured qualified type authority");

  qn.base_name = "Local";
  qn.base_text_id = local_text;
  const c4c::QualifiedTypeProbe local_probe =
      c4c::probe_qualified_type(parser, qn);
  expect_true(!local_probe.has_resolved_typedef,
              "block-local typedefs should not become visible through namespace qualified lookup");
}

void test_parser_using_value_alias_rejects_missing_structured_target_bridge() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec bridge_ts{};
  bridge_ts.array_size = -1;
  bridge_ts.inner_rank = -1;
  bridge_ts.base = c4c::TB_DOUBLE;
  parser.register_var_type_binding(parser_test_text_id(parser, "Bridge"), bridge_ts);

  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "RenderedTarget"));
  parser.register_using_value_alias_for_testing(
      0, alias_text, target_key, "Bridge");

  c4c::Parser::VisibleNameResult resolved;
  expect_true(!parser.lookup_using_value_alias(0, alias_text, &resolved),
              "structured using value aliases should not resolve through a compatibility bridge when the target key has no value/function binding");
  resolved = {};
  expect_true(!parser.lookup_using_value_alias(0, c4c::kInvalidText, &resolved),
              "using value aliases should require an existing alias TextId instead of recovering one from rendered spelling");
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "Alias")) == nullptr,
              "structured using value aliases should not project a compatibility bridge type when the target key is missing");

  c4c::TypeSpec explicit_ts{};
  explicit_ts.array_size = -1;
  explicit_ts.inner_rank = -1;
  explicit_ts.base = c4c::TB_DOUBLE;
  const c4c::Parser::SymbolId explicit_symbol =
      parser.parser_symbol_tables().intern_identifier("ExplicitBridge");
  parser.parser_symbol_tables().var_types[explicit_symbol] = explicit_ts;

  const c4c::TextId compat_alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "CompatAlias");
  parser.register_using_value_alias_for_testing(
      0, compat_alias_text, c4c::QualifiedNameKey{}, "ExplicitBridge");

  resolved = {};
  expect_true(parser.lookup_using_value_alias(0, compat_alias_text,
                                             &resolved) &&
                  parser.visible_name_spelling(resolved) == "ExplicitBridge",
              "using value aliases should preserve explicit compatibility-name resolution");
  const c4c::TypeSpec* compat_alias =
      parser.find_visible_var_type(parser_test_text_id(parser, "CompatAlias"));
  expect_true(compat_alias == nullptr,
              "using value aliases should not use explicit compatibility names as semantic value lookup authority");
}

void test_parser_using_value_alias_prefers_structured_target_type() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  c4c::TypeSpec bridge_ts{};
  bridge_ts.array_size = -1;
  bridge_ts.inner_rank = -1;
  bridge_ts.base = c4c::TB_DOUBLE;

  const c4c::TextId alias_text = texts.intern("Alias");
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::Target"));
  parser.register_structured_var_type_binding(target_key, target_ts);
  parser.register_var_type_binding(parser_test_text_id(parser, "Bridge"), bridge_ts);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "Bridge");

  const c4c::TypeSpec* visible_alias = parser.find_visible_var_type(parser_test_text_id(parser, "Alias"));
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "using value aliases should prefer the structured target key over the compatibility bridge type");
  expect_eq(parser.resolve_visible_value_name(alias_text, "Alias"), "ns::Target",
            "using value aliases should keep rendered output derived from the structured target key");
}

void test_parser_using_value_alias_respects_local_shadowing() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  c4c::TypeSpec local_ts{};
  local_ts.array_size = -1;
  local_ts.inner_rank = -1;
  local_ts.base = c4c::TB_DOUBLE;

  const c4c::TextId alias_text = texts.intern("Alias");
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "ns::Target"));
  parser.register_structured_var_type_binding(target_key, target_ts);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "corrupted");

  parser.push_local_binding_scope();
  parser.bind_local_value(alias_text, local_ts);
  const c4c::TypeSpec* visible_alias = parser.find_visible_var_type(parser_test_text_id(parser, "Alias"));
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_DOUBLE,
              "local value bindings should shadow using value aliases before structured alias resolution");
  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible value scope");
}

void test_parser_visible_value_alias_rejects_scope_local_target_bridge() {
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
      0, alias_text, parser.intern_semantic_name_key(parser_test_text_id(parser, "Target")), "corrupted");

  const c4c::TypeSpec* visible_alias = parser.find_visible_var_type(parser_test_text_id(parser, "Alias"));
  expect_true(visible_alias == nullptr,
              "structured using value aliases should not validate through a scope-local target spelling");
  expect_eq(parser.resolve_visible_value_name(alias_text, "Alias"), "Alias",
            "structured using value aliases should leave the alias unresolved when the structured target is missing");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local visible value scope");
}

void test_parser_using_type_import_prefers_structured_type_over_corrupt_rendered_name() {
  c4c::Lexer lexer("using ns::Alias;\n", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec target_ts{};
  target_ts.array_size = -1;
  target_ts.inner_rank = -1;
  target_ts.base = c4c::TB_INT;

  c4c::TypeSpec bridge_ts{};
  bridge_ts.array_size = -1;
  bridge_ts.inner_rank = -1;
  bridge_ts.base = c4c::TB_DOUBLE;

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "ns");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  parser.register_structured_typedef_binding_in_context(
      ns_context, alias_text, target_ts);
  parser.register_typedef_binding(parser_test_text_id(parser, "corrupted"), bridge_ts, true);

  (void)parse_top_level(parser);

  const c4c::TypeSpec* visible_alias =
      parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias"));
  expect_true(visible_alias != nullptr && visible_alias->base == c4c::TB_INT,
              "using type imports should prefer the structured target type before rendered fallback names");
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
  parser.register_typedef_binding(parser_test_text_id(parser, "Alias"), alias_ts, true);
  parser.register_var_type_binding(parser_test_text_id(parser, "value"), value_ts);

  expect_true(parser.find_typedef_type(parser_test_text_id(parser, "Alias")) == nullptr,
              "local typedef registration should keep the flat typedef table empty");
  expect_true(parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias")) != nullptr &&
                  parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias"))->base == c4c::TB_INT,
              "local typedef registration should resolve through lexical scope storage");
  expect_true(parser.find_var_type(parser_test_text_id(parser, "value")) == nullptr,
              "local value registration should keep the flat value table empty");
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) != nullptr &&
                  parser.find_visible_var_type(parser_test_text_id(parser, "value"))->base == c4c::TB_DOUBLE,
              "local value registration should resolve through lexical scope storage");

  expect_true(parser.pop_local_binding_scope(),
              "test fixture should balance the local binding scope");
  expect_true(parser.find_visible_typedef_type(parser_test_text_id(parser, "Alias")) == nullptr,
              "popping the local scope should remove lexical typedef visibility");
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) == nullptr,
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
  expect_true(parser.find_var_type(parser_test_text_id(parser, "value")) == nullptr,
              "if-condition declarations should not leak through the flat var table");
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) == nullptr,
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
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) == nullptr,
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
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) == nullptr,
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
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) == nullptr,
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
  expect_true(parser.find_visible_var_type(parser_test_text_id(parser, "value")) == nullptr,
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

  const c4c::TypeSpec* alias_ts = parser.find_typedef_type(parser_test_text_id(parser, "Alias"));
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

void test_parser_local_using_alias_qualified_static_call_keeps_spelled_value() {
  c4c::Lexer lexer("namespace api {\n"
                   "struct widget {\n"
                   "  static int make() { return 1; }\n"
                   "};\n"
                   "}\n"
                   "int main() {\n"
                   "  using Alias = api::widget;\n"
                   "  Alias::make();\n"
                   "}\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::Node* program = parser.parse();
  expect_true(program != nullptr && program->kind == c4c::NK_PROGRAM,
              "local using-alias static call regression should parse as a program");
  expect_eq_int(program->n_children, 2,
                "the static call regression should contain the namespace item and main");

  c4c::Node* main_fn = program->children[1];
  expect_true(main_fn != nullptr && main_fn->kind == c4c::NK_FUNCTION,
              "the static call regression should include a parsed main function");
  expect_true(main_fn->body != nullptr && main_fn->body->kind == c4c::NK_BLOCK,
              "main should parse with a block body");
  expect_eq_int(main_fn->body->n_children, 2,
                "main should retain the using alias and static call statement");
  c4c::Node* stmt = main_fn->body->children[1];
  expect_true(stmt != nullptr && stmt->kind == c4c::NK_EXPR_STMT,
              "qualified static calls through a local using alias should stay expression statements");
  expect_true(stmt->left != nullptr && stmt->left->kind == c4c::NK_CALL,
              "qualified static calls through a local using alias should remain calls");
  expect_true(stmt->left->left != nullptr &&
                  stmt->left->left->kind == c4c::NK_VAR,
              "qualified static calls through a local using alias should keep a variable callee");
  expect_eq(stmt->left->left->name, "Alias::make",
            "qualified static calls through a local using alias should keep the spelled callee");
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
  expect_true(parser.find_template_struct_primary(
                  parser.current_namespace_context_id(),
                  lexer.text_table().intern("Trait")) != nullptr,
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

  const c4c::TextId param_text = texts.intern("T");
  parser.push_template_scope(
      c4c::Parser::TemplateScopeKind::FreeFunctionTemplate,
      {{.name_text_id = param_text, .name = "T", .is_nttp = false}});
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

  expect_true(parser.is_template_scope_type_param(param_text),
              "template-scope lookup should match the semantic TextId");
  expect_true(!parser.is_template_scope_type_param(other_text),
              "template-scope lookup should not recover identity from spelling");
  expect_true(!parser.is_template_scope_type_param(c4c::kInvalidText),
              "template-scope lookup should reject missing structured metadata");
  parser.pop_template_scope();
}

void test_parser_template_specialization_binding_prefers_param_text_id() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId trait_text = texts.intern("Trait");
  const c4c::TextId param_text = texts.intern("T");

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Trait");
  primary->unqualified_name = arena.strdup("Trait");
  primary->unqualified_text_id = trait_text;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("PrimaryT");
  primary->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  primary->template_param_name_text_ids[0] = param_text;
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;

  c4c::Node* specialization = parser.make_node(c4c::NK_STRUCT_DEF, 2);
  specialization->name = arena.strdup("Trait_T_int");
  specialization->unqualified_name = arena.strdup("Trait");
  specialization->unqualified_text_id = trait_text;
  specialization->namespace_context_id = parser.current_namespace_context_id();
  specialization->template_origin_name = arena.strdup("Trait");
  specialization->n_template_params = 1;
  specialization->template_param_names = arena.alloc_array<const char*>(1);
  specialization->template_param_names[0] = arena.strdup("StaleRenderedName");
  specialization->template_param_name_text_ids =
      arena.alloc_array<c4c::TextId>(1);
  specialization->template_param_name_text_ids[0] = param_text;
  specialization->template_param_is_nttp = arena.alloc_array<bool>(1);
  specialization->template_param_is_nttp[0] = false;
  specialization->template_param_is_pack = arena.alloc_array<bool>(1);
  specialization->template_param_is_pack[0] = false;
  specialization->template_param_has_default = arena.alloc_array<bool>(1);
  specialization->template_param_has_default[0] = false;
  specialization->n_template_args = 1;
  specialization->template_arg_types = arena.alloc_array<c4c::TypeSpec>(1);
  specialization->template_arg_types[0].array_size = -1;
  specialization->template_arg_types[0].inner_rank = -1;
  specialization->template_arg_types[0].base = c4c::TB_TYPEDEF;
  specialization->template_arg_types[0].tag = arena.strdup("T");
  specialization->template_arg_is_value = arena.alloc_array<bool>(1);
  specialization->template_arg_is_value[0] = false;

  c4c::Parser::TemplateArgParseResult actual{};
  actual.is_value = false;
  actual.type.array_size = -1;
  actual.type.inner_rank = -1;
  actual.type.base = c4c::TB_INT;
  std::vector<c4c::Parser::TemplateArgParseResult> actual_args = {actual};
  std::vector<c4c::Node*> specializations = {specialization};
  std::vector<std::pair<std::string, c4c::TypeSpec>> type_bindings;
  std::vector<std::pair<std::string, long long>> nttp_bindings;

  const c4c::Node* selected = c4c::select_template_struct_pattern(
      actual_args, primary, &specializations, parser, &type_bindings,
      &nttp_bindings);

  expect_true(selected == specialization,
              "template specialization matching should bind by parameter TextId even when rendered parameter spelling is stale");
  expect_eq_int(static_cast<int>(type_bindings.size()), 1,
                "TextId-based specialization matching should emit the type binding");
  expect_eq(type_bindings[0].first, "StaleRenderedName",
            "specialization binding output should preserve compatibility spelling");
  expect_true(type_bindings[0].second.base == c4c::TB_INT,
              "specialization binding should capture the concrete argument type");
  expect_true(nttp_bindings.empty(),
              "type-only specialization matching should not emit NTTP bindings");
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
  const c4c::QualifiedNameKey target_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "Target"));
  parser.register_known_fn_name(target_key);
  parser.register_using_value_alias_for_testing(0, alias_text, target_key,
                                                "corrupted");

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
  parser.register_synthesized_typedef_binding(synth_text_id);
  expect_true(parser.is_typedef_name(synth_text_id),
              "synthesized typedef registration should prefer the semantic TextId");
  const c4c::TypeSpec* synthesized_type =
      parser.find_typedef_type(parser_test_text_id(parser, "SynthParam"));
  expect_true(synthesized_type != nullptr,
              "synthesized typedef registration should store the TextId binding");
  expect_true(synthesized_type->base == c4c::TB_TYPEDEF,
              "synthesized typedef registration should store a typedef type");
  expect_eq(synthesized_type->tag ? synthesized_type->tag : "", "SynthParam",
            "synthesized typedef registration should store the TextId spelling");
  expect_true(!parser.has_typedef_name(parser_test_text_id(parser, "corrupted")),
              "synthesized typedef registration should not store the fallback spelling");

  parser.unregister_typedef_binding(synth_text_id);
  expect_true(!parser.is_typedef_name(synth_text_id),
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
  parser.register_typedef_binding(parser_test_text_id(parser, "Alias"), global_alias_ts, true);

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
  expect_true(parser.find_template_struct_primary(
                  parser.current_namespace_context_id(),
                  lexer.text_table().intern("Trait")) != nullptr,
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

void test_parser_template_lookup_uses_structured_template_keys() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId trait_text = texts.intern("Trait");
  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Trait");
  primary->unqualified_name = arena.strdup("Trait");
  primary->unqualified_text_id = trait_text;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;

  expect_true(parser.find_template_struct_primary(
                  parser.current_namespace_context_id(), trait_text) == nullptr,
              "template primary lookup should require the QualifiedNameKey table");
  expect_true(parser.find_template_struct_primary(
                  parser.current_namespace_context_id(),
                  c4c::kInvalidText) == nullptr,
              "template primary lookup should not have a TextId-less rendered-name fallback");

  c4c::Node* specialization = parser.make_node(c4c::NK_STRUCT_DEF, 2);
  specialization->name = arena.strdup("Trait_T_int");
  specialization->unqualified_name = arena.strdup("Trait");
  specialization->unqualified_text_id = trait_text;
  specialization->namespace_context_id = parser.current_namespace_context_id();
  expect_true(parser.find_template_struct_specializations(primary) == nullptr,
              "template specialization lookup should require structured registration");
  expect_true(parser.find_template_struct_specializations(
                  parser.current_namespace_context_id(), trait_text) == nullptr,
              "direct template specialization lookup should require structured registration");
  expect_true(parser.find_template_struct_specializations(
                  parser.current_namespace_context_id(),
                  c4c::kInvalidText) == nullptr,
              "template specialization lookup should not have a TextId-less rendered-name fallback");

  const c4c::QualifiedNameKey trait_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), trait_text);
  parser.template_state_.template_struct_defs_by_key[trait_key] = primary;
  parser.template_state_.template_struct_specializations_by_key[trait_key] = {
      specialization};
  expect_true(parser.find_template_struct_primary(
                  parser.current_namespace_context_id(), trait_text) == primary,
              "structured template primary lookup should use QualifiedNameKey state");
  const std::vector<c4c::Node*>* structured_specializations =
      parser.find_template_struct_specializations(
          parser.current_namespace_context_id(), trait_text);
  expect_true(structured_specializations != nullptr &&
                  structured_specializations->size() == 1 &&
                  (*structured_specializations)[0] == specialization,
              "structured template specialization lookup should use QualifiedNameKey state");
}

void test_parser_nttp_default_cache_uses_structured_key_only() {
  c4c::Lexer lexer("template<int N = M + 1>\nstruct ParsedTrait {};\n",
                   c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena parsed_arena;
  c4c::Parser parsed_parser(tokens, parsed_arena, &lexer.text_table(),
                            &lexer.file_table(),
                            c4c::SourceProfile::CppSubset);
  (void)parse_top_level(parsed_parser);
  const c4c::QualifiedNameKey parsed_trait_key =
      parsed_parser.alias_template_key_in_context(
          parsed_parser.current_namespace_context_id(),
          parsed_parser.find_parser_text_id("ParsedTrait"));
  const c4c::ParserTemplateState::NttpDefaultExprKey parsed_cache_key{
      parsed_trait_key, 0};
  expect_true(parsed_parser.template_state_.nttp_default_expr_tokens_by_key.find(
                  parsed_cache_key) !=
                  parsed_parser.template_state_.nttp_default_expr_tokens_by_key.end(),
              "parsed NTTP defaults should populate the structured cache");
  expect_true(parsed_parser.template_state_.nttp_default_expr_tokens.find(
                  "ParsedTrait:0") ==
                  parsed_parser.template_state_.nttp_default_expr_tokens.end(),
              "parsed NTTP defaults should not populate a rendered-name mirror");

  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::TextId trait_text = texts.intern("Trait");
  const c4c::QualifiedNameKey trait_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), trait_text);
  std::vector<c4c::Token> structured_tokens = {
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "2"),
  };
  parser.cache_nttp_default_expr_tokens(trait_key, 0,
                                        structured_tokens);

  const c4c::ParserTemplateState::NttpDefaultExprKey cache_key{trait_key, 0};
  expect_true(parser.template_state_.nttp_default_expr_tokens_by_key.find(
                  cache_key) !=
                  parser.template_state_.nttp_default_expr_tokens_by_key.end(),
              "NTTP default cache should populate the structured key");
  expect_true(parser.template_state_.nttp_default_expr_tokens.find("Trait:0") ==
                  parser.template_state_.nttp_default_expr_tokens.end(),
              "NTTP default cache should not populate a rendered-name mirror");

  parser.template_state_.nttp_default_expr_tokens["Trait:0"] = {
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "3"),
  };
  long long value = 0;
  expect_true(parser.eval_deferred_nttp_default(trait_key, 0, {}, {}, &value),
              "keyed NTTP default cache lookup should use the structured result");
  expect_eq_int(value, 2,
                "mismatched NTTP default cache entries should not let the rendered-name mirror override structured data");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.nttp_default_expr_cache_mismatch_count),
      0,
      "rendered-name NTTP default mirrors should not be evaluated as semantic cache telemetry");

  const c4c::TextId ns_text = texts.intern("ns");
  const int other_context =
      parser.ensure_named_namespace_context(0, ns_text, "ns");
  const c4c::QualifiedNameKey other_trait_key =
      parser.alias_template_key_in_context(other_context, trait_text);
  parser.cache_nttp_default_expr_tokens(other_trait_key, 0, {
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "7"),
  });
  parser.template_state_.nttp_default_expr_tokens["Trait:0"] = {
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "11"),
  };
  expect_true(parser.eval_deferred_nttp_default(other_trait_key, 0, {}, {},
                                                &value),
              "keyed NTTP default lookup should not reconstruct identity from the rendered name");
  expect_eq_int(value, 7,
                "ambiguous rendered-name NTTP defaults should follow the supplied primary template key");

  parser.template_state_.nttp_default_expr_tokens_by_key.erase(cache_key);
  expect_true(!parser.eval_deferred_nttp_default(trait_key, 0, {}, {}, &value),
              "valid-TextId NTTP default cache lookup should not promote a legacy-only rendered mirror");

  parser.template_state_.nttp_default_expr_tokens["LegacyTrait:0"] = {
      parser.make_injected_token(seed, c4c::TokenKind::IntLit, "5"),
  };
  const c4c::TextId legacy_trait_text =
      parser.find_parser_text_id("LegacyTrait");
  expect_true(legacy_trait_text == c4c::kInvalidText,
              "rendered-only NTTP default cache entries should not create template identity metadata");
  const c4c::ParserTemplateState::NttpDefaultExprKey legacy_cache_key{
      c4c::QualifiedNameKey{}, 0};
  expect_true(parser.template_state_.nttp_default_expr_tokens_by_key.find(
                  legacy_cache_key) ==
                  parser.template_state_.nttp_default_expr_tokens_by_key.end(),
              "TextId-less NTTP default cache mirrors should remain outside structured lookup");
}

void test_parser_template_instantiation_dedup_keys_structure_specialization_reuse() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::TextId trait_text = texts.intern("Trait");
  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Trait");
  primary->unqualified_name = arena.strdup("Trait");
  primary->unqualified_text_id = trait_text;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), trait_text, primary);

  c4c::Node* specialization = parser.make_node(c4c::NK_STRUCT_DEF, 2);
  specialization->name = arena.strdup("Trait_T_int");
  specialization->unqualified_name = arena.strdup("Trait");
  specialization->unqualified_text_id = trait_text;
  specialization->namespace_context_id = parser.current_namespace_context_id();
  specialization->template_origin_name = arena.strdup("Trait");
  specialization->n_template_args = 1;
  specialization->template_arg_types = arena.alloc_array<c4c::TypeSpec>(1);
  specialization->template_arg_types[0].array_size = -1;
  specialization->template_arg_types[0].inner_rank = -1;
  specialization->template_arg_types[0].base = c4c::TB_INT;
  specialization->template_arg_is_value = arena.alloc_array<bool>(1);
  specialization->template_arg_is_value[0] = false;
  c4c::Node* value_field = parser.make_node(c4c::NK_DECL, 2);
  value_field->name = arena.strdup("value");
  value_field->is_static = true;
  value_field->ival = 19;
  specialization->n_fields = 1;
  specialization->fields = arena.alloc_array<c4c::Node*>(1);
  specialization->fields[0] = value_field;
  parser.register_template_struct_specialization(
      parser.current_namespace_context_id(), trait_text, specialization);

  primary = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), trait_text);
  expect_true(primary != nullptr,
              "template specialization fixture should register the primary");
  expect_true(parser.find_template_struct_specializations(primary) != nullptr,
              "template specialization fixture should register the specialization");

  const std::vector<c4c::Token> toks = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };

  long long value = 0;
  expect_true(parser.eval_deferred_nttp_expr_tokens("Trait", toks, {}, {}, &value),
              "template instantiation reuse should resolve explicit specialization members");
  expect_eq_int(value, 19,
                "explicit specialization reuse should preserve the specialized static member value");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.instantiated_template_struct_keys_by_key.size()),
      1,
      "template instantiation should populate the structured de-dup key");

  parser.template_state_.instantiated_template_struct_keys_by_key.clear();
  value = 0;
  expect_true(parser.eval_deferred_nttp_expr_tokens("Trait", toks, {}, {}, &value),
              "template instantiation reuse should rebuild structured de-dup state from typed metadata");
  expect_eq_int(value, 19,
                "structured template instantiation reuse should preserve the specialized static member value");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.instantiated_template_struct_keys_by_key.size()),
      1,
      "template instantiation lookup should recreate the structured key");

  c4c::Parser::TemplateArgParseResult arg{};
  arg.is_value = false;
  arg.type.array_size = -1;
  arg.type.inner_rank = -1;
  arg.type.base = c4c::TB_INT;
  std::string resolved_mangled;
  c4c::TypeSpec resolved_specialization{};
  expect_true(parser.ensure_template_struct_instantiated_from_args(
                  "Trait", primary, {arg}, 1, &resolved_mangled,
                  "template_specialization_record_def_test",
                  &resolved_specialization),
              "explicit specialization reuse should produce a resolved TypeSpec");
  expect_true(resolved_specialization.record_def == specialization,
              "explicit specialization reuse should carry the selected record_def");

  c4c::Node* stale_instantiation = parser.make_node(c4c::NK_STRUCT_DEF, 3);
  stale_instantiation->name = arena.strdup("StaleTrait");
  stale_instantiation->n_member_typedefs = 0;
  parser.definition_state_.struct_tag_def_map[resolved_mangled] =
      stale_instantiation;
  resolved_specialization = {};
  expect_true(parser.ensure_template_struct_instantiated_from_args(
                  "Trait", primary, {arg}, 1, &resolved_mangled,
                  "template_specialization_stale_record_def_test",
                  &resolved_specialization),
              "explicit specialization reuse should ignore stale rendered map payloads");
  expect_true(resolved_specialization.record_def == specialization,
              "stale rendered template tag-map entries should not override record_def");
}

void test_parser_template_static_member_lookup_prefers_record_definition() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  auto make_static_value_record = [&](const char* record_name,
                                      const char* member_name,
                                      long long value) {
    c4c::Node* record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
    record->name = arena.strdup(record_name);
    record->n_fields = 1;
    record->fields = arena.alloc_array<c4c::Node*>(1);
    c4c::Node* member = parser.make_node(c4c::NK_DECL, 1);
    member->name = arena.strdup(member_name);
    member->is_static = true;
    member->ival = value;
    record->fields[0] = member;
    return record;
  };

  c4c::Node* real_base =
      make_static_value_record("RealBase", "value", 17);
  c4c::Node* stale_base =
      make_static_value_record("StaleBase", "value", 33);
  parser.definition_state_.struct_tag_def_map["SharedBase"] = stale_base;

  const c4c::TextId trait_text = texts.intern("Trait");
  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Trait");
  primary->unqualified_name = arena.strdup("Trait");
  primary->unqualified_text_id = trait_text;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), trait_text, primary);

  c4c::Node* specialization = parser.make_node(c4c::NK_STRUCT_DEF, 2);
  specialization->name = arena.strdup("Trait_T_int");
  specialization->unqualified_name = arena.strdup("Trait");
  specialization->unqualified_text_id = trait_text;
  specialization->namespace_context_id = parser.current_namespace_context_id();
  specialization->template_origin_name = arena.strdup("Trait");
  specialization->n_template_args = 1;
  specialization->template_arg_types = arena.alloc_array<c4c::TypeSpec>(1);
  specialization->template_arg_types[0].array_size = -1;
  specialization->template_arg_types[0].inner_rank = -1;
  specialization->template_arg_types[0].base = c4c::TB_INT;
  specialization->template_arg_is_value = arena.alloc_array<bool>(1);
  specialization->template_arg_is_value[0] = false;
  specialization->n_bases = 1;
  specialization->base_types = arena.alloc_array<c4c::TypeSpec>(1);
  specialization->base_types[0].array_size = -1;
  specialization->base_types[0].inner_rank = -1;
  specialization->base_types[0].base = c4c::TB_STRUCT;
  specialization->base_types[0].tag = arena.strdup("SharedBase");
  specialization->base_types[0].record_def = real_base;
  parser.register_template_struct_specialization(
      parser.current_namespace_context_id(), trait_text, specialization);

  c4c::Parser::TemplateArgParseResult arg{};
  arg.is_value = false;
  arg.type.array_size = -1;
  arg.type.inner_rank = -1;
  arg.type.base = c4c::TB_INT;
  std::string resolved_mangled;
  c4c::TypeSpec resolved_specialization{};
  expect_true(parser.ensure_template_struct_instantiated_from_args(
                  "Trait", primary, {arg}, 1, &resolved_mangled,
                  "template_static_member_record_def_test",
                  &resolved_specialization),
              "template static member fixture should resolve the specialization");
  expect_true(resolved_specialization.record_def == specialization,
              "template static member instantiation should carry record_def");

  c4c::Node* stale_instantiation =
      make_static_value_record("StaleTrait", "value", 99);
  parser.definition_state_.struct_tag_def_map[resolved_mangled] =
      stale_instantiation;

  const std::vector<c4c::Token> toks = {
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Trait"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  };

  long long value = 0;
  expect_true(parser.eval_deferred_nttp_expr_tokens("Trait", toks, {}, {},
                                                    &value),
              "template static member lookup should traverse typed records");
  expect_eq_int(value, 17,
                "template static member lookup should prefer record_def over stale rendered tag maps");
}

void test_sema_static_member_type_lookup_prefers_structured_member_key() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base, int ptr_level = 0) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    ts.ptr_level = ptr_level;
    return ts;
  };

  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId actual_text = texts.intern("actual");
  const c4c::TextId stale_text = texts.intern("stale");

  c4c::Node* owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  owner->name = arena.strdup("Owner");
  owner->unqualified_name = arena.strdup("Owner");
  owner->unqualified_text_id = owner_text;
  owner->namespace_context_id = parser.current_namespace_context_id();
  owner->n_fields = 2;
  owner->fields = arena.alloc_array<c4c::Node*>(2);

  c4c::Node* actual = parser.make_node(c4c::NK_DECL, 1);
  actual->name = arena.strdup("actual");
  actual->unqualified_name = arena.strdup("actual");
  actual->unqualified_text_id = actual_text;
  actual->namespace_context_id = owner->namespace_context_id;
  actual->is_static = true;
  actual->type = make_ts(c4c::TB_INT, 1);
  owner->fields[0] = actual;

  c4c::Node* stale = parser.make_node(c4c::NK_DECL, 1);
  stale->name = arena.strdup("stale");
  stale->unqualified_name = arena.strdup("stale");
  stale->unqualified_text_id = stale_text;
  stale->namespace_context_id = owner->namespace_context_id;
  stale->is_static = true;
  stale->type = make_ts(c4c::TB_FLOAT);
  owner->fields[1] = stale;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("Owner::stale");
  ref->unqualified_name = arena.strdup("actual");
  ref->unqualified_text_id = actual_text;
  ref->namespace_context_id = owner->namespace_context_id;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("Owner");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = owner_text;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("returns_actual");
  fn->unqualified_name = arena.strdup("returns_actual");
  fn->unqualified_text_id = texts.intern("returns_actual");
  fn->namespace_context_id = owner->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT, 1);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = owner;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(result.ok,
              "Sema static member lookup should use structured member TextId over rendered member spelling");
}

void test_sema_unqualified_symbol_lookup_prefers_structured_key_over_rendered_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    return ts;
  };

  const c4c::TextId actual_text = texts.intern("actual");

  c4c::Node* actual = parser.make_node(c4c::NK_DECL, 1);
  actual->name = arena.strdup("actual");
  actual->unqualified_name = arena.strdup("actual");
  actual->unqualified_text_id = actual_text;
  actual->namespace_context_id = parser.current_namespace_context_id();
  actual->type = make_ts(c4c::TB_INT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("wrong::stale");
  ref->unqualified_name = arena.strdup("stale");
  ref->unqualified_text_id = actual_text;
  ref->namespace_context_id = actual->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("returns_actual");
  fn->unqualified_name = arena.strdup("returns_actual");
  fn->unqualified_text_id = texts.intern("returns_actual");
  fn->namespace_context_id = actual->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->n_params = 1;
  fn->params = arena.alloc_array<c4c::Node*>(1);
  fn->params[0] = actual;
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 1;
  program->children = arena.alloc_array<c4c::Node*>(1);
  program->children[0] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  const std::string diag =
      result.diagnostics.empty() ? "" : (": " + result.diagnostics.front().message);
  expect_true(result.ok,
              "unqualified symbol lookup should use structured TextId before rendered names" + diag);
}

void test_sema_unqualified_symbol_lookup_rejects_stale_rendered_local_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    return ts;
  };

  c4c::Node* init = parser.make_node(c4c::NK_INT_LIT, 2);
  init->ival = 1;

  c4c::Node* stale_local = parser.make_node(c4c::NK_DECL, 2);
  stale_local->name = arena.strdup("stale");
  stale_local->unqualified_name = arena.strdup("actual");
  stale_local->unqualified_text_id = texts.intern("actual");
  stale_local->namespace_context_id = parser.current_namespace_context_id();
  stale_local->type = make_ts(c4c::TB_INT);
  stale_local->init = init;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 3);
  ref->name = arena.strdup("stale");
  ref->unqualified_name = arena.strdup("stale");
  ref->unqualified_text_id = texts.intern("missing_local");
  ref->namespace_context_id = stale_local->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 3);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 2;
  body->children = arena.alloc_array<c4c::Node*>(2);
  body->children[0] = stale_local;
  body->children[1] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->name = arena.strdup("rejects_stale_local");
  fn->unqualified_name = arena.strdup("rejects_stale_local");
  fn->unqualified_text_id = texts.intern("rejects_stale_local");
  fn->namespace_context_id = stale_local->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 1;
  program->children = arena.alloc_array<c4c::Node*>(1);
  program->children[0] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "unqualified symbol lookup should reject stale rendered local names after a structured local miss");
}

void test_sema_unqualified_symbol_lookup_rejects_stale_rendered_global_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    return ts;
  };

  c4c::Node* stale_global = parser.make_node(c4c::NK_DECL, 1);
  stale_global->name = arena.strdup("stale_global");
  stale_global->unqualified_name = arena.strdup("actual_global");
  stale_global->unqualified_text_id = texts.intern("actual_global");
  stale_global->namespace_context_id = parser.current_namespace_context_id();
  stale_global->type = make_ts(c4c::TB_INT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 3);
  ref->name = arena.strdup("stale_global");
  ref->unqualified_name = arena.strdup("stale_global");
  ref->unqualified_text_id = texts.intern("missing_global");
  ref->namespace_context_id = stale_global->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 3);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_stale_global");
  fn->unqualified_name = arena.strdup("rejects_stale_global");
  fn->unqualified_text_id = texts.intern("rejects_stale_global");
  fn->namespace_context_id = stale_global->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = stale_global;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "unqualified symbol lookup should reject stale rendered global names after a structured global miss");
}

c4c::TypeSpec make_sema_lookup_ts(c4c::TypeBase base, int ptr_level = 0) {
  c4c::TypeSpec ts{};
  ts.array_size = -1;
  ts.inner_rank = -1;
  ts.base = base;
  ts.ptr_level = ptr_level;
  return ts;
}

c4c::Node* make_sema_lookup_function(c4c::Parser& parser, c4c::Arena& arena,
                                     const char* rendered_name,
                                     const char* structured_name,
                                     c4c::TextId structured_text,
                                     c4c::TypeSpec ret,
                                     c4c::TypeSpec param) {
  c4c::Node* arg = parser.make_node(c4c::NK_DECL, 1);
  arg->name = arena.strdup("arg");
  arg->unqualified_name = arena.strdup("arg");
  arg->unqualified_text_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, "arg");
  arg->namespace_context_id = parser.current_namespace_context_id();
  arg->type = param;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->name = arena.strdup(rendered_name);
  fn->unqualified_name = arena.strdup(structured_name);
  fn->unqualified_text_id = structured_text;
  fn->namespace_context_id = parser.current_namespace_context_id();
  fn->type = ret;
  fn->n_params = 1;
  fn->params = arena.alloc_array<c4c::Node*>(1);
  fn->params[0] = arg;
  return fn;
}

c4c::Node* make_sema_lookup_calling_function(c4c::Parser& parser,
                                             c4c::Arena& arena,
                                             const char* caller_name,
                                             const char* rendered_callee,
                                             const char* structured_callee,
                                             c4c::TextId structured_callee_text) {
  c4c::Node* literal = parser.make_node(c4c::NK_INT_LIT, 1);
  literal->ival = 1;

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 1);
  callee->name = arena.strdup(rendered_callee);
  callee->unqualified_name = arena.strdup(structured_callee);
  callee->unqualified_text_id = structured_callee_text;
  callee->namespace_context_id = parser.current_namespace_context_id();

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 1);
  call->left = callee;
  call->n_children = 1;
  call->children = arena.alloc_array<c4c::Node*>(1);
  call->children[0] = literal;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 1);
  ret->left = call;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 1);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->name = arena.strdup(caller_name);
  fn->unqualified_name = arena.strdup(caller_name);
  fn->unqualified_text_id =
      parser.parser_text_id_for_token(c4c::kInvalidText, caller_name);
  fn->namespace_context_id = parser.current_namespace_context_id();
  fn->type = make_sema_lookup_ts(c4c::TB_INT, 1);
  fn->body = body;
  return fn;
}

void test_sema_overload_lookup_prefers_structured_key_over_rendered_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId actual_ref_text = texts.intern("actual_ref_overload");
  const c4c::TextId actual_cpp_text = texts.intern("operator_actual_lookup");
  c4c::TypeSpec int_value = make_sema_lookup_ts(c4c::TB_INT);
  c4c::TypeSpec double_value = make_sema_lookup_ts(c4c::TB_DOUBLE);
  c4c::TypeSpec int_ptr = make_sema_lookup_ts(c4c::TB_INT, 1);
  c4c::TypeSpec stale_ret = make_sema_lookup_ts(c4c::TB_DOUBLE);
  c4c::TypeSpec lvalue_int_ref = int_value;
  lvalue_int_ref.is_lvalue_ref = true;
  c4c::TypeSpec rvalue_int_ref = int_value;
  rvalue_int_ref.is_rvalue_ref = true;

  c4c::Node* stale_ref_lvalue = make_sema_lookup_function(
      parser, arena, "stale_ref_overload", "stale_ref_overload",
      texts.intern("stale_ref_overload"), stale_ret, lvalue_int_ref);
  c4c::Node* stale_ref_rvalue = make_sema_lookup_function(
      parser, arena, "stale_ref_overload", "stale_ref_overload",
      texts.intern("stale_ref_overload"), stale_ret, rvalue_int_ref);
  c4c::Node* actual_ref_lvalue = make_sema_lookup_function(
      parser, arena, "actual_ref_overload", "actual_ref_overload",
      actual_ref_text, int_ptr, lvalue_int_ref);
  c4c::Node* actual_ref_rvalue = make_sema_lookup_function(
      parser, arena, "actual_ref_overload", "actual_ref_overload",
      actual_ref_text, int_ptr, rvalue_int_ref);

  c4c::Node* stale_cpp_int = make_sema_lookup_function(
      parser, arena, "operator_stale_lookup", "operator_stale_lookup",
      texts.intern("operator_stale_lookup"), stale_ret, int_value);
  c4c::Node* stale_cpp_double = make_sema_lookup_function(
      parser, arena, "operator_stale_lookup", "operator_stale_lookup",
      texts.intern("operator_stale_lookup"), stale_ret, double_value);
  c4c::Node* actual_cpp_int = make_sema_lookup_function(
      parser, arena, "operator_actual_lookup", "operator_actual_lookup",
      actual_cpp_text, int_ptr, int_value);
  c4c::Node* actual_cpp_double = make_sema_lookup_function(
      parser, arena, "operator_actual_lookup", "operator_actual_lookup",
      actual_cpp_text, int_ptr, double_value);

  c4c::Node* ref_caller = make_sema_lookup_calling_function(
      parser, arena, "call_structured_ref_overload", "stale_ref_overload",
      "actual_ref_overload", actual_ref_text);
  c4c::Node* cpp_caller = make_sema_lookup_calling_function(
      parser, arena, "call_structured_cpp_overload", "operator_stale_lookup",
      "operator_actual_lookup", actual_cpp_text);

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 10;
  program->children = arena.alloc_array<c4c::Node*>(10);
  program->children[0] = stale_ref_lvalue;
  program->children[1] = stale_ref_rvalue;
  program->children[2] = actual_ref_lvalue;
  program->children[3] = actual_ref_rvalue;
  program->children[4] = stale_cpp_int;
  program->children[5] = stale_cpp_double;
  program->children[6] = actual_cpp_int;
  program->children[7] = actual_cpp_double;
  program->children[8] = ref_caller;
  program->children[9] = cpp_caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  const std::string diag =
      result.diagnostics.empty() ? "" : (": " + result.diagnostics.front().message);
  expect_true(result.ok,
              "overload lookup should use structured function keys before rendered names" + diag);
}

void test_sema_overload_lookup_ignores_stale_rendered_name_after_structured_miss() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId missing_ref_text = texts.intern("missing_ref_overload");
  const c4c::TextId missing_cpp_text = texts.intern("operator_missing_lookup");
  c4c::TypeSpec int_value = make_sema_lookup_ts(c4c::TB_INT);
  c4c::TypeSpec double_value = make_sema_lookup_ts(c4c::TB_DOUBLE);
  c4c::TypeSpec stale_ret = make_sema_lookup_ts(c4c::TB_DOUBLE);
  c4c::TypeSpec lvalue_int_ref = int_value;
  lvalue_int_ref.is_lvalue_ref = true;
  c4c::TypeSpec rvalue_int_ref = int_value;
  rvalue_int_ref.is_rvalue_ref = true;

  c4c::Node* stale_ref_lvalue = make_sema_lookup_function(
      parser, arena, "stale_ref_overload", "stale_ref_overload",
      texts.intern("stale_ref_overload"), stale_ret, lvalue_int_ref);
  c4c::Node* stale_ref_rvalue = make_sema_lookup_function(
      parser, arena, "stale_ref_overload", "stale_ref_overload",
      texts.intern("stale_ref_overload"), stale_ret, rvalue_int_ref);
  c4c::Node* stale_cpp_int = make_sema_lookup_function(
      parser, arena, "operator_stale_lookup", "operator_stale_lookup",
      texts.intern("operator_stale_lookup"), stale_ret, int_value);
  c4c::Node* stale_cpp_double = make_sema_lookup_function(
      parser, arena, "operator_stale_lookup", "operator_stale_lookup",
      texts.intern("operator_stale_lookup"), stale_ret, double_value);

  c4c::Node* ref_caller = make_sema_lookup_calling_function(
      parser, arena, "call_missing_ref_overload", "stale_ref_overload",
      "missing_ref_overload", missing_ref_text);
  c4c::Node* cpp_caller = make_sema_lookup_calling_function(
      parser, arena, "call_missing_cpp_overload", "operator_stale_lookup",
      "operator_missing_lookup", missing_cpp_text);

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 6;
  program->children = arena.alloc_array<c4c::Node*>(6);
  program->children[0] = stale_ref_lvalue;
  program->children[1] = stale_ref_rvalue;
  program->children[2] = stale_cpp_int;
  program->children[3] = stale_cpp_double;
  program->children[4] = ref_caller;
  program->children[5] = cpp_caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  const std::string diag =
      result.diagnostics.empty() ? "" : (": " + result.diagnostics.front().message);
  expect_true(result.ok,
              "overload lookup should ignore stale rendered names after structured metadata misses" + diag);
}

void test_sema_overload_lookup_keeps_no_metadata_rendered_compatibility() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::TypeSpec int_value = make_sema_lookup_ts(c4c::TB_INT);
  c4c::TypeSpec double_value = make_sema_lookup_ts(c4c::TB_DOUBLE);
  c4c::TypeSpec stale_ret = make_sema_lookup_ts(c4c::TB_DOUBLE);

  c4c::Node* stale_cpp_int = make_sema_lookup_function(
      parser, arena, "operator_compat_lookup", "operator_compat_lookup",
      texts.intern("operator_compat_lookup"), stale_ret, int_value);
  c4c::Node* stale_cpp_double = make_sema_lookup_function(
      parser, arena, "operator_compat_lookup", "operator_compat_lookup",
      texts.intern("operator_compat_lookup"), stale_ret, double_value);

  c4c::Node* caller = make_sema_lookup_calling_function(
      parser, arena, "call_compat_cpp_overload", "operator_compat_lookup",
      "operator_compat_lookup", c4c::kInvalidText);
  caller->body->children[0]->left->left->unqualified_text_id = c4c::kInvalidText;
  caller->body->children[0]->left->left->namespace_context_id = -1;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = stale_cpp_int;
  program->children[1] = stale_cpp_double;
  program->children[2] = caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok && !result.diagnostics.empty() &&
                  result.diagnostics.front().message == "incompatible return type",
              "overload lookup should still consult rendered compatibility when references have no structured carrier");
}

void test_sema_namespace_owner_resolution_prefers_structured_owner_key() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base, int ptr_level = 0, const char* tag = nullptr) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    ts.ptr_level = ptr_level;
    ts.tag = tag;
    return ts;
  };

  const c4c::TextId real_ns_text = texts.intern("real");
  const c4c::TextId wrong_ns_text = texts.intern("wrong");
  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId self_text = texts.intern("self");
  const int real_ns = parser.ensure_named_namespace_context(0, real_ns_text, "real");
  const int wrong_ns = parser.ensure_named_namespace_context(0, wrong_ns_text, "wrong");

  c4c::Node* wrong_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  wrong_owner->name = arena.strdup("Owner");
  wrong_owner->unqualified_name = arena.strdup("Owner");
  wrong_owner->unqualified_text_id = owner_text;
  wrong_owner->namespace_context_id = wrong_ns;

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("Owner");
  real_owner->unqualified_name = arena.strdup("Owner");
  real_owner->unqualified_text_id = owner_text;
  real_owner->namespace_context_id = real_ns;

  c4c::Node* this_ref = parser.make_node(c4c::NK_VAR, 2);
  this_ref->name = arena.strdup("this");
  this_ref->unqualified_name = arena.strdup("this");
  this_ref->unqualified_text_id = texts.intern("this");
  this_ref->namespace_context_id = real_ns;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = this_ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("wrong::Owner::self");
  fn->unqualified_name = arena.strdup("self");
  fn->unqualified_text_id = self_text;
  fn->namespace_context_id = real_ns;
  fn->n_qualifier_segments = 1;
  fn->qualifier_segments = arena.alloc_array<const char*>(1);
  fn->qualifier_segments[0] = arena.strdup("Owner");
  fn->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  fn->qualifier_text_ids[0] = owner_text;
  fn->type = make_ts(c4c::TB_STRUCT, 1, "Owner");
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = wrong_owner;
  program->children[1] = real_owner;
  program->children[2] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(result.ok,
              "namespace owner resolution should use the method owner TextId before rendered owner spelling");
}

void test_sema_method_validation_prefers_structured_owner_key_for_fields() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base, int ptr_level = 0, const char* tag = nullptr) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    ts.ptr_level = ptr_level;
    ts.tag = tag;
    return ts;
  };

  const c4c::TextId real_ns_text = texts.intern("real");
  const c4c::TextId wrong_ns_text = texts.intern("wrong");
  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId actual_text = texts.intern("actual");
  const c4c::TextId get_text = texts.intern("get");
  const int real_ns = parser.ensure_named_namespace_context(0, real_ns_text, "real");
  const int wrong_ns = parser.ensure_named_namespace_context(0, wrong_ns_text, "wrong");

  c4c::Node* wrong_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  wrong_owner->name = arena.strdup("Owner");
  wrong_owner->unqualified_name = arena.strdup("Owner");
  wrong_owner->unqualified_text_id = owner_text;
  wrong_owner->namespace_context_id = wrong_ns;

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("Owner");
  real_owner->unqualified_name = arena.strdup("Owner");
  real_owner->unqualified_text_id = owner_text;
  real_owner->namespace_context_id = real_ns;
  real_owner->n_fields = 1;
  real_owner->fields = arena.alloc_array<c4c::Node*>(1);

  c4c::Node* actual = parser.make_node(c4c::NK_DECL, 1);
  actual->name = arena.strdup("actual");
  actual->unqualified_name = arena.strdup("actual");
  actual->unqualified_text_id = actual_text;
  actual->namespace_context_id = real_ns;
  actual->type = make_ts(c4c::TB_INT);
  real_owner->fields[0] = actual;

  c4c::Node* field_ref = parser.make_node(c4c::NK_VAR, 2);
  field_ref->name = arena.strdup("stale");
  field_ref->unqualified_name = arena.strdup("actual");
  field_ref->unqualified_text_id = actual_text;
  field_ref->namespace_context_id = real_ns;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = field_ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("wrong::Owner::get");
  fn->unqualified_name = arena.strdup("get");
  fn->unqualified_text_id = get_text;
  fn->namespace_context_id = real_ns;
  fn->n_qualifier_segments = 1;
  fn->qualifier_segments = arena.alloc_array<const char*>(1);
  fn->qualifier_segments[0] = arena.strdup("Owner");
  fn->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  fn->qualifier_text_ids[0] = owner_text;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = wrong_owner;
  program->children[1] = real_owner;
  program->children[2] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(result.ok,
              "method validation should use structured owner and field keys before rendered spelling");
}

void test_sema_method_validation_rejects_stale_rendered_field_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base, int ptr_level = 0, const char* tag = nullptr) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    ts.ptr_level = ptr_level;
    ts.tag = tag;
    return ts;
  };

  const c4c::TextId real_ns_text = texts.intern("real");
  const c4c::TextId wrong_ns_text = texts.intern("wrong");
  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId actual_text = texts.intern("actual");
  const c4c::TextId stale_text = texts.intern("stale");
  const c4c::TextId get_text = texts.intern("get");
  const int real_ns = parser.ensure_named_namespace_context(0, real_ns_text, "real");
  const int wrong_ns = parser.ensure_named_namespace_context(0, wrong_ns_text, "wrong");

  c4c::Node* wrong_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  wrong_owner->name = arena.strdup("Owner");
  wrong_owner->unqualified_name = arena.strdup("Owner");
  wrong_owner->unqualified_text_id = owner_text;
  wrong_owner->namespace_context_id = wrong_ns;
  wrong_owner->n_fields = 1;
  wrong_owner->fields = arena.alloc_array<c4c::Node*>(1);

  c4c::Node* stale = parser.make_node(c4c::NK_DECL, 1);
  stale->name = arena.strdup("stale");
  stale->unqualified_name = arena.strdup("stale");
  stale->unqualified_text_id = stale_text;
  stale->namespace_context_id = wrong_ns;
  stale->type = make_ts(c4c::TB_INT);
  wrong_owner->fields[0] = stale;

  c4c::Node* real_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  real_owner->name = arena.strdup("Owner");
  real_owner->unqualified_name = arena.strdup("Owner");
  real_owner->unqualified_text_id = owner_text;
  real_owner->namespace_context_id = real_ns;
  real_owner->n_fields = 1;
  real_owner->fields = arena.alloc_array<c4c::Node*>(1);

  c4c::Node* actual = parser.make_node(c4c::NK_DECL, 1);
  actual->name = arena.strdup("actual");
  actual->unqualified_name = arena.strdup("actual");
  actual->unqualified_text_id = actual_text;
  actual->namespace_context_id = real_ns;
  actual->type = make_ts(c4c::TB_INT);
  real_owner->fields[0] = actual;

  c4c::Node* field_ref = parser.make_node(c4c::NK_VAR, 2);
  field_ref->name = arena.strdup("stale");
  field_ref->unqualified_name = arena.strdup("stale");
  field_ref->unqualified_text_id = stale_text;
  field_ref->namespace_context_id = real_ns;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = field_ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("wrong::Owner::get");
  fn->unqualified_name = arena.strdup("get");
  fn->unqualified_text_id = get_text;
  fn->namespace_context_id = real_ns;
  fn->n_qualifier_segments = 1;
  fn->qualifier_segments = arena.alloc_array<const char*>(1);
  fn->qualifier_segments[0] = arena.strdup("Owner");
  fn->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  fn->qualifier_text_ids[0] = owner_text;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = wrong_owner;
  program->children[1] = real_owner;
  program->children[2] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "method validation should reject stale rendered field spelling when structured metadata misses");
}

void test_parser_template_instantiation_dedup_keys_skip_mark_on_failed_instantiation() {
  c4c::Arena arena;
  c4c::Parser parser({}, arena, nullptr, nullptr, c4c::SourceProfile::CppSubset);

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Broken");
  primary->unqualified_name = arena.strdup("Broken");
  primary->unqualified_text_id = 17;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), primary->unqualified_text_id,
      primary);

  c4c::Parser::TemplateArgParseResult arg{};
  arg.is_value = false;
  arg.type.array_size = -1;
  arg.type.inner_rank = -1;
  arg.type.base = c4c::TB_INT;
  std::string mangled;
  bool threw = false;
  try {
    (void)parser.ensure_template_struct_instantiated_from_args(
        "Broken", primary, {arg}, 1, &mangled,
        "template_instantiation_dedup_failed_mark");
  } catch (const std::exception&) {
    threw = true;
  }

  expect_true(threw,
              "no-text-table fixture should stop before template instantiation mark");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.instantiated_template_struct_keys_by_key.size()),
      0,
      "failed template instantiation should not mark a structured de-dup key");
}

void test_parser_template_instantiation_dedup_keys_structure_direct_emission() {
  c4c::Lexer lexer(
      "template<typename T>\n"
      "struct Box { T value; };\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  (void)parse_top_level(parser);
  expect_true(parser.find_template_struct_primary(
                  parser.current_namespace_context_id(),
                  lexer.text_table().intern("Box")) != nullptr,
              "direct emission fixture should register the template primary");

  auto parse_box_int = [&]() {
    parser.replace_token_stream_for_testing({
        parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box"),
        parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
        parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
        parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
        parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
    });
    return parser.parse_base_type();
  };

  c4c::TypeSpec first = parse_box_int();
  expect_true(first.base == c4c::TB_STRUCT && first.tag != nullptr,
              "template type parsing should emit a concrete struct type");
  expect_true(first.record_def != nullptr,
              "direct template emission should return the created record_def");
  expect_true(parser.definition_state_.struct_tag_def_map[first.tag] ==
                  first.record_def,
              "direct template emission record_def should match the tag map entry");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.instantiated_template_struct_keys_by_key.size()),
      1,
      "direct template emission should populate the structured de-dup key");

  parser.template_state_.instantiated_template_struct_keys_by_key.clear();
  parser.definition_state_.struct_tag_def_map.erase(first.tag);
  parser.definition_state_.defined_struct_tags.erase(first.tag);
  c4c::TypeSpec second = parse_box_int();
  expect_true(second.base == c4c::TB_STRUCT && second.tag != nullptr,
              "missing structured direct-emission de-dup should fall through to concrete emission");
  expect_true(second.record_def != nullptr,
              "demoted direct template emission should return the recreated record_def");
  expect_eq(first.tag, second.tag,
            "recreated direct-emission de-dup should preserve the instantiated tag spelling");
  expect_true(parser.definition_state_.struct_tag_def_map.count(first.tag) > 0,
              "missing structured direct-emission de-dup should recreate the concrete struct definition");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.instantiated_template_struct_keys_by_key.size()),
      1,
      "direct template emission should recreate the structured de-dup key");

  c4c::TypeSpec third = parse_box_int();
  expect_true(third.base == c4c::TB_STRUCT && third.tag != nullptr,
              "structured direct-emission de-dup should reuse the existing concrete type");
  expect_true(third.record_def == second.record_def,
              "structured direct-emission de-dup should keep the existing record_def");
  expect_eq(first.tag, third.tag,
            "structured direct-emission de-dup should preserve the instantiated tag");
  expect_eq_int(
      static_cast<int>(
          parser.template_state_.instantiated_template_struct_keys_by_key.size()),
      1,
      "direct template emission should keep one structured de-dup key after reuse");
}

void test_parser_template_substitution_preserves_record_definition_payloads() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token box_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box");
  const c4c::Token payload_alias_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "PayloadAlias");

  const c4c::TextId box_text = box_token.text_id;
  c4c::TypeSpec param_ts{};
  param_ts.base = c4c::TB_TYPEDEF;
  param_ts.tag = arena.strdup("T");
  param_ts.array_size = -1;
  param_ts.inner_rank = -1;

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->unqualified_name = arena.strdup("Box");
  primary->unqualified_text_id = box_text;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->n_template_args = 0;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;
  primary->n_bases = 1;
  primary->base_types = arena.alloc_array<c4c::TypeSpec>(1);
  primary->base_types[0] = param_ts;
  primary->n_member_typedefs = 1;
  primary->member_typedef_names = arena.alloc_array<const char*>(1);
  primary->member_typedef_names[0] = arena.strdup("Alias");
  primary->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  primary->member_typedef_types[0] = param_ts;
  primary->n_fields = 1;
  primary->fields = arena.alloc_array<c4c::Node*>(1);
  primary->fields[0] = parser.make_node(c4c::NK_DECL, 1);
  primary->fields[0]->name = arena.strdup("field");
  primary->fields[0]->type = param_ts;
  primary->n_children = 1;
  primary->children = arena.alloc_array<c4c::Node*>(1);
  c4c::Node* method = parser.make_node(c4c::NK_FUNCTION, 1);
  method->name = arena.strdup("get");
  method->type = param_ts;
  method->n_params = 1;
  method->params = arena.alloc_array<c4c::Node*>(1);
  method->params[0] = parser.make_node(c4c::NK_DECL, 1);
  method->params[0]->name = arena.strdup("arg");
  method->params[0]->type = param_ts;
  primary->children[0] = method;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), box_text, primary);
  const c4c::QualifiedNameKey box_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), box_text);
  parser.template_state_.template_struct_defs_by_key[box_key] = primary;
  c4c::TypeSpec box_alias{};
  box_alias.base = c4c::TB_STRUCT;
  box_alias.tag = arena.strdup("Box");
  box_alias.array_size = -1;
  box_alias.inner_rank = -1;
  parser.register_typedef_binding(box_text, box_alias, true);

  c4c::Node* payload = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  payload->name = arena.strdup("Payload");
  payload->n_fields = 0;
  parser.register_struct_definition_for_testing("Payload", payload);
  c4c::TypeSpec payload_alias{};
  payload_alias.base = c4c::TB_STRUCT;
  payload_alias.tag = arena.strdup("Payload");
  payload_alias.array_size = -1;
  payload_alias.inner_rank = -1;
  payload_alias.record_def = payload;
  parser.register_typedef_binding(payload_alias_token.text_id, payload_alias,
                                  true);
  c4c::Node* stale_payload = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_payload->name = arena.strdup("Payload");
  stale_payload->n_fields = 0;
  parser.register_struct_definition_for_testing("Payload", stale_payload);

  parser.replace_token_stream_for_testing({
      box_token,
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      payload_alias_token,
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  const c4c::TypeSpec box_ts = parser.parse_base_type();
  expect_true(box_ts.base == c4c::TB_STRUCT && box_ts.tag != nullptr,
              "direct template instantiation should produce a concrete struct type");
  expect_true(box_ts.record_def != nullptr,
              "direct template instantiation should return the instantiated record");
  const c4c::Node* box = box_ts.record_def;
  expect_true(box->n_bases == 1 && box->base_types,
              "template instantiation should clone base payloads");
  expect_true(box->base_types[0].tag != nullptr &&
                  std::string_view(box->base_types[0].tag) == "Payload",
              "template base substitution should preserve rendered tag spelling");
  expect_true(box->base_types[0].record_def == payload,
              "template base substitution should preserve record_def");
  expect_true(box->base_types[0].record_def != stale_payload,
              "template base substitution should not use stale rendered tag-map state");
  expect_true(box->n_member_typedefs == 1 && box->member_typedef_types,
              "template instantiation should clone member typedef payloads");
  expect_true(box->member_typedef_types[0].record_def == payload,
              "template member typedef substitution should preserve record_def");
  expect_true(box->n_fields == 1 && box->fields && box->fields[0],
              "template instantiation should clone field payloads");
  expect_true(box->fields[0]->type.record_def == payload,
              "template field substitution should preserve record_def");
  expect_true(box->n_children == 1 && box->children && box->children[0],
              "template instantiation should clone method payloads");
  const c4c::Node* cloned_method = box->children[0];
  expect_true(cloned_method->type.record_def == payload,
              "template method return substitution should preserve record_def");
  expect_true(cloned_method->n_params == 1 && cloned_method->params &&
                  cloned_method->params[0],
              "template instantiation should clone method parameter payloads");
  expect_true(cloned_method->params[0]->type.record_def == payload,
              "template method parameter substitution should preserve record_def");
}

void test_parser_direct_record_types_carry_record_definition() {
  c4c::Lexer lexer("struct Direct { int value; } after;\n");
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table());

  const c4c::TypeSpec direct_ts = parser.parse_base_type();
  expect_true(direct_ts.base == c4c::TB_STRUCT,
              "direct struct type parsing should produce a struct TypeSpec");
  expect_true(direct_ts.tag != nullptr,
              "direct struct type parsing should preserve the final tag spelling");
  expect_true(direct_ts.record_def != nullptr,
              "direct struct TypeSpec should carry the concrete parser record definition");
  expect_true(direct_ts.record_def->kind == c4c::NK_STRUCT_DEF,
              "direct struct record identity should point at an NK_STRUCT_DEF");
  expect_true(!direct_ts.record_def->is_union,
              "direct struct record identity should preserve record kind");
  expect_true(direct_ts.record_def->name == direct_ts.tag ||
                  std::string_view(direct_ts.record_def->name) ==
                      std::string_view(direct_ts.tag),
              "direct struct record identity should not change TypeSpec tag spelling");
  expect_true(parser.definition_state_.struct_tag_def_map[direct_ts.tag] ==
                  direct_ts.record_def,
              "direct struct record identity should match the compatibility tag map entry");

  c4c::Lexer union_lexer("union Payload { int value; } after;\n");
  const std::vector<c4c::Token> union_tokens = union_lexer.scan_all();
  c4c::Arena union_arena;
  c4c::Parser union_parser(union_tokens, union_arena, &union_lexer.text_table(),
                           &union_lexer.file_table());

  const c4c::TypeSpec union_ts = union_parser.parse_base_type();
  expect_true(union_ts.base == c4c::TB_UNION,
              "direct union type parsing should produce a union TypeSpec");
  expect_true(union_ts.record_def != nullptr,
              "direct union TypeSpec should carry the concrete parser record definition");
  expect_true(union_ts.record_def->kind == c4c::NK_STRUCT_DEF,
              "direct union record identity should point at an NK_STRUCT_DEF");
  expect_true(union_ts.record_def->is_union,
              "direct union record identity should preserve record kind");
  expect_true(union_ts.tag != nullptr &&
                  std::string_view(union_ts.record_def->name) ==
                      std::string_view(union_ts.tag),
              "direct union record identity should preserve TypeSpec tag spelling");
}

void test_parser_tag_only_record_types_keep_null_record_definition() {
  c4c::Lexer lexer("struct Forward after;\n");
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table());

  const c4c::TypeSpec tag_only_ts = parser.parse_base_type();
  expect_true(tag_only_ts.base == c4c::TB_STRUCT,
              "tag-only struct parsing should still produce a struct TypeSpec");
  expect_true(tag_only_ts.tag != nullptr,
              "tag-only struct parsing should preserve the tag spelling");
  expect_true(tag_only_ts.record_def == nullptr,
              "tag-only struct TypeSpec should not synthesize typed record identity");
}

c4c::TypeSpec parser_test_scalar_type(c4c::TypeBase base) {
  c4c::TypeSpec ts{};
  ts.base = base;
  ts.enum_underlying_base = c4c::TB_VOID;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

c4c::Node* parser_test_field(c4c::Parser& parser, c4c::Arena& arena,
                             const char* name, c4c::TypeSpec type) {
  c4c::Node* field = parser.make_node(c4c::NK_DECL, 1);
  field->name = arena.strdup(name);
  field->type = type;
  return field;
}

c4c::Node* parser_test_record(c4c::Parser& parser, c4c::Arena& arena,
                              const char* name,
                              std::initializer_list<c4c::Node*> fields) {
  c4c::Node* record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  record->name = arena.strdup(name);
  record->n_fields = static_cast<int>(fields.size());
  record->fields = arena.alloc_array<c4c::Node*>(record->n_fields);
  int index = 0;
  for (c4c::Node* field : fields) {
    record->fields[index++] = field;
  }
  return record;
}

void test_parser_direct_record_type_head_uses_structured_metadata() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId legacy_text =
      parser_test_text_id(parser, "LegacyOnly");
  c4c::Node* legacy_record =
      parser_test_record(parser, arena, "LegacyOnly", {});
  legacy_record->unqualified_text_id = c4c::kInvalidText;
  legacy_record->namespace_context_id = -1;
  parser.definition_state_.defined_struct_tags.insert("LegacyOnly");
  parser.register_struct_definition_for_testing("LegacyOnly", legacy_record);

  expect_true(!c4c::is_known_simple_type_head(parser, legacy_text,
                                              "LegacyOnly"),
              "rendered record-tag storage alone should not authorize a direct type-head probe");
  c4c::Parser::QualifiedNameRef legacy_qn;
  legacy_qn.base_name = "LegacyOnly";
  legacy_qn.base_text_id = legacy_text;
  const c4c::QualifiedTypeProbe legacy_probe =
      c4c::probe_qualified_type(parser, legacy_qn);
  expect_true(!legacy_probe.has_resolved_typedef,
              "direct record probes should reject rendered-only tag compatibility");

  const c4c::TextId structured_text =
      parser_test_text_id(parser, "Structured");
  c4c::Node* structured_record =
      parser_test_record(parser, arena, "RenderedMismatch", {});
  structured_record->unqualified_text_id = structured_text;
  structured_record->namespace_context_id =
      parser.current_namespace_context_id();
  parser.register_struct_definition_for_testing("stale_rendered_key",
                                                structured_record);

  expect_true(c4c::is_known_simple_type_head(parser, structured_text,
                                             "Structured"),
              "direct type-head probes should use record TextId metadata when rendered storage disagrees");
  c4c::Parser::QualifiedNameRef structured_qn;
  structured_qn.base_name = "Structured";
  structured_qn.base_text_id = structured_text;
  const c4c::QualifiedTypeProbe structured_probe =
      c4c::probe_qualified_type(parser, structured_qn);
  expect_true(structured_probe.has_resolved_typedef,
              "direct record probes should accept structured record metadata");
}

void test_parser_record_layout_const_eval_uses_record_definition_authority() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

  c4c::Node* real = parser_test_record(
      parser, arena, "Real",
      {parser_test_field(parser, arena, "head",
                         parser_test_scalar_type(c4c::TB_CHAR)),
       parser_test_field(parser, arena, "value",
                         parser_test_scalar_type(c4c::TB_INT))});
  c4c::Node* stale = parser_test_record(
      parser, arena, "Stale",
      {parser_test_field(parser, arena, "head",
                         parser_test_scalar_type(c4c::TB_CHAR)),
       parser_test_field(parser, arena, "value",
                         parser_test_scalar_type(c4c::TB_CHAR))});

  std::unordered_map<std::string, c4c::Node*> compatibility_tag_map;
  compatibility_tag_map["Shared"] = stale;

  c4c::TypeSpec typed = parser_test_scalar_type(c4c::TB_STRUCT);
  typed.tag = arena.strdup("Shared");
  typed.record_def = real;

  c4c::Node* align_node = parser.make_node(c4c::NK_ALIGNOF_TYPE, 1);
  align_node->type = typed;
  long long align_value = 0;
  expect_true(c4c::eval_const_int(align_node, &align_value,
                                  &compatibility_tag_map),
              "alignof should evaluate for typed record TypeSpecs");
  expect_eq_int(static_cast<int>(align_value), 4,
                "alignof should use record_def authority before stale final-spelling fallback");

  c4c::Node* size_node = parser.make_node(c4c::NK_SIZEOF_TYPE, 1);
  size_node->type = typed;
  long long size_value = 0;
  expect_true(c4c::eval_const_int(size_node, &size_value,
                                  &compatibility_tag_map),
              "sizeof should evaluate for typed record TypeSpecs");
  expect_eq_int(static_cast<int>(size_value), 8,
                "sizeof should use record_def authority before stale final-spelling fallback");

  c4c::Node* offset_node = parser.make_node(c4c::NK_OFFSETOF, 1);
  offset_node->type = typed;
  offset_node->name = arena.strdup("value");
  long long offset_value = 0;
  expect_true(c4c::eval_const_int(offset_node, &offset_value,
                                  &compatibility_tag_map),
              "offsetof should evaluate for typed record TypeSpecs");
  expect_eq_int(static_cast<int>(offset_value), 4,
                "offsetof should use record_def authority before stale final-spelling fallback");
}

void test_parser_record_layout_const_eval_keeps_final_spelling_fallback() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files);

  c4c::Node* fallback = parser_test_record(
      parser, arena, "Fallback",
      {parser_test_field(parser, arena, "head",
                         parser_test_scalar_type(c4c::TB_CHAR)),
       parser_test_field(parser, arena, "value",
                         parser_test_scalar_type(c4c::TB_CHAR))});

  std::unordered_map<std::string, c4c::Node*> compatibility_tag_map;
  compatibility_tag_map["Fallback"] = fallback;

  c4c::TypeSpec tag_only = parser_test_scalar_type(c4c::TB_STRUCT);
  tag_only.tag = arena.strdup("Fallback");

  c4c::Node* align_node = parser.make_node(c4c::NK_ALIGNOF_TYPE, 1);
  align_node->type = tag_only;
  long long align_value = 0;
  expect_true(c4c::eval_const_int(align_node, &align_value,
                                  &compatibility_tag_map),
              "alignof should keep tag-only final-spelling compatibility fallback");
  expect_eq_int(static_cast<int>(align_value), 1,
                "alignof fallback should use the rendered compatibility tag map");

  c4c::Node* size_node = parser.make_node(c4c::NK_SIZEOF_TYPE, 1);
  size_node->type = tag_only;
  long long size_value = 0;
  expect_true(c4c::eval_const_int(size_node, &size_value,
                                  &compatibility_tag_map),
              "sizeof should keep tag-only final-spelling compatibility fallback");
  expect_eq_int(static_cast<int>(size_value), 2,
                "sizeof fallback should use the rendered compatibility tag map");

  c4c::Node* offset_node = parser.make_node(c4c::NK_OFFSETOF, 1);
  offset_node->type = tag_only;
  offset_node->name = arena.strdup("value");
  long long offset_value = 0;
  expect_true(c4c::eval_const_int(offset_node, &offset_value,
                                  &compatibility_tag_map),
              "offsetof should keep tag-only final-spelling compatibility fallback");
  expect_eq_int(static_cast<int>(offset_value), 1,
                "offsetof fallback should use the rendered compatibility tag map");
}

void test_parser_incomplete_decl_checks_prefer_record_definition() {
  auto make_alias_type = [](c4c::Arena& arena,
                            c4c::Node* real) -> c4c::TypeSpec {
    c4c::TypeSpec alias_ts = parser_test_scalar_type(c4c::TB_STRUCT);
    alias_ts.tag = arena.strdup("Shared");
    alias_ts.record_def = real;
    return alias_ts;
  };

  {
    c4c::Lexer lexer("Alias global_value;\n", c4c::LexProfile::CppSubset);
    const std::vector<c4c::Token> tokens = lexer.scan_all();
    c4c::Arena arena;
    c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                       c4c::SourceProfile::CppSubset);

    c4c::Node* real = parser_test_record(parser, arena, "Real", {});
    c4c::Node* stale = parser.make_node(c4c::NK_STRUCT_DEF, 1);
    stale->name = arena.strdup("Stale");
    stale->n_fields = -1;
    parser.definition_state_.struct_tag_def_map["Shared"] = stale;
    parser.register_typedef_binding(parser_test_text_id(parser, "Alias"), make_alias_type(arena, real), true);

    c4c::Node* decl = parse_top_level(parser);
    expect_true(decl != nullptr && decl->kind == c4c::NK_GLOBAL_VAR,
                "top-level declarations should accept a complete record_def even when the rendered tag map is stale");
    expect_true(decl->type.record_def == real,
                "top-level declaration should preserve the typed record identity");
  }

  {
    c4c::Lexer lexer("Alias local_value;\n", c4c::LexProfile::CppSubset);
    const std::vector<c4c::Token> tokens = lexer.scan_all();
    c4c::Arena arena;
    c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                       c4c::SourceProfile::CppSubset);

    c4c::Node* real = parser_test_record(parser, arena, "Real", {});
    c4c::Node* stale = parser.make_node(c4c::NK_STRUCT_DEF, 1);
    stale->name = arena.strdup("Stale");
    stale->n_fields = -1;
    parser.definition_state_.struct_tag_def_map["Shared"] = stale;

    const c4c::TextId alias_text = lexer.text_table().intern("Alias");
    parser.push_local_binding_scope();
    parser.bind_local_typedef(alias_text, make_alias_type(arena, real));

    c4c::Node* decl = c4c::parse_stmt(parser);
    expect_true(decl != nullptr && decl->kind == c4c::NK_DECL,
                "local declarations should accept a complete record_def even when the rendered tag map is stale");
    expect_true(decl->type.record_def == real,
                "local declaration should preserve the typed record identity");
    expect_true(parser.pop_local_binding_scope(),
                "test fixture should balance the local visible typedef scope");
  }
}

void test_parser_alias_template_value_probes_use_token_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::TextId direct_alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(
          parser.current_namespace_context_id(),
          direct_alias_text),
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
  resolved_parser.register_typedef_binding(parser_test_text_id(resolved_parser, "ns::Alias"), alias_ts, true);
  const c4c::QualifiedNameKey resolved_alias_key =
      resolved_parser.intern_semantic_name_key(parser_test_text_id(resolved_parser, "ns::Alias"));
  resolved_parser.register_known_fn_name(resolved_alias_key);
  resolved_parser.register_using_value_alias_for_testing(
      0, alias_text, resolved_alias_key, "corrupted");
  resolved_parser.register_alias_template_info_for_testing(
      resolved_parser.alias_template_key_in_context(
          resolved_parser.current_namespace_context_id(),
          resolved_parser.find_parser_text_id("ns::Alias")),
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
      parser.current_namespace_context_id(), alias_text);
  c4c::ParserAliasTemplateInfo info;
  info.param_names = {"T"};
  info.aliased_type.array_size = -1;
  info.aliased_type.inner_rank = -1;
  info.aliased_type.base = c4c::TB_INT;
  parser.register_alias_template_info_for_testing(alias_key, info);
  const c4c::QualifiedNameKey bridge_key =
      parser.intern_semantic_name_key(parser_test_text_id(parser, "Bridge"));
  parser.register_known_fn_name(bridge_key);
  parser.register_using_value_alias_for_testing(
      0, alias_text, bridge_key, "Bridge");

  const c4c::ParserAliasTemplateInfo* found =
      parser.find_alias_template_info_in_context(
          parser.current_namespace_context_id(), alias_text);
  expect_true(found != nullptr && found->aliased_type.base == c4c::TB_INT,
              "alias-template info lookup should prefer the structured key before any rendered-name recovery");
}

void test_parser_alias_template_substitution_prefers_param_text_id() {
  c4c::Lexer lexer("Alias<int>", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_placeholder{};
  alias_placeholder.array_size = -1;
  alias_placeholder.inner_rank = -1;
  alias_placeholder.base = c4c::TB_TYPEDEF;
  alias_placeholder.tag = arena.strdup("Alias");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("T");
  parser.register_typedef_binding(alias_text, alias_placeholder, true);

  c4c::ParserAliasTemplateInfo info;
  info.param_names = {"StaleRenderedT"};
  info.param_name_text_ids = {param_text};
  info.param_is_nttp = {false};
  info.param_is_pack = {false};
  info.param_has_default = {false};
  info.aliased_type.array_size = -1;
  info.aliased_type.inner_rank = -1;
  info.aliased_type.base = c4c::TB_TYPEDEF;
  info.aliased_type.tag = arena.strdup("T");
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(
          parser.current_namespace_context_id(), alias_text),
      info);

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(resolved.base == c4c::TB_INT,
              "alias-template substitution should bind by parameter TextId even when rendered parameter spelling is stale");
}

void test_parser_alias_template_substitution_does_not_require_param_name_spelling() {
  c4c::Lexer lexer("Alias<int>", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec alias_placeholder{};
  alias_placeholder.array_size = -1;
  alias_placeholder.inner_rank = -1;
  alias_placeholder.base = c4c::TB_TYPEDEF;
  alias_placeholder.tag = arena.strdup("Alias");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("T");
  parser.register_typedef_binding(alias_text, alias_placeholder, true);

  c4c::ParserAliasTemplateInfo info;
  info.param_name_text_ids = {param_text};
  info.param_is_nttp = {false};
  info.param_is_pack = {false};
  info.param_has_default = {false};
  info.aliased_type.array_size = -1;
  info.aliased_type.inner_rank = -1;
  info.aliased_type.base = c4c::TB_TYPEDEF;
  info.aliased_type.tag = arena.strdup("T");
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(
          parser.current_namespace_context_id(), alias_text),
      info);

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(resolved.base == c4c::TB_INT,
              "alias-template substitution should use structured parameter metadata without rendered param_names");
}

void test_parser_alias_template_member_typedef_carrier_uses_structured_rhs() {
  c4c::Lexer lexer(
      "template<typename T> struct Owner { using type = T; };\n"
      "template<typename T> using Alias = typename Owner<T>::type;\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  (void)parse_top_level(parser);
  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Alias");
  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  const c4c::TextId param_text = parser.find_parser_text_id("T");
  const c4c::QualifiedNameKey alias_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), alias_text);
  const c4c::QualifiedNameKey owner_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), owner_text);

  const c4c::ParserAliasTemplateInfo* info =
      parser.find_alias_template_info(alias_key);
  expect_true(info != nullptr,
              "parsed alias templates should register structured alias metadata");
  expect_true(info->member_typedef.valid,
              "alias-template member typedef carrier should be populated from the parsed RHS");
  expect_true(info->member_typedef.owner_key == owner_key,
              "alias-template member typedef carrier should preserve the structured owner key");
  expect_true(info->member_typedef.member_text_id == member_text,
              "alias-template member typedef carrier should preserve the member TextId");
  expect_eq_int(static_cast<int>(info->member_typedef.owner_args.size()), 1,
                "alias-template member typedef carrier should preserve parsed owner args");
  expect_true(!info->member_typedef.owner_args[0].is_value &&
                  info->member_typedef.owner_args[0].type.base == c4c::TB_TYPEDEF &&
                  info->member_typedef.owner_args[0].type.tag != nullptr &&
                  parser.find_parser_text_id(
                      info->member_typedef.owner_args[0].type.tag) == param_text,
              "alias-template member typedef carrier should keep substitutable type args structured");

  c4c::ParserAliasTemplateInfo& mutable_info =
      parser.template_state_.alias_template_info[alias_key];
  mutable_info.aliased_type.tag = arena.strdup("CorruptRenderedOwner");
  mutable_info.aliased_type.deferred_member_type_name =
      arena.strdup("corrupt_member");
  expect_true(mutable_info.member_typedef.owner_key == owner_key &&
                  mutable_info.member_typedef.member_text_id == member_text,
              "alias-template member typedef carrier should survive rendered/deferred TypeSpec spelling drift");
}

void test_parser_alias_template_member_typedef_substitution_uses_structured_carrier() {
  c4c::Lexer lexer(
      "template<typename T> struct Owner { using type = T; };\n"
      "template<typename T> using Alias = typename Owner<T>::type;\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  (void)parse_top_level(parser);
  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Alias");
  const c4c::QualifiedNameKey alias_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), alias_text);
  c4c::ParserAliasTemplateInfo& info =
      parser.template_state_.alias_template_info[alias_key];
  expect_true(info.member_typedef.valid,
              "alias-template member typedef substitution test requires the structured carrier");
  info.aliased_type.base = c4c::TB_STRUCT;
  info.aliased_type.tag = arena.strdup("StaleRenderedOwner");
  info.aliased_type.tpl_struct_origin = info.aliased_type.tag;
  info.aliased_type.deferred_member_type_name = arena.strdup("stale_member");

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(resolved.base == c4c::TB_INT,
              "alias-template member typedef substitution should resolve through the structured carrier despite stale rendered/deferred TypeSpec spelling");
}

void test_parser_qualified_alias_template_member_typedef_substitution_uses_structured_carrier() {
  c4c::Lexer lexer(
      "namespace ns {\n"
      "template<typename T> struct Owner { using type = T; };\n"
      "template<typename T> using Alias = typename Owner<T>::type;\n"
      "}\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  (void)parse_top_level(parser);

  const c4c::TextId ns_text = parser.find_parser_text_id("ns");
  const int ns_context = parser.find_named_namespace_child(0, ns_text);
  expect_true(ns_context >= 0,
              "qualified alias-template carrier test requires the namespace context");
  const c4c::TextId alias_text = parser.find_parser_text_id("Alias");
  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  const c4c::QualifiedNameKey alias_key =
      parser.alias_template_key_in_context(ns_context, alias_text);
  const c4c::QualifiedNameKey owner_key =
      parser.alias_template_key_in_context(ns_context, owner_text);
  c4c::ParserAliasTemplateInfo& info =
      parser.template_state_.alias_template_info[alias_key];
  expect_true(info.member_typedef.valid &&
                  info.member_typedef.owner_key == owner_key &&
                  info.member_typedef.member_text_id == member_text,
              "qualified alias-template member typedef test requires the structured carrier");
  info.aliased_type.base = c4c::TB_STRUCT;
  info.aliased_type.tag = arena.strdup("wrong::RenderedOwner");
  info.aliased_type.tpl_struct_origin = info.aliased_type.tag;
  info.aliased_type.deferred_member_type_name = arena.strdup("wrong_member");

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ns"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(resolved.base == c4c::TB_INT,
              "qualified alias-template member typedef substitution should consume the structured carrier despite stale rendered/deferred spelling");
}

void test_parser_alias_of_alias_member_typedef_substitution_uses_structured_carrier() {
  c4c::Lexer lexer(
      "using Direct = int;\n"
      "using AliasArg = Direct;\n"
      "template<typename T> struct Owner { using type = T; };\n"
      "template<typename T> using Alias = typename Owner<T>::type;\n",
      c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  (void)parse_top_level(parser);
  (void)parse_top_level(parser);
  (void)parse_top_level(parser);
  (void)parse_top_level(parser);

  const c4c::TextId alias_text = parser.find_parser_text_id("Alias");
  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  const c4c::QualifiedNameKey alias_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), alias_text);
  const c4c::QualifiedNameKey owner_key =
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           owner_text);
  c4c::ParserAliasTemplateInfo& info =
      parser.template_state_.alias_template_info[alias_key];
  expect_true(info.member_typedef.valid &&
                  info.member_typedef.owner_key == owner_key &&
                  info.member_typedef.member_text_id == member_text,
              "alias-of-alias member typedef test requires the structured carrier");
  info.aliased_type.base = c4c::TB_STRUCT;
  info.aliased_type.tag = arena.strdup("WrongRenderedOwner");
  info.aliased_type.tpl_struct_origin = info.aliased_type.tag;
  info.aliased_type.deferred_member_type_name = arena.strdup("wrong_member");

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "AliasArg"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
  });

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(resolved.base == c4c::TB_INT,
              "alias-template member typedef substitution should consume alias-of-alias arguments through the structured carrier despite stale rendered/deferred spelling");
}

void test_template_arg_ref_equivalence_ignores_debug_text_when_structured_payload_matches() {
  c4c::Arena arena;

  c4c::TypeSpec lhs{};
  lhs.array_size = -1;
  lhs.inner_rank = -1;
  lhs.base = c4c::TB_STRUCT;
  lhs.tag = arena.strdup("Box");
  lhs.tpl_struct_args.size = 1;
  lhs.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  lhs.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  lhs.tpl_struct_args.data[0].value = 7;
  lhs.tpl_struct_args.data[0].debug_text = arena.strdup("N");

  c4c::TypeSpec rhs = lhs;
  rhs.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  rhs.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  rhs.tpl_struct_args.data[0].value = 7;
  rhs.tpl_struct_args.data[0].debug_text = arena.strdup("DifferentRenderedName");

  expect_true(c4c::type_binding_values_equivalent(lhs, rhs),
              "template argument equivalence should prefer structured kind/value payload over debug_text");
}

void test_canonical_template_struct_type_key_prefers_structured_arg_over_debug_text() {
  c4c::Arena arena;

  auto make_box = [&](const char* debug_text) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = c4c::TB_TYPEDEF;
    ts.tpl_struct_origin = arena.strdup("Box");
    ts.tpl_struct_args.size = 1;
    ts.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
    ts.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
    ts.tpl_struct_args.data[0].value = 7;
    ts.tpl_struct_args.data[0].debug_text = arena.strdup(debug_text);
    return ts;
  };

  const std::string lhs_key =
      c4c::canonical_template_struct_type_key(make_box("N"));
  const std::string rhs_key =
      c4c::canonical_template_struct_type_key(make_box("DifferentRenderedName"));

  expect_eq(lhs_key, rhs_key,
            "canonical template struct type keys should prefer structured template arg value over debug_text");
  expect_true(lhs_key.find("v:7") != std::string::npos,
              "canonical template struct type key should include the structured value argument");
}

void test_parser_template_arg_ref_rendering_prefers_structured_nested_arg() {
  c4c::Lexer lexer("Outer<InnerAlias> after", c4c::LexProfile::CppSubset);
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  const c4c::TextId outer_text = lexer.text_table().intern("Outer");
  c4c::Node* outer = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  outer->name = arena.strdup("Outer");
  outer->unqualified_name = arena.strdup("Outer");
  outer->unqualified_text_id = outer_text;
  outer->n_template_params = 1;
  outer->template_param_names = arena.alloc_array<const char*>(1);
  outer->template_param_names[0] = arena.strdup("Ts");
  outer->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  outer->template_param_name_text_ids[0] = lexer.text_table().intern("Ts");
  outer->template_param_is_nttp = arena.alloc_array<bool>(1);
  outer->template_param_is_nttp[0] = false;
  outer->template_param_is_pack = arena.alloc_array<bool>(1);
  outer->template_param_is_pack[0] = true;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), outer_text, outer);

  c4c::TypeSpec outer_alias{};
  outer_alias.array_size = -1;
  outer_alias.inner_rank = -1;
  outer_alias.base = c4c::TB_STRUCT;
  outer_alias.tag = arena.strdup("Outer");
  parser.register_typedef_binding(outer_text, outer_alias, true);

  c4c::TypeSpec inner_alias{};
  inner_alias.array_size = -1;
  inner_alias.inner_rank = -1;
  inner_alias.base = c4c::TB_TYPEDEF;
  inner_alias.tag = arena.strdup("Inner");
  inner_alias.tpl_struct_origin = arena.strdup("Inner");
  inner_alias.tpl_struct_args.size = 1;
  inner_alias.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  inner_alias.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  inner_alias.tpl_struct_args.data[0].value = 7;
  inner_alias.tpl_struct_args.data[0].debug_text = arena.strdup("StaleRenderedN");
  const c4c::TextId inner_alias_text = lexer.text_table().intern("InnerAlias");
  parser.register_typedef_binding(inner_alias_text, inner_alias, true);

  const c4c::TypeSpec parsed = parser.parse_base_type();
  expect_true(parsed.tpl_struct_args.size == 1 && parsed.tpl_struct_args.data,
              "pack template parsing should preserve one rendered template arg ref");
  expect_eq(parsed.tpl_struct_args.data[0].debug_text, "@Inner:7",
            "parser template arg ref rendering should prefer structured nested value over debug_text");
}

void test_consteval_template_arg_expr_payload_ignores_stale_rendered_name() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->kind = c4c::NK_FUNCTION;
  fn->n_template_params = 1;
  fn->template_param_names = arena.alloc_array<const char*>(1);
  fn->template_param_names[0] = arena.strdup("N");
  fn->template_param_is_nttp = arena.alloc_array<bool>(1);
  fn->template_param_is_nttp[0] = true;

  c4c::Node* lhs = parser.make_node(c4c::NK_VAR, 1);
  lhs->kind = c4c::NK_VAR;
  lhs->name = arena.strdup("Structured");
  c4c::Node* rhs = parser.make_node(c4c::NK_INT_LIT, 1);
  rhs->kind = c4c::NK_INT_LIT;
  rhs->ival = 1;
  c4c::Node* expr = parser.make_node(c4c::NK_BINOP, 1);
  expr->kind = c4c::NK_BINOP;
  expr->op = arena.strdup("+");
  expr->left = lhs;
  expr->right = rhs;

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 1);
  callee->kind = c4c::NK_VAR;
  callee->name = arena.strdup("fn");
  callee->has_template_args = true;
  callee->n_template_args = 1;
  callee->template_arg_is_value = arena.alloc_array<bool>(1);
  callee->template_arg_is_value[0] = true;
  callee->template_arg_values = arena.alloc_array<long long>(1);
  callee->template_arg_values[0] = 0;
  callee->template_arg_nttp_names = arena.alloc_array<const char*>(1);
  callee->template_arg_nttp_names[0] = arena.strdup("$expr:Rendered+1");
  callee->template_arg_exprs = arena.alloc_array<c4c::Node*>(1);
  callee->template_arg_exprs[0] = expr;

  c4c::hir::ConstMap outer_nttp;
  outer_nttp["Structured"] = 6;
  outer_nttp["Rendered"] = 100;
  c4c::hir::ConstEvalEnv outer_env{};
  outer_env.nttp_bindings = &outer_nttp;
  c4c::hir::TypeBindings type_bindings;
  c4c::hir::NttpBindings nttp_bindings;
  c4c::hir::bind_consteval_call_env(
      callee, fn, outer_env, &type_bindings, &nttp_bindings);

  expect_true(nttp_bindings.count("N") == 1,
              "consteval call binding should evaluate structured NTTP expression payload");
  expect_eq_int(static_cast<int>(nttp_bindings["N"]), 7,
                "stale rendered $expr text must not select the NTTP value");
}

void test_consteval_value_lookup_prefers_structured_metadata_over_stale_rendered_name() {
  c4c::TextTable texts;
  const c4c::TextId actual_text = texts.intern("Actual");
  const c4c::TextId text_only_text = texts.intern("TextOnly");

  c4c::Node ref{};
  ref.kind = c4c::NK_VAR;
  ref.name = "stale";
  ref.unqualified_name = "Actual";
  ref.unqualified_text_id = actual_text;
  ref.namespace_context_id = 7;

  c4c::hir::ConstMap rendered;
  rendered["stale"] = 1;
  c4c::hir::ConstTextMap by_text;
  by_text[actual_text] = 2;
  c4c::hir::ConstStructuredMap by_key;
  c4c::hir::ConstEvalStructuredNameKey key;
  key.namespace_context_id = 7;
  key.base_text_id = actual_text;
  by_key[key] = 3;

  c4c::hir::ConstEvalEnv env{};
  env.named_consts = &rendered;
  env.named_consts_by_text = &by_text;
  env.named_consts_by_key = &by_key;

  auto structured = c4c::hir::evaluate_constant_expr(&ref, env);
  expect_true(structured.ok(), "structured consteval value lookup should produce a value");
  expect_eq_int(static_cast<int>(structured.as_int()), 3,
                "consteval value lookup should prefer structured metadata over stale rendered names");

  c4c::Node text_ref = ref;
  text_ref.name = "stale_text_only";
  text_ref.unqualified_name = "TextOnly";
  text_ref.unqualified_text_id = text_only_text;
  text_ref.namespace_context_id = -1;
  rendered["stale_text_only"] = 4;
  by_text[text_only_text] = 5;

  auto text = c4c::hir::evaluate_constant_expr(&text_ref, env);
  expect_true(text.ok(), "TextId consteval value lookup should produce a value");
  expect_eq_int(static_cast<int>(text.as_int()), 5,
                "consteval value lookup should prefer TextId metadata over stale rendered names");
}

c4c::Node* make_consteval_returning(c4c::Parser& parser, c4c::Arena& arena,
                                    const char* name, c4c::TextId text_id,
                                    long long value) {
  c4c::Node* literal = parser.make_node(c4c::NK_INT_LIT, 1);
  literal->ival = value;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 1);
  ret->left = literal;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 1);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->name = arena.strdup(name);
  fn->unqualified_name = arena.strdup(name);
  fn->unqualified_text_id = text_id;
  fn->namespace_context_id = 11;
  fn->type = make_sema_lookup_ts(c4c::TB_INT);
  fn->is_consteval = true;
  fn->body = body;
  return fn;
}

c4c::Node* make_consteval_calling(c4c::Parser& parser, c4c::Arena& arena,
                                  c4c::Node* callee) {
  c4c::Node* call = parser.make_node(c4c::NK_CALL, 1);
  call->left = callee;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 1);
  ret->left = call;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 1);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->name = arena.strdup("caller");
  fn->unqualified_name = arena.strdup("caller");
  fn->unqualified_text_id = parser.parser_text_id_for_token(c4c::kInvalidText, "caller");
  fn->namespace_context_id = 11;
  fn->is_consteval = true;
  fn->body = body;
  return fn;
}

void test_consteval_function_lookup_prefers_structured_and_text_metadata_over_stale_rendered_name() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId stale_text = texts.intern("stale_consteval");
  const c4c::TextId actual_text = texts.intern("actual_consteval");
  c4c::Node* stale_fn = make_consteval_returning(parser, arena, "stale_consteval",
                                                 stale_text, 10);
  c4c::Node* actual_fn = make_consteval_returning(parser, arena, "actual_consteval",
                                                  actual_text, 7);

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 1);
  callee->name = arena.strdup("stale_consteval");
  callee->unqualified_name = arena.strdup("actual_consteval");
  callee->unqualified_text_id = actual_text;
  callee->namespace_context_id = 11;
  c4c::Node* caller = make_consteval_calling(parser, arena, callee);

  std::unordered_map<std::string, const c4c::Node*> rendered;
  rendered["stale_consteval"] = stale_fn;
  c4c::hir::ConstEvalFunctionTextMap by_text;
  by_text[actual_text] = actual_fn;
  c4c::hir::ConstEvalFunctionStructuredMap by_key;
  c4c::hir::ConstEvalStructuredNameKey key;
  key.namespace_context_id = 11;
  key.base_text_id = actual_text;
  by_key[key] = actual_fn;

  c4c::hir::ConstEvalEnv env{};
  auto structured = c4c::hir::evaluate_consteval_call(
      caller, {}, env, rendered, 0, &by_text, &by_key);
  expect_true(structured.ok(), "structured consteval function lookup should evaluate");
  expect_eq_int(static_cast<int>(structured.as_int()), 7,
                "consteval function lookup should prefer structured metadata over stale rendered names");

  c4c::hir::ConstEvalFunctionStructuredMap empty_key;
  auto text = c4c::hir::evaluate_consteval_call(
      caller, {}, env, rendered, 0, &by_text, &empty_key);
  expect_true(text.ok(), "TextId consteval function lookup should evaluate");
  expect_eq_int(static_cast<int>(text.as_int()), 7,
                "consteval function lookup should prefer TextId metadata over stale rendered names");

  c4c::hir::ConstEvalFunctionTextMap empty_text;
  auto authoritative_miss = c4c::hir::evaluate_consteval_call(
      caller, {}, env, rendered, 0, &empty_text, &empty_key);
  expect_true(!authoritative_miss.ok(),
              "consteval function lookup should reject stale rendered names after metadata misses");

  c4c::Node* legacy_callee = parser.make_node(c4c::NK_VAR, 1);
  legacy_callee->name = arena.strdup("stale_consteval");
  c4c::Node* legacy_caller = make_consteval_calling(parser, arena, legacy_callee);
  auto compatibility = c4c::hir::evaluate_consteval_call(
      legacy_caller, {}, env, rendered, 0, &empty_text, &empty_key);
  expect_true(compatibility.ok(),
              "consteval function lookup should retain rendered fallback without metadata");
  expect_eq_int(static_cast<int>(compatibility.as_int()), 10,
                "consteval function lookup fallback should still use rendered names");
}

void test_sema_consteval_function_lookup_prefers_text_metadata_over_stale_rendered_name() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId stale_text = texts.intern("stale_consteval");
  const c4c::TextId actual_text = texts.intern("actual_consteval");
  c4c::Node* stale_fn = make_consteval_returning(parser, arena, "stale_consteval",
                                                 stale_text, 0);
  c4c::Node* actual_fn = make_consteval_returning(parser, arena, "actual_consteval",
                                                  actual_text, 1);

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 1);
  callee->name = arena.strdup("stale_consteval");
  callee->unqualified_name = arena.strdup("actual_consteval");
  callee->unqualified_text_id = actual_text;
  callee->namespace_context_id = -1;

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 1);
  call->left = callee;

  c4c::Node* static_assert_node = parser.make_node(c4c::NK_STATIC_ASSERT, 1);
  static_assert_node->left = call;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = stale_fn;
  program->children[1] = actual_fn;
  program->children[2] = static_assert_node;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  const std::string diag =
      result.diagnostics.empty() ? "" : (": " + result.diagnostics.front().message);
  expect_true(result.ok,
              "Sema consteval lookup should prefer TextId metadata over stale rendered names" + diag);
}

void test_sema_consteval_function_lookup_rejects_stale_rendered_name_after_text_miss() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId stale_text = texts.intern("stale_consteval");
  const c4c::TextId missing_text = texts.intern("missing_consteval");
  c4c::Node* stale_fn = make_consteval_returning(parser, arena, "stale_consteval",
                                                 stale_text, 0);

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 1);
  callee->name = arena.strdup("stale_consteval");
  callee->unqualified_name = arena.strdup("missing_consteval");
  callee->unqualified_text_id = missing_text;
  callee->namespace_context_id = -1;

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 1);
  call->left = callee;

  c4c::Node* static_assert_node = parser.make_node(c4c::NK_STATIC_ASSERT, 1);
  static_assert_node->left = call;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = stale_fn;
  program->children[1] = static_assert_node;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  const std::string diag =
      result.diagnostics.empty() ? "" : (": " + result.diagnostics.front().message);
  expect_true(result.ok,
              "Sema consteval lookup should not use stale rendered fallback after TextId miss" + diag);
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
  test_parser_id_first_binding_helpers_prefer_text_ids();
  test_parser_heavy_snapshot_restores_symbol_id_keyed_tables();
  test_parser_keeps_qualified_bindings_string_keyed();
  test_parser_structured_value_registration_avoids_string_bridge_and_legacy_mirror();
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
  test_parser_dependent_typename_owner_alias_prefers_record_definition();
  test_parser_member_typedef_suffix_prefers_record_definition();
  test_parser_member_typedef_suffix_uses_tagless_record_definition();
  test_parser_member_typedef_suffix_rejects_rendered_owner_fallbacks();
  test_parser_nested_dependent_typename_prefers_record_definition();
  test_parser_nested_dependent_typename_uses_tagless_record_definition();
  test_parser_record_ctor_probe_prefers_record_definition();
  test_parser_is_typedef_name_uses_local_visible_scope_lookup();
  test_parser_conflicting_user_typedef_binding_uses_local_visible_scope_lookup();
  test_parser_record_body_context_keeps_visible_template_origin_lookup_local();
  test_parser_visible_type_alias_uses_scope_local_typedef_facade();
  test_parser_visible_type_alias_resolves_scope_local_target_type();
  test_parser_visible_type_alias_uses_token_text_id_scope_lookup();
  test_parser_visible_type_alias_keeps_qualified_target_resolution();
  test_parser_resolve_typedef_type_chain_uses_local_visible_scope_lookup();
  test_parser_using_value_import_keeps_structured_target_key();
  test_parser_using_value_import_prefers_structured_type_over_corrupt_rendered_name();
  test_parser_global_using_value_import_keeps_global_target_resolution();
  test_parser_out_of_class_operator_registers_structured_global_key();
  test_parser_out_of_class_constructor_registers_structured_global_key();
  test_parser_namespace_lookup_rejects_type_projection_bridges_and_demotes_value_bridges();
  test_parser_qualified_type_parse_fallback_requires_structured_type();
  test_parser_qualified_functional_cast_owner_requires_structured_authority();
  test_parser_qualified_member_typedef_lookup_requires_structured_metadata();
  test_parser_record_body_member_typedef_writers_register_direct_keys();
  test_parser_template_record_member_typedef_writer_registers_dependent_key();
  test_parser_c_style_cast_member_typedef_uses_structured_metadata();
  test_parser_template_instantiation_member_typedef_uses_concrete_key();
  test_parser_namespace_typedef_registration_stays_namespace_scoped();
  test_parser_using_value_alias_rejects_missing_structured_target_bridge();
  test_parser_using_value_alias_prefers_structured_target_type();
  test_parser_using_value_alias_respects_local_shadowing();
  test_parser_visible_value_alias_rejects_scope_local_target_bridge();
  test_parser_using_type_import_prefers_structured_type_over_corrupt_rendered_name();
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
  test_parser_local_using_alias_qualified_static_call_keeps_spelled_value();
  test_parser_stmt_disambiguates_qualified_visible_value_member_access_as_expr();
  test_parser_stmt_disambiguates_qualified_visible_value_member_access_assignment_as_expr();
  test_parser_template_member_suffix_probe_uses_token_spelling();
  test_parser_template_type_arg_probes_use_token_spelling();
  test_parser_template_scope_type_param_prefers_text_id_over_spelling();
  test_parser_template_specialization_binding_prefers_param_text_id();
  test_parser_template_type_arg_uses_visible_scope_local_alias();
  test_parser_synthesized_typedef_binding_unregisters_by_text_id();
  test_parser_template_type_arg_prefers_local_visible_typedef_text_id();
  test_parser_deferred_nttp_builtin_trait_uses_visible_scope_local_alias();
  test_parser_deferred_nttp_member_lookup_uses_visible_scope_local_aliases();
  test_parser_template_lookup_uses_structured_template_keys();
  test_parser_nttp_default_cache_uses_structured_key_only();
  test_parser_template_instantiation_dedup_keys_structure_specialization_reuse();
  test_parser_template_static_member_lookup_prefers_record_definition();
  test_sema_static_member_type_lookup_prefers_structured_member_key();
  test_sema_unqualified_symbol_lookup_prefers_structured_key_over_rendered_spelling();
  test_sema_unqualified_symbol_lookup_rejects_stale_rendered_local_spelling();
  test_sema_unqualified_symbol_lookup_rejects_stale_rendered_global_spelling();
  test_sema_overload_lookup_prefers_structured_key_over_rendered_spelling();
  test_sema_overload_lookup_ignores_stale_rendered_name_after_structured_miss();
  test_sema_overload_lookup_keeps_no_metadata_rendered_compatibility();
  test_sema_namespace_owner_resolution_prefers_structured_owner_key();
  test_sema_method_validation_prefers_structured_owner_key_for_fields();
  test_sema_method_validation_rejects_stale_rendered_field_spelling();
  test_parser_template_instantiation_dedup_keys_skip_mark_on_failed_instantiation();
  test_parser_template_instantiation_dedup_keys_structure_direct_emission();
  test_parser_template_substitution_preserves_record_definition_payloads();
  test_parser_direct_record_types_carry_record_definition();
  test_parser_tag_only_record_types_keep_null_record_definition();
  test_parser_direct_record_type_head_uses_structured_metadata();
  test_parser_record_layout_const_eval_uses_record_definition_authority();
  test_parser_record_layout_const_eval_keeps_final_spelling_fallback();
  test_parser_incomplete_decl_checks_prefer_record_definition();
  test_parser_alias_template_value_probes_use_token_spelling();
  test_parser_alias_template_info_prefers_structured_key_over_recovery();
  test_parser_alias_template_substitution_prefers_param_text_id();
  test_parser_alias_template_substitution_does_not_require_param_name_spelling();
  test_parser_alias_template_member_typedef_carrier_uses_structured_rhs();
  test_parser_alias_template_member_typedef_substitution_uses_structured_carrier();
  test_parser_qualified_alias_template_member_typedef_substitution_uses_structured_carrier();
  test_parser_alias_of_alias_member_typedef_substitution_uses_structured_carrier();
  test_template_arg_ref_equivalence_ignores_debug_text_when_structured_payload_matches();
  test_canonical_template_struct_type_key_prefers_structured_arg_over_debug_text();
  test_parser_template_arg_ref_rendering_prefers_structured_nested_arg();
  test_consteval_template_arg_expr_payload_ignores_stale_rendered_name();
  test_consteval_value_lookup_prefers_structured_metadata_over_stale_rendered_name();
  test_consteval_function_lookup_prefers_structured_and_text_metadata_over_stale_rendered_name();
  test_sema_consteval_function_lookup_prefers_text_metadata_over_stale_rendered_name();
  test_sema_consteval_function_lookup_rejects_stale_rendered_name_after_text_miss();
  test_parser_typename_template_parameter_probe_uses_token_spelling();
  test_parser_post_pointer_qualifier_probes_use_token_spelling();
  test_parser_qualified_declarator_name_uses_token_spelling();

  std::cout << "PASS: frontend_parser_tests\n";
  return 0;
}
