# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Demote Compatibility String Helpers And Tests

## Just Finished

Step 4 packet completed: template primary/specialization rendered-name bridge
surfaces now label retained string maps as final-spelling compatibility
mirrors, while keeping `QualifiedNameKey` tables as the documented primary
authority.

`ParserTemplateState` now comments `template_struct_defs` and
`template_struct_specializations` as rendered-name compatibility mirrors.
Template registration and lookup helpers use mirror/final-spelling local names
for the string-map paths, and the frontend parser test was renamed with
assertions updated to describe structured-key authority versus TextId-less
compatibility. No behavior changed.

## Suggested Next

Next Step 4 packet: have the supervisor/plan-owner decide whether Step 4 has
remaining honest compatibility-demotion surfaces, or whether the runbook step
is exhausted. If continuing Step 4, a bounded candidate is the NTTP default
cache rendered-name mirror around `nttp_default_expr_tokens` and its existing
frontend parser tests, preserving all current fallback behavior.

## Watchouts

This packet intentionally did not convert or delete
`template_struct_defs` / `template_struct_specializations` fallback reads. It
also did not alter `struct_tag_def_map`, `defined_struct_tags`,
`nttp_default_expr_tokens`, public helper behavior, or template string-map
fallback behavior.

The retained template string maps remain functional for TextId-less
compatibility and final-spelling bridge lookups. Stale rendered-name precedence
is still rejected only where a valid structured key/TextId path exists.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser subset
output: 1 test passed, 0 failed.
