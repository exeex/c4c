#include "lexer.hpp"
#include "impl/parser_impl.hpp"
#include "impl/types/types_helpers.hpp"
#include "parser.hpp"
#include "sema/consteval.hpp"
#include "sema/type_utils.hpp"
#include "sema/validate.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

c4c::Node* parse_cpp_source(c4c::Arena& arena, c4c::Lexer& lexer) {
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");
  c4c::Node* root = parser.parse();
  expect_true(root != nullptr, "test source should produce a program node");
  return root;
}

const c4c::Node* find_record(const c4c::Node* root, const char* name) {
  if (!root || root->kind != c4c::NK_PROGRAM) return nullptr;
  for (int i = 0; i < root->n_children; ++i) {
    const c4c::Node* child = root->children[i];
    if (child && child->kind == c4c::NK_STRUCT_DEF && child->name &&
        std::string(child->name) == name) {
      return child;
    }
  }
  return nullptr;
}

const c4c::Node* find_field(const c4c::Node* record, const char* name) {
  if (!record) return nullptr;
  for (int i = 0; i < record->n_fields; ++i) {
    const c4c::Node* field = record->fields[i];
    if (field && field->name && std::string(field->name) == name) {
      return field;
    }
  }
  return nullptr;
}

const c4c::Node* find_function(const c4c::Node* root, const char* name) {
  if (!root || root->kind != c4c::NK_PROGRAM) return nullptr;
  for (int i = 0; i < root->n_children; ++i) {
    const c4c::Node* child = root->children[i];
    if (child && child->kind == c4c::NK_FUNCTION && child->name &&
        std::string(child->name) == name) {
      return child;
    }
  }
  return nullptr;
}

c4c::Node* find_function(c4c::Node* root, const char* name) {
  return const_cast<c4c::Node*>(
      find_function(static_cast<const c4c::Node*>(root), name));
}

c4c::Node* find_return_var(c4c::Node* fn) {
  if (!fn || !fn->body || fn->body->kind != c4c::NK_BLOCK) return nullptr;
  for (int i = 0; i < fn->body->n_children; ++i) {
    c4c::Node* child = fn->body->children[i];
    if (child && child->kind == c4c::NK_RETURN &&
        child->left && child->left->kind == c4c::NK_VAR) {
      return child->left;
    }
  }
  return nullptr;
}

c4c::Node* find_local_decl(c4c::Node* fn, const char* name) {
  if (!fn || !fn->body || fn->body->kind != c4c::NK_BLOCK) return nullptr;
  for (int i = 0; i < fn->body->n_children; ++i) {
    c4c::Node* child = fn->body->children[i];
    if (child && child->kind == c4c::NK_DECL && child->name &&
        std::string(child->name) == name) {
      return child;
    }
  }
  return nullptr;
}

c4c::Node* find_return_call_callee(c4c::Node* fn) {
  if (!fn || !fn->body || fn->body->kind != c4c::NK_BLOCK) return nullptr;
  for (int i = 0; i < fn->body->n_children; ++i) {
    c4c::Node* child = fn->body->children[i];
    if (child && child->kind == c4c::NK_RETURN && child->left &&
        child->left->kind == c4c::NK_CALL && child->left->left &&
        child->left->left->kind == c4c::NK_VAR) {
      return child->left->left;
    }
  }
  return nullptr;
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

void test_sema_global_lookup_uses_using_value_alias_target_key() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      "int exported_value;\n"
      "namespace wrap {\n"
      "using ::exported_value;\n"
      "int read_alias() { return wrap::exported_value; }\n"
      "}\n",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "using-value-alias-target.cpp");
  c4c::Node* root = parser.parse();
  c4c::Node* fn = find_function(root, "wrap::read_alias");
  c4c::Node* ref = find_return_var(fn);
  expect_true(ref != nullptr, "test should parse a return reference");
  expect_true(ref->using_value_alias_target_text_id != c4c::kInvalidText,
              "using-value-alias reference should carry target TextId");
  expect_true(ref->using_value_alias_target_namespace_context_id == 0,
              "using ::value alias target should carry global namespace");

  ref->name = arena.strdup("stale_rendered_alias_fallback");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema global lookup should use the structured using-value-alias "
              "target key instead of rendered fallback spelling");
}

void test_sema_function_call_uses_using_value_alias_target_key() {
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

  const c4c::TextId owner_text = texts.intern("Api");
  const c4c::TextId target_text = texts.intern("target");
  const int target_namespace_context = parser.current_namespace_context_id();

  c4c::Node* target = parser.make_node(c4c::NK_FUNCTION, 1);
  target->name = arena.strdup("target");
  target->unqualified_name = arena.strdup("target");
  target->unqualified_text_id = target_text;
  target->namespace_context_id = target_namespace_context;
  target->n_qualifier_segments = 1;
  target->qualifier_segments = arena.alloc_array<const char*>(1);
  target->qualifier_segments[0] = arena.strdup("Api");
  target->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  target->qualifier_text_ids[0] = owner_text;
  target->type = make_ts(c4c::TB_INT);
  target->type.is_lvalue_ref = true;

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 2);
  callee->name = arena.strdup("target");
  callee->unqualified_name = arena.strdup("target");
  callee->unqualified_text_id = target_text;
  callee->namespace_context_id = parser.current_namespace_context_id();
  callee->using_value_alias_target_text_id = target_text;
  callee->using_value_alias_target_namespace_context_id = 0;
  callee->n_using_value_alias_target_qualifier_segments = 1;
  callee->using_value_alias_target_qualifier_text_ids =
      arena.alloc_array<c4c::TextId>(1);
  callee->using_value_alias_target_qualifier_text_ids[0] = owner_text;

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 2);
  call->left = callee;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = call;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* caller = parser.make_node(c4c::NK_FUNCTION, 2);
  caller->name = arena.strdup("caller");
  caller->unqualified_name = arena.strdup("caller");
  caller->unqualified_text_id = texts.intern("caller");
  caller->namespace_context_id = parser.current_namespace_context_id();
  caller->type = make_ts(c4c::TB_INT);
  caller->type.is_lvalue_ref = true;
  caller->body = body;

  c4c::Node* root = parser.make_node(c4c::NK_PROGRAM, 1);
  root->n_children = 2;
  root->children = arena.alloc_array<c4c::Node*>(2);
  root->children[0] = target;
  root->children[1] = caller;

  expect_true(callee->using_value_alias_target_text_id != c4c::kInvalidText,
              "using-function call should carry target TextId");
  expect_true(callee->n_using_value_alias_target_qualifier_segments == 1,
              "using-function call should carry target qualifier metadata");

  callee->name = arena.strdup("stale_rendered_function_alias");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema function-call lookup should use the structured "
              "using-value-alias target key instead of rendered callee "
              "spelling" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
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

void test_namespace_qualified_function_decl_handoff_uses_context_metadata() {
  const char* source =
      "namespace Api {}\n"
      "int Api::target() { return 7; }\n"
      "int caller() { return Api::target(); }\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  c4c::Node* target = find_function(root, "Api::target");
  c4c::Node* caller = find_function(root, "caller");
  c4c::Node* callee = find_return_call_callee(caller);
  expect_true(target != nullptr, "parsed namespace function declaration exists");
  expect_true(callee != nullptr, "parsed namespace function call exists");
  expect_true(target->namespace_context_id > 0,
              "namespace function declaration should carry namespace context");
  expect_true(target->unqualified_text_id != c4c::kInvalidText,
              "namespace function declaration should carry base TextId");
  expect_true(target->unqualified_name &&
                  std::string(target->unqualified_name) == "target",
              "namespace function declaration should keep base name separate "
              "from rendered spelling");
  expect_true(callee->namespace_context_id == target->namespace_context_id,
              "namespace function call should carry the resolved context");
  expect_true(callee->unqualified_text_id == target->unqualified_text_id,
              "namespace function call and declaration should share base "
              "TextId metadata");

  target->name = arena.strdup("stale_decl_rendered_name");
  callee->name = arena.strdup("stale_call_rendered_name");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema function lookup should use namespace/base TextId metadata "
              "instead of rendered qualified spelling" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
}

void test_out_of_class_method_owner_handoff_uses_qualifier_metadata() {
  const char* source =
      "struct Owner {\n"
      "  int x;\n"
      "  int method();\n"
      "};\n"
      "int Owner::method() { return x; }\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  const c4c::Node* owner = find_record(root, "Owner");
  c4c::Node* method = find_function(root, "Owner::method");
  expect_true(owner != nullptr, "parsed Owner record should exist");
  expect_true(method != nullptr, "parsed out-of-class method should exist");
  expect_true(method->namespace_context_id == owner->namespace_context_id,
              "out-of-class method should carry owner namespace context");
  expect_true(method->n_qualifier_segments == 1 && method->qualifier_text_ids &&
                  method->qualifier_text_ids[0] == owner->unqualified_text_id,
              "out-of-class method should carry owner qualifier TextId");
  expect_true(method->unqualified_text_id != c4c::kInvalidText,
              "out-of-class method should carry method base TextId");

  method->name = arena.strdup("rendered_method_without_owner");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema method owner lookup should use parser qualifier metadata "
              "instead of rendered owner spelling" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
}

void test_parsed_local_var_ref_handoff_uses_text_id_not_rendered_spelling() {
  const char* source =
      "int uses_local() {\n"
      "  int value = 3;\n"
      "  return value;\n"
      "}\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  c4c::Node* fn = find_function(root, "uses_local");
  c4c::Node* decl = find_local_decl(fn, "value");
  c4c::Node* ref = find_return_var(fn);
  expect_true(decl != nullptr, "parsed local declaration exists");
  expect_true(ref != nullptr, "parsed local return reference exists");
  expect_true(decl->unqualified_text_id != c4c::kInvalidText,
              "local declaration should carry parser TextId metadata");
  expect_true(ref->unqualified_text_id == decl->unqualified_text_id,
              "local reference should preserve the declaration TextId identity");
  expect_true(ref->unqualified_name && std::string(ref->unqualified_name) == "value",
              "local reference should keep base spelling separate from rendered name");

  ref->name = arena.strdup("stale_rendered_local_name");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema local var-ref lookup should use parser TextId metadata "
              "instead of rendered Node::name spelling" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
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
  const c4c::TextId structured_owner_text = texts.intern("StructuredOwner");
  const c4c::TextId stale_text = texts.intern("stale");

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

  c4c::Node* structured_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  structured_owner->name = arena.strdup("StructuredOwner");
  structured_owner->unqualified_name = arena.strdup("StructuredOwner");
  structured_owner->unqualified_text_id = structured_owner_text;
  structured_owner->namespace_context_id = rendered_owner->namespace_context_id;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("RenderedOwner::stale");
  ref->unqualified_name = arena.strdup("stale");
  ref->unqualified_text_id = stale_text;
  ref->namespace_context_id = rendered_owner->namespace_context_id;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("StructuredOwner");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = structured_owner_text;

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
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = rendered_owner;
  program->children[1] = structured_owner;
  program->children[2] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema static member lookup should reject stale rendered owner "
              "fallback after structured owner/member miss");
}

