# Scalar Expression And Terminator Selection For X86 Backend

Status: Active
Source Idea: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Activated from: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md

## Purpose

Resume the x86 scalar/prepared-emitter leaf now that
`c_testsuite_x86_backend_src_00204_c` has advanced out of idea-61
prepared-module ownership and again stops at the downstream scalar-emitter
restriction through the canonical prepared-module handoff.

## Goal

Repair one prepared scalar expression or terminator consumption seam at a time
so owned cases move past the current x86 emitter restriction without adding
named-case matcher growth.

## Core Rule

Do not claim scalar-emitter progress through testcase-shaped x86 fast paths or
expression-specific matchers when the missing ownership is still a generic
prepared value-home, move-bundle, branch-condition, or terminator-consumption
seam.

## Read First

- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `src/backend/targets/x86_64/emitter.cpp`
- `src/backend/targets/x86_64/emitter_expr.cpp`
- `src/backend/targets/x86_64/emitter_stmt.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_codegen_route_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that now stop with the x86 scalar-emitter diagnostics owned
  by idea 60, including the current minimal-return / guard-family restriction
  through the canonical prepared-module handoff
- shared prepared value-home, move-bundle, branch-condition, and control-flow
  contracts when the missing meaning is upstream of x86-specific rendering
- durable rehoming of cases that advance out of scalar emission into later
  prepared-module, call-family, or runtime leaves once scalar selection
  succeeds

## Non-Goals

- reopening semantic-lowering, short-circuit handoff, or prepared-module
  ownership that earlier ideas now handle
- adding x86-only matcher lanes for one named return, compare, or branch shape
- call-family ownership that still belongs in idea 65
- prepared-module multi-function or call-bundle work that still belongs in
  idea 61

## Working Model

- keep one prepared scalar seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  prove the seam without collapsing the packet into one testcase
- extend shared prepared contracts first when x86 lacks a generic fact it
  should consume
- route cases back out as soon as the next real blocker belongs in another
  downstream idea

## Execution Rules

- prefer one scalar expression or terminator seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  prepared value-home, move-bundle, branch-condition, or scalar terminator
  consumption
- when a targeted case graduates into ideas 61 or 65, record that in
  `todo.md` and keep this runbook focused on still-owned scalar-emitter work
- reject emitter-side named-case growth that only moves one arithmetic or
  branch spelling forward

## Step 1: Refresh Idea-60 Ownership And Confirm The Next Scalar Seam

Goal: confirm the returned idea-60 ownership for
`c_testsuite_x86_backend_src_00204_c` and identify the exact prepared scalar
return or terminator seam that now blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route coverage nearest the current scalar-emitter seam
- shared prepared contract and x86 emitter files near the current failure

Actions:

- rerun or inspect the narrow subset that now graduates `00204.c` out of idea
  61
- confirm that `00204.c` no longer fails in prepared-module consumption and
  now stops in an idea-60-owned scalar-emitter restriction
- identify the exact prepared return, value-home, move-bundle,
  branch-condition, or terminator fact that x86 fails to consume for the
  current route
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-60 scalar seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Repair Compare-Result Bool/Cast Materialization

Goal: repair the compare-result bool/cast local-slot seam that still drops
`00204.c` out at `match` before the old call-boundary target.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- shared prepared value-home or branch-condition facts only if the current
  compare-result materialization cannot be expressed from existing ownership
- `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

Actions:

- implement one generic repair for compare-result bool materialization through
  the prepared local-slot path
- keep the packet scoped to compare-result `CastInst` / bool consumption and
  do not bundle float, HFA, pointer, or call-boundary work into the same slice
- prefer contract-first repair if the missing fact belongs in shared prepared
  ownership rather than x86-local pattern growth
- confirm `match` and nearby same-family routes move past the old bool/cast
  dropout without adding testcase-shaped recognition

Completion check:

- the targeted bool/cast family no longer fails for the current compare-result
  local-slot seam, and any remaining `00204.c` failure is clearly a different
  downstream family

## Step 2.2: Repair Float/HFA Local-Slot Consumption

Goal: repair the float/HFA local-slot consumption seam that still drops
`00204.c` out at `fa_hfa11` once the bool/cast route is no longer blocking.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- shared prepared value-home or move-bundle facts only if float/HFA local-slot
  rendering lacks a target-independent ownership fact
- `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

Actions:

- implement one generic repair for float/HFA local-slot load or consumption
  through the prepared renderer
- keep the packet scoped to float/HFA local-slot ownership and do not reopen
  compare-result bool/cast or later call-family work in the same slice
- prove the repair on the nearest backend coverage plus the owned c-testsuite
  route without relying on one named helper only
- confirm `fa_hfa11` and nearby same-family routes move past the old float/HFA
  dropout without expanding into unrelated scalar families

Completion check:

- the targeted float/HFA family no longer fails for the current local-slot
  seam, and `00204.c` either reaches the next downstream leaf or exposes one
  new clearly isolated idea-60 seam

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted Step 2 packet shrinks the real idea-60 family and
preserves explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed scalar prepared/emitter path
- record in `todo.md` when advanced cases now belong in ideas 61 or 65
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-60 scalar-emitter family and
  preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 60 Is Exhausted

Goal: keep repeating the Step 1 -> 2.3 loop until the remaining failures no
longer belong to idea 60.

Actions:

- keep idea 60 active only while cases still fail for scalar expression or
  terminator consumption reasons that are not better explained by another open
  leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-60
  scalar seam, or lifecycle state is ready to hand off or close because idea
  60 no longer owns the remaining failures
