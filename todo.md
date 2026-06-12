Status: Active
Source Idea Path: ideas/open/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Named Scalar I32 Source Lookup

# Current Packet

## Just Finished

Step 2 repaired the named scalar i32 Route 6 source lookup publication in
`src/backend/prealloc/call_plans.cpp`. `populate_call_plans(...)` now publishes
or completes one BIR `CallArgumentSourceRelationship` from prepared call-plan
authority only when the BIR argument is a named i32 value, the prepared
source id is present, the prepared source name resolves to the same BIR value
name, and the prepared source encoding is a Route 6 argument-value source
(`Register` or `FrameSlot`).

The x86 consumed helper remains fail-closed. The repair does not touch
`find_consumed_scalar_i32_call_argument_source(...)`; it still requires a
Route 6 record and a matching prepared source id. The publisher refuses
duplicates, contradictory existing source ids/names/encodings, unsupported
encodings, selected/ABI-bound source paths, direct-global dependency paths,
non-i32 arguments, unnamed arguments, and missing prepared source id/name
facts.

The delegated proof advanced past:
`x86 Route 6 call-use boundary: scalar call argument source did not thread through ConsumedPlans`.
`backend_prepared_lookup_helper` and `backend_x86_route_debug` remained green.
The remaining `backend_x86_handoff_boundary` failure is now the already
split-out idea 236 selected-value-chain blocker:
`scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership: shared helper stopped publishing the true global-root selected-value chain`.

## Suggested Next

Supervisor review should decide whether this Step 2 slice is acceptable with
the known idea 236 blocker still failing in the shared handoff-boundary
binary. The smallest next implementation packet is outside idea 235: activate
or execute idea 236 for the prepared compare-join selected-value-chain
metadata failure.

## Watchouts

- This slice intentionally mutates prepared BIR call source metadata during
  call-plan population; it does not add an x86 fallback.
- The handoff-boundary binary remains red because it includes the separate
  idea 236 selected-value-chain assertion after the Route 6 assertion.
- No route-debug expected strings, baselines, wrappers, helper-oracle output,
  prepared call/debug output, x86 compare-join stack-home code, or
  selected-value-chain metadata were changed.

## Proof

Ran the supervisor-selected proof command:

```bash
cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_route_debug_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_x86_handoff_boundary|backend_x86_route_debug|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log 2>&1
```

Result: failed with exit `8` because `backend_x86_handoff_boundary` now reaches
the idea 236 selected-value-chain failure. Passing:
`backend_prepared_lookup_helper`, `backend_x86_route_debug`. The delegated
Route 6 consumed scalar i32 call-use boundary advanced. `test_after.log` is
the proof log path.
