#include "lexer.hpp"
#include "impl/parser_impl.hpp"
#include "impl/types/types_helpers.hpp"
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

c4c::TypeSpec make_struct(c4c::TextId name_text_id,
                          const char* rendered_tag) {
  c4c::TypeSpec ts{};
  ts.base = c4c::TB_STRUCT;
  ts.tag_text_id = name_text_id;
  set_legacy_tag_if_present(ts, rendered_tag, 0);
  ts.namespace_context_id = 0;
  ts.n_qualifier_segments = 0;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

c4c::Node* make_record(c4c::Parser& parser, const char* name,
                       c4c::TextId name_text_id) {
  c4c::Node* record = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  record->name = name;
  record->unqualified_name = name;
  record->unqualified_text_id = name_text_id;
  record->namespace_context_id = 0;
  parser.definition_state_.struct_tag_def_map[name] = record;
  return record;
}

void test_visible_type_head_name_uses_text_identity_before_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId alias_text_id = texts.intern("Alias");
  const c4c::TextId real_text_id = texts.intern("RealRecord");
  parser.register_typedef_binding(
      alias_text_id, make_struct(real_text_id, "StaleRenderedRecord"), true);

  const std::string visible_name =
      c4c::visible_type_head_name(parser, alias_text_id, "Alias");
  expect_true(visible_name == "RealRecord",
              "visible type head should use tag_text_id before rendered tag");
}

void test_record_lookup_uses_text_identity_before_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId real_text_id = texts.intern("RealRecord");
  const c4c::TextId stale_text_id = texts.intern("StaleRenderedRecord");
  c4c::Node* real_record = make_record(parser, "RealRecord", real_text_id);
  c4c::Node* stale_record =
      make_record(parser, "StaleRenderedRecord", stale_text_id);

  c4c::TypeSpec structured = make_struct(real_text_id, "StaleRenderedRecord");
  expect_true(c4c::type_spec_structured_record_definition(
                  parser, &structured) == real_record,
              "record lookup should use tag_text_id before rendered tag");

  c4c::TypeSpec legacy = make_struct(c4c::kInvalidText,
                                     "StaleRenderedRecord");
  expect_true(c4c::type_spec_structured_record_definition(
                  parser, &legacy) == stale_record,
              "record lookup should preserve no-metadata rendered-tag compatibility");
}

}  // namespace

int main() {
  test_visible_type_head_name_uses_text_identity_before_rendered_tag();
  test_record_lookup_uses_text_identity_before_rendered_tag();
  std::cout << "PASS: cpp_hir_parser_type_helper_metadata_test\n";
  return 0;
}
