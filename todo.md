# Current Packet

Status: Active
Source Idea Path: ideas/open/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair and cover bounded mask-lane coherence

## Just Finished

Step 2's verifier tightening and nearby fixture coverage were accepted in
`0d8e74852`: the zero-initializer contract now rejects selected lanes, nonzero
inactive payloads, and invalid enum values using structured fields only. A
fresh build and the focused backend command passed (6/6), with a non-decreasing
matching regression guard.

The supervisor's subsequent full command, `cmake --build --preset default &&
ctest --test-dir build -j --output-on-failure`, did not meet acceptance: it
reached 3038 tests but failed actual scalar-to-vector lowering at
`llvm_gcc_c_torture_src_scal_to_vec1_c` with
`LirShuffleVectorOp.native_vector_authority.mask_lanes: must mirror the
structured shuffle mask`. The unit fixture was therefore not representative
of the emitted carrier. Step 2 remains active; no full-baseline success is
claimed.

## Suggested Next

Execute repaired plan Step 2 only: inspect the actual emitted structured
mask-lane facts in the scalar-to-vector splat lowering that fails the full
baseline, then implement only the valid zero-mask publication/coherence fix.
Keep the existing fail-closed malformed-form checks and add representative
nearby coverage before rerunning the bounded proof.

## Watchouts

Do not assume the lowerings publish the valid default inactive lanes merely
because the fixture does. Inspect the emitted structured carrier, not display
text. This repair does not parse text, select shuffle semantics, claim a 754
row, or change vector/second-shape facts, poison, aggregate, extract/insert,
provenance, CFG/PHI, target/MIR, or emission behavior.

## Proof

Accepted narrow evidence from `0d8e74852`: fresh
`cmake --build --preset default`, focused backend 6/6, and a non-decreasing
matching regression guard. It is insufficient for the actual lowering route.
After the repaired Step 2 packet, rerun a fresh build and representative
same-feature proof; return readiness still requires the supervisor-owned
matching regression comparison and fresh 100% baseline decision.
