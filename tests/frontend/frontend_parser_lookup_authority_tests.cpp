#include "lexer.hpp"
#include "impl/parser_impl.hpp"
#include "impl/types/types_helpers.hpp"
#include "parser.hpp"
#include "sema/validate.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
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
  test_qualified_known_function_lookup_uses_key_not_rendered_spelling();
  test_alias_template_lookup_rejects_visible_type_rendered_reentry();
  test_qualified_typedef_name_uses_structured_result_not_rendered_reentry();
  test_dependent_typename_rejects_visible_type_rendered_reentry();
  test_sema_static_member_lookup_rejects_stale_rendered_owner_reentry();
  test_sema_instance_field_lookup_rejects_stale_member_spelling();
  test_parsed_record_fields_carry_member_text_ids_into_sema();
  test_parsed_nested_record_fields_carry_member_text_ids();
  test_sema_global_lookup_rejects_stale_qualified_rendered_reentry();
  test_sema_global_lookup_rejects_same_member_wrong_owner_reentry();
  test_sema_global_lookup_rejects_unqualified_rendered_same_member_reentry();
  test_sema_function_lookup_rejects_same_member_wrong_owner_reentry();
  test_sema_ref_overload_lookup_rejects_same_member_wrong_owner_reentry();
  test_sema_func_local_lookup_rejects_rendered_after_metadata_miss();
  test_sema_consteval_lookup_uses_qualified_key_not_rendered_spelling();
  test_sema_this_lookup_rejects_rendered_after_metadata_miss();
  test_sema_global_lookup_rejects_rendered_after_metadata_miss();
  test_sema_enum_lookup_rejects_rendered_after_metadata_miss();
  test_sema_enum_lookup_rejects_same_member_wrong_owner_reentry();
  std::cout << "PASS: frontend_parser_lookup_authority_tests\n";
  return 0;
}
