# LIR-To-New-BIR Container Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed After: closed 864 direct one-double-argument call-result authority
handoff

## Purpose

Resume idea 734 from its accepted post-Step 7.50 state and receive exactly the
closed 864 non-body-parameter handoff into typed Raw BIR.

## Goal

Implement one bounded Raw-BIR receiver packet for direct nonvariadic
`double(double)` `LirCallOp` result authority.

## Core Rule

Receive only structured LIR authority from the accepted 864 handoff. Do not
recover semantic identity, type, role, opcode, operand relation, or consumer
coherence from text, names, rendered operands, signatures, diagnostics,
compatibility mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/864_lir_next_non_body_parameter_authority_handoff.md`
- Accepted 734 Step 7.50 receiver commit `72a368b06`
- 864 implementation commit `874499489`
- Existing Raw-BIR direct call-result receiver patterns from accepted 734 work

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.50 as historical work.
- Add only the typed Raw-BIR destination, importer dispatch, reachable verifier
  path, and transactional coverage needed for the 864-authorized row.
- The selected row is
  `LirCallOp.direct_one_double_arg_scalar_floating_call_authority` for a
  direct nonvariadic `double(double)` call result.
- Preserve the handed-off native result `LirValueId`, current-function owner
  `LinkNameId`, direct callee `LinkNameId`, exact `double(double)` type tuple,
  and role `DirectCallResult`.
- Downstream consumer coherence is deliberately outside this row.

## Non-Goals

- Do not edit LIR producer/schema/verifier authority for this row; 864 owns
  that producer-side prerequisite.
- Do not repeat Step 7.50, reopen fixed direct-call arguments 0/1, or reopen
  accepted body-parameter receipts.
- Do not require, infer, or verify downstream floating binary LHS consumer
  coherence for this row.
- Do not receive another call-result row, call-argument row, floating binary
  parameter-use row, or generic ordinary-value row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata, CFG/
  PHI, residual instruction/terminator, inline-assembly, ABI-expanded or
  aggregate parameters, body-parameter rows, generic residual sweeps, or any
  other family.
- Do not weaken unsupported diagnostics, expectation contracts, or no-partial-
  publication behavior.

## Execution Rules

- Keep the packet receiver-side only and tied to the exact 864 handoff tuple.
- Reuse existing typed Raw-BIR receiver conventions where they match direct
  call-result identity.
- Reject malformed authority before any partial Raw-BIR publication.
- Add nearby same-feature positive and malformed receiver coverage for absent
  authority, invalid or stale result IDs, duplicate result IDs, foreign owner
  or callee, signature/type mismatch, argument-type mismatch, invalid role,
  and misleading presentation strings.
- Prove the row does not depend on selected downstream floating binary LHS
  consumer coherence.
- Keep nonselected rows fail-closed without presentation recovery.
- For code changes, run a fresh build, focused receiver proof, and
  `git diff --check`. Escalate to broader backend proof if shared importer or
  verifier code is touched.

## Steps

### Step 7.51 - Receive the one 864-authorized double(double) direct call-result authority row

Goal: consume exactly the 864 direct nonvariadic `double(double)` call-result
handoff in typed Raw BIR.

Primary targets:

- Raw-BIR receiver container/builder/view surface for the selected direct
  call-result relation.
- LIR-to-Raw-BIR importer dispatch for the structured
  `LirDirectOneDoubleArgScalarFloatingCallAuthority` tuple.
- Reachable Raw-BIR verifier checks and transactional malformed-input
  coverage.

Actions:

- Inspect accepted Step 7.50 and adjacent direct call-result receiver patterns
  before editing.
- Add only the destination and importer handling needed for
  `LirCallOp.direct_one_double_arg_scalar_floating_call_authority` with role
  `DirectCallResult`.
- Preserve and verify the native result `LirValueId`, owner `LinkNameId`,
  direct callee `LinkNameId`, exact `double(double)` type tuple, and explicit
  role.
- Add positive and malformed receiver coverage matching the 864 handoff's
  malformed matrix.
- Keep downstream floating binary LHS consumer coherence out of the receiver
  contract for this row.
- Keep all nonselected rows fail-closed without presentation recovery.

Completion check:

- Fresh build passes.
- `git diff --check` passes.
- Focused receiver proof passes for the Raw-BIR importer path.
- The packet does not modify LIR producer authority, repeat accepted Steps 1
  through 7.50, require downstream consumer coherence, or admit any row beyond
  the selected 864 handoff.
