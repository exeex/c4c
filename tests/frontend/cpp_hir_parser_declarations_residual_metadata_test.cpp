#include "lexer.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <exception>
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
auto legacy_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
  return ts.tag;
}

const char* legacy_tag_if_present(const c4c::TypeSpec&, long) {
  return nullptr;
}

c4c::Node* parse_cpp(c4c::Arena& arena, c4c::Lexer& lexer) {
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "cpp_hir_parser_declarations_residual_metadata_test.cpp");
  c4c::Node* root = parser.parse();
  expect_true(root && root->kind == c4c::NK_PROGRAM,
              "test source should parse as a program");
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

const c4c::Node* find_function(const c4c::Node* root,
                               const std::string& name) {
  if (!root || root->kind != c4c::NK_PROGRAM) return nullptr;
  for (int i = 0; i < root->n_children; ++i) {
    const c4c::Node* child = root->children[i];
    if (child && child->kind == c4c::NK_FUNCTION && child->name &&
        child->name == name) {
      return child;
    }
  }
  return nullptr;
}

void test_template_param_typedef_producer_keeps_structured_identity() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      "template <typename T>\n"
      "struct Box {\n"
      "  typedef T value_type;\n"
      "};\n",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const std::vector<c4c::Token> tokens = lexer.scan_all();
  c4c::Parser parser(tokens, arena, &lexer.text_table(), &lexer.file_table(),
                     c4c::SourceProfile::CppSubset,
                     "cpp_hir_parser_declarations_residual_metadata_test.cpp");
  (void)parser.parse();
  const c4c::TextId box_text = lexer.text_table().intern("Box");
  const c4c::Node* box = parser.find_template_struct_primary(
      parser.current_namespace_context_id(), box_text);
  expect_true(box != nullptr, "template primary record should be present");
  expect_true(box->n_template_params == 1,
              "template record should retain one template parameter");
  expect_true(box->n_member_typedefs == 1,
              "template record should retain its member typedef");
  expect_true(box->member_typedef_text_ids[0] != c4c::kInvalidText,
              "member typedef should carry structured name metadata");

  const c4c::TypeSpec& alias_type = box->member_typedef_types[0];
  expect_true(alias_type.base == c4c::TB_TYPEDEF,
              "template parameter alias should remain a typedef TypeSpec");
  expect_true(alias_type.tag_text_id == box->template_param_name_text_ids[0],
              "template parameter alias should use tag_text_id as semantic identity");
  expect_true(alias_type.template_param_text_id ==
                  box->template_param_name_text_ids[0],
              "template parameter alias should keep explicit template-param identity");
  expect_true(legacy_tag_if_present(alias_type, 0) == nullptr ||
                  std::string(legacy_tag_if_present(alias_type, 0)) == "T",
              "template parameter alias rendered tag is compatibility display only");
}

void test_declaration_paths_accept_current_struct_by_metadata() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      "struct Self {\n"
      "  Self* next;\n"
      "  void set(Self value);\n"
      "};\n"
      "void Self::set(Self value) { Self local = value; }\n",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const c4c::Node* root = parse_cpp(arena, lexer);
  expect_true(find_record(root, "Self") != nullptr,
              "current struct declaration should parse");
  expect_true(find_function(root, "Self::set") != nullptr,
              "out-of-class current-struct method should parse");
}

void test_conversion_operator_mangling_uses_structured_type_name() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      "struct Target {};\n"
      "struct Holder { operator Target*(); };\n"
      "Holder::operator Target*() { return 0; }\n",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const c4c::Node* root = parse_cpp(arena, lexer);
  expect_true(find_function(root, "Holder::operator_conv_struct_Target_ptr") !=
                  nullptr,
              "conversion operator mangling should use structured target type spelling");
}

