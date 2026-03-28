#include "alu.hpp"

#include "support.hpp"

#include <array>

namespace c4c::backend::aarch64 {

bool render_alu_instruction(std::ostream& out,
                            const c4c::codegen::lir::LirInst& inst) {
  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    const char* opcode = nullptr;
    switch (cast->kind) {
      case c4c::codegen::lir::LirCastKind::ZExt:
        opcode = "zext";
        break;
      case c4c::codegen::lir::LirCastKind::SExt:
        opcode = "sext";
        break;
      case c4c::codegen::lir::LirCastKind::PtrToInt:
        opcode = "ptrtoint";
        break;
      default:
        fail_unsupported("cast kind outside the compare/branch or memory-addressing slice");
    }
    out << "  " << cast->result << " = " << opcode << " " << cast->from_type
        << " " << cast->operand << " to " << cast->to_type << "\n";
    return true;
  }

  const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst);
  if (!bin) {
    return false;
  }

  static constexpr std::array<const char*, 6> kSupportedOps{
      "add", "sub", "mul", "and", "or", "xor"};
  bool supported = false;
  for (const char* opcode : kSupportedOps) {
    if (bin->opcode == opcode) {
      supported = true;
      break;
    }
  }
  if (!supported) {
    fail_unsupported("binary opcode '" + bin->opcode + "'");
  }

  out << "  " << bin->result << " = " << bin->opcode << " " << bin->type_str
      << " " << bin->lhs << ", " << bin->rhs << "\n";
  return true;
}

}  // namespace c4c::backend::aarch64
