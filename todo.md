# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the direct legacy `TypeSpec::tag` fixture write in
`test_parser_nested_dependent_typename_prefers_record_definition` behind
`set_legacy_tag_if_present(..., 0)`. The stale rendered name
`StaleNested` is still injected when the legacy field exists, while
`record_def = real_nested` continues to preserve the record-definition
authority coverage.

## Suggested Next

Re-run the deletion probe by temporarily removing `TypeSpec::tag` from
`src/frontend/parser/ast.hpp`, building, and recording the next compile
boundary. Restore the probe edit afterward unless the deletion compiles and is
accepted as the final field removal.

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

Proof is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: passed.

The delegated observational parser-test run was appended with:

```sh
ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: failed only at the known pre-existing assertion,
`namespace owner resolution should use the method owner TextId before rendered
owner spelling`.
