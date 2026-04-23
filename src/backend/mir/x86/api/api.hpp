#pragma once

#include "../../../backend.hpp"

#include <string>

namespace c4c::backend::x86::api {

// Public x86 entrypoints accept the shared prepared module. Their contract is
// to route into x86 encoding using `frame_plan`, `call_plans`, and
// `storage_plans` as target-facing authority rather than recomputing those
// decisions locally. The prepared plans should already distinguish GPR, FPR,
// and VREG homes before x86 sees them, and `dynamic_stack_plan` should already
// describe VLA / dynamic-alloca stack motion before x86 emission starts.

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
