#include "lexer.hpp"
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

void test_preliminary_functional_cast_uses_template_param_text_id() {
  c4c::Lexer lexer("Box<unsigned, T(7)> after",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  auto tokens = lexer.scan_all();
  c4c::Arena arena;
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset);

  const c4c::TextId box_text = lexer.text_table().intern("Box");
  const c4c::TextId t_text = lexer.text_table().intern("T");
  const c4c::TextId n_text = lexer.text_table().intern("N");

  c4c::Node* primary = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  primary->name = arena.strdup("Box");
  primary->unqualified_name = arena.strdup("Box");
  primary->unqualified_text_id = box_text;
  primary->namespace_context_id = parser.current_namespace_context_id();
  primary->n_template_params = 2;
  primary->template_param_names = arena.alloc_array<const char*>(2);
  primary->template_param_names[0] = arena.strdup("StaleRenderedT");
  primary->template_param_names[1] = arena.strdup("N");
  primary->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(2);
  primary->template_param_name_text_ids[0] = t_text;
  primary->template_param_name_text_ids[1] = n_text;
  primary->template_param_is_nttp = arena.alloc_array<bool>(2);
  primary->template_param_is_nttp[0] = false;
  primary->template_param_is_nttp[1] = true;
  primary->template_param_is_pack = arena.alloc_array<bool>(2);
  primary->template_param_is_pack[0] = false;
  primary->template_param_is_pack[1] = false;
  primary->template_param_has_default = arena.alloc_array<bool>(2);
  primary->template_param_has_default[0] = false;
  primary->template_param_has_default[1] = false;
  primary->template_param_default_values = arena.alloc_array<long long>(2);
  primary->template_param_default_values[0] = 0;
  primary->template_param_default_values[1] = 0;

  parser.register_template_struct_primary(parser.current_namespace_context_id(),
                                          box_text, primary);
  const c4c::QualifiedNameKey box_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), box_text);
  parser.template_state_.template_struct_defs_by_key[box_key] = primary;

  c4c::TypeSpec box_alias{};
  box_alias.array_size = -1;
  box_alias.inner_rank = -1;
  box_alias.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(box_alias, arena.strdup("Box"), 0);
  box_alias.tag_text_id = box_text;
  parser.register_typedef_binding(box_text, box_alias, true);

  expect_true(std::string(primary->template_param_names[0]) == "StaleRenderedT",
              "test fixture should keep stale rendered parameter spelling");

  (void)parser.parse_base_type();

  c4c::ParserTemplateState::TemplateInstantiationKey expected_key{};
  expected_key.template_key = box_key;
  expected_key.arguments.push_back({false, "uint"});
  expected_key.arguments.push_back({true, "7"});
  expect_true(
      parser.template_state_.instantiated_template_struct_keys_by_key.count(
          expected_key) == 1,
      "functional cast preliminary evaluation should materialize the structured Box<unsigned, 7> instantiation key through template_param_text_id before stale parameter spelling");
}

}  // namespace

int main() {
  test_preliminary_functional_cast_uses_template_param_text_id();
  std::cout << "PASS: cpp_hir_parser_type_base_prelim_eval_metadata_test\n";
  return 0;
}
