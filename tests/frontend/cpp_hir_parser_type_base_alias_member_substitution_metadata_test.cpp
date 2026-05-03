#include "parser.hpp"
#include "lexer.hpp"

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

void test_alias_member_substitution_prefers_template_param_text_id() {
  const char* source =
      "template <typename T, typename U>\n"
      "struct Owner { using type = T; };\n"
      "template <typename T, typename U>\n"
      "using Alias = typename Owner<T, U>::type;\n";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "cpp_hir_parser_type_base_alias_member_substitution_metadata_test.cpp");
  (void)parser.parse();

  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId t_text = parser.find_parser_text_id("T");
  c4c::Node* owner = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), owner_text);
  expect_true(owner && owner->n_member_typedefs == 1 &&
                  owner->member_typedef_types,
              "Owner<T, U>::type should be available for the substitution test");

  c4c::TypeSpec& member_type = owner->member_typedef_types[0];
  expect_true(member_type.base == c4c::TB_TYPEDEF,
              "Owner<T, U>::type should carry a typedef parameter type");
  member_type.tag = arena.strdup("U");
  member_type.tag_text_id = c4c::kInvalidText;
  member_type.template_param_text_id = t_text;

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Comma, ","),
      parser.make_injected_token(seed, c4c::TokenKind::KwDouble, "double"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(
      resolved.base == c4c::TB_INT,
      "alias-template member typedef substitution should bind by structured "
      "template_param_text_id before stale rendered tag text");
}

}  // namespace

int main() {
  test_alias_member_substitution_prefers_template_param_text_id();
  std::cout
      << "PASS: cpp_hir_parser_type_base_alias_member_substitution_metadata_test\n";
  return 0;
}
