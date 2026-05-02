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

c4c::TypeSpec make_typedef_ref(c4c::TextId name_text_id,
                               const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.tag_text_id = name_text_id;
  ts.namespace_context_id = 0;
  ts.n_qualifier_segments = 0;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

c4c::TypeSpec make_legacy_typedef_ref(const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_TYPEDEF;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.tag_text_id = c4c::kInvalidText;
  ts.namespace_context_id = -1;
  ts.n_qualifier_segments = 0;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void test_record_ctor_type_prefers_metadata_over_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId box_text_id = texts.intern("Box");
  const c4c::TextId missing_text_id = texts.intern("MissingBox");

  c4c::Node* box_def = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  box_def->name = "Box";
  box_def->unqualified_text_id = box_text_id;
  box_def->namespace_context_id = 0;
  parser.definition_state_.struct_tag_def_map["Box"] = box_def;
  parser.definition_state_.defined_struct_tags.insert("StaleBox");

  expect_true(parser.resolves_to_record_ctor_type(
                  make_typedef_ref(box_text_id, "StaleBox")),
              "record constructor detection should use tag_text_id metadata before stale rendered tags");

  expect_true(!parser.resolves_to_record_ctor_type(
                  make_typedef_ref(missing_text_id, "StaleBox")),
              "record constructor detection should not fall back to rendered tags when structured metadata is present but misses");

  expect_true(parser.resolves_to_record_ctor_type(
                  make_legacy_typedef_ref("StaleBox")),
              "record constructor detection should preserve explicit no-metadata legacy tag compatibility");
}

}  // namespace

int main() {
  test_record_ctor_type_prefers_metadata_over_rendered_tag();
  std::cout << "PASS: cpp_hir_parser_core_record_ctor_metadata_test\n";
  return 0;
}
