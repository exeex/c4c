#pragma once

#include "../module/module.hpp"

namespace c4c::backend::aarch64::api {

[[nodiscard]] c4c::backend::aarch64::module::BuildResult build_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::aarch64::api
