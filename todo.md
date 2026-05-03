# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the rendered `TypeSpec::tag` assertion reads in
`test_parser_template_record_member_typedef_writer_registers_dependent_key`
to structured `tag_text_id == param_text` checks for `dependent_member`,
`resolved_member`, and `visible_member_type`.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe by removing the field from
`src/frontend/parser/ast.hpp`, building with the delegated probe command, and
restoring the temporary deletion afterward to expose the next compile boundary.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The first boundary is read-only fixture assertion logic, not production
  parser code; keep the next packet confined to replacing rendered-name checks
  with equivalent structured metadata assertions.
- The known `frontend_parser_tests` failure
  `namespace owner resolution should use the method owner TextId before rendered owner spelling`
  remains pre-existing observational noise for this packet.
- Later direct `TypeSpec::tag` boundaries in the same test file are still
  present after line 2714 and should be handled according to the next deletion
  probe boundary, not by speculative widening.

## Proof

Build proof and observational parser-test output are recorded in
`test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: passed.

```sh
ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: failed only at the packet-noted pre-existing
`namespace owner resolution should use the method owner TextId before rendered owner spelling`
case; no new blocker was observed after the build passed.
