# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_template_type_arg_probes_use_token_spelling` off direct
`arg.type.tag` access. The fixture now asserts the parser-owned `TextId`
identity through `tag_text_id`, verifies structured template parameter metadata
through `template_param_text_id`, and checks spelling through
`parser.parser_text(...)`.

## Suggested Next

Re-run the `TypeSpec::tag` deletion probe to find the next remaining direct
legacy spelling boundary after this parser fixture migration.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The deletion probe restored `src/frontend/parser/ast.hpp`; final field
  deletion is not proven yet.
- The previous deletion probe also exposed later direct `TypeSpec::tag` residuals in
  `frontend_parser_tests`, `frontend_parser_lookup_authority_tests`, and HIR
  fixture tests.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: build completed successfully; `frontend_parser_tests` exited nonzero
with the same single known pre-existing failure as `test_before.log`:
`namespace owner resolution should use the method owner TextId before rendered
owner spelling`.
