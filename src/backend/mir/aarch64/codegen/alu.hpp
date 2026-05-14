#pragma once

#include "../module/module.hpp"

#include <cstddef>
#include <optional>
#include <unordered_map>

namespace c4c::backend::aarch64::codegen {

struct BlockScalarLoweringState {
  std::unordered_map<c4c::ValueNameId, RegisterOperand> emitted_registers;
};

[[nodiscard]] std::optional<abi::RegisterView> scalar_register_view(
    bir::TypeKind type);
[[nodiscard]] std::optional<RegisterOperand> find_emitted_scalar_register(
    const BlockScalarLoweringState& state,
    c4c::ValueNameId value_name);
void record_emitted_scalar_register(BlockScalarLoweringState& state,
                                    c4c::ValueNameId value_name,
                                    RegisterOperand reg);
[[nodiscard]] std::optional<module::MachineInstruction> lower_scalar_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
