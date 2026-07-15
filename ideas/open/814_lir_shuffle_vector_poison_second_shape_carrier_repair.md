# LIR Shuffle-Vector Poison Second-Shape Carrier Repair

Status: Open
Type: bounded native-vector carrier/lowering blocker for 754 Step 9
Predecessor: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`

## Goal

Diagnose and repair the existing `LirShuffleVectorOp` splat lowering so its native vector authority carrier always supplies the required `second_vector_shape` fact when the second operand is poison, while preserving fail-closed validation for malformed carriers.

## Why This Exists

754 Step 9's fresh full checkpoint completed 3037/3038 and failed only at `llvm_gcc_c_torture_src_scal_to_vec1_c` with `LirShuffleVectorOp.native_vector_authority.second_vector_shape: must be present`. The rejected Step 9 packet selected scalar-to-vector splat `LirInsertElementOp`, but this failure comes from the pre-existing, unselected shuffle splat lowering: it emits a native carrier without a second-vector shape when its second operand is poison. The missing fact prevents 754 from obtaining its mandatory 100% full baseline and is outside that source's selected-row contract.

## In Scope

- Trace the existing `LirShuffleVectorOp` splat lowering and its native carrier construction for a poison second operand.
- Repair only the missing `second_vector_shape` carrier fact and structured poison-second-operand treatment needed to make the existing lowering coherent.
- Preserve fail-closed verifier/lowering behavior and add nearby valid plus malformed carrier/lowering coverage for this fact.
- Prove the bounded repair with a fresh build, relevant focused proof, matching regression guard, and a supervisor-owned full-baseline decision.

## Out Of Scope

- Selecting, implementing, or claiming any 754 vector-row capability, including `LirShuffleVectorOp` operation semantics.
- Aggregate work; `LirExtractElementOp` or `LirInsertElementOp` row contracts; generic provenance; CFG/PHI; target/MIR/emission; or parse/display text.
- Reopening 811 carrier publication or inferring carrier facts from rendered text, `%t` spelling, instruction order, or testcase names.

## Acceptance Criteria

- The existing shuffle splat lowering represents a poison second operand with a coherent native `second_vector_shape` fact required by its carrier.
- Nearby valid forms lower successfully, and absent, foreign, malformed, or incoherent second-shape/poison carrier forms reject fail closed.
- The repair has accepted narrow evidence and the supervisor accepts a fresh 100% full baseline before returning to 754.
- The result records only this prerequisite handoff and makes no claim that a 754 vector row is selected or complete.

## Parent Return Contract

After this blocker has an accepted narrow repair and the supervisor accepts a 100% fresh full baseline, reactivate `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` at its unchanged Step 9, *Implement and prove the Step 8 selection*. Reuse Step 9's completed audit, but make a fresh row selection/proof decision. Do not repeat accepted Steps 1--8 and do not silently implement `LirExtractElementOp` or `LirShuffleVectorOp` as a 754 row.

## Reviewer Reject Signals

- Reject a shuffle row-capability claim, mask-semantics implementation, or 754 row selection disguised as this carrier/lowering repair.
- Reject treating a poison second operand as an unstructured display token or recovering its shape from rendered text.
- Reject carriers that retain the missing-second-shape failure under renamed fields, raw-string fallbacks, or weaker validation.
- Reject expectation downgrades, named-case-only fixes, helper renames, or classification-only changes claimed as capability progress.
- Reject broad rewrites into aggregate, ExtractElement, InsertElement, provenance, CFG/PHI, target/MIR/emission, or unrelated tests.
