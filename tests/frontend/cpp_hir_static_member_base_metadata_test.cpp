#include "hir/impl/templates/templates.hpp"
#include "parser/ast.hpp"

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

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

c4c::Node make_int_literal(long long value) {
  c4c::Node lit{};
  lit.kind = c4c::NK_INT_LIT;
  lit.ival = value;
  return lit;
}

c4c::Node make_static_value_decl(const char* name, c4c::Node* init) {
  c4c::Node decl{};
  decl.kind = c4c::NK_DECL;
  decl.is_static = true;
  decl.name = name;
  decl.init = init;
  return decl;
}

c4c::Node make_record(const char* name, c4c::TextId text_id, int namespace_id,
                      c4c::Node** fields, int field_count) {
  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = name;
  record.unqualified_name = name;
  record.unqualified_text_id = text_id;
  record.namespace_context_id = namespace_id;
  record.fields = fields;
  record.n_fields = field_count;
  return record;
}

void test_static_member_base_lookup_prefers_record_def_over_stale_tag() {
  c4c::Node good_value = make_int_literal(42);
  c4c::Node stale_value = make_int_literal(7);
  c4c::Node good_decl = make_static_value_decl("value", &good_value);
  c4c::Node stale_decl = make_static_value_decl("value", &stale_value);
  c4c::Node* good_fields[] = {&good_decl};
  c4c::Node* stale_fields[] = {&stale_decl};
  c4c::Node good_base = make_record("StructuredBase", 101, 3, good_fields, 1);
  c4c::Node stale_base =
      make_record("StaleRenderedBase", 102, 3, stale_fields, 1);

  c4c::TypeSpec base_ts{};
  base_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(base_ts, "StaleRenderedBase", 0);
  base_ts.record_def = &good_base;

  c4c::Node derived = make_record("DerivedRecordDef", 201, 3, nullptr, 0);
  derived.base_types = &base_ts;
  derived.n_bases = 1;

  const std::unordered_map<std::string, const c4c::Node*> struct_defs = {
      {"StructuredBase", &good_base},
      {"StaleRenderedBase", &stale_base},
  };

  long long out = 0;
  expect_true(c4c::hir::eval_struct_static_member_value_hir(
                  &derived, struct_defs, "value", nullptr, &out),
              "record_def base metadata should resolve inherited static member");
  expect_true(out == 42,
              "record_def base metadata must win over stale rendered tag");
}

void test_static_member_base_lookup_prefers_text_id_over_stale_tag() {
  c4c::Node good_value = make_int_literal(43);
  c4c::Node stale_value = make_int_literal(9);
  c4c::Node good_decl = make_static_value_decl("value", &good_value);
  c4c::Node stale_decl = make_static_value_decl("value", &stale_value);
  c4c::Node* good_fields[] = {&good_decl};
  c4c::Node* stale_fields[] = {&stale_decl};
  c4c::Node good_base = make_record("StructuredTextBase", 301, 5, good_fields, 1);
  c4c::Node stale_base =
      make_record("StaleTextRenderedBase", 302, 5, stale_fields, 1);

  c4c::TypeSpec base_ts{};
  base_ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(base_ts, "StaleTextRenderedBase", 0);
  base_ts.tag_text_id = 301;
  base_ts.namespace_context_id = 5;

  c4c::Node derived = make_record("DerivedTextId", 401, 5, nullptr, 0);
  derived.base_types = &base_ts;
  derived.n_bases = 1;

  const std::unordered_map<std::string, const c4c::Node*> struct_defs = {
      {"StructuredTextBase", &good_base},
      {"StaleTextRenderedBase", &stale_base},
  };

  long long out = 0;
  expect_true(c4c::hir::eval_struct_static_member_value_hir(
                  &derived, struct_defs, "value", nullptr, &out),
              "tag_text_id base metadata should resolve inherited static member");
  expect_true(out == 43,
              "tag_text_id base metadata must win over stale rendered tag");
}

}  // namespace

int main() {
  test_static_member_base_lookup_prefers_record_def_over_stale_tag();
  test_static_member_base_lookup_prefers_text_id_over_stale_tag();
  std::cout << "PASS: cpp_hir_static_member_base_metadata_test\n";
  return 0;
}
