# LIR Residual Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md

## Purpose

Continue the bounded residual LIR authority route without reopening accepted
cast-result or pointer-subtraction work and without converting this into a
generic residual sweep.

## Goal

Select exactly one currently valid residual instruction, terminator, or
inline-assembly value/type authority family that can be published and verified,
or prove that no such family is currently executable under this source.

## Core Rule

Every accepted packet must name native structured authority. Do not recover
identity, type, edge, or operand semantics from text, printer output,
diagnostics, testcase names, or opaque inline-assembly templates/constraints.

## Read First

- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
- `docs/lir_remaining_authority_owner_triage/classification.md`
- `docs/lir_intrinsic_binding_evidence/01_intrinsic_inline_asm_binding_route.md`
- `docs/lir_to_new_bir_final_coverage_convergence/terminal_disposition_matrix.md`

## Scope

- This runbook owns only one selected residual producer/schema/verifier
  authority family at a time.
- Accepted historical 796 work for scalar casts and pointer-subtraction
  result authority is evidence only and must not be repeated.
- Raw-BIR receipt remains downstream of a future exact handoff and is not part
  of this source.

## Non-Goals

- Do not edit Raw-BIR, the LIR-to-BIR importer, or receiver tests.
- Do not reopen 801, 806, 813, 847, or 797.
- Do not perform a combined residual sweep.
- Do not parse inline-assembly text, templates, constraints, or clobbers.
- Do not weaken verifier diagnostics or test expectations.

## Execution Rules

- Keep source changes bounded to the selected producer/schema/verifier family.
- If no exact current residual family has native facts ready to publish, stop
  with a lifecycle result instead of inventing a receiver or text fallback.
- Require nearby same-feature positive and negative coverage for any repair.
- Use fresh build plus the focused proof selected by the supervisor for each
  code-changing packet; broader validation remains supervisor-owned.

## Ordered Steps

### Step 1: Reconcile residual candidates

Goal: identify whether 796 has one currently executable residual family after
the accepted 797 final convergence and 849 intrinsic-binding evidence.

Actions:

- Read the source idea resumption records and the listed documentation inputs.
- Exclude accepted scalar cast and pointer-subtraction result routes.
- Exclude families whose only available facts are text, monostate, printer
  output, opaque inline-assembly payload, or downstream Raw-BIR receipt.
- Name exactly one candidate family with native structured facts and a verifier
  publication path, or record that no executable 796 family remains.

Completion check:

- `todo.md` records either the selected family and Step 2 target, or a
  lifecycle blocker/disposition for plan-owner.

### Step 2: Publish the selected residual authority

Goal: implement only the one Step 1-selected producer/schema/verifier handoff.

Actions:

- Add or repair the native structured authority carrier for the selected
  family.
- Add verifier checks for missing, invalid, foreign, type-incoherent, or
  display-derived authority as applicable.
- Preserve fail-closed behavior for every nonselected residual family.
- Add nearby positive and negative coverage for the selected family.

Completion check:

- Fresh build passes.
- Supervisor-selected focused proof passes.
- The accepted record names the exact future handoff consumer, if any.

### Step 3: Prove and return

Goal: close or reroute this bounded 796 packet without leaving a retired
runbook behind.

Actions:

- Record the accepted proof and exact selected-family disposition.
- If a future Raw-BIR receiver is authorized, identify the downstream 734
  return condition without activating 734.
- If no selected family is executable, request lifecycle closure or successor
  creation from plan-owner.

Completion check:

- The source idea has an exact completion, return, or successor record and the
  supervisor has enough evidence to close, repair, or switch.
