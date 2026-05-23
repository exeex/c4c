#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::vector<module::MachineInstruction> lower_entry_formal_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
