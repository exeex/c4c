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

}  // namespace

int main() {
  test_builtin_va_list_producer_sets_structured_identity();
  test_template_scope_type_param_producer_uses_text_carrier();
  test_simple_typedef_producer_keeps_carrier_despite_stale_tag();
  std::cout << "PASS: cpp_hir_parser_type_base_producer_metadata_test\n";
  return 0;
}
