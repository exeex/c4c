# Semantic LIR-To-BIR Gap Closure For X86 Backend

Status: Active
Source Idea: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md

## Purpose

Turn idea 58 into an execution runbook that closes the upstream semantic
`lir_to_bir` gaps still blocking x86 prepared-module handoff, without drifting
into x86-local matcher growth or stack/addressing work that belongs in idea 62.

## Goal

Repair the owned semantic-lowering family so those backend and c-testsuite
cases reach canonical prepared-x86 consumption instead of failing with the
current pre-handoff semantic-lowering diagnostic.

## Core Rule

Do not make named-testcase shortcuts or x86-local escape hatches. Repair the
shared semantic lowering and prepared-BIR shaping seams that generalize across
the owned failure family, and rehome durable stack/addressing gaps to idea 62
instead of leaving them mixed into this bucket.

## Read First

- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `src/backend/backend.cpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir_cfg.cpp`
- `src/backend/bir/lir_to_bir_scalar.cpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `tests/backend/backend_x86_handoff_boundary_lir_test.cpp`

## Scope

- close upstream semantic `lir_to_bir` lowering gaps that currently abort the
  x86 backend before canonical prepared-module handoff exists
- keep ownership on shared lowering and prepared-BIR shaping, not on x86-local
  rendering shortcuts
- keep the dynamic local/member/array addressing boundary explicit so durable
  stack/addressing semantics can move to idea 62 when confirmed
- prove progress against the owned backend failure family, not only synthetic
  boundary checks

## Non-Goals

- prepared short-circuit or guard-chain consumption after lowering already
  reaches x86
- call-bundle or multi-function prepared-module support
- runtime correctness bugs after codegen succeeds
- widening idea 58 into stack/addressing ownership that is more precisely
  described by idea 62

## Working Model

- the current x86 route in `src/backend/backend.cpp` should stop failing with
  the semantic `lir_to_bir` handoff diagnostic once the required prepared-BIR
  facts are produced
- generic semantic lowering belongs in shared `src/backend/bir/lir_to_bir*.cpp`
  seams, not in x86-local dispatch code
- stack/addressing failures are adjacent but separate: if a missing capability
  is fundamentally dynamic local/member/array addressing, capture that as idea
  62 ownership instead of treating it as generic idea 58 progress
- accepted slices should move nearby owned cases together rather than proving
  one named failing testcase in isolation

## Execution Rules

- prefer one bounded semantic seam per packet over broad backend rewrites
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow backend proof` for accepted code slices, then broaden
  when shared lowering surfaces move enough that the narrow bucket stops being
  credible
- reject slices whose main effect is expectation rewrites or testcase-shaped
  recognition instead of real semantic lowering
- when audit evidence shows a durable stack/addressing-only blocker, preserve
  that note in lifecycle state and route it to idea 62 instead of hiding it in
  idea 58 implementation work

## Step 1: Audit Owned Semantic-Lowering Gaps

Goal: map the currently owned failure family to concrete semantic-lowering
producer seams before changing behavior.

Primary targets:

- `src/backend/backend.cpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir_cfg.cpp`
- `src/backend/bir/lir_to_bir_scalar.cpp`
- `src/backend/bir/lir_to_bir_memory.cpp`
- `tests/backend/backend_x86_handoff_boundary_lir_test.cpp`

Actions:

- trace where the x86 route rejects modules because semantic `lir_to_bir`
  output is still missing required prepared-BIR facts
- tie representative owned failures to concrete shared lowering seams rather
  than to x86-local render code
- separate generic idea-58 gaps from stack/addressing-owned idea-62 gaps when
  the missing capability is specifically dynamic local/member/array semantics

Completion check:

- the active owned cluster is mapped to one or more shared lowering seams, and
  the idea-58 versus idea-62 boundary is explicit enough to guide the first
  implementation packet

## Step 2: Repair Generic Semantic Handoff Production

Goal: make shared semantic lowering produce the prepared-BIR facts required for
owned cases to reach canonical x86 handoff.

Primary targets:

- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir_cfg.cpp`
- `src/backend/bir/lir_to_bir_scalar.cpp`
- `src/backend/bir/lir_to_bir_memory.cpp`

Actions:

- implement the missing shared lowering or shaping rules in the producer seams
  identified by Step 1
- keep the route semantic and general across nearby owned failures instead of
  matching one testcase shape
- do not patch `src/backend/backend.cpp` or x86-local code as a substitute for
  producing the missing prepared-BIR facts upstream

Completion check:

- owned cases stop failing solely because semantic `lir_to_bir` cannot produce
  the prepared-BIR handoff required by the x86 backend

## Step 3: Stabilize Ownership Around Stack/Addressing Cases

Goal: keep idea 58 narrowly honest when dynamic local/member/array addressing
semantics are the real blocker.

Primary targets:

- `ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md`
- `ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md`
- `src/backend/bir/lir_to_bir_memory_addressing.cpp`
- `src/backend/bir/lir_to_bir_memory_local_slots.cpp`

Actions:

- confirm whether remaining failures after Step 2 are still generic idea-58
  lowering gaps or better described as stack/addressing ownership
- when the durable blocker is stack/addressing semantics, route follow-on work
  to idea 62 instead of extending idea 58 with stack-specific fixes
- keep any boundary clarification in lifecycle state minimal and durable rather
  than turning `plan.md` into packet history

Completion check:

- remaining stack/addressing failures are clearly separated from generic
  semantic-lowering progress, preventing idea-58 execution from drifting into
  idea-62 scope

## Step 4: Validate Handoff Progress Against Owned Failures

Goal: prove that accepted slices advance the owned failure family toward real
prepared-x86 consumption.

Actions:

- require a fresh build for every accepted code slice
- prove the repaired route with backend-focused tests that exercise the owned
  semantic-lowering family
- broaden validation when changes cut across multiple shared lowering files or
  move both generic and stack/addressing-adjacent seams together
- reject proof that only demonstrates boundary fixtures while owned c-testsuite
  failures remain unexamined

Completion check:

- accepted slices have fresh proof and show credible progress on the owned
  semantic-lowering family rather than only on narrow boundary assertions
