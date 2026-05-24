#pragma once

#include "frame.hpp"
#include "names.hpp"
#include "regalloc.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedStorageEncodingKind {
  None,
  Register,
  FrameSlot,
  Immediate,
  ComputedAddress,
  SymbolAddress,
};

struct PreparedCallArgumentPlan {
  std::size_t instruction_index = 0;
  std::size_t arg_index = 0;
  PreparedRegisterBank value_bank = PreparedRegisterBank::None;
  PreparedStorageEncodingKind source_encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedValueId> source_value_id;
  std::optional<PreparedValueId> source_base_value_id;
  std::optional<bir::Value> source_literal;
  std::optional<std::string> source_symbol_name;
  std::optional<LinkNameId> source_symbol_name_id;
  std::optional<std::string> source_register_name;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<PreparedRegisterBank> source_register_bank;
  std::optional<ValueNameId> source_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::optional<std::string> destination_register_name;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<PreparedRegisterBank> destination_register_bank;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<PreparedRegisterPlacement> source_register_placement;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
};

struct PreparedCallResultPlan {
  std::size_t instruction_index = 0;
  PreparedRegisterBank value_bank = PreparedRegisterBank::None;
  PreparedMoveStorageKind source_storage_kind = PreparedMoveStorageKind::None;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<PreparedValueId> destination_value_id;
  std::optional<std::string> source_register_name;
  std::size_t source_contiguous_width = 1;
  std::vector<std::string> source_occupied_register_names;
  std::optional<PreparedRegisterBank> source_register_bank;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::string> destination_register_name;
  std::size_t destination_contiguous_width = 1;
  std::vector<std::string> destination_occupied_register_names;
  std::optional<PreparedRegisterBank> destination_register_bank;
  std::optional<PreparedFrameSlotId> destination_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<PreparedRegisterPlacement> source_register_placement;
  std::optional<PreparedRegisterPlacement> destination_register_placement;
  std::optional<PreparedSpillSlotPlacement> destination_spill_slot_placement;
};

struct PreparedClobberedRegister {
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::string register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> placement;
};

enum class PreparedCallPreservationRoute {
  Unknown,
  CalleeSavedRegister,
  StackSlot,
};

[[nodiscard]] constexpr std::string_view prepared_call_preservation_route_name(
    PreparedCallPreservationRoute route) {
  switch (route) {
    case PreparedCallPreservationRoute::Unknown:
      return "unknown";
    case PreparedCallPreservationRoute::CalleeSavedRegister:
      return "callee_saved_register";
    case PreparedCallPreservationRoute::StackSlot:
      return "stack_slot";
  }
  return "unknown";
}

struct PreparedCallPreservedValue {
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  PreparedCallPreservationRoute route = PreparedCallPreservationRoute::Unknown;
  std::optional<std::size_t> callee_saved_save_index;
  std::size_t contiguous_width = 1;
  std::optional<std::string> register_name;
  std::optional<PreparedRegisterBank> register_bank;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> stack_size_bytes;
  std::optional<std::size_t> stack_align_bytes;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedSpillSlotPlacement> spill_slot_placement;
};

enum class PreparedCallWrapperKind {
  SameModule,
  DirectExternFixedArity,
  DirectExternVariadic,
  Indirect,
};

[[nodiscard]] constexpr std::string_view prepared_call_wrapper_kind_name(
    PreparedCallWrapperKind kind) {
  switch (kind) {
    case PreparedCallWrapperKind::SameModule:
      return "same_module";
    case PreparedCallWrapperKind::DirectExternFixedArity:
      return "direct_extern_fixed_arity";
    case PreparedCallWrapperKind::DirectExternVariadic:
      return "direct_extern_variadic";
    case PreparedCallWrapperKind::Indirect:
      return "indirect";
  }
  return "unknown";
}

struct PreparedIndirectCalleePlan {
  ValueNameId value_name = kInvalidValueName;
  std::optional<PreparedValueId> value_id;
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  PreparedRegisterBank bank = PreparedRegisterBank::None;
  std::optional<std::string> register_name;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<ValueNameId> pointer_base_value_name;
  std::optional<std::int64_t> pointer_byte_delta;
  std::optional<PreparedRegisterPlacement> register_placement;
};

struct PreparedMemoryReturnPlan {
  std::optional<std::size_t> sret_arg_index;
  SlotNameId storage_slot_name = kInvalidSlotName;
  PreparedStorageEncodingKind encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct PreparedCallPlan {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  PreparedCallWrapperKind wrapper_kind = PreparedCallWrapperKind::Indirect;
  std::size_t variadic_fpr_arg_register_count = 0;
  bool is_indirect = false;
  std::optional<std::string> direct_callee_name;
  std::optional<PreparedIndirectCalleePlan> indirect_callee;
  std::optional<PreparedMemoryReturnPlan> memory_return;
  std::vector<PreparedCallArgumentPlan> arguments;
  std::optional<PreparedCallResultPlan> result;
  std::vector<PreparedCallPreservedValue> preserved_values;
  std::vector<PreparedClobberedRegister> clobbered_registers;
};

struct PreparedCallPlansFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedCallPlan> calls;
};

struct PreparedCallPlans {
  std::vector<PreparedCallPlansFunction> functions;
};

enum class PreparedCallBoundaryMoveClassificationStatus {
  Available,
  UnsupportedOpKind,
  MissingAbiIndex,
  MissingCallArgumentPlan,
  MissingCallResultPlan,
  MismatchedCallResultPlan,
  MissingAbiBinding,
};

[[nodiscard]] constexpr std::string_view prepared_call_boundary_move_classification_status_name(
    PreparedCallBoundaryMoveClassificationStatus status) {
  switch (status) {
    case PreparedCallBoundaryMoveClassificationStatus::Available:
      return "available";
    case PreparedCallBoundaryMoveClassificationStatus::UnsupportedOpKind:
      return "unsupported_op_kind";
    case PreparedCallBoundaryMoveClassificationStatus::MissingAbiIndex:
      return "missing_abi_index";
    case PreparedCallBoundaryMoveClassificationStatus::MissingCallArgumentPlan:
      return "missing_call_argument_plan";
    case PreparedCallBoundaryMoveClassificationStatus::MissingCallResultPlan:
      return "missing_call_result_plan";
    case PreparedCallBoundaryMoveClassificationStatus::MismatchedCallResultPlan:
      return "mismatched_call_result_plan";
    case PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding:
      return "missing_abi_binding";
  }
  return "unknown";
}

struct PreparedCallBoundaryMoveClassification {
  PreparedCallBoundaryMoveClassificationStatus status =
      PreparedCallBoundaryMoveClassificationStatus::Available;
  const PreparedCallPlan* call_plan = nullptr;
  const PreparedMoveBundle* bundle = nullptr;
  const PreparedMoveResolution* move = nullptr;
  PreparedMovePhase phase = PreparedMovePhase::BeforeInstruction;
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::size_t> abi_index;
  const PreparedCallArgumentPlan* argument_plan = nullptr;
  const PreparedCallResultPlan* result_plan = nullptr;
  const PreparedAbiBinding* abi_binding = nullptr;
};

[[nodiscard]] bool prepared_call_boundary_move_classification_available(
    const PreparedCallBoundaryMoveClassification& classification);

[[nodiscard]] PreparedCallBoundaryMoveClassification
classify_prepared_call_boundary_move(const PreparedCallPlan& call_plan,
                                     const PreparedMoveBundle& bundle,
                                     const PreparedMoveResolution& move);

}  // namespace c4c::backend::prepare
