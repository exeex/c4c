# Load Local-Memory Semantic Family Follow-On For X86 Backend

Status: Active
Source Idea: ideas/open/66_load_local_memory_semantic_family_follow_on_for_x86_backend.md
Activated from: ideas/open/65_semantic_call_family_lowering_for_x86_backend.md

## Purpose

Turn idea 66 into an execution runbook for backend cases whose next real
blocker is `load local-memory semantic family` lowering rather than semantic
call-family ownership or later prepared-module consumption.

## Goal

Repair one load local-memory seam at a time so owned cases reach normal
prepared-x86 consumption and then graduate cleanly into the downstream idea
that matches their next failure mode.

## Core Rule

Do not claim x86 backend progress by adding testcase-shaped local-load
shortcuts while canonical local-memory ownership is still missing upstream.

## Read First

- `ideas/open/66_load_local_memory_semantic_family_follow_on_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/65_semantic_call_family_lowering_for_x86_backend.md`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that still report the semantic `lir_to_bir` lowering
  diagnostic when the missing capability is really `load local-memory semantic
  family` lowering after call lowering already succeeds
- upstream semantic/BIR work that makes local-memory load ownership canonical
  enough for prepared x86 consumption
- durable rehoming of cases that advance into ideas 59, 60, 61, or 63

## Non-Goals

- direct-call, indirect-call, or call-return failures that still belong in
  idea 65
- prepared call-bundle or prepared-module work once semantic lowering already
  succeeds
- CFG or scalar-emitter work once a case already reaches those downstream
  routes
- emitter-local or testcase-local load matcher growth for one named spelling

## Working Model

- keep one load local-memory seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  confirm the seam is real without collapsing the packet into one testcase
- treat local-memory load ownership as semantic lowering work until prepared
  x86 can consume the route normally
- when a case graduates downstream, record that route in `todo.md` and keep
  this runbook focused on the remaining load local-memory ownership

## Execution Rules

- prefer one load local-memory seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed local-memory path
- when a targeted case graduates into ideas 59, 60, 61, or 63, record that in
  `todo.md` and keep this runbook focused on still-owned load local-memory
  work
- reject testcase-shaped lowering shortcuts that only make one named local-load
  case pass

## Step 1: Audit The Active Load Local-Memory Family

Goal: confirm the current owned failures and isolate the first load
local-memory seam that keeps them from reaching prepared x86 consumption.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route coverage nearest the local-memory load path
- `src/backend/bir/lir_to_bir_memory.cpp`

Actions:

- reproduce a narrow owned subset that still reports the semantic
  `lir_to_bir` lowering diagnostic for load local-memory reasons
- group the failures by missing local-memory load capability rather than by
  testcase name
- choose one concrete first seam and identify the nearest protective backend
  coverage for it
- note any cases that have already graduated into ideas 59, 60, 61, or 63 so
  they do not get pulled back into this runbook

Completion check:

- the first implementation packet is narrowed to one load local-memory seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Refresh Remaining Idea-66 Ownership And Pick The Next Seam

Goal: keep the active packet focused on one still-owned load local-memory seam
after each accepted graduation out of idea 66.

Primary targets:

- the narrow owned subset that still reports the semantic `lir_to_bir`
  lowering diagnostic for load local-memory reasons
- `todo.md` routing notes for recently graduated cases
- shared semantic/BIR lowering files nearest the next local-memory seam

Actions:

- rerun or inspect the narrow owned subset after the latest accepted slice
- exclude cases that now belong in ideas 59, 60, 61, or 63 instead of pulling
  them back into idea 66
- identify one concrete next load local-memory seam and the nearest backend
  coverage that can prove it without relying only on a named c-testsuite case
- keep the next packet on canonical local-memory ownership rather than
  drifting into call-family cleanup or prepared-module cleanup

Completion check:

- the next executor packet is narrowed to one still-owned load local-memory
  seam with named proof targets after the latest graduations are accounted for

## Step 2.2: Repair The Selected Load Local-Memory Seam

Goal: implement the smallest durable local-memory repair that advances the
selected still-owned cases into prepared x86 consumption.

Primary targets:

- shared semantic/BIR lowering code for the chosen seam
- prepared contract structs only if the missing local-memory meaning is
  genuinely upstream

Actions:

- repair the selected load local-memory seam in the layer that owns the
  missing meaning
- keep new contract facts target-independent when the seam is not x86-specific
- avoid emitter-local fallback paths that merely bypass the diagnostic
- confirm the targeted cases now move past the old idea-66 failure family

Completion check:

- the targeted owned cases no longer fail with the load local-memory form of
  the `lir_to_bir` lowering diagnostic and instead reach the next downstream
  owned route

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-66 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that can prove it without relying only on a named c-testsuite case
- record in `todo.md` when advanced cases now belong in ideas 59, 60, 61, or
  63
- only return to Step 2.1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-66 diagnostic family and
  preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 66 Is Exhausted

Goal: keep repeating the Step 2.1 -> 2.3 loop until the remaining failures no
longer belong to idea 66.

Actions:

- keep idea 66 active only while cases still fail before prepared x86 handoff
  because local-memory load lowering is not yet canonical enough
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 2.1 for a still-owned load
  local-memory seam, or lifecycle state is ready to hand off/close because
  idea 66 no longer owns the remaining failures
