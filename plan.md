# DirectScalar Switch-Selector Authority Runbook

Status: Active
Source Idea: ideas/open/825_lir_next_body_parameter_authority_handoff.md
Resumed from: ideas/closed/827_lir_direct_scalar_binary_lhs_authority_repair.md, Step 2

## Purpose

Publish one checked native DirectScalar parameter authority row for an
unchanged integer parameter consumed directly as `LirSwitch.selector`.

## Core Rule

Use the selected native parameter-definition tuple only. Do not reuse binary
LHS authority, materialize an `add`, or derive authority from display text.

## Read First

- `ideas/open/825_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/827_lir_direct_scalar_binary_lhs_authority_repair.md`
- `src/codegen/lir/hir_to_lir/core.cpp`
- `src/codegen/lir/verify.cpp`

## Non-Goals

- Raw-BIR/importer or receiver work, generic parameter admission, and any
  second parameter-use row.
- Accepting or blending preserved Idea 821/822 work merely because it is dirty.

## Ordered Steps

### Step 2 - Publish and verify the selected authority tuple

Goal: publish the optional direct `LirSwitch` selector authority from the
native DirectScalar definition and verify its exact consumer relation.

Actions:

- retain the tuple `(value, owner, parameter index, type, DirectScalar ABI,
  SwitchSelector role)` only when the selector directly names that parameter;
- preserve fail-closed missing, malformed, duplicate, foreign, and incoherent
  rejection; do not reuse `scalar_lhs_parameter_authority` or materialize an
  add;
- treat the existing dirty selector implementation as unaccepted until it has
  its own focused positive/malformed proof.

Completion check: the selected one-row route has independent build and focused
proof, with no credit taken from 827/828 or preserved adjacent dirty slices.

### Step 3 - Write the 734 handoff

Goal: record the exact selected tuple, consumer relation, rejection boundary,
and bounded 734 receiver return action.

Completion check: a durable one-row handoff exists and no nonselected form is
admitted.
