#include "sema/validate.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr c4c::TextId kStaleText = 11;
constexpr c4c::TextId kMissingText = 13;

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

c4c::TypeSpec int_type() {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_INT;
  return ts;
}

c4c::TypeSpec int_pointer_type() {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_INT;
  ts.ptr_level = 1;
  return ts;
}

c4c::Node base_node(c4c::NodeKind kind) {
  c4c::Node n{};
  n.kind = kind;
  n.line = 1;
  n.column = 1;
  n.namespace_context_id = -1;
  n.unqualified_text_id = c4c::kInvalidText;
  n.using_value_alias_target_text_id = c4c::kInvalidText;
  n.using_value_alias_target_namespace_context_id = -1;
  return n;
}

c4c::Node global_var(const char* name, c4c::TextId text_id) {
  c4c::Node n = base_node(c4c::NK_GLOBAL_VAR);
  n.name = name;
  n.unqualified_name = name;
  n.unqualified_text_id = text_id;
  n.namespace_context_id = 0;
  n.type = int_type();
  return n;
}

c4c::Node var_ref(const char* rendered_name, const char* unqualified_name,
                  c4c::TextId text_id, int namespace_context_id) {
  c4c::Node n = base_node(c4c::NK_VAR);
  n.name = rendered_name;
  n.unqualified_name = unqualified_name;
  n.unqualified_text_id = text_id;
  n.namespace_context_id = namespace_context_id;
  return n;
}

c4c::Node return_stmt(c4c::Node* expr) {
  c4c::Node n = base_node(c4c::NK_RETURN);
  n.left = expr;
  return n;
}

c4c::Node block(c4c::Node** children, int n_children) {
  c4c::Node n = base_node(c4c::NK_BLOCK);
  n.children = children;
  n.n_children = n_children;
  return n;
}

c4c::Node function(c4c::Node* body) {
  c4c::Node n = base_node(c4c::NK_FUNCTION);
  n.name = "main";
  n.unqualified_name = "main";
  n.unqualified_text_id = 100;
  n.namespace_context_id = 0;
  n.type = int_type();
  n.body = body;
  return n;
}

c4c::Node function_decl(const char* rendered_name, const char* unqualified_name,
                        c4c::TextId text_id, c4c::TypeSpec ret) {
  c4c::Node n = base_node(c4c::NK_FUNCTION);
  n.name = rendered_name;
  n.unqualified_name = unqualified_name;
  n.unqualified_text_id = text_id;
  n.namespace_context_id = 0;
  n.type = ret;
  return n;
}

c4c::Node call_expr(c4c::Node* callee) {
  c4c::Node n = base_node(c4c::NK_CALL);
  n.left = callee;
  return n;
}

c4c::Node program(c4c::Node** children, int n_children) {
  c4c::Node n = base_node(c4c::NK_PROGRAM);
  n.children = children;
  n.n_children = n_children;
  return n;
}

c4c::Node enum_def(const char** names, c4c::TextId* text_ids, int n_names) {
  c4c::Node n = base_node(c4c::NK_ENUM_DEF);
  n.name = "E";
  n.unqualified_name = "E";
  n.unqualified_text_id = 200;
  n.namespace_context_id = 0;
  n.enum_names = names;
  n.enum_name_text_ids = text_ids;
  n.n_enum_variants = n_names;
  return n;
}

bool has_undeclared_identifier(const c4c::sema::ValidateResult& result,
                               std::string_view name) {
  for (const c4c::sema::Diagnostic& diag : result.diagnostics) {
    if (diag.message.find("use of undeclared identifier") != std::string::npos &&
        diag.message.find(name) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool has_diagnostic_message(const c4c::sema::ValidateResult& result,
                            std::string_view message) {
  for (const c4c::sema::Diagnostic& diag : result.diagnostics) {
    if (diag.message.find(message) != std::string::npos) return true;
  }
  return false;
}

c4c::sema::ValidateResult validate_single_return(c4c::Node& binding,
                                                 c4c::Node& ref) {
  c4c::Node ret = return_stmt(&ref);
  c4c::Node* block_children[] = {&ret};
  c4c::Node body = block(block_children, 1);
  c4c::Node fn = function(&body);
  c4c::Node* root_children[] = {&binding, &fn};
  c4c::Node root = program(root_children, 2);
  return c4c::sema::validate_program(&root);
}

void test_function_metadata_miss_rejects_stale_rendered_name() {
  c4c::Node stale =
      function_decl("stale_function", "stale_function", kStaleText,
                    int_pointer_type());
  c4c::Node callee = var_ref("stale_function", "real_function",
                             kMissingText, 0);
  c4c::Node call = call_expr(&callee);

  const c4c::sema::ValidateResult result = validate_single_return(stale, call);

  expect_true(result.ok,
              "structured function reference miss should not recover through rendered name");
  expect_true(!has_diagnostic_message(result, "incompatible return type"),
              "function metadata miss should not use stale rendered function signature");
}

void test_global_metadata_miss_rejects_stale_rendered_name() {
  c4c::Node stale = global_var("stale_global", kStaleText);
  c4c::Node ref = var_ref("stale_global", "real_global", kMissingText, 0);

  const c4c::sema::ValidateResult result = validate_single_return(stale, ref);

  expect_true(!result.ok,
              "structured global reference miss should not recover through rendered name");
  expect_true(has_undeclared_identifier(result, "stale_global"),
              "global metadata miss should report the stale rendered reference");
}

void test_enum_metadata_miss_rejects_stale_rendered_name() {
  const char* names[] = {"stale_enum"};
  c4c::TextId text_ids[] = {kStaleText};
  c4c::Node stale_enum = enum_def(names, text_ids, 1);
  c4c::Node ref = var_ref("stale_enum", "real_enum", kMissingText, 0);

  const c4c::sema::ValidateResult result = validate_single_return(stale_enum, ref);

  expect_true(!result.ok,
              "structured enum reference miss should not recover through rendered name");
  expect_true(has_undeclared_identifier(result, "stale_enum"),
              "enum metadata miss should report the stale rendered reference");
}

void test_no_metadata_reference_keeps_rendered_compatibility() {
  c4c::Node stale = global_var("rendered_only", kStaleText);
  c4c::Node ref = var_ref("rendered_only", "rendered_only",
                          c4c::kInvalidText, -1);

  const c4c::sema::ValidateResult result = validate_single_return(stale, ref);

  expect_true(result.ok,
              "rendered-name fallback should remain available without metadata");
}

void test_no_metadata_function_reference_keeps_rendered_compatibility() {
  c4c::Node rendered =
      function_decl("rendered_function", "rendered_function", kStaleText,
                    int_pointer_type());
  c4c::Node callee = var_ref("rendered_function", "rendered_function",
                             c4c::kInvalidText, -1);
  c4c::Node call = call_expr(&callee);

  const c4c::sema::ValidateResult result =
      validate_single_return(rendered, call);

  expect_true(!result.ok,
              "rendered function lookup should remain available without metadata");
  expect_true(has_diagnostic_message(result, "incompatible return type"),
              "no-metadata function reference should use rendered function signature");
}

}  // namespace

int main() {
  test_function_metadata_miss_rejects_stale_rendered_name();
  test_global_metadata_miss_rejects_stale_rendered_name();
  test_enum_metadata_miss_rejects_stale_rendered_name();
  test_no_metadata_reference_keeps_rendered_compatibility();
  test_no_metadata_function_reference_keeps_rendered_compatibility();
  std::cout << "PASS: cpp_hir_sema_lookup_value_metadata_test\n";
  return 0;
}
