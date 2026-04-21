# Call-Bundle And Multi-Function Prepared-Module Consumption

Status: Active
Source Idea: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Activated from: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md

## Purpose

Resume the x86 prepared-module leaf after executor inspection proved that
`c_testsuite_x86_backend_src_00204_c` is currently blocked earlier in the
bounded multi-defined helper/module lane, before any idea-60 scalar-return
consumption seam can apply.

## Goal

Repair one prepared-module traversal or call-bundle consumption seam at a time
so owned cases move past the current bounded multi-defined restriction without
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
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prep/prealloc.hpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `tests/backend/backend_x86_handoff_boundary_multi_defined_rejection_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that stop because x86 cannot yet consume the prepared
  multi-function module inventory, same-module helper relationships, or
  authoritative call/result handoff through the canonical prepared-module lane
- shared prepared module/value-home/move-bundle contracts when the missing
  meaning is upstream of x86-specific rendering
- durable rehoming of cases that advance out of module traversal into later
  scalar or call-family leaves once prepared-module consumption succeeds

## Non-Goals

- reopening semantic lowering or stack/addressing ownership already settled by
  earlier ideas
- adding one more bounded x86 entry-topology matcher for a named helper shape
- scalar-return or terminator selection that still belongs in idea 60 once the
  multi-function route is genuinely consumed
- call-family ownership that still belongs in idea 65

## Working Model

- keep one prepared-module traversal or call-bundle seam per packet
- use the nearest multi-defined handoff coverage plus the nearest owned
  c-testsuite case to prove the seam without collapsing the packet into one
  testcase
- extend shared prepared contracts first when x86 lacks a generic fact it
  should consume
- route cases back out as soon as the next real blocker belongs in another
  downstream idea

## Execution Rules

- prefer one prepared-module traversal or call/result-handoff seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  bounded multi-defined traversal or prepared call-bundle consumption
- when a targeted case graduates into ideas 60 or 65, record that in
  `todo.md` and keep this runbook focused on still-owned module/call work
- reject entry-topology matcher growth that only moves one named helper family
  forward

## Step 1: Refresh Idea-61 Ownership And Confirm The Next Prepared-Module Seam

Goal: confirm the new idea-61 ownership for
`c_testsuite_x86_backend_src_00204_c` and identify the exact prepared-module
traversal or call/result-handoff seam that currently blocks x86 emission.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend handoff coverage nearest the current multi-defined
  route
- shared prepared contract and x86 prepared-module files near the current
  failure

Actions:

- rerun or inspect the narrow subset that currently reproduces the
  `00204.c` blocker
- confirm that `00204.c` is stopped by idea-61-owned multi-function
  prepared-module consumption before scalar-return selection begins
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
multi-defined restriction.

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
- record in `todo.md` when advanced cases now belong in ideas 60 or 65
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
