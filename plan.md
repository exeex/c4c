# LIR-To-New-BIR Body-Parameter Argument-1 Receipt Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md

## Purpose

Resume 734 after closed 829 supplied the next exact body-parameter authority
handoff. Receive exactly that one fixed-direct-call argument-1 DirectScalar
body-parameter row into typed Raw BIR without reopening prior receiver rows.

## Goal

Add the Raw-BIR destination, importer dispatch, reachable verifier path, and
transactional malformed-authority coverage for the one 829-authorized
`LirCallOp.structured_args[1]` body-parameter authority tuple.

## Core Rule

Consume only native structured LIR authority. Do not reconstruct parameter
identity, type, role, call coherence, or signature facts from text, names,
rendered operands, diagnostics, signature strings, or compatibility mirrors.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/829_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/830_lir_direct_call_structured_argument_identity_prerequisite.md`
- Prior Step 7.40 receiver commit `6609d92d4`
- Current Raw-BIR call-argument containers, importer dispatch, reachable
  verifier checks, and `backend_lir_to_bir_interface` coverage

## Current Targets And Scope

- Preserve accepted 734 Steps 1 through 7.40.
- Receive only the closed-829 tuple: current-function `DirectScalar`
  parameter 1 value, owner, parameter index, type, ABI, and explicit
  `FixedDirectCallArgument1` role, used unchanged as
  `LirCallOp.structured_args[1]` for a direct, non-variadic, specified
  two-parameter call.
- Preserve the closed-830 call argument/type/signature coherence prerequisite
  as producer evidence; do not reimplement it.
- Add same-feature positive and malformed-authority receiver coverage.

## Non-Goals

- Do not edit LIR producers, schemas, or verifier authority publication.
- Do not receive argument 0 again or reopen accepted DirectPointer,
  DirectScalar GEP, binary-LHS, binary-RHS, ReturnValue, switch-selector,
  truthiness-comparison-LHS, or fixed-direct-call-argument-0 rows.
- Do not receive other parameter forms, other argument indices, generic calls,
  variadic/indirect/unspecified calls, ABI conversion, Raw-BIR families beyond
  this row, memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, or inline-assembly forms.

## Working Model

Step 7.40 already proved the argument-0 call-argument parameter receiver
pattern. Step 7.41 should extend the Raw-BIR receiving boundary only where the
role/index-specific authority differs, while preserving fail-closed behavior
for missing, invalid, duplicate, foreign, owner/index/type/ABI/role, and
consumer-incoherent input.

## Execution Rules

- Keep implementation bounded to backend Raw-BIR containers/importer/verifier
  and nearby backend interface tests.
- Preserve module-transactional behavior: malformed selected input publishes
  nothing.
- Run a fresh build plus focused backend receiver proof.
- Use matching before/after focused or backend regression logs before
  accepting code.
- After acceptance, send the exhausted runbook back to plan-owner for an
  explicit 734 close/repair/successor decision.

## Steps

### Step 7.41 - Receive The One 829-Authorized Body-Parameter Argument-1 Row

Goal: receive exactly closed 829's fixed-direct-call argument-1 DirectScalar
body-parameter authority tuple into typed Raw BIR.

Actions:

- Inspect the Step 7.40 argument-0 Raw-BIR destination, importer, verifier,
  and tests to identify the minimal role/index-specific extension point.
- Add or extend the typed Raw-BIR call-argument destination for the argument-1
  body-parameter tuple without replacing the native authority source.
- Import only `LirCallOp.structured_args[1]` when the closed-829 authority is
  present, valid, current-function-owned, DirectScalar, role-matched, and
  coherent with the direct fixed callee parameter 1.
- Add reachable verifier checks for the received tuple and transactional
  rollback on malformed selected input.
- Add focused positive and malformed receiver coverage for missing, invalid,
  duplicate, foreign, owner/index/type/ABI/role, and consumer-incoherent
  authority where not already covered by the shared Step 7.40 path.

Completion check:

- Fresh build passes.
- Focused backend receiver proof passes.
- Matching before/after focused or backend regression guard is non-regressing.
- `todo.md` records the exact received tuple and asks plan-owner to decide the
  next 734 lifecycle state.
