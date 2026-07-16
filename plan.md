# LIR-To-New-BIR Container Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed After: closed 857 DirectScalar binary-`fadd` LHS parameter-authority handoff

## Purpose

Resume idea 734 from its accepted post-Step 7.45 state and receive exactly the
closed 857 body-parameter handoff into typed Raw BIR.

## Goal

Implement one bounded Raw-BIR receiver packet for the selected
`LirBinOp.scalar_lhs_parameter_authority` floating `fadd` LHS DirectScalar
parameter-use row.

## Core Rule

Receive only structured LIR authority from the accepted 857 handoff. Do not
recover semantic identity from text, names, rendered operands, signatures,
diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/857_lir_next_body_parameter_authority_handoff.md`
- Accepted 734 Step 7.45 receiver commit `2b7e897ce`
- 857 implementation commit `6c11a15c1`
- 857 lifecycle close commit `8531a9572`
- Existing Raw-BIR body-parameter receiver patterns for Steps 7.34 through
  7.45

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.45 as historical work.
- Add only the typed Raw-BIR destination, importer dispatch, reachable verifier
  path, and transactional coverage needed for the 857-authorized row.
- The selected row is a current-function `DirectScalar` floating parameter used
  as LHS of binary `fadd` through `LirBinOp.scalar_lhs_parameter_authority`.
- Preserve original parameter identity, owner, parameter index, floating type,
  DirectScalar ABI, explicit `Lhs` role, `fadd` opcode, LHS value coherence,
  matching operation type, and nonselected scalar RHS.

## Non-Goals

- Do not edit LIR producer/schema/verifier authority for this row; 857 owns
  that producer-side prerequisite.
- Do not repeat Step 7.45 or reopen accepted DirectPointer or DirectScalar
  body-parameter receipts.
- Do not receive floating binary-`fadd` RHS or any other parameter row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, ABI-expanded or aggregate
  parameters, direct-call argument positions beyond bounded accepted slots,
  generic parameter sweeps, or any other family.
- Do not weaken unsupported diagnostics, expectation contracts, or no-partial-
  publication behavior.

## Execution Rules

- Keep the packet receiver-side only and tied to the exact 857 handoff tuple.
- Reuse existing typed Raw-BIR body-parameter receiver conventions where they
  match the selected LHS row.
- Reject malformed authority before any partial Raw-BIR publication.
- Add nearby same-feature positive and malformed receiver coverage for
  omitted or missing authority, invalid value, duplicate definition, foreign
  owner, wrong index, wrong type, wrong ABI, wrong role, non-`fadd`, LHS
  mismatch, type mismatch, selected-RHS incoherence, duplicate selected
  consumer forms, and a neighboring nonselected-operator negative such as
  `fsub` where appropriate.
- For code changes, run a fresh build, focused receiver proof, and
  `git diff --check`. Escalate to broader backend proof if shared importer or
  verifier code is touched.

## Steps

### Step 7.46 - Receive the one 857-authorized DirectScalar binary-fadd-LHS parameter authority row

Goal: consume exactly the 857 `fadd` LHS DirectScalar parameter-use handoff in
typed Raw BIR.

Primary targets:

- Raw-BIR body-parameter receiver container/builder/view surface for the
  selected LHS relation.
- LIR-to-Raw-BIR importer dispatch for `LirBinOp.scalar_lhs_parameter_authority`.
- Reachable Raw-BIR verifier checks and transactional malformed-input coverage.

Actions:

- Inspect the accepted Step 7.45 RHS receiver and adjacent body-parameter
  receiver patterns before editing.
- Add only the destination and importer handling needed for
  `LirBinOp.scalar_lhs_parameter_authority` with `Lhs` role and binary `fadd`.
- Preserve and verify the original parameter `LirValueId`, owner
  `LinkNameId`, parameter index, matching floating `LirTypeRef`,
  `LirNativeBodyParameterAbi::DirectScalar`, explicit `Lhs` role, LHS operand
  identity, operation type, and nonselected scalar RHS coherence.
- Add positive and malformed receiver coverage matching the 857 handoff's
  malformed matrix.
- Keep all nonselected rows fail-closed without presentation recovery.

Completion check:

- Fresh build passes.
- Focused receiver proof passes for the body-parameter Raw-BIR importer path.
- `git diff --check` passes.
- The packet does not modify LIR producer authority, repeat accepted Steps 1
  through 7.45, or admit any row beyond the selected 857 handoff.
