Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Split Scalar-Only Cases

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from `ideas/open/03_bir-memory-coordinator-dispatch-split.md`.

## Suggested Next

Execute `Step 1: Split Scalar-Only Cases` from `plan.md`.

## Watchouts

- Do not create new `.hpp` files.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Keep memory state ownership on `BirFunctionLowerer`.
- Split by real instruction-family boundaries, not one file per narrow case.
- Preserve behavior and diagnostics; do not rewrite expectations as proof.

## Proof

Lifecycle-only activation; no build or test proof required.