void test_sema_template_static_member_lookup_rejects_stale_rendered_owner_reentry() {
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
  const c4c::TextId structured_owner_text = texts.intern("StructuredOwner<int>");
  const c4c::TextId stale_text = texts.intern("stale");

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

  c4c::Node* structured_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  structured_owner->name = arena.strdup("StructuredOwner<int>");
  structured_owner->unqualified_name = arena.strdup("StructuredOwner<int>");
  structured_owner->unqualified_text_id = structured_owner_text;
  structured_owner->namespace_context_id = rendered_owner->namespace_context_id;
  structured_owner->template_origin_name = arena.strdup("StructuredOwner");
  structured_owner->n_fields = 1;
  structured_owner->fields = arena.alloc_array<c4c::Node*>(1);
  c4c::Node* other = parser.make_node(c4c::NK_DECL, 1);
  other->name = arena.strdup("other");
  other->unqualified_name = arena.strdup("other");
  other->unqualified_text_id = texts.intern("other");
  other->namespace_context_id = structured_owner->namespace_context_id;
  other->is_static = true;
  other->type = make_ts(c4c::TB_INT);
  structured_owner->fields[0] = other;
  structured_owner->n_template_args = 1;
  structured_owner->template_arg_types = arena.alloc_array<c4c::TypeSpec>(1);
  structured_owner->template_arg_types[0] = make_ts(c4c::TB_INT);
  structured_owner->template_arg_is_value = arena.alloc_array<bool>(1);
  structured_owner->template_arg_is_value[0] = false;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("RenderedOwner::stale");
  ref->unqualified_name = arena.strdup("stale");
  ref->unqualified_text_id = stale_text;
  ref->namespace_context_id = rendered_owner->namespace_context_id;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("StructuredOwner<int>");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = structured_owner_text;
  ref->has_template_args = true;
  ref->n_template_args = 1;
  ref->template_arg_types = arena.alloc_array<c4c::TypeSpec>(1);
  ref->template_arg_types[0] = make_ts(c4c::TB_INT);
  ref->template_arg_is_value = arena.alloc_array<bool>(1);
  ref->template_arg_is_value[0] = false;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("returns_missing_template_member");
  fn->unqualified_name = arena.strdup("returns_missing_template_member");
  fn->unqualified_text_id = texts.intern("returns_missing_template_member");
  fn->namespace_context_id = rendered_owner->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = rendered_owner;
  program->children[1] = structured_owner;
  program->children[2] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema template static-member lookup should reject stale rendered "
              "owner fallback after structured owner/member miss");
}

void test_sema_template_static_member_lookup_rejects_rendered_after_metadata_miss() {
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

  c4c::Node* rendered_only_owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  rendered_only_owner->name = arena.strdup("RenderedOwner<int>");
  rendered_only_owner->namespace_context_id = parser.current_namespace_context_id();
  rendered_only_owner->n_fields = 1;
  rendered_only_owner->fields = arena.alloc_array<c4c::Node*>(1);

  c4c::Node* stale = parser.make_node(c4c::NK_DECL, 1);
  stale->name = arena.strdup("stale");
  stale->namespace_context_id = rendered_only_owner->namespace_context_id;
  stale->is_static = true;
  stale->type = make_ts(c4c::TB_INT);
  rendered_only_owner->fields[0] = stale;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("RenderedOwner<int>::stale");
  ref->namespace_context_id = rendered_only_owner->namespace_context_id;
  ref->has_template_args = true;
  ref->n_template_args = 1;
  ref->template_arg_types = arena.alloc_array<c4c::TypeSpec>(1);
  ref->template_arg_types[0] = make_ts(c4c::TB_INT);
  ref->template_arg_is_value = arena.alloc_array<bool>(1);
  ref->template_arg_is_value[0] = false;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_rendered_template_member");
  fn->unqualified_name = arena.strdup("rejects_rendered_template_member");
  fn->unqualified_text_id = texts.intern("rejects_rendered_template_member");
  fn->namespace_context_id = rendered_only_owner->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_only_owner;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema template static-member lookup should reject rendered "
              "owner acceptance when structured owner/member metadata is absent");
}

void test_sema_static_member_lookup_rejects_rendered_after_metadata_miss() {
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
  const c4c::TextId stale_text = texts.intern("stale");

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
  ref->unqualified_name = arena.strdup("stale");
  ref->unqualified_text_id = stale_text;
  ref->namespace_context_id = rendered_owner->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_rendered_member");
  fn->unqualified_name = arena.strdup("rejects_rendered_member");
  fn->unqualified_text_id = texts.intern("rejects_rendered_member");
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
              "Sema static-member lookup should reject rendered owner "
              "acceptance when structured owner metadata is absent");
}

void test_parsed_static_member_lookup_uses_qualifier_metadata_after_rendered_tag_drift() {
  const char* source =
      "struct Owner {\n"
      "  static int value;\n"
      "};\n"
      "int read() { return Owner::value; }\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  const c4c::Node* owner = find_record(root, "Owner");
  const c4c::Node* field = find_field(owner, "value");
  c4c::Node* reader = find_function(root, "read");
  c4c::Node* ref = find_return_var(reader);
  expect_true(owner != nullptr, "parsed Owner record should exist");
  expect_true(field != nullptr && field->is_static,
              "parsed static member should exist");
  expect_true(ref != nullptr, "parsed static member reference should exist");
  expect_true(owner->unqualified_text_id != c4c::kInvalidText,
              "parsed static owner should carry owner TextId");
  expect_true(field->unqualified_text_id != c4c::kInvalidText,
              "parsed static member should carry member TextId");
  expect_true(ref->namespace_context_id == owner->namespace_context_id,
              "static member reference should carry owner namespace context");
  expect_true(ref->n_qualifier_segments == 1 && ref->qualifier_text_ids &&
                  ref->qualifier_text_ids[0] == owner->unqualified_text_id,
              "static member reference should carry owner qualifier TextId");
  expect_true(ref->unqualified_text_id == field->unqualified_text_id,
              "static member reference should carry member TextId");

  ref->name = arena.strdup("RenderedOwnerDrift::rendered_value");
  ref->unqualified_name = arena.strdup("rendered_value");
  ref->qualifier_segments[0] = arena.strdup("RenderedOwnerDrift");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema static member lookup should use parser qualifier/member "
              "metadata instead of rendered owner/tag spelling" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
}

void test_sema_instance_field_lookup_rejects_stale_member_spelling() {
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

  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId stale_text = texts.intern("stale");
  const c4c::TextId missing_text = texts.intern("missing");
  const c4c::TextId method_text = texts.intern("method");

  c4c::Node* owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  owner->name = arena.strdup("Owner");
  owner->unqualified_name = arena.strdup("Owner");
  owner->unqualified_text_id = owner_text;
  owner->namespace_context_id = parser.current_namespace_context_id();
  owner->n_fields = 1;
  owner->fields = arena.alloc_array<c4c::Node*>(1);

  c4c::Node* stale = parser.make_node(c4c::NK_DECL, 1);
  stale->name = arena.strdup("stale");
  stale->unqualified_name = arena.strdup("stale");
  stale->unqualified_text_id = stale_text;
  stale->namespace_context_id = owner->namespace_context_id;
  stale->type = make_ts(c4c::TB_INT);
  owner->fields[0] = stale;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("stale");
  ref->unqualified_name = arena.strdup("stale");
  ref->unqualified_text_id = missing_text;
  ref->namespace_context_id = owner->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* method = parser.make_node(c4c::NK_FUNCTION, 2);
  method->name = arena.strdup("Owner::method");
  method->unqualified_name = arena.strdup("method");
  method->unqualified_text_id = method_text;
  method->namespace_context_id = owner->namespace_context_id;
  method->n_qualifier_segments = 1;
  method->qualifier_segments = arena.alloc_array<const char*>(1);
  method->qualifier_segments[0] = arena.strdup("Owner");
  method->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  method->qualifier_text_ids[0] = owner_text;
  method->type = make_ts(c4c::TB_INT);
  method->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = owner;
  program->children[1] = method;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema instance field lookup should reject stale rendered member "
              "spelling after structured member TextId miss");
}

void test_parsed_record_fields_carry_member_text_ids_into_sema() {
  const char* source =
      "struct Owner {\n"
      "  int x, y;\n"
      "  int (*callback)(int);\n"
      "  enum Kind { A } kind;\n"
      "  int method() { return x + y; }\n"
      "};\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  const c4c::Node* owner = find_record(root, "Owner");
  expect_true(owner != nullptr, "parsed Owner record should exist");
  for (const char* name : {"x", "y", "callback", "kind"}) {
    const c4c::Node* field = find_field(owner, name);
    expect_true(field != nullptr,
                std::string("parsed field should exist: ") + name);
    expect_true(field->unqualified_text_id != c4c::kInvalidText,
                std::string("parsed field should carry TextId: ") + name);
    expect_true(field->unqualified_name && std::string(field->unqualified_name) == name,
                std::string("parsed field should carry unqualified name: ") + name);
  }

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "implicit method-body field lookup should validate through "
              "structured field TextId metadata");
}

