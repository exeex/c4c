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

template <typename T>
auto set_legacy_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
  ts.tag = tag;
}

void set_legacy_tag_if_present(c4c::TypeSpec&, const char*, long) {}

c4c::TypeSpec make_struct(c4c::TextId name_text_id) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.tag_text_id = name_text_id;
  ts.namespace_context_id = 0;
  ts.n_qualifier_segments = 0;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

c4c::TypeSpec make_struct_like_typedef_ref(c4c::TextId name_text_id,
                                           const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.tag_text_id = name_text_id;
  ts.namespace_context_id = 0;
  ts.n_qualifier_segments = 0;
  ts.ptr_level = 1;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void test_struct_like_typedef_prefers_metadata_over_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId alias_text_id = texts.intern("StructAlias");
  const c4c::TextId stale_text_id = texts.intern("StaleAlias");
  const c4c::TextId missing_text_id = texts.intern("MissingAlias");
  const c4c::TextId box_text_id = texts.intern("Box");
  const c4c::TextId stale_box_text_id = texts.intern("StaleBox");

  parser.register_typedef_binding(alias_text_id, make_struct(box_text_id), true);
  parser.register_typedef_binding(stale_text_id,
                                  make_struct(stale_box_text_id), true);

  const c4c::TypeSpec resolved = parser.resolve_struct_like_typedef_type(
      make_struct_like_typedef_ref(alias_text_id, "StaleAlias"));
  expect_true(resolved.base == c4c::TB_STRUCT &&
                  resolved.tag_text_id == box_text_id,
              "struct-like typedef resolution should use tag_text_id metadata before stale rendered tags");

  const c4c::TypeSpec unresolved = parser.resolve_struct_like_typedef_type(
      make_struct_like_typedef_ref(missing_text_id, "StructAlias"));
  expect_true(unresolved.base == c4c::TB_TYPEDEF &&
                  unresolved.tag_text_id == missing_text_id,
              "struct-like typedef resolution should not fall back to rendered tags when structured metadata is present but misses");
}

}  // namespace

int main() {
  test_struct_like_typedef_prefers_metadata_over_rendered_tag();
  std::cout << "PASS: cpp_hir_parser_core_struct_like_typedef_metadata_test\n";
  return 0;
}
