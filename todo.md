# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_template_specialization_binding_prefers_param_text_id` off the
direct `specialization->template_arg_types[0].tag` fixture write while
preserving the existing `tag_text_id`/template-parameter TextId matching path
and stale rendered parameter spelling coverage.

## Suggested Next

Continue with the next remaining direct `TypeSpec::tag` fixture access under
the supervisor-selected packet and proof scope.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.
- This packet did not sweep later direct tag uses in
  `tests/frontend/frontend_parser_tests.cpp`; those remain for separate
  packets.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: build completed and the focused parser test scope exited nonzero with
the same known pre-existing failure as `test_before.log`:

```text
FAIL: namespace owner resolution should use the method owner TextId before rendered owner spelling
```

Failure set: unchanged from baseline. Ownership: outside this packet.
