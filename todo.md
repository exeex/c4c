# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for
`tests/frontend/cpp_hir_member_typedef_binding_metadata_test.cpp` and
`tests/frontend/cpp_hir_nested_member_typedef_record_def_metadata_test.cpp`.

The small member-typedef structured metadata fixtures no longer contain direct
test-body `TypeSpec::tag` writes. Stale rendered typedef/member-owner spelling
setup now goes through SFINAE-gated helpers, so the stale-rendered disagreement
coverage remains active while `TypeSpec::tag` exists but both files also
compile under a controlled field-deletion probe.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target cpp_hir_member_typedef_binding_metadata_test cpp_hir_nested_member_typedef_record_def_metadata_test`.
Both focused targets built successfully, so there is no remaining
deletion-probe blocker in these owned fixtures. Restored the field before final
proof.

## Suggested Next

Migrate the next supervisor-chosen direct `TypeSpec::tag` fixture debt exposed
by the broader deletion probe.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- `cpp_hir_static_member_base_metadata_test.cpp` still has SFINAE helper
  references to `.tag` by design; no direct fixture-body `.tag` references
  remain.
- The newly migrated member-typedef fixtures still have SFINAE helper
  references to `.tag` by design; no direct fixture-body `.tag` references
  remain.
- The focused deletion probe passed for both owned targets. The larger
  remaining broad-probe direct tag debt is in
  `tests/frontend/frontend_hir_lookup_tests.cpp`.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target cpp_hir_member_typedef_binding_metadata_test cpp_hir_nested_member_typedef_record_def_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_member_typedef_binding_structured_metadata|cpp_hir_nested_member_typedef_record_def_structured_metadata)$'`

Result: passed. `cpp_hir_member_typedef_binding_structured_metadata` and
`cpp_hir_nested_member_typedef_record_def_structured_metadata` are green in
`test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran
  `cmake --build build --target cpp_hir_member_typedef_binding_metadata_test cpp_hir_nested_member_typedef_record_def_metadata_test`;
  both focused targets passed with the field removed, so no remaining blocker
  was found in these targets.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
