# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe after the fallback
typedef assertion migration from `356afb335`. Removing the field still fails at
compile time; the first remaining boundary is
`tests/frontend/frontend_parser_tests.cpp:4181` in
`test_parser_template_type_arg_probes_use_token_spelling`, which reads
`arg.type.tag`.

## Suggested Next

Migrate the
`test_parser_template_type_arg_probes_use_token_spelling` fixture assertion off
`arg.type.tag`, preserving its token-spelling/identity contract through
structured parser metadata, then re-run the deletion probe.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The deletion probe restored `src/frontend/parser/ast.hpp`; final field
  deletion is not proven yet.
- The same probe also exposed later direct `TypeSpec::tag` residuals in
  `frontend_parser_tests`, `frontend_parser_lookup_authority_tests`, and HIR
  fixture tests, but the first compile boundary is the frontend parser fixture
  at line 4181.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: command exited nonzero during the temporary deletion probe. The first
reported compiler error is `tests/frontend/frontend_parser_tests.cpp:4181:22:
error: 'struct c4c::TypeSpec' has no member named 'tag'`.
After restoring `src/frontend/parser/ast.hpp`, `cmake --build --preset default`
completed successfully; this restore check did not replace `test_after.log`.
