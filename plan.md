# LIR-To-New-BIR Scalar Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 820 DirectScalar producer/verifier publication

## Purpose

Receive exactly the one already selected scalar `LirBinOp.lhs` body-parameter authority row. This is a bounded Raw-BIR receiver packet and does not complete 734's coverage matrix.

## Historical Progress

Steps 1 through 7.34 are accepted historical 734 work, most recently the direct-pointer body-parameter receipt (`8418036b1`). Closed 818 selected the sole scalar receiver row and closed 820 resolved its independent LIR producer/verifier prerequisite. Do not repeat completed receiver rows or producer-authority work.

## Core Rule

Receive only the structured current-function authority selected by closed 818: matching `LirValueId`, parameter index, scalar `LirTypeRef`, current `LinkNameId` owner, `LirNativeBodyParameterAbi::DirectScalar`, explicit `Lhs` role, and matching `LirBinOp.lhs` SSA value/type. Do not derive authority from presentation fields, text, operands, or diagnostics.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md` (the closed-818 and closed-820 resumption records)
- `ideas/closed/818_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/820_lir_directscalar_parameter_producer_verifier_publication.md`
- the existing unaccepted Step 7.35 receiver work, only for diagnosis; it is not accepted progress

## Non-Goals

- Accepting, modifying, or relying on pending Ideas 821/822 selector work or the existing unaccepted Step 7.35-shaped receiver patch.
- Generic scalar receipt, other parameter forms, DirectScalar producer or verifier work, Raw-BIR redesign, or presentation-derived recovery.
- Repeating Steps 1 through 7.34 or claiming source-idea completion.

## Ordered Steps

### Step 7.35 - Receive the one 818-authorized body-parameter authority row

Goal: transactionally import only closed 818's selected scalar `LirBinOp.lhs` body-parameter authority into its typed Raw-BIR destination.

Actions:

- inspect the unaccepted receiver patch only as a starting diagnosis; retain it as unaccepted until a bounded implementation packet proves it;
- add only the minimum typed Raw-BIR destination, importer dispatch, reachable verifier work, and nearby positive/malformed-authority coverage for the selected row;
- reject missing, invalid, duplicate, foreign-owner, out-of-range, non-scalar, type- or ABI-incoherent, wrong-role, and lhs value/type-mismatch authority transactionally;
- run a fresh build, the fixed executable `^backend_` proof, and the full checkpoint when the implementation packet is ready. Missing backend executables must be repaired or replaced by a supervisor-selected executable matching proof before acceptance.

Completion check: exactly the selected scalar `LirBinOp.lhs` row imports and verifies from structured authority; no generic scalar or other parameter form is admitted, no presentation recovery occurs, and the supervisor accepts the fresh focused and full proof. Reassess 734's source completion gate after this bounded receipt.
