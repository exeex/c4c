# Stack Addressing And Dynamic Local Access For X86 Backend

Status: Active
Source Idea: ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md
Supersedes: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md

## Purpose

Turn idea 62 into an execution runbook that makes stack layout and addressing
semantics canonical enough for dynamic local/member/array access cases to reach
prepared-x86 consumption, without drifting back into generic idea-58 lowering
work or downstream x86 matcher growth.

## Goal

Repair the owned stack/addressing family so confirmed cases such as `00040` and
the dynamic member-array boundary fixtures stop failing in `gep` or related
local-memory semantic lanes before prepared-x86 handoff.

## Core Rule

Do not patch x86-local emitters or add testcase-shaped address matchers.
Repair shared stack-slot, pointer-value, and address-form lowering so the
prepared/BIR route carries canonical memory meaning across nearby owned cases.

## Read First

- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- `src/backend/bir/lir_to_bir_memory_local_slots.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/c/external/c-testsuite/src/00040.c`

## Scope

- canonical stack-slot meaning for dynamic locals, member access, and array
  access that still fail before prepared-x86 consumption
- shared semantic lowering for `gep` and related local-memory routes when the
  missing capability is address-form or stack-layout meaning
- proof against owned backend and c-testsuite cases, not only synthetic notes
  coverage

## Non-Goals

- generic semantic-lowering gaps unrelated to stack/addressing ownership
- prepared short-circuit or guard-chain consumption once lowering reaches x86
- call-bundle or multi-function prepared-module support
- runtime correctness bugs after codegen succeeds

## Working Model

- the current failures in this bucket are not best described as "x86 still
  needs handoff" but as "shared lowering still does not publish canonical local
  address meaning"
- the right repair surface is shared `lir_to_bir` memory lowering and its
  address/slot helpers, not x86-local fallback recognition
- accepted slices should move multiple nearby addressing cases together or make
  the ownership boundary clearer when a remaining failure belongs elsewhere

## Execution Rules

- prefer one bounded addressing seam per packet over broad memory-lowering
  rewrites
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow backend proof` for accepted code slices, then broaden
  when one packet moves multiple address/slot surfaces together
- reject slices whose main effect is expectation rewrites or one-case address
  shortcuts instead of canonical stack/address semantics
- when a repaired case graduates into x86-prepared consumption failures, route
  it to the correct downstream idea instead of keeping it in this plan

## Step 1: Audit Confirmed Stack/Addressing Blockers

Goal: map the currently confirmed owned failures to concrete shared addressing
or stack-slot seams before changing behavior.

Primary targets:

- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- `src/backend/bir/lir_to_bir_memory_local_slots.cpp`
- `tests/backend/backend_lir_to_bir_notes_test.cpp`
- `tests/c/external/c-testsuite/src/00040.c`

Actions:

- trace why `00040` and the dynamic indexed/member-array boundary fixtures fail
  in the `gep local-memory semantic family`
- identify which stack-slot, pointer-value, or address-form facts are missing
  from shared lowering
- keep the idea-62 versus downstream-x86 boundary explicit when a case starts
  reaching prepared emission

Completion check:

- the active owned failures are mapped to one or more concrete shared
  addressing seams, and the first implementation packet can stay inside idea
  62 scope

## Step 2: Canonicalize Dynamic Local And Address-Form Lowering

Goal: make shared lowering publish the stack/addressing facts required for
owned cases to survive `gep` and related local-memory lowering.

Primary targets:

- `src/backend/bir/lir_to_bir_memory.cpp`
- `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- `src/backend/bir/lir_to_bir_memory_local_slots.cpp`

Actions:

- implement the missing stack-slot or address-form rules identified by Step 1
- keep the repair general across local/member/array access instead of matching
  one testcase spelling
- do not substitute x86-local render logic for missing shared memory meaning

Completion check:

- owned cases no longer fail solely because `lir_to_bir` cannot lower the
  required dynamic local/member/array addressing route

## Step 3: Stabilize Prepared-Handoff Boundaries

Goal: keep idea 62 narrowly honest once repaired cases start reaching later
backend stages.

Primary targets:

- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md`
- `ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md`

Actions:

- confirm whether newly exposed failures after Step 2 still belong to
  stack/addressing ownership or to downstream x86-prepared leaves
- preserve only durable routing notes in lifecycle state when a case graduates
  out of idea 62
- avoid silently expanding this plan into CFG, scalar-emission, or
  multi-function ownership

Completion check:

- remaining failures are clearly separated between stack/addressing work and
  downstream x86-prepared work, preventing scope drift

## Step 4: Validate Progress Against Owned Failures

Goal: prove that accepted slices advance real stack/addressing capability
instead of only moving boundary-test notes.

Actions:

- require a fresh build for every accepted code slice
- prove repaired seams with backend-focused tests that exercise owned
  stack/addressing routes and at least one real c-testsuite case when the
  touched seam claims c-testsuite progress
- broaden validation when one packet changes multiple addressing helpers or
  both local and global addressing surfaces together
- reject proof that only demonstrates fixture coverage while owned c-testsuite
  failures remain unexamined

Completion check:

- accepted slices have fresh proof and show credible progress on owned
  stack/addressing failures rather than only on synthetic diagnostics
