# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Implement and prove the Step 8 selection

## Just Finished

811 completed the native vector authority carrier prerequisite. Its accepted
publication and coverage commits are `76f92ad60` and `56203cbf9`; the accepted
matching `^backend_` guard increased from 5 to 6 passing tests with zero
failures. This is a prerequisite handoff only, not a selected vector-row
implementation.

## Suggested Next

Before modifying code, rerun a fresh one-row audit of `LirInsertElementOp`,
`LirExtractElementOp`, and `LirShuffleVectorOp` against 811's structured
carrier. Record exactly one selected seam and its native positive/malformed
matrix here, or route a newly discovered out-of-scope prerequisite. Leave the
other two rows unselected.

## Watchouts

Do not repeat 754 Steps 1--8 or 811 publication. Do not infer any identity,
vector shape, index, or mask fact from rendered text. The carrier does not by
itself authorize a row; do not widen into aggregate, generic provenance,
CFG/PHI, pointers, Raw-BIR, target, MIR, or emission.

## Proof

Preserve 811's accepted fresh-build plus matching canonical
`ctest --test-dir build -j --output-on-failure -R '^backend_'` 5-to-6 passing
guard and zero failures, plus its closure-quality full CTest checkpoint at
3038/3038. Step 9 requires its own fresh build, selected same-feature proof,
and matching regression guard after a row is selected.
