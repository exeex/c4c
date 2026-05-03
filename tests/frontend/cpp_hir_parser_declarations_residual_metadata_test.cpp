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

}  // namespace

int main() {
  test_template_param_typedef_producer_keeps_structured_identity();
  test_declaration_paths_accept_current_struct_by_metadata();
  test_conversion_operator_mangling_uses_structured_type_name();
  std::cout << "PASS: cpp_hir_parser_declarations_residual_metadata_test\n";
  return 0;
}
