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

void test_template_origin_gate_prefers_text_id_with_deferred_member_arg() {
  const char* source =
      "template <typename T, typename U>\n"
      "struct Owner { using type = T; };\n"
      "template <typename... Args>\n"
      "struct Box {};\n";

  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(
      tokens, arena, &lexer.text_table(), &lexer.file_table(),
      c4c::SourceProfile::CppSubset,
      "cpp_hir_parser_type_base_deferred_member_template_origin_metadata_test.cpp");
  (void)parser.parse();

  const c4c::TextId box_text = parser.find_parser_text_id("Box");
  const c4c::TextId owner_text = parser.find_parser_text_id("Owner");
  const c4c::TextId member_text = parser.find_parser_text_id("type");
  expect_true(box_text != c4c::kInvalidText &&
                  owner_text != c4c::kInvalidText &&
                  member_text != c4c::kInvalidText,
              "test should intern Box, Owner, and type identifiers");

  c4c::Node* box = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), box_text);
  c4c::Node* owner = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), owner_text);
  expect_true(box != nullptr && owner != nullptr,
              "template primaries should be registered");

  const c4c::Token seed = tokens.empty() ? c4c::Token{} : tokens.front();
  const c4c::Token stale_box_token = parser.make_injected_token(
      seed, c4c::TokenKind::Identifier, "StaleRenderedBox");
  c4c::Node* stale_box = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  stale_box->name = arena.strdup("StaleRenderedBox");
  stale_box->unqualified_name = arena.strdup("StaleRenderedBox");
  stale_box->unqualified_text_id = stale_box_token.text_id;
  stale_box->namespace_context_id = parser.current_namespace_context_id();
  stale_box->n_template_params = 1;
  stale_box->template_param_names = arena.alloc_array<const char*>(1);
  stale_box->template_param_names[0] = arena.strdup("Args");
  stale_box->template_param_is_nttp = arena.alloc_array<bool>(1);
  stale_box->template_param_is_nttp[0] = false;
  stale_box->template_param_is_pack = arena.alloc_array<bool>(1);
  stale_box->template_param_is_pack[0] = true;
  stale_box->n_fields = 2;
  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), stale_box_token.text_id,
      stale_box);

  c4c::TypeSpec stale_box_ts{};
  stale_box_ts.array_size = -1;
  stale_box_ts.inner_rank = -1;
  stale_box_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(stale_box_ts, arena.strdup("StaleRenderedBox"), 0);
  stale_box_ts.tag_text_id = box_text;
  stale_box_ts.record_def = box;
  parser.register_typedef_binding(box_text, stale_box_ts, true);

  expect_true(owner->n_member_typedefs == 1 && owner->member_typedef_types,
              "Owner<T, U>::type should be available");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Owner"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Comma, ","),
      parser.make_injected_token(seed, c4c::TokenKind::KwDouble, "double"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "type"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  const c4c::TypeSpec resolved = parser.parse_type_name();
  expect_true(resolved.tpl_struct_args.data &&
                  resolved.tpl_struct_args.size == 1,
              "Box<...> should parse through structured tag_text_id even when "
              "the rendered template spelling is stale");
  expect_true(resolved.tpl_struct_origin_key.base_text_id == box_text,
              "structured Box identity should stay authoritative over the "
              "stale rendered template primary");
  const c4c::TemplateArgRef& arg = resolved.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Type,
              "Box argument should be recorded as a type");
  expect_true(arg.type.base == c4c::TB_INT,
              "deferred Owner<int, double>::type argument should resolve to "
              "int after structured template-origin lookup");
}

}  // namespace

int main() {
  test_template_origin_gate_prefers_text_id_with_deferred_member_arg();
  std::cout << "PASS: "
               "cpp_hir_parser_type_base_deferred_member_template_origin_metadata_test\n";
  return 0;
}
