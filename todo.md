# Current Packet

Status: Active
Source Idea Path: ideas/open/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair and cover bounded mask-lane coherence

## Just Finished

Step 2 diagnostic action completed: the delegated LLVM-path command,
`./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/scal-to-vec1.c
-o /tmp/scal-to-vec1.ll`, reproduces the verifier failure before emission.
Both scalar-to-vector splat constructors in
`src/codegen/lir/hir_to_lir/expr/binary.cpp` (the vector-result and ordinary
binary paths) construct `mask_lanes` with `std::vector<LirShuffleMaskLane>(lanes)`.
That value-initializes every lane as `{kind = Inactive, selected_lane = 0}`.

The concrete mismatch is `kind`: LLVM `shufflevector ... zeroinitializer`
uses an all-zero integer mask, so each output lane selects first-vector lane
0; its coherent native value is `{kind = Selected, selected_lane = 0}`, not
an inactive lane. `0d8e74852` instead made the verifier require `Inactive, 0`,
so it rejects the actual structured carrier. The unit fixture shares that
incorrect default and is not representative of the semantic mask.

## Suggested Next

Implement the bounded Step 2 coherence repair at the existing scalar-splat
construction seam: publish one `{Selected, 0}` `LirShuffleMaskLane` per lane
in both duplicate splat emitters in `binary.cpp`, and make the zero-mask
verifier require that exact structured value. Update nearby coverage so the
valid fixture and malformed cases use the same selected-zero contract, with a
representative `scal-to-vec1.c` lowering-path test.

## Watchouts

Do not describe `zeroinitializer` as an inactive shuffle mask: it is the
display mirror for a vector of integer zero selection indices. Preserve
fail-closed rejection for wrong lane count, wrong mask type/token, inactive
lanes, nonzero selected payloads, and invalid enum values. This remains a
structured carrier repair only; it introduces no text recovery or shuffle-row
semantic selection.

## Proof

No build or test proof was delegated for this diagnostic-only packet, and no
canonical regression log was written. Reproduction/observation only:
`./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/scal-to-vec1.c
-o /tmp/scal-to-vec1.ll` failed with
`LirShuffleVectorOp.native_vector_authority.mask_lanes: must mirror the
structured shuffle mask`. The implementation packet must receive its exact
fresh-build and focused-proof command from the supervisor.
