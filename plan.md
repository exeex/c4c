# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Active
Source Idea: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Activated from: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md

## Purpose

Turn idea 62 into an execution runbook for backend cases whose next real
blocker is stack/addressing semantics rather than the broader idea-58
semantic-lowering bucket.

## Goal

Repair one stack/addressing seam at a time so owned cases reach normal
prepared-x86 consumption and then graduate cleanly into the downstream idea
that matches their next failure mode.

## Core Rule

Do not claim x86 backend progress by adding emitter-local shortcuts for one
local/member/array testcase shape while canonical stack/addressing meaning is
still missing upstream.

## Read First

- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/prealloc/legalize.cpp`
- `tests/backend/codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir.c`
- `tests/backend/codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir.c`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that still report the semantic `lir_to_bir` lowering
  diagnostic when the missing capability is really local/member/array access
  or other stack/addressing semantics
- upstream semantic/BIR or prepared-addressing work that makes stack-backed
  meaning canonical enough for prepared x86 consumption
- durable rehoming of cases that advance into ideas 59, 60, 61, or 63

## Non-Goals

- generic semantic-lowering gaps whose durable blocker is not stack/addressing
- prepared CFG, prepared-module, or scalar-emitter work once a case already
  reaches those downstream routes
- emitter-local matcher growth for one dynamic local/member/array testcase
- runtime-correctness debugging after codegen already succeeds

## Working Model

- keep one stack/addressing seam per packet
- use representative backend route tests plus the nearest c-testsuite case to
  confirm the seam is real without collapsing the packet into one testcase
- treat byval local/member access, dynamic array indexing, and stack-slot
  meaning as semantic-lowering ownership until prepared x86 can consume the
  route normally
- when a case graduates downstream, record that route in `todo.md` and keep
  this runbook focused on the remaining stack/addressing family

## Execution Rules

- prefer one stack/addressing seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed stack/addressing path
- when a targeted case graduates into ideas 59, 60, 61, or 63, record that in
  `todo.md` and keep this runbook focused on still-owned stack/addressing work
- reject testcase-shaped lowering shortcuts that only make one named case pass

## Step 1: Audit The Active Stack/Addressing Family

Goal: confirm the current owned failures and isolate the first
stack/addressing seam that keeps them from reaching prepared x86 consumption.

Primary targets:

- representative failing backend route tests from idea 62
- `c_testsuite_x86_backend_src_00204_c` and any other current cases routed
  here from idea 58
- `src/backend/prealloc/legalize.cpp`

Actions:

- reproduce a narrow owned subset that still reports the semantic
  `lir_to_bir` lowering diagnostic for stack/addressing reasons
- group the failures by missing stack/addressing capability rather than by
  testcase name
- choose one concrete first seam and identify the nearest protective backend
  coverage for it
- note any cases that have already graduated into ideas 59, 60, 61, or 63 so
  they do not get pulled back into this runbook

Completion check:

- the first implementation packet is narrowed to one stack/addressing seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Refresh Remaining Idea-62 Ownership And Pick The Next Seam

Goal: keep the active packet focused on one still-owned stack/addressing seam
after each accepted graduation out of idea 62.

Primary targets:

- the narrow owned subset that still reports the semantic `lir_to_bir`
  lowering diagnostic for stack/addressing reasons
- `todo.md` routing notes for recently graduated cases
- shared semantic/BIR lowering files nearest the next stack/addressing seam

Actions:

- rerun or inspect the narrow owned subset after the latest accepted slice
- exclude cases that now belong in ideas 59, 60, 61, or 63 instead of pulling
  them back into idea 62
- identify one concrete next stack/addressing seam and the nearest backend
  coverage that can prove it without relying only on a named c-testsuite case
- keep the next packet on canonical stack/addressing meaning rather than
  drifting into emitter-local or generic variadic cleanup

Completion check:

- the next executor packet is narrowed to one still-owned stack/addressing
  seam with named proof targets after the latest graduations are accounted for

## Step 2.2: Repair The Selected Stack/Addressing Seam

Goal: implement the smallest durable stack/addressing repair that advances the
selected still-owned cases into prepared x86 consumption.

Primary targets:

- shared semantic/BIR lowering code for the chosen seam
- prepared contract structs only if the missing stack/addressing meaning is
  genuinely upstream

Actions:

- repair the selected stack/addressing seam in the layer that owns the missing
  meaning
- keep new contract facts target-independent when the seam is not x86-specific
- avoid emitter-local fallback paths that merely bypass the diagnostic
- confirm the targeted cases now move past the old idea-62 failure family

Completion check:

- the targeted owned cases no longer fail with the stack/addressing form of
  the semantic `lir_to_bir` lowering diagnostic and instead reach the next
  downstream owned route

## Step 2.3: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-62 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed stack/addressing path
- record in `todo.md` when advanced cases now belong in ideas 59, 60, 61, or
  63
- only return to Step 2.1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-62 diagnostic family and
  preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 62 Is Exhausted

Goal: keep repeating the Step 2.1 -> 2.3 loop until the remaining failures no
longer belong to idea 62.

Actions:

- keep idea 62 active only while cases still fail before prepared x86 handoff
  because stack/addressing semantics are not yet canonical enough
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 2.1 for a still-owned
  stack/addressing seam, or lifecycle state is ready to hand off/close because
  idea 62 no longer owns the remaining failures
