# LIR-To-New-BIR Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md

## Purpose

Resume 734 after closed 853's accepted producer/schema/verifier handoff and
receive exactly one further function-body parameter row into typed Raw BIR.

## Goal

Add the Raw-BIR receiver for the closed-853 DirectPointer pointer-truthiness
body-parameter authority while preserving all accepted receiver history through
Step 7.41.

## Core Rule

Use only native structured LIR authority. Do not recover parameter identity,
type, ABI, owner, role, or consumer coherence from text, names, rendered
operands, signatures, diagnostics, compatibility mirrors, or testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/853_lir_next_body_parameter_authority_handoff.md`
- Current Raw-BIR body-parameter receiver code and verifier coverage
- Existing accepted Step 7.34 through Step 7.41 parameter receiver tests

## Current Targets And Scope

- Preserve accepted Steps 1 through 7.41, most recently receiver commit
  `380ee782f`.
- Consume only closed 853's DirectPointer `PointerTruthiness` tuple:
  original current-function parameter `LirValueId`, owner, parameter index,
  pointer `LirTypeRef`, `DirectPointer` ABI, explicit role, and verified
  `PtrToInt` plus `icmp ne i64 <ptr-int>, 0` consumer relation.
- Add only the minimum typed Raw-BIR destination, importer dispatch, reachable
  verifier path, and transactional positive/negative coverage needed for that
  row.

## Non-Goals

- Do not repeat accepted parameter receipts through fixed-direct-call
  argument 1.
- Do not receive another parameter row, memory/VA row, aggregate/vector row,
  module/type/global/metadata row, residual instruction/terminator row, or
  inline-assembly row.
- Do not edit LIR producer/schema code as part of this receiver packet.
- Do not infer missing facts from pointer text, cast spelling, comparison
  text, signatures, names, diagnostics, or compatibility mirrors.

## Execution Rules

- Keep the packet bounded to Step 7.42 unless the verifier exposes a true
  blocker outside 734's receiver scope.
- Add nearby same-feature malformed-authority coverage for the selected row.
- For code changes, run a fresh build, focused backend receiver proof, and the
  supervisor-selected regression guard.
- After the one receipt, return to the 734 source completion gate; do not
  claim source-wide completeness from this row alone.

## Steps

### Step 7.42 - Receive DirectPointer Truthiness Parameter Authority

Goal: receive closed 853's one DirectPointer pointer-truthiness body-parameter
authority row into typed Raw BIR.

Actions:

- Inspect the existing typed Raw-BIR parameter receiver structures for the
  accepted DirectPointer and DirectScalar rows.
- Add the smallest destination/verifier representation for the selected
  `PointerTruthiness` role if none already exists.
- Import only a verified closed-853 tuple whose original parameter identity,
  owner, parameter index, pointer type, DirectPointer ABI, role, `PtrToInt`,
  and `icmp ne i64 <ptr-int>, 0` consumer coherence are present and
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
