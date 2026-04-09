// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/float.rs
//
// This subtree does not yet have the shared C++ encoder surface materialized,
// so this file is kept as a source-level mirror of the Rust helpers rather
// than a fabricated standalone implementation.
//
// Rust behavior mirrored here:
// - encode_float_load: flw/fld-style loads, including symbol relocations
// - encode_float_store: fsw/fsd-style stores, including symbol relocations
// - encode_fp_arith / encode_fp_arith_d: binary FP arithmetic with optional rm
// - encode_fp_unary: unary FP ops with optional rm
// - encode_fp_sgnj: sign-injection and min/max style R-type encoders
// - encode_fp_cmp: FP compare producing integer results
// - encode_fclass: classify FP operand into integer register
// - encode_fcvt_int / encode_fcvt_from_int / encode_fcvt_fp: conversion helpers
// - encode_fmv_x_f / encode_fmv_f_x: cross-domain move helpers
// - encode_fma: fused multiply-add family
//
// The direct C++ implementation should live here once the shared encoder
// types are available in C++ form.

namespace c4c::backend::riscv::assembler::encoder {

// Intentionally empty for now: the actual encoder helpers need the shared
// C++ encoder interface that is not yet present in this subtree.

}  // namespace c4c::backend::riscv::assembler::encoder
