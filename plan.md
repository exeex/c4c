# LIR Shuffle-Vector Native Mask-Lane Coherence Repair Runbook

Status: Active
Source Idea: ideas/open/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md
Switched from: ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md Step 3

## Purpose

Restore the existing scalar-to-vector splat shuffle lowering's native
mask-lane publication and structured-mask/display coherence so the full
baseline can run without an unrelated carrier-validation failure.

## Core Rule

Repair only the existing zero-initializer splat mask carrier. Do not select or
implement shuffle row semantics, make a 754 claim, or recover facts from text.

## Read First

- `ideas/open/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md`
- `ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`
- `ideas/closed/811_lir_native_vector_authority_carrier_publication.md`
- Existing scalar-to-vector splat lowering, `LirShuffleVectorOp` carrier, and
  verifier seams named by the full-baseline failure.

## Non-Goals

- Do not change vector/second-shape or poison handoff facts, aggregate,
  ExtractElement, InsertElement, generic provenance, CFG/PHI, target/MIR,
  emission, or parse/display-text behavior.
- Do not implement mask/lane semantic selection beyond publication and
  coherence of the existing structured splat mask.

## Steps

### Step 1 - Diagnose the splat mask-lane carrier seam

Goal: identify the existing zero-initializer splat mask construction that
fails to publish coherent native `mask_lanes` and a matching display mirror.

Actions:

- trace the splat lowering, structured shuffle mask, carrier construction, and
  verifier requirement;
- record valid, missing, incoherent, malformed, and non-mirroring cases in
  `todo.md`;
- confirm the route introduces neither text recovery nor shuffle semantics.

Completion check: one native lowering/carrier seam and a focused fail-closed
matrix are documented for implementation.

### Step 2 - Repair and cover bounded mask-lane coherence

Goal: implement only the Step 1 mask-lane publication/mirror contract and add
nearby coverage.

Completion check: the existing splat mask supplies coherent native
`mask_lanes` and matching display evidence; malformed forms reject; no
out-of-scope route changes.

### Step 3 - Prove the blocker handoff and return decision

Goal: obtain bounded proof and make the parent return decision.

Actions:

- run a fresh build, focused same-feature proof, and matching regression
  guard;
- require supervisor-owned fresh 100% full-baseline acceptance before
  declaring this blocker return-ready;
- return to 814 Step 3 only with the exact preserved return point.

Completion check: accepted narrow proof and supervisor acceptance of a 100%
full baseline are recorded, or every remaining failure has its own explicit
lifecycle route.
