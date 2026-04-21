# Semantic LIR-To-BIR Gap Closure For X86 Backend

Status: Active
Source Idea: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Activated from: ideas/open/66_load_local_memory_semantic_family_follow_on_for_x86_backend.md

## Purpose

Turn idea 58 back into the active execution runbook for backend cases whose
next real blocker is still upstream semantic `lir_to_bir` lowering rather than
a narrower semantic leaf or later prepared-x86 consumption.

## Goal

Repair one semantic `lir_to_bir` seam at a time so owned cases either reach
normal prepared-x86 consumption or graduate cleanly into a narrower open leaf
that matches their next failure mode.

## Core Rule

Do not claim x86 backend progress by adding testcase-shaped semantic lowering
shortcuts or x86-only bypasses while canonical upstream ownership is still
missing.

## Read First

- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `ideas/open/57_x86_backend_c_testsuite_capability_families.md`
- `ideas/open/62_stack_and_member_addressing_semantic_family_follow_on_for_x86_backend.md`
- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_to_bir_module.cpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/c/external/c-testsuite/src/00204.c`

## Scope

- backend failures that still report the semantic `lir_to_bir` lowering
  diagnostic and are not better explained by narrower semantic leaves such as
  ideas 62, 65, or 66
- upstream semantic/BIR work that keeps owned cases moving toward prepared-x86
  consumption instead of aborting in semantic lowering
- durable rehoming of cases that advance into narrower semantic leaves or
  downstream prepared-x86 ideas

## Non-Goals

- call-family ownership that still belongs in idea 65
- load-local-memory ownership that still belongs in idea 66
- stack/member-addressing ownership that still belongs in idea 62
- prepared call-bundle, prepared-module, CFG, or scalar-emitter work once
  semantic lowering already succeeds
- testcase-local matcher growth for one named spelling

## Working Model

- keep one semantic seam per packet
- use the nearest backend route coverage plus the nearest c-testsuite case to
  confirm the seam is real without collapsing the packet into one testcase
- route cases out to narrower semantic leaves as soon as the next real blocker
  is more specific than idea 58
- when a case reaches prepared-x86 or a narrower semantic leaf, record that
  route in `todo.md` and keep this runbook focused on remaining broad semantic
  ownership

## Execution Rules

- prefer one semantic seam per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow proof` for every accepted code slice
- keep proof on owned failures plus the nearest backend coverage that protects
  the changed semantic path
- when a targeted case graduates into ideas 59, 60, 61, 62, 65, or 66, record
  that in `todo.md` and keep this runbook focused on still-owned broad
  semantic work
- reject testcase-shaped lowering shortcuts that only make one named failure
  advance

## Step 1: Refresh Remaining Idea-58 Ownership And Pick The Next Semantic Seam

Goal: confirm the current broad idea-58 ownership after the idea-66 handoff
and isolate the next still-owned semantic seam.

Primary targets:

- `c_testsuite_x86_backend_src_00204_c`
- representative backend route coverage nearest the next semantic seam
- shared semantic lowering files near the latest function failure

Actions:

- rerun or inspect the narrow owned subset after commit `736d15d6`
- exclude cases that now belong in ideas 59, 60, 61, 62, 65, or 66 instead of
  pulling them back into idea 58
- confirm whether `00204.c` is the next active case and identify the concrete
  semantic seam at function `myprintf`
- choose the nearest protective backend coverage that can prove that seam
  without relying only on the named c-testsuite case

Completion check:

- the next executor packet is narrowed to one still-owned idea-58 semantic seam
  with named proof targets and a clear ownership boundary

## Step 2.1: Repair The Selected Semantic Seam

Goal: implement the smallest durable semantic/BIR repair that advances the
selected idea-58 case toward prepared-x86 consumption or into a narrower leaf.

Primary targets:

- shared semantic/BIR lowering code for the chosen seam
- target-independent contracts when the missing meaning is upstream rather than
  x86-specific

Actions:

- repair the selected semantic seam in the layer that owns the missing meaning
- avoid x86-only fallback paths that merely bypass the diagnostic
- keep the repair general across nearby cases that share the same semantic
  blocker
- confirm the targeted cases now move past the old idea-58 failure family or
  graduate cleanly into a narrower leaf

Completion check:

- the targeted owned cases no longer fail for the selected idea-58 seam and
  instead reach the next downstream route

## Step 2.2: Prove Family Shrinkage And Record Rehoming

Goal: show the accepted slice shrinks the real idea-58 family and preserves
explicit routing for any graduated cases.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam on the targeted owned cases plus the nearest backend
  coverage that protects the changed semantic path
- record in `todo.md` when advanced cases now belong in ideas 59, 60, 61, 62,
  65, or 66
- only return to Step 1 after the current seam is proven and any graduated
  routing is explicit

Completion check:

- accepted slices show real shrinkage of the idea-58 semantic-lowering family
  and preserve clear routing for any graduated downstream cases

## Step 3: Continue The Loop Until Idea 58 Is Exhausted

Goal: keep repeating the Step 1 -> 2.2 loop until the remaining failures no
longer belong to idea 58.

Actions:

- keep idea 58 active only while cases still fail before prepared-x86 handoff
  for broad semantic `lir_to_bir` reasons that are not better explained by a
  narrower open leaf
- use `todo.md` to preserve which cases graduated downstream after each packet
- call lifecycle review again when the next step becomes oversized or when the
  remaining family is exhausted

Completion check:

- the next active packet is queued under Step 1 for a still-owned idea-58
  semantic seam, or lifecycle state is ready to hand off/close because idea 58
  no longer owns the remaining failures
