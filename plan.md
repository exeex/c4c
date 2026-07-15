# LIR-To-New-BIR DirectScalar Binary-RHS Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 823 DirectScalar binary-RHS authority handoff

## Purpose

Receive exactly one already-authorized `LirBinOp.rhs` DirectScalar body-parameter row into typed Raw-BIR. This bounded receipt does not complete 734's coverage matrix.

## Historical Progress

Steps 1 through 7.35 are accepted historical 734 work, most recently the DirectScalar binary-LHS receipt in `18443fc0f`. Closed 823's accepted producer/schema/verifier handoff is `83d7959f2`; its fresh focused proof and matching before/after 1/1 regression guard are recorded in the source resumption record. Do not repeat accepted receiver rows or producer authority publication.

## Core Rule

Receive only closed 823's native structured tuple: `LirBinOp.scalar_rhs_parameter_authority` carrying the RHS `LirValueId`, parameter index, `LirTypeRef`, current-function `LinkNameId` owner, `LirNativeBodyParameterAbi::DirectScalar`, explicit `Rhs` role, matching `LirBinOp.rhs` SSA operand/value, and matching operation type. Do not derive authority from text, names, diagnostics, rendered operands, or the LHS carrier.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md` (the closed-823 resumption record)
- `ideas/closed/823_lir_next_body_parameter_authority_handoff.md`
- the accepted Step 7.35 history only as completed context

## Non-Goals

- Any producer/schema/verifier change, generic scalar/parameter admission, or receipt of another body-parameter form.
- Repeating the accepted direct-pointer or DirectScalar binary-LHS receipts.
- Memory/VA, aggregate/vector, module/type/global/metadata, instruction/terminator, inline-assembly, or presentation-derived recovery.
- Accepting, modifying, or relying on pending Ideas 821/822 selector work.

## Ordered Steps

### Step 7.36 - Receive the one 823-authorized DirectScalar binary-RHS body-parameter authority row

Goal: transactionally import only closed 823's structured RHS authority into the minimum typed Raw-BIR binary destination.

Actions:

- add only the required typed Raw-BIR destination, importer dispatch, reachable verifier work, and nearby positive/malformed-authority coverage;
- preserve current-function owner, parameter index, type, ABI, `Rhs` role, RHS SSA value, and operation type exactly;
- reject absent carrier, invalid value, duplicate definition, foreign owner, wrong role, type/ABI mismatch, and RHS operand/value mismatch without partial Raw-BIR publication;
- run a fresh build and focused receiver proof selected by the supervisor, then the appropriate broader acceptance checkpoint.

Completion check: exactly the selected RHS tuple imports and verifies from native structured authority, with transactional malformed rejection and no generic scalar/parameter admission. Reassess 734's source completion gate after this bounded receipt.
