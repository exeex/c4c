# Scalar Canonicalization Pass

Status: scaffold.

Input: legalized BIR. Output: canonical target-independent scalar operations.

Owns scalar operation normalization, comparison normalization, select semantics,
and target-independent integer/floating helper representation. It must preserve
source-level signedness, width, poison/undef, and exceptional semantics.

Legacy coverage to review: scalar lowering, `prealloc/comparison.*`, select
chains, i128/f128 semantic operations, casts, and producer/consumer relations.

Does not own runtime-helper selection or target instruction sequences.
