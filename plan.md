# Call-Bundle And Multi-Function Prepared-Module Consumption

Status: Active
Source Idea: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Activated from: ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md

## Purpose

Switch the active backend leaf back to idea 61 now that the accepted idea-68
local-slot repair advanced `c_testsuite_x86_backend_src_00204_c` downstream
into the bounded multi-function prepared-module family again.

## Goal

Repair one prepared-module traversal or call-bundle consumption seam at a time
so owned cases move past the current bounded multi-function restriction without
adding topology-shaped x86 fast paths.

## Core Rule

Do not claim prepared-module or call-bundle progress through one more bounded
`main + helper` matcher or local ABI fallback when the missing ownership is
still a generic prepared module-traversal, same-module function-relationship,
or call/result-handoff seam.

## Read First

- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`
- `ideas/open/68_prepared_local_slot_handoff_consumption_for_x86_backend.md`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_rejection_test.cpp`
- `tests/backend/backend_x86_route_debug_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that stop because x86 cannot yet consume the prepared
  multi-function module inventory, same-module helper relationships, or
  authoritative call/result handoff through the canonical prepared-module lane
- shared prepared module, value-home, and move-bundle contracts when the
  missing meaning is upstream of x86-specific rendering
- durable rehoming of cases that advance out of module traversal into later
  scalar, local-slot, call-family, or runtime leaves once prepared-module
  consumption succeeds

## Non-Goals

- reopening semantic lowering or stack/addressing ownership already settled by
  earlier ideas
- adding one more bounded x86 entry-topology matcher for a named helper shape
- local-slot ownership that only belongs back in idea 68 if the top-level
  rejection regresses to an authoritative prepared local-slot handoff
- trace-only observability work that still belongs in idea 67

## Working Model

- keep one prepared-module traversal or call-bundle seam per packet
- use the nearest multi-defined handoff coverage plus the nearest owned
  c-testsuite case to prove the seam without collapsing the packet into one
  testcase
- extend shared prepared contracts first when x86 lacks a generic fact it
  should consume
- route cases back out as soon as the next real blocker belongs in another
  downstream leaf

## Execution Rules

- prefer one prepared-module traversal or call/result-handoff seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  bounded multi-function traversal or prepared call-bundle consumption
- when a targeted case graduates into ideas 60, 65, or 68, record that in
  `todo.md` and keep this runbook focused on still-owned module/call work
- reject entry-topology matcher growth that only moves one named helper family
  forward

## Step 1: Refresh Idea-61 Ownership And Confirm The Next Prepared-Module Seam

Goal: confirm that `c_testsuite_x86_backend_src_00204_c` is back under idea 61
and identify the exact prepared-module traversal or call/result-handoff seam
that currently blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend handoff coverage nearest the current multi-function
  route
- shared prepared contract and x86 prepared-module files near the current
  failure

Actions:

- rerun or inspect the narrow subset that currently reproduces the `00204.c`
  blocker
- confirm that `00204.c` is stopped by an idea-61-owned bounded
  multi-function prepared-module restriction rather than a remaining idea-68
  local-slot seam
- identify the exact prepared module-traversal, same-module helper, or
  call/result-handoff fact that x86 fails to consume for the current route
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-61 seam with
  named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Prepared-Module Or Call-Bundle Seam

Goal: implement the smallest durable prepared-module or call-bundle repair
that advances the selected idea-61 case beyond the current bounded
multi-function restriction.

Primary targets:

- shared prepared module, value-home, or move-bundle contracts for the chosen
  seam
- x86 prepared-module helpers that should consume those contracts generically

Actions:

- repair the selected seam at the layer that owns the missing meaning
- prefer contract-first fixes when x86 is missing a shared prepared fact
- keep the repair generic across nearby same-family multi-function routes
- confirm the targeted cases now move past the old idea-61 failure family or
  graduate cleanly into a later leaf

Completion check:

- the targeted owned cases no longer fail for the selected idea-61 seam and
  instead reach the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-61 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed prepared-module or call-bundle path
- record in `todo.md` when advanced cases now belong in ideas 60, 65, or 68
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-61 family and preserve clear
  routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 61 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 61.

Actions:

- keep idea 61 active only while cases still fail for prepared-module
  traversal or call-bundle consumption reasons that are not better explained
  by another open leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-61
  seam, or lifecycle state is ready to hand off or close because idea 61 no
  longer owns the remaining failures
