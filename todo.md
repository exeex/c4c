# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 baseline repair for
`cpp_hir_parser_type_base_producer_structured_metadata` in
`tests/frontend/cpp_hir_parser_type_base_producer_metadata_test.cpp`.

The forward-union producer fixture now matches the current parser structured
contract: `union ForwardUnion value` carries source spelling through
`tag_text_id` and attaches a parser-owned `NK_STRUCT_DEF` record identity while
remaining incomplete (`n_fields < 0`) and preserving `is_union`.

This preserves the existing stale-rendered-spelling guard: clearing the legacy
rendered tag does not remove the structured union identity or its TextId
carrier, and the test still rejects an invented complete `record_def`.

## Suggested Next

Move the next TypeSpec tag deletion-probe packet to
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`, which is now the
first broad-build compile blocker after temporarily deleting `TypeSpec::tag`.
Start with the direct stale-rendered fixture writes at lines such as 736, 783,
2364, and 2414, then keep the same SFINAE/TextId/record_def migration shape
used by the earlier parser fixtures.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- The delegated build target `frontend_parser_tests c4cll` does not rebuild
  `cpp_hir_parser_type_base_producer_metadata_test`; the focused binary target
  had to be built separately before CTest observed this packet's test edit.
- The deletion probe also exposed direct `.tag` fixture debt in
  `cpp_hir_static_member_base_metadata_test.cpp`,
  `frontend_hir_lookup_tests.cpp`,
  `cpp_hir_member_typedef_binding_metadata_test.cpp`, and
  `cpp_hir_nested_member_typedef_record_def_metadata_test.cpp`, but
  `frontend_parser_lookup_authority_tests.cpp` appeared first in the broad
  build output.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target frontend_parser_tests c4cll && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_type_base_producer_structured_metadata)$'`

Result: passed after rebuilding the focused test binary with
`cmake --build build --target cpp_hir_parser_type_base_producer_metadata_test`.
Both `frontend_parser_tests` and
`cpp_hir_parser_type_base_producer_structured_metadata` are green in
`test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran `cmake --build build`; the frontend/parser/HIR libraries got past the
  removal, then the broad build stopped first in
  `tests/frontend/frontend_parser_lookup_authority_tests.cpp` on direct
  `TypeSpec::tag` fixture reads/writes.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
