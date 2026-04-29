# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Reprove Parser Cleanup And Decide Closure Or Split

## Just Finished

Step 4 is exhausted after the record-layout compatibility tag-map demotion,
template primary/specialization rendered-name mirror demotion, and NTTP default
expression rendered-cache demotion packets.

The remaining public helper/test string surfaces are not a precise additional
Step 4 demotion packet: `struct_tag_def_map`, template rendered mirrors, and
`nttp_default_expr_tokens` are now visibly compatibility/final-spelling mirrors;
`defined_struct_tags`, rendered template instantiation keys, typedef-chain
helper maps, diagnostics, source spelling, and string-facing public bridge tests
fit final-spelling, compatibility, or downstream boundary categories rather than
ordinary parser semantic lookup authority.

## Suggested Next

Step 5 should reprove the parser cleanup route and decide closure versus split.
Suggested supervisor proof is a fresh focused frontend/parser run covering the
changed parser tests plus the template specialization subset used during Steps
3 and 4:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_(template_specialization_member_typedef_trait_parse_cpp|template_specialization_typedef_chain_parse_cpp|template_specialization_visible_typedef_chain_parse_cpp|template_struct_specialization_parse_cpp|template_struct_specialization_runtime_cpp|template_bool_specialization_parse_cpp|specialization_identity_cpp|template_forward_pick_specialization_identity_cpp|eastl_slice6_template_defaults_and_refqual_cpp))$' > test_after.log 2>&1`

After that proof, inspect the accumulated diff for unsupported downgrades,
testcase-shaped shortcuts, or any remaining unclassified parser-owned string
lookup authority. If none are present, ask the plan-owner to close idea 123
with the selected regression guard. If a remaining issue is real but outside
this runbook, split it into a new `ideas/open/` initiative instead of extending
Step 4.

## Watchouts

Do not treat Step 5 as permission to delete retained strings. The intended
terminal state still preserves display, diagnostics, source spelling, final
emitted names, TextId-less compatibility, and public string-facing bridge
behavior.

Closure requires source-idea acceptance, not just `plan.md` exhaustion. If the
proof or diff review finds a genuine unresolved parser-owned semantic lookup,
route it explicitly: keep it in Step 5 only if it is a closure blocker inside
idea 123, or create a separate open idea if it is a downstream/cross-module
boundary.

## Proof

Latest Step 4 proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser subset
output: 1 test passed, 0 failed.

No validation was run for this lifecycle-only Step 4 exhaustion decision.
