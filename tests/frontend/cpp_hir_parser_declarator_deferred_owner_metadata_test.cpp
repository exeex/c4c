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

c4c::QualifiedNameKey parser_test_qualified_name_key(
    c4c::Parser& parser, std::initializer_list<std::string> qualifiers,
    const std::string& base) {
  c4c::Parser::QualifiedNameRef qn;
  for (const std::string& qualifier : qualifiers) {
    qn.qualifier_segments.push_back(qualifier);
    qn.qualifier_text_ids.push_back(parser.find_parser_text_id(qualifier));
  }
  qn.base_name = base;
  qn.base_text_id = parser.find_parser_text_id(base);
  return parser.qualified_name_key(qn);
}

void test_deferred_template_owner_prefers_structured_identity_over_stale_tag() {
  const char* source =
      "template <typename T>\n"
      "struct Owner { using type = int; };\n"
      "template <typename T>\n"
      "struct StaleOwner { using type = long; };\n";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(
      tokens, arena, &lexer.text_table(), &lexer.file_table(),
      c4c::SourceProfile::CppSubset,
      "cpp_hir_parser_declarator_deferred_owner_metadata_test.cpp");
  (void)parser.parse();

  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId stale_owner_text = parser.find_parser_text_id("StaleOwner");
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  expect_true(owner_text != c4c::kInvalidText &&
                  stale_owner_text != c4c::kInvalidText &&
                  member_text != c4c::kInvalidText,
              "test should intern owner and member identifiers");

  c4c::Node* owner = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), owner_text);
  c4c::Node* stale_owner = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), stale_owner_text);
  expect_true(owner != nullptr && stale_owner != nullptr,
              "template primaries should be registered");

  c4c::TypeSpec stale_rendered_owner{};
  stale_rendered_owner.array_size = -1;
  stale_rendered_owner.inner_rank = -1;
  stale_rendered_owner.base = c4c::TB_STRUCT;
  stale_rendered_owner.tag = arena.strdup("StaleOwner");
  stale_rendered_owner.tag_text_id = owner_text;
  stale_rendered_owner.record_def = owner;
  stale_rendered_owner.tpl_struct_origin = arena.strdup("StaleOwner");
  stale_rendered_owner.tpl_struct_origin_key =
      parser_test_qualified_name_key(parser, {}, "Owner");
  parser.register_typedef_binding(owner_text, stale_rendered_owner, true);

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Owner"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  std::string resolved_name;
  c4c::TypeSpec resolved_type{};
  bool has_type = false;
  expect_true(parser.parse_dependent_typename_specifier(
                  &resolved_name, &resolved_type, &has_type),
              "dependent typename spelling should be accepted");
  expect_true(has_type, "deferred template owner should produce a TypeSpec");
  expect_true(resolved_type.deferred_member_type_text_id == member_text,
              "deferred owner handoff should preserve member TextId metadata");
  expect_true(resolved_type.tpl_struct_origin_key.base_text_id == owner_text ||
                  resolved_type.record_def == owner ||
                  resolved_type.tag_text_id == owner_text,
              "structured Owner identity should win over stale rendered owner spelling");
  expect_true(resolved_type.tpl_struct_origin_key.base_text_id !=
                  stale_owner_text,
              "stale rendered template owner should not become structured authority");
}

}  // namespace

int main() {
  test_deferred_template_owner_prefers_structured_identity_over_stale_tag();
  std::cout << "PASS: "
               "cpp_hir_parser_declarator_deferred_owner_metadata_test\n";
  return 0;
}
