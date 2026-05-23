#pragma once

#include "../module/module.hpp"

#include <cstddef>
#include <optional>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_helper_call(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
