#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;

void insert_prepared_frame_boundary_nodes(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::MachineFunction& function);

[[nodiscard]] std::vector<module::MachineInstruction> lower_entry_formal_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
