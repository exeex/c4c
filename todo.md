# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.5
Current Step Title: Migrate Atomics And Intrinsics Lowering Families
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Started step 2.5 by materializing the reviewed
`lowering/atomics_intrinsics_lowering.{hpp,cpp}` seam and moving the live
legacy atomic load/store/RMW/cmpxchg/fence plus explicit intrinsic lowering
implementations out of `atomics.cpp` and `intrinsics.cpp`. Both legacy files
are now dormant compatibility translation units, while the new reviewed seam
owns the active atomics and intrinsic lowering family.

## Suggested Next

Continue step 2.5 by classifying any remaining transitional helper ownership
around the new seam, especially shared declarations still living on the
transitional `X86Codegen` surface in `x86_codegen.hpp`, without moving module
or prepared-route logic into the atomics/intrinsics bucket.

## Watchouts

- The reviewed atomics/intrinsics seam still relies on transitional `X86Codegen`
  declarations in `x86_codegen.hpp`; keep that shared surface unchanged for
  now.
- `atomics.cpp` and `intrinsics.cpp` are intentionally dormant compatibility
  surfaces now; do not repopulate them just to preserve the legacy bucket.
- Keep explicit ISA lowering in the canonical lowering stack; do not leak this
  family into prepared dispatch or module emission helpers.
- Preserve the reviewed combined seam shape for now instead of splitting
  atomics and intrinsics into separate ownership buckets mid-step.

## Proof

Step 2.5 atomics/intrinsics lowering seam start on 2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
