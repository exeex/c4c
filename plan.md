# LIR-To-New-BIR Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md

## Purpose

Resume 734 after closed 854's accepted producer/schema/verifier handoff and
receive exactly one further function-body parameter row into typed Raw BIR.

## Goal

Add the Raw-BIR receiver for the closed-854 DirectScalar unary-`fneg`
body-parameter authority while preserving all accepted receiver history
through Step 7.42.

## Core Rule

Use only native structured LIR authority. Do not recover parameter identity,
type, ABI, owner, role, or consumer coherence from text, names, rendered
operands, signatures, diagnostics, compatibility mirrors, or testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/854_lir_next_body_parameter_authority_handoff.md`
- Current Raw-BIR body-parameter receiver code and verifier coverage
- Existing accepted Step 7.34 through Step 7.42 parameter receiver tests

## Current Targets And Scope

- Preserve accepted Steps 1 through 7.42, most recently receiver commit
  `96853b2a2`.
- Consume only closed 854's DirectScalar unary-`fneg` tuple: original
  current-function parameter `LirValueId`, owner, parameter index, matching
  `LirTypeRef`, `DirectScalar` ABI, explicit `Lhs` role, and unary `LirBinOp`
  consumer coherence with opcode `fneg`, parameter SSA/value as `lhs`, and
  empty `rhs`.
- Add only the minimum typed Raw-BIR destination, importer dispatch, reachable
  verifier path, and transactional positive/negative coverage needed for that
  row.

## Non-Goals

- Do not repeat accepted parameter receipts through pointer truthiness.
- Do not receive another parameter row, memory/VA row, aggregate/vector row,
  module/type/global/metadata row, residual instruction/terminator row, or
  inline-assembly row.
- Do not edit LIR producer/schema code as part of this receiver packet.
- Do not infer missing facts from operand text, opcode spelling, signatures,
  names, diagnostics, compatibility mirrors, or rendered output.

## Execution Rules

- Keep the packet bounded to Step 7.43 unless the verifier exposes a true
  blocker outside 734's receiver scope.
- Add nearby same-feature malformed-authority coverage for the selected row.
- For code changes, run a fresh build, focused backend receiver proof, and the
  supervisor-selected regression guard.
- After the one receipt, return to the 734 source completion gate; do not
  claim source-wide completeness from this row alone.

## Steps

### Step 7.43 - Receive DirectScalar Fneg Parameter Authority

Goal: receive closed 854's one DirectScalar unary-`fneg` body-parameter
authority row into typed Raw BIR.

Actions:

- Inspect the existing typed Raw-BIR parameter receiver structures for the
  accepted DirectPointer and DirectScalar rows.
- Add the smallest destination/verifier representation for the selected `Lhs`
  unary-`fneg` role if none already exists.
- Import only a verified closed-854 tuple whose original parameter identity,
  owner, parameter index, scalar type, DirectScalar ABI, explicit `Lhs` role,
  unary `fneg` opcode, `lhs` parameter value, and empty `rhs` are present and
  consistent.
- Reject missing, invalid, foreign, duplicate, owner/index/type/ABI/role, and
  consumer-incoherent authority before Raw-BIR publication.
- Add focused positive and malformed tests next to the existing body-parameter
  receiver coverage.

Completion check:

- Fresh build passes.
- Focused backend receiver proof passes.
- Matching regression guard is non-regressive.
- `git diff --check` passes.
- `todo.md` records the exact received tuple, proof, and source completion
  reassessment request.
