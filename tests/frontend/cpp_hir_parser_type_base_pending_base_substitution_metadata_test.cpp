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

void test_pending_base_substitution_prefers_template_param_text_id() {
  const char* source =
      "template <typename T>\n"
      "struct Base {};\n"
      "template <typename T>\n"
      "struct Derived : Base<T> {};\n";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "cpp_hir_parser_type_base_pending_base_substitution_metadata_test.cpp");
  (void)parser.parse();

  const c4c::TextId base_text = parser.find_parser_text_id("Base");
  const c4c::TextId derived_text = parser.find_parser_text_id("Derived");
  const c4c::TextId t_text = parser.find_parser_text_id("T");
  expect_true(base_text != c4c::kInvalidText &&
                  derived_text != c4c::kInvalidText &&
                  t_text != c4c::kInvalidText,
              "test should intern template names and T");

  c4c::Node* derived = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), derived_text);
  expect_true(derived && derived->n_bases == 1 && derived->base_types,
              "Derived<T> should carry one pending template base");
  expect_true(derived->template_param_name_text_ids &&
                  derived->template_param_name_text_ids[0] == t_text,
              "Derived<T> should retain structured parameter TextId metadata");
  expect_true(derived->base_types[0].tpl_struct_args.data &&
                  derived->base_types[0].tpl_struct_args.size == 1,
              "Derived<T> base should carry structured Base<T> argument refs");
  const c4c::TemplateArgRef& base_arg =
      derived->base_types[0].tpl_struct_args.data[0];
  expect_true(base_arg.kind == c4c::TemplateArgKind::Type &&
                  base_arg.type.template_param_text_id == t_text,
              "Base<T> argument should carry template_param_text_id metadata");

  derived->template_param_names[0] = arena.strdup("StaleRenderedT");
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
              "instantiated Derived<int> should preserve its base list");
  const c4c::TypeSpec& base_ts = instantiated.record_def->base_types[0];
  expect_true(base_ts.record_def &&
                  base_ts.record_def->template_origin_name &&
                  std::string(base_ts.record_def->template_origin_name) ==
                      "Base",
              "pending Base<T> should materialize Base<int> through structured "
              "template_param_text_id before stale rendered parameter names");
  expect_true(base_ts.record_def->n_template_args == 1 &&
                  base_ts.record_def->template_arg_types &&
                  base_ts.record_def->template_arg_types[0].base == c4c::TB_INT,
              "materialized Base record should receive int as its direct type argument");
}

}  // namespace

int main() {
  test_pending_base_substitution_prefers_template_param_text_id();
  std::cout << "PASS: "
               "cpp_hir_parser_type_base_pending_base_substitution_metadata_test\n";
  return 0;
}
