# x86_64 Backend Bring-Up After BIR Cutover

Status: Open
Last Updated: 2026-04-09

## Why This Idea

Recent backend refactoring was validated primarily on ARM/AArch64-oriented
paths. On the current x86_64 host, the same tree now exposes a large native
backend coverage gap after the BIR cutover and legacy rescue removal from
idea 41.

This is not the old x86_64 Linux ABI recovery problem from
`ideas/closed/x86_64_linux_failures_plan.md`. That work largely recovered the
runtime ABI surface on the earlier architecture. The new failure shape is that
many `x86_backend` tests now stop at explicit unsupported-slice diagnostics
instead of silently succeeding through legacy backend IR or LLVM asm fallback.

## Source Context

- `ideas/closed/40_target_profile_and_execution_domain_foundation.md`
- `ideas/closed/41_bir_full_coverage_and_ir_legacy_removal.md`
- `ideas/closed/x86_64_linux_failures_plan.md`
- `test_x86_64_backend_baseline_20260409T140902Z.log`

## Baseline Snapshot

Captured on 2026-04-09 on the current x86_64 host:

- full suite: `2843` tests
- passing: `2656`
- failing: `187`
- pass rate: `93%`

Failure buckets from `test_x86_64_backend_baseline_20260409T140902Z.log`:

- `179` `c_testsuite_x86_backend_*` failures
- `2` internal backend runtime failures:
  - `backend_runtime_variadic_double_bytes`
  - `backend_runtime_variadic_sum2`
- `6` `cpp_eastl_*` parse recipe failures

This idea owns the x86_64 backend bring-up buckets only. The EASTL parser
failures are adjacent noise and should remain out of scope unless a specific
backend-oriented slice proves they are coupled.

## What Changed In Idea 41

Idea 41 removed the old ways unsupported native-asm requests could still look
"green":

- legacy backend IR as the emitter-facing fallback contract was removed
- `--codegen asm` no longer rescues unsupported backend slices through LLVM asm
- unsupported direct-LIR/direct-BIR shapes now fail explicitly at the backend
  boundary

That behavior is intentional. The current x86_64 failures mostly indicate
native coverage gaps, not a regression where fallback needs to come back.

## Current Failure Shape

### Bucket 1: x86 direct-LIR / direct-BIR coverage gaps

Most failing `c_testsuite_x86_backend_*` cases now fail with the explicit
diagnostic emitted from `src/backend/backend.cpp`:

`x86 backend emitter does not support this direct LIR module; only direct-LIR slices that lower natively or through direct BIR are currently supported`

This matches the post-idea-41 contract and strongly suggests that x86_64 still
lacks enough native direct-LIR or shared direct-BIR coverage for the test
shapes that were previously rescued.

### Bucket 2: x86 variadic runtime regressions

Two focused runtime tests still fail on x86_64:

- `backend_runtime_variadic_double_bytes`
- `backend_runtime_variadic_sum2`

These should be treated as a separate ABI/runtime lane from the large direct-LIR
coverage bucket.

### Bucket 3: Non-backend parser noise

The `cpp_eastl_*` recipe failures are currently out of scope for this idea.
They should not be silently bundled into x86 backend bring-up unless later
evidence shows a shared cause.

## Goal

Recover x86_64 native backend viability after the BIR/legacy-removal cutover by
expanding the supported direct-BIR/direct-LIR surface enough that the current
x86 backend test buckets stop failing at the unsupported-module boundary, while
keeping idea 41's no-rescue architecture intact.

## Non-Goals

- do not revive legacy backend IR
- do not reintroduce LLVM asm rescue for `--codegen asm`
- do not absorb unrelated EASTL/C++ parser failures
- do not silently widen this into an all-target backend initiative

## Working Model

- treat unsupported x86 backend cases as real missing coverage, not as
  pressure to restore fallback
- classify each failing shape by the earliest boundary:
  - shared `lir_to_bir` gap
  - x86 native direct-LIR gap
  - x86 runtime/ABI lowering bug
- land one bounded seam at a time with a targeted regression before code
  changes
- prefer cases that collapse many `c_testsuite_x86_backend_*` failures at once
  instead of chasing unrelated one-offs

## Current Execution Stance

The active runbook now has a deliberate architecture-reset lane inside Step 2.

