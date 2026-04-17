// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/pseudo.rs
//
// This slice keeps the standalone helpers concrete, but the bulk of the
// pseudo-instruction encoders still depend on the shared RISC-V C++ encoder
// surface that has not been materialized yet in this subtree.
//
// Concrete helpers translated here:
// - sign_extend_li
// - extract_modifier_symbol
//
// Rust surface mirrored below for future worker slices:
// - encode_li / encode_li_32bit / encode_li_immediate
// - encode_mv / encode_not / encode_neg / encode_negw / encode_sext_w
// - encode_seqz / encode_snez / encode_sltz / encode_sgtz
// - encode_beqz / encode_bnez / encode_blez / encode_bgez / encode_bltz
// - encode_bgtz / encode_bgt / encode_ble / encode_bgtu / encode_bleu
// - get_branch_target
// - encode_j_pseudo / encode_jr / encode_call / encode_tail / encode_jump
// - encode_la / encode_lla
// - encode_rdcsr / encode_csrr / encode_csrw / encode_csrs / encode_csrc
// - encode_fmv_s / encode_fmv_d / encode_fabs_s / encode_fabs_d
// - encode_fneg_s / encode_fneg_d
// - extract_modifier_symbol / parse_reloc_modifier

#include <cstdint>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::assembler::encoder {

/// Sign-extend a value from `bits` width to i64.
std::int64_t sign_extend_li(std::int64_t val, std::uint32_t bits) {
  const auto shift = static_cast<std::int64_t>(64u - bits);
  return (val << shift) >> shift;
}

/// Extract the payload from `%modifier(symbol)` style expressions.
std::string extract_modifier_symbol(std::string_view text) {
  const auto begin = text.find('(');
  if (begin != std::string_view::npos) {
    const auto end = text.rfind(')');
    if (end != std::string_view::npos && end > begin) {
      return std::string(text.substr(begin + 1, end - begin - 1));
    }
  }
  return std::string(text);
}

// The remaining functions are intentionally left as a source-level mirror.
// They need the shared RISC-V encoder types and opcode constants that are not
// yet available as C++ declarations in this subtree.

}  // namespace c4c::backend::riscv::assembler::encoder
