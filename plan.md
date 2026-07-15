# LIR-To-New-BIR Direct-Pointer Body-Parameter Receiver Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 817 direct-pointer body-parameter authority handoff
(`613f947b5`).

## Purpose

Receive exactly closed 817's one checked direct non-expanded pointer
function-body parameter row. This is a single bounded Raw-BIR receiver packet
and does not complete 734's coverage matrix.

## Historical Progress

Steps 1 through 7.33 are accepted historical 734 work, most recently the
inline-assembly output-only receipt (`3b8870e2b`). Do not repeat any completed
receiver row or producer-authority work.

## Core Rule

Use only the structured current-function `LirValueId`, parameter index,
pointer `LirTypeRef`, current `LinkNameId` owner, and explicit
`LirNativeBodyParameterAbi::DirectPointer` from closed 817. Do not derive
authority from spelling, signature text, raw operands, diagnostics, or any
other presentation field.

## Read First

- `ideas/closed/817_lir_body_parameter_receiver_authority_handoff.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (closed-817
  resumption record)
- `docs/lir_to_new_bir_remaining_coverage/successor_queue.md`

## Non-Goals

- Scalar, byval/aggregate, HFA/vector, array, variadic, or any other parameter
  form;
- producer/schema/verifier republishing, broad ABI work, declaration-only
  publication, target lowering, MIR/emission, or importer/dispatcher sweeps;
- remaining memory/VA, aggregate/vector, module/type/global, instruction,
  terminator, inline-assembly, and documentation-convergence work.

## Ordered Steps

### Step 7.34 - Receive selected direct-pointer body-parameter authority

Goal: transactionally import closed 817's single `LirGepOp.ptr` typed-GEP-base
row for direct-pointer parameter `p[0]` into one typed Raw-BIR destination.

Actions:

- consume exactly `LirValueId`, parameter index, pointer `LirTypeRef`, current
  `LinkNameId` owner, and `LirNativeBodyParameterAbi::DirectPointer`;
- add only the minimum typed Raw-BIR destination, importer dispatch, reachable
  verifier work, and nearby positive/malformed-authority receiver coverage;
- reject missing, invalid, foreign, duplicate, type-incoherent, malformed-ABI,
  scalar, byval/aggregate, HFA/vector, array, variadic, and every other
  parameter form transactionally;
- run a fresh build and focused `^backend_lir_to_bir_interface$` receiver proof
  before the supervisor-selected matching regression guard and broader proof.

Completion check: exactly the selected direct non-expanded pointer typed-GEP
base imports and verifies from structured authority; no presentation recovery
or other parameter receipt occurs. Reassess 734's source completion gate after
this bounded receipt.
