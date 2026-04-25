Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prefer ID-First Parser Helpers

# Current Packet

## Just Finished

Step 6 - Prefer ID-First Parser Helpers continued by adding unqualified
ID-first lookup overloads for `find_typedef_type(TextId, spelling)` and
`find_var_type(TextId, spelling)`. The string overloads remain compatibility
bridges, and `find_visible_typedef_type(TextId, spelling)` plus
`find_visible_var_type(TextId, spelling)` now probe the ID-first global lookup
before the string fallback path.

Focused parser coverage now verifies that mismatched fallback spellings do not
win when a global TextId-backed typedef or value binding exists, while separate
fallback bindings remain present for compatibility.

## Suggested Next

Next coherent packet: continue Step 6 by auditing remaining parser-owned
string fallback probes that already have a `QualifiedNameKey` carrier and can
be routed through structured/ID-first lookup without changing public
compatibility overloads.

## Watchouts

- The new TextId lookup overloads are intentionally unqualified helpers; the
  qualified structured path remains owned by `QualifiedNameKey` helpers.
- The visible lookup paths still keep the string fallback after the ID-first
  probe for compatibility when no TextId-backed binding exists.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
