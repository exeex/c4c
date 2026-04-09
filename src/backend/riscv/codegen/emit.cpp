// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/emit.rs
// The Rust source owns the RISC-V codegen core.  The shared C++ surface for
// `RiscvCodegen` / `CodegenState` does not exist yet, so this translation keeps
// the reusable register helpers concrete and leaves the large method surface as
// a source-level mirror for the sibling slices.

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace c4c::backend::riscv::codegen {

struct PhysReg {
  std::uint32_t value = 0;

  constexpr PhysReg() = default;
  constexpr explicit PhysReg(std::uint32_t v) : value(v) {}
};

constexpr std::array<PhysReg, 6> RISCV_CALLEE_SAVED = {
    PhysReg(1), PhysReg(7), PhysReg(8), PhysReg(9), PhysReg(10), PhysReg(11),
};

constexpr std::array<PhysReg, 5> CALL_TEMP_CALLEE_SAVED = {
    PhysReg(2), PhysReg(3), PhysReg(4), PhysReg(5), PhysReg(6),
};

constexpr std::array<const char*, 8> RISCV_ARG_REGS = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
};

const char* callee_saved_name(PhysReg reg) {
  switch (reg.value) {
    case 1: return "s1";
    case 2: return "s2";
    case 3: return "s3";
    case 4: return "s4";
    case 5: return "s5";
    case 6: return "s6";
    case 7: return "s7";
    case 8: return "s8";
    case 9: return "s9";
    case 10: return "s10";
    case 11: return "s11";
    default:
      throw std::invalid_argument("invalid RISC-V callee-saved register index");
  }
}

std::optional<PhysReg> riscv_reg_to_callee_saved(std::string_view name) {
  if (name == "s1" || name == "x9") return PhysReg(1);
  if (name == "s2" || name == "x18") return PhysReg(2);
  if (name == "s3" || name == "x19") return PhysReg(3);
  if (name == "s4" || name == "x20") return PhysReg(4);
  if (name == "s5" || name == "x21") return PhysReg(5);
  if (name == "s6" || name == "x22") return PhysReg(6);
  if (name == "s7" || name == "x23") return PhysReg(7);
  if (name == "s8" || name == "x24") return PhysReg(8);
  if (name == "s9" || name == "x25") return PhysReg(9);
  if (name == "s10" || name == "x26") return PhysReg(10);
  if (name == "s11" || name == "x27") return PhysReg(11);
  return std::nullopt;
}

std::optional<PhysReg> constraint_to_callee_saved_riscv(std::string_view constraint) {
  if (!constraint.empty() && constraint.front() == '{' && constraint.back() == '}') {
    return riscv_reg_to_callee_saved(constraint.substr(1, constraint.size() - 2));
  }
  return riscv_reg_to_callee_saved(constraint);
}

// Source-level mirror of the rest of `emit.rs`.
//
// The following Rust-owned methods are translated in sibling slices and depend
// on the missing shared `RiscvCodegen` / `CodegenState` surface:
// - `RiscvCodegen::new`
// - option setters and pre-directive emission
// - comparison operand loading and stack-slot helpers
// - operand loading / storing
// - 128-bit helpers
// - the `ArchCodegen` trait implementation
//
// `collect_inline_asm_callee_saved_riscv` also lives here in Rust, but it
// depends on the shared IR and backend generation helpers that are not yet
// exposed in C++.

}  // namespace c4c::backend::riscv::codegen
