Status: Active
Source Idea Path: ideas/open/85_parser_ctor_init_visible_head_probe_split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Re-run focused parser proof

# Current Packet

## Just Finished
Added the remaining focused parser regression for the visible-head handoff
boundary. The constructor-init probe suite now names separate checks for the
parenthesized value path, grouped pointer/reference starters, and the
visible-head handoff itself.

## Suggested Next
Re-run the focused parser proof and, if it stays green, treat the active
runbook as complete and evaluate the source idea for closure.

## Watchouts
Do not let this route drift back into the broader lexical-scope lookup plan.
The new handoff regression is meant to pin the existing parser split, not to
license a broader classifier reorder. `Box value(source(other));` and the
qualified visible-value call shapes must stay on the direct-init declaration
side while the visible-type cases remain function declarations.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log`
passed after adding the dedicated visible-head handoff regression. Proof log:
`test_after.log`.
