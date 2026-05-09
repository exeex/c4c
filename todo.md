Status: Active
Source Idea Path: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Replace NTTP Array-Size Name Matching With Parameter-Domain Lookup

# Current Packet

## Just Finished

Completed Step 4 NTTP array-size substitution repair. Replaced the remaining
parser template-instantiation array-size rendered-name loops with a shared
metadata-first helper that substitutes through NTTP parameter TextId/domain
metadata when available and keeps string matching only as the legacy
no-metadata bridge. Added parser coverage for stale rendered spelling with a
valid parameter TextId and same-spelling stale TextId rejection.

## Suggested Next

Next implementation packet target: continue with the next supervisor-selected
plan step or request plan-owner review if Step 4 was the final active runbook
implementation packet.

## Watchouts

- Do not weaken tests or add named-fixture shortcuts to make a narrow case pass.
- The new helper deliberately refuses rendered-name fallback after
  authoritative NTTP metadata is present but misses; this is covered by the
  same-spelling stale TextId parser fixture.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$' --output-on-failure`

Full build and focused ctest output captured in `test_after.log`.
