# Current Packet

Status: Active
Source Idea Path: ideas/open/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair and cover bounded mask-lane coherence

## Just Finished

Plan Step 1 complete. The two scalar-to-vector splat emitters in
`src/codegen/lir/hir_to_lir/expr/binary.cpp` construct `%ins = insertelement`
then `LirShuffleVectorOp{%shuf, vec_ty, %ins, poison, <lanes x i32>,
zeroinitializer, authority}`. Each authority supplies the result/first/second
vector shapes and `mask_lanes` as `vector<LirShuffleMaskLane>(lanes)`, whose
default entries are native `Inactive, 0`; no fact is recovered from display
text. `LirShuffleVectorOp` publishes that carrier unchanged, while
`lir_printer.cpp` prints only the display mirror. `verify.cpp` first checks the
ordinary carrier, then requires lane count == result lanes, `<lanes x i32>`,
and the `ZeroInitializer` mask token.

Matrix (all is relative to this zero-initializer splat only): valid native
`lanes` default `Inactive,0` plus the matching token/type passes; missing or
wrong-count lanes rejects; wrong mask type or non-`zeroinitializer` display
token rejects. The remaining native seam is fail-open: a `Selected` lane,
nonzero `selected_lane` on an inactive entry, or an invalid `Kind` enum value
is not inspected, so it can contradict the zero mask and still pass. Step 2
must make that structured-lane/display coherence reject fail closed, with
nearby valid and malformed coverage; it must not add mask selection or recover
facts from the printed instruction.

## Suggested Next

Execute plan Step 2 only: tighten the shuffle verifier's zero-initializer
structured-lane coherence check and add nearby valid/malformed carrier tests.

## Watchouts

The lowerings already publish default inactive lanes; the gap is verifier
inspection of lane payloads, not carrier construction or printing. Keep the
zero-mask contract exact: reject selected/nonzero/invalid structured entries;
do not parse text, select shuffle semantics, claim a 754 row, or change vector,
second-shape, poison, aggregate, extract/insert, provenance, CFG/PHI,
target/MIR, or emission behavior.

## Proof

No proof command was delegated for this read-only diagnostic packet; no build
or tests ran and no `test_after.log` was created. Step 2 requires the
supervisor-selected build and focused same-feature proof; return readiness also
requires matching regression guard and a supervisor-owned fresh 100% baseline.
