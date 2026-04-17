#include "support.hpp"

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
      summary.has_side_effects = summary.has_side_effects || call->inline_asm->side_effects;
    }
  }

  return summary;
}

}  // namespace c4c::backend::prepare::stack_layout
