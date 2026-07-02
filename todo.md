Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct The First Materialization Packet

# Current Packet

## Just Finished

Activated `plan.md` for Step 1.

## Suggested Next

Delegate Step 1 to reconstruct the first coherent materialization packet from
the 151-row `coherent_rv64_mir_materialization` lane.

## Watchouts

- Keep the 31 prepared authority rows and the `src/960209-1.c` evidence-gap row
  out of this implementation route.
- Reject filename-specific or testcase-shaped materialization shortcuts.
- Do not weaken expectations, unsupported markers, allowlists, or runtime
  comparison behavior.

## Proof

- Lifecycle-only activation; run `git diff --check -- plan.md todo.md`.
