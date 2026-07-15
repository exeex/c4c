# LIR Switch Selector Type-Reference Verifier Repair

Status: Closed — capability complete
Type: bounded verifier-correctness blocker
Formerly blocked: `ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` Step 2

## Goal

Repair the deterministic `LirSwitch.selector_type_ref` verifier defect so a switch's structured selector type reference is checked against the selector-selected integer value definition correctly.

## Closure Record

Capability complete. Commit `8218993a5` requires a valid structured `selector_type_ref` before the semantic integer-width comparison against the selector-selected definition. Nearby coverage proves a valid matching reference and rejection for missing, non-integer, stale-width, and same-width incoherent references.

The supervisor accepted fresh proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`. Matching regression-guard `test_before.log` and `test_after.log` results passed.

Return disposition: reactivate 801 at unchanged Step 2, `Repair anonymous layout / structured-call compatibility`. This closure accepts no anonymous-layout, direct-complex call, aggregate-use, or other preserved working-tree change; 801 retains its direct-complex call-signature and full-baseline acceptance gate.
