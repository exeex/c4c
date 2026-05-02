#include "parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool condition, const std::string& msg) {
  if (!condition) fail(msg);
}

c4c::TypeSpec make_struct_ref(c4c::TextId name_text_id,
                              const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.tag = rendered_tag;
  ts.tag_text_id = name_text_id;
  ts.namespace_context_id = 0;
  ts.n_qualifier_segments = 0;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void test_nominal_identity_prefers_text_metadata_over_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId box_text_id = texts.intern("Box");
  c4c::TypeSpec lhs = make_struct_ref(box_text_id, "StaleLhs");
  c4c::TypeSpec rhs = make_struct_ref(box_text_id, "StaleRhs");

  expect_true(parser.are_types_compatible(lhs, rhs),
              "nominal TypeSpec identity should use matching tag_text_id metadata before stale rendered tags");

  rhs.tag = "StaleLhs";
  rhs.tag_text_id = texts.intern("OtherBox");
  expect_true(!parser.are_types_compatible(lhs, rhs),
              "nominal TypeSpec identity should reject mismatched tag_text_id metadata despite matching rendered tags");
}

}  // namespace

int main() {
  test_nominal_identity_prefers_text_metadata_over_rendered_tag();
  std::cout << "PASS: cpp_hir_parser_core_nominal_typespec_metadata_test\n";
  return 0;
}
