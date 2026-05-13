#pragma once

#include "../abi/abi.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::module {

enum class TargetRegisterReferenceKind {
  ValueHome,
  RegallocAssignment,
  SpillAuthority,
  StoragePlan,
};

struct TargetRegisterRecord {
  TargetRegisterReferenceKind reference_kind = TargetRegisterReferenceKind::ValueHome;
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::prepare::PreparedRegisterClass register_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank register_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::string_view physical_register;
  std::size_t contiguous_width = 1;
  std::vector<std::string_view> occupied_registers;
};

struct OperandRecord {
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::string_view label;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::prepare::PreparedValueKind value_kind =
      c4c::backend::prepare::PreparedValueKind::Temporary;
  std::optional<c4c::backend::prepare::PreparedObjectId> stack_object_id;
  c4c::backend::prepare::PreparedValueHomeKind home_kind =
      c4c::backend::prepare::PreparedValueHomeKind::None;
  c4c::backend::prepare::PreparedStorageEncodingKind storage_encoding =
      c4c::backend::prepare::PreparedStorageEncodingKind::None;
  c4c::backend::prepare::PreparedRegisterClass register_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank storage_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::size_t register_group_width = 1;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<c4c::LinkNameId> symbol_name;
  std::string_view symbol_label;
  std::optional<c4c::ValueNameId> pointer_base_value_name;
  std::string_view pointer_base_label;
  std::optional<std::int64_t> pointer_byte_delta;
  std::optional<std::size_t> value_home_register;
  std::optional<std::size_t> assigned_register;
  std::optional<std::size_t> spill_register_authority;
  std::optional<std::size_t> storage_register;
  const c4c::backend::prepare::PreparedValueHome* source_value_home = nullptr;
  const c4c::backend::prepare::PreparedRegallocValue* source_regalloc = nullptr;
  const c4c::backend::prepare::PreparedStoragePlanValue* source_storage = nullptr;
};

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
  std::vector<OperandRecord> operands;
  std::vector<TargetRegisterRecord> target_registers;
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
