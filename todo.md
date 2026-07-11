Status: Active
Source Idea Path: ideas/open/693_bir_route_index_retirement_research.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Define Test And Dump Policy

# Current Packet

## Just Finished

Completed Step 6: Define Test And Dump Policy by writing
`docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md`.
The document maps current route/prepared fact tests to keep, gate, rewrite, or
delete decisions, explains how MIR, object, runtime, and object-runtime proof
replace intermediate route dump comparison, defines when a new route-view test
is justified, and records specific Route 4, Route 5, and Route 7 dump-policy
rules.

## Suggested Next

Proceed to Step 7 by writing the follow-up idea recommendations. The next
packet should dependency-order the implementation ideas that follow this
research, including named view wrappers, Route 4 proof migration, Route 7
comparison adapter migration, Route 5 publication proof migration, dump
rewrites, and deferred higher-risk route families.

## Watchouts

- Follow-up recommendations should preserve the proof hierarchy from Step 6:
  runtime/object-runtime and object proof outrank MIR/prepared proof, and named
  route-view proof is only justified for BIR view or compatibility-adapter
  migration.
- Route-numbered dump snippets such as Route 4 attribution, `route5_status`,
  `route5_agrees`, and Route 7 validation status should be treated as
  transitional baseline text, not durable correctness gates.
- Do not recommend expectation downgrades, unsupported-marker changes,
  allowlist changes, baseline-only acceptance, or route-view tests that prove
  only a narrow named case.

## Proof

Docs-only proof. No build required. Ran:

```sh
test -f docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md && rg -n "keep|gate|rewrite|delete|dump|baseline|MIR|object|runtime|route-view test|intermediate|proof|Route 4|Route 5|Route 7" docs/bir_route_index_retirement_research/06_test_and_dump_policy_after_route_retirement.md
```

The delegated proof writes no root-level log; `test_after.log` was intentionally
not updated because the packet marked this as docs-only and forbade touching
root-level `.log` files.
