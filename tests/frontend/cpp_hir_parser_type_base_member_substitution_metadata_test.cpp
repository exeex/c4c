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
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype((void)(ts.tag = tag)) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

void make_stale_t_param(c4c::Arena& arena, c4c::TextId t_text,
                        c4c::TypeSpec* type) {
  expect_true(type != nullptr, "test bug: type should be present");
  type->base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(*type, arena.strdup("U"), 0);
  type->tag_text_id = c4c::kInvalidText;
  type->template_param_text_id = t_text;
}

void expect_int_type(const c4c::TypeSpec& type, const std::string& msg) {
  expect_true(type.base == c4c::TB_INT, msg);
}

void test_instantiated_members_prefer_structured_param_metadata() {
  const char* source =
      "template <typename T, typename U>\n"
      "struct Box {\n"
      "  using Alias = T;\n"
      "  T field;\n"
      "  T method(T value);\n"
      "};\n";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(
      tokens, arena, &lexer.text_table(), &lexer.file_table(),
      c4c::SourceProfile::CppSubset,
      "cpp_hir_parser_type_base_member_substitution_metadata_test.cpp");
  (void)parser.parse();

  const c4c::TextId box_text = parser.find_parser_text_id("Box");
  const c4c::TextId t_text = parser.find_parser_text_id("T");
  expect_true(box_text != c4c::kInvalidText && t_text != c4c::kInvalidText,
              "test should intern Box and T");

  c4c::Node* box = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), box_text);
  expect_true(box && box->n_member_typedefs == 1 && box->member_typedef_types,
              "Box<T, U> should expose one member typedef");
  expect_true(box->n_fields == 1 && box->fields && box->fields[0],
              "Box<T, U> should expose one field");
  expect_true(box->n_children == 1 && box->children && box->children[0],
              "Box<T, U> should expose one method");
  expect_true(box->children[0]->n_params == 1 && box->children[0]->params &&
                  box->children[0]->params[0],
              "Box<T, U>::method should expose one parameter");

  make_stale_t_param(arena, t_text, &box->member_typedef_types[0]);
  make_stale_t_param(arena, t_text, &box->fields[0]->type);
  make_stale_t_param(arena, t_text, &box->children[0]->type);
  make_stale_t_param(arena, t_text, &box->children[0]->params[0]->type);

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Comma, ","),
      parser.make_injected_token(seed, c4c::TokenKind::KwDouble, "double"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  const c4c::TypeSpec instantiated = parser.parse_type_name();
  const c4c::Node* record = instantiated.record_def;
  expect_true(record && record->template_origin_name &&
                  std::string(record->template_origin_name) == "Box",
              "Box<int, double> should instantiate to a concrete record");

  expect_true(record->n_member_typedefs == 1 && record->member_typedef_types,
              "instantiated Box should retain the member typedef");
  expect_true(record->n_fields == 1 && record->fields && record->fields[0],
              "instantiated Box should retain the field");
  expect_true(record->n_children == 1 && record->children &&
                  record->children[0],
              "instantiated Box should retain the method");
  expect_true(record->children[0]->n_params == 1 &&
                  record->children[0]->params &&
                  record->children[0]->params[0],
              "instantiated Box::method should retain its parameter");

  expect_int_type(record->member_typedef_types[0],
                  "member typedef should bind structured T, not stale tag U");
  expect_int_type(record->fields[0]->type,
                  "field should bind structured T, not stale tag U");
  expect_int_type(record->children[0]->type,
                  "method return should bind structured T, not stale tag U");
  expect_int_type(record->children[0]->params[0]->type,
                  "method parameter should bind structured T, not stale tag U");
}

}  // namespace

int main() {
  test_instantiated_members_prefer_structured_param_metadata();
  std::cout
      << "PASS: cpp_hir_parser_type_base_member_substitution_metadata_test\n";
  return 0;
}
