# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Demote Compatibility String Helpers And Tests

## Just Finished

Step 4 packet completed: record-layout constant-evaluation helper surfaces now
label retained rendered struct-tag maps as tag-only final-spelling
compatibility fallback, while keeping `TypeSpec::record_def` as the documented
record-layout authority.

`resolve_record_type_spec()` and the record-layout `eval_const_int()` plumbing
now use `compatibility_tag_map` parameter names/comments for the rendered tag
path. The frontend parser record-layout tests were renamed and their assertions
updated to describe record-definition authority versus final-spelling fallback.
No behavior changed.

## Suggested Next

Next Step 4 packet should pick another bounded retained rendered-spelling
surface and demote it as compatibility/final-spelling fallback without changing
lookup behavior. The record-layout `resolve_record_type_spec()` /
`eval_const_int()` helper surface is done.

## Watchouts

This packet intentionally did not alter `struct_tag_def_map` behavior,
`TypeSpec::record_def` behavior, or support helpers outside record-layout
constant evaluation. The rendered tag map remains functional for tag-only
TypeSpecs that lack typed record identity.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser subset
output: 1 test passed, 0 failed.
