# LIR PHI Incoming Producer Authority Repair Runbook

Status: Active
Source Idea: ideas/open/804_lir_phi_incoming_producer_authority_repair.md
Activated from: 754 full-baseline blocker switch; resume 754 Step 2 only after
this bounded route and its full-baseline gate are accepted.

## Purpose

Repair the one PHI incoming authority handoff preventing the clean full
baseline, while preserving the accepted CFG/PHI authority contract.

## Core Rule

Native checked current-function IDs are authority. Do not use display text,
instruction order, or testcase identity, and do not weaken PHI verification.

## Read First

- `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- the accepted CFG/PHI authority history around `6ece9fe8f`
- the PHI lowering, producer-handoff, and verifier surfaces reached by the
  failing `llvm_gcc_c_torture_src_vrp_2_c` trace

## Non-Goals

- Reopening CFG/PHI schema, predecessor, edge, or verifier semantics.
- Aggregate/vector row work, Raw-BIR, generic provenance, text recovery, or
  residual-family conversion.

## Ordered Steps

### Step 1 - Trace the failing PHI incoming producer handoff

Goal: identify the exact native authority fact lost before the existing PHI
incoming verifier check.

Actions:

- Reproduce and narrow the supplied `vrp_2.c` diagnostic without broadening
  into a full-suite acceptance attempt.
- Trace its producer, immediate lowering handoff, and PHI construction against
  the accepted contract; distinguish missing, stale, and foreign authority.
- Record the bounded seam and reject any route requiring a CFG/PHI contract
  rewrite or text recovery.

Completion check: one concrete producer-to-PHI handoff seam and authority
failure mode are evidenced, or the route is returned for lifecycle repair.

### Step 2 - Repair only the selected producer-side handoff

Goal: publish the checked current-function ID required by the existing PHI
incoming contract.

Actions:

- Implement the smallest native producer/immediate-lowering handoff repair.
- Preserve existing PHI verifier, predecessor, and edge rules.
- Add nearby positive and malformed-authority coverage for this producer family.

Completion check: the selected incoming has valid authority; missing, unknown,
foreign, and stale forms reject; no generic conversion or text identity exists.

### Step 3 - Prove the blocker and return it to 754

Goal: produce accepted bounded evidence and restore the parent baseline gate.

Actions:

- Obtain a fresh build and focused same-feature proof.
- Have the supervisor run and accept the 100% full baseline before declaring
  the blocker clear.
- Record the accepted handoff, proof references, and 754 return point.

Completion check: the supervisor has accepted the full baseline and 754 can be
reactivated at unchanged Step 2; otherwise keep an executable repair route.
