#pragma once

#include "pointer_carriers.hpp"

#include "../regalloc.hpp"
#include "../value_locations.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

struct PreparedComputedValueLookup {
  c4c::backend::bir::NameTables bir_names;
  std::unordered_map<ValueNameId, const c4c::backend::bir::BinaryInst*> binaries_by_value_name;
  std::unordered_map<ValueNameId, const c4c::backend::bir::LoadGlobalInst*> global_loads_by_value_name;
};

[[nodiscard]] PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedStackLayout* stack_layout,
    const PreparedAddressingFunction* function_addressing,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedComputedValueLookup& computed_values,
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
