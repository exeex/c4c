# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Demote Compatibility String Helpers And Tests

## Just Finished

Step 4 packet completed: NTTP default expression cache comments, local names,
and frontend parser test wording now make the `NttpDefaultExprKey` table the
structured cache and `nttp_default_expr_tokens` the rendered-name
compatibility/final-spelling mirror.

`eval_deferred_nttp_default()` and `cache_nttp_default_expr_tokens()` now use
rendered-mirror local names for the string-key path while preserving the
structured-cache lookup order, TextId-less fallback behavior, and mismatch
counter behavior exactly. The NTTP cache test was renamed and its assertions now
describe the rendered mirror as secondary compatibility state.

## Suggested Next

Next Step 4 packet: have the supervisor/plan-owner decide whether Step 4 has
remaining honest compatibility-demotion surfaces, or whether the runbook step
is exhausted. A bounded candidate, if continuing Step 4, is to classify any
remaining public helper/test string surfaces that are final-spelling bridge
state rather than semantic lookup authority.

## Watchouts

This packet intentionally did not convert or delete
`nttp_default_expr_tokens` fallback reads. The retained rendered-name cache
mirror remains functional for TextId-less compatibility/final-spelling lookup,
and stale rendered-name precedence is still rejected only where a valid
structured key/TextId path exists.

No `template_struct_defs` / `template_struct_specializations`,
`struct_tag_def_map`, `defined_struct_tags`, or public helper behavior was
changed.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser subset
output: 1 test passed, 0 failed.
