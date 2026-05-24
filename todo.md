Status: Active
Source Idea Path: ideas/open/prealloc-call-boundary-ordering-republication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Cross-Target Ordering Proof

# Current Packet

## Just Finished

Completed Step 4: `Add Cross-Target Ordering Proof`.

Added `backend_x86_call_boundary_effect_ordering`, an x86-labeled backend MIR
test that consumes `PreparedCallBoundaryEffectPlan` and
`plan_prepared_call_boundary_effects` without AArch64 codegen headers, register
views, memory operands, ABI lane assumptions, or instruction records. The test
exercises real effect ordering across explicit before-call moves, preservation
home population, explicit after-call moves, and preservation republication, and
also checks unavailable explicit-effect rejection through a neutral
classification status.

## Suggested Next

Ask plan-owner to decide whether the active call-boundary republication idea is
complete enough to close or needs a narrow Step 5 recording pass.

## Watchouts

The cross-target test proves neutral ordering and endpoint facts only; it does
not claim x86 lowering support. Preservation-home population and republication
emission remain target-local unless a later source idea defines a tighter
neutral register-identity or emission boundary.

## Proof

Ran:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Result: `160/160` backend tests passed, `0` failed.

Proof log: `test_after.log`.
