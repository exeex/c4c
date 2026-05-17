#pragma once

#include "../regalloc.hpp"
#include "../value_locations.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace c4c::backend::prepare {

namespace regalloc_detail {

struct PreparedPointerCarrierState {
  ValueNameId base_value_name = kInvalidValueName;
  std::int64_t byte_delta = 0;
  std::size_t step_bytes = 0;
};

using PreparedPointerCarrierMap = std::unordered_map<ValueNameId, PreparedPointerCarrierState>;

[[nodiscard]] PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedRegallocValue& value);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
