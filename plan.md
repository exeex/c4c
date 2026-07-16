# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md

## Purpose

Resume 734 after closed 829's one-row body-parameter authority handoff and
receive that exact current-LIR semantic fact into typed Raw BIR.

## Goal

Receive the `FixedDirectCallArgument1` DirectScalar body-parameter authority
row into a typed Raw-BIR call-argument destination with importer dispatch,
reachable verification, and transactional malformed coverage.

## Core Rule

Do not recover authority from text, names, rendered operands, signatures,
`monostate`, diagnostics, or unclassified values. The receiver must consume
only the native tuple handed off by closed 829.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/829_lir_next_body_parameter_authority_handoff.md`
- The previous accepted Step 7.40 receiver commit `6609d92d4`
- Current Raw-BIR call-argument parameter authority containers, importer, and
  reachable verifier tests

## Current Targets And Scope

- Preserve accepted Steps 1 through 7.40 as historical work.
- Add exactly one Step 7.41 receiver row for closed 829's handoff:
  current-function DirectScalar parameter 1 used unchanged as fixed direct-call
  argument 1 at `LirCallOp.structured_args[1]`.
- Preserve value, owner, parameter index, type, ABI, explicit
  `FixedDirectCallArgument1` role, and call argument/type/signature coherence.
- Add transactional positive and malformed-authority coverage.

## Non-Goals

- Do not repeat Step 7.40 or receive argument 0 again.
- Do not receive another parameter, memory/VA, aggregate/vector,
  module/type/global, instruction/terminator, or inline-assembly form.
- Do not edit native LIR producer authority; closed 829 owns that producer.
- Do not infer from presentation fields or broaden to generic call arguments.

## Working Model

Closed 829 publishes the native authority. 734 owns only the typed Raw-BIR
destination/importer/verifier receipt. The previous argument-0 receipt is the
nearest local pattern, but this packet must stay argument-1-specific.

## Execution Rules

- Keep implementation bounded to one Raw-BIR receiver row and nearby tests.
- Reject missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent authority transactionally before publication.
- Run a fresh build plus focused backend proof for this receiver family.
- Use matching before/after regression logs for the focused backend proof
  before accepting a code change.

## Steps

### Step 7.41 - Receive The 829 Fixed-Direct-Call Argument-1 Body-Parameter Row

Goal: consume closed 829's DirectScalar argument-1 authority into typed Raw
BIR without presentation recovery.

Actions:

- Add or extend the typed Raw-BIR destination for fixed direct-call argument-1
  body-parameter authority.
- Import only the handed-off native tuple from
  `LirCallOp.structured_args[1].fixed_direct_call_argument_parameter_authority`.
- Verify reachable Raw-BIR preserves value, owner, parameter index, type, ABI,
  role, and direct call argument-1 coherence.
- Add positive and malformed transactional coverage matching 829's producer
  authority failure modes.

Completion check:

- Fresh build and focused backend proof pass with a matching non-regressing
  before/after guard, and the supervisor can reassess the 734 completion gate.