This means some near-term slices are expected to:

- move ownership out of monolithic files such as `lir_to_bir.cpp`
- establish split module surfaces under `src/backend/lowering/lir_to_bir/`
- defer build recovery and regression work until the seam boundary is in the
  right place
- explicitly override the repo's usual test-first/build-first execution habits
  while this lane is active

That is intentional. The goal of this lane is to stop further testcase-matcher
accumulation and re-establish a backend-owned shared lowering structure before
resuming normal compile/test-driven seam recovery.

## Proposed Plan

### Step 1: Classify the current x86 backend failure matrix

Goal: reduce the 179-case `c_testsuite_x86_backend_*` bucket into a small set
of repeatable unsupported-shape families.

Concrete actions:

- sample failing cases across early, middle, and late test IDs
- capture the emitted LIR or BIR shape for representative failures
- group them by missing seam, such as:
  - unsupported memory/load-store forms
  - unsupported control-flow / phi shapes
  - unsupported call / varargs shapes
  - unsupported global-address or aggregate paths
- record which families already lower through shared BIR and which still stop
  at x86 direct-LIR

Completion check:

- the large failure bucket is reduced to a small named family list
- each family has at least one representative regression target

Step 1 classification snapshot from the current tree on 2026-04-09:

- Family A: single-function alloca-backed local-slot and control-flow modules
  that still miss both the shared BIR contracts and the surviving x86 native
  direct-LIR subset.
  Current-tree status after the `00057.c`, `00086.c`, and `00138.c` slices:
  - recovered Step 2 representatives:
    `c_testsuite_x86_backend_src_00057_c`,
    `c_testsuite_x86_backend_src_00086_c`, and
    `c_testsuite_x86_backend_src_00138_c`
  - remaining simple survivor for the active Step 2 lane:
    `c_testsuite_x86_backend_src_00141_c`
  Common shapes: entry allocas, load/store through local slots, local-array
  GEPs, compare-and-branch control flow, and simple local arithmetic.
- Family A2: switch-entry fallthrough loop modules that still look
  single-function and local-slot backed, but now appear to require a different
  shared ownership seam than the earlier constant/immediate folds.
  Representative current failure:
  `c_testsuite_x86_backend_src_00143_c`.
  Common shapes: local arrays plus pointers, a `switch` that enters a
  fallthrough copy chain, a do-while backedge, and a second verification loop.
  This seam is now parked separately in
  `ideas/open/46_x86_64_shared_bir_switch_fallthrough_loop_modules.md` so idea
  44 can stay focused on the remaining simpler Family A survivor.
- Family B: global-rich and initializer-heavy modules whose direct-LIR surface
  still exceeds the shared BIR gateway because they carry many globals, string
  pool entries, extern declarations, or helper functions. Representative
  failures from the saved baseline sample:
  `c_testsuite_x86_backend_src_00200_c`,
  `c_testsuite_x86_backend_src_00216_c`, and
  `c_testsuite_x86_backend_src_00040_c`.
  Common shapes: multi-function modules, libc-facing extern call surfaces,
  aggregate initializers, and global/string-backed pointer materialization.
- Family C: baseline drift that should not be used as a current failing target.
  The 2026-04-09 saved log still lists some cases that now pass on the current
  tree, including `c_testsuite_x86_backend_src_00065_c` and
  `c_testsuite_x86_backend_src_00116_c`.

Step 2 should start with Family A. `00057.c` is the narrowest regression
target because it isolates stack-local array allocas plus a simple compare and
branch without introducing calls, globals, or helper functions. `00086.c` and
`00138.c` were the next nearby validations once that seam moved. The current
tree now confirms the simple Family A lane is recovered through
`c_testsuite_x86_backend_src_00141_c`, whose dead local-slot add/store chain
can fold to a direct-BIR immediate return without reviving any legacy rescue
path. `00143.c` remains split into the parked Family A2 switch/fallthrough
loop initiative.

Current-tree Step 2 progress after the `00141.c` slice:

- recovered Family A representatives:
  `c_testsuite_x86_backend_src_00057_c`,
  `c_testsuite_x86_backend_src_00086_c`,
  `c_testsuite_x86_backend_src_00138_c`, and
  `c_testsuite_x86_backend_src_00141_c`
