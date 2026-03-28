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

  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
    out << "  " << gep->result << " = getelementptr ";
    if (gep->inbounds) {
      out << "inbounds ";
    }
    out << gep->element_type << ", ptr " << gep->ptr;
    for (const auto& index : gep->indices) {
      out << ", " << index;
    }
    out << "\n";
    return true;
  }

  if (const auto* va_start = std::get_if<c4c::codegen::lir::LirVaStartOp>(&inst)) {
    out << "  call void @llvm.va_start.p0(ptr " << va_start->ap_ptr << ")\n";
    return true;
  }

  if (const auto* va_end = std::get_if<c4c::codegen::lir::LirVaEndOp>(&inst)) {
    out << "  call void @llvm.va_end.p0(ptr " << va_end->ap_ptr << ")\n";
    return true;
  }

  if (const auto* va_copy = std::get_if<c4c::codegen::lir::LirVaCopyOp>(&inst)) {
    out << "  call void @llvm.va_copy.p0.p0(ptr " << va_copy->dst_ptr
        << ", ptr " << va_copy->src_ptr << ")\n";
    return true;
  }

  if (const auto* va_arg = std::get_if<c4c::codegen::lir::LirVaArgOp>(&inst)) {
    out << "  " << va_arg->result << " = va_arg ptr " << va_arg->ap_ptr << ", "
        << va_arg->type_str << "\n";
    return true;
  }

  return false;
}

}  // namespace c4c::backend::aarch64
