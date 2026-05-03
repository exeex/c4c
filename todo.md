# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe after the deferred
member typedef parser fixture migration. Removing the field now reaches
`tests/frontend/frontend_parser_tests.cpp` first, at
`test_parser_template_base_deferred_member_typedef_uses_member_text_id`:
`deferred_base.tag = arena.strdup("Owner")` on line 2928, followed by
`box_alias.tag = arena.strdup("Box")` on line 2957.

## Suggested Next

Migrate the template-base deferred member typedef parser fixture off direct
`TypeSpec::tag` setup in `tests/frontend/frontend_parser_tests.cpp`, keeping the
scope to that coherent fixture boundary.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion; `src/frontend/parser/ast.hpp` was restored
  after this probe.
- `src/frontend/parser/ast.hpp` was restored after the probe, and a separate
  restore build completed successfully without overwriting `test_after.log`.
- The first remaining compile boundary is outside this packet's owned code
  files because `tests/frontend/frontend_parser_tests.cpp` was inspection-only.

## Proof

Temporary deletion-probe build output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: command exited nonzero as expected for the deletion probe. The first
remaining compile boundary is
`tests/frontend/frontend_parser_tests.cpp:2928:17: error: 'struct c4c::TypeSpec'
has no member named 'tag'` in
`test_parser_template_base_deferred_member_typedef_uses_member_text_id`.

Restore check, with `src/frontend/parser/ast.hpp` restored:

```sh
cmake --build --preset default >/tmp/c4c_restore_build.log 2>&1
```

Result: command exited zero.
