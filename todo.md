# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2: recover the highest-leverage shared BIR seam for the remaining x86
  unsupported modules, continuing from `00138.c` now that the signed narrow
  local-slot increment-and-compare slice in `00086.c` lowers through shared BIR

## Next Slice

- add the narrowest regression around
  `c_testsuite_x86_backend_src_00138_c`
- determine whether `00138.c` is now the first global-or-string materialization
  seam that should move into shared BIR, or whether it belongs to a separate
  parked family from `00143.c`
- re-check representative survivors
  `c_testsuite_x86_backend_src_00143_c` and one later `0018x`-series x86
  backend failure to confirm the family split after `00086.c`
- keep the new slice scoped away from the already-fixed local-slot
  increment-and-compare matcher unless another narrow-slot variant proves
  blocked on the same ownership point

## Completed Items

- activated idea 44 into `plan.md`
- created `todo.md` linked to the same source idea
- classified the x86 backend unsupported-module bucket into three concrete
  families:
  Family A single-function alloca-backed local-slot/control-flow failures,
  Family B global-rich initializer-heavy modules, and
  Family C stale baseline entries that already pass on the current tree
- verified current-tree drift on representative cases:
  `c_testsuite_x86_backend_src_00065_c` and
  `c_testsuite_x86_backend_src_00116_c` now pass, while
  `c_testsuite_x86_backend_src_00057_c`,
  `c_testsuite_x86_backend_src_00086_c`,
  `c_testsuite_x86_backend_src_00138_c`, and
  `c_testsuite_x86_backend_src_00143_c` still fail at the x86 unsupported
  direct-LIR/direct-BIR boundary
- added an internal x86 backend route regression for
  `tests/c/external/c-testsuite/src/00057.c` that requires native asm output
  and forbids LLVM text fallback
- taught shared `lir_to_bir` conditional-return lowering to fold the prepared
  dead-local `sizeof` compare-and-branch slice when the compare width stays
  wider than the branch/return width
- verified `c_testsuite_x86_backend_src_00057_c` now passes while nearby
  representatives `c_testsuite_x86_backend_src_00086_c` and
  `c_testsuite_x86_backend_src_00138_c` still fail at the same unsupported
  boundary
- ran full-suite monotonic validation against the saved 2026-04-09 x86
  baseline:
  `test_fail_before.log` = 2656 pass / 187 fail / 2843 total,
  `test_fail_after.log` = 2658 pass / 186 fail / 2844 total,
  zero newly failing tests
- added an internal x86 backend route regression plus backend BIR pipeline
  coverage for the signed `i16` local-slot increment-and-compare slice behind
  `tests/c/external/c-testsuite/src/00086.c`
- taught shared `lir_to_bir` lowering to fold a bounded no-param signed
  narrow-slot update-and-compare conditional-return module into direct BIR,
  covering the `short x; x = x + 1; if (x != 1)` shape without reviving any
  legacy backend rescue path
- verified `c_testsuite_x86_backend_src_00086_c` now passes while nearby
  representatives `c_testsuite_x86_backend_src_00138_c` and
  `c_testsuite_x86_backend_src_00143_c` still fail at the unsupported
  boundary
- ran full-suite monotonic validation for the `00086.c` slice:
  `test_fail_before.log` = 2656 pass / 187 fail / 2843 total,
  `test_fail_after.log` = 2660 pass / 185 fail / 2845 total,
  zero newly failing tests

## Blockers

- none recorded

## Notes To Resume

- baseline reference: [test_x86_64_backend_baseline_20260409T140902Z.log](/workspaces/c4c/test_x86_64_backend_baseline_20260409T140902Z.log)
- durable classification snapshot recorded in
  [ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md](/workspaces/c4c/ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md)
- keep `cpp_eastl_*` parse failures out of scope unless a backend-owned coupling
  is proven
- preserve idea 41's no-rescue architecture while shrinking the unsupported
  x86 surface
- landed Step 2 starter slice:
  `c_testsuite_x86_backend_src_00057_c` now routes through shared lowering by
  folding the prepared dead-local `sizeof` branch to a constant BIR return
- landed the next local-slot/control-flow slice:
  `c_testsuite_x86_backend_src_00086_c` now routes through shared lowering by
  folding the prepared signed narrow-slot increment-and-compare branch to a
  constant BIR return
- `00138.c` still fails but now looks more like string/global materialization
  than the closed `00057.c`/`00086.c` local-slot seam; `00143.c` still looks
  like a separate control-flow-heavy survivor that needs re-checking before it
  is grouped with `00138.c`
