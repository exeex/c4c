# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Consume Canonical Move Bundles For Join, Call, And Return Boundaries
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now makes the bounded compare-join return helper source
parameter-selected arms from the authoritative prepared value-home contract
instead of reopening the entry ABI register by local convention. The joined
branch proof now mutates the compare-join parameter home to `r10d` and confirms
the x86 prepared consumer follows that shared home while still honoring the
authoritative prepared `BeforeReturn` bundle.

## Suggested Next

Keep Step 3.3 on boundary move-bundle consumption and audit the remaining
compare-join and short-circuit helper paths for any parameter- or
continuation-sourcing that still reopens entry ABI or local carrier assumptions
instead of shared prepared homes and bundles, especially on the EdgeStoreSlot
variants.

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
- Compare-join parameter-return rendering now follows authoritative prepared
  value homes for register and rematerializable lanes, but compare-driven entry
  setup still assumes the canonical parameter ABI register for the initial
  zero-test; keep any future refinement shared and bundle-driven instead of
  growing x86-only entry fallback logic.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the compare-join return helper started sourcing
parameter-selected arms from authoritative prepared value homes and the joined
branch boundary tests added non-ABI register-home proof for that route. Proof
log: `test_after.log`.
