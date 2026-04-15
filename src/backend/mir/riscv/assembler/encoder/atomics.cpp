// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/atomics.rs
//
// This slice is a structural mirror of the Rust atomic encoder helpers.
// The shared C++ encoder surface for `Operand`, `EncodeResult`, `encode_r`,
// `get_reg`, and `get_mem` is not yet materialized in this subtree, so the
// translation is kept as a faithful source-level reference rather than a
// fabricated standalone implementation.
//
// Rust behavior mirrored here:
// - `encode_lr`: encode LR.W / LR.D with aq=0, rl=0
// - `encode_sc`: encode SC.W / SC.D with aq=0, rl=0
// - `encode_amo`: encode AMO ops with funct5 in the top 5 bits of funct7
// - `encode_lr_suffixed`: parse width suffixes and .aq/.rl/.aqrl modifiers
// - `encode_sc_suffixed`: same suffix handling for SC
// - `encode_amo_suffixed`: parse amoswap/amoadd/... plus width and ordering
// - `parse_aq_rl`: fold suffixes into aq/rl bits
//
// The direct C++ implementation should live here once the shared encoder types
// are available in C++ form.

namespace c4c::backend::riscv::assembler::encoder {

// Intentionally empty for now: the actual encoder helpers need the shared
// C++ encoder interface that is not yet present in this subtree.

}  // namespace c4c::backend::riscv::assembler::encoder
