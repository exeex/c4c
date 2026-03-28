#include "memory.hpp"

#include <ostream>

namespace c4c::backend::aarch64 {

bool render_memory_instruction(std::ostream& out,
                               const c4c::codegen::lir::LirInst& inst) {
  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    out << "  " << alloca->result << " = alloca " << alloca->type_str;
    if (!alloca->count.empty()) {
      out << ", i64 " << alloca->count;
    }
    if (alloca->align > 0) {
      out << ", align " << alloca->align;
    }
    out << "\n";
    return true;
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    out << "  " << load->result << " = load " << load->type_str << ", ptr "
        << load->ptr << "\n";
    return true;
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    out << "  store " << store->type_str << " " << store->val << ", ptr "
        << store->ptr << "\n";
    return true;
  }

  return false;
}

}  // namespace c4c::backend::aarch64
