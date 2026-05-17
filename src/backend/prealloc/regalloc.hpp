#pragma once

#include "frame.hpp"
#include "liveness.hpp"
#include "names.hpp"

#include "../bir/bir.hpp"
#include "../../target_profile.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedRegisterClass {
  None,
  General,
  Float,
  Vector,
  AggregateAddress,
};

[[nodiscard]] constexpr std::string_view prepared_register_class_name(
    PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::None:
      return "none";
    case PreparedRegisterClass::General:
      return "general";
    case PreparedRegisterClass::Float:
      return "float";
    case PreparedRegisterClass::Vector:
      return "vector";
    case PreparedRegisterClass::AggregateAddress:
      return "aggregate_address";
  }
  return "unknown";
}

struct PreparedRegisterGroupOverride {
  FunctionNameId function_name = kInvalidFunctionName;
  ValueNameId value_name = kInvalidValueName;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  std::size_t contiguous_width = 1;
};

// Step 5 fence: register-group overrides are keyed by prepared interned
// function/value IDs. Any raw register spellings downstream are target-physical
// route names, not semantic value identity.
struct PreparedRegisterGroupOverrides {
  std::vector<PreparedRegisterGroupOverride> values;
};

struct PreparedTargetRegisterIdentity {
  c4c::TargetArch target_arch = c4c::TargetArch::Unknown;
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  std::size_t physical_index = 0;

  friend bool operator==(const PreparedTargetRegisterIdentity& lhs,
                         const PreparedTargetRegisterIdentity& rhs) {
    return lhs.target_arch == rhs.target_arch && lhs.bank == rhs.bank &&
           lhs.register_class == rhs.register_class &&
           lhs.physical_index == rhs.physical_index;
  }

  friend bool operator!=(const PreparedTargetRegisterIdentity& lhs,
                         const PreparedTargetRegisterIdentity& rhs) {
    return !(lhs == rhs);
  }
};

enum class PreparedAllocationStatus {
  Unallocated,
  AssignedRegister,
  AssignedStackSlot,
  Split,
  Spilled,
};

[[nodiscard]] constexpr std::string_view prepared_allocation_status_name(
    PreparedAllocationStatus status) {
  switch (status) {
    case PreparedAllocationStatus::Unallocated:
      return "unallocated";
    case PreparedAllocationStatus::AssignedRegister:
      return "assigned_register";
    case PreparedAllocationStatus::AssignedStackSlot:
      return "assigned_stack_slot";
    case PreparedAllocationStatus::Split:
      return "split";
    case PreparedAllocationStatus::Spilled:
      return "spilled";
  }
  return "unknown";
}

enum class PreparedSpillReloadOpKind {
  Spill,
  Reload,
  Rematerialize,
};

[[nodiscard]] constexpr std::string_view prepared_spill_reload_op_kind_name(
    PreparedSpillReloadOpKind kind) {
  switch (kind) {
    case PreparedSpillReloadOpKind::Spill:
      return "spill";
    case PreparedSpillReloadOpKind::Reload:
      return "reload";
    case PreparedSpillReloadOpKind::Rematerialize:
      return "rematerialize";
  }
  return "unknown";
}

struct PreparedPhysicalRegisterAssignment {
  PreparedRegisterClass reg_class = PreparedRegisterClass::None;
  std::string register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> placement;
};

struct PreparedRegisterCandidateSpan {
  std::string register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> placement;
};

struct PreparedStackSlotAssignment {
  PreparedFrameSlotId slot_id = 0;
  std::size_t offset_bytes = 0;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::optional<PreparedSpillSlotPlacement> placement;
};

struct PreparedAllocationConstraint {
  PreparedValueId value_id = 0;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  std::size_t register_group_width = 1;
  bool requires_register = false;
  bool requires_home_slot = false;
  bool cannot_cross_call = false;
  std::optional<std::string> fixed_register_name;
  std::vector<std::string> preferred_register_names;
  std::vector<std::string> forbidden_register_names;
  std::optional<PreparedRegisterPlacement> fixed_register_placement;
  std::vector<PreparedRegisterPlacement> preferred_register_placements;
  std::vector<PreparedRegisterPlacement> forbidden_register_placements;
};

