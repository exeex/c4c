# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3.1
Current Step Title: Finish Compare-Driven Boundary Home Authority
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now makes compare-driven param-zero entry require authoritative
prepared parameter homes instead of reopening ABI fallback when the home is
missing, and it frame-wraps stack-slot entry loads using the prepared frame
size. The bounded compare-branch proof now covers both stack-slot and
rematerializable parameter-home entry lanes and rejects the route when the
authoritative prepared entry home is removed.

## Suggested Next

Advance Step 3.3.1 on compare-driven boundary home authority by proving the
remaining compare-join entry and parameter-selected return lanes consume shared
prepared homes and `BeforeReturn` bundles for stack-backed or rematerializable
sources without reopening ABI or local-home fallback.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
- The bounded stack-home proofs still mutate prepared value homes inside the
  scalar smoke tests; that is acceptable as bounded proof for the cleaned
  consumer, but later validation should still look for a naturally produced
  stack-backed lane if one can stay honest and narrow.
- The rematerializable scalar proof is still limited to the immediate-root
  no-parameter lane; later widening should confirm shared producer support
  before depending on rematerializable parameter-style sources.
- `PreparedMovePhase::BlockEntry` is currently inferred from shared
  `phi_...` move reasons, while call/result/return bundles come from
  destination-kind classification in shared prepare; keep any later phase
  refinement shared instead of pushing it into x86.
- The bounded call lanes now depend on shared `CallInst.arg_abi` and
  `CallInst.result_abi` inference in `legalize.cpp`; any follow-up should keep
  that metadata shared instead of reopening x86-local ABI defaults.
- The local-slot fast path now throws the same authoritative prepared
  call-bundle contract error as the generic emitter when a named `i32`
  argument loses its `BeforeCall` bundle; later call-lane work should preserve
  that shared failure surface instead of degrading back to generic
  unsupported-route rejection.
- The bounded direct extern-call and multi-defined call expectations now
  include materialized prepared call-result homes (`r11d`/`r12d` or stack
  stores), so future route changes should treat those emitted homes as part of
  the prepared handoff contract rather than as incidental register noise.
- Compare-driven compare-join entry and parameter-selected return lanes are the
  next active Step 3.3.1 scope; short-circuit and `EdgeStoreSlot`-style routes
  now belong to later Step 3.3.2 follow-up rather than being silently mixed
  into the same packet.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the compare-driven entry helper rejected missing prepared
entry homes, frame-wrapped stack-slot entry loads, and the compare-branch
tests added bounded stack-slot and rematerializable entry-home coverage.