void test_parsed_nested_record_fields_carry_member_text_ids() {
  const char* source =
      "struct Outer {\n"
      "  struct Inner { int value; } inner, other;\n"
      "};\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  const c4c::Node* outer = find_record(root, "Outer");
  expect_true(outer != nullptr, "parsed Outer record should exist");
  for (const char* name : {"inner", "other"}) {
    const c4c::Node* field = find_field(outer, name);
    expect_true(field != nullptr,
                std::string("parsed nested-record field should exist: ") + name);
    expect_true(field->unqualified_text_id != c4c::kInvalidText,
                std::string("parsed nested-record field should carry TextId: ") +
                    name);
  }
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

void test_sema_global_lookup_rejects_same_member_wrong_owner_reentry() {
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

  const c4c::TextId owner_a_text = texts.intern("OwnerA");
  const c4c::TextId owner_b_text = texts.intern("OwnerB");
  const c4c::TextId member_text = texts.intern("same_member");
  const int namespace_context = parser.current_namespace_context_id();

  c4c::Node* rendered_global = parser.make_node(c4c::NK_GLOBAL_VAR, 1);
  rendered_global->name = arena.strdup("OwnerA::same_member");
  rendered_global->unqualified_name = arena.strdup("same_member");
  rendered_global->unqualified_text_id = member_text;
  rendered_global->namespace_context_id = namespace_context;
  rendered_global->n_qualifier_segments = 1;
  rendered_global->qualifier_segments = arena.alloc_array<const char*>(1);
  rendered_global->qualifier_segments[0] = arena.strdup("OwnerA");
  rendered_global->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  rendered_global->qualifier_text_ids[0] = owner_a_text;
  rendered_global->type = make_ts(c4c::TB_INT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("OwnerA::same_member");
  ref->unqualified_name = arena.strdup("same_member");
  ref->unqualified_text_id = member_text;
  ref->namespace_context_id = namespace_context;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("OwnerB");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = owner_b_text;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_wrong_owner_global");
  fn->unqualified_name = arena.strdup("rejects_wrong_owner_global");
  fn->unqualified_text_id = texts.intern("rejects_wrong_owner_global");
  fn->namespace_context_id = namespace_context;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_global;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema global lookup should reject same-member rendered fallback "
              "when structured owner metadata names a different owner");
}

void test_sema_global_lookup_rejects_unqualified_rendered_same_member_reentry() {
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

  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId member_text = texts.intern("same_member");
  const int namespace_context = parser.current_namespace_context_id();

  c4c::Node* rendered_global = parser.make_node(c4c::NK_GLOBAL_VAR, 1);
  rendered_global->name = arena.strdup("same_member");
  rendered_global->namespace_context_id = namespace_context;
  rendered_global->type = make_ts(c4c::TB_INT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("same_member");
  ref->unqualified_name = arena.strdup("same_member");
  ref->unqualified_text_id = member_text;
  ref->namespace_context_id = namespace_context;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("Owner");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = owner_text;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_unqualified_same_member_global");
  fn->unqualified_name = arena.strdup("rejects_unqualified_same_member_global");
  fn->unqualified_text_id =
      texts.intern("rejects_unqualified_same_member_global");
  fn->namespace_context_id = namespace_context;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_global;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema global lookup should reject unqualified rendered fallback "
              "when a qualified reference carries owner metadata");
}

void test_sema_function_lookup_rejects_same_member_wrong_owner_reentry() {
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

  const c4c::TextId owner_a_text = texts.intern("OwnerA");
  const c4c::TextId owner_b_text = texts.intern("OwnerB");
  const c4c::TextId member_text = texts.intern("same_function");
  const int namespace_context = parser.current_namespace_context_id();

  c4c::Node* rendered_function = parser.make_node(c4c::NK_FUNCTION, 1);
  rendered_function->name = arena.strdup("same_function");
  rendered_function->unqualified_name = arena.strdup("same_function");
  rendered_function->unqualified_text_id = member_text;
  rendered_function->namespace_context_id = namespace_context;
  rendered_function->n_qualifier_segments = 1;
  rendered_function->qualifier_segments = arena.alloc_array<const char*>(1);
  rendered_function->qualifier_segments[0] = arena.strdup("OwnerA");
  rendered_function->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  rendered_function->qualifier_text_ids[0] = owner_a_text;
  rendered_function->type = make_ts(c4c::TB_INT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("same_function");
  ref->unqualified_name = arena.strdup("same_function");
  ref->unqualified_text_id = member_text;
  ref->namespace_context_id = namespace_context;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("OwnerB");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = owner_b_text;

  c4c::Node* expr = parser.make_node(c4c::NK_EXPR_STMT, 2);
  expr->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = expr;

  c4c::Node* caller = parser.make_node(c4c::NK_FUNCTION, 2);
  caller->name = arena.strdup("rejects_wrong_owner_function");
  caller->unqualified_name = arena.strdup("rejects_wrong_owner_function");
  caller->unqualified_text_id = texts.intern("rejects_wrong_owner_function");
  caller->namespace_context_id = namespace_context;
  caller->type = make_ts(c4c::TB_VOID);
  caller->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_function;
  program->children[1] = caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema function lookup should reject same-member rendered "
              "fallback when structured owner metadata names a different owner");
}

void test_sema_ref_overload_lookup_rejects_same_member_wrong_owner_reentry() {
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

  const c4c::TextId owner_a_text = texts.intern("OwnerA");
  const c4c::TextId owner_b_text = texts.intern("OwnerB");
  const c4c::TextId member_text = texts.intern("same_overload");
  const c4c::TextId param_text = texts.intern("value");
  const int namespace_context = parser.current_namespace_context_id();

  auto make_overload = [&](bool rvalue_ref) {
    c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
    fn->name = arena.strdup("same_overload");
    fn->unqualified_name = arena.strdup("same_overload");
    fn->unqualified_text_id = member_text;
    fn->namespace_context_id = namespace_context;
    fn->n_qualifier_segments = 1;
    fn->qualifier_segments = arena.alloc_array<const char*>(1);
    fn->qualifier_segments[0] = arena.strdup("OwnerA");
    fn->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
    fn->qualifier_text_ids[0] = owner_a_text;
    fn->type = make_ts(c4c::TB_INT);
    fn->n_params = 1;
    fn->params = arena.alloc_array<c4c::Node*>(1);
    c4c::Node* param = parser.make_node(c4c::NK_DECL, 1);
    param->name = arena.strdup("value");
    param->unqualified_name = arena.strdup("value");
    param->unqualified_text_id = param_text;
    param->namespace_context_id = namespace_context;
    param->type = make_ts(c4c::TB_INT);
    param->type.is_lvalue_ref = !rvalue_ref;
    param->type.is_rvalue_ref = rvalue_ref;
    fn->params[0] = param;
    return fn;
  };

  c4c::Node* lvalue_overload = make_overload(false);
  c4c::Node* rvalue_overload = make_overload(true);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("same_overload");
  ref->unqualified_name = arena.strdup("same_overload");
  ref->unqualified_text_id = member_text;
  ref->namespace_context_id = namespace_context;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("OwnerB");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = owner_b_text;

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 2);
  call->left = ref;

  c4c::Node* expr = parser.make_node(c4c::NK_EXPR_STMT, 2);
  expr->left = call;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = expr;

  c4c::Node* caller = parser.make_node(c4c::NK_FUNCTION, 2);
  caller->name = arena.strdup("rejects_wrong_owner_overload");
  caller->unqualified_name = arena.strdup("rejects_wrong_owner_overload");
  caller->unqualified_text_id = texts.intern("rejects_wrong_owner_overload");
  caller->namespace_context_id = namespace_context;
  caller->type = make_ts(c4c::TB_VOID);
  caller->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = lvalue_overload;
  program->children[1] = rvalue_overload;
  program->children[2] = caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(result.ok,
              "Sema ref-overload lookup should reject the wrong-owner "
              "rendered overload set and leave the stale call non-diagnostic");
}

void test_sema_function_lookup_rejects_legacy_rendered_after_structured_miss() {
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

  c4c::Node* legacy_function = parser.make_node(c4c::NK_FUNCTION, 1);
  legacy_function->name = arena.strdup("legacy_rendered_function");
  legacy_function->type = make_ts(c4c::TB_INT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("legacy_rendered_function");
  ref->unqualified_name = arena.strdup("missing_function");
  ref->unqualified_text_id = texts.intern("missing_function");
  ref->namespace_context_id = parser.current_namespace_context_id();

  c4c::Node* expr = parser.make_node(c4c::NK_EXPR_STMT, 2);
  expr->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = expr;

  c4c::Node* caller = parser.make_node(c4c::NK_FUNCTION, 2);
  caller->name = arena.strdup("rejects_legacy_function_fallback");
  caller->unqualified_name = arena.strdup("rejects_legacy_function_fallback");
  caller->unqualified_text_id =
      texts.intern("rejects_legacy_function_fallback");
  caller->namespace_context_id = parser.current_namespace_context_id();
  caller->type = make_ts(c4c::TB_VOID);
  caller->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = legacy_function;
  program->children[1] = caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema function lookup should not recover through a legacy "
              "rendered function spelling after structured key miss");
}

void test_sema_ref_overload_lookup_rejects_legacy_rendered_after_structured_miss() {
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

  const c4c::TextId param_text = texts.intern("value");
  const int namespace_context = parser.current_namespace_context_id();

  auto make_overload = [&](bool rvalue_ref) {
    c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
    fn->name = arena.strdup("legacy_rendered_ref_overload");
    fn->type = make_ts(c4c::TB_INT);
    fn->n_params = 1;
    fn->params = arena.alloc_array<c4c::Node*>(1);
    c4c::Node* param = parser.make_node(c4c::NK_DECL, 1);
    param->name = arena.strdup("value");
    param->unqualified_name = arena.strdup("value");
    param->unqualified_text_id = param_text;
    param->namespace_context_id = namespace_context;
    param->type = make_ts(c4c::TB_INT);
    param->type.is_lvalue_ref = !rvalue_ref;
    param->type.is_rvalue_ref = rvalue_ref;
    fn->params[0] = param;
    return fn;
  };

  c4c::Node* lvalue_overload = make_overload(false);
  c4c::Node* rvalue_overload = make_overload(true);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("legacy_rendered_ref_overload");
  ref->unqualified_name = arena.strdup("missing_ref_overload");
  ref->unqualified_text_id = texts.intern("missing_ref_overload");
  ref->namespace_context_id = namespace_context;

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 2);
  call->left = ref;

  c4c::Node* expr = parser.make_node(c4c::NK_EXPR_STMT, 2);
  expr->left = call;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = expr;

  c4c::Node* caller = parser.make_node(c4c::NK_FUNCTION, 2);
  caller->name = arena.strdup("rejects_legacy_ref_overload_fallback");
  caller->unqualified_name = arena.strdup("rejects_legacy_ref_overload_fallback");
  caller->unqualified_text_id =
      texts.intern("rejects_legacy_ref_overload_fallback");
  caller->namespace_context_id = namespace_context;
  caller->type = make_ts(c4c::TB_VOID);
  caller->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = lvalue_overload;
  program->children[1] = rvalue_overload;
  program->children[2] = caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(result.ok,
              "Sema ref-overload lookup should reject the legacy rendered "
              "overload set after structured key miss and leave the stale call "
              "non-diagnostic");
}

void test_sema_cpp_overload_lookup_rejects_legacy_rendered_after_structured_miss() {
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

  const int namespace_context = parser.current_namespace_context_id();

  auto make_overload = [&](c4c::TypeBase param_base) {
    c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
    fn->name = arena.strdup("operator_plus");
    fn->type = make_ts(c4c::TB_INT);
    fn->n_params = 1;
    fn->params = arena.alloc_array<c4c::Node*>(1);
    c4c::Node* param = parser.make_node(c4c::NK_DECL, 1);
    param->name = arena.strdup("value");
    param->unqualified_name = arena.strdup("value");
    param->unqualified_text_id = texts.intern("value");
    param->namespace_context_id = namespace_context;
    param->type = make_ts(param_base);
    fn->params[0] = param;
    return fn;
  };

  c4c::Node* int_overload = make_overload(c4c::TB_INT);
  c4c::Node* short_overload = make_overload(c4c::TB_SHORT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("operator_plus");
  ref->unqualified_name = arena.strdup("missing_cpp_overload");
  ref->unqualified_text_id = texts.intern("missing_cpp_overload");
  ref->namespace_context_id = namespace_context;

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 2);
  call->left = ref;

  c4c::Node* expr = parser.make_node(c4c::NK_EXPR_STMT, 2);
  expr->left = call;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = expr;

  c4c::Node* caller = parser.make_node(c4c::NK_FUNCTION, 2);
  caller->name = arena.strdup("rejects_legacy_cpp_overload_fallback");
  caller->unqualified_name = arena.strdup("rejects_legacy_cpp_overload_fallback");
  caller->unqualified_text_id =
      texts.intern("rejects_legacy_cpp_overload_fallback");
  caller->namespace_context_id = namespace_context;
  caller->type = make_ts(c4c::TB_VOID);
  caller->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = int_overload;
  program->children[1] = short_overload;
  program->children[2] = caller;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(result.ok,
              "Sema C++ overload lookup should reject the legacy rendered "
              "overload set after structured key miss and leave the stale call "
              "non-diagnostic");
}

void test_sema_namespace_ordinary_cpp_overload_decls_use_structured_set() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      R"cpp(
        namespace eastl {
          void AssertionFailure(const char*);
          void AssertionFailure(void*, const char*);
          void ZeroArg();
          void ZeroArg(int);
        }
      )cpp",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));

  c4c::Node* root = parse_cpp_source(arena, lexer);
  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema should accept ordinary namespace C++ overload "
              "declarations through the structured overload set");
}

void test_sema_cpp_overload_decls_reject_same_params_different_return() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      R"cpp(
        namespace eastl {
          int SameParams(int);
          void SameParams(int);
        }
      )cpp",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));

  c4c::Node* root = parse_cpp_source(arena, lexer);
  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(!result.ok,
              "Sema should keep same-parameter different-return declarations "
              "as conflicting types, not a C++ overload set");
}

void test_sema_cpp_overload_decls_reject_c_language_linkage() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      R"cpp(
        extern "C" {
          void c_linkage(int);
          void c_linkage(short);
        }
      )cpp",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));

  c4c::Node* root = parse_cpp_source(arena, lexer);
  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(!result.ok,
              "Sema should not turn extern C declarations into a C++ "
              "overload set");
}

