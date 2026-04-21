# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Active
Source Idea: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Activated from: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md

## Purpose

Resume the narrower stack/member/addressing runbook now that commit
`5a81abdb` has cleared the upstream aggregate-phi blocker and moved
`c_testsuite_x86_backend_src_00204_c` back into a later `gep local-memory`
semantic seam in function `myprintf`.

## Goal

Repair one stack/addressing semantic seam at a time so owned cases either
reach normal prepared-x86 consumption or graduate cleanly into a later
prepared-x86 leaf.

## Core Rule

Do not claim stack/addressing progress through testcase-shaped GEP shortcuts or
x86-only bypasses when the missing ownership is still upstream semantic
addressing canonicalization.

## Read First

- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir_memory_local_slots.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that still stop in semantic `lir_to_bir` with
  `gep local-memory semantic family` or another clearer stack/member/array
  addressing seam
- upstream local-memory and addressing work that makes canonical stack or
  aggregate access reachable by prepared-x86 consumers
- durable rehoming of cases that advance downstream into ideas 58, 59, 60, 61,
  65, or 66

## Non-Goals

- broad scalar-control-flow work that still belongs in idea 58
- direct-call, indirect-call, or call-return ownership that still belongs in
  idea 65
- prepared CFG, prepared-module, or scalar-emitter work once semantic
  addressing already succeeds
- testcase-local matcher growth for one named GEP spelling

## Working Model

- keep one addressing seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  confirm the seam is real without collapsing the packet into one testcase
- route cases back out as soon as the next real blocker is upstream scalar
  control-flow or downstream prepared-x86 ownership instead of addressing

## Execution Rules

- prefer one stack/addressing seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed addressing path
- when a targeted case graduates into ideas 58, 59, 60, 61, 65, or 66, record
  that in `todo.md` and keep this runbook focused on still-owned
  stack/addressing work
- reject testcase-shaped GEP/addressing shortcuts that only make one named
  failure advance

## Step 1: Refresh Reopened Idea-62 Ownership And Confirm The Next Addressing Seam

Goal: confirm the reopened idea-62 ownership after commit `5a81abdb` and
identify the concrete stack/member/addressing seam that now owns
`c_testsuite_x86_backend_src_00204_c`.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route coverage nearest the current GEP/addressing seam
- shared semantic local-memory files near function `myprintf`

Actions:

- rerun or inspect the narrow reopened subset after commit `5a81abdb`
- confirm that `00204.c` no longer fails in upstream scalar-control-flow
  ownership and now stops later in an idea-62 seam
- identify the exact `myprintf` addressing operation that triggers the
  `gep local-memory semantic family` failure
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-62 addressing
  seam with named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Addressing Seam

Goal: implement the smallest durable stack/addressing repair that advances the
selected idea-62 case toward prepared-x86 consumption or into a later leaf.

Primary targets:

- shared semantic local-memory or addressing code for the chosen seam
- target-independent contracts when the missing meaning is upstream rather than
  x86-specific

Actions:

- repair the selected stack/member/addressing seam in the layer that owns the
  missing meaning
- avoid x86-only fallback paths that merely bypass the diagnostic
- keep the repair general across nearby cases that share the same addressing
  blocker
- confirm the targeted cases now move past the old idea-62 failure family or
  graduate cleanly into a downstream leaf

Completion check:

- the targeted owned cases no longer fail for the selected idea-62 seam and
  instead reach the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-62 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed addressing path
- record in `todo.md` when advanced cases now belong in ideas 58, 59, 60, 61,
  65, or 66
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-62 stack/addressing family
  and preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 62 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 62.

Actions:

- keep idea 62 active only while cases still fail before prepared-x86 handoff
  for stack/member/addressing reasons that are not better explained by another
  open leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-62
  addressing seam, or lifecycle state is ready to hand off/close because idea
  62 no longer owns the remaining failures
