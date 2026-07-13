#pragma once

#include "calls.hpp"
#include "frame.hpp"
#include "names.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedStoragePlanValue {
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::size_t contiguous_width = 1;
  std::optional<std::string> register_name;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<c4c::backend::bir::Value::F128Payload> immediate_f128;
  std::optional<LinkNameId> symbol_name;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedSpillSlotPlacement> spill_slot_placement;
};

struct PreparedStoragePlanFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedStoragePlanValue> values;
};

struct PreparedStoragePlans {
  std::vector<PreparedStoragePlanFunction> functions;
};

}  // namespace c4c::backend::prepare
