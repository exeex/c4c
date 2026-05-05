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

void test_type_name_preserves_deferred_owner_through_function_ref_declarator() {
  const char* source =
      "struct T {};\n"
      "struct U {};\n"
      "template <typename T>\n"
      "struct Owner { using type = T; };\n"
      "template <typename X>\n"
      "struct H {\n"
      "  template <typename Y>\n"
      "  struct Rebind { using Type = Y; };\n"
      "};\n";

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
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  expect_true(owner_text != c4c::kInvalidText &&
                  member_text != c4c::kInvalidText,
              "test should intern Owner and type identifiers");

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Owner"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::Amp, "&"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  std::string direct_name;
  c4c::TypeSpec direct_ts{};
  bool direct_has_type = false;
  {
    c4c::Parser direct_parser(
        tokens, arena, &lexer.text_table(), &lexer.file_table(),
        c4c::SourceProfile::CppSubset,
        "cpp_hir_parser_declarator_deferred_owner_metadata_test.cpp");
    (void)direct_parser.parse();
    direct_parser.replace_token_stream_for_testing({
        direct_parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Owner"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
        direct_parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
    });
    expect_true(direct_parser.parse_dependent_typename_specifier(
                    &direct_name, &direct_ts, &direct_has_type),
                "direct dependent typename should parse");
  }
  expect_true(direct_has_type &&
                  direct_ts.deferred_member_type_text_id == member_text,
              "direct dependent typename should preserve deferred member TextId");

  c4c::TypeSpec ts = parser.parse_type_name();
  expect_true(ts.is_fn_ptr && ts.is_lvalue_ref,
              "function-reference type-id should keep declarator shape");
  expect_true(ts.deferred_member_type_text_id == member_text,
              "function-reference type-id should preserve deferred member TextId; got " +
                  std::to_string(ts.deferred_member_type_text_id) +
                  " tag_text_id=" + std::to_string(ts.tag_text_id));
  expect_true(ts.tag_text_id == owner_text ||
                  ts.tpl_struct_origin_key.base_text_id == owner_text ||
                  ts.record_def != nullptr,
              "function-reference type-id should preserve structured owner identity");

  c4c::Parser template_member_parser(
      tokens, arena, &lexer.text_table(), &lexer.file_table(),
      c4c::SourceProfile::CppSubset,
      "cpp_hir_parser_declarator_deferred_owner_metadata_test.cpp");
  (void)template_member_parser.parse();
  const c4c::TextId rebind_text =
      template_member_parser.find_parser_text_id("Rebind");
  const c4c::TextId type_text =
      template_member_parser.find_parser_text_id("Type");
  expect_true(rebind_text != c4c::kInvalidText &&
                  type_text != c4c::kInvalidText,
              "test should intern Rebind and Type identifiers");
  template_member_parser.replace_token_stream_for_testing({
      template_member_parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "H"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "T"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::KwTemplate, "template"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Rebind"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "U"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Type"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Amp, "&"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::LParen, "("),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Arg"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::RParen, ")"),
      template_member_parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });
  c4c::TypeSpec template_member_ts =
      template_member_parser.parse_type_name();
  expect_true(template_member_ts.is_fn_ptr && template_member_ts.is_lvalue_ref,
              "template-member function-reference type-id should keep declarator shape");
  expect_true(template_member_ts.deferred_member_type_text_id == type_text,
              "template-member function-reference type-id should preserve "
              "deferred member TextId");
  expect_true(template_member_ts.tag_text_id != c4c::kInvalidText ||
                  template_member_ts.tpl_struct_origin_key.base_text_id !=
                      c4c::kInvalidText ||
                  template_member_ts.record_def != nullptr,
              "template-member type-id should preserve structured owner identity");
}

}  // namespace

int main() {
  test_deferred_template_owner_prefers_structured_identity_over_stale_tag();
  test_type_name_preserves_deferred_owner_through_function_ref_declarator();
  std::cout << "PASS: "
               "cpp_hir_parser_declarator_deferred_owner_metadata_test\n";
  return 0;
}