void test_sema_func_local_lookup_rejects_rendered_after_metadata_miss() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base, int ptr_level = 0) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    ts.ptr_level = ptr_level;
    return ts;
  };

  const c4c::TextId func_text = texts.intern("__func__");
  const c4c::TextId missing_text = texts.intern("missing_func");

  c4c::Node* canonical_ref = parser.make_node(c4c::NK_VAR, 2);
  canonical_ref->name = arena.strdup("__func__");
  canonical_ref->unqualified_name = arena.strdup("__func__");
  canonical_ref->unqualified_text_id = func_text;

  c4c::Node* canonical_ret = parser.make_node(c4c::NK_RETURN, 2);
  canonical_ret->left = canonical_ref;

  c4c::Node* stale_ref = parser.make_node(c4c::NK_VAR, 3);
  stale_ref->name = arena.strdup("__func__");
  stale_ref->unqualified_name = arena.strdup("__func__");
  stale_ref->unqualified_text_id = missing_text;

  c4c::Node* stale_ret = parser.make_node(c4c::NK_RETURN, 3);
  stale_ret->left = stale_ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 2;
  body->children = arena.alloc_array<c4c::Node*>(2);
  body->children[0] = canonical_ret;
  body->children[1] = stale_ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 1);
  fn->name = arena.strdup("f");
  fn->unqualified_name = arena.strdup("f");
  fn->unqualified_text_id = texts.intern("f");
  fn->namespace_context_id = parser.current_namespace_context_id();
  fn->type = make_ts(c4c::TB_CHAR, 1);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 1;
  program->children = arena.alloc_array<c4c::Node*>(1);
  program->children[0] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema __func__ lookup should reject stale rendered local "
              "fallback after structured local key miss");
}

void test_sema_consteval_lookup_uses_qualified_key_not_rendered_spelling() {
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

  auto make_consteval_function = [&](const char* owner, long long value,
                                     int line) {
    const c4c::TextId owner_text = texts.intern(owner);
    const c4c::TextId value_text = texts.intern("value");
    const std::string rendered = std::string(owner) + "::value";

    c4c::Node* lit = parser.make_node(c4c::NK_INT_LIT, line);
    lit->ival = value;

    c4c::Node* ret = parser.make_node(c4c::NK_RETURN, line);
    ret->left = lit;

    c4c::Node* body = parser.make_node(c4c::NK_BLOCK, line);
    body->n_children = 1;
    body->children = arena.alloc_array<c4c::Node*>(1);
    body->children[0] = ret;

    c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, line);
    fn->name = arena.strdup(rendered);
    fn->unqualified_name = arena.strdup("value");
    fn->unqualified_text_id = value_text;
    fn->namespace_context_id = parser.current_namespace_context_id();
    fn->n_qualifier_segments = 1;
    fn->qualifier_segments = arena.alloc_array<const char*>(1);
    fn->qualifier_segments[0] = arena.strdup(owner);
    fn->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
    fn->qualifier_text_ids[0] = owner_text;
    fn->type = make_ts(c4c::TB_INT);
    fn->is_consteval = true;
    fn->body = body;
    return fn;
  };

  c4c::Node* owner_b_false = make_consteval_function("OwnerB", 0, 1);
  c4c::Node* owner_a_true = make_consteval_function("OwnerA", 1, 2);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 3);
  ref->name = arena.strdup("OwnerA::value");
  ref->unqualified_name = arena.strdup("value");
  ref->unqualified_text_id = texts.intern("value");
  ref->namespace_context_id = parser.current_namespace_context_id();
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("OwnerB");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = texts.intern("OwnerB");

  c4c::Node* call = parser.make_node(c4c::NK_CALL, 3);
  call->left = ref;

  c4c::Node* assertion = parser.make_node(c4c::NK_STATIC_ASSERT, 3);
  assertion->left = call;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 3;
  program->children = arena.alloc_array<c4c::Node*>(3);
  program->children[0] = owner_b_false;
  program->children[1] = owner_a_true;
  program->children[2] = assertion;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema consteval lookup should preserve parser-qualified owner "
              "metadata instead of recovering from stale rendered spelling");
}

void test_consteval_forwarded_nttp_uses_text_id_not_rendered_name() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  const c4c::TextId inner_text = texts.intern("InnerN");
  const c4c::TextId outer_text = texts.intern("OuterN");

  c4c::Node* func_def = parser.make_node(c4c::NK_FUNCTION, 1);
  func_def->name = arena.strdup("templated");
  func_def->unqualified_name = arena.strdup("templated");
  func_def->unqualified_text_id = texts.intern("templated");
  func_def->namespace_context_id = parser.current_namespace_context_id();
  func_def->n_template_params = 1;
  func_def->template_param_names = arena.alloc_array<const char*>(1);
  func_def->template_param_names[0] = arena.strdup("InnerN");
  func_def->template_param_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  func_def->template_param_name_text_ids[0] = inner_text;
  func_def->template_param_is_nttp = arena.alloc_array<bool>(1);
  func_def->template_param_is_nttp[0] = true;

  c4c::Node* callee = parser.make_node(c4c::NK_VAR, 2);
  callee->name = arena.strdup("templated");
  callee->unqualified_name = arena.strdup("templated");
  callee->unqualified_text_id = func_def->unqualified_text_id;
  callee->namespace_context_id = parser.current_namespace_context_id();
  callee->has_template_args = true;
  callee->n_template_args = 1;
  callee->template_arg_is_value = arena.alloc_array<bool>(1);
  callee->template_arg_is_value[0] = true;
  callee->template_arg_values = arena.alloc_array<long long>(1);
  callee->template_arg_values[0] = 0;
  callee->template_arg_nttp_names = arena.alloc_array<const char*>(1);
  callee->template_arg_nttp_names[0] = arena.strdup("RenderedOuter");
  callee->template_arg_nttp_text_ids = arena.alloc_array<c4c::TextId>(1);
  callee->template_arg_nttp_text_ids[0] = outer_text;

  std::unordered_map<std::string, long long> legacy_outer;
  legacy_outer["RenderedOuter"] = 11;
  c4c::hir::ConstTextMap text_outer;
  text_outer[outer_text] = 5;
  c4c::hir::ConstEvalEnv outer_env;
  outer_env.nttp_bindings = nullptr;
  outer_env.nttp_bindings_by_text = &text_outer;

  c4c::hir::TypeBindings type_bindings;
  std::unordered_map<std::string, long long> nttp_bindings;
  c4c::hir::ConstTextMap nttp_bindings_by_text;
  c4c::hir::ConstStructuredMap nttp_bindings_by_key;
  (void)c4c::hir::bind_consteval_call_env(
      callee, func_def, outer_env, &type_bindings, &nttp_bindings, nullptr,
      nullptr, nullptr, nullptr, &nttp_bindings_by_text,
      &nttp_bindings_by_key);

  expect_true(nttp_bindings["InnerN"] == 5,
              "forwarded consteval NTTP binding should use parser TextId "
              "metadata instead of rendered argument spelling");
  expect_true(nttp_bindings_by_text[inner_text] == 5,
              "forwarded consteval NTTP binding should preserve structured "
              "TextId output metadata");

  outer_env.nttp_bindings = &legacy_outer;
  c4c::hir::ConstTextMap mismatched_text_outer;
  mismatched_text_outer[texts.intern("OtherOuter")] = 17;
  outer_env.nttp_bindings_by_text = &mismatched_text_outer;
  nttp_bindings.clear();
  nttp_bindings_by_text.clear();
  nttp_bindings_by_key.clear();
  (void)c4c::hir::bind_consteval_call_env(
      callee, func_def, outer_env, &type_bindings, &nttp_bindings, nullptr,
      nullptr, nullptr, nullptr, &nttp_bindings_by_text,
      &nttp_bindings_by_key);
  expect_true(nttp_bindings.empty(),
              "forwarded consteval NTTP binding should not reopen rendered "
              "name lookup after authoritative TextId metadata misses");
}

void test_parser_deferred_nttp_default_uses_structured_binding_metadata() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::TextId trait_text = texts.intern("Trait");
  const c4c::TextId structured_text = texts.intern("StructuredN");
  const c4c::TextId other_text = texts.intern("OtherN");
  const c4c::QualifiedNameKey trait_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), trait_text);

  parser.cache_nttp_default_expr_tokens(
      trait_key, 1,
      {parser.make_injected_token(seed, c4c::TokenKind::Identifier,
                                  "StructuredN")});

  std::vector<std::pair<std::string, long long>> rendered_bindings;
  rendered_bindings.push_back({"StructuredN", 100});

  std::vector<c4c::ParserNttpBindingMetadata> structured_bindings;
  c4c::ParserNttpBindingMetadata binding{};
  binding.name = "RenderedN";
  binding.name_text_id = structured_text;
  binding.name_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), structured_text);
  binding.value = 6;
  structured_bindings.push_back(binding);

  long long value = 0;
  expect_true(parser.eval_deferred_nttp_default(
                  trait_key, 1, {}, rendered_bindings, &value,
                  &structured_bindings),
              "deferred NTTP defaults should use structured TextId binding "
              "metadata before rendered binding names");
  expect_true(value == 6,
              "structured NTTP metadata should win over a rendered binding "
              "with the same expression spelling");

  parser.cache_nttp_default_expr_tokens(
      trait_key, 2,
      {parser.make_injected_token(seed, c4c::TokenKind::Identifier,
                                  "RenderedN")});
  rendered_bindings.clear();
  rendered_bindings.push_back({"RenderedN", 100});
  structured_bindings[0].name_text_id = other_text;
  structured_bindings[0].name_key = parser.alias_template_key_in_context(
      parser.current_namespace_context_id(), other_text);
  value = 0;
  expect_true(!parser.eval_deferred_nttp_default(
                  trait_key, 2, {}, rendered_bindings, &value,
                  &structured_bindings),
              "deferred NTTP defaults should not reopen rendered-name binding "
              "lookup after authoritative structured metadata misses");
}

void test_alias_template_deferred_nttp_bases_carry_structured_record_def() {
  const char* source =
      "template <typename T, T v>\n"
      "struct integral_constant {\n"
      "  static constexpr T value = v;\n"
      "};\n"
      "template <bool B>\n"
      "using bool_constant = integral_constant<bool, B>;\n"
      "template <typename T>\n"
      "struct signed_probe : bool_constant<(T(-1) < T(0))> {};\n"
      "enum Color { Red, Blue };\n"
      "template <typename T>\n"
      "struct is_enum : integral_constant<bool, __is_enum(T)> {};\n"
      "int read_signed() {\n"
      "  return signed_probe<int>::value ? 0 : 1;\n"
      "}\n"
      "int read_unsigned() {\n"
      "  return signed_probe<unsigned int>::value ? 2 : 0;\n"
      "}\n"
      "int read_enum() {\n"
      "  return is_enum<Color>::value ? 0 : 3;\n"
      "}\n";
  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  int signed_probe_instances = 0;
  int is_enum_instances = 0;
  for (int i = 0; root && i < root->n_children; ++i) {
    c4c::Node* record = root->children[i];
    if (!record || record->kind != c4c::NK_STRUCT_DEF ||
        !record->template_origin_name || record->n_template_args <= 0) {
      continue;
    }
    const std::string origin(record->template_origin_name);
    if (origin != "signed_probe" && origin != "is_enum") continue;
    expect_true(record->n_bases == 1 && record->base_types != nullptr,
                "instantiated trait wrapper should preserve its inherited base");
    expect_true(record->base_types[0].record_def != nullptr,
                std::string("alias-template deferred NTTP base should carry "
                            "record_def for ") +
                    (record->name ? record->name : "<unnamed>"));
    record->base_types[0].tag = arena.strdup("RenderedBaseDrift");
    if (record->base_types[0].tpl_struct_args.data &&
        record->base_types[0].tpl_struct_args.size > 0) {
      record->base_types[0].tpl_struct_args.data[0].debug_text =
          arena.strdup("$expr:RenderedFallbackDrift");
    }
    if (origin == "signed_probe") ++signed_probe_instances;
    if (origin == "is_enum") ++is_enum_instances;
  }

  expect_true(signed_probe_instances >= 2,
              "signed_probe<int> and signed_probe<unsigned int> should instantiate");
  expect_true(is_enum_instances >= 1,
              "is_enum<Color> should instantiate");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema inherited static-member lookup should consume base "
              "record_def metadata despite rendered base spelling drift" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
}

