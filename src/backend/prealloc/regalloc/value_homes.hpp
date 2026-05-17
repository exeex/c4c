#pragma once

#include "pointer_carriers.hpp"

#include "../regalloc.hpp"
#include "../value_locations.hpp"

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedRegallocValue& value);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
