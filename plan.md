# LIR PHI Scalar Dereference-Load Authority Runbook

Status: Active
Source Idea: ideas/open/809_lir_phi_scalar_dereference_load_authority.md
Activated from: 806 Step 4 after its completed trace established an out-of-scope producer handoff.

## Purpose

Repair and prove the scalar dereference-load authority handoff, then return
the parent chain only through its required full-baseline gate.

## Core Rule

Preserve a native checked current-function `LirValueId`; do not reconstruct
identity from temporary text or broaden the repair beyond the scalar
`UnaryOp::Deref` load-result handoff.

## Read First

- `ideas/open/809_lir_phi_scalar_dereference_load_authority.md`
- `ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`
- accepted postfix commit `961ce9fda`
- the scalar `UnaryOp::Deref` lowering and existing conditional-PHI authority path

## Non-Goals

- Postfix, `fneg`, `xor`, scalar unary-minus, CFG, PHI verifier, generic
  conditional lowering, or generic provenance changes.
- Full-baseline clearance without supervisor acceptance.

## Ordered Steps

### Step 1 - Repair and focus-proof the scalar dereference-load handoff

Goal: carry the immediate scalar dereference `LirLoadOp` result's checked
native authority into the existing conditional PHI path.

Actions:

- Inspect the traced `UnaryOp::Deref` load lowering and make the smallest
  bounded handoff repair.
- Add nearby same-family positive and malformed/foreign, stale, or unknown
  authority coverage without weakening the existing PHI contract.
- Run a fresh build, the new nearby coverage, and focused
  `llvm_gcc_c_torture_src_20060910_1_c` proof.

Completion check: focused proof demonstrates the repaired native handoff and
the malformed-authority contract still rejects invalid authority.

### Step 2 - Full-baseline gate and parent return

Goal: establish the only evidence that may release 806's parent gate.

Actions:

- After supervisor acceptance of Step 1, run the supervisor-selected fresh
  full baseline.
- Require a supervisor-accepted 100% result before recording the parent
  return.
- Return execution to 806 at its recorded full-baseline step, then its parent
  chain; do not repeat accepted producer-family work.

Completion check: accepted 100% full baseline is recorded and 806 can resume
at its exact return point; otherwise retain this active blocker route.
