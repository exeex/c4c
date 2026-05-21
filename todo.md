Status: Active
Source Idea Path: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Pointer-Addressed Store Boundary

# Current Packet

## Just Finished

Lifecycle switched from parked idea 360 to the split pointer-addressed store
residual. Idea 360's starting-state owner was repaired by commit `87e79a50a`,
but close was rejected because the focused regression guard remained 6/7 with
`00181` still failing.

## Suggested Next

Execute plan Step 1: localize the first post-starting-state materialized
pointer-addressed store that should mutate a Hanoi tower but leaves later
printed states unchanged.

## Watchouts

- Keep the idea 360 starting state correct: `A: 1 2 3 4`, `B/C` zero.
- Do not reintroduce the reviewed unguarded `StoreLocal` fallback without a
  localized owner and focused backend coverage.
- Keep `00170`, `00189`, and the named backend contracts stable.

## Proof

Lifecycle-only switch. Existing focused before/after logs showed 6/7 both
before and after; `c4c-regression-guard` rejected close for idea 360 because
the pass count did not strictly increase.
