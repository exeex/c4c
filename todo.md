# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 temporarily removed `TypeSpec::tag` from
`src/frontend/parser/ast.hpp` and re-ran the deletion probe. The first
remaining compile boundary is in `tests/frontend/frontend_parser_tests.cpp`:
`test_parser_nested_dependent_typename_prefers_record_definition` writes
`nested_field_type.tag` at line 1500. The probe edit was restored afterward.

## Suggested Next

Migrate the next direct-tag fixture boundary in
`tests/frontend/frontend_parser_tests.cpp`: replace the line 1500
`nested_field_type.tag` stale-name write in
`test_parser_nested_dependent_typename_prefers_record_definition` with the
current legacy-tag helper or structured metadata path, preserving the test's
record-definition authority coverage.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion; this packet restored `TypeSpec::tag`.
- The next compile boundary is outside this executor packet's owned code files:
  `tests/frontend/frontend_parser_tests.cpp` was read-only for this probe.
- The known `frontend_parser_tests` namespace-owner assertion is outside these
  fixture migration packets and should not be repaired as part of narrow stale
  tag fixture writes.

## Proof

Deletion-probe proof was recorded in `test_after.log`:

```sh
cmake --build --preset default > test_after.log 2>&1
```

The probe failed as expected with the first error:
`tests/frontend/frontend_parser_tests.cpp:1500:21: error: 'struct c4c::TypeSpec' has no member named 'tag'`.

Restored-build proof passed after putting `TypeSpec::tag` back:

```sh
cmake --build --preset default
```