void test_alias_template_nttp_base_carrier_ignores_stale_debug_text() {
  c4c::Arena arena;
  c4c::Lexer lexer("Alias<7>",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("B");
  const c4c::TextId stale_text = lexer.text_table().intern("RenderedDrift");

  c4c::Node* carrier_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  carrier_record->name = arena.strdup("Carrier_7");
  carrier_record->unqualified_name = arena.strdup("Carrier_7");
  carrier_record->unqualified_text_id = lexer.text_table().intern("Carrier_7");
  carrier_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec aliased{};
  aliased.array_size = -1;
  aliased.inner_rank = -1;
  aliased.base = c4c::TB_STRUCT;
  aliased.tag = arena.strdup("Carrier_RenderedDrift");
  aliased.tpl_struct_origin = arena.strdup("Carrier");
  aliased.record_def = carrier_record;
  aliased.tpl_struct_args.size = 1;
  aliased.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  aliased.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Value;
  aliased.tpl_struct_args.data[0].type = {};
  aliased.tpl_struct_args.data[0].type.array_size = -1;
  aliased.tpl_struct_args.data[0].type.inner_rank = -1;
  aliased.tpl_struct_args.data[0].value = 0;
  aliased.tpl_struct_args.data[0].nttp_text_id = param_text;
  aliased.tpl_struct_args.data[0].debug_text = arena.strdup("RenderedDrift");

  c4c::ParserAliasTemplateInfo info{};
  info.param_names.push_back(arena.strdup("B"));
  info.param_name_text_ids.push_back(param_text);
  info.param_is_nttp.push_back(true);
  info.param_is_pack.push_back(false);
  info.param_has_default.push_back(false);
  info.aliased_type = aliased;

  c4c::TypeSpec alias_typedef{};
  alias_typedef.array_size = -1;
  alias_typedef.inner_rank = -1;
  alias_typedef.base = c4c::TB_TYPEDEF;
  alias_typedef.tag = arena.strdup("Alias");
  alias_typedef.tag_text_id = alias_text;
  parser.register_typedef_binding(alias_text, alias_typedef, true);
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           alias_text),
      info);

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def == carrier_record,
              "alias NTTP substitution should preserve the inherited base "
              "record_def carrier");
  expect_true(resolved.tpl_struct_args.data != nullptr &&
                  resolved.tpl_struct_args.size == 1,
              "alias NTTP substitution should retain the structured value arg");
  const c4c::TemplateArgRef& arg = resolved.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Value && arg.value == 7,
              "alias NTTP substitution should use TemplateArgRef TextId "
              "identity instead of stale debug text");
  expect_true(arg.debug_text && std::string(arg.debug_text) == "7",
              "substituted alias NTTP debug text should be regenerated from "
              "the actual argument, not the stale carrier spelling");
  expect_true(stale_text != c4c::kInvalidText,
              "test setup should intern the stale rendered spelling");
}

void test_alias_template_mixed_carrier_skips_rendered_arg_fallback() {
  c4c::Arena arena;
  c4c::Lexer lexer("Alias<7>",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("B");

  c4c::Node* carrier_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  carrier_record->name = arena.strdup("Carrier_Mixed");
  carrier_record->unqualified_name = arena.strdup("Carrier_Mixed");
  carrier_record->unqualified_text_id = lexer.text_table().intern("Carrier_Mixed");
  carrier_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec aliased{};
  aliased.array_size = -1;
  aliased.inner_rank = -1;
  aliased.base = c4c::TB_STRUCT;
  aliased.tag = arena.strdup("Carrier_OpaqueNoCarrier_RenderedDrift");
  aliased.tpl_struct_origin = arena.strdup("Carrier");
  aliased.record_def = carrier_record;
  aliased.tpl_struct_args.size = 2;
  aliased.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(2);
  aliased.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  aliased.tpl_struct_args.data[0].type = {};
  aliased.tpl_struct_args.data[0].type.base = c4c::TB_VOID;
  aliased.tpl_struct_args.data[0].type.array_size = -1;
  aliased.tpl_struct_args.data[0].type.inner_rank = -1;
  aliased.tpl_struct_args.data[0].value = 0;
  aliased.tpl_struct_args.data[0].nttp_text_id = c4c::kInvalidText;
  aliased.tpl_struct_args.data[0].debug_text = arena.strdup("OpaqueNoCarrier");
  aliased.tpl_struct_args.data[1].kind = c4c::TemplateArgKind::Value;
  aliased.tpl_struct_args.data[1].type = {};
  aliased.tpl_struct_args.data[1].type.array_size = -1;
  aliased.tpl_struct_args.data[1].type.inner_rank = -1;
  aliased.tpl_struct_args.data[1].value = 0;
  aliased.tpl_struct_args.data[1].nttp_text_id = param_text;
  aliased.tpl_struct_args.data[1].debug_text = arena.strdup("RenderedDrift");

  c4c::ParserAliasTemplateInfo info{};
  info.param_names.push_back(arena.strdup("B"));
  info.param_name_text_ids.push_back(param_text);
  info.param_is_nttp.push_back(true);
  info.param_is_pack.push_back(false);
  info.param_has_default.push_back(false);
  info.aliased_type = aliased;

  c4c::TypeSpec alias_typedef{};
  alias_typedef.array_size = -1;
  alias_typedef.inner_rank = -1;
  alias_typedef.base = c4c::TB_TYPEDEF;
  alias_typedef.tag = arena.strdup("Alias");
  alias_typedef.tag_text_id = alias_text;
  parser.register_typedef_binding(alias_text, alias_typedef, true);
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           alias_text),
      info);

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def == carrier_record,
              "mixed alias substitution should preserve the record_def carrier");
  expect_true(resolved.tpl_struct_args.data != nullptr &&
                  resolved.tpl_struct_args.size == 2,
              "mixed alias substitution should preserve template arg arity");
  const c4c::TemplateArgRef& opaque_arg = resolved.tpl_struct_args.data[0];
  expect_true(opaque_arg.kind == c4c::TemplateArgKind::Type &&
                  opaque_arg.debug_text &&
                  std::string(opaque_arg.debug_text) == "OpaqueNoCarrier",
              "debug-only no-carrier arg should remain compatibility data");
  const c4c::TemplateArgRef& structured_arg = resolved.tpl_struct_args.data[1];
  expect_true(structured_arg.kind == c4c::TemplateArgKind::Value &&
                  structured_arg.value == 7,
              "mixed alias substitution should not let stale rendered arg refs "
              "override structured NTTP metadata");
  expect_true(structured_arg.debug_text &&
                  std::string(structured_arg.debug_text) == "7",
              "mixed alias substitution should regenerate structured NTTP "
              "debug text after substitution");
}

void test_alias_template_record_def_blocks_debug_arg_substitution() {
  c4c::Arena arena;
  c4c::Lexer lexer("Alias<int>",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("T");

  c4c::Node* carrier_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  carrier_record->name = arena.strdup("Carrier_T");
  carrier_record->unqualified_name = arena.strdup("Carrier_T");
  carrier_record->unqualified_text_id = lexer.text_table().intern("Carrier_T");
  carrier_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec aliased{};
  aliased.array_size = -1;
  aliased.inner_rank = -1;
  aliased.base = c4c::TB_STRUCT;
  aliased.tag = arena.strdup("Carrier_T");
  aliased.tpl_struct_origin = arena.strdup("Carrier");
  aliased.record_def = carrier_record;
  aliased.tpl_struct_args.size = 1;
  aliased.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  aliased.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  aliased.tpl_struct_args.data[0].type = {};
  aliased.tpl_struct_args.data[0].type.base = c4c::TB_VOID;
  aliased.tpl_struct_args.data[0].type.array_size = -1;
  aliased.tpl_struct_args.data[0].type.inner_rank = -1;
  aliased.tpl_struct_args.data[0].value = 0;
  aliased.tpl_struct_args.data[0].nttp_text_id = c4c::kInvalidText;
  aliased.tpl_struct_args.data[0].debug_text = arena.strdup("T");

  c4c::ParserAliasTemplateInfo info{};
  info.param_names.push_back(arena.strdup("T"));
  info.param_name_text_ids.push_back(param_text);
  info.param_is_nttp.push_back(false);
  info.param_is_pack.push_back(false);
  info.param_has_default.push_back(false);
  info.aliased_type = aliased;

  c4c::TypeSpec alias_typedef{};
  alias_typedef.array_size = -1;
  alias_typedef.inner_rank = -1;
  alias_typedef.base = c4c::TB_TYPEDEF;
  alias_typedef.tag = arena.strdup("Alias");
  alias_typedef.tag_text_id = alias_text;
  parser.register_typedef_binding(alias_text, alias_typedef, true);
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           alias_text),
      info);

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def == carrier_record,
              "alias substitution should preserve the record_def carrier");
  expect_true(resolved.tpl_struct_origin_key.base_text_id == c4c::kInvalidText,
              "test fixture should exercise the record-def-only carrier path");
  expect_true(resolved.tpl_struct_args.data != nullptr &&
                  resolved.tpl_struct_args.size == 1,
              "record-def-only alias substitution should preserve template arg "
              "arity");
  const c4c::TemplateArgRef& arg = resolved.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Type && arg.debug_text &&
                  std::string(arg.debug_text) == "T",
              "record_def carriers should not let rendered arg refs substitute "
              "debug-only type args as semantic metadata");
}

void test_alias_template_origin_key_blocks_debug_arg_substitution() {
  c4c::Arena arena;
  c4c::Lexer lexer("Alias<int>",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("T");
  const c4c::TextId carrier_text = lexer.text_table().intern("Carrier");

  c4c::Node* carrier_record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  carrier_record->name = arena.strdup("Carrier_T");
  carrier_record->unqualified_name = arena.strdup("Carrier_T");
  carrier_record->unqualified_text_id = lexer.text_table().intern("Carrier_T");
  carrier_record->namespace_context_id = parser.current_namespace_context_id();

  c4c::TypeSpec aliased{};
  aliased.array_size = -1;
  aliased.inner_rank = -1;
  aliased.base = c4c::TB_STRUCT;
  aliased.tag = arena.strdup("Carrier_T");
  aliased.tpl_struct_origin = arena.strdup("Carrier");
  aliased.tpl_struct_origin_key.context_id =
      parser.current_namespace_context_id();
  aliased.tpl_struct_origin_key.base_text_id = carrier_text;
  aliased.record_def = carrier_record;
  aliased.tpl_struct_args.size = 1;
  aliased.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  aliased.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  aliased.tpl_struct_args.data[0].type = {};
  aliased.tpl_struct_args.data[0].type.base = c4c::TB_VOID;
  aliased.tpl_struct_args.data[0].type.array_size = -1;
  aliased.tpl_struct_args.data[0].type.inner_rank = -1;
  aliased.tpl_struct_args.data[0].value = 0;
  aliased.tpl_struct_args.data[0].nttp_text_id = c4c::kInvalidText;
  aliased.tpl_struct_args.data[0].debug_text = arena.strdup("T");

  c4c::ParserAliasTemplateInfo info{};
  info.param_names.push_back(arena.strdup("T"));
  info.param_name_text_ids.push_back(param_text);
  info.param_is_nttp.push_back(false);
  info.param_is_pack.push_back(false);
  info.param_has_default.push_back(false);
  info.aliased_type = aliased;

  c4c::TypeSpec alias_typedef{};
  alias_typedef.array_size = -1;
  alias_typedef.inner_rank = -1;
  alias_typedef.base = c4c::TB_TYPEDEF;
  alias_typedef.tag = arena.strdup("Alias");
  alias_typedef.tag_text_id = alias_text;
  parser.register_typedef_binding(alias_text, alias_typedef, true);
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           alias_text),
      info);

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def == carrier_record,
              "alias substitution should preserve structured record metadata");
  expect_true(resolved.tpl_struct_origin_key.base_text_id == carrier_text,
              "alias substitution should preserve the template-origin key");
  expect_true(resolved.tpl_struct_args.data != nullptr &&
                  resolved.tpl_struct_args.size == 1,
              "alias substitution should preserve template arg arity");
  const c4c::TemplateArgRef& arg = resolved.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Type && arg.debug_text &&
                  std::string(arg.debug_text) == "T",
              "template-origin-key carriers should not let rendered arg refs "
              "substitute debug-only type args as semantic metadata");
}

