#include "lexer.hpp"
#include "impl/parser_impl.hpp"
#include "impl/types/types_helpers.hpp"
#include "parser.hpp"
#include "sema/validate.hpp"

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

void test_alias_template_lookup_rejects_visible_type_rendered_reentry() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  const c4c::TextId rendered_target_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "RenderedDrift");
  const c4c::QualifiedNameKey rendered_target_key =
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           rendered_target_text);

  c4c::ParserAliasTemplateInfo info;
  info.aliased_type.array_size = -1;
  info.aliased_type.inner_rank = -1;
  info.aliased_type.base = c4c::TB_INT;
  parser.register_alias_template_info_for_testing(rendered_target_key, info);
  parser.register_known_fn_name(rendered_target_key);
  parser.register_using_value_alias_for_testing(
      parser.current_namespace_context_id(), alias_text, rendered_target_key);

  const c4c::ParserAliasTemplateInfo* found =
      parser.find_alias_template_info_in_context(
          parser.current_namespace_context_id(), alias_text);
  expect_true(found == nullptr,
              "alias-template lookup should not recover through a "
              "visible-type rendered spelling");
}

void test_qualified_typedef_name_uses_structured_result_not_rendered_reentry() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "StructuredNs");
  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text);
  expect_true(ns_context > 0, "test namespace context should be created");

  c4c::TypeSpec alias_type{};
  alias_type.array_size = -1;
  alias_type.inner_rank = -1;
  alias_type.base = c4c::TB_INT;
  parser.register_structured_typedef_binding_in_context(ns_context, alias_text,
                                                        alias_type);

  c4c::Parser::QualifiedNameRef qualified;
  qualified.qualifier_segments.push_back("RenderedNsDrift");
  qualified.qualifier_text_ids.push_back(ns_text);
  qualified.base_name = "RenderedAliasDrift";
  qualified.base_text_id = alias_text;

  const std::string rendered = parser.visible_name_spelling(
      parser.resolve_qualified_type(qualified));
  expect_true(!rendered.empty(),
              "test qualified typedef should have a display spelling");
  expect_true(parser.find_parser_text_id(rendered) == c4c::kInvalidText,
              "test should not pre-seed a full rendered typedef TextId");

  const std::string resolved =
      c4c::resolve_qualified_typedef_name(parser, qualified);
  expect_true(resolved == rendered,
              "qualified typedef resolution should trust the structured "
              "qualified result, not a rendered-spelling TextId re-entry");
}

void test_dependent_typename_rejects_visible_type_rendered_reentry() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::TextId alias_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  const c4c::TextId ns_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "StructuredNs");
  const c4c::TextId target_text =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Target");
  const int ns_context = parser.ensure_named_namespace_context(0, ns_text);
  expect_true(ns_context > 0, "test namespace context should be created");

  const c4c::QualifiedNameKey target_key =
      parser.alias_template_key_in_context(ns_context, target_text);
  c4c::ParserAliasTemplateInfo info;
  info.aliased_type.array_size = -1;
  info.aliased_type.inner_rank = -1;
  info.aliased_type.base = c4c::TB_INT;
  parser.register_alias_template_info_for_testing(target_key, info);
  parser.register_known_fn_name(target_key);
  parser.register_using_value_alias_for_testing(
      parser.current_namespace_context_id(), alias_text, target_key);

  c4c::TypeSpec legacy_rendered_type{};
  legacy_rendered_type.array_size = -1;
  legacy_rendered_type.inner_rank = -1;
  legacy_rendered_type.base = c4c::TB_LONG;
  const c4c::TextId rendered_target_text =
      parser.parser_text_id_for_token(c4c::kInvalidText,
                                      "StructuredNs::Target");
  parser.register_typedef_binding(rendered_target_text, legacy_rendered_type,
                                  true);

  const c4c::Parser::VisibleNameResult visible_type =
      parser.resolve_visible_type(alias_text);
  const std::string rendered = parser.visible_name_spelling(visible_type);
  expect_true(rendered == "StructuredNs::Target",
              "test visible type should render a qualified target spelling");
  expect_true(parser.has_typedef_type(parser.find_parser_text_id(rendered)),
              "test should seed the old full-rendered typedef bridge");
  expect_true(parser.find_visible_typedef_type(alias_text) == nullptr,
              "test should not provide direct visible typedef authority for "
              "the alias name");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwTypename, "typename"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
  });

  std::string resolved_name;
  bool has_resolved_type = true;
  expect_true(parser.parse_dependent_typename_specifier(
                  &resolved_name, nullptr, &has_resolved_type),
              "dependent typename spelling should still be consumed");
  expect_true(resolved_name == "Alias",
              "dependent typename parsing should not replace an alias with "
              "visible-type rendered spelling");
  expect_true(!has_resolved_type,
              "legacy full-rendered typedef storage should not provide "
              "structured dependent typename type authority");
}

