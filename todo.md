# Current Packet

Status: Active
Source Idea Path: ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair and cover the bounded carrier/lowering seam

## Just Finished

Plan Step 1 completed: both scalar-to-vector splat `LirShuffleVectorOp` constructions in `src/codegen/lir/hir_to_lir/expr/binary.cpp` pass poison as `vec2` and omit only `native_vector_authority.second_vector_shape`. The exact structured valid form keeps that field present as the same known local vector `shape` used for `result_shape` and `first_vector_shape`; poison has no `second_vector_use`. `LirNativeVectorAuthority` owns this native fact, and the verifier requires it for every shuffle second operand—without display-text inference.

## Suggested Next

Execute plan Step 2 only: populate the known `shape` as `second_vector_shape` at the two splat shuffle constructions and add nearby coverage for the valid structured poison form and fail-closed malformed forms.

## Watchouts

Focused Step 1 matrix: valid = poison `vec2`, no `second_vector_use`, and a coherent `second_vector_shape` equal to the native result/first shape; invalid = absent second shape; foreign, unknown, or incoherent second shape or owner/use; malformed poison representation. This is only a native carrier/lowering prerequisite: do not implement shuffle semantics, select a 754 row, infer from display text, or widen into aggregate, ExtractElement, InsertElement, provenance, CFG/PHI, target/MIR, or emission.

## Proof

No proof required or run for this documentation-only Step 1 audit. The incoming 754 full checkpoint was 3037/3038 with one failure, `llvm_gcc_c_torture_src_scal_to_vec1_c`, and is not an accepted baseline.
