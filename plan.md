# LIR Memcpy Selected Pointer/Object Authority Publication Runbook

Status: Active
Source Idea: ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md
Supersedes: blocked idea 734 until this producer handoff is accepted

## Purpose

Repair exactly the selected PL `LirMemcpyOp` producer authority that idea 734
cannot obtain from text, then hand the verified row back to its receiver.

## Goal

Publish typed current-function destination/source/size and pointer-object-
lifetime ownership for one selected non-volatile memcpy row, with no receiver
or presentation recovery.

## Core Rule

Structured typed fields are the sole semantic authority.  Display operands and
rendered LLVM may demonstrate parity but must never create, repair, or select
an identity, object, size, or lifetime fact.

## Read First

- `ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (resumption at
  Step 7.20 after this handoff)
- `docs/lir_remaining_ordinary_value_identity/authority_matrix.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and the selected PL
  `emit_lval_dispatch` memcpy producer

## Non-Goals

- Do not edit Raw-BIR/importer code or tests, and do not make 734 receiver
  progress in this plan.
- Do not generalize to other memcpy, memory/object, stack, parameter, va-list,
  aggregate/vector, CFG, target, allocation, MIR, or emission work.

## Execution Rules

1. Keep the selected producer row singular and name its exact source/fixture in
   the durable handoff before claiming completion.
2. Preserve textual operands only as compatibility display; do not parse them.
3. Fail verification before consumer-visible publication; no partial authority
   may survive an invalid selected row.
4. Prove build, focused producer/verifier coverage, then use the supervisor's
   matching regression and broader checks as required before handoff.

## Ordered Steps

### Step 1 - Define and populate the selected memcpy typed authority

Goal: make only the selected PL non-volatile memcpy row carry the typed
destination/source/size and object/lifetime fields required by its consumer.

Primary targets:

- LIR memcpy schema and the selected `emit_lval_dispatch` producer path
- narrowly adjacent producer fixture

Actions:

- identify the one exact selected source path and retain its compatibility
  spelling independently of authority;
- populate current-function destination/source `LirValueId`s, typed size
  authority, and typed destination/source object IDs with their live ownership
  relation; and
- leave all unselected memcpy producers and operand forms unchanged/fail-closed.

Completion check:

- the selected row has no semantic dependence on monostate/text operands, and
  the focused fixture exposes the intended typed fields.

### Step 2 - Enforce the selected row's verifier boundary

Goal: reject incoherent selected-row value, object, owner, type, size, and
lifetime authority before it can be handed to a receiver.

Actions:

- add only the reachable verifier checks needed for the selected schema;
- reject missing, invalid, duplicate/cross-function, type/kind, object-link,
  size, and lifetime failures; and
- preserve transactional behavior with no fallback to display text.

Completion check:

- each malformed selected-row neighbor fails for typed-authority reasons and
  valid current-function ownership succeeds.

### Step 3 - Prove the producer and publish the consumer handoff

Goal: establish focused proof and document the exact authority contract for
idea 734 without receiver work.

Actions:

- run a fresh build and the selected focused producer/verifier coverage;
- have the supervisor apply matching regression/broader validation as required;
- write the durable handoff with fields, ownership/lifetime rules, rejection
  cases, accepted commit/proof, and 734 return point `Step 7.20`.

Completion check:

- an executor can reactivate 734 and receive exactly one row without
re-auditing or parsing producer presentation.
