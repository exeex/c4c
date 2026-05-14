#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <optional>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::optional<module::MachineInstruction>
lower_prepared_return_terminator(const module::BlockLoweringContext& context,
                                 const BlockScalarLoweringState& scalar_state,
                                 module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
