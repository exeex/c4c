# Scalar Expression And Terminator Selection For X86 Backend

Status: Active
Source Idea: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Activated from: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md

## Purpose

Resume the x86 scalar/prepared-emitter leaf now that idea 67 is parked and the
current durable blocker remains the downstream scalar-emitter restriction for
`c_testsuite_x86_backend_src_00204_c`.

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
- `ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md`
- `src/backend/targets/x86_64/emitter.cpp`
- `src/backend/targets/x86_64/emitter_expr.cpp`
- `src/backend/targets/x86_64/emitter_stmt.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_codegen_route_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that stop with the x86 scalar-emitter diagnostics owned by
  idea 60, including the current minimal-return / guard-family restriction that
  `00204.c` now reaches after the idea-67 observability work
- shared prepared value-home, move-bundle, branch-condition, and control-flow
  contracts when the missing meaning is upstream of x86-specific rendering
- durable rehoming of cases that advance out of scalar emission into later
  prepared-module, call-family, or runtime leaves once scalar selection
  succeeds

## Non-Goals

- reopening idea-67 observability work unless a fresh supported-CLI scan shows
  the route has regressed back to an opaque rejection
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
`c_testsuite_x86_backend_src_00204_c` after idea 67 was parked and identify the
exact prepared scalar return or terminator seam that now blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route coverage nearest the current scalar-emitter seam
- shared prepared contract and x86 emitter files near the current failure

Actions:

- rerun or inspect the narrow subset that now leaves idea 67 parked and keeps
  `00204.c` at the downstream scalar restriction
- confirm that `00204.c` does not currently need more observability work and
  still stops in an idea-60-owned scalar-emitter restriction
- identify the exact prepared return, value-home, move-bundle,
  branch-condition, or terminator fact that x86 fails to consume for the
  current route
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-60 scalar seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Scalar Prepared/Emitter Seam

Goal: implement the smallest durable scalar-emitter repair that advances the
selected idea-60 case beyond the current minimal scalar restriction.

Primary targets:

- shared prepared value-home, move-bundle, branch-condition, or control-flow
  contracts for the chosen seam
- x86 scalar emission helpers that should consume those contracts generically

Actions:

- repair the selected scalar seam at the layer that owns the missing meaning
- prefer contract-first fixes when x86 is missing a shared prepared fact
- keep the repair generic across nearby same-family scalar routes
- confirm the targeted cases now move past the old idea-60 failure family or
  graduate cleanly into a later leaf

Completion check:

- the targeted owned cases no longer fail for the selected idea-60 scalar seam
  and instead reach the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-60 family and preserves
explicit routing for any graduated cases.

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

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
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
