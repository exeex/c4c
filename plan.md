# LIR-To-New-BIR Direct-Scalar Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 818 scalar binary-LHS body-parameter authority handoff
(`0864c8aed` selection; `b16935c69` producer/proof).

## Purpose

Receive exactly the one 818-authorized plain fixed scalar `LirBinOp.lhs`
function-body parameter row. This is a bounded Raw-BIR receiver packet and
does not complete 734's coverage matrix.

## Historical Progress

Steps 1 through 7.34 are accepted historical 734 work, most recently the
direct-pointer body-parameter receipt (`8418036b1`). Do not repeat completed
receiver rows or producer-authority work.

## Core Rule

Consume only the matching native `LirValueId`, parameter index, `LirTypeRef`,
current-function `LinkNameId` owner, `LirNativeBodyParameterAbi::DirectScalar`,
explicit `Lhs` role, and matching `LirBinOp.lhs` SSA value/type. Presentation
fields, including names, signatures, raw operands, diagnostics, and testcase
identity, are never authority.

## Read First

- `ideas/closed/818_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/819_lir_scalar_binary_lhs_parameter_authority.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (closed-818
  resumption record)

## Non-Goals

- Any other parameter form, ABI class, operand role, or binary operation;
- producer/schema/verifier republishing, broad ABI work, declaration-only
  publication, target lowering, MIR/emission, or importer/dispatcher sweeps;
- remaining memory/VA, aggregate/vector, module/type/global, instruction,
  terminator, inline-assembly, and documentation-convergence work.

## Ordered Steps

### Step 7.35 - Receive the one 818-authorized body-parameter authority row

Goal: transactionally import the selected DirectScalar `LirBinOp.lhs` body
parameter authority into one typed Raw-BIR destination.

Actions:

- consume exactly the closed-818 structured handoff fields;
- add only the minimum typed Raw-BIR destination, importer dispatch, reachable
  verifier work, and nearby positive/malformed-authority receiver coverage;
- reject absent, invalid, duplicate, foreign-owner, out-of-range, non-scalar,
  type-or-ABI-incoherent, wrong-role, and lhs value/type-mismatch forms before
  publication;
- run a fresh build and focused receiver proof before the supervisor-selected
  matching regression guard and broader proof.

Completion check: exactly the selected DirectScalar `LirBinOp.lhs` parameter
row imports and verifies from structured authority; no presentation recovery or
other parameter receipt occurs. Reassess 734's source completion gate after
this bounded receipt.
