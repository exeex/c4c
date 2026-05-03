#include "sema/consteval.hpp"
#include "sema/type_utils.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

void expect_eq_int(int actual, int expected, const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::to_string(expected) +
         "\nActual: " + std::to_string(actual));
  }
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

c4c::Node sizeof_type_node(const c4c::TypeSpec& ts) {
  c4c::Node n{};
  n.kind = c4c::NK_SIZEOF_TYPE;
  n.type = ts;
  return n;
}

c4c::TypeSpec record_ref(c4c::TextId text_id, const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.namespace_context_id = 3;
  ts.tag_text_id = text_id;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  return ts;
}

c4c::hir::HirStructDef struct_def(std::string tag,
                                  c4c::TextId text_id,
                                  int size_bytes) {
  c4c::hir::HirStructDef def{};
  def.tag = std::move(tag);
  def.tag_text_id = text_id;
  def.ns_qual.context_id = 3;
  def.size_bytes = size_bytes;
  return def;
}

void test_consteval_sizeof_prefers_owner_metadata_over_stale_rendered_tag() {
  constexpr c4c::TextId real_text = 41;
  constexpr c4c::TextId stale_text = 42;

  std::unordered_map<std::string, c4c::hir::HirStructDef> struct_defs;
  struct_defs.emplace("RealRecord", struct_def("RealRecord", real_text, 24));
  struct_defs.emplace("StaleRecord", struct_def("StaleRecord", stale_text, 8));

  c4c::hir::HirRecordOwnerKey real_key =
      c4c::hir::make_hir_record_owner_key(struct_defs.at("RealRecord"));
  std::unordered_map<c4c::hir::HirRecordOwnerKey, std::string,
                     c4c::hir::HirRecordOwnerKeyHash>
      owner_index;
  owner_index.emplace(real_key, "RealRecord");

  c4c::hir::ConstEvalEnv env{};
  env.struct_defs = &struct_defs;
  env.struct_def_owner_index = &owner_index;

  c4c::Node n = sizeof_type_node(record_ref(real_text, "StaleRecord"));
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&n, env);

  expect_true(result.ok(),
              "sizeof should resolve record layout through owner metadata");
  expect_eq_int(static_cast<int>(result.as_int()), 24,
                "stale rendered tag must not select the compatibility layout");
}

void test_consteval_sizeof_metadata_miss_rejects_stale_rendered_tag() {
  constexpr c4c::TextId real_text = 41;
  constexpr c4c::TextId stale_text = 42;

  std::unordered_map<std::string, c4c::hir::HirStructDef> struct_defs;
  struct_defs.emplace("StaleRecord", struct_def("StaleRecord", stale_text, 8));
  std::unordered_map<c4c::hir::HirRecordOwnerKey, std::string,
                     c4c::hir::HirRecordOwnerKeyHash>
      owner_index;

  c4c::hir::ConstEvalEnv env{};
  env.struct_defs = &struct_defs;
  env.struct_def_owner_index = &owner_index;

  c4c::Node n = sizeof_type_node(record_ref(real_text, "StaleRecord"));
  const c4c::hir::ConstEvalResult result =
      c4c::hir::evaluate_constant_expr(&n, env);

  expect_true(!result.ok(),
              "structured owner metadata miss should not recover through stale rendered tag");
}

void test_type_binding_equivalence_rejects_rendered_name_when_metadata_exists() {
  c4c::TypeSpec lhs = record_ref(51, "RenderedSame");
  c4c::TypeSpec rhs = record_ref(52, "RenderedSame");

  expect_true(!c4c::type_binding_values_equivalent(lhs, rhs),
              "text-name metadata should block rendered-name compatibility");
}

void test_type_binding_equivalence_keeps_no_metadata_rendered_compatibility() {
  c4c::TypeSpec lhs{};
  lhs.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(lhs, "RenderedOnly", 0);

  c4c::TypeSpec rhs{};
  rhs.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(rhs, "RenderedOnly", 0);

  expect_true(c4c::type_binding_values_equivalent(lhs, rhs),
              "rendered tags remain a no-metadata compatibility fallback");
}

}  // namespace

int main() {
  test_consteval_sizeof_prefers_owner_metadata_over_stale_rendered_tag();
  test_consteval_sizeof_metadata_miss_rejects_stale_rendered_tag();
  test_type_binding_equivalence_rejects_rendered_name_when_metadata_exists();
  test_type_binding_equivalence_keeps_no_metadata_rendered_compatibility();
  std::cout << "PASS: cpp_hir_sema_consteval_type_utils_metadata_test\n";
  return 0;
}
