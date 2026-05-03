#include "parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

template <typename T>
auto legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
  return ts.tag;
}

const char* legacy_tag_if_present(const c4c::TypeSpec&, long) {
  return nullptr;
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

void test_builtin_va_list_producer_sets_structured_identity() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwBuiltin,
                                 "__builtin_va_list"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  const c4c::TypeSpec ts = parser.parse_base_type();
  expect_true(ts.base == c4c::TB_VA_LIST,
              "__builtin_va_list should parse as TB_VA_LIST");
  expect_true(ts.tag_text_id ==
                  parser.parser_text_id_for_token(c4c::kInvalidText,
                                                  "__va_list"),
              "__builtin_va_list producer should attach structured __va_list identity");
}

void test_template_scope_type_param_producer_uses_text_carrier() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token param_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T");

  parser.push_template_scope(
      c4c::Parser::TemplateScopeKind::FreeFunctionTemplate,
      {{.name_text_id = param_token.text_id,
        .name = "StaleFrameName",
        .is_nttp = false}});
  parser.replace_token_stream_for_testing({
      param_token,
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });

  const c4c::TypeSpec ts = parser.parse_base_type();
  parser.pop_template_scope();
  expect_true(ts.base == c4c::TB_TYPEDEF,
              "template-scope type parameter should parse as typedef carrier");
  expect_true(ts.tag_text_id == param_token.text_id,
              "template-scope type parameter should keep tag_text_id carrier");
  expect_true(ts.template_param_text_id == param_token.text_id,
              "template-scope type parameter should keep template_param_text_id carrier");
}

void test_simple_typedef_producer_keeps_carrier_despite_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token alias_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "RealAlias");

  c4c::TypeSpec stale_alias{};
  stale_alias.array_size = -1;
  stale_alias.inner_rank = -1;
  stale_alias.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(stale_alias, arena.strdup("StaleRenderedAlias"), 0);
  stale_alias.tag_text_id = texts.intern("OldCarrier");
  parser.register_typedef_binding(alias_token.text_id, stale_alias, true);

  parser.replace_token_stream_for_testing({
      alias_token,
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });

  const c4c::TypeSpec ts = parser.parse_base_type();
  expect_true(ts.base == c4c::TB_STRUCT,
              "simple visible typedef producer should resolve the typedef type");
  expect_true(ts.tag_text_id == alias_token.text_id,
              "simple visible typedef producer should preserve source TextId carrier before stale rendered tag");
  const char* legacy_tag = legacy_tag_if_present(ts, 0);
  expect_true(!legacy_tag || std::string(legacy_tag) == "StaleRenderedAlias",
              "rendered tag remains compatibility spelling only");
}

void test_record_producers_assign_structured_metadata_before_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token record_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "RealRecord");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwStruct, "struct"),
      record_token,
      parser.make_injected_token(seed, c4c::TokenKind::LBrace, "{"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "field"),
      parser.make_injected_token(seed, c4c::TokenKind::Semi, ";"),
      parser.make_injected_token(seed, c4c::TokenKind::RBrace, "}"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  c4c::TypeSpec ts = parser.parse_base_type();
  expect_true(ts.base == c4c::TB_STRUCT,
              "struct producer should parse a structured record type");
  expect_true(ts.record_def && ts.record_def->n_fields >= 0,
              "struct producer should attach record_def metadata");
  expect_true(ts.tag_text_id == record_token.text_id,
              "struct producer should attach tag_text_id metadata before rendered tag");
  set_legacy_tag_if_present(ts, nullptr, 0);
  expect_true(ts.record_def && ts.tag_text_id == record_token.text_id,
              "struct metadata should survive absent rendered tag spelling");
}

void test_union_forward_producer_assigns_text_identity_without_record_def() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token union_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "ForwardUnion");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwUnion, "union"),
      union_token,
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "value"),
  });

  c4c::TypeSpec ts = parser.parse_base_type();
  expect_true(ts.base == c4c::TB_UNION,
              "union producer should parse a union type");
  expect_true(ts.record_def == nullptr,
              "forward union producer should not invent a complete record_def");
  expect_true(ts.tag_text_id == union_token.text_id,
              "forward union producer should still attach tag_text_id metadata");
  set_legacy_tag_if_present(ts, nullptr, 0);
  expect_true(ts.tag_text_id == union_token.text_id,
              "forward union identity should not depend on rendered tag spelling");
}

void test_enum_producer_preserves_text_identity_and_underlying_base() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token enum_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "SmallEnum");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwEnum, "enum"),
      enum_token,
      parser.make_injected_token(seed, c4c::TokenKind::Colon, ":"),
      parser.make_injected_token(seed, c4c::TokenKind::KwUnsigned, "unsigned"),
      parser.make_injected_token(seed, c4c::TokenKind::KwChar, "char"),
      parser.make_injected_token(seed, c4c::TokenKind::LBrace, "{"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "SmallA"),
      parser.make_injected_token(seed, c4c::TokenKind::RBrace, "}"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  c4c::TypeSpec ts = parser.parse_base_type();
  expect_true(ts.base == c4c::TB_ENUM,
              "enum producer should parse an enum type");
  expect_true(ts.tag_text_id == enum_token.text_id,
              "enum producer should attach tag_text_id metadata before rendered tag");
  expect_true(ts.enum_underlying_base == c4c::TB_UCHAR,
              "enum producer should preserve fixed underlying type metadata");
  set_legacy_tag_if_present(ts, nullptr, 0);
  expect_true(ts.tag_text_id == enum_token.text_id &&
                  ts.enum_underlying_base == c4c::TB_UCHAR,
              "enum metadata should survive absent rendered tag spelling");
}

void test_struct_template_fallthrough_uses_text_identity() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token pair_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Pair");

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("StalePairPrimary");
  primary->unqualified_name = arena.strdup("Pair");
  primary->unqualified_text_id = pair_token.text_id;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 1;
  primary->template_param_names = arena.alloc_array<const char*>(1);
  primary->template_param_names[0] = arena.strdup("T");
  primary->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  primary->template_param_name_text_ids[0] = texts.intern("T");
  primary->template_param_is_nttp = arena.alloc_array<bool>(1);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_pack = arena.alloc_array<bool>(1);
  primary->template_param_is_pack[0] = false;

  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), pair_token.text_id, primary);
  const c4c::QualifiedNameKey pair_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), pair_token.text_id);
  parser.template_state_.template_struct_defs_by_key[pair_key] = primary;

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwStruct, "struct"),
      pair_token,
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  c4c::TypeSpec ts = parser.parse_base_type();
  expect_true(ts.base == c4c::TB_STRUCT,
              "struct Pair<int> should remain a struct type");
  expect_true(parser.check(c4c::TokenKind::Identifier) &&
                  parser.token_spelling(parser.cur()) == "after",
              "struct Pair<int> fall-through should consume template arguments via structured template identity");
}

}  // namespace

int main() {
  test_builtin_va_list_producer_sets_structured_identity();
  test_template_scope_type_param_producer_uses_text_carrier();
  test_simple_typedef_producer_keeps_carrier_despite_stale_tag();
  test_record_producers_assign_structured_metadata_before_tag();
  test_union_forward_producer_assigns_text_identity_without_record_def();
  test_enum_producer_preserves_text_identity_and_underlying_base();
  test_struct_template_fallthrough_uses_text_identity();
  std::cout << "PASS: cpp_hir_parser_type_base_producer_metadata_test\n";
  return 0;
}
