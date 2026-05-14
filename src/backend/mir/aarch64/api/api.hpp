#pragma once

#include "../module/module.hpp"

namespace c4c::backend::aarch64::api {

namespace prepare = c4c::backend::prepare;

[[nodiscard]] c4c::backend::aarch64::module::BuildResult build_prepared_module(
    const prepare::PreparedBirModule& module);

}  // namespace c4c::backend::aarch64::api
