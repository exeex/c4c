#pragma once

#include "../addressing.hpp"
#include "../regalloc.hpp"

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

[[nodiscard]] PreparedPointerCarrierMap build_pointer_carrier_map(
    PreparedNameTables& names,
    const bir::Function& function,
    const PreparedAddressingFunction* function_addressing);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