- full-suite monotonic validation now stands at:
  `2847` total,
  `2664` passing,
  `183` failing,
  zero newly failing tests versus `test_fail_before.log`
- remaining shared-BIR bring-up work is no longer the simple Family A lane;
  the next reassessment point is whether a bounded Family B seam should stay in
  this idea or whether the remaining highest-leverage work should switch to the
  Step 3 x86-native or Step 4 variadic-runtime lanes
- subsequent Step 3/native slices now confirm the next move should stay in the
  bounded x86-owned lane before reopening Family B:
  `c_testsuite_x86_backend_src_00180_c` and
  `c_testsuite_x86_backend_src_00184_c` now pass through native direct-LIR
  assembly, while nearby one-function stdio-backed survivors
  `c_testsuite_x86_backend_src_00183_c` and
  `c_testsuite_x86_backend_src_00185_c` still fail at the same unsupported
  module boundary
- current-tree full-suite monotonic validation after the `00184.c` slice now
  stands at:
  `2849` total,
  `2670` passing,
  `179` failing,
  zero newly failing tests versus `test_fail_before.log`

### Step 2: Recover the highest-leverage shared BIR seam

Goal: promote the smallest shared `lir_to_bir` pattern cluster that unlocks the
largest x86 backend failure bucket.

Current subphase: architecture reset before new coverage claims.

Immediate subphase actions:

- move helper ownership from `lir_to_bir.cpp` into split lowering files
- wire pass-oriented entry surfaces and shared metadata first
- tolerate and intentionally ignore a temporary build/test gap while these
  ownership moves are still being connected
- once the seam stabilizes, restore narrow compile checks
- only after compile recovery, resume targeted regressions and broader suite
  validation

Concrete actions:

- choose the most common unsupported family from Step 1
- add the narrowest backend pipeline regression that proves the missing shared
  lowering seam
- implement the smallest `lir_to_bir` / `bir_validate` / `bir_printer`
  expansion needed
- verify the shape now routes through shared BIR on x86_64

Completion check:

- at least one representative x86 failing case no longer fails at the
  unsupported direct-LIR boundary
- nearby backend route tests pin the newly owned seam

### Step 3: Recover the highest-leverage x86 native direct-LIR seam

Goal: where shared BIR is still intentionally narrower, restore the smallest
native x86 direct-LIR slice that is still meant to exist post-cutover.

Concrete actions:

- identify representative failures that should remain native x86-owned rather
  than promoted into shared BIR
- add focused x86 emitter tests or internal backend runtime tests
- implement only the bounded x86 emitter/lowering change needed for that seam

Completion check:

- one x86-native unsupported family becomes supported without reviving fallback

### Step 4: Fix the remaining focused x86 variadic runtime failures

Goal: clear the two targeted runtime ABI regressions after the unsupported-slice
matrix is better understood.

Concrete actions:

- run `backend_runtime_variadic_double_bytes` and
  `backend_runtime_variadic_sum2` in isolation
- compare generated IR/asm against Clang on the host triple
- determine whether the root cause lives in call lowering, ABI classification,
  or x86 emission

Completion check:

- both focused variadic runtime tests pass or are split into a narrower follow-on
  idea with a concrete root-cause note

### Step 5: Full-suite monotonic validation

Goal: prove x86 bring-up work improves the suite without regressing the rest of
the tree.

Concrete actions:

- keep before/after full-suite logs for each landed slice
- require monotonic `ctest` results
- spin any newly discovered but separate initiative into its own open idea

Completion check:

- total failing tests are below the 187-case baseline
- no previously passing test becomes failing

## Acceptance Criteria

- the current x86 backend fail surface is classified into explicit seam
  families instead of one opaque 179-case bucket
- at least one high-leverage x86 backend family is recovered without restoring
  legacy fallback behavior
- x86 variadic runtime regressions are either fixed or isolated with a precise
  follow-on note
- full-suite results improve monotonically from the 2026-04-09 baseline

## Notes

- The repo currently has a planning-state inconsistency: active `plan.md` /
  `todo.md` refer to a missing `ideas/open/43_c_style_cast_reference_followups_review.md`.
  This idea is intentionally recorded as open inventory only; it is not yet the
  active plan.
