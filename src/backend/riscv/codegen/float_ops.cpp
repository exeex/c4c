// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs
// The shared RISC-V C++ codegen surface does not exist yet, so this file keeps
// the Rust method boundaries as a source-level mirror while still translating
// the local float mnemonic/body selection logic into concrete C++ helpers.

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::riscv::codegen {

namespace {

enum class FloatOpKind {
  Add,
  Sub,
  Mul,
  Div,
};

enum class FloatTyKind {
  F32,
  F64,
};

constexpr std::string_view riscv_float_binop_mnemonic(FloatOpKind op) {
  switch (op) {
    case FloatOpKind::Add: return "fadd";
    case FloatOpKind::Sub: return "fsub";
    case FloatOpKind::Mul: return "fmul";
    case FloatOpKind::Div: return "fdiv";
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

// Source-level mirrors of the Rust impl methods:
//
// pub(super) fn emit_float_binop_impl(&mut self, dest: &Value, op: FloatOp,
//                                     lhs: &Operand, rhs: &Operand, ty: IrType)
// pub(super) fn emit_float_binop_body(&mut self, mnemonic: &str, ty: IrType)
// pub(super) fn emit_float_binop_mnemonic(&self, op: FloatOp) -> &'static str
// pub(super) fn emit_f128_neg_impl(&mut self, dest: &Value, src: &Operand)
//
// The method bodies still depend on the shared RiscvCodegen / Operand / Value
// surface, which is not translated into a common C++ header yet. The helpers
// above preserve the part that is local and mechanically translatable.

}  // namespace c4c::backend::riscv::codegen
