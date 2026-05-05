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
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

void test_template_arg_ref_rendering_uses_text_id_before_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token box_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Box");
  const c4c::Token real_arg_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "RealArg");
  const c4c::TextId real_arg_text = real_arg_token.text_id;

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
  primary->template_param_is_pack[0] = true;

  parser.register_template_struct_primary(
      parser.current_namespace_context_id(), box_token.text_id, primary);
  const c4c::QualifiedNameKey box_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), box_token.text_id);
  parser.template_state_.template_struct_defs_by_key[box_key] = primary;

  c4c::TypeSpec box_alias{};
  box_alias.array_size = -1;
  box_alias.inner_rank = -1;
  box_alias.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(box_alias, arena.strdup("Box"), 0);
  parser.register_typedef_binding(box_token.text_id, box_alias, true);

  c4c::TypeSpec arg_alias{};
  arg_alias.array_size = -1;
  arg_alias.inner_rank = -1;
  arg_alias.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(arg_alias, arena.strdup("StaleRenderedArg"), 0);
  arg_alias.tag_text_id = real_arg_text;
  parser.register_typedef_binding(real_arg_token.text_id, arg_alias, true);

  parser.replace_token_stream_for_testing({
      box_token,
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      real_arg_token,
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  const c4c::TypeSpec box_ts = parser.parse_base_type();
  expect_true(box_ts.tpl_struct_args.data && box_ts.tpl_struct_args.size == 1,
              "template base type should carry structured argument refs");
  const c4c::TemplateArgRef& arg = box_ts.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Type,
              "template arg should be recorded as a type ref");
  expect_true(arg.type.tag_text_id == real_arg_text,
              "template arg should retain structured TextId identity; got " +
                  std::to_string(arg.type.tag_text_id) + " expected " +
                  std::to_string(real_arg_text) + " debug " +
                  (arg.debug_text ? std::string(arg.debug_text)
                                  : std::string("<null>")));
  expect_true(arg.debug_text && std::string(arg.debug_text) == "struct_RealArg",
              "template arg debug rendering should prefer TextId metadata before stale rendered tag");
}

}  // namespace

int main() {
  test_template_arg_ref_rendering_uses_text_id_before_stale_tag();
  std::cout << "PASS: cpp_hir_parser_type_base_residual_metadata_test\n";
  return 0;
}
