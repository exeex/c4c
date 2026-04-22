#pragma once

#include "../../../../backend.hpp"

#include <string>

namespace c4c::backend::x86::api {

[[nodiscard]] std::string emit_module(const c4c::backend::bir::Module& module,
                                      const c4c::TargetProfile& target_profile);
[[nodiscard]] std::string emit_module(const c4c::codegen::lir::LirModule& module,
                                      const c4c::TargetProfile& target_profile);
[[nodiscard]] c4c::backend::BackendAssembleResult assemble_module(
    const c4c::codegen::lir::LirModule& module,
    const c4c::TargetProfile& target_profile,
    const std::string& output_path);
[[nodiscard]] std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::x86::api
