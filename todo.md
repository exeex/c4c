# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Fixture Helpers Off Direct Tag Access

## Just Finished

Step 2 migrated
`test_parser_top_level_typedef_uses_unresolved_identifier_type_head_fallback`
off the direct `alias_ts->tag` assertion. The test now preserves the unresolved
placeholder contract by checking that the registered typedef's
`tag_text_id` matches the parser-owned `ForwardDecl` text id, with a spelling
check through structured metadata.

## Suggested Next

Continue with the next frontend parser fixture residual that directly reads
`TypeSpec::tag`, then re-run the same narrow frontend parser proof selected by
the supervisor for that packet.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- The focused frontend parser test still has the known pre-existing
  `namespace owner resolution should use the method owner TextId before rendered
  owner spelling` failure.
- This packet intentionally touched only the local fallback typedef test and did
  not sweep later direct tag uses in `tests/frontend/frontend_parser_tests.cpp`.

## Proof

Proof output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1; ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1
```

Result: command exited nonzero in the `ctest` phase. The build completed and
linked `frontend_parser_tests`; the only reported failure remains the known
pre-existing `namespace owner resolution should use the method owner TextId
before rendered owner spelling` failure.
