Status: Active
Source Idea Path: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Qualified-Name Text Authority

# Current Packet

## Just Finished

Completed Step 1 inventory for qualified-name text authority.

Owned helper family:

- `qualified_name_from_text` (`src/frontend/parser/impl/types/types_helpers.hpp`):
  rendered text -> `QualifiedNameRef` splitter. Direct caller found:
  `split_qualified_member_type_name` only. Classification: compatibility
  bridge, not current semantic lookup authority. Removal note: remove once a
  build confirms no header-only/ODR consumer depends on the unused wrapper.
- `split_qualified_member_type_name` (`types_helpers.hpp`): rendered member
  type text -> owner `QualifiedNameRef` plus member spelling. No direct
  callers found under `src/` or `tests/`. Classification: retired
  compatibility bridge candidate. Removal note: first code packet may delete it
  if the build stays green; do not preserve it as semantic authority.
- `append_qualified_name_tokens` (`types_helpers.hpp`): rendered spelling ->
  injected identifier/`::` token sequence. Direct callers:
  `append_typespec_reparse_tokens` legacy display-tag fallback and
  `instantiate_template_struct_via_injected_parse` template-base injection.
  Classification: source-syntax reconstruction / compatibility injection.
  Non-removal note: retain while injected parse and legacy deferred syntax
  reconstruction still exist; it must not feed ordinary semantic lookup.
- `QualifiedNameRef::qualifier_segments` and `QualifiedNameRef::base_name`:
  parse-time string mirrors beside `qualifier_text_ids`, `base_text_id`,
  symbol ids, and global qualification. Classifications by caller family:
  `peek_qualified_name` / `parse_qualified_name` are source-syntax carrier
  construction; `qualified_name_text`, `spelled`, `apply_qualified_name`, and
  legacy AST/TypeSpec mirroring are display/debug or compatibility projection;
  `resolve_namespace_context`, `resolve_namespace_name`,
  `resolve_qualified_type`, `resolve_qualified_value`,
  `find_qualified_name_key`, and `Parser::qualified_name_key` are semantic
  lookup/key construction; dependent-typename and injected-parse callers in
  `types/declarator.cpp` are a mixed path where structured lookup exists but
  syntax reconstruction still uses spelling.

First semantic authority path:

- Target `QualifiedNameRef` -> `QualifiedNameKey` construction in
  `src/frontend/parser/impl/core.cpp`, especially `find_qualified_name_key`
  and `Parser::qualified_name_key`, then the direct semantic consumers
  `resolve_qualified_type`, `find_template_struct_primary`, and
  `parse_dependent_typename_specifier`. The code packet should make TextId
  segments/base ids the semantic authority when present, quarantine
  `base_name`/`qualifier_segments` string fallback as an explicit legacy bridge,
  and avoid adding any new rendered `A::B::C` splitter.

## Suggested Next

Begin Step 2 with a bounded code packet in `src/frontend/parser/impl/core.cpp`
and the nearest dependent typename caller:

- Add a TextId-first helper or tighten the existing
  `QualifiedNameRef` -> `QualifiedNameKey` path so semantic lookup uses
  `base_text_id` plus complete `qualifier_text_ids` when available.
- Keep fallback from `base_name` / `qualifier_segments` only in a named
  compatibility branch for legacy constructed refs with missing ids.
- Update the direct dependent-typename or qualified-base-type caller only as
  needed to pass structured `QualifiedNameRef` data through that path; do not
  remove syntax reconstruction helpers in the same packet.
- Focused future proof command:
  `cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_member_typedef_lookup_structured_metadata|cpp_hir_parser_type_base_deferred_member_template_origin_structured_metadata|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_sema_namespace_cross_namespace_lookup_parse_cpp)$' -j --output-on-failure`

## Watchouts

- Do not classify diagnostic/debug spelling or deferred syntax reconstruction
  as semantic authority.
- Do not add a new rendered qualified-name splitter as the primary lookup path.
- Retained string helpers need explicit compatibility, display, or syntax
  reconstruction ownership plus removal/non-removal conditions.
- `qualified_name_from_text` and `split_qualified_member_type_name` look unused
  today, but removing them is a cleanup packet, not the first semantic repair.
- `append_qualified_name_tokens` is intentionally retained for injected syntax;
  the risk is callers using its output as ordinary lookup input.
- `Parser::qualified_name_key` currently interns fallback TextIds from string
  mirrors. That fallback is the area to bound, not a reason to drop AST display
  fields.

## Proof

Inventory-only packet; no build required and no `test_after.log` produced.

Read-only proof/searches run:

- `rg -n "qualified_name_from_text|split_qualified_member_type_name|append_qualified_name_tokens|qualifier_segments\\(|base_name\\(" -S .`
- `rg -n "qualified_name_from_text|split_qualified_member_type_name|append_qualified_name_tokens" src tests -S`
- `rg -n "\\.qualifier_segments|\\.base_name|\\.base_text_id|\\.qualifier_text_ids|\\.is_global_qualified" src/frontend/parser/impl src/frontend/parser/parser.* -S`
- `rg -n "find\\(\"::\"\\)|rfind\\(\"::\"\\)|split_qualified|qualified_name_from_text|append_qualified_name_tokens" src/frontend/parser/impl -S`
- `ctest --test-dir build -N | rg -n "frontend|parser|lookup|hir"` to verify focused future test names.
