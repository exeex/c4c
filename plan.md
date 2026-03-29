# X86 Backend C-Testsuite Convergence Runbook

Status: Active
Source Idea: ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md

## Purpose

Turn the vendored external C testsuite into explicit x86 backend-owned assembly
coverage by converting one bounded fallback-to-IR failure family at a time.

## Goal

Identify the smallest repeatable `c_testsuite_x86_backend_*` failure family,
land the narrow backend support needed for that family, and prove a monotonic
improvement in the `x86_backend` labeled surface without weakening the fallback
guard.

## Core Rule

For any case promoted by this runbook, `c4cll --codegen lir --target
x86_64-unknown-linux-gnu` must emit native x86 assembly and must not succeed by
falling back to LLVM IR.

## Read First

- [ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md](/workspaces/c4c/ideas/open/34_backend_x86_c_testsuite_backend_convergence_plan.md)
- [tests/c/external/c-testsuite/RunCase.cmake](/workspaces/c4c/tests/c/external/c-testsuite/RunCase.cmake)
- [tests/TestEntry.cmake](/workspaces/c4c/tests/TestEntry.cmake)
- [src/backend/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)

## Current Scope

- x86 backend-owned c-testsuite cases only
- backend fallback classification and one bounded lowering family
- adjacent x86 emitter or adapter files only when required by the promoted
  family

## Non-Goals

- relaxing `c_testsuite_x86_backend_*` to accept LLVM IR fallback
- changing the legacy `c_testsuite_*` frontend path semantics
- chasing unrelated failing cases outside the chosen backend mechanism family
- broad assembler, linker, or optimizer work unless a targeted family proves it
  is required

## Working Model

1. Measure the current `x86_backend` baseline and preserve the pass/fail count.
2. Bucket failing `c_testsuite_x86_backend_*` cases by shared lowering shape.
3. Pick the smallest repeatable family with one root cause.
4. Add or adjust the narrowest validating tests for that family.
5. Implement the smallest backend change that removes fallback for that family.
6. Re-run targeted and broader backend validation and record the improvement.

## Execution Rules

- Keep the legacy `c_testsuite_*` surface green as the unchanged reference.
- Treat Clang and the existing frontend surface as the behavioral oracle when
  diagnosing a backend-owned failing case.
- Do not silently absorb separate initiatives into this runbook. Record them in
  `ideas/open/` if they are distinct from the chosen failure family.
- Prefer one root cause and one shippable family per implementation slice.

## Ordered Steps

### Step 1: Reconfirm the x86 backend baseline

Goal: refresh the active pass/fail picture before choosing an implementation
slice.

Primary targets:

- [tests/c/external/c-testsuite/RunCase.cmake](/workspaces/c4c/tests/c/external/c-testsuite/RunCase.cmake)
- [tests/TestEntry.cmake](/workspaces/c4c/tests/TestEntry.cmake)

Actions:

- build the tree if needed
- run the current `x86_backend` label and capture the passing count, failing
  count, and dominant failure marker(s)
- sample a few representative failing cases to confirm they are still blocked by
  `[BACKEND_FALLBACK_IR]`

Completion check:

- the runbook has a current baseline count and representative failing cases for
  the chosen family

### Step 2: Classify one bounded fallback family

Goal: reduce the failure set into a single backend mechanism family that can be
  fixed as one slice.

Primary targets:

- [tests/c/external/c-testsuite/src](/workspaces/c4c/tests/c/external/c-testsuite/src)
- [src/backend/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)

Actions:

- inspect failing sources and any backend traces needed to identify the shared
  lowering shape
- compare against a known passing backend-owned case and against Clang output
  when needed
- write down the selected family and the excluded nearby families in
  `plan_todo.md`

Completion check:

- exactly one repeatable failure family is selected, described, and bounded

### Step 3: Add the narrowest durable validation

Goal: make the selected family observable before changing backend behavior.

Primary targets:

- [tests/c/external/c-testsuite/RunCase.cmake](/workspaces/c4c/tests/c/external/c-testsuite/RunCase.cmake)
- [tests/TestEntry.cmake](/workspaces/c4c/tests/TestEntry.cmake)

Actions:

- add or adjust the targeted backend-owned test coverage needed to demonstrate
  the chosen family
- keep the assertion focused on native x86 assembly success and existing output
  expectations
- avoid widening test scope beyond the selected family

Completion check:

- there is a targeted test slice that fails before the backend fix and encodes
  the intended backend-owned behavior

### Step 4: Implement the minimum backend support

Goal: remove fallback-to-IR for the selected family with the smallest emitter or
  adapter change.

Primary targets:

- [src/backend/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)

Actions:

- implement only the support required by the chosen lowering shape
- touch adjacent x86 backend files only if `emit.cpp` cannot contain the fix
- add brief comments only where the new lowering path is not obvious

Completion check:

- promoted family cases emit native x86 assembly and stop failing due to
  fallback

### Step 5: Prove monotonic backend progress

Goal: show that the slice improves the x86 backend surface without regressions.

Primary targets:

- [plan_todo.md](/workspaces/c4c/plan_todo.md)

Actions:

- rerun the targeted tests for the selected family
- rerun `ctest --test-dir build -L x86_backend --output-on-failure`
- if the patch claims broader backend safety, rerun
  `ctest --test-dir build -L backend --output-on-failure`
- record before/after counts, remaining blockers, and the next bounded family in
  `plan_todo.md`

Completion check:

- targeted selected-family tests pass, no previously passing backend-owned cases
  regress, and the recorded `x86_backend` pass count is monotonic
