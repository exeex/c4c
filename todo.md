# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3.3
Current Step Title: Close Residual Call, Result, And Return Boundary Seams
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Lifecycle review confirmed that Step 3.3.2's short-circuit and
`EdgeStoreSlot` ownership checkpoint is already covered by the existing
prepared helper and bounded proof surfaces, so the active route advances past
that stale executor target and onto Step 3.3.3.

## Suggested Next

Advance Step 3.3.3 by closing the remaining bounded call/result/return
boundary seams in the x86 prepared consumer, keeping the route focused on
authoritative prepared move bundles and emitted homes instead of local ABI or
return fallback logic.

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
- Compare-driven stack-slot parameter returns now frame-wrap their
  compare-join loads the same way entry setup does; keep any later stack-home
  refinement shared instead of reintroducing return-side ABI fallback.
- Step 3.3.2 is now treated as a covered checkpoint rather than a remaining
  executor packet; do not reopen short-circuit churn unless a new uncovered
  consumer seam is identified.
- Keep Step 3.3.3 focused on residual call/result/return boundary cleanup; do
  not silently widen back into short-circuit or compare-join helper work.

## Proof

Lifecycle-only review checkpoint that moved active execution from Step
3.3.1/3.3.2 onto Step 3.3.3. No new build or test command was run for this
lifecycle repair.
