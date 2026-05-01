#include "impl/parser_impl.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void test_global_qualified_lookup_rejects_rendered_fallback_authority() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  c4c::TypeSpec legacy_type{};
  legacy_type.array_size = -1;
  legacy_type.inner_rank = -1;
  legacy_type.base = c4c::TB_INT;

  c4c::TypeSpec legacy_value{};
  legacy_value.array_size = -1;
  legacy_value.inner_rank = -1;
  legacy_value.base = c4c::TB_SHORT;

  const c4c::TextId type_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "LegacyOnlyType");
  const c4c::TextId rendered_type_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "::LegacyOnlyType");
  parser.binding_state_.non_atom_typedef_types[rendered_type_text] =
      legacy_type;

  const c4c::TextId value_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "LegacyOnlyValue");
  const c4c::TextId rendered_value_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "::LegacyOnlyValue");
  parser.binding_state_.var_types_by_text_id[rendered_value_text] =
      legacy_value;

  c4c::Parser::QualifiedNameRef global_qn;
  global_qn.is_global_qualified = true;

  global_qn.base_name = "LegacyOnlyType";
  global_qn.base_text_id = type_text;
  expect_true(!parser.resolve_qualified_type(global_qn),
              "global-qualified type lookup should not manufacture a hit "
              "from rendered fallback storage");
  global_qn.base_text_id = c4c::kInvalidText;
  expect_true(!parser.resolve_qualified_type(global_qn),
              "global-qualified TextId-less type lookup should reject "
              "rendered fallback storage");

  global_qn.base_name = "LegacyOnlyValue";
  global_qn.base_text_id = value_text;
  expect_true(!parser.resolve_qualified_value(global_qn),
              "global-qualified value lookup should not manufacture a hit "
              "from rendered fallback storage");
  global_qn.base_text_id = c4c::kInvalidText;
  expect_true(!parser.resolve_qualified_value(global_qn),
              "global-qualified TextId-less value lookup should reject "
              "rendered fallback storage");
}

void test_qualified_known_function_lookup_uses_key_not_rendered_spelling() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "StructuredNs");
  const c4c::TextId fn_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "structuredFn");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text);
  expect_true(ns_context > 0, "test namespace context should be created");

  const c4c::QualifiedNameKey target_key =
      parser.known_fn_name_key_in_context(ns_context, fn_text);
  expect_true(target_key.base_text_id == fn_text,
              "test known-function key should carry the function TextId");
  parser.register_known_fn_name(target_key);

  c4c::Parser::QualifiedNameRef qualified;
  qualified.qualifier_segments.push_back("RenderedNsDrift");
  qualified.qualifier_text_ids.push_back(ns_text);
  qualified.base_name = "renderedFnDrift";
  qualified.base_text_id = fn_text;

  parser.shared_lookup_state_.attach_text_table(nullptr);

  const c4c::Parser::VisibleNameResult result =
      parser.resolve_qualified_value(qualified);
  expect_true(static_cast<bool>(result),
              "qualified known-function lookup should not depend on "
              "rendered spelling production");
  expect_true(result.key == target_key,
              "qualified known-function lookup should return the structured "
              "registered key");
}

}  // namespace

int main() {
  test_global_qualified_lookup_rejects_rendered_fallback_authority();
  test_qualified_known_function_lookup_uses_key_not_rendered_spelling();
  std::cout << "PASS: frontend_parser_lookup_authority_tests\n";
  return 0;
}
