#include "parser/ast.hpp"
#include "sema/canonical_symbol.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

template <typename T>
auto legacy_tag_if_present(const T& ts, int) -> decltype(ts.tag) {
  return ts.tag;
}

const char* legacy_tag_if_present(const c4c::TypeSpec&, long) {
  return nullptr;
}

c4c::Node make_record(std::string_view name) {
  c4c::Node record{};
  record.kind = c4c::NK_STRUCT_DEF;
  record.name = name.data();
  record.unqualified_name = name.data();
  record.unqualified_text_id = 42;
  record.namespace_context_id = 7;
  return record;
}

void test_canonicalize_type_prefers_record_def_over_stale_tag() {
  c4c::Node record = make_record("StructuredCanonical");

  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.record_def = &record;
  set_legacy_tag_if_present(ts, "StaleCanonical", 0);

  const c4c::sema::CanonicalType ct = c4c::sema::canonicalize_type(ts);
  expect_eq(ct.user_spelling, "StructuredCanonical",
            "canonical leaf spelling should prefer record_def metadata");
}

void test_typespec_from_canonical_preserves_final_spelling_compatibility() {
  c4c::sema::CanonicalType ct{};
  ct.kind = c4c::sema::CanonicalTypeKind::Struct;
  ct.source_base = c4c::TB_STRUCT;
  ct.user_spelling = "RenderedCanonical";

  const c4c::TypeSpec ts = c4c::sema::typespec_from_canonical(ct);

  expect_true(ts.base == c4c::TB_STRUCT,
              "canonical struct should reconstruct a struct TypeSpec");
  const char* final_spelling = legacy_tag_if_present(ts, 0);
  if (final_spelling) {
    expect_eq(final_spelling, "RenderedCanonical",
              "TypeSpec reconstruction should retain canonical final spelling");
  }
}

}  // namespace

int main() {
  test_canonicalize_type_prefers_record_def_over_stale_tag();
  test_typespec_from_canonical_preserves_final_spelling_compatibility();
  std::cout << "PASS: cpp_hir_sema_canonical_symbol_metadata_test\n";
  return 0;
}
