Status: Active
Source Idea Path: ideas/open/02_bir-memory-helper-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Equivalent Helpers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from `ideas/open/02_bir-memory-helper-consolidation.md`.

## Suggested Next

Execute `Step 1: Inventory Equivalent Helpers` from `plan.md`.

## Watchouts

- Do not create additional `.hpp` files.
- Do not split `coordinator.cpp`.
- Keep `BirFunctionLowerer` as the memory state owner.
- Keep shared helpers pure and argument-driven behind `memory_helpers.hpp`.
- Preserve behavior; do not rewrite expectations as proof.

## Proof

Lifecycle-only activation; no build or test proof required.
