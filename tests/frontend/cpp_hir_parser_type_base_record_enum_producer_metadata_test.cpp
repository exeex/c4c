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

void test_record_and_enum_producers_survive_missing_rendered_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};
  const c4c::Token record_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "CarrierRecord");

  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwStruct, "struct"),
      record_token,
      parser.make_injected_token(seed, c4c::TokenKind::LBrace, "{"),
      parser.make_injected_token(seed, c4c::TokenKind::KwInt, "int"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "field"),
      parser.make_injected_token(seed, c4c::TokenKind::Semi, ";"),
      parser.make_injected_token(seed, c4c::TokenKind::RBrace, "}"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  c4c::TypeSpec record_ts = parser.parse_base_type();
  set_legacy_tag_if_present(record_ts, nullptr, 0);
  expect_true(record_ts.base == c4c::TB_STRUCT &&
                  record_ts.record_def &&
                  record_ts.tag_text_id == record_token.text_id,
              "record producer should expose record_def/tag_text_id without rendered tag");

  const c4c::Token enum_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "CarrierEnum");
  parser.replace_token_stream_for_testing({
      parser.make_injected_token(seed, c4c::TokenKind::KwEnum, "enum"),
      enum_token,
      parser.make_injected_token(seed, c4c::TokenKind::Colon, ":"),
      parser.make_injected_token(seed, c4c::TokenKind::KwUnsigned, "unsigned"),
      parser.make_injected_token(seed, c4c::TokenKind::KwChar, "char"),
      parser.make_injected_token(seed, c4c::TokenKind::LBrace, "{"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "CarrierA"),
      parser.make_injected_token(seed, c4c::TokenKind::RBrace, "}"),
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  c4c::TypeSpec enum_ts = parser.parse_base_type();
  set_legacy_tag_if_present(enum_ts, nullptr, 0);
  expect_true(enum_ts.base == c4c::TB_ENUM &&
                  enum_ts.tag_text_id == enum_token.text_id &&
                  enum_ts.enum_underlying_base == c4c::TB_UCHAR,
              "enum producer should expose tag_text_id/underlying metadata without rendered tag");
}

}  // namespace

int main() {
  test_record_and_enum_producers_survive_missing_rendered_tag();
  std::cout
      << "PASS: cpp_hir_parser_type_base_record_enum_producer_metadata_test\n";
  return 0;
}
