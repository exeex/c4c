#pragma once

#include "pointer_carriers.hpp"

#include "../regalloc.hpp"
#include "../value_locations.hpp"

#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedStackLayout* stack_layout,
    const PreparedAddressingFunction* function_addressing,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedRegallocValue& value);

[[nodiscard]] std::vector<PreparedValueHome> build_prepared_value_homes(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedStackLayout* stack_layout,
    const PreparedAddressingFunction* function_addressing,
    const PreparedRegallocFunction& regalloc_function);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
