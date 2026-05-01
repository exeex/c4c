# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.3 removed the public parser state setter overloads
`set_current_struct_tag(std::string_view)` and
`set_last_resolved_typedef(std::string_view)`. The replacement setters accept
`TextId` plus optional display spelling and no longer intern rendered state
names through `parser_text_id_for_token(kInvalidText, ...)`.

Parser call sites now pass existing token/declarator/type metadata where
available. Record-body current-struct context carries parsed tag `TextId`
metadata into `set_current_struct_tag`, visible typedef casts and base typedef
resolution pass typedef `TextId` metadata into `set_last_resolved_typedef`, and
tentative restore paths restore saved `TextId`s without rediscovering them from
rendered text. The unqualified typedef-resolution route in `parse_base_type`
now uses `ts.tag_text_id` when present instead of looking up the unqualified
display tag.

## Suggested Next

Create a focused metadata-producer packet for concrete qualified
owner-member typedef `TypeSpec` identities. The blocked route is the qualified
`has_typedef` resolution path in `src/frontend/parser/impl/types/base.cpp`
around `const char* tname = ts.tag`: instantiated owner-member cases such as
`typename ns::holder<int>::type` still need the concrete owner-member identity
(`ns::holder_T_int::type`) produced as metadata before this resolver can stop
using the rendered `ts.tag` compatibility lookup for qualified names.

## Watchouts

- Do not restore string/string_view parser state setter authority. The setters
  are now `TextId`-based; display strings passed to them are fallback/display
  payloads only.
- The qualified `has_typedef` path was deliberately left compatibility-backed
  for qualified names. A metadata-only attempt using qualifier/base `TextId`s
  passed ordinary qualified casts but regressed
  `cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
  because it found the uninstantiated structured member typedef (`T`) instead
  of the concrete instantiated owner-member typedef (`int`).
- The exact missing producer metadata is a concrete owner-member `TextId` or
  equivalent domain key for instantiated member typedefs such as
  `ns::holder_T_int::type`. Treat that as a new producer packet rather than a
  lookup-only cleanup.
- Remaining `parser_text_id_for_token`, `find_parser_text_id`, `parser_text`,
  `token_spelling`, synthetic token production, diagnostics, display/final
  spelling, and compatibility paths are not automatically Step 2 violations;
  classify them by concrete semantic lookup authority before removing them.

## Proof

Passed:
`(cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure) > test_after.log 2>&1`

Proof log: regenerated canonical `test_after.log` after this setter-conversion
packet (926 tests passed).
