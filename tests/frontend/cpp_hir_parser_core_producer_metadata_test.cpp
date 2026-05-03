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

void test_parser_core_producers_store_structured_type_identity() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId synth_text_id = texts.intern("SynthParam");
  parser.register_synthesized_typedef_binding(synth_text_id);
  const c4c::TypeSpec* synth_type = parser.find_typedef_type(synth_text_id);
  expect_true(synth_type != nullptr,
              "synthesized typedef binding should store a typedef type");
  expect_true(synth_type->base == c4c::TB_TYPEDEF,
              "synthesized typedef binding should keep typedef base");
  expect_true(synth_type->tag_text_id == synth_text_id,
              "synthesized typedef binding should carry TextId identity");

  const c4c::TextId alias_text_id = texts.intern("Alias");
  const c4c::TextId real_record_text_id = texts.intern("RealRecord");
  parser.register_tag_type_binding(alias_text_id, c4c::TB_STRUCT,
                                   "RenderedCompatibilityRecord",
                                   real_record_text_id, c4c::TB_VOID);
  const c4c::TypeSpec* tag_type = parser.find_typedef_type(alias_text_id);
  expect_true(tag_type != nullptr,
              "tag type binding should store a typedef type");
  expect_true(tag_type->base == c4c::TB_STRUCT,
              "tag type binding should preserve the record base");
  expect_true(tag_type->tag_text_id == real_record_text_id,
              "tag type binding should carry explicit tag TextId identity");

  const c4c::TextId true_text_id = texts.intern("__true_type");
  const c4c::TextId false_text_id = texts.intern("__false_type");
  const c4c::TypeSpec* true_type = parser.find_typedef_type(true_text_id);
  const c4c::TypeSpec* false_type = parser.find_typedef_type(false_text_id);
  expect_true(true_type != nullptr && true_type->base == c4c::TB_STRUCT,
              "builtin __true_type should be cached as a struct type");
  expect_true(false_type != nullptr && false_type->base == c4c::TB_STRUCT,
              "builtin __false_type should be cached as a struct type");
  expect_true(true_type->tag_text_id == true_text_id,
              "builtin __true_type should carry TextId identity");
  expect_true(false_type->tag_text_id == false_text_id,
              "builtin __false_type should carry TextId identity");

  const c4c::TextId qualified_true_text_id = texts.intern("std::__true_type");
  const c4c::TypeSpec* qualified_true_type =
      parser.find_typedef_type(qualified_true_text_id);
  expect_true(qualified_true_type != nullptr &&
                  qualified_true_type->tag_text_id == true_text_id,
              "qualified builtin true typedef should keep unqualified semantic tag identity");
}

}  // namespace

int main() {
  test_parser_core_producers_store_structured_type_identity();
  std::cout << "PASS: cpp_hir_parser_core_producer_metadata_test\n";
  return 0;
}
