#include "branch.hpp"

#include "returns.hpp"
#include "support.hpp"

namespace c4c::backend::aarch64 {

bool render_branch_instruction(std::ostream& out,
                               const c4c::codegen::lir::LirInst& inst) {
  const auto* cmp = std::get_if<c4c::codegen::lir::LirCmpOp>(&inst);
  if (!cmp) {
    return false;
  }

  if (cmp->is_float) {
    fail_unsupported("floating-point compare instructions");
  }

  out << "  " << cmp->result << " = icmp " << cmp->predicate << " "
      << cmp->type_str << " " << cmp->lhs << ", " << cmp->rhs << "\n";
  return true;
}

void render_terminator(std::ostream& out,
                       const c4c::codegen::lir::LirTerminator& terminator) {
  if (const auto* br = std::get_if<c4c::codegen::lir::LirBr>(&terminator)) {
    out << "  br label %" << br->target_label << "\n";
    return;
  }
  if (const auto* cond_br =
          std::get_if<c4c::codegen::lir::LirCondBr>(&terminator)) {
    out << "  br i1 " << cond_br->cond_name << ", label %"
        << cond_br->true_label << ", label %" << cond_br->false_label << "\n";
    return;
  }
  if (const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&terminator)) {
    render_return(out, *ret);
    return;
  }
  if (std::get_if<c4c::codegen::lir::LirUnreachable>(&terminator)) {
    out << "  unreachable\n";
    return;
  }
  if (std::get_if<c4c::codegen::lir::LirSwitch>(&terminator)) {
    fail_unsupported("switch terminators");
  }
  if (std::get_if<c4c::codegen::lir::LirIndirectBr>(&terminator)) {
    fail_unsupported("indirect branch terminators");
  }
  fail_unsupported("unknown terminators");
}

}  // namespace c4c::backend::aarch64
