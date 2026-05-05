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

c4c::TypeSpec make_scalar(c4c::TypeBase base) {
  c4c::TypeSpec ts{};
  ts.base = base;
  ts.array_size = -1;
  ts.inner_rank = -1;
  return ts;
}

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

void test_typedef_chain_prefers_text_metadata_over_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);

  const c4c::TextId alias_text_id = texts.intern("Alias");
  const c4c::TextId stale_text_id = texts.intern("StaleAlias");
  const c4c::TextId missing_text_id = texts.intern("MissingAlias");

  parser.register_typedef_binding(alias_text_id, make_scalar(c4c::TB_INT), true);
  parser.register_typedef_binding(stale_text_id, make_scalar(c4c::TB_DOUBLE), true);

  const c4c::TypeSpec resolved =
      parser.resolve_typedef_type_chain(make_typedef_ref(alias_text_id, "StaleAlias"));
  expect_true(resolved.base == c4c::TB_INT,
              "typedef chain should resolve by tag_text_id metadata before stale rendered tags");

  const c4c::TypeSpec unresolved =
      parser.resolve_typedef_type_chain(make_typedef_ref(missing_text_id, "Alias"));
  expect_true(unresolved.base == c4c::TB_TYPEDEF &&
                  unresolved.tag_text_id == missing_text_id,
              "typedef chain should not fall back to rendered tags when structured metadata is present but misses");
}

}  // namespace

int main() {
  test_typedef_chain_prefers_text_metadata_over_rendered_tag();
  std::cout << "PASS: cpp_hir_parser_core_typedef_chain_metadata_test\n";
  return 0;
}
