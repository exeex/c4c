Status: Active
Source Idea Path: ideas/open/85_parser_ctor_init_visible_head_probe_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the constructor-init ambiguity seams

# Current Packet

## Just Finished
Switched the active lifecycle from idea 83 to the new constructor-init visible
head decomposition idea. The active runbook now targets the repeated
`source(other)` regression family as its own route.

## Suggested Next
Map the constructor-init ambiguity seams into the parenthesized local-value
direct-init, grouped pointer/reference starter, and visible-head handoff
boundaries before touching the parser probe again.

## Watchouts
Do not let this route drift back into the broader lexical-scope lookup plan.
The prior classifier-first reorder still regressed
`value-expression direct-init forms should stay declarations` on the
`Box value(source(other));` path, so the visible-head handoff must be
separated before any retry.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed on the restored baseline. The attempted classifier-first reorder failed
on `value-expression direct-init forms should stay declarations`. Proof log:
`test_after.log`.
