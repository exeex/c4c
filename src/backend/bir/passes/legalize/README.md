# Legalize Pass

Status: scaffold.

Input: verified `RawBir`. Output: BIR using legal target-independent types,
opcode forms, constant forms, and explicit semantic operations.

This pass owns raw-form normalization only. It does not choose registers,
calling-convention locations, frame offsets, target opcodes, or instruction
encodings. Its postconditions are prerequisites for every later canonical pass.

Legacy coverage to review: `prealloc/legalize.cpp`, integer and floating-width
normalization, constants, casts, comparison forms, and raw intrinsic forms.

Open review: enumerate every permitted Raw-only form and define whether the pass
is idempotent after one successful run.
