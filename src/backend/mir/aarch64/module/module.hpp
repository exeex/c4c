#pragma once

#include "../abi/abi.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::module {

struct BlockRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view label;
  c4c::backend::bir::TerminatorKind terminator_kind = c4c::backend::bir::TerminatorKind::Return;
  c4c::BlockLabelId branch_target_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId true_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId false_label = c4c::kInvalidBlockLabel;
  const c4c::backend::bir::Block* source_block = nullptr;
  const c4c::backend::prepare::PreparedControlFlowBlock* control_flow = nullptr;
};

struct FunctionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::string_view label;
  const c4c::backend::bir::Function* source_function = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* control_flow = nullptr;
  std::vector<BlockRecord> blocks;
};

struct Module {
  const c4c::backend::prepare::PreparedBirModule* prepared = nullptr;
  c4c::TargetProfile target_profile{};
  std::vector<FunctionRecord> functions;
};

struct BuildResult {
  std::optional<Module> module;
  std::optional<c4c::backend::aarch64::abi::HandoffError> error;
};

[[nodiscard]] BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::module
