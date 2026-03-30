#include "emit.hpp"

namespace c4c::backend::x86 {

AsmOperandKind X86Codegen::classify_constraint(const std::string& constraint) {
  // The ref implementation distinguishes explicit registers, tied operands,
  // condition codes, x87 stack operands, and the usual GP/XMM alternatives.
  if (constraint.empty()) {
    return AsmOperandKind::Memory;
  }
  return AsmOperandKind::Register;
}

void X86Codegen::setup_operand_metadata(AsmOperand& op, const Operand& val, bool is_output) {
  this->state.emit("    <setup-inline-asm-operand-metadata>");
}

bool X86Codegen::resolve_memory_operand(AsmOperand& op, const Operand& val, const std::vector<std::string>& excluded) {
  this->state.emit("    <resolve-inline-asm-memory-operand>");
  return false;
}

std::string X86Codegen::assign_scratch_reg(const AsmOperandKind& kind, const std::vector<std::string>& excluded) {
  this->state.emit("    <assign-inline-asm-scratch-reg>");
  return "rax";
}

void X86Codegen::load_input_to_reg(const AsmOperand& op, const Operand& val, const std::string& constraint) {
  this->state.emit("    <load-inline-asm-input>");
}

void X86Codegen::preload_readwrite_output(const AsmOperand& op, const Value& ptr) {
  this->state.emit("    <preload-inline-asm-output>");
}

std::string X86Codegen::substitute_template_line(const std::string& line,
                                                 const std::vector<AsmOperand>& operands,
                                                 const std::vector<std::size_t>& gcc_to_internal,
                                                 const std::vector<IrType>& operand_types,
                                                 const std::vector<std::pair<std::string, BlockId>>& goto_labels) {
  this->state.emit("    <substitute-inline-asm-template-line>");
  return line;
}

void X86Codegen::store_output_from_reg(const AsmOperand& op,
                                       const Value& ptr,
                                       const std::string& constraint,
                                       const std::vector<const char*>& all_output_regs) {
  this->state.emit("    <store-inline-asm-output>");
}

void X86Codegen::reset_scratch_state() {
  this->state.emit("    <reset-inline-asm-scratch-state>");
}

}  // namespace c4c::backend::x86
