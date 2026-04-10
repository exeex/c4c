#include "x86_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

[[noreturn]] void throw_unwired_translated_asm_emitter_owner() {
  throw std::logic_error(
      "x86 translated asm_emitter owner methods are compiled for symbol/link coverage, but the exporter-backed X86Codegen inline-asm state is not wired yet");
}

}  // namespace

AsmOperandKind X86Codegen::classify_constraint(const std::string& constraint) {
  (void)constraint;
  throw_unwired_translated_asm_emitter_owner();
  return static_cast<AsmOperandKind>(0);
}

void X86Codegen::setup_operand_metadata(AsmOperand& op, const Operand& val, bool is_output) {
  (void)op;
  (void)val;
  (void)is_output;
  throw_unwired_translated_asm_emitter_owner();
}

bool X86Codegen::resolve_memory_operand(AsmOperand& op, const Operand& val, const std::vector<std::string>& excluded) {
  (void)op;
  (void)val;
  (void)excluded;
  throw_unwired_translated_asm_emitter_owner();
  return false;
}

std::string X86Codegen::assign_scratch_reg(const AsmOperandKind& kind, const std::vector<std::string>& excluded) {
  (void)kind;
  (void)excluded;
  throw_unwired_translated_asm_emitter_owner();
  return {};
}

void X86Codegen::load_input_to_reg(const AsmOperand& op, const Operand& val, const std::string& constraint) {
  (void)op;
  (void)val;
  (void)constraint;
  throw_unwired_translated_asm_emitter_owner();
}

void X86Codegen::preload_readwrite_output(const AsmOperand& op, const Value& ptr) {
  (void)op;
  (void)ptr;
  throw_unwired_translated_asm_emitter_owner();
}

std::string X86Codegen::substitute_template_line(const std::string& line,
                                                 const std::vector<AsmOperand>& operands,
                                                 const std::vector<std::size_t>& gcc_to_internal,
                                                 const std::vector<IrType>& operand_types,
                                                 const std::vector<std::pair<std::string, BlockId>>& goto_labels) {
  (void)line;
  (void)operands;
  (void)gcc_to_internal;
  (void)operand_types;
  (void)goto_labels;
  throw_unwired_translated_asm_emitter_owner();
  return {};
}

void X86Codegen::store_output_from_reg(const AsmOperand& op,
                                       const Value& ptr,
                                       const std::string& constraint,
                                       const std::vector<const char*>& all_output_regs) {
  (void)op;
  (void)ptr;
  (void)constraint;
  (void)all_output_regs;
  throw_unwired_translated_asm_emitter_owner();
}

void X86Codegen::reset_scratch_state() { throw_unwired_translated_asm_emitter_owner(); }

}  // namespace c4c::backend::x86
