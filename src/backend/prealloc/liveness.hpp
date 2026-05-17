#pragma once

#include "names.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedValueKind {
  StackObject,
  Parameter,
  CallResult,
  Phi,
  Temporary,
};

[[nodiscard]] constexpr std::string_view prepared_value_kind_name(PreparedValueKind kind) {
  switch (kind) {
    case PreparedValueKind::StackObject:
      return "stack_object";
    case PreparedValueKind::Parameter:
      return "parameter";
    case PreparedValueKind::CallResult:
      return "call_result";
    case PreparedValueKind::Phi:
      return "phi";
    case PreparedValueKind::Temporary:
      return "temporary";
  }
  return "unknown";
}

struct PreparedLiveInterval {
  PreparedValueId value_id = 0;
  std::size_t start_point = 0;
  std::size_t end_point = 0;
};

struct PreparedLivenessValue {
  PreparedValueId value_id = 0;
  std::optional<PreparedObjectId> stack_object_id;
  FunctionNameId function_name = kInvalidFunctionName;
  ValueNameId value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  PreparedValueKind value_kind = PreparedValueKind::Temporary;
  bool address_taken = false;
  bool requires_home_slot = false;
  bool crosses_call = false;
  std::optional<std::size_t> definition_point;
  std::vector<std::size_t> use_points;
  std::optional<PreparedLiveInterval> live_interval;
};

struct PreparedLivenessBlock {
  BlockLabelId block_name = kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t start_point = 0;
  std::size_t end_point = 0;
  std::vector<std::size_t> predecessor_block_indices;
  std::vector<std::size_t> successor_block_indices;
  std::vector<PreparedValueId> live_in;
  std::vector<PreparedValueId> live_out;
};

struct PreparedLivenessFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t instruction_count = 0;
  std::vector<PreparedLiveInterval> intervals;
  std::vector<std::size_t> call_points;
  std::vector<std::size_t> block_loop_depth;
  std::vector<PreparedLivenessBlock> blocks;
  std::vector<PreparedLivenessValue> values;
};

struct PreparedLiveness {
  std::vector<PreparedLivenessFunction> functions;
};

}  // namespace c4c::backend::prepare
