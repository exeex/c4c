#include "lexer.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

template <typename T>
auto clear_legacy_tag_if_present(T& ts, int)
    -> decltype(ts.tag = nullptr, void()) {
  ts.tag = nullptr;
}

void clear_legacy_tag_if_present(c4c::TypeSpec&, long) {}

void test_instantiated_base_resolves_deferred_member_without_rendered_tag() {
  const char* source =
      "template <typename T>\n"
      "struct Base {};\n"
      "template <typename T>\n"
      "struct Owner { using type = Base<T>; };\n"
      "template <typename T>\n"
      "struct Derived : Owner<T>::type {};\n";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(
      tokens, arena, &lexer.text_table(), &lexer.file_table(),
      c4c::SourceProfile::CppSubset,
      "cpp_hir_parser_type_base_instantiated_deferred_member_metadata_test.cpp");
  (void)parser.parse();

  const c4c::TextId derived_text = parser.find_parser_text_id("Derived");
  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  expect_true(derived_text != c4c::kInvalidText &&
                  owner_text != c4c::kInvalidText &&
                  member_text != c4c::kInvalidText,
              "test should intern Derived, Owner, and type identifiers");

  c4c::Node* derived = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), derived_text);
  c4c::Node* owner = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), owner_text);
  expect_true(derived && derived->n_bases == 1 && derived->base_types,
              "Derived<T> should retain its deferred member base");
  expect_true(owner != nullptr, "Owner<T> primary should be registered");
  c4c::TypeSpec& base_owner = derived->base_types[0];
  expect_true(base_owner.record_def == owner ||
                  base_owner.tag_text_id == owner_text ||
                  base_owner.tpl_struct_origin_key.base_text_id == owner_text,
              "deferred member base should carry structured owner metadata");
  base_owner.deferred_member_type_text_id = member_text;
  clear_legacy_tag_if_present(base_owner, 0);
  base_owner.deferred_member_type_name = arena.strdup("stale_type");

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Derived"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  const c4c::TypeSpec instantiated = parser.parse_type_name();
  expect_true(instantiated.record_def &&
                  instantiated.record_def->template_origin_name &&
                  std::string(instantiated.record_def->template_origin_name) ==
                      "Derived",
              "Derived<int> should instantiate to a concrete record");
  expect_true(instantiated.record_def->n_bases == 1 &&
                  instantiated.record_def->base_types,
              "instantiated Derived<int> should preserve its base");

  const c4c::TypeSpec& base_ts = instantiated.record_def->base_types[0];
  expect_true(base_ts.record_def &&
                  base_ts.record_def->template_origin_name &&
                  std::string(base_ts.record_def->template_origin_name) ==
                      "Base",
              "deferred base should resolve through structured owner/member "
              "metadata without rendered owner tag compatibility");
  expect_true(base_ts.record_def->n_template_args == 1 &&
                  base_ts.record_def->template_arg_types &&
                  base_ts.record_def->template_arg_types[0].base == c4c::TB_INT,
              "resolved Base record should receive the instantiated int argument");
}

}  // namespace

int main() {
  test_instantiated_base_resolves_deferred_member_without_rendered_tag();
  std::cout
      << "PASS: "
         "cpp_hir_parser_type_base_instantiated_deferred_member_metadata_test\n";
  return 0;
}
