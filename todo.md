Status: Active
Source Idea Path: ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect The Current Memory Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from Step 1 of `plan.md`; no
implementation packet has run yet.

## Suggested Next

Delegate Step 1 to an executor: inspect the current memory boundary, identify
the macro-included declaration pattern, and list which declarations belong in
`lowering.hpp` versus pure helper declarations in `memory_helpers.hpp`.

## Watchouts

- Do not edit implementation files as part of lifecycle activation.
- Do not rewrite source idea intent unless durable intent changes.
- Do not convert stateful memory lowering into broad free functions taking
  `BirFunctionLowerer& self`.
- Do not add new `.hpp` files or rewrite test expectations.

## Proof

Lifecycle-only activation. No build or test proof required yet.
