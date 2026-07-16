# LIR-To-New-BIR Container Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed After: closed 863 direct zero-argument scalar floating call-result
authority handoff

## Purpose

Resume idea 734 from its accepted post-Step 7.49 state and receive exactly the
closed 863 non-body-parameter handoff into typed Raw BIR.

## Goal

Implement one bounded Raw-BIR receiver packet for direct zero-argument scalar
floating `LirCallOp` result authority consumed as the LHS of a downstream
floating binary operation.

## Core Rule

Receive only structured LIR authority from the accepted 863 handoff. Do not
recover semantic identity from text, names, rendered operands, signatures,
diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/863_lir_next_non_body_parameter_authority_handoff.md`
- Accepted 734 Step 7.49 receiver commit `44edfbacd`
- 863 implementation commit `36f860ea3`
- Existing Raw-BIR direct call-result and floating binary receiver patterns
  from accepted 734 work

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.49 as historical work.
- Add only the typed Raw-BIR destination, importer dispatch, reachable verifier
  path, and transactional coverage needed for the 863-authorized row.
- The selected row is a direct zero-argument scalar floating `LirCallOp`
  result consumed as the LHS of a downstream floating binary operation.
- Preserve the handed-off native result `LirValueId`, current-function owner
  `LinkNameId`, direct callee `LinkNameId`, exact floating `LirTypeRef`, and
  role `ResultIntoFloatingBinaryLhs`.

## Non-Goals

- Do not edit LIR producer/schema/verifier authority for this row; 863 owns
  that producer-side prerequisite.
- Do not repeat Step 7.49 or reopen accepted body-parameter receipts.
- Do not receive another call-result row, call-argument row, floating binary
  parameter-use row, or generic ordinary-value row.
- Do not absorb memory/VA, aggregate/vector, module/type/global/metadata, CFG/
  PHI, residual instruction/terminator, inline-assembly, ABI-expanded or
  aggregate parameters, body-parameter rows, generic residual sweeps, or any
  other family.
- Do not weaken unsupported diagnostics, expectation contracts, or no-partial-
  publication behavior.

## Execution Rules

- Keep the packet receiver-side only and tied to the exact 863 handoff tuple.
- Reuse existing typed Raw-BIR receiver conventions where they match direct
  call-result identity and floating binary consumer coherence.
- Reject malformed authority before any partial Raw-BIR publication.
- Add nearby same-feature positive and malformed receiver coverage for absent
  authority, stale result, foreign owner, callee incoherence, type
  incoherence, role incoherence, and consumer incoherence.
- Keep nonselected rows fail-closed without presentation recovery.
- For code changes, run a fresh build, focused receiver proof, and
  `git diff --check`. Escalate to broader backend proof if shared importer or
  verifier code is touched.

## Steps

### Step 7.50 - Receive the one 863-authorized direct floating call-result authority row

Goal: consume exactly the 863 direct zero-argument scalar floating call-result
handoff in typed Raw BIR.

Primary targets:

- Raw-BIR receiver container/builder/view surface for the selected direct
  floating call-result relation.
- LIR-to-Raw-BIR importer dispatch for the structured
  `LirDirectZeroArgScalarFloatingCallAuthority` tuple.
- Reachable Raw-BIR verifier checks and transactional malformed-input
  coverage.

Actions:

- Inspect accepted Step 7.49 and adjacent direct call-result / floating binary
  receiver patterns before editing.
- Add only the destination and importer handling needed for direct
  zero-argument scalar floating `LirCallOp` result authority with role
  `ResultIntoFloatingBinaryLhs`.
- Preserve and verify the native result `LirValueId`, owner `LinkNameId`,
  direct callee `LinkNameId`, matching floating `LirTypeRef`, explicit role,
  and downstream floating binary LHS consumer coherence.
- Add positive and malformed receiver coverage matching the 863 handoff's
  malformed matrix.
- Keep all nonselected rows fail-closed without presentation recovery.

Completion check:

- Fresh build passes.
- Focused receiver proof passes for the Raw-BIR importer path.
- `git diff --check` passes.
- The packet does not modify LIR producer authority, repeat accepted Steps 1
  through 7.49, or admit any row beyond the selected 863 handoff.
