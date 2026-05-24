# Prealloc Responsibility Map And Layout Plan

## Intent

Create an internal responsibility model for `src/backend/prealloc` and use it to
plan follow-up layout/consolidation work.

Unlike AArch64 codegen, this subsystem does not have a clear external reference
model. The first step should therefore be discovery: describe what prealloc is
supposed to own, where the current files fit, and which boundaries should be
cleaned up before future x86/RISC-V work leans more heavily on prealloc.

This is primarily a planning idea. Its main output should be a map and multiple
smaller follow-up ideas, not a broad implementation rewrite.

## Working Model

Treat `prealloc` as the target-parameterized preparation layer between semantic
BIR and target MIR/codegen. It may compute stable facts, plans, and identities
that targets consume, but it should not emit target instructions or own final
target-specific register spelling.

A useful initial responsibility model is:

- pipeline coordination and phase ordering
- control-flow normalization, out-of-SSA, and move bundles
- liveness and register-allocation planning
- stack layout and frame planning
- call/return ABI movement plans
- value homes, storage encodings, and publication plans
- runtime helper carriers for i128/f128/atomics/intrinsics/inline asm
- dynamic stack and variadic entry plans
- target register profile facts
- prepared-module lookup/accessor helpers
- prepared printer/debug dump support

The audit should refine this model based on the actual code.

## Current Hotspots

Start by inspecting these large or structurally important files:

- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/out_of_ssa.cpp`
- `src/backend/prealloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/f128_runtime_helpers.cpp`
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.*`
- `src/backend/prealloc/prepared_lookups.*`
- `src/backend/prealloc/decoded_home_storage.*`
- `src/backend/prealloc/prepared_printer/`
- `src/backend/prealloc/regalloc/`
- `src/backend/prealloc/stack_layout/`

## Questions To Answer

- Which files define durable data contracts, and which only implement one
  phase?
- Which headers are oversized because several unrelated records share one file?
- Which `.cpp` files are large because they implement multiple phases?
- Which helper surfaces were recently added for AArch64 forward migration and
  now need better homes?
- Which pieces are target-neutral, target-parameterized, or accidentally
  target-specific?
- Which prepared-printer code mirrors data families cleanly, and which printer
  files need to follow future layout moves?

## Required Output

The runbook generated from this idea should produce:

1. A file-to-responsibility map for `src/backend/prealloc`.
2. A proposed stable package/family model for the subsystem.
3. A prioritized list of cleanup candidates, separated into:
   - header/data-contract splits
   - `.cpp` phase splits or merges
   - helper relocation/renaming
   - prepared-printer alignment
   - follow-up semantic migration candidates, if any
4. Multiple new `ideas/open/*.md` follow-up ideas, each scoped to one focused
   cleanup slice.

## Constraints

- Do not do broad implementation cleanup in the audit itself.
- Do not move target-specific instruction emission or register spelling into
  prealloc.
- Do not split data contracts so finely that targets need to include many tiny
  headers for one logical plan.
- Do not hide semantic changes inside layout work.
- Preserve current tests and prepared dump behavior.
- If the audit discovers a real semantic ownership issue, record it as a
  separate follow-up idea rather than mixing it with layout cleanup.

## Acceptance

- `src/backend/prealloc` has a documented responsibility map.
- The audit proposes a coherent internal layout model despite lacking an
  external reference.
- Follow-up ideas exist for the highest-value cleanup slices.
- The active plan closes without leaving broad code movement half-finished.
