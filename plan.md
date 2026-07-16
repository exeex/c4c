# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed After: closed 855 binary-`fmul` LHS parameter-authority handoff

## Purpose

Resume idea 734 after accepted Step 7.43 and closed 855's producer/verifier
handoff by receiving exactly one native body-parameter row into typed Raw BIR.

## Goal

Consume the closed 855 `DirectScalar` binary-`fmul` LHS parameter authority row
without rediscovering authority from presentation fields.

## Core Rule

Use only native structured LIR authority. Do not recover parameter identity,
type, ABI, owner, role, or consumer coherence from text, names, rendered
operands, signatures, diagnostics, compatibility mirrors, `monostate`, or
testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/855_lir_next_body_parameter_authority_handoff.md`
- The accepted Step 7.43 receiver commit `617a8fae9`
- The accepted 855 producer commit `dcb6e1233`
- Existing Raw-BIR body-parameter receiver patterns and rollback coverage

## Current Targets And Scope

- Preserve accepted 734 receiver progress through Step 7.43.
- Receive exactly the closed 855 handoff row:
  `LirBinOp.scalar_lhs_parameter_authority` for a current-function
  `DirectScalar` floating parameter used as LHS of binary floating `fmul`.
- Preserve the tuple: original parameter `LirValueId`, current
  `LirFunction.link_name_id` owner, parameter index, matching floating
  `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and explicit
  `LirScalarBinaryParameterRole::Lhs`.
- Preserve the consumer relation: `LirBinOp` opcode `fmul`, `lhs` equal to the
  same parameter SSA/value, `type_str` matching the authority type, and `rhs`
  a nonselected scalar operand.

## Non-Goals

- Do not edit LIR producer authority or verifier code for 855.
- Do not repeat Step 7.43 or reopen accepted DirectPointer/DirectScalar
  body-parameter receipts.
- Do not receive any other parameter row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, or other non-parameter
  families.
- Do not infer missing authority from presentation fields or testcase shape.

## Execution Rules

- Keep the runbook bounded to one Raw-BIR receiver packet.
- Add nearby positive and malformed receiver coverage for the selected row.
- For code changes, run a fresh build and focused backend receiver proof.
- After acceptance, reassess source completion before selecting any further
  receiver row.

## Steps

### Step 7.44 - Receive DirectScalar Binary-Fmul LHS Parameter Authority

Goal: map the closed 855 binary-`fmul` LHS body-parameter authority into typed
Raw BIR with transactional verification.

Actions:

- Inspect the existing body-parameter Raw-BIR receiver path and Step 7.43
  unary-`fneg` receiver pattern.
- Admit only the selected `DirectScalar` floating parameter LHS tuple and its
  matching binary `fmul` consumer relation.
- Store enough typed Raw-BIR authority to preserve parameter value, owner,
  index, type, ABI, explicit LHS role, opcode, lhs identity, matching result
  type, and nonselected scalar RHS coherence.
- Reject missing, invalid, duplicate, foreign, owner/index/type/ABI/role,
  non-`fmul`, LHS mismatch, type mismatch, and RHS consumer incoherence before
  partial publication.
- Prove positive and malformed cases with focused receiver coverage.

Completion check:

- Fresh build and focused receiver proof pass.
- `git diff --check` passes.
- No Raw-BIR receipt is claimed for any row other than the closed 855
  binary-`fmul` LHS handoff.