void test_sema_static_member_lookup_rejects_stale_rendered_owner_reentry() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    return ts;
  };

  const c4c::TextId rendered_owner_text = texts.intern("RenderedOwner");
  const c4c::TextId missing_owner_text = texts.intern("MissingOwner");
  const c4c::TextId stale_text = texts.intern("stale");
  const c4c::TextId missing_text = texts.intern("missing");

  c4c::Node* rendered_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  rendered_owner->name = arena.strdup("RenderedOwner");
  rendered_owner->unqualified_name = arena.strdup("RenderedOwner");
  rendered_owner->unqualified_text_id = rendered_owner_text;
  rendered_owner->namespace_context_id = parser.current_namespace_context_id();
  rendered_owner->n_fields = 1;
  rendered_owner->fields = arena.alloc_array<c4c::Node*>(1);

  c4c::Node* stale = parser.make_node(c4c::NK_DECL, 1);
  stale->name = arena.strdup("stale");
  stale->unqualified_name = arena.strdup("stale");
  stale->unqualified_text_id = stale_text;
  stale->namespace_context_id = rendered_owner->namespace_context_id;
  stale->is_static = true;
  stale->type = make_ts(c4c::TB_INT);
  rendered_owner->fields[0] = stale;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("RenderedOwner::stale");
  ref->unqualified_name = arena.strdup("missing");
  ref->unqualified_text_id = missing_text;
  ref->namespace_context_id = rendered_owner->namespace_context_id;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("MissingOwner");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = missing_owner_text;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("returns_missing");
  fn->unqualified_name = arena.strdup("returns_missing");
  fn->unqualified_text_id = texts.intern("returns_missing");
  fn->namespace_context_id = rendered_owner->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_owner;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema static member lookup should reject stale rendered owner "
              "fallback after structured owner/member miss");
}

void test_sema_global_lookup_rejects_stale_qualified_rendered_reentry() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    return ts;
  };

  const c4c::TextId rendered_ns_text = texts.intern("RenderedNs");
  const c4c::TextId stale_text = texts.intern("stale");
  const c4c::TextId missing_text = texts.intern("missing");

  c4c::Node* stale_global = parser.make_node(c4c::NK_DECL, 1);
  stale_global->name = arena.strdup("RenderedNs::stale");
  stale_global->unqualified_name = arena.strdup("stale");
  stale_global->unqualified_text_id = stale_text;
  stale_global->namespace_context_id = parser.current_namespace_context_id();
  stale_global->type = make_ts(c4c::TB_SHORT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("RenderedNs::stale");
  ref->unqualified_name = arena.strdup("missing");
  ref->unqualified_text_id = missing_text;
  ref->namespace_context_id = stale_global->namespace_context_id;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("RenderedNs");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = rendered_ns_text;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("returns_stale");
  fn->unqualified_name = arena.strdup("returns_stale");
  fn->unqualified_text_id = texts.intern("returns_stale");
  fn->namespace_context_id = stale_global->namespace_context_id;
  fn->type = make_ts(c4c::TB_SHORT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = stale_global;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema global lookup should reject stale qualified rendered "
              "fallback after structured global key miss");
}

}  // namespace

int main() {
  test_global_qualified_lookup_rejects_rendered_fallback_authority();
  test_qualified_known_function_lookup_uses_key_not_rendered_spelling();
  test_alias_template_lookup_rejects_visible_type_rendered_reentry();
  test_qualified_typedef_name_uses_structured_result_not_rendered_reentry();
  test_dependent_typename_rejects_visible_type_rendered_reentry();
  test_sema_static_member_lookup_rejects_stale_rendered_owner_reentry();
  test_sema_global_lookup_rejects_stale_qualified_rendered_reentry();
  std::cout << "PASS: frontend_parser_lookup_authority_tests\n";
  return 0;
}
