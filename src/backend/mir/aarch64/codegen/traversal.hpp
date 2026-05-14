#pragma once

#include "../module/module.hpp"

#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::vector<module::MachineFunction> lower_prepared_functions(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] module::FunctionLoweringContext make_function_lowering_context(
    const prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile,
    const prepare::PreparedControlFlowFunction& function);

}  // namespace c4c::backend::aarch64::codegen
