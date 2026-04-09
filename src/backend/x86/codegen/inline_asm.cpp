#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

std::vector<AsmOperand> X86Codegen::substitute_x86_asm_operands(
    const std::string& template_line,
    const std::vector<AsmOperand>& operands,
    const std::vector<std::size_t>& gcc_to_internal,
    const std::vector<IrType>& operand_types,
    const std::vector<std::pair<std::string, BlockId>>& goto_labels) {
  this->state.emit("    <substitute-inline-asm-operands>");
  return operands;
}

void X86Codegen::emit_operand_with_modifier(const AsmOperand& op,
                                            char modifier,
                                            const Operand& val) {
  this->state.emit("    <emit-inline-asm-operand>");
}

std::optional<char> X86Codegen::default_modifier_for_type(std::optional<IrType> ty) {
  if (!ty.has_value()) {
    return std::nullopt;
  }
  return 'q';
}

std::string X86Codegen::format_x86_reg(const std::string& reg, std::optional<char> modifier) {
  return reg;
}

std::string X86Codegen::reg_to_32(const std::string& reg) { return reg; }
std::string X86Codegen::reg_to_64(const std::string& reg) { return reg; }
std::string X86Codegen::reg_to_16(const std::string& reg) { return reg; }
std::string X86Codegen::reg_to_8l(const std::string& reg) { return reg; }

const char* X86Codegen::gcc_cc_to_x86(const std::string& cond) {
  return "cc";
}

}  // namespace c4c::backend::x86
