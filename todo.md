# Current Packet

Status: Active
Source Idea Path: ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the blocker handoff and return decision

## Just Finished

Plan Step 2 completed: both scalar-to-vector splat `LirShuffleVectorOp` constructions now pass poison as `vec2` with no `second_vector_use` and the local native `shape` recorded as `second_vector_shape`. Nearby verifier coverage accepts that structured poison form and rejects missing, incoherent, and extraneous second-use carrier evidence without display-text inference.

## Suggested Next

Execute plan Step 3 only: prove the blocker handoff and make the return decision using the preserved 754 Step 9 return point.

## Watchouts

The focused matrix remains fail-closed: valid poison has no `second_vector_use` and a coherent native second shape; malformed absence, incoherent shape, and extraneous/unknown carrier evidence reject. This is only a native carrier/lowering prerequisite: do not implement shuffle semantics, select a 754 row, infer from display text, or widen into aggregate, ExtractElement, InsertElement, provenance, CFG/PHI, target/MIR, or emission.

## Proof

Narrow proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`. The incoming 754 full checkpoint was 3037/3038 with one failure, `llvm_gcc_c_torture_src_scal_to_vec1_c`, and is not an accepted baseline.
