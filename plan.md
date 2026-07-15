# LIR-To-New-BIR DirectScalar Return-Value Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 824 return-value parameter authority handoff

## Purpose

Receive exactly one already-authorized unchanged current-function DirectScalar
parameter return into typed Raw-BIR. This bounded receipt does not complete
734's coverage matrix.

## Historical Progress

Steps 1 through 7.36 are accepted historical 734 work, most recently the
DirectScalar binary-RHS receipt in `c87976453`. Closed 824's accepted
producer/schema/verifier handoff is `fac485148`, with a fresh build plus
`^backend_` 6/6 proof and matching before/after 6/6 non-regression guard. Do
not repeat accepted receiver rows or producer authority publication.

## Core Rule

Receive only `LirRet.return_value_parameter_authority`, when present for an
unchanged current-function DirectScalar parameter returned through the exact
SSA return operand. Its authority is the value identity, parameter index,
`LirTypeRef`, owning `LinkNameId`, `LirNativeBodyParameterAbi::DirectScalar`,
and `ReturnValue` role. It must agree with exactly one native definition and
the return operand and signature return. No presentation field is authority.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md` (the closed-824
  resumption record)
- `ideas/closed/824_lir_next_body_parameter_authority_handoff.md`
- accepted Step 7.36 history only as completed context

## Non-Goals

- Producer/schema/verifier changes, generic scalar/parameter admission, or
  receipt of another body-parameter form.
- Repeating accepted direct-pointer or DirectScalar binary-LHS/RHS receipts.
- Memory/VA, aggregate/vector, module/type/global/metadata,
  instruction/terminator, inline assembly, or presentation-derived recovery.
- Accepting, modifying, or relying on pending Ideas 821/822 selector work.

## Ordered Steps

### Step 7.37 - Receive the one 824-authorized DirectScalar return-value parameter authority row

Goal: transactionally import only closed 824's structured return-value
authority into the minimum typed Raw-BIR return destination.

Actions:

- add only the required typed Raw-BIR destination, importer dispatch,
  reachable verifier work, and nearby positive/malformed-authority coverage;
- preserve value identity, parameter index, type, owner, DirectScalar ABI,
  ReturnValue role, exact SSA return operand relation, and signature-return
  agreement;
- require exactly one matching native definition and reject missing carrier
  for the selected form, malformed/foreign/duplicate authority, owner/index/
  type/ABI/role mismatch, return-operand mismatch, and signature-return
  mismatch transactionally;
- leave nonselected return forms carrier-free and outside this row;
- run a fresh build and focused receiver proof selected by the supervisor,
  then the appropriate broader acceptance checkpoint.

Completion check: exactly the selected ReturnValue tuple imports and verifies
from native structured authority, malformed input produces no partial Raw-BIR
publication, and no generic parameter admission occurs. Reassess 734's source
completion gate after this bounded receipt.
