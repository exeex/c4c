# Intrinsic Canonicalization Pass

Status: scaffold.

Input: canonical aggregate and memory BIR. Output: the final canonical BIR
profile accepted by `CanonicalBir` verification.

Owns target-independent intrinsic semantics and an opaque semantic inline-asm
operation. It must leave target constraints, register classes, clobber
realization, runtime-helper choice, and instruction selection to later stages.

Legacy coverage to review: `prealloc/intrinsics.*`, atomics that lower through
intrinsics, i128/f128 operations, inline-asm semantic payload, and unsupported
intrinsic diagnostics.
