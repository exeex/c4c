#pragma once

#include "../module/module.hpp"

#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::vector<module::FunctionRecord> derive_compatibility_function_records(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const std::vector<module::MachineFunction>& functions);

[[nodiscard]] module::CompatibilityProjection derive_compatibility_projection(
    const std::vector<module::FunctionRecord>& functions);

}  // namespace c4c::backend::aarch64::codegen