void test_alias_template_origin_key_only_blocks_debug_arg_substitution() {
  c4c::Arena arena;
  c4c::Lexer lexer("Alias<int>",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("T");
  const c4c::TextId carrier_text = lexer.text_table().intern("Carrier");

  c4c::TypeSpec aliased{};
  aliased.array_size = -1;
  aliased.inner_rank = -1;
  aliased.base = c4c::TB_STRUCT;
  aliased.tag = arena.strdup("Carrier_T");
  aliased.tpl_struct_origin = arena.strdup("Carrier");
  aliased.tpl_struct_origin_key.context_id =
      parser.current_namespace_context_id();
  aliased.tpl_struct_origin_key.base_text_id = carrier_text;
  aliased.tpl_struct_args.size = 1;
  aliased.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  aliased.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  aliased.tpl_struct_args.data[0].type = {};
  aliased.tpl_struct_args.data[0].type.base = c4c::TB_VOID;
  aliased.tpl_struct_args.data[0].type.array_size = -1;
  aliased.tpl_struct_args.data[0].type.inner_rank = -1;
  aliased.tpl_struct_args.data[0].value = 0;
  aliased.tpl_struct_args.data[0].nttp_text_id = c4c::kInvalidText;
  aliased.tpl_struct_args.data[0].debug_text = arena.strdup("T");

  c4c::ParserAliasTemplateInfo info{};
  info.param_names.push_back(arena.strdup("T"));
  info.param_name_text_ids.push_back(param_text);
  info.param_is_nttp.push_back(false);
  info.param_is_pack.push_back(false);
  info.param_has_default.push_back(false);
  info.aliased_type = aliased;

  c4c::TypeSpec alias_typedef{};
  alias_typedef.array_size = -1;
  alias_typedef.inner_rank = -1;
  alias_typedef.base = c4c::TB_TYPEDEF;
  alias_typedef.tag = arena.strdup("Alias");
  alias_typedef.tag_text_id = alias_text;
  parser.register_typedef_binding(alias_text, alias_typedef, true);
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           alias_text),
      info);

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def == nullptr,
              "test fixture should exercise the origin-key-only carrier path");
  expect_true(resolved.tpl_struct_origin_key.base_text_id == carrier_text,
              "origin-key-only alias substitution should preserve the "
              "template-origin key");
  expect_true(resolved.tpl_struct_args.data != nullptr &&
                  resolved.tpl_struct_args.size == 1,
              "origin-key-only alias substitution should preserve template arg "
              "arity");
  const c4c::TemplateArgRef& arg = resolved.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Type && arg.debug_text &&
                  std::string(arg.debug_text) == "T",
              "origin-key-only carriers should not let rendered arg refs "
              "substitute debug-only type args as semantic metadata");
}

void test_alias_member_typedef_origin_key_only_materializes_record_def() {
  const char* source =
      "template <typename T>\n"
      "struct Carrier {};\n"
      "template <typename T>\n"
      "struct Owner {\n"
      "  using type = Carrier<T>;\n"
      "};\n"
      "template <typename T>\n"
      "using Alias = typename Owner<T>::type;\n";

  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");
  (void)parser.parse();

  const c4c::TextId owner_text = lexer.text_table().intern("Owner");
  c4c::Node* owner_primary = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), owner_text);
  expect_true(owner_primary && owner_primary->n_member_typedefs == 1 &&
                  owner_primary->member_typedef_types,
              "Owner primary should carry its member typedef");
  c4c::TypeSpec& member_type = owner_primary->member_typedef_types[0];
  expect_true(member_type.record_def == nullptr &&
                  member_type.tpl_struct_origin_key.base_text_id !=
                      c4c::kInvalidText,
              "test fixture should start from an origin-key-only member type");
  member_type.tag = arena.strdup("RenderedCarrierDrift");
  member_type.tpl_struct_origin = arena.strdup("RenderedCarrierDrift");
  if (member_type.tpl_struct_args.data && member_type.tpl_struct_args.size > 0) {
    member_type.tpl_struct_args.data[0].debug_text =
        arena.strdup("RenderedArgDrift");
  }

  c4c::Token seed{};
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def != nullptr,
              "alias member-typedef origin-key-only handoff should materialize "
              "record_def without rendered owner/template spelling");
  expect_true(resolved.record_def->template_origin_name &&
                  std::string(resolved.record_def->template_origin_name) ==
                      "Carrier",
              "materialized member typedef should use the structured origin key "
              "instead of stale rendered origin text");
  expect_true(resolved.record_def->n_template_args == 1 &&
                  resolved.record_def->template_arg_types &&
                  !resolved.record_def->template_arg_is_value[0] &&
                  resolved.record_def->template_arg_types[0].base == c4c::TB_INT,
              "materialized member typedef should use structured alias args "
              "instead of stale debug text");
}

void test_alias_template_no_carrier_debug_arg_stays_debug_only() {
  c4c::Arena arena;
  c4c::Lexer lexer("Alias<int>",
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");

  const c4c::TextId alias_text = lexer.text_table().intern("Alias");
  const c4c::TextId param_text = lexer.text_table().intern("T");

  c4c::TypeSpec aliased{};
  aliased.array_size = -1;
  aliased.inner_rank = -1;
  aliased.base = c4c::TB_STRUCT;
  aliased.tag = arena.strdup("Carrier_T");
  aliased.tpl_struct_origin = arena.strdup("Carrier");
  aliased.tpl_struct_args.size = 1;
  aliased.tpl_struct_args.data = arena.alloc_array<c4c::TemplateArgRef>(1);
  aliased.tpl_struct_args.data[0].kind = c4c::TemplateArgKind::Type;
  aliased.tpl_struct_args.data[0].type = {};
  aliased.tpl_struct_args.data[0].type.base = c4c::TB_VOID;
  aliased.tpl_struct_args.data[0].type.array_size = -1;
  aliased.tpl_struct_args.data[0].type.inner_rank = -1;
  aliased.tpl_struct_args.data[0].value = 0;
  aliased.tpl_struct_args.data[0].nttp_text_id = c4c::kInvalidText;
  aliased.tpl_struct_args.data[0].debug_text = arena.strdup("T");

  c4c::ParserAliasTemplateInfo info{};
  info.param_names.push_back(arena.strdup("T"));
  info.param_name_text_ids.push_back(param_text);
  info.param_is_nttp.push_back(false);
  info.param_is_pack.push_back(false);
  info.param_has_default.push_back(false);
  info.aliased_type = aliased;

  c4c::TypeSpec alias_typedef{};
  alias_typedef.array_size = -1;
  alias_typedef.inner_rank = -1;
  alias_typedef.base = c4c::TB_TYPEDEF;
  alias_typedef.tag = arena.strdup("Alias");
  alias_typedef.tag_text_id = alias_text;
  parser.register_typedef_binding(alias_text, alias_typedef, true);
  parser.register_alias_template_info_for_testing(
      parser.alias_template_key_in_context(parser.current_namespace_context_id(),
                                           alias_text),
      info);

  c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.record_def == nullptr &&
                  resolved.tpl_struct_origin_key.base_text_id ==
                      c4c::kInvalidText,
              "test fixture should exercise the fully no-carrier path");
  expect_true(resolved.tpl_struct_args.data != nullptr &&
                  resolved.tpl_struct_args.size == 1,
              "no-carrier alias substitution should preserve template arg "
              "arity");
  const c4c::TemplateArgRef& arg = resolved.tpl_struct_args.data[0];
  expect_true(arg.kind == c4c::TemplateArgKind::Type && arg.type.base == c4c::TB_VOID &&
                  arg.debug_text && std::string(arg.debug_text) == "T",
              "fully no-carrier debug-only type args should not be promoted "
              "into rendered substitution authority");
}

