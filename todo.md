Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Structured NTTP Default Cache Keys

# Current Packet

## Just Finished

Step 4 - Add Structured NTTP Default Cache Keys completed. Added
`ParserTemplateState::NttpDefaultExprKey` keyed by `QualifiedNameKey` plus
parameter index, routed parser-owned NTTP default token caching through a helper
that populates both the structured map and rendered-name compatibility mirror,
and corrected deferred NTTP default evaluation to dual-read structured and
rendered cache entries when both exist.

Focused parser tests now cover parsed deferred NTTP defaults populating both
caches, mismatched structured/rendered entries being detected while preserving
the rendered legacy result, and legacy rendered cache fallback remaining
behavior-compatible when no structured entry is present.

## Suggested Next

Next coherent packet: supervisor review or the next plan step. This slice is
ready for review with the focused parser proof passing.

## Watchouts

- `src/frontend/parser/impl/declarations.cpp` was touched narrowly to route the
  existing NTTP default-token storage through the new mirror-populating helper;
  no broader declaration parsing behavior was changed.
- The rendered `template:param` cache remains populated and still preserves
  legacy behavior when both cache families contain divergent entries.
- Structured NTTP default lookup currently uses the evaluator's current
  namespace-context key path; structured-only lookup remains available only when
  no rendered cache entry exists.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
