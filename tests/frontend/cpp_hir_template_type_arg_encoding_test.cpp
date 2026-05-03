#include "hir/hir_ir.hpp"
#include "parser/ast.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

c4c::Node make_record(std::string_view name) {
  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = name.data();
  record.unqualified_name = name.data();
  record.unqualified_text_id = 17;
  record.namespace_context_id = 3;
  return record;
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

void test_template_type_arg_encoding_prefers_record_def_over_stale_tag() {
  c4c::Node record = make_record("StructuredArg");

  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  set_legacy_tag_if_present(ts, "StaleRenderedArg", 0);
  ts.record_def = &record;

  expect_eq(c4c::hir::encode_template_type_arg_ref_hir(ts), "StructuredArg",
            "plain template type-arg encoding should prefer record_def over stale tag");

  ts.is_const = true;
  expect_eq(c4c::hir::encode_template_type_arg_ref_hir(ts),
            "const_struct_StructuredArg",
            "decorated template type-arg encoding should prefer record_def over stale tag");
}

void test_template_type_arg_encoding_prefers_text_id_over_stale_tag() {
  c4c::TextId qualifier_ids[] = {11, 12};

  c4c::TypeSpec ts{};
  ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(ts, "StaleRenderedTypedef", 0);
  ts.tag_text_id = 23;
  ts.namespace_context_id = 5;
  ts.is_global_qualified = true;
  ts.qualifier_text_ids = qualifier_ids;
  ts.n_qualifier_segments = 2;

  expect_eq(c4c::hir::encode_template_type_arg_ref_hir(ts),
            "tag_ctx5_global_q11_12_text23",
            "template type-arg encoding should prefer tag_text_id over stale tag");
}

}  // namespace

int main() {
  test_template_type_arg_encoding_prefers_record_def_over_stale_tag();
  test_template_type_arg_encoding_prefers_text_id_over_stale_tag();
  std::cout << "PASS: cpp_hir_template_type_arg_encoding_test\n";
  return 0;
}
