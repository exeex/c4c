# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 reran the temporary `TypeSpec::tag` deletion probe after migrating
`test_parser_dependent_typename_owner_alias_prefers_record_definition`. The
probe temporarily disabled the field in `src/frontend/parser/ast.hpp` and ran:

```sh
cmake --build --preset default > test_after.log 2>&1
```

The previous record-definition preference fixture boundary no longer appears
in the compile errors. The first remaining `frontend_parser_tests.cpp`
boundary moved to the direct `TypeSpec::tag` write in
`test_parser_member_typedef_suffix_prefers_record_definition`.

The probe edit was restored, and a normal `cmake --build --preset default`
passed.

## Suggested Next

Execute the next Step 2 fixture-migration packet on
`test_parser_member_typedef_suffix_prefers_record_definition` in
`tests/frontend/frontend_parser_tests.cpp`. Preserve its record-definition
preference contract by moving the direct stale rendered `alias_ts.tag` write
behind `set_legacy_tag_if_present`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Step 3 deletion probe failed as expected with `TypeSpec::tag` disabled and
recorded the next frontend/HIR fixture boundary in `test_after.log`. Restored
proof:

```sh
cmake --build --preset default
```
