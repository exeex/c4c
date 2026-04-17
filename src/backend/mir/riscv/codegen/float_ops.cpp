// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs
// The bounded non-F128 float binop seam now compiles against the shared RV64
// codegen surface while the broader floating-point owner work stays parked.

#include "riscv_codegen.hpp"
#include "../../f128_softfloat.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::riscv::codegen {

namespace {

enum class FloatTyKind {
  F32,
  F64,
};

constexpr std::string_view riscv_float_binop_mnemonic(FloatOp op) {
  switch (op) {
    case FloatOp::Add: return "fadd";
    case FloatOp::Sub: return "fsub";
    case FloatOp::Mul: return "fmul";
    case FloatOp::Div: return "fdiv";
  }
  return "fadd";
}

constexpr std::string_view riscv_float_suffix(FloatTyKind ty) {
  return ty == FloatTyKind::F64 ? "d" : "s";
}

std::vector<std::string> riscv_float_binop_body(std::string_view mnemonic, FloatTyKind ty) {
  std::vector<std::string> lines;
  const auto suffix = riscv_float_suffix(ty);
  lines.emplace_back("    mv t2, t0");
  if (ty == FloatTyKind::F64) {
    lines.emplace_back("    fmv.d.x ft0, t1");
    lines.emplace_back("    fmv.d.x ft1, t2");
    lines.emplace_back(std::string("    ") + mnemonic.data() + "." + std::string(suffix) +
                       " ft0, ft0, ft1");
    lines.emplace_back("    fmv.x.d t0, ft0");
  } else {
    lines.emplace_back("    fmv.w.x ft0, t1");
    lines.emplace_back("    fmv.w.x ft1, t2");
    lines.emplace_back(std::string("    ") + mnemonic.data() + "." + std::string(suffix) +
                       " ft0, ft0, ft1");
    lines.emplace_back("    fmv.x.w t0, ft0");
  }
  return lines;
}

}  // namespace

void RiscvCodegen::emit_float_binop_impl(const Value& dest,
                                         FloatOp op,
                                         const Operand& lhs,
                                         const Operand& rhs,
                                         IrType ty) {
  if (ty == IrType::F128) {
    c4c::backend::emit_riscv_f128_binop(*this, dest, op, lhs, rhs);
    return;
  }

  const auto float_ty = ty == IrType::F64 ? FloatTyKind::F64 : FloatTyKind::F32;
  const auto mnemonic = riscv_float_binop_mnemonic(op);

  operand_to_t0(lhs);
  state.emit("    mv t1, t0");
  operand_to_t0(rhs);

  for (const auto& line : riscv_float_binop_body(mnemonic, float_ty)) {
    state.emit(line);
  }

  store_t0_to(dest);
}

void RiscvCodegen::emit_f128_neg_impl(const Value& dest, const Operand& src) {
  emit_f128_neg_full(dest, src);
}

}  // namespace c4c::backend::riscv::codegen
