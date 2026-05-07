#include "impl/parser_impl.hpp"

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
    -> decltype(ts.tag = tag, bool()) {
  ts.tag = tag;
  return true;
}

bool set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {
  return false;
}

c4c::TypeSpec typedef_type(c4c::TextId tag_text_id, const char* rendered_tag) {
  c4c::TypeSpec ts = {};
  ts.base = c4c::TB_TYPEDEF;
  ts.enum_underlying_base = c4c::TB_VOID;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.tag_text_id = tag_text_id;
  ts.template_param_text_id = c4c::kInvalidText;
  ts.array_size = -1;
  return ts;
}

c4c::Node sizeof_type_node(const c4c::TypeSpec& ts) {
  c4c::Node n = {};
  n.kind = c4c::NK_SIZEOF_TYPE;
  n.type = ts;
  return n;
}

void test_enum_sizeof_dependency_uses_structured_typedef_identity() {
  const c4c::TextId real_typedef = 11;
  c4c::Node n = sizeof_type_node(typedef_type(real_typedef, "StaleRendered"));
  c4c::ParserEnumConstTable consts;
  long long out = 0;

  expect_true(!c4c::eval_enum_expr(&n, consts, &out),
              "enum sizeof typedef should remain dependent by tag_text_id");
  expect_true(c4c::is_dependent_enum_expr(&n, consts),
              "dependent enum expression should use tag_text_id before stale rendered tag");

  set_legacy_tag_if_present(n.type, nullptr, 0);
  expect_true(!c4c::eval_enum_expr(&n, consts, &out),
              "tag_text_id-only typedef should remain dependent without rendered tag");
  expect_true(c4c::is_dependent_enum_expr(&n, consts),
              "tag_text_id-only typedef should be detected as dependent");

  n.type.tag_text_id = c4c::kInvalidText;
  if (set_legacy_tag_if_present(n.type, "LegacyOnly", 0)) {
    expect_true(!c4c::eval_enum_expr(&n, consts, &out),
                "legacy rendered typedef tag remains explicit compatibility fallback");
    expect_true(c4c::is_dependent_enum_expr(&n, consts),
                "legacy rendered typedef fallback should still classify dependency");
  }
}

void test_record_layout_resolution_uses_record_definition_before_stale_tag() {
  c4c::Node real_record = {};
  real_record.kind = c4c::NK_STRUCT_DEF;
  real_record.name = "RealRecord";
  real_record.unqualified_text_id = 21;

  c4c::Node stale_record = {};
  stale_record.kind = c4c::NK_STRUCT_DEF;
  stale_record.name = "StaleRendered";
  stale_record.unqualified_text_id = 22;

  std::unordered_map<std::string, c4c::Node*> records;
  records.emplace("RealRecord", &real_record);
  records.emplace("StaleRendered", &stale_record);

  c4c::TypeSpec query = {};
  query.base = c4c::TB_STRUCT;
  query.enum_underlying_base = c4c::TB_VOID;
  set_legacy_tag_if_present(query, "StaleRendered", 0);
  query.tag_text_id = real_record.unqualified_text_id;
  query.record_def = &real_record;
  query.array_size = -1;

  expect_true(c4c::resolve_record_type_spec(query, &records) == &real_record,
              "record layout lookup should use record_def before stale rendered tag");

  query.record_def = nullptr;
  expect_true(c4c::resolve_record_type_spec(query, &records) == &real_record,
              "public parser support keeps bounded tag_text_id compatibility for non-layout callers");

  query.tag_text_id = 99;
  expect_true(c4c::resolve_record_type_spec(query, &records) == nullptr,
              "structured record metadata miss should not recover through stale rendered tag");

  query.tag_text_id = c4c::kInvalidText;
  if (set_legacy_tag_if_present(query, "StaleRendered", 0)) {
    expect_true(c4c::resolve_record_type_spec(query, &records) == &stale_record,
                "legacy rendered record tag remains explicit compatibility fallback");
  }

  c4c::Node incomplete_record = {};
  incomplete_record.kind = c4c::NK_STRUCT_DEF;
  incomplete_record.name = "RealRecord";
  incomplete_record.unqualified_text_id = real_record.unqualified_text_id;
  incomplete_record.namespace_context_id = 7;
  incomplete_record.n_fields = -1;
  query.record_def = &incomplete_record;
  query.tag_text_id = real_record.unqualified_text_id;
  query.namespace_context_id = incomplete_record.namespace_context_id;
  expect_true(c4c::resolve_record_type_spec(query, &records) == &incomplete_record,
              "structured incomplete record_def should not complete through parser rendered-tag map");
}

}  // namespace

int main() {
  test_enum_sizeof_dependency_uses_structured_typedef_identity();
  test_record_layout_resolution_uses_record_definition_before_stale_tag();
  std::cout << "PASS: cpp_hir_parser_support_residual_metadata_test\n";
  return 0;
}
