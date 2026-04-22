# Call-Bundle And Multi-Function Prepared-Module Consumption

Status: Active
Source Idea: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Activated from: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md

## Purpose

Switch the active backend leaf from idea 68 to the prepared-module and
call-bundle consumption leaf now that full `c_testsuite_x86_backend_src_00204_c`
no longer stops in the authoritative prepared local-slot family and instead
stabilizes on the downstream prepared-module restriction.

## Goal

Repair one prepared-module or prepared call-bundle consumption seam at a time
so owned cases move past the current module-level rejection without adding
bounded `main + helper` or named-call x86 fast paths.

## Core Rule

Do not claim prepared-module progress through one bounded multi-function entry
lane or one call spelling when x86 still lacks a generic prepared-module
traversal or authoritative call-bundle consumer for the same-family cases.

## Read First

- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_x86_route_debug_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that stop with the prepared-module restriction owned by idea
  61, including the newly graduated full-case `00204.c` route
- authoritative prepared call-bundle consumption failures when the owned route
  reaches module emission but x86 still rejects the published call contract
- shared prepared module, function-relationship, value-home, or call-bundle
  contracts when the missing meaning is upstream of x86 module emission
- durable rehoming of cases that advance out of prepared-module restriction and
  into a later runtime or correctness leaf

## Non-Goals

- reopening idea-68 ownership unless the top-level failure regresses back to
  the authoritative prepared local-slot family
- adding x86-only entry-lane or helper-lane matchers for one bounded
  multi-function module shape
- reopening local call ABI inference when authoritative prepared
  `BeforeCall` or `AfterCall` ownership already exists
- trace-only observability work that still belongs in idea 67

## Working Model

- keep one prepared-module traversal or prepared call-bundle seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  prove the seam without collapsing the packet into one testcase
- extend shared prepared contracts first when x86 lacks a target-independent
  module-relationship or call-bundle fact it should already consume
- route cases back out as soon as the next real blocker belongs in another leaf

## Execution Rules

- prefer one prepared-module or call-bundle seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed prepared-module or call-bundle consumption path
- record in `todo.md` when advanced cases now belong in another downstream leaf
- reject bounded `main + helper` or named-call growth that only moves one owned
  route forward

## Step 1: Refresh Idea-61 Ownership And Confirm The Next Prepared-Module Seam

Goal: confirm that `c_testsuite_x86_backend_src_00204_c` and any adjacent owned
cases now belong to idea 61 and identify the exact prepared-module traversal or
call-bundle seam that currently blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route or boundary coverage nearest the current
  prepared-module restriction
- shared prepared-module and call-bundle contract surfaces near the current
  failure

Actions:

- rerun or inspect the narrow subset that shows the full `00204.c` route now
  fails only in the idea-61 prepared-module family
- confirm that the observed blocker is not better explained by idea 60, 68, or
  a runtime-correctness leaf
- identify whether the missing meaning is module traversal, same-module symbol
  classification, direct variadic runtime-call handling, or authoritative
  `BeforeCall` / `AfterCall` bundle consumption
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-61 seam with
  named proof targets and a clear ownership boundary

## Step 2.1: Preserve The Rehomed Boundary And Reject Entry-Lane Overfit

Goal: keep the rehomed `00204.c` baseline anchored to the current idea-61
restriction while rejecting probes that only add another bounded
multi-function entry lane or regress the focused `myprintf` guardrail.

Primary targets:

- `tests/backend/backend_x86_route_debug_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

Actions:

- treat the focused `myprintf` rejection as a guardrail for any remaining idea-61
  repair on this route
- reject module-shape widening that only accepts one bounded `main-entry +
  helper` topology without consuming a generic prepared-module relationship
- reject local ABI fallback that bypasses authoritative prepared call-bundle
  ownership
- keep the current packet record explicit that `00204.c` now belongs to idea 61
  because the top-level failure is the prepared-module restriction

Completion check:

- the runbook and `todo.md` clearly preserve the rehomed baseline and the
  route-drift rejection that blocks another bounded entry-lane shortcut

## Step 2.2: Repair The Current Prepared-Module Or Call-Bundle Seam

Goal: implement the smallest durable repair that lets the owned full-module
route clear the current prepared-module restriction without regressing the
focused `myprintf` route or reopening local-slot ownership.

Primary targets:

- shared prepared module, function-relationship, value-home, or call-bundle
  contracts for the chosen seam
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/prep/prealloc.hpp` when the current module or call contract is
  not expressive enough for generic consumption

Actions:

- repair the selected seam at the layer that owns the missing meaning
- prefer contract-first fixes when x86 is missing a target-independent prepared
  fact
- make x86 consume normalized prepared-module traversal or prepared call-bundle
  ownership instead of adding one more bounded topology matcher
- keep the repair generic across nearby same-family prepared-module routes
  instead of admitting another testcase-shaped fast path
- confirm the targeted cases either clear the old idea-61 failure family or
  expose a stable downstream runtime/correctness issue without reopening idea
  68

Completion check:

- the targeted owned case no longer fails for the selected idea-61 seam and
  either emits successfully or graduates cleanly into a downstream non-idea-61
  leaf

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-61 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed prepared-module or call-bundle path
- record in `todo.md` when advanced cases now belong in another leaf
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-61 prepared-module or
  call-bundle family and preserve clear routing for any graduated downstream
  cases

## Step 3: Continue The Loop Until Idea 61 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 61.

Actions:

- keep idea 61 active only while cases still fail for prepared-module or
  authoritative call-bundle reasons that are not better explained by another
  open leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-61 seam,
  or lifecycle state is ready to hand off or close because idea 61 no longer
  owns the remaining failures
