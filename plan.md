# LIR Ternary PHI-Incoming Operand-Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/809_lir_ternary_phi_incoming_operand_authority_handoff.md
Activated from: 807 Step 2 blocker; 807 is parked for resumption after this
returned-operand authority handoff is resolved.

## Purpose

Repair the narrow ternary lowering seam that drops a returned operand's native
value authority while constructing a PHI incoming.

## Core Rule

Retain/pass the returned `LirOperand` native ID at the ternary PHI-incoming
construction seam. Keep producer creation and the existing PHI verifier
contract unchanged.

## Read First

- `ideas/open/809_lir_ternary_phi_incoming_operand_authority_handoff.md`
- `ideas/open/807_lir_phi_floating_unary_minus_fneg_authority.md`
- ternary lowering and its `LirPhiOp::incoming` construction
- `src/codegen/lir/hir_to_lir/expr/misc.cpp` only as the parked 807 producer
  evidence; do not edit it for this blocker

## Non-Goals

- `fneg` or any other producer creation, unary-family work, postfix, scalar
  bit-not, PHI schema/verifier changes, CFG/edge semantics, Raw-BIR, or text
  identity recovery.

## Ordered Steps

### Step 1 - Trace the returned-operand to PHI-incoming construction seam

Goal: locate the exact ternary lowering construction that recreates `%t6`
without the returned operand's native ID and choose nearby preservation and
rejection coverage.

Actions:

- Reproduce `condition ? -input : 0.0` using the existing unaccepted 807
  producer hunk as evidence, without accepting or expanding that hunk.
- Trace the returned `LirOperand` through ternary lowering into
  `LirPhiOp::incoming` and identify the field/constructor boundary where
  `value_id` is omitted.
- Identify nearest coverage that proves native-ID preservation and retains
  missing, unknown, foreign, and stale rejection under the existing verifier.

Completion check: the single incoming-construction seam and focused coverage
targets are recorded, with no producer or verifier change proposed.

### Step 2 - Retain returned operand authority in ternary PHI construction

Goal: make the smallest construction-side change that preserves native
authority on the PHI incoming.

Actions:

- Pass/retain the returned `LirOperand` ID when constructing the ternary PHI
  incoming.
- Add only nearby preservation and rejection coverage required for this seam.
- Do not edit producer creation, PHI schema, or verification rules.

Completion check: valid returned operands retain their IDs at the incoming and
all existing malformed-authority rejection categories remain rejected.

### Step 3 - Prove the blocker and return to 807

Goal: obtain accepted focused proof and make 807's exact continuation clear.

Actions:

- Run the fresh build and focused same-feature proof selected by the
  supervisor.
- Record accepted blocker evidence and return control to 807 Step 2 for its
  parked `fneg` hunk and producer-family coverage.

Completion check: blocker proof is accepted and 807 can resume at its recorded
Step 2 return point without reconstructing this diagnosis.
