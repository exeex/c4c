#include "lexer.hpp"
#include "impl/parser_impl.hpp"
#include "impl/types/types_helpers.hpp"
#include "parser.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

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
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

void test_reparse_tokens_use_text_identity_before_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId ns_text_id = texts.intern("RealNs");
  const c4c::TextId real_text_id = texts.intern("RealRecord");
  c4c::TextId qualifiers[] = {ns_text_id};
  c4c::TypeSpec ts = make_struct(real_text_id, "StaleNs::StaleRecord");
  ts.qualifier_text_ids = qualifiers;
  ts.n_qualifier_segments = 1;

  c4c::Token seed{};
  std::vector<c4c::Token> tokens;
  c4c::append_typespec_reparse_tokens(parser, &tokens, seed, ts);

  std::string rendered;
  for (const c4c::Token& token : tokens) {
    rendered += parser.token_spelling(token);
  }
  expect_true(rendered == "RealNs::RealRecord",
              "reparse tokens should use qualifier/tag TextIds before rendered tag");
}

void test_specialization_score_uses_template_param_text_identity() {
  c4c::TextId param_text_id = 41;

  c4c::TypeSpec structured_arg{};
  structured_arg.base = c4c::TB_TYPEDEF;
  structured_arg.tag_text_id = param_text_id;
  set_legacy_tag_if_present(structured_arg, "StaleParam", 0);

  const char* param_names[] = {"T"};
  c4c::TextId param_text_ids[] = {param_text_id};
  bool param_is_nttp[] = {false};
  c4c::TypeSpec arg_types[] = {structured_arg};
  bool arg_is_value[] = {false};

  c4c::Node spec{};
  spec.n_template_params = 1;
  spec.template_param_names = param_names;
  spec.template_param_name_text_ids = param_text_ids;
  spec.template_param_is_nttp = param_is_nttp;
  spec.n_template_args = 1;
  spec.template_arg_types = arg_types;
  spec.template_arg_is_value = arg_is_value;

  expect_true(c4c::specialization_match_score(&spec) == 0,
              "specialization scoring should recognize structured template param identity");

  arg_types[0].tag_text_id = c4c::kInvalidText;
  expect_true(c4c::specialization_match_score(&spec) == 4,
              "specialization scoring should penalize missing structured template param identity");
}

void test_canonical_key_uses_text_identity_before_rendered_tag() {
  c4c::TypeSpec ts = make_struct(77, "StaleRecord");
  const std::string key = c4c::canonical_template_struct_type_key(ts);
  expect_true(key.find("text#77") != std::string::npos,
              "canonical key should encode structured tag TextId");
  expect_true(key.find("StaleRecord") == std::string::npos,
              "canonical key should not prefer stale rendered tag");
}

void test_mangling_uses_record_metadata_before_rendered_tag() {
  c4c::Node record{};
  record.name = "RealRecord";
  record.unqualified_name = "RealRecord";
  record.unqualified_text_id = 91;

  c4c::TypeSpec ts = make_struct(91, "StaleRecord");
  ts.record_def = &record;

  std::string mangled;
  c4c::append_type_mangled_suffix(mangled, ts);
  expect_true(mangled == "struct_RealRecord",
              "mangled suffix should use record metadata before rendered tag");

  ts.record_def = nullptr;
  set_legacy_tag_if_present(ts, "CompatRecord", 0);
  mangled.clear();
  c4c::append_type_mangled_suffix(mangled, ts);
  expect_true(mangled == "struct_anon",
              "mangled suffix should use anonymous fallback when structured record metadata is absent");
}

}  // namespace

int main() {
  test_reparse_tokens_use_text_identity_before_rendered_tag();
  test_specialization_score_uses_template_param_text_identity();
  test_canonical_key_uses_text_identity_before_rendered_tag();
  test_mangling_uses_record_metadata_before_rendered_tag();
  std::cout << "PASS: cpp_hir_parser_type_helper_residual_metadata_test\n";
  return 0;
}
