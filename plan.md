# LIR-To-New-BIR Container Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed After: closed 859 DirectScalar binary-`fsub` LHS parameter-authority handoff

## Purpose

Resume idea 734 from its accepted post-Step 7.47 state and receive exactly the
closed 859 body-parameter handoff into typed Raw BIR.

## Goal

Implement one bounded Raw-BIR receiver packet for the selected
`LirBinOp.scalar_lhs_parameter_authority` floating `fsub` LHS DirectScalar
parameter-use row.

## Core Rule

Receive only structured LIR authority from the accepted 859 handoff. Do not
recover semantic identity from text, names, rendered operands, signatures,
diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/859_lir_next_body_parameter_authority_handoff.md`
- Accepted 734 Step 7.47 receiver commit `b26d47f15`
- 859 implementation commit `c3a7883a4`
- Existing Raw-BIR body-parameter receiver patterns for Steps 7.34 through
  7.47

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.47 as historical work.
- Add only the typed Raw-BIR destination, importer dispatch, reachable verifier
  path, and transactional coverage needed for the 859-authorized row.
- The selected row is a current-function `DirectScalar` floating parameter used
  as LHS of binary `fsub` through `LirBinOp.scalar_lhs_parameter_authority`.
- Preserve original parameter identity, owner, parameter index, floating type,
  DirectScalar ABI, explicit `Lhs` role, `fsub` opcode, LHS value coherence,
  matching operation type, and nonselected scalar RHS.

## Non-Goals

- Do not edit LIR producer/schema/verifier authority for this row; 859 owns
  that producer-side prerequisite.
- Do not repeat Step 7.47 or reopen accepted DirectPointer or DirectScalar
  body-parameter receipts.
- Do not receive floating binary-`fsub` RHS or any other parameter row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, ABI-expanded or aggregate
  parameters, direct-call argument positions beyond bounded accepted slots,
  generic parameter sweeps, or any other family.
- Do not weaken unsupported diagnostics, expectation contracts, or no-partial-
  publication behavior.

## Execution Rules

- Keep the packet receiver-side only and tied to the exact 859 handoff tuple.
- Reuse existing typed Raw-BIR body-parameter receiver conventions where they
  match the selected LHS row.
- Reject malformed authority before any partial Raw-BIR publication.
- Add nearby same-feature positive and malformed receiver coverage for
  omitted or missing authority, invalid value, duplicate definition, foreign
  owner, wrong index, wrong type, wrong ABI, wrong role, non-`fsub`, LHS
  mismatch, type mismatch, selected-RHS incoherence, duplicate selected
  consumer forms, and a neighboring nonselected-operator negative such as
  `fdiv` where appropriate.
- For code changes, run a fresh build, focused receiver proof, and
  `git diff --check`. Escalate to broader backend proof if shared importer or
  verifier code is touched.

## Steps

### Step 7.48 - Receive the one 859-authorized DirectScalar binary-fsub-LHS parameter authority row

Goal: consume exactly the 859 `fsub` LHS DirectScalar parameter-use handoff in
typed Raw BIR.

Primary targets:

- Raw-BIR body-parameter receiver container/builder/view surface for the
  selected LHS relation.
- LIR-to-Raw-BIR importer dispatch for `LirBinOp.scalar_lhs_parameter_authority`.
- Reachable Raw-BIR verifier checks and transactional malformed-input coverage.

Actions:

- Inspect the accepted Step 7.47 RHS receiver and adjacent body-parameter
  receiver patterns before editing.
- Add only the destination and importer handling needed for
  `LirBinOp.scalar_lhs_parameter_authority` with `Lhs` role and binary `fsub`.
- Preserve and verify the original parameter `LirValueId`, owner
  `LinkNameId`, parameter index, matching floating `LirTypeRef`,
  `LirNativeBodyParameterAbi::DirectScalar`, explicit `Lhs` role, LHS operand
  identity, operation type, and nonselected scalar RHS coherence.
- Add positive and malformed receiver coverage matching the 859 handoff's
  malformed matrix.
- Keep all nonselected rows fail-closed without presentation recovery.

Completion check:

- Fresh build passes.
- Focused receiver proof passes for the body-parameter Raw-BIR importer path.
- `git diff --check` passes.
- The packet does not modify LIR producer authority, repeat accepted Steps 1
  through 7.47, or admit any row beyond the selected 859 handoff.
