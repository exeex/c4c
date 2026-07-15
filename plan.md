# LIR PHI Floating Unary-Minus `fneg` Authority Runbook

Status: Active
Source Idea: ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md
Activated from: 806 Step 3 full-baseline gate; 806 is parked for resumption
after 807 and 808 resolve their separately scoped producer families.

## Purpose

Repair the one traced floating unary-minus `fneg` result-authority handoff
that reaches a PHI incoming in `ieee/pr50310.c`.

## Core Rule

The producer result must carry a native checked current-function `LirValueId`.
Keep the existing PHI verifier contract; do not recover authority from display
text or move the repair to a PHI or ternary consumer.

## Read First

- `ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md`
- `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`
- `src/codegen/lir/hir_to_lir/expr/misc.cpp` floating `UnaryOp::Minus` branch
- `review/806_step1_phi_producer_trace.md`

## Non-Goals

- Postfix old-value authority, scalar-integer-minus `sub`, scalar bit-not
  `xor`, other unary operators, generic provenance, and PHI/CFG semantics.
- Rendered `%t` recovery, testcase-name paths, or expectation downgrades.

## Ordered Steps

### Step 1 - Reconfirm the bounded floating-minus handoff and choose coverage

Goal: verify the `fneg` producer-to-PHI authority route and select nearby
positive and malformed-authority coverage before changing code.

Actions:

- Reproduce `ieee/pr50310.c` narrowly and trace the returned `fneg` operand
  from floating `UnaryOp::Minus` lowering to the PHI incoming construction.
- Confirm the `fresh_tmp(ctx)` result lacks the native ID required by the
  existing current-function verifier and that this is distinct from 804/806.
- Identify a nearby same-family positive check and a malformed-authority check
  that exercise the existing verifier contract.

Completion check: the exact producer handoff and focused coverage targets are
recorded without expanding to another unary or residual producer family.

### Step 2 - Publish native authority for the `fneg` result

Goal: make the floating-minus producer return a checked current-function
`LirValueId` while preserving existing verifier behavior.

Actions:

- Apply the smallest change at the traced floating `UnaryOp::Minus` lowering
  handoff.
- Add nearby same-family positive and malformed-authority coverage.
- Keep missing, unknown, foreign, and stale authority rejected; do not change
  PHI schema, verifier, ternary lowering, or text identity behavior.

Completion check: valid floating-minus PHI input carries native authority and
the focused positive plus malformed cases pass.

### Step 3 - Prove the focused successor and hand off the parent gate

Goal: provide accepted focused proof for 807 and preserve the remaining
full-baseline sequence.

Actions:

- Obtain a fresh build and focused same-feature proof.
- Have the supervisor accept the focused evidence and record any remaining
  808/full-baseline dependency without claiming 806 or 804 clearance.

Completion check: 807 has accepted focused proof; 806 remains parked at Step
3 until 808 and the required follow-on full baseline are resolved.
