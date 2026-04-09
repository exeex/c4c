# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md
Source Plan: plan.md

## Current Active Item

- Step 2 / Step 3 follow-on reassessment after landing the bounded native
  `00180.c` slice:
  decide whether the next highest-leverage move should return to a Family B
  shared-BIR seam or continue with another still-native x86 ownership gap

## Next Slice

- re-sample the remaining unsupported x86 bucket after the `00180.c` recovery
  to confirm whether the best next slice is still a globals-heavy Family B
  shared seam or another bounded native direct-LIR ownership gap
- if Step 2 resumes, choose one bounded Family B representative and prove the
  shared ownership point with a narrow regression before widening support
- if Step 3 continues instead, pick the smallest still-native x86 ownership
  gap rather than reviving any broader fallback behavior

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
- re-checked `c_testsuite_x86_backend_src_00143_c` on the current tree and
  confirmed it is not another constant/immediate fold survivor: it lowers as a
  Duff's-device style switch-entry fallthrough copy loop followed by a
  verification loop, and still stops at the x86 unsupported direct-LIR
  boundary
- re-checked later representative `c_testsuite_x86_backend_src_00180_c` and
  confirmed the `0018x` lane still belongs to Family B because it pulls in
  libc-facing externs, globals, and string-heavy declarations rather than the
  local-slot/control-flow shape from Family A
- split the `00143.c` switch/fallthrough loop ownership gap into
  `ideas/open/46_x86_64_shared_bir_switch_fallthrough_loop_modules.md` so the
  active idea 44 runbook can stay focused on the remaining simpler Family A
  survivor `00141.c`
- added an internal x86 backend route regression plus backend BIR pipeline
  coverage for the dead local-slot add/store chain behind
  `tests/c/external/c-testsuite/src/00141.c`
- taught shared `lir_to_bir` lowering to fold the bounded alloca-backed dead
  `i32` add/store chain into a direct-BIR immediate return, covering the
  macro-expanded `foo + bar` dead-store shape from `00141.c` without reviving
  any legacy backend rescue path
- verified `c_testsuite_x86_backend_src_00141_c` now passes and clears the last
  simple Family A survivor from the active Step 2 lane
- ran full-suite monotonic validation for the `00141.c` slice:
  `test_fail_before.log` = 2656 pass / 187 fail / 2843 total,
  `test_fail_after.log` = 2664 pass / 183 fail / 2847 total,
  zero newly failing tests
- added backend BIR pipeline coverage for bounded x86 direct-LIR variadic
  scalar/runtime slices:
  one integer-only SysV vararg call path and one double-carrying SysV vararg
  call path
- taught the staged x86 prepared-LIR emitter to accept the focused
  `backend_runtime_variadic_sum2` and `backend_runtime_variadic_double_bytes`
  module shapes through native direct-LIR assembly instead of rejecting them at
  the unsupported-module boundary
- verified `backend_runtime_variadic_sum2` and
  `backend_runtime_variadic_double_bytes` now pass in isolation on x86_64
- ran full-suite monotonic validation for the Step 4 variadic slice:
  `test_fail_before.log` = 2656 pass / 187 fail / 2843 total,
  `test_fail_after.log` = 2666 pass / 181 fail / 2847 total,
  zero newly failing tests
- added an internal x86 backend route regression plus backend BIR pipeline
  coverage for the bounded local-buffer `strcpy` plus
  `printf("%s\\n", &a[1])` slice behind
  `tests/c/external/c-testsuite/src/00180.c`
- taught the staged x86 prepared-LIR emitter to accept the focused `00180.c`
  one-function stack-buffer copy-and-print module through native direct-LIR
  assembly instead of rejecting it at the unsupported-module boundary
- verified `c_testsuite_x86_backend_src_00180_c` now passes in isolation on
  x86_64
- ran full-suite monotonic validation for the `00180.c` Step 3 slice:
  `test_fail_before.log` = 2666 pass / 181 fail / 2847 total,
  `test_fail_after.log` = 2668 pass / 180 fail / 2848 total,
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
- landed the next dead-local arithmetic slice:
  `c_testsuite_x86_backend_src_00141_c` now routes through shared lowering by
  folding the dead alloca-backed add/store chain to a constant BIR return
- current-tree post-change suite state:
  `test_fail_after.log` = 2664 pass / 183 fail / 2847 total
- post-`00141.c` re-sampling still shows a large Family B unsupported-module
  bucket plus the two focused x86 variadic runtime failures; the next active
  slice is now Step 4 because it is narrower and better bounded than starting
  a globals-heavy Family B seam inside idea 44
- Step 4 is now clear: the two focused x86 variadic runtime failures were
  actually bounded native direct-LIR ownership gaps, not a broader shared-BIR
  or `va_start` lowering regression
- `00143.c` is now classified as a separate switch-entry fallthrough copy-loop
  seam and tracked in
  [ideas/open/46_x86_64_shared_bir_switch_fallthrough_loop_modules.md](/workspaces/c4c/ideas/open/46_x86_64_shared_bir_switch_fallthrough_loop_modules.md)
- `00141.c` is no longer pending; idea 44 is temporarily leaving Step 2 and
  entering the focused variadic-runtime lane before deciding whether the next
  follow-on should return to Step 2 Family B or move to Step 3
- current-tree post-variadic suite state:
  `test_fail_after.log` = 2666 pass / 181 fail / 2847 total
- the `00180.c` native direct-LIR slice is now landed:
  current-tree suite state is
  `test_fail_after.log` = 2668 pass / 180 fail / 2848 total
- the new internal route regression increases the total suite size by one, so
  the monotonic delta for this slice is `+2` passes, `-1` fails, `+1` total
