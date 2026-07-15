# LIR Shuffle-Vector Poison Second-Shape Carrier Repair Runbook

Status: Active
Source Idea: ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md
Switched from: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md Step 9

## Purpose

Restore the existing shuffle splat lowering's coherent native carrier for a poison second operand so the full baseline can run without an unrelated carrier-validation failure.

## Core Rule

Repair only the required native `second_vector_shape` fact and structured poison-second-operand handoff. Do not turn this prerequisite into a shuffle row-semantic capability or a 754 vector-row selection.

## Read First

- `ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/811_lir_native_vector_authority_carrier_publication.md`
- Existing `LirShuffleVectorOp` construction, verifier, and splat lowering seams named by the diagnosis.

## Non-Goals

- Do not alter aggregate, ExtractElement, InsertElement, generic provenance, CFG/PHI, target/MIR/emission, or parse/display-text behavior.
- Do not select or implement shuffle operation semantics for 754.

## Steps

### Step 1 - Diagnose the poison second-operand carrier handoff

Goal: identify the precise existing shuffle splat construction path that omits `second_vector_shape` for poison, and define the smallest structured representation that keeps the carrier coherent.

Actions:

- trace the splat lowering, carrier construction, and verifier requirement;
- record the valid poison-second-operand form plus absent, foreign, malformed, and incoherent forms in `todo.md`;
- confirm the repair neither adds row semantics nor derives facts from display text.

Completion check: a single native carrier/lowering repair seam and a focused valid/malformed matrix are documented for executor implementation.

### Step 2 - Repair and cover the bounded carrier/lowering seam

Goal: implement only the Step 1 contract and add nearby coverage.

Completion check: the valid structured poison form supplies a coherent `second_vector_shape`; malformed carrier/lowering forms fail closed; no out-of-scope route changes.

### Step 3 - Prove the blocker handoff and return decision

Goal: obtain the bounded proof and make the prerequisite handoff decision.

Actions:

- run a fresh build, focused same-feature proof, and matching regression guard;
- require supervisor-owned fresh full-baseline acceptance before declaring the blocker return-ready;
- return to 754 Step 9 only with the source's exact preserved return point.

Completion check: accepted narrow proof and supervisor acceptance of a 100% full baseline are recorded, or the remaining failure has its own explicit lifecycle route.
