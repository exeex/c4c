# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Consume Canonical Move Bundles For Join, Call, And Return Boundaries
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now removes the remaining named-return boundary fallback from the
bounded x86 compare-branch lane. The prepared-module emitter no longer falls
back to local param-derived return rendering when a named branch leaf loses its
authoritative prepared value home, and the bounded compare-branch proof now
rejects reopening that local return fallback when the shared home is removed.

## Suggested Next

Keep Step 3.3 on boundary move-bundle consumption and audit the remaining
join/return helper paths for any stale local movement assumptions that still
survive when authoritative prepared homes or bundles are missing or partially
rewired, especially in the joined-branch and compare-join return helpers.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
- Parameter value homes still need to mean the canonical entry ABI location for
  consumers, not the later regalloc-assigned destination register that
  `BeforeInstruction` bundles may target.
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
- Compare-join return rendering now tolerates stale local instruction-index
  hints by resolving the unique authoritative `BeforeReturn` bundle for the
  block when the exact hinted point is not present; keep any future refinement
  shared and bundle-driven rather than adding x86-only fallback reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the x86 prepared compare-branch renderer stopped reopening
local named-return fallback when the authoritative prepared home is missing and
the bounded compare-branch tests added prepared-home-removal rejection proof.
The accepted proof log was rolled forward into the canonical `test_before.log`
baseline.
