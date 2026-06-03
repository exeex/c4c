#pragma once

#include "../addressing.hpp"
#include "../regalloc.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace c4c::backend::prepare {

namespace regalloc_detail {

enum class PreparedPointerCarrierAuthority {
  None,
  PreparedPointerValueAccess,
  BirPointerSymbol,
};

struct PreparedPointerCarrierState {
  ValueNameId base_value_name = kInvalidValueName;
  std::optional<LinkNameId> base_symbol_name;
  std::int64_t byte_delta = 0;
  std::size_t step_bytes = 0;
  PreparedPointerCarrierAuthority authority = PreparedPointerCarrierAuthority::None;
};

using PreparedPointerCarrierMap = std::unordered_map<ValueNameId, PreparedPointerCarrierState>;

[[nodiscard]] constexpr bool has_semantic_pointer_carrier_authority(
    PreparedPointerCarrierAuthority authority) {
  switch (authority) {
    case PreparedPointerCarrierAuthority::PreparedPointerValueAccess:
    case PreparedPointerCarrierAuthority::BirPointerSymbol:
      return true;
    case PreparedPointerCarrierAuthority::None:
      return false;
  }
  return false;
}

[[nodiscard]] constexpr bool has_semantic_pointer_carrier_authority(
    const PreparedPointerCarrierState& carrier) {
  return has_semantic_pointer_carrier_authority(carrier.authority);
}

[[nodiscard]] PreparedPointerCarrierMap build_pointer_carrier_map(
    PreparedNameTables& names,
    const bir::Module& module,
    const bir::Function& function,
    const PreparedAddressingFunction* function_addressing);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
