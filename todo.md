# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reprobe TypeSpec Tag Removal Build Boundary

## Just Finished

Completed Step 1 fixture migration for
`tests/frontend/cpp_hir_member_typedef_readiness_metadata_test.cpp` and
`tests/frontend/cpp_hir_member_typedef_origin_binding_metadata_test.cpp`.

The remaining direct `TypeSpec::tag` fixture setup in these targets now goes
through SFINAE-gated helpers. Stale-rendered-spelling disagreement assertions
remain active while `TypeSpec::tag` exists, and the explicit tag-only
no-metadata legacy compatibility check is skipped only when the field is
absent.

Temporarily removed `const char* tag` from `TypeSpec` in
`src/frontend/parser/ast.hpp` and ran
`cmake --build build --target cpp_hir_member_typedef_readiness_metadata_test cpp_hir_member_typedef_origin_binding_metadata_test`.
Both owned targets built successfully with the field removed, so no remaining
deletion-probe blocker was found inside these owned targets. Restored the field
before final proof, leaving no `ast.hpp` diff.

## Suggested Next

Run the supervisor-chosen next broad TypeSpec::tag deletion boundary check and
delegate the next fixture target if another direct test debt bucket appears.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Do not remove diagnostics, dumps, mangling, ABI/link spelling, or final
  output merely because they are string-shaped.
- Preserve stale-rendered-spelling disagreement tests.
- Split distinct downstream carrier boundaries into `ideas/open/*.md` instead
  of silently broadening the parent runbook.
- These two owned tests still have SFINAE helper references to `.tag` by design.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build --target cpp_hir_member_typedef_readiness_metadata_test cpp_hir_member_typedef_origin_binding_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_member_typedef_readiness_structured_metadata|cpp_hir_member_typedef_origin_binding_structured_metadata)$'`

Result: passed. `cpp_hir_member_typedef_readiness_structured_metadata` and
`cpp_hir_member_typedef_origin_binding_structured_metadata` are green in
`test_after.log`.

Controlled deletion probe:
- Temporarily removed `const char* tag` from `TypeSpec` in
  `src/frontend/parser/ast.hpp`.
- Ran
  `cmake --build build --target cpp_hir_member_typedef_readiness_metadata_test cpp_hir_member_typedef_origin_binding_metadata_test`;
  both focused targets built successfully with the field removed, so no next
  blocker appeared inside these targets.
- Restored the field and reran the delegated proof with the field present,
  leaving the repository buildable for the owned proof subset.
