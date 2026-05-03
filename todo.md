# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe by removing the
field from `src/frontend/parser/ast.hpp` and building with the delegated
command. The probe still fails in `tests/frontend/frontend_parser_tests.cpp`;
the first remaining compile boundary is
`test_parser_template_record_member_typedef_writer_registers_dependent_key`
at line 2602, where the fixture directly reads `dependent_member->tag` and
maps the rendered spelling back through `parser.find_parser_text_id`.
The temporary `ast.hpp` deletion edit was restored after the probe.

## Suggested Next

Migrate the Step 3 boundary in
`test_parser_template_record_member_typedef_writer_registers_dependent_key`
from direct rendered `TypeSpec::tag` reads to structured `tag_text_id` checks
for `dependent_member`, `resolved_member`, and `visible_member_type`, then
re-run the deletion probe to expose the next boundary.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The first boundary is read-only fixture assertion logic, not production
  parser code; keep the next packet confined to replacing rendered-name checks
  with equivalent structured metadata assertions.
- Later direct `TypeSpec::tag` boundaries in the same test file are still
  present after line 2714, but they are not the first compile boundary while
  the line 2602 family remains.

## Proof

Deletion-probe proof is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: failed as expected for the probe. The first diagnostic is
`tests/frontend/frontend_parser_tests.cpp:2602:37: error: 'const struct c4c::TypeSpec' has no member named 'tag'`.

```sh
cmake --build --preset default
```

Result: passed after restoring `src/frontend/parser/ast.hpp`.
