# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for
`tests/frontend/cpp_hir_static_member_base_metadata_test.cpp`.

The static-member-base structured metadata fixture no longer contains direct
test-body `TypeSpec::tag` writes. Stale rendered base spelling setup now goes
through a SFINAE-gated helper, so the stale-rendered disagreement coverage
remains active while `TypeSpec::tag` exists but the file also compiles under a
controlled field-deletion probe.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target cpp_hir_static_member_base_metadata_test`. The
focused target built successfully, so there is no remaining deletion-probe
blocker in this owned fixture. Restored the field before final proof.

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
- The focused deletion probe passed for the owned target. The broader
  deletion-probe frontier still includes related direct tag debt in
  `frontend_hir_lookup_tests.cpp`,
  `cpp_hir_member_typedef_binding_metadata_test.cpp`, and
  `cpp_hir_nested_member_typedef_record_def_metadata_test.cpp`.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target cpp_hir_static_member_base_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_static_member_base_structured_metadata$'`

Result: passed. `cpp_hir_static_member_base_structured_metadata` is green in
`test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran `cmake --build build --target cpp_hir_static_member_base_metadata_test`;
  the focused target passed with the field removed, so no remaining blocker was
  found in this target.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
