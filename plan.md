# Prepared Local-Slot Handoff Consumption For X86 Backend

Status: Active
Source Idea: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md
Activated from: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md

## Purpose

Switch the active backend leaf from idea 60 to the new local-slot handoff leaf
now that the full `c_testsuite_x86_backend_src_00204_c` route no longer stops
at the scalar restriction and instead fails later in the authoritative prepared
local-slot handoff path.

## Goal

Repair one authoritative prepared local-slot handoff seam at a time so owned
cases move past the current local-slot rejection without adding testcase-shaped
helper branches.

## Core Rule

Do not claim local-slot handoff progress through one named helper fast path
when the missing ownership is still a generic prepared local-slot,
continuation, value-home, or control-flow consumption seam.

## Read First

- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`
- `tests/backend/backend_x86_route_debug_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that stop with the authoritative prepared local-slot
  handoff diagnostic owned by idea 68, including the current full-case
  `00204.c` route
- shared prepared local-slot, continuation, value-home, and control-flow
  contracts when the missing meaning is upstream of x86 local-slot rendering
- durable rehoming of cases that advance out of local-slot handoff and into a
  later scalar-helper, call/helper-family, or runtime leaf

## Non-Goals

- reopening idea-60 scalar ownership unless the full-case route regresses back
  to that top-level blocker
- adding x86-only helper branches for one named `00204.c` subfunction or one
  exact local-slot topology
- call-bundle or multi-function prepared-module work that still belongs in
  idea 61
- trace-only observability work that still belongs in idea 67

## Working Model

- keep one prepared local-slot seam per packet
- use the nearest backend route or boundary coverage plus the nearest c-testsuite
  case to prove the seam without collapsing the packet into one testcase
- extend shared prepared contracts first when x86 lacks a generic fact it
  should consume for a local-slot helper route
- route cases back out as soon as the next real blocker belongs in another
  downstream idea

## Execution Rules

- prefer one local-slot handoff seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed prepared local-slot/control-flow consumption path
- record in `todo.md` when a targeted case graduates back into idea 60, idea
  61, or another downstream leaf
- reject helper-side named-case growth that only moves one local-slot spelling
  forward

## Step 1: Refresh Idea-68 Ownership And Confirm The Next Local-Slot Seam

Goal: confirm the current idea-68 ownership for
`c_testsuite_x86_backend_src_00204_c` and identify the exact prepared local-slot,
continuation, or control-flow seam that now blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route or boundary coverage nearest the current
  local-slot handoff seam
- shared prepared contract and x86 local-slot files near the current failure

Actions:

- rerun or inspect the narrow subset that shows the full `00204.c` route now
  fails in the authoritative prepared local-slot family
- confirm that the observed blocker is not better explained by idea 59, 60, or
  61
- identify the exact prepared local-slot instruction, continuation, value-home,
  or control-flow fact that x86 fails to consume for the current route
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-68 local-slot
  seam with named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Prepared Local-Slot Handoff Seam

Goal: implement the smallest durable local-slot handoff repair that advances
the selected idea-68 case beyond the current authoritative local-slot
rejection.

Primary targets:

- shared prepared local-slot, continuation, value-home, or control-flow
  contracts for the chosen seam
- x86 local-slot helpers that should consume those contracts generically

Actions:

- repair the selected seam at the layer that owns the missing meaning
- prefer contract-first fixes when x86 is missing a shared prepared fact
- keep the repair generic across nearby same-family local-slot routes
- confirm the targeted cases now move past the old idea-68 failure family or
  graduate cleanly into a later leaf

Completion check:

- the targeted owned cases no longer fail for the selected idea-68 local-slot
  seam and instead reach the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-68 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed local-slot/control-flow path
- record in `todo.md` when advanced cases now belong in idea 60, idea 61, or a
  later downstream leaf
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-68 local-slot handoff family
  and preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 68 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 68.

Actions:

- keep idea 68 active only while cases still fail for authoritative prepared
  local-slot handoff reasons that are not better explained by another open leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-68
  local-slot seam, or lifecycle state is ready to hand off or close because
  idea 68 no longer owns the remaining failures
