#pragma once

#include "module.hpp"

namespace c4c::backend::prepare {

void populate_i128_runtime_helper_lanes(PreparedBirModule& prepared);

[[nodiscard]] bool prepared_i128_runtime_helper_has_abi_contract(
    const PreparedI128RuntimeHelper& helper);

}  // namespace c4c::backend::prepare
