#pragma once

#include "instruction.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct InlineAsmSubstitutionResult {
  std::optional<std::vector<std::string>> lines;
  std::string diagnostic;
};

[[nodiscard]] bool has_prepared_inline_asm_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<module::MachineInstruction> lower_inline_asm_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] InlineAsmSubstitutionResult substitute_inline_asm_template(
    const AssemblerInstructionRecord& assembler);

}  // namespace c4c::backend::aarch64::codegen
