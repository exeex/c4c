# Aggregate Canonicalization Pass

Status: scaffold.

Input: canonical scalar, SSA, and memory semantics. Output: canonical aggregate
value, copy, layout-independent access, and semantic GEP forms.

The pass must distinguish language-visible aggregate semantics from target data
layout and ABI realization. Any target-dependent decomposition belongs to
preparation or MIR.

Legacy coverage to review: aggregate lowering, by-value values and copies,
arrays, structs, unions, complex values, local-array semantic GEP, and aggregate
call/return handoff.
