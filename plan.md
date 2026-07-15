# LIR Shuffle-Vector Poison Second-Shape Carrier Repair Runbook

Status: Active
Source Idea: ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md
Resumed from: closed 815 native shuffle mask-lane coherence blocker at Step 3

## Purpose

Restore the existing shuffle splat lowering's coherent native carrier for a
poison second operand so the parent baseline handoff can proceed without an
unrelated carrier-validation failure.

## Core Rule

Retain the accepted `second_vector_shape` and structured poison-second-operand
handoff only. Do not turn this prerequisite into a shuffle row-semantic
capability or a 754 vector-row selection.

## Read First

- `ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`
- `ideas/closed/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md`
- `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/811_lir_native_vector_authority_carrier_publication.md`

## Non-Goals

- Do not repeat accepted Steps 1--2 or alter aggregate, ExtractElement,
  InsertElement, generic provenance, CFG/PHI, target/MIR/emission, or
  parse/display-text behavior.
- Do not select or implement shuffle operation semantics for 754.

## Accepted History

- Step 1 is accepted: the poison second-operand carrier seam and fail-closed
  matrix were diagnosed.
- Step 2 is accepted in `c1cde8430`: both scalar-to-vector splat
  `LirShuffleVectorOp` constructions publish the coherent native poison second
  shape with nearby valid and malformed coverage.
- Separate blocker 815 is closed in `510388751`; its 6/6 matching guard and
  supervisor-owned 3038/3038 full baseline acceptance clear the mask-lane
  coherence prerequisite.

## Steps

### Step 1 - Diagnose the poison second-operand carrier handoff

Status: Complete (accepted; do not repeat).

### Step 2 - Repair and cover the bounded carrier/lowering seam

Status: Complete (accepted in `c1cde8430`; do not repeat).

### Step 3 - Prove the blocker handoff and return decision

Goal: use the accepted second-shape handoff and closed 815 proof to make only
814's parent return decision.

Actions:

- retain the recorded fresh build, same-feature proof, matching guard, and
  supervisor-owned 3038/3038 baseline acceptance; do not rerun completed
  implementation steps;
- confirm the returned prerequisite still makes no shuffle-row semantic or 754
  capability claim;
- reactivate 754 only at its exact preserved Step 9 return point when this
  blocker handoff is accepted.

Completion check: 814's accepted prerequisite handoff is recorded and its
parent return decision has an explicit lifecycle route; no Steps 1--2 work is
repeated.
