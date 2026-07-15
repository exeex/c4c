# LIR PHI Scalar Bit-Not `xor` Authority Runbook

Status: Active
Source Idea: ideas/open/808_lir_phi_scalar_bit_not_xor_authority.md
Activated from: 806 Step 3 full-baseline gate; 806 remains parked until this
separately scoped producer family and the required follow-on full baseline are
accepted.

## Purpose

Repair the scalar `UnaryOp::BitNot` / `xor` result-authority handoff evidenced
by `pr68376-2.c`, without changing PHI or ternary consumer semantics.

## Core Rule

Publish a checked native current-function `LirValueId` at the scalar bit-not
producer. Keep the existing PHI verifier contract; do not derive authority
from display text or shift the repair to a PHI or ternary consumer.

## Read First

- `ideas/open/808_lir_phi_scalar_bit_not_xor_authority.md`
- `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`
- `src/codegen/lir/hir_to_lir/expr/misc.cpp` scalar `UnaryOp::BitNot` branch
- `review/806_step1_phi_producer_trace.md`

## Non-Goals

- Postfix old-value authority, scalar or floating unary-minus, other unary
  operators, generic provenance, and PHI/CFG semantics.
- Rendered `%t` recovery, testcase-specific behavior, or expectation
  downgrades.

## Ordered Steps

### Step 1 - Reconfirm the bounded scalar bit-not handoff and coverage

Goal: verify the `xor` producer-to-PHI authority route and select nearby
positive and malformed-authority coverage before changing code.

Actions:

- Reproduce `pr68376-2.c` narrowly and trace the scalar `UnaryOp::BitNot`
  result to the PHI incoming construction.
- Confirm the producer result lacks the native ID required by the existing
  current-function verifier and is distinct from 804, 806, and 807 routes.
- Identify same-family positive and malformed-authority coverage.

Completion check: the exact `xor` producer handoff and focused coverage are
recorded without broadening to another unary family or consumer contract.

### Step 2 - Publish native authority for the scalar `xor` result

Goal: make the scalar bit-not producer return a checked current-function
`LirValueId` while preserving existing verifier behavior.

Actions:

- Apply the smallest change at the traced scalar `UnaryOp::BitNot` lowering
  handoff.
- Add nearby same-family positive and malformed-authority coverage.
- Keep missing, unknown, foreign, and stale authority rejected; do not change
  PHI schema, verifier, ternary lowering, or text identity behavior.

Completion check: valid scalar-bit-not PHI input carries native authority and
the focused positive plus malformed cases pass.

### Step 3 - Prove the focused successor and hand off the parent gate

Goal: provide accepted focused proof for 808 and preserve the parent baseline
sequence.

Actions:

- Obtain a fresh build and focused same-feature proof.
- Have the supervisor accept the focused evidence, then resume 806 at Step 3
  for the required 100% full baseline; do not claim 806 or 804 clearance here.

Completion check: 808 has accepted focused proof and 806's exact full-baseline
return point remains explicit.
