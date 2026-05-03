# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe after the template
type-arg assertion migration from commit `61a13c372`. The probe removed the
`tag` member from `src/frontend/parser/ast.hpp`, ran the delegated default
build, recorded the compile failure in `test_after.log`, and restored
`ast.hpp`.

## Suggested Next

Migrate the first remaining compile boundary in
`tests/frontend/frontend_parser_tests.cpp`:
`test_parser_template_specialization_binding_prefers_param_text_id` still
writes `specialization->template_arg_types[0].tag` at line 4260 while also
carrying the authoritative `tag_text_id`.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The deletion probe restored `src/frontend/parser/ast.hpp`; final field
  deletion is not proven yet.
- This deletion probe first fails in `frontend_parser_tests.cpp` at
  `test_parser_template_specialization_binding_prefers_param_text_id`; later
  compile residuals remain in `frontend_parser_tests`,
  `frontend_parser_lookup_authority_tests`, and HIR fixture tests.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: the temporary field deletion failed to compile, as expected. The first
remaining boundary is:

```text
/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp:4260:41: error: 'struct c4c::TypeSpec' has no member named 'tag'
```

Ownership: parser fixture metadata migration. `src/frontend/parser/ast.hpp` was
restored after the probe, so the compile failure is preserved only in
`test_after.log`.
