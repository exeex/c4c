#include "calls.hpp"

#include "support.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

bool render_call_instruction(std::ostream& out,
                             const c4c::codegen::lir::LirInst& inst) {
  const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
  if (!call) {
    return false;
  }

  if (!call->callee_type_suffix.empty()) {
    fail_unsupported("indirect or typed-suffix call instructions");
  }

  out << "  ";
  if (!call->result.empty()) {
    out << call->result << " = ";
  }
  out << "call " << call->return_type << " " << call->callee << "("
      << call->args_str << ")\n";
  return true;
}

}  // namespace c4c::backend::aarch64
