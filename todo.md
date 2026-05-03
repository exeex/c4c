# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe by removing the
field from `src/frontend/parser/ast.hpp`, building with the delegated command,
and restoring the temporary deletion after the compile failed. The first
remaining boundary is `tests/frontend/frontend_parser_tests.cpp` in
`test_parser_template_instantiation_member_typedef_uses_concrete_key`, starting
at line 2708 with fixture setup assigning `param_ts.tag`.

## Suggested Next

Delegate a narrow test-fixture migration packet that owns
`tests/frontend/frontend_parser_tests.cpp` and replaces the first boundary in
`test_parser_template_instantiation_member_typedef_uses_concrete_key` with
equivalent structured metadata setup before re-running the deletion probe.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion.
- The first remaining boundary is test fixture setup in
  `tests/frontend/frontend_parser_tests.cpp`, not production parser code; keep
  the next packet confined to replacing rendered `tag` setup/checks with
  equivalent structured metadata assertions for that local fixture family.
- The known `frontend_parser_tests` failure
  `namespace owner resolution should use the method owner TextId before rendered owner spelling`
  remains pre-existing observational noise for this packet.
- Later direct `TypeSpec::tag` boundaries in the same test file are still
  present after line 2708; handle only the next coherent local fixture family,
  not the entire file by speculative widening.
- The same probe log also shows later HIR fixture boundaries in
  `tests/frontend/frontend_hir_lookup_tests.cpp` and focused HIR metadata test
  files, but those are beyond the first reported compile boundary.

## Proof

Deletion-probe build output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: failed as expected for the temporary deletion probe. The first reported
compile error is
`tests/frontend/frontend_parser_tests.cpp:2708:12: error: 'struct c4c::TypeSpec' has no member named 'tag'`.
