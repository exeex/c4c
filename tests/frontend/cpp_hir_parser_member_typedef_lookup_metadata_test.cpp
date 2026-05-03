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

c4c::Node* make_owner(c4c::Parser& parser,
                      c4c::Arena& arena,
                      const char* name,
                      c4c::TextId name_text_id,
                      c4c::TypeBase alias_base) {
  c4c::Node* owner = parser.make_node(c4c::NK_STRUCT_DEF, 1);
  owner->name = arena.strdup(name);
  owner->unqualified_name = arena.strdup(name);
  owner->unqualified_text_id = name_text_id;
  owner->namespace_context_id = parser.current_namespace_context_id();
  owner->n_fields = 0;
  owner->n_member_typedefs = 1;
  owner->member_typedef_names = arena.alloc_array<const char*>(1);
  owner->member_typedef_names[0] = arena.strdup("Alias");
  owner->member_typedef_text_ids = arena.alloc_array<c4c::TextId>(1);
  owner->member_typedef_text_ids[0] =
      parser.parser_text_id_for_token(c4c::kInvalidText, "Alias");
  owner->member_typedef_types = arena.alloc_array<c4c::TypeSpec>(1);
  owner->member_typedef_types[0].array_size = -1;
  owner->member_typedef_types[0].inner_rank = -1;
  owner->member_typedef_types[0].base = alias_base;
  return owner;
}

void test_member_typedef_lookup_uses_owner_text_id_before_stale_tag() {
  c4c::Arena arena;
  c4c::TextTable texts;
  c4c::FileTable files;
  c4c::Parser parser({}, arena, &texts, &files, c4c::SourceProfile::CppSubset);
  c4c::Token seed{};

  const c4c::Token real_owner_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "RealOwner");
  const c4c::Token stale_owner_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "StaleOwner");
  const c4c::Token alias_name_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "OwnerAlias");
  const c4c::Token member_token =
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "Alias");

  c4c::Node* real_owner =
      make_owner(parser, arena, "RealOwner", real_owner_token.text_id,
                 c4c::TB_INT);
  c4c::Node* stale_owner =
      make_owner(parser, arena, "StaleOwner", stale_owner_token.text_id,
                 c4c::TB_DOUBLE);
  parser.register_struct_definition_for_testing("RealOwner", real_owner);
  parser.register_struct_definition_for_testing("StaleOwner", stale_owner);

  c4c::TypeSpec owner_alias{};
  owner_alias.array_size = -1;
  owner_alias.inner_rank = -1;
  owner_alias.base = c4c::TB_STRUCT;
  owner_alias.tag = arena.strdup("StaleOwner");
  owner_alias.tag_text_id = real_owner_token.text_id;
  parser.register_typedef_binding(alias_name_token.text_id, owner_alias, true);

  parser.replace_token_stream_for_testing({
      alias_name_token,
      parser.make_injected_token(seed, c4c::TokenKind::ColonColon, "::"),
      member_token,
      parser.make_injected_token(seed, c4c::TokenKind::Identifier, "after"),
  });

  const c4c::TypeSpec resolved = parser.parse_base_type();
  expect_true(resolved.base == c4c::TB_INT,
              "member typedef lookup should use structured owner TextId before stale rendered tag");
}

}  // namespace

int main() {
  test_member_typedef_lookup_uses_owner_text_id_before_stale_tag();
  std::cout << "PASS: cpp_hir_parser_member_typedef_lookup_metadata_test\n";
  return 0;
}