void test_record_member_template_constructor_prelude_carrier() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      "template <typename... T>\n"
      "struct Box {};\n"
      "template <unsigned long... I>\n"
      "struct Index {};\n"
      "struct Carrier {\n"
      "  template <class Value = int, class... Args, unsigned long... Is>\n"
      "  Carrier(Box<Value> one, Box<Args...> many, Index<Is...> seq);\n"
      "};\n",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const c4c::Node* root = parse_cpp(arena, lexer);
  const c4c::Node* carrier = find_record(root, "Carrier");
  expect_true(carrier != nullptr, "Carrier record should parse");
  expect_true(carrier->n_children == 1,
              "Carrier should retain one member constructor");
  const c4c::Node* ctor = carrier->children[0];
  expect_true(ctor && ctor->kind == c4c::NK_FUNCTION && ctor->is_constructor,
              "member template should attach to the constructor Node");
  expect_true(ctor->n_template_params == 3,
              "constructor Node should carry member-template params");
  expect_true(ctor->template_param_name_text_ids[0] != c4c::kInvalidText &&
                  ctor->template_param_name_text_ids[1] != c4c::kInvalidText &&
                  ctor->template_param_name_text_ids[2] != c4c::kInvalidText,
              "constructor template params should carry text ids");
  expect_true(!ctor->template_param_is_nttp[0] &&
                  !ctor->template_param_is_nttp[1] &&
                  ctor->template_param_is_nttp[2],
              "constructor prelude should distinguish type and NTTP params");
  expect_true(!ctor->template_param_is_pack[0] &&
                  ctor->template_param_is_pack[1] &&
                  ctor->template_param_is_pack[2],
              "constructor prelude should retain pack flags");
  expect_true(ctor->template_param_has_default[0] &&
                  ctor->template_param_default_types[0].base == c4c::TB_INT,
              "constructor prelude should retain type-param defaults");
  expect_true(ctor->n_params == 3,
              "constructor should retain its three explicit params");

  const c4c::TemplateArgRefList& value_args =
      ctor->params[0]->type.tpl_struct_args;
  expect_true(value_args.size == 1 && value_args.data,
              "Box<Value> should retain one nested template arg");
  const c4c::TypeSpec& value_ref = value_args.data[0].type;
  expect_true(value_args.data[0].kind == c4c::TemplateArgKind::Type,
              "Box<Value> arg should be a type arg");
  expect_true(value_ref.template_param_text_id ==
                  ctor->template_param_name_text_ids[0],
              "Box<Value> should carry template-param text identity");
  expect_true(value_ref.template_param_index == 0 &&
                  value_ref.template_param_owner_text_id ==
                      ctor->unqualified_text_id,
              "Box<Value> should carry template-param owner/index identity");

  const c4c::TemplateArgRefList& pack_args =
      ctor->params[1]->type.tpl_struct_args;
  expect_true(pack_args.size == 1 && pack_args.data,
              "Box<Args...> should retain one nested pack arg");
  const c4c::TypeSpec& pack_ref = pack_args.data[0].type;
  expect_true(pack_args.data[0].kind == c4c::TemplateArgKind::Type,
              "Box<Args...> arg should be a type arg");
  expect_true(pack_ref.template_param_text_id ==
                  ctor->template_param_name_text_ids[1],
              "Box<Args...> should carry pack text identity");
  expect_true(pack_ref.template_param_index == 1 &&
                  pack_ref.template_param_owner_text_id ==
                      ctor->unqualified_text_id,
              "Box<Args...> should carry pack owner/index identity");

  const c4c::TemplateArgRefList& nttp_args =
      ctor->params[2]->type.tpl_struct_args;
  expect_true(nttp_args.size == 1 && nttp_args.data,
              "Index<Is...> should retain one nested value pack arg");
  expect_true(nttp_args.data[0].kind == c4c::TemplateArgKind::Value,
              "Index<Is...> arg should be a value arg");
  expect_true(nttp_args.data[0].nttp_text_id ==
                  ctor->template_param_name_text_ids[2],
              "Index<Is...> should carry NTTP text identity");
}

void test_qualified_explicit_constructor_template_id_carrier() {
  c4c::Arena arena;
  c4c::Lexer lexer(
      "namespace ns {\n"
      "template <typename T, typename U>\n"
      "struct Owner { Owner(int); };\n"
      "}\n"
      "int main() {\n"
      "  auto value = ns::Owner<int, long>(1);\n"
      "  return 0;\n"
      "}\n",
      c4c::lex_profile_from(c4c::SourceProfile::CppSubset));
  const c4c::Node* root = parse_cpp(arena, lexer);
  const c4c::Node* main_fn = find_function(root, "main");
  expect_true(main_fn && main_fn->body && main_fn->body->n_children >= 1,
              "qualified constructor template-id fixture should have main body");
  const c4c::Node* decl = main_fn->body->children[0];
  expect_true(decl && decl->kind == c4c::NK_DECL && decl->init,
              "qualified constructor template-id fixture should have initialized local");
  const c4c::Node* call = decl->init;
  expect_true(call->kind == c4c::NK_CALL && call->left &&
                  call->left->kind == c4c::NK_VAR,
              "qualified constructor template-id should parse as callee call");
  const c4c::Node* callee = call->left;
  expect_true(callee->has_template_args && callee->n_template_args == 2,
              "constructor callee should retain explicit template args");
  expect_true(callee->template_arg_types &&
                  callee->template_arg_types[0].base == c4c::TB_INT &&
                  callee->template_arg_types[1].base == c4c::TB_LONG,
              "constructor callee template args should retain structured types");
  expect_true(callee->type.base == c4c::TB_STRUCT &&
                  callee->type.tpl_struct_args.size == 2 &&
                  callee->type.tpl_struct_args.data,
              "constructor callee should carry structured owner TypeSpec args");
  expect_true(callee->type.tpl_struct_args.data[0].kind ==
                      c4c::TemplateArgKind::Type &&
                  callee->type.tpl_struct_args.data[0].type.base ==
                      c4c::TB_INT &&
                  callee->type.tpl_struct_args.data[1].kind ==
                      c4c::TemplateArgKind::Type &&
                  callee->type.tpl_struct_args.data[1].type.base ==
                      c4c::TB_LONG,
              "constructor owner TypeSpec args should remain structured");
  expect_true(call->type.base == c4c::TB_STRUCT &&
                  call->type.tpl_struct_args.size == 2,
              "constructor call should carry the same concrete owner TypeSpec");
}

}  // namespace

int main() {
  test_template_param_typedef_producer_keeps_structured_identity();
  test_declaration_paths_accept_current_struct_by_metadata();
  test_conversion_operator_mangling_uses_structured_type_name();
  test_record_member_template_constructor_prelude_carrier();
  test_qualified_explicit_constructor_template_id_carrier();
  std::cout << "PASS: cpp_hir_parser_declarations_residual_metadata_test\n";
  return 0;
}
