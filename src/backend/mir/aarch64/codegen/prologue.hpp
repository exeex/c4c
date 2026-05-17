#pragma once

#include "../module/module.hpp"

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;

void insert_prepared_frame_boundary_nodes(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedControlFlowFunction& prepared_function,
    module::MachineFunction& function);

}  // namespace c4c::backend::aarch64::codegen