void test_dependent_member_typedef_base_carries_structured_record_def() {
  const char* definitions =
      "template <typename T, T v>\n"
      "struct integral_constant {\n"
      "  static constexpr T value = v;\n"
      "  using type = integral_constant<T, v>;\n"
      "};\n"
      "using true_type = integral_constant<bool, true>;\n"
      "using false_type = integral_constant<bool, false>;\n"
      "template <typename T>\n"
      "struct arithmetic {\n"
      "  static constexpr bool value = true;\n"
      "};\n"
      "template <>\n"
      "struct arithmetic<void> {\n"
      "  static constexpr bool value = false;\n"
      "};\n"
      "template <typename T, bool = arithmetic<T>::value>\n"
      "struct is_signed_helper : integral_constant<bool, (T(-1) < T(0))> {};\n"
      "template <typename T>\n"
      "struct is_signed_helper<T, false> : false_type {};\n"
      "template <typename T>\n"
      "struct is_signed : is_signed_helper<T>::type {};\n";
  {
    c4c::Arena arena;
    c4c::Lexer lexer(std::string(definitions),
                     c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
    const std::vector<c4c::Token> tokens = lexer.scan_all();
    c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                       c4c::SourceProfile::CppSubset,
                       "frontend_parser_lookup_authority_tests.cpp");
    (void)parser.parse();

    const c4c::TextId is_signed_text = lexer.text_table().intern("is_signed");
    c4c::Node* primary = parser.find_template_struct_primary(
        parser.current_namespace_context_id(), is_signed_text);
    expect_true(primary && primary->n_bases == 1 && primary->base_types,
                "is_signed primary should expose its dependent base carrier");
    expect_true(primary->base_types[0].tpl_struct_origin_key.base_text_id !=
                    c4c::kInvalidText,
                "dependent member typedef base template-origin key should be "
                "structured");
    primary->base_types[0].tpl_struct_origin =
        arena.strdup("StaleRenderedTemplateOrigin");
    if (primary->base_types[0].tpl_struct_args.data &&
        primary->base_types[0].tpl_struct_args.size > 0) {
      for (int ai = 0; ai < primary->base_types[0].tpl_struct_args.size; ++ai) {
        primary->base_types[0].tpl_struct_args.data[ai].debug_text =
            arena.strdup("$expr:0");
      }
    }
    const c4c::TextId helper_text =
        lexer.text_table().intern("is_signed_helper");
    c4c::Node* helper_primary = parser.find_template_struct_primary(
        parser.current_namespace_context_id(), helper_text);
    expect_true(helper_primary && helper_primary->n_template_params >= 2 &&
                    helper_primary->template_param_default_exprs &&
                    helper_primary->template_param_default_exprs[1],
                "is_signed_helper primary should carry a rendered deferred "
                "NTTP default expression for drift coverage");
    helper_primary->template_param_default_exprs[1] = arena.strdup("0");

    c4c::Token seed{};
    parser.replace_token_stream_for_testing({
        parser.make_injected_token(seed, c4c::TokenKind::Identifier,
                                   "is_signed_helper"),
        parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
        parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
        parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
        parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
    });
    c4c::TypeSpec direct_resolved = parser.parse_base_type();
    expect_true(direct_resolved.record_def &&
                    direct_resolved.record_def->n_template_args >= 2 &&
                    direct_resolved.record_def->template_arg_is_value &&
                    direct_resolved.record_def->template_arg_values,
                "direct template primary instantiation should materialize "
                "defaulted NTTP args through structured metadata");
    expect_true(direct_resolved.record_def->template_arg_is_value[1] &&
                    direct_resolved.record_def->template_arg_values[1] == 1,
                "direct template primary instantiation should use cached "
                "default tokens before stale rendered default-expression text");

    auto make_type_arg = [](c4c::TypeBase base) {
      c4c::Parser::TemplateArgParseResult arg{};
      arg.is_value = false;
      arg.type.array_size = -1;
      arg.type.inner_rank = -1;
      arg.type.base = base;
      return arg;
    };
    for (c4c::TypeBase base : {c4c::TB_INT, c4c::TB_UINT, c4c::TB_VOID}) {
      std::vector<c4c::Parser::TemplateArgParseResult> args;
      args.push_back(make_type_arg(base));
      std::string mangled;
      c4c::TypeSpec resolved{};
      expect_true(parser.ensure_template_struct_instantiated_from_args(
                      "is_signed", primary, args, primary->line, &mangled,
                      "dependent_member_typedef_debug_drift_test", &resolved),
                  "debug-drifted dependent member-typedef primary should still "
                  "instantiate from structured metadata");
      std::string detail;
      if (resolved.record_def && resolved.record_def->n_bases == 1 &&
          resolved.record_def->base_types) {
        const c4c::TypeSpec& base_ts = resolved.record_def->base_types[0];
        detail += " tag=";
        detail += base_ts.tag ? base_ts.tag : "<null>";
        detail += " origin=";
        detail += base_ts.tpl_struct_origin ? base_ts.tpl_struct_origin : "<null>";
        detail += " deferred=";
        detail += base_ts.deferred_member_type_name
                      ? base_ts.deferred_member_type_name
                      : "<null>";
        detail += " args=";
        detail += std::to_string(base_ts.tpl_struct_args.size);
      }
      expect_true(resolved.record_def && resolved.record_def->n_bases == 1 &&
                      resolved.record_def->base_types &&
                      resolved.record_def->base_types[0].record_def,
                  "debug-drifted dependent member-typedef instantiation should "
                  "attach concrete base record_def before Sema" +
                      detail);
    }
  }

  std::string source = std::string(definitions) +
      "int read_signed() {\n"
      "  return is_signed<int>::value ? 0 : 1;\n"
      "}\n"
      "int read_unsigned() {\n"
      "  return is_signed<unsigned int>::value ? 2 : 0;\n"
      "}\n"
      "int read_void() {\n"
      "  return is_signed<void>::value ? 4 : 0;\n"
      "}\n";
  c4c::Arena arena;
  c4c::Lexer lexer(source,
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  c4c::Node* root = parse_cpp_source(arena, lexer);

  int is_signed_instances = 0;
  std::vector<c4c::Node*> pending;
  if (root) pending.push_back(root);
  while (!pending.empty()) {
    c4c::Node* record = pending.back();
    pending.pop_back();
    if (record && record->children) {
      for (int i = 0; i < record->n_children; ++i) {
        if (record->children[i]) pending.push_back(record->children[i]);
      }
    }
    if (!record || record->kind != c4c::NK_STRUCT_DEF ||
        !record->template_origin_name || record->n_template_args <= 0 ||
        std::string(record->template_origin_name) != "is_signed") {
      continue;
    }
    ++is_signed_instances;
    expect_true(record->n_bases == 1 && record->base_types != nullptr,
                "instantiated is_signed<T> should preserve its inherited base");
    expect_true(record->base_types[0].record_def != nullptr,
                std::string("dependent member-typedef base should carry "
                            "record_def for ") +
                    (record->name ? record->name : "<unnamed>"));
    expect_true(record->base_types[0].deferred_member_type_text_id ==
                    c4c::kInvalidText,
                "dependent member-typedef base should be resolved before Sema");
    record->base_types[0].tag = arena.strdup("RenderedMemberBaseDrift");
    if (record->base_types[0].tpl_struct_args.data &&
        record->base_types[0].tpl_struct_args.size > 0) {
      record->base_types[0].tpl_struct_args.data[0].debug_text =
          arena.strdup("$expr:RenderedFallbackDrift");
    }
  }

  expect_true(is_signed_instances >= 3,
              "is_signed<int>, is_signed<unsigned int>, and is_signed<void> "
              "should instantiate");

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(root);
  expect_true(result.ok,
              "Sema inherited static-member lookup should consume dependent "
              "member-typedef base record_def metadata despite rendered base "
              "spelling drift" +
                  (result.diagnostics.empty()
                       ? std::string()
                       : std::string(": ") + result.diagnostics.front().message));
}

void test_default_only_template_base_uses_cached_default_metadata() {
  const char* source =
      "template <typename T>\n"
      "struct arithmetic {\n"
      "  static constexpr bool value = true;\n"
      "};\n"
      "template <typename T = int, bool = arithmetic<T>::value>\n"
      "struct default_only_base {};\n"
      "template <typename T>\n"
      "struct default_only_derived : default_only_base<> {};\n";

  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");
  (void)parser.parse();

  const c4c::TextId base_text = lexer.text_table().intern("default_only_base");
  c4c::Node* base_primary = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), base_text);
  expect_true(base_primary && base_primary->n_template_params >= 2 &&
                  base_primary->template_param_default_exprs &&
                  base_primary->template_param_default_exprs[1],
              "default_only_base should carry a rendered deferred NTTP default "
              "expression for drift coverage");
  base_primary->template_param_default_exprs[1] = arena.strdup("0");

  const c4c::TextId derived_text =
      lexer.text_table().intern("default_only_derived");
  c4c::Node* derived_primary = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), derived_text);
  expect_true(derived_primary && derived_primary->n_bases == 1 &&
                  derived_primary->base_types,
              "default_only_derived should expose its default-only base");
  expect_true(derived_primary->base_types[0].record_def ||
                  derived_primary->base_types[0]
                          .tpl_struct_origin_key.base_text_id !=
                      c4c::kInvalidText,
              "default-only base should carry parser-owned metadata before "
              "instantiation");

  c4c::Parser::TemplateArgParseResult int_arg{};
  int_arg.is_value = false;
  int_arg.type.array_size = -1;
  int_arg.type.inner_rank = -1;
  int_arg.type.base = c4c::TB_INT;
  std::vector<c4c::Parser::TemplateArgParseResult> args;
  args.push_back(int_arg);
  std::string mangled;
  c4c::TypeSpec resolved{};
  expect_true(parser.ensure_template_struct_instantiated_from_args(
                  "default_only_derived", derived_primary, args,
                  derived_primary->line, &mangled,
                  "default_only_base_cached_default_test", &resolved),
              "default-only base inheritance should instantiate");

  c4c::Node* derived_record = resolved.record_def;
  expect_true(derived_record && derived_record->n_bases == 1 &&
                  derived_record->base_types &&
                  derived_record->base_types[0].record_def,
              "default-only base should attach record_def before Sema");
  c4c::Node* base_record = derived_record->base_types[0].record_def;
  expect_true(base_record->n_template_args >= 2 &&
                  base_record->template_arg_is_value &&
                  base_record->template_arg_values,
              "default-only base record should materialize defaulted template "
              "args");
  expect_true(base_record->template_arg_is_value[1] &&
                  base_record->template_arg_values[1] == 1,
              "default-only base should use cached default tokens before stale "
              "rendered default-expression text");
}

void test_direct_template_explicit_nttp_expr_ignores_stale_display_text() {
  const char* source =
      "template <typename T, T v>\n"
      "struct integral_constant {\n"
      "  static constexpr T value = v;\n"
      "  using type = integral_constant<T, v>;\n"
      "};\n"
      "using false_type = integral_constant<bool, false>;\n"
      "template <typename T, bool = true>\n"
      "struct is_signed_helper : integral_constant<bool, true> {};\n"
      "template <typename T>\n"
      "struct is_signed_helper<T, false> : false_type {};\n";

  c4c::Arena arena;
  c4c::Lexer lexer(std::string(source),
                   c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "frontend_parser_lookup_authority_tests.cpp");
  (void)parser.parse();

  const c4c::TextId base_text = lexer.text_table().intern("is_signed_helper");
  c4c::Node* primary = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), base_text);
  expect_true(primary && primary->n_template_params == 2,
              "is_signed_helper primary should be registered");

  c4c::Token seed{};
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::Identifier,
                                 "is_signed_helper"),
      parser.make_injected_token(seed, c4c::TokenKind::Less, "<"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Comma, ","),
      parser.make_injected_token(seed, c4c::TokenKind::Plus, "+"),
      parser.make_injected_token(seed, c4c::TokenKind::KwTrue, "false"),
      parser.make_injected_token(seed, c4c::TokenKind::Greater, ">"),
      parser.make_injected_token(seed, c4c::TokenKind::EndOfFile, ""),
  });

  c4c::TypeSpec resolved = parser.parse_base_type();
  std::string detail = " tag=";
  detail += resolved.tag ? resolved.tag : "<null>";
  detail += " origin=";
  detail += resolved.tpl_struct_origin ? resolved.tpl_struct_origin : "<null>";
  detail += " args=";
  detail += std::to_string(resolved.tpl_struct_args.size);
  if (resolved.tpl_struct_args.data && resolved.tpl_struct_args.size > 1) {
    detail += " arg1_debug=";
    detail += resolved.tpl_struct_args.data[1].debug_text
                  ? resolved.tpl_struct_args.data[1].debug_text
                  : "<null>";
    detail += " arg1_value=";
    detail += std::to_string(resolved.tpl_struct_args.data[1].value);
  }
  expect_true(resolved.record_def && resolved.record_def->n_template_args >= 2 &&
                  resolved.record_def->template_arg_is_value &&
                  resolved.record_def->template_arg_values,
              "direct explicit NTTP expression should instantiate from "
              "structured parser expression metadata" +
                  detail);
  expect_true(resolved.record_def->template_arg_is_value[1] &&
                  resolved.record_def->template_arg_values[1] == 1,
              "direct explicit NTTP expression should ignore stale `$expr:` "
              "display text when a parsed expression carrier exists");
}

void test_typespec_template_origin_equality_uses_structured_key() {
  c4c::Arena arena;
  c4c::TextTable texts;
  const c4c::TextId carrier_text = texts.intern("Carrier");
  const c4c::TextId other_text = texts.intern("OtherCarrier");

  c4c::TypeSpec lhs{};
  lhs.array_size = -1;
  lhs.inner_rank = -1;
  lhs.base = c4c::TB_TYPEDEF;
  lhs.tpl_struct_origin = arena.strdup("StaleLeftOrigin");
  lhs.tpl_struct_origin_key.context_id = 0;
  lhs.tpl_struct_origin_key.base_text_id = carrier_text;

  c4c::TypeSpec rhs = lhs;
  rhs.tpl_struct_origin = arena.strdup("StaleRightOrigin");

  expect_true(c4c::type_binding_values_equivalent(lhs, rhs),
              "TypeSpec equality should use structured template-origin key "
              "instead of stale rendered origin spelling");

  rhs.tpl_struct_origin_key.base_text_id = other_text;
  expect_true(!c4c::type_binding_values_equivalent(lhs, rhs),
              "TypeSpec equality should reject different structured "
              "template-origin keys even when rendered spelling is ignored");

  rhs.tpl_struct_origin_key = {};
  rhs.tpl_struct_origin = lhs.tpl_struct_origin;
  expect_true(!c4c::type_binding_values_equivalent(lhs, rhs),
              "TypeSpec equality should not fall back to rendered origin "
              "when only one side has a structured key");

  lhs.tpl_struct_origin_key = {};
  expect_true(c4c::type_binding_values_equivalent(lhs, rhs),
              "TypeSpec equality should fall back to rendered origin only "
              "when both structured template-origin keys are absent");
}

