# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2: recover the highest-leverage shared BIR seam for the remaining x86
  unsupported modules, continuing from `00143.c` now that the string-literal
  compare-with-phi-return slice in `00138.c` lowers through shared BIR

## Next Slice

- add the narrowest regression around
  `c_testsuite_x86_backend_src_00143_c`
- determine whether `00143.c` should still be treated as the next
  control-flow-heavy Family A survivor or whether it now needs a separate
  switch/loop-specific parked seam from the fixed `00138.c` string-literal fold
- re-check one later `0018x`-series x86 backend failure after `00143.c` to keep
  the Family A vs Family B split current
- keep the next slice scoped away from the already-fixed `00057.c`,
  `00086.c`, and `00138.c` constant/immediate folds unless another survivor
  proves blocked on the same ownership point

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
- added an internal x86 backend route regression plus backend BIR lowering and
  pipeline coverage for the string-literal compare-with-phi-return slice behind
  `tests/c/external/c-testsuite/src/00138.c`
- taught shared `lir_to_bir` lowering to fold the bounded string-pool local
  pointer compare-and-phi-return module into a direct BIR immediate return,
  covering the `char *a = "hi"; return (a[1] == 'i') ? 0 : 1;` shape without
  reviving any legacy backend rescue path
- verified `c_testsuite_x86_backend_src_00138_c` now passes while nearby
  representative `c_testsuite_x86_backend_src_00143_c` still fails at the same
  unsupported boundary
- ran full-suite monotonic validation for the `00138.c` slice:
  `test_fail_before.log` = 2656 pass / 187 fail / 2843 total,
  `test_fail_after.log` = 2662 pass / 184 fail / 2846 total,
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
- landed the next string-backed/control-flow slice:
  `c_testsuite_x86_backend_src_00138_c` now routes through shared lowering by
  folding the string-pool local-pointer compare and phi return to a constant
  BIR return
- `00143.c` still looks like the next control-flow-heavy survivor and should be
  re-checked before it is grouped with later Family A or Family B failures
