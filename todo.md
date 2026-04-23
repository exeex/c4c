# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.5
Current Step Title: Migrate Atomics And Intrinsics Lowering Families
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed step 2.5's remaining shared-surface audit by moving the
atomics/intrinsics-only enum inventories (`AtomicOrdering`, `AtomicRmwOp`,
`IntrinsicOp`) out of `x86_codegen.hpp` and into
`lowering/atomics_intrinsics_lowering.hpp`, leaving the transitional
`X86Codegen` header with only forward declarations plus the member entrypoints
that still need those seam-owned types.

## Suggested Next

Re-audit step 2.5 for whether the remaining `X86Codegen` member declarations
for atomics/intrinsics are the irreducible shared dispatch surface or whether a
later runbook step should move that ownership boundary altogether.

## Watchouts

- `atomics.cpp` and `intrinsics.cpp` are intentionally dormant compatibility
  surfaces now; do not repopulate them just to preserve the legacy bucket.
- Keep explicit ISA lowering in the canonical lowering stack; do not leak this
  family into prepared dispatch or module emission helpers.
- Preserve the reviewed combined seam shape for now instead of splitting
  atomics and intrinsics into separate ownership buckets mid-step.
- The active lowering entrypoints still remain declared on `X86Codegen`
  because C++ still needs those member declarations on the class definition
  even though the seam now owns the enum inventories and implementations.

## Proof

Step 2.5 atomics/intrinsics shared-surface enum ownership follow-up on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
