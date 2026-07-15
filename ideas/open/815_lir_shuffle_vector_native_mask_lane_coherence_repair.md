# LIR Shuffle-Vector Native Mask-Lane Coherence Repair

Status: Open
Type: bounded native shuffle-carrier blocker for 814 Step 3
Predecessor: `ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`

## Goal

Repair publication and coherence of the existing `LirShuffleVectorOp` native
`mask_lanes` facts and mask display mirror for the scalar-to-vector splat
lowerings that use a zero-initializer shuffle mask.

## Why This Exists

After 814's accepted poison second-shape repair `c1cde8430`, the supervisor's
fresh full checkpoint completed 3035/3038. Its only failures are
`llvm_gcc_c_torture_src_pr60960_c`, `llvm_gcc_c_torture_src_scal_to_vec1_c`,
and `llvm_gcc_c_torture_src_scal_to_vec2_c`, each reporting
`LirShuffleVectorOp.native_vector_authority.mask_lanes: must mirror the
structured shuffle mask`. The failed contract is mask-lane publication and
display coherence for an existing splat lowering, outside 814's exact
second-shape/poison handoff scope.

## In Scope

- Trace the existing scalar-to-vector splat `LirShuffleVectorOp` lowering that
  carries the zero-initializer shuffle mask.
- Publish the structured native `mask_lanes` facts required by that existing
  carrier and keep its mask display mirror coherent.
- Add nearby valid and malformed coverage for missing, incoherent, or
  non-mirroring mask-lane evidence.
- Prove the bounded repair with a fresh build, focused same-feature proof,
  matching regression guard, and a supervisor-accepted fresh 100% full
  baseline before returning to 814.

## Out Of Scope

- Shuffle row-semantic selection or implementation, including any 754 row
  claim, lane-selection semantics, or text parsing.
- Changes to vector/second-shape facts, poison handoff, aggregate work,
  ExtractElement, InsertElement, generic provenance, CFG/PHI, target/MIR,
  emission, or unrelated lowerings.
- Recovering mask facts from rendered text, instruction order, `%t` spelling,
  or testcase names.

## Acceptance Criteria

- The existing zero-initializer splat mask publishes coherent native
  `mask_lanes` and a matching mask display mirror through the
  `LirShuffleVectorOp` carrier.
- Nearby valid forms lower successfully; missing, incoherent, malformed, or
  non-mirroring mask-lane evidence rejects fail closed.
- Fresh narrow proof and a matching regression guard are accepted, followed by
  a supervisor-accepted fresh 100% full baseline.
- The result records only this mask-lane prerequisite and makes no shuffle-row
  semantic or 754 capability claim.

## Parent Return Contract

After this blocker has an accepted narrow repair and the supervisor accepts a
fresh 100% full baseline, reactivate
`ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md` at
unchanged Step 3, `Prove the blocker handoff and return decision`. Do not
repeat 814 Steps 1--2; use its accepted `c1cde8430` second-shape/poison
handoff, then make the preserved parent return decision.

## Reviewer Reject Signals

- Reject a shuffle row-semantic claim, mask-selection implementation, or 754
  row selection disguised as mask-lane publication/coherence.
- Reject mask facts recovered from display text, rendered LLVM, instruction
  order, `%t` spelling, or testcase names.
- Reject retaining the non-mirroring mask-lane failure behind renamed fields,
  raw-string fallbacks, or weaker validation.
- Reject expectation downgrades, named-case-only fixes, helper renames, or
  classification-only changes claimed as capability progress.
- Reject broad rewrites into vectors/second shape, aggregate, extract/insert,
  provenance, CFG/PHI, target/MIR/emission, or unrelated tests.
