# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Re-run The TypeSpec Tag Deletion Probe

## Just Finished

Step 3 re-ran the temporary `TypeSpec::tag` deletion probe after the parser
fixture migration. The build still fails on direct `TypeSpec::tag` use, with
the first remaining boundary in
`tests/frontend/frontend_parser_tests.cpp` inside
`test_parser_deferred_member_typedef_lookup_uses_member_text_id` at
`box_alias.tag = arena.strdup("Box")`, followed by `arg_ts.tag =
arena.strdup("Owner")` in the same fixture.

## Suggested Next

Migrate the
`test_parser_deferred_member_typedef_lookup_uses_member_text_id` fixture off
direct `TypeSpec::tag` setup by using the existing TextId and record metadata
paths for the local `box_alias` and `arg_ts` bindings.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Temporary deletion probes must be restored unless the packet is the final
  accepted field-removal deletion; `src/frontend/parser/ast.hpp` was restored
  after this probe.
- This packet did not edit `tests/frontend/frontend_parser_tests.cpp`; the
  named fixture was inspected only to identify the next boundary.
- The next boundary is still parser-fixture-owned and appears before the later
  direct `TypeSpec::tag` boundaries in the same test file. Handle only this
  coherent fixture migration next, not the entire file by speculative widening.

## Proof

Temporary deletion-probe build output is recorded in `test_after.log`.

```sh
cmake --build --preset default > test_after.log 2>&1
```

Result: build failed as expected for the deletion probe. The first compiler
error is `tests/frontend/frontend_parser_tests.cpp:2864:13`, where
`test_parser_deferred_member_typedef_lookup_uses_member_text_id` still assigns
`box_alias.tag` after `TypeSpec::tag` is temporarily removed.
