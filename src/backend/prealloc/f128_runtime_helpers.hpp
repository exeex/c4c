#pragma once

#include "module.hpp"

namespace c4c::backend::prepare {

void populate_f128_runtime_helper_facts(PreparedBirModule& prepared);

[[nodiscard]] bool prepared_f128_runtime_helper_has_abi_contract(
    const PreparedF128RuntimeHelper& helper);

[[nodiscard]] bool prepared_f128_runtime_helper_has_scalar_cmp_result_bridge_contract(
    const PreparedF128RuntimeHelper& helper);

}  // namespace c4c::backend::prepare