struct PreparedInterferenceEdge {
  PreparedValueId lhs_value_id = 0;
  PreparedValueId rhs_value_id = 0;
  std::string reason;
};

enum class PreparedMoveStorageKind {
  None,
  Register,
  StackSlot,
};

enum class PreparedMoveDestinationKind {
  Value,
  CallArgumentAbi,
  CallResultAbi,
  FunctionReturnAbi,
};

enum class PreparedMoveResolutionOpKind {
  Move,
  SaveDestinationToTemp,
};

enum class PreparedMoveAuthorityKind {
  None,
  OutOfSsaParallelCopy,
};

[[nodiscard]] constexpr std::string_view prepared_move_authority_kind_name(
    PreparedMoveAuthorityKind kind) {
  switch (kind) {
    case PreparedMoveAuthorityKind::None:
      return "none";
    case PreparedMoveAuthorityKind::OutOfSsaParallelCopy:
      return "out_of_ssa_parallel_copy";
  }
  return "unknown";
}

struct PreparedMoveResolution {
  PreparedValueId from_value_id = 0;
  PreparedValueId to_value_id = 0;
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::optional<std::string> destination_register_name;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bool uses_cycle_temp_source = false;
  bool coalesced_by_assigned_storage = false;
  std::optional<std::size_t> source_parallel_copy_step_index;
  std::optional<std::int64_t> source_immediate_i32;
  PreparedMoveResolutionOpKind op_kind = PreparedMoveResolutionOpKind::Move;
  PreparedMoveAuthorityKind authority_kind = PreparedMoveAuthorityKind::None;
  std::optional<BlockLabelId> source_parallel_copy_predecessor_label;
  std::optional<BlockLabelId> source_parallel_copy_successor_label;
  std::string reason;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
};

struct PreparedAbiBinding {
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::optional<std::string> destination_register_name;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
};

struct PreparedSpillReloadOp {
  PreparedValueId value_id = 0;
  PreparedSpillReloadOpKind op_kind = PreparedSpillReloadOpKind::Spill;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  PreparedRegisterBank register_bank = PreparedRegisterBank::None;
  std::optional<std::string> register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedSpillSlotPlacement> spill_slot_placement;
};

struct PreparedRegallocValue {
  PreparedValueId value_id = 0;
  std::optional<PreparedObjectId> stack_object_id;
  FunctionNameId function_name = kInvalidFunctionName;
  ValueNameId value_name = kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::optional<c4c::backend::bir::Value::F128Payload> constant_f128_payload;
  PreparedValueKind value_kind = PreparedValueKind::Temporary;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  std::size_t register_group_width = 1;
  PreparedAllocationStatus allocation_status = PreparedAllocationStatus::Unallocated;
  bool spillable = true;
  bool requires_home_slot = false;
  bool crosses_call = false;
  std::size_t priority = 0;
  double spill_weight = 0.0;
  std::optional<PreparedLiveInterval> live_interval;
  std::optional<PreparedPhysicalRegisterAssignment> assigned_register;
  std::optional<PreparedStackSlotAssignment> assigned_stack_slot;
  std::optional<PreparedPhysicalRegisterAssignment> spill_register_authority;
};

struct PreparedRegallocFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedRegallocValue> values;
  std::vector<PreparedAllocationConstraint> constraints;
  std::vector<PreparedInterferenceEdge> interference;
  std::vector<PreparedMoveResolution> move_resolution;
  std::vector<PreparedSpillReloadOp> spill_reload_ops;
};

struct PreparedRegalloc {
  std::vector<PreparedRegallocFunction> functions;
};

}  // namespace c4c::backend::prepare
