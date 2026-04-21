# Semantic Call Family Lowering For X86 Backend

Status: Active
Source Idea: ideas/open/65_semantic_call_family_lowering_for_x86_backend.md
Activated from: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Supersedes: ideas/closed/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md

## Purpose

Turn idea 65 into an execution runbook for backend cases whose next real
blocker is semantic call-family lowering rather than stack/addressing or later
prepared-module consumption.

## Goal

Repair one semantic call-family seam at a time so owned cases reach normal
prepared-x86 consumption and then graduate cleanly into the downstream idea
that matches their next failure mode.

## Core Rule

Do not claim x86 backend progress by adding call-shaped lowering shortcuts for
one named testcase while canonical semantic call ownership is still missing
upstream.

## Read First

- `ideas/open/65_semantic_call_family_lowering_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_calling.cpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that still report the semantic `lir_to_bir` lowering
  diagnostic when the missing capability is really direct-call, indirect-call,
  or call-return lowering
- upstream semantic/BIR work that makes semantic call ownership canonical
  enough for prepared x86 consumption
- durable rehoming of cases that advance into ideas 59, 60, 61, or 63

## Non-Goals

- stack/addressing failures that still belong in the closed idea-62 lane
- prepared call-bundle or prepared-module work once semantic lowering already
  succeeds
- CFG or scalar-emitter work once a case already reaches those downstream
  routes
- emitter-local matcher growth for one named call spelling

## Working Model

- keep one semantic call-family seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  confirm the seam is real without collapsing the packet into one testcase
- treat direct-call, indirect-call, and call-return ownership as semantic
  lowering work until prepared x86 can consume the route normally
- when a case graduates downstream, record that route in `todo.md` and keep
  this runbook focused on the remaining semantic call-family ownership

## Execution Rules

- prefer one semantic call-family seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed semantic call-family path
- when a targeted case graduates into ideas 59, 60, 61, or 63, record that in
  `todo.md` and keep this runbook focused on still-owned semantic call-family
  work
- reject testcase-shaped lowering shortcuts that only make one named call case
  pass

## Step 1: Audit The Active Semantic Call Family

Goal: confirm the current owned failures and isolate the first semantic
call-family seam that keeps them from reaching prepared x86 consumption.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route coverage nearest the direct-call path
- `src/backend/bir/lir_to_bir_calling.cpp`

Actions:

- reproduce a narrow owned subset that still reports the semantic
  `lir_to_bir` lowering diagnostic for call-family reasons
- group the failures by missing semantic call capability rather than by
  testcase name
- choose one concrete first seam and identify the nearest protective backend
  coverage for it
- note any cases that have already graduated into ideas 59, 60, 61, or 63 so
  they do not get pulled back into this runbook

Completion check:

- the first implementation packet is narrowed to one semantic call-family seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Refresh Remaining Idea-65 Ownership And Pick The Next Seam

Goal: keep the active packet focused on one still-owned semantic call-family
seam after each accepted graduation out of idea 65.

Primary targets:

- the narrow owned subset that still reports the semantic `lir_to_bir`
  lowering diagnostic for call-family reasons
- `todo.md` routing notes for recently graduated cases
- shared semantic/BIR lowering files nearest the next call-family seam

Actions:

- rerun or inspect the narrow owned subset after the latest accepted slice
- exclude cases that now belong in ideas 59, 60, 61, or 63 instead of pulling
  them back into idea 65
- identify one concrete next semantic call-family seam and the nearest backend
  coverage that can prove it without relying only on a named c-testsuite case
- keep the next packet on canonical semantic call ownership rather than
  drifting into emitter-local or prepared-module cleanup

Completion check:

- the next executor packet is narrowed to one still-owned semantic call-family
  seam with named proof targets after the latest graduations are accounted for

## Step 2.2: Repair The Selected Semantic Call-Family Seam

Goal: implement the smallest durable semantic call-family repair that advances
the selected still-owned cases into prepared x86 consumption.

Primary targets:

- shared semantic/BIR lowering code for the chosen seam
- prepared contract structs only if the missing semantic call meaning is
  genuinely upstream

Actions:

- repair the selected semantic call-family seam in the layer that owns the
  missing meaning
- keep new contract facts target-independent when the seam is not x86-specific
- avoid emitter-local fallback paths that merely bypass the diagnostic
- confirm the targeted cases now move past the old idea-65 failure family

Completion check:

- the targeted owned cases no longer fail with the semantic call-family form
  of the `lir_to_bir` lowering diagnostic and instead reach the next
  downstream owned route

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-65 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed semantic call-family path
- record in `todo.md` when advanced cases now belong in ideas 59, 60, 61, or
  63
- only return to Step 2.1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-65 diagnostic family and
  preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 65 Is Exhausted

Goal: keep repeating the Step 2.1 -> 2.3 loop until the remaining failures no
longer belong to idea 65.

Actions:

- keep idea 65 active only while cases still fail before prepared x86 handoff
  because semantic call lowering is not yet canonical enough
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 2.1 for a still-owned semantic
  call-family seam, or lifecycle state is ready to hand off/close because idea
  65 no longer owns the remaining failures
