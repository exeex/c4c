# Prepared Local-Slot Handoff Consumption For X86 Backend

Status: Active
Source Idea: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md
Activated from: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md

## Purpose

Switch the active backend leaf back to idea 68 now that full
`c_testsuite_x86_backend_src_00204_c` no longer stops in the generic
prepared-module restriction and instead stabilizes on the authoritative
prepared local-slot handoff family again.

## Goal

Repair one authoritative prepared local-slot or continuation-consumption seam
at a time so owned cases move past the current local-slot handoff rejection
without reopening bounded module-shape or testcase-specific x86 fast paths.

## Core Rule

Do not claim local-slot progress by re-deriving helper meaning from raw block
topology once authoritative prepared control-flow or local-slot ownership is
already published.

## Read First

- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_x86_route_debug_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that stop with the authoritative prepared local-slot handoff
  restriction owned by idea 68, including the rehomed full-case `00204.c`
- shared prepared local-slot, continuation, control-flow, or value-home
  contracts when the missing meaning is upstream of x86 local-slot rendering
- durable rehoming of cases that advance out of local-slot handoff and into a
  later scalar-helper, call/helper-family, or runtime leaf

## Non-Goals

- reopening idea-61 ownership unless the top-level failure regresses back to a
  prepared-module or authoritative prepared call-bundle restriction
- recovering helper meaning from raw CFG or lane topology once authoritative
  prepared ownership already exists
- adding one more helper-shaped matcher for `00204.c` instead of consuming the
  prepared contract generally
- trace-only observability work that still belongs in idea 67

## Working Model

- keep one authoritative local-slot or continuation seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  prove the seam without collapsing the packet into one testcase
- extend shared prepared contracts first when x86 lacks a target-independent
  local-slot or control-flow fact it should already consume
- route cases back out as soon as the next real blocker belongs in another leaf

## Execution Rules

- prefer one local-slot or continuation-consumption seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed local-slot handoff path
- record in `todo.md` when advanced cases now belong in another downstream leaf
- reject helper-topology or testcase-shaped x86 growth that only moves one
  owned route forward

## Step 1: Refresh Idea-68 Ownership And Confirm The Next Local-Slot Seam

Goal: confirm that `c_testsuite_x86_backend_src_00204_c` now belongs to idea 68
again and identify the exact authoritative prepared local-slot or continuation
seam that currently blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route or boundary coverage nearest the local-slot
  restriction
- shared prepared local-slot and control-flow contract surfaces near the
  current failure

Actions:

- rerun or inspect the narrow subset that shows the full `00204.c` route now
  fails only in the idea-68 local-slot handoff family
- confirm that the observed blocker is not better explained by idea 59, 60,
  61, or a runtime-correctness leaf
- identify whether the missing meaning is authoritative helper instruction
  selection, continuation consumption, join transfer handling, or prepared
  local-slot value-home usage
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-68 seam with
  named proof targets and a clear ownership boundary

## Step 2.1: Preserve The Rehomed Boundary And Reject Helper-Shape Overfit

Goal: keep the rehomed `00204.c` baseline anchored to the current idea-68
restriction while rejecting probes that only add another helper-shaped x86
matcher or regress the focused `myprintf` guardrail.

Primary targets:

- `tests/backend/backend_x86_route_debug_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- treat the focused `myprintf` rejection as a guardrail for any remaining
  idea-68 repair on this route
- reject helper-shape widening that only accepts one bounded continuation or
  helper topology without consuming a generic prepared local-slot plan
- reject moving the route backward into idea 61 through module-shape rewrites
- keep the current packet record explicit that `00204.c` now belongs to idea 68
  because the top-level failure is the authoritative prepared local-slot
  restriction

Completion check:

- the runbook and `todo.md` clearly preserve the rehomed baseline and the
  route-drift rejection that blocks another helper-shaped shortcut

## Step 2.2: Repair The Current Local-Slot Or Continuation Seam

Goal: implement the smallest durable repair that lets the owned route clear the
current idea-68 local-slot restriction without regressing the focused
`myprintf` route or reopening module-shape ownership.

Primary targets:

- shared prepared local-slot, continuation, control-flow, or value-home
  contracts for the chosen seam
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prep/prealloc.hpp` when the current local-slot or control-flow
  contract is not expressive enough for generic consumption

Actions:

- repair the selected seam at the layer that owns the missing meaning
- prefer contract-first fixes when x86 is missing a target-independent prepared
  local-slot or continuation fact
- make x86 consume normalized prepared helper plans instead of adding one more
  testcase-shaped helper matcher
- keep the repair generic across nearby same-family local-slot routes
- confirm the targeted cases either clear the old idea-68 failure family or
  expose a stable downstream non-idea-68 issue

Completion check:

- the targeted owned case no longer fails for the selected idea-68 seam and
  either emits successfully or graduates cleanly into a downstream non-idea-68
  leaf

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-68 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed local-slot handoff path
- record in `todo.md` when advanced cases now belong in another leaf
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
  local-slot reasons that are not better explained by another open leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-68
  seam, or lifecycle state is ready to hand off or close because idea 68 no
  longer owns the remaining failures
