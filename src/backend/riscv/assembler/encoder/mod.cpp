// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/mod.rs
// Structural mirror of the ref Rust source; implementation is deferred to the
// per-module encoder files that live beside this one.
//
// Rust module layout mirrored here:
// - mod base;
// - mod atomics;
// - mod system;
// - mod float;
// - mod pseudo;
// - mod compressed;
// - mod vector;
//
// Public surface mirrored from the Rust source:
// - EncodeResult / RelocType / Relocation
// - reg_num / freg_num / vreg_num
// - any_reg_num / is_int_reg / is_fp_reg
// - encode_r / encode_i / encode_s / encode_b / encode_u / encode_j
// - get_reg / get_freg / get_any_reg / get_vreg / get_imm / get_symbol / get_mem
// - parse_fence_bits / parse_rm
// - encode_instruction
//
// The Rust source also defines opcode constants and a large instruction dispatch
// table. That behavior is intentionally left for the file-by-file worker lanes.

namespace c4c::backend::riscv::assembler::encoder {

}  // namespace c4c::backend::riscv::assembler::encoder
