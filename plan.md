# Semantic LIR-To-BIR Gap Closure For X86 Backend

Status: Active
Source Idea: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Activated from: ideas/open/57_x86_backend_c_testsuite_capability_families.md

## Purpose

Turn idea 58 into an execution runbook for backend cases that still stop before
prepared x86 consumption because semantic `lir_to_bir` lowering is missing.

## Goal

Repair the active semantic-lowering failure family so owned cases reach the
normal prepared-x86 route, then rehome any graduated cases into the more
precise downstream idea that matches their next failure mode.

## Core Rule

Do not claim x86 backend progress by adding emitter-side special cases for
tests that still fail before prepared emission exists. Fix the upstream
lowering or canonical BIR shaping seam first.

## Read First

- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_scalar.cpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `tests/backend/codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir.c`
- `tests/backend/codegen_route_x86_64_variadic_pair_second_observe_semantic_bir.c`

## Scope

- backend failures with `error: x86 backend emit path requires semantic
  lir_to_bir lowering before ...`
- upstream semantic/BIR shaping work that lets those cases reach prepared x86
- durable rehoming of cases that graduate into idea 59, 60, 61, or 62

## Non-Goals

- x86 emitter-side matcher growth for cases that still fail before prepared
  emission
- reopening runtime-correctness ownership now closed under idea 63
- stack/addressing work whose durable failure mode is better owned by idea 62
- call-bundle, prepared-module, or CFG handoff work once a case has already
  advanced into ideas 59 or 61

## Working Model

- this leaf still owns the broadest remaining failure family in the umbrella
- the next useful packet should isolate one real semantic-lowering seam, not
  a mixed grab-bag of named testcases
- representative backend route tests are good audit targets because they
  surface the semantic BIR gap without requiring c-testsuite-only diagnosis
- when a case no longer fails in this diagnostic family, preserve that routing
  fact in `todo.md` and move execution to the next still-owned seam

## Execution Rules

- prefer one semantic-lowering seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed lowering path
- when a targeted case graduates into another open idea, record that in
  `todo.md` and keep this runbook focused on the remaining semantic bucket
- reject testcase-shaped lowering shortcuts that only make one named case pass

## Step 1: Audit The Active Semantic-Lowering Family

Goal: confirm the current owned failures and isolate the first semantic/BIR
seam that keeps them from reaching prepared x86 consumption.

Primary targets:

- representative failing backend route tests from idea 58
- owned c-testsuite cases that still stop in the same diagnostic family
- `src/backend/prealloc/legalize.cpp`

Actions:

- reproduce a narrow owned subset that still reports the semantic
  `lir_to_bir` lowering diagnostic
- group the current failures by the missing semantic/BIR capability rather than
  by testcase name
- choose one concrete first seam and identify the nearest protective backend
  coverage for it
- note any cases that have already graduated into idea 62, 59, 60, or 61 so
  they do not get pulled back into this runbook

Completion check:

- the first implementation packet is narrowed to one semantic-lowering seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Refresh Remaining Idea-58 Ownership And Pick The Next Seam

Goal: keep the active packet focused on one still-owned semantic-lowering seam
after each accepted graduation out of idea 58.

Primary targets:

- the narrow owned subset that still reports the semantic `lir_to_bir`
  lowering diagnostic
- `todo.md` routing notes for recently graduated cases
- shared semantic/BIR lowering files nearest the next failure seam

Actions:

- rerun or inspect the narrow owned subset after the latest accepted slice
- exclude cases that now belong in ideas 62, 59, 60, or 61 instead of pulling
  them back into idea 58
- identify one concrete next semantic/BIR seam and the nearest backend
  coverage that can prove it without relying only on a named c-testsuite case
- keep the next packet upstream and semantic rather than drifting into
  prepared-emitter work

Completion check:

- the next executor packet is narrowed to one still-owned semantic/BIR seam
  with named proof targets after the latest graduations are accounted for

## Step 2.2: Repair The Selected Semantic/BIR Seam

Goal: implement the smallest durable semantic/BIR lowering repair that advances
the selected still-owned cases into prepared x86 consumption.

Primary targets:

- shared semantic/BIR lowering code for the chosen seam
- prepared contract structs only if the missing meaning is genuinely upstream

Actions:

- repair the selected semantic/BIR seam in the layer that owns the missing
  meaning
- keep new contract facts target-independent when the seam is not x86-specific
- avoid emitter-local fallback paths that merely bypass the diagnostic
- confirm the targeted cases now move past the old idea-58 failure family

Completion check:

- the targeted owned cases no longer fail with the semantic `lir_to_bir`
  lowering diagnostic and instead reach the next downstream owned route

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-58 diagnostic family and
preserves explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed lowering path
- record in `todo.md` when advanced cases now belong in ideas 62, 59, 60, or
  61
- only return to Step 2.1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-58 diagnostic family and
  preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 58 Is Exhausted

Goal: keep repeating the Step 2.1 -> 2.3 loop until the remaining failures no
longer belong to idea 58.

Actions:

- keep idea 58 active only while cases still fail before prepared x86 handoff
  with the semantic `lir_to_bir` diagnostic family
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 2.1 for a still-owned seam, or
  lifecycle state is ready to hand off/close because idea 58 no longer owns
  the remaining failures
