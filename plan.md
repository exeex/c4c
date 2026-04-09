# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md

## Purpose

Recover x86_64 native backend viability after the BIR cutover by shrinking the
current unsupported-module failure surface without restoring any legacy rescue
paths removed by idea 41.

## Goal

Classify the current x86 backend failures into repeatable seam families, then
recover the highest-leverage shared-BIR and x86-native slices one bounded,
test-backed seam at a time.

## Core Rule

Do not revive legacy backend IR or LLVM asm rescue. Treat explicit unsupported
x86 backend failures as real missing coverage and fix the owning lowering or
emitter seam directly.

## Read First

- [ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md](/workspaces/c4c/ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md)
- [test_x86_64_backend_baseline_20260409T140902Z.log](/workspaces/c4c/test_x86_64_backend_baseline_20260409T140902Z.log)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)

## Current Scope

- `c_testsuite_x86_backend_*` failures that now stop at explicit unsupported
  direct-LIR or direct-BIR boundaries
- the two focused x86 variadic runtime failures:
  `backend_runtime_variadic_double_bytes` and
  `backend_runtime_variadic_sum2`
- monotonic full-suite validation against the 2026-04-09 x86_64 baseline

## Non-Goals

- do not restore fallback behavior removed by idea 41
- do not absorb the `cpp_eastl_*` parser failures into this plan
- do not widen this into an all-target backend initiative
- do not chase unrelated one-off failures outside the active seam family

## Working Model

- classify failures by earliest owning boundary:
  - shared `lir_to_bir` gap
  - x86 native direct-LIR gap
  - x86 runtime or ABI lowering bug
- prefer seams that collapse many x86 backend failures at once
- add the narrowest regression that proves ownership before broadening support
- preserve explicit unsupported diagnostics for still-unowned shapes

## Execution Phases

This runbook now has an explicit front-loaded structure-reset phase before
normal compile/test discipline resumes.

Phase 1: Ownership wiring

- prioritize moving ownership into the right `lir_to_bir/*` and backend
  modules even if the tree is temporarily not build-clean
- allow narrow slices that only re-home API surface, helpers, and metadata
  without immediately restoring compilation or tests
- while this phase is active, explicitly ignore the usual build-first,
  test-first, and regression-first expectations from
  `prompts/EXECUTE_PLAN.md`; those requirements are suspended until Phase 2
  begins
- forbid new testcase-shaped x86 direct-LIR matchers during this phase

Phase 2: Compile recovery

- once a seam family has been re-homed enough to stabilize its ownership,
  start pulling the build back up for that seam
- prefer narrow syntax/build recovery over broad regression runs
- fix interface drift before resuming larger behavioral work

Phase 3: Regression validation

- after ownership and compilation are both back under control for a seam,
  restore the usual targeted-test and full-suite monotonic workflow
- only treat a seam as landed after targeted validation and the required
  monotonic suite check

## Execution Rules

- start each slice from the highest-priority incomplete item in `todo.md`
- during the current architecture-reset lane, it is acceptable to land a
  narrow ownership-wiring slice before build/test recovery, but only when the
  slice clearly improves module boundaries and does not claim behavioral
  completion
- more strongly: while the active slice is marked as Phase 1 ownership wiring,
  do not stop to restore build/test discipline just because the generic
  execution prompt says to do so; this runbook overrides that expectation for
  the current lane
- once a seam leaves the ownership-wiring phase, return to the normal
  test-backed workflow before claiming coverage recovery
- compare ABI-sensitive behavior against Clang when runtime lowering is unclear
- record before and after suite logs for each landed slice
- if execution uncovers separate work, write it into `ideas/open/` instead of
  silently expanding this runbook

## Step 1: Classify The Current x86 Failure Matrix

Goal: reduce the 179-case `c_testsuite_x86_backend_*` bucket into a small set
of named unsupported-shape families.

Primary targets:

- [test_x86_64_backend_baseline_20260409T140902Z.log](/workspaces/c4c/test_x86_64_backend_baseline_20260409T140902Z.log)
- x86 backend tests that fail at the unsupported-module boundary

Concrete actions:

- sample failing x86 backend cases across early, middle, and late test ids
- capture representative emitted LIR or BIR shapes for those failures
- group failures by missing seam such as memory, control flow, call, varargs,
  global-address, or aggregate handling
- record whether each family should route through shared BIR or remain
  x86-native ownership

Completion check:

- the large x86 failure bucket is reduced to a short named family list
- each family has at least one representative regression target

## Step 2: Recover The Highest-Leverage Shared BIR Seam

Goal: promote the smallest shared lowering seam that unlocks the largest x86
failure family.

Primary targets:

- shared `lir_to_bir` ownership points
- nearby validation and printer coverage

Step 2 currently begins with an architecture-reset subphase before attempting
new coverage claims:

- first re-home `lir_to_bir` ownership into split files such as
  `cfg.cpp`, `types.cpp`, `memory.cpp`, `calls.cpp`, `phi.cpp`, and
  `aggregates.cpp`
- while that ownership-wiring subphase is active, build/test recovery is not a
  gating requirement
- then recover compilation for the re-homed seams
- only then resume targeted regression additions for new shared-BIR behavior

Concrete actions:

- choose the most common Step 1 family that should lower through shared BIR
- if the current seam is still in the ownership-wiring subphase, first move the
  relevant helpers, metadata, and matcher fragments into the correct split
  module even if build/test work follows in later slices
- after ownership is stable, add the narrowest regression that proves the
  missing shared seam
- implement only the required `lir_to_bir`, validation, or printing expansion
- verify representative x86 failures now route through shared BIR instead of
  stopping at the unsupported direct-LIR boundary

Completion check:

- the targeted seam is no longer primarily owned by the monolithic
  `lir_to_bir.cpp`
- at least one representative x86 failing case no longer dies at the
  unsupported direct-LIR boundary
- nearby backend route tests pin the newly owned seam

## Step 3: Recover The Highest-Leverage x86 Native Direct-LIR Seam

Goal: restore the smallest x86-native direct-LIR slice that should still exist
after the cutover.

Primary targets:

- x86 emitter or lowering code for still-native shapes

Concrete actions:

- identify representative failures that should remain x86-owned rather than
  promoted into shared BIR
- add focused x86 emitter tests or internal backend runtime coverage
- implement the bounded x86 change needed for that seam and nothing broader

Completion check:

- one x86-native unsupported family becomes supported without reviving fallback

## Step 4: Fix The Focused x86 Variadic Runtime Failures

Goal: clear the two targeted ABI/runtime regressions once the broader matrix is
classified.

Primary targets:

- `backend_runtime_variadic_double_bytes`
- `backend_runtime_variadic_sum2`

Concrete actions:

- run both tests in isolation
- compare generated IR or asm against Clang on the host triple
- determine whether the issue belongs to call lowering, ABI classification, or
  x86 emission
- fix the smallest owning seam, or split a narrower follow-on idea if needed

Completion check:

- both targeted runtime tests pass, or a narrower follow-on idea exists with a
  concrete root-cause note

## Step 5: Full-Suite Monotonic Validation

Goal: prove the x86 bring-up work improves the suite without regressing the
rest of the tree.

Concrete actions:

- keep before and after full-suite logs for each landed slice
- require monotonic `ctest` results
- verify total failures move below the 187-case 2026-04-09 baseline
- split any newly discovered but separate initiative into its own open idea

Completion check:

- total failing tests are below the 187-case baseline
- no previously passing test becomes failing