void test_sema_this_lookup_rejects_rendered_after_metadata_miss() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files,
                     c4c::SourceProfile::CppSubset);

  auto make_ts = [](c4c::TypeBase base, int ptr_level = 0, const char* tag = nullptr) {
    c4c::TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    ts.base = base;
    ts.ptr_level = ptr_level;
    ts.tag = tag;
    return ts;
  };

  const c4c::TextId owner_text = texts.intern("Owner");
  const c4c::TextId method_text = texts.intern("self");
  const c4c::TextId this_text = texts.intern("this");
  const c4c::TextId missing_text = texts.intern("missing_this");
  const int namespace_context = parser.current_namespace_context_id();

  c4c::Node* owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  owner->name = arena.strdup("Owner");
  owner->unqualified_name = arena.strdup("Owner");
  owner->unqualified_text_id = owner_text;
  owner->namespace_context_id = namespace_context;

  c4c::Node* canonical_ref = parser.make_node(c4c::NK_VAR, 2);
  canonical_ref->name = arena.strdup("this");
  canonical_ref->unqualified_name = arena.strdup("this");
  canonical_ref->unqualified_text_id = this_text;
  canonical_ref->namespace_context_id = namespace_context;

  c4c::Node* canonical_ret = parser.make_node(c4c::NK_RETURN, 2);
  canonical_ret->left = canonical_ref;

  c4c::Node* stale_ref = parser.make_node(c4c::NK_VAR, 3);
  stale_ref->name = arena.strdup("this");
  stale_ref->unqualified_name = arena.strdup("this");
  stale_ref->unqualified_text_id = missing_text;
  stale_ref->namespace_context_id = namespace_context;

  c4c::Node* stale_ret = parser.make_node(c4c::NK_RETURN, 3);
  stale_ret->left = stale_ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 2;
  body->children = arena.alloc_array<c4c::Node*>(2);
  body->children[0] = canonical_ret;
  body->children[1] = stale_ret;

  c4c::Node* method = parser.make_node(c4c::NK_FUNCTION, 1);
  method->name = arena.strdup("Owner::self");
  method->unqualified_name = arena.strdup("self");
  method->unqualified_text_id = method_text;
  method->namespace_context_id = namespace_context;
  method->n_qualifier_segments = 1;
  method->qualifier_segments = arena.alloc_array<const char*>(1);
  method->qualifier_segments[0] = arena.strdup("Owner");
  method->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  method->qualifier_text_ids[0] = owner_text;
  method->type = make_ts(c4c::TB_STRUCT, 1, "Owner");
  method->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = owner;
  program->children[1] = method;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema this lookup should reject stale rendered local fallback "
              "after structured local key miss");
}

void test_sema_global_lookup_rejects_rendered_after_metadata_miss() {
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

  c4c::Node* rendered_only_global = parser.make_node(c4c::NK_DECL, 1);
  rendered_only_global->name = arena.strdup("stale_rendered_global");
  rendered_only_global->namespace_context_id = parser.current_namespace_context_id();
  rendered_only_global->type = make_ts(c4c::TB_SHORT);

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("stale_rendered_global");
  ref->unqualified_name = arena.strdup("missing_global");
  ref->unqualified_text_id = texts.intern("missing_global");
  ref->namespace_context_id = rendered_only_global->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_rendered_global");
  fn->unqualified_name = arena.strdup("rejects_rendered_global");
  fn->unqualified_text_id = texts.intern("rejects_rendered_global");
  fn->namespace_context_id = rendered_only_global->namespace_context_id;
  fn->type = make_ts(c4c::TB_SHORT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_only_global;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema global lookup should reject rendered compatibility after "
              "the reference structured metadata misses");
}

void test_sema_enum_lookup_rejects_rendered_after_metadata_miss() {
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

  c4c::Node* rendered_only_enum = parser.make_node(c4c::NK_ENUM_DEF, 1);
  rendered_only_enum->name = arena.strdup("E");
  rendered_only_enum->unqualified_name = arena.strdup("E");
  rendered_only_enum->unqualified_text_id = texts.intern("E");
  rendered_only_enum->namespace_context_id = parser.current_namespace_context_id();
  rendered_only_enum->n_enum_variants = 1;
  rendered_only_enum->enum_names = arena.alloc_array<const char*>(1);
  rendered_only_enum->enum_names[0] = arena.strdup("stale_rendered_enum");

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("stale_rendered_enum");
  ref->unqualified_name = arena.strdup("missing_enum");
  ref->unqualified_text_id = texts.intern("missing_enum");
  ref->namespace_context_id = rendered_only_enum->namespace_context_id;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_rendered_enum");
  fn->unqualified_name = arena.strdup("rejects_rendered_enum");
  fn->unqualified_text_id = texts.intern("rejects_rendered_enum");
  fn->namespace_context_id = rendered_only_enum->namespace_context_id;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_only_enum;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema enum lookup should reject rendered compatibility after "
              "the reference structured metadata misses");
}

void test_sema_enum_lookup_rejects_same_member_wrong_owner_reentry() {
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

  const c4c::TextId owner_a_text = texts.intern("OwnerA");
  const c4c::TextId owner_b_text = texts.intern("OwnerB");
  const c4c::TextId enum_text = texts.intern("Domain");
  const c4c::TextId member_text = texts.intern("same_enum_member");
  const int namespace_context = parser.current_namespace_context_id();

  c4c::Node* rendered_enum = parser.make_node(c4c::NK_ENUM_DEF, 1);
  rendered_enum->name = arena.strdup("OwnerA::Domain");
  rendered_enum->unqualified_name = arena.strdup("Domain");
  rendered_enum->unqualified_text_id = enum_text;
  rendered_enum->namespace_context_id = namespace_context;
  rendered_enum->n_qualifier_segments = 1;
  rendered_enum->qualifier_segments = arena.alloc_array<const char*>(1);
  rendered_enum->qualifier_segments[0] = arena.strdup("OwnerA");
  rendered_enum->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  rendered_enum->qualifier_text_ids[0] = owner_a_text;
  rendered_enum->n_enum_variants = 1;
  rendered_enum->enum_names = arena.alloc_array<const char*>(1);
  rendered_enum->enum_name_text_ids = arena.alloc_array<c4c::TextId>(1);
  rendered_enum->enum_vals = arena.alloc_array<long long>(1);
  rendered_enum->enum_names[0] = arena.strdup("OwnerA::same_enum_member");
  rendered_enum->enum_name_text_ids[0] = member_text;
  rendered_enum->enum_vals[0] = 1;

  c4c::Node* ref = parser.make_node(c4c::NK_VAR, 2);
  ref->name = arena.strdup("OwnerA::same_enum_member");
  ref->unqualified_name = arena.strdup("same_enum_member");
  ref->unqualified_text_id = member_text;
  ref->namespace_context_id = namespace_context;
  ref->n_qualifier_segments = 1;
  ref->qualifier_segments = arena.alloc_array<const char*>(1);
  ref->qualifier_segments[0] = arena.strdup("OwnerB");
  ref->qualifier_text_ids = arena.alloc_array<c4c::TextId>(1);
  ref->qualifier_text_ids[0] = owner_b_text;

  c4c::Node* ret = parser.make_node(c4c::NK_RETURN, 2);
  ret->left = ref;

  c4c::Node* body = parser.make_node(c4c::NK_BLOCK, 2);
  body->n_children = 1;
  body->children = arena.alloc_array<c4c::Node*>(1);
  body->children[0] = ret;

  c4c::Node* fn = parser.make_node(c4c::NK_FUNCTION, 2);
  fn->name = arena.strdup("rejects_wrong_owner_enum");
  fn->unqualified_name = arena.strdup("rejects_wrong_owner_enum");
  fn->unqualified_text_id = texts.intern("rejects_wrong_owner_enum");
  fn->namespace_context_id = namespace_context;
  fn->type = make_ts(c4c::TB_INT);
  fn->body = body;

  c4c::Node* program = parser.make_node(c4c::NK_PROGRAM, 1);
  program->n_children = 2;
  program->children = arena.alloc_array<c4c::Node*>(2);
  program->children[0] = rendered_enum;
  program->children[1] = fn;

  const c4c::sema::ValidateResult result = c4c::sema::validate_program(program);
  expect_true(!result.ok,
              "Sema enum lookup should reject same-member rendered fallback "
              "when structured enum-domain metadata names a different owner");
}

}  // namespace

int main() {
  test_global_qualified_lookup_rejects_rendered_fallback_authority();
  test_sema_global_lookup_uses_using_value_alias_target_key();
  test_sema_function_call_uses_using_value_alias_target_key();
  test_qualified_known_function_lookup_uses_key_not_rendered_spelling();
  test_namespace_qualified_function_decl_handoff_uses_context_metadata();
  test_out_of_class_method_owner_handoff_uses_qualifier_metadata();
  test_parsed_local_var_ref_handoff_uses_text_id_not_rendered_spelling();
  test_alias_template_lookup_rejects_visible_type_rendered_reentry();
  test_qualified_typedef_name_uses_structured_result_not_rendered_reentry();
  test_dependent_typename_rejects_visible_type_rendered_reentry();
  test_sema_static_member_lookup_rejects_stale_rendered_owner_reentry();
  test_sema_template_static_member_lookup_rejects_stale_rendered_owner_reentry();
  test_sema_template_static_member_lookup_rejects_rendered_after_metadata_miss();
  test_sema_static_member_lookup_rejects_rendered_after_metadata_miss();
  test_parsed_static_member_lookup_uses_qualifier_metadata_after_rendered_tag_drift();
  test_sema_instance_field_lookup_rejects_stale_member_spelling();
  test_parsed_record_fields_carry_member_text_ids_into_sema();
  test_parsed_nested_record_fields_carry_member_text_ids();
  test_sema_global_lookup_rejects_stale_qualified_rendered_reentry();
  test_sema_global_lookup_rejects_same_member_wrong_owner_reentry();
  test_sema_global_lookup_rejects_unqualified_rendered_same_member_reentry();
  test_sema_function_lookup_rejects_same_member_wrong_owner_reentry();
  test_sema_ref_overload_lookup_rejects_same_member_wrong_owner_reentry();
  test_sema_function_lookup_rejects_legacy_rendered_after_structured_miss();
  test_sema_ref_overload_lookup_rejects_legacy_rendered_after_structured_miss();
  test_sema_cpp_overload_lookup_rejects_legacy_rendered_after_structured_miss();
  test_sema_namespace_ordinary_cpp_overload_decls_use_structured_set();
  test_sema_cpp_overload_decls_reject_same_params_different_return();
  test_sema_cpp_overload_decls_reject_c_language_linkage();
  test_sema_func_local_lookup_rejects_rendered_after_metadata_miss();
  test_sema_consteval_lookup_uses_qualified_key_not_rendered_spelling();
  test_consteval_forwarded_nttp_uses_text_id_not_rendered_name();
  test_parser_deferred_nttp_default_uses_structured_binding_metadata();
  test_alias_template_deferred_nttp_bases_carry_structured_record_def();
  test_alias_template_nttp_base_carrier_ignores_stale_debug_text();
  test_alias_template_mixed_carrier_skips_rendered_arg_fallback();
  test_alias_template_record_def_blocks_debug_arg_substitution();
  test_alias_template_origin_key_blocks_debug_arg_substitution();
  test_alias_template_origin_key_only_blocks_debug_arg_substitution();
  test_alias_member_typedef_origin_key_only_materializes_record_def();
  test_alias_template_no_carrier_debug_arg_stays_debug_only();
  test_dependent_member_typedef_base_carries_structured_record_def();
  test_default_only_template_base_uses_cached_default_metadata();
  test_direct_template_explicit_nttp_expr_ignores_stale_display_text();
  test_typespec_template_origin_equality_uses_structured_key();
  test_sema_this_lookup_rejects_rendered_after_metadata_miss();
  test_sema_global_lookup_rejects_rendered_after_metadata_miss();
  test_sema_enum_lookup_rejects_rendered_after_metadata_miss();
  test_sema_enum_lookup_rejects_same_member_wrong_owner_reentry();
  std::cout << "PASS: frontend_parser_lookup_authority_tests\n";
  return 0;
}
