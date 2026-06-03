#include "../module.hpp"
#include "stack_layout.hpp"

#include <variant>

namespace c4c::backend::prepare::stack_layout {

FunctionInlineAsmSummary summarize_inline_asm(const bir::Function& function) {
  FunctionInlineAsmSummary summary;

  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr || !call->inline_asm.has_value()) {
        continue;
      }

      ++summary.instruction_count;
      summary.has_conservative_side_effect_placement =
          summary.has_conservative_side_effect_placement || call->inline_asm->side_effects;
      for (const auto& operand : call->inline_asm->operands) {
        if ((operand.kind == bir::InlineAsmOperandKind::MemoryInput &&
             operand.memory_address.has_value()) ||
            (operand.kind == bir::InlineAsmOperandKind::AddressInput &&
             operand.address.has_value())) {
          ++summary.structured_memory_address_operand_count;
        }
      }
    }
  }

  return summary;
}

}  // namespace c4c::backend::prepare::stack_layout
