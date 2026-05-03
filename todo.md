# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated the first deletion-probe fixture boundary in
`test_parser_template_instantiation_member_typedef_uses_concrete_key` off
direct `TypeSpec::tag` setup. The local `param_ts`, `box_alias`, and
`payload_alias` setup now uses `tag_text_id`, `record_def`, template parameter
TextId metadata, and member typedef TextId metadata instead of rendered tag
assignment.

## Suggested Next

Re-run the temporary `TypeSpec::tag` deletion probe to identify the next
compile boundary after the migrated parser fixture setup.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The known `frontend_parser_tests` failure
  `namespace owner resolution should use the method owner TextId before rendered owner spelling`
  remains pre-existing observational noise for this packet; the failure set in
  `test_after.log` matches `test_before.log`.
- Later direct `TypeSpec::tag` boundaries in the same test file are still
  present after this migrated function; handle only the next coherent boundary
  reported by the next deletion probe, not the entire file by speculative
  widening.
- The same probe log also shows later HIR fixture boundaries in
  `tests/frontend/frontend_hir_lookup_tests.cpp` and focused HIR metadata test
  files, but those are beyond the first reported compile boundary.

## Proof

Build plus focused parser test output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: build succeeded, then `frontend_parser_tests` exited nonzero with the
known pre-existing failure
`namespace owner resolution should use the method owner TextId before rendered owner spelling`.
The failure set stayed the same as `test_before.log`.
