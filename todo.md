# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 reran the temporary `TypeSpec::tag` deletion probe after migrating the
local-visible owner alias fixture. The probe temporarily disabled the field in
`src/frontend/parser/ast.hpp` and ran:

```sh
cmake --build --preset default > test_after.log 2>&1
```

The previous `test_parser_dependent_typename_uses_local_visible_owner_alias`
boundary no longer appears in the compile errors. The first remaining
`frontend_parser_tests.cpp` boundary moved to the direct `TypeSpec::tag` write
in `test_parser_dependent_typename_owner_alias_prefers_record_definition`.
Later visible residuals remain in the same file and in
`frontend_parser_lookup_authority_tests.cpp`,
`cpp_hir_static_member_base_metadata_test.cpp`,
`frontend_hir_lookup_tests.cpp`, and smaller `cpp_hir_*` metadata tests.

The probe edit was restored, and a normal `cmake --build --preset default`
passed.

## Suggested Next

Execute the next Step 2 fixture-migration packet on
`test_parser_dependent_typename_owner_alias_prefers_record_definition` in
`tests/frontend/frontend_parser_tests.cpp`. Preserve its record-definition
preference contract by moving the direct stale rendered `alias_ts.tag` write
behind `set_legacy_tag_if_present`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The next target should not reintroduce a replacement semantic string field on
  `TypeSpec`; use local compatibility helpers or structured metadata.

## Proof

Step 3 deletion probe failed as expected with `TypeSpec::tag` disabled and
recorded the next frontend/HIR fixture boundary in `test_after.log`. Restored
proof:

```sh
cmake --build --preset default
```

Baseline review:
hook-produced `test_baseline.new.log` at commit `a6cc8201e` was rejected
against accepted `test_baseline.log` because it regressed from 3000/3000 to
2892/3023 with 131 new failures. The candidate was deleted with
`scripts/plan_review_state.py reject-baseline --delete-candidate`; keep the
accepted full-suite baseline at `ba8120f246`.
