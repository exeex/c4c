#include "prealloc.hpp"
#include "target_register_profile.hpp"
#include "stack_layout/stack_layout.hpp"

#include <array>
#include <algorithm>
#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

struct ActiveRegisterAssignment {
  std::size_t value_index = 0;
  std::size_t end_point = 0;
  std::string register_name;
  std::vector<std::string> occupied_register_names;
};

struct ProgramPointLocation {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

struct PreparedPointerCarrierState {
  ValueNameId base_value_name = kInvalidValueName;
  std::int64_t byte_delta = 0;
  std::size_t step_bytes = 0;
};

using PreparedPointerCarrierMap = std::unordered_map<ValueNameId, PreparedPointerCarrierState>;

[[nodiscard]] PreparedRegisterClass classify_register_class(const PreparedLivenessValue& value) {
  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return PreparedRegisterClass::General;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return PreparedRegisterClass::Float;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return PreparedRegisterClass::None;
  }
  return PreparedRegisterClass::None;
}

[[nodiscard]] PreparedRegisterClass resolve_register_class(
    const PreparedBirModule& prepared,
    const PreparedLivenessValue& value) {
  if (const auto* override =
          find_prepared_register_group_override(prepared, value.function_name, value.value_name);
      override != nullptr && override->register_class != PreparedRegisterClass::None) {
    return override->register_class;
  }
  return classify_register_class(value);
}

[[nodiscard]] std::size_t resolve_register_group_width(const PreparedBirModule& prepared,
                                                       const PreparedLivenessValue& value) {
  if (const auto* override =
          find_prepared_register_group_override(prepared, value.function_name, value.value_name);
      override != nullptr) {
    return std::max<std::size_t>(override->contiguous_width, 1);
  }
  return 1;
}

[[nodiscard]] PreparedRegisterBank register_bank_from_class(PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::General:
      return PreparedRegisterBank::Gpr;
    case PreparedRegisterClass::Float:
      return PreparedRegisterBank::Fpr;
    case PreparedRegisterClass::Vector:
      return PreparedRegisterBank::Vreg;
    case PreparedRegisterClass::AggregateAddress:
      return PreparedRegisterBank::AggregateAddress;
    case PreparedRegisterClass::None:
      return PreparedRegisterBank::None;
  }
  return PreparedRegisterBank::None;
}

[[nodiscard]] bool intervals_overlap(const PreparedLiveInterval& lhs,
                                     const PreparedLiveInterval& rhs) {
  return std::max(lhs.start_point, rhs.start_point) <= std::min(lhs.end_point, rhs.end_point);
}

[[nodiscard]] std::size_t value_priority(const PreparedLivenessValue& value) {
  std::size_t priority = value.use_points.size();
  if (value.live_interval.has_value() && value.live_interval->end_point >= value.live_interval->start_point) {
    priority += (value.live_interval->end_point - value.live_interval->start_point) + 1U;
  }
  if (value.crosses_call) {
    priority += 2U;
  }
  if (value.requires_home_slot) {
    priority += 1U;
  }
  return priority;
}

[[nodiscard]] std::size_t loop_depth_weight(std::size_t loop_depth) {
  switch (loop_depth) {
    case 0:
      return 1U;
    case 1:
      return 10U;
    case 2:
      return 100U;
    case 3:
      return 1000U;
    default:
      return 10000U;
  }
}

[[nodiscard]] std::optional<ProgramPointLocation> locate_program_point(
    const PreparedLivenessFunction& function,
    std::size_t point) {
  for (const auto& block : function.blocks) {
    if (point < block.start_point || point > block.end_point) {
      continue;
    }
    return ProgramPointLocation{
        .block_index = block.block_index,
        .instruction_index = point - block.start_point,
    };
  }
  return std::nullopt;
}

[[nodiscard]] std::size_t weighted_use_score(const PreparedLivenessFunction& function,
                                             const PreparedLivenessValue& value) {
  std::size_t weighted_uses = 0;
  for (const std::size_t use_point : value.use_points) {
    std::size_t weight = 1U;
    if (const auto location = locate_program_point(function, use_point); location.has_value() &&
        location->block_index < function.block_loop_depth.size()) {
      weight = loop_depth_weight(function.block_loop_depth[location->block_index]);
    }
    weighted_uses += weight;
  }
  return weighted_uses;
}

[[nodiscard]] std::vector<std::string> materialize_register_names(
    const std::vector<std::string_view>& register_names) {
  std::vector<std::string> materialized;
  materialized.reserve(register_names.size());
  for (const std::string_view register_name : register_names) {
    materialized.emplace_back(register_name);
  }
  return materialized;
}

[[nodiscard]] std::size_t published_register_group_width(const PreparedRegallocValue& value) {
  if (value.assigned_register.has_value()) {
    return std::max<std::size_t>(value.assigned_register->contiguous_width, 1);
  }
  if (value.spill_register_authority.has_value()) {
    return std::max<std::size_t>(value.spill_register_authority->contiguous_width, 1);
  }
  return std::max<std::size_t>(value.register_group_width, 1);
}

[[nodiscard]] std::optional<std::pair<std::string, std::size_t>> split_trailing_register_index(
    std::string_view register_name) {
  if (register_name.empty()) {
    return std::nullopt;
  }
  std::size_t digit_start = register_name.size();
  while (digit_start > 0 && register_name[digit_start - 1] >= '0' &&
         register_name[digit_start - 1] <= '9') {
    --digit_start;
  }
  if (digit_start == register_name.size()) {
    return std::nullopt;
  }

  std::size_t parsed_index = 0;
  const char* const begin = register_name.data() + digit_start;
  const char* const end = register_name.data() + register_name.size();
  const auto result = std::from_chars(begin, end, parsed_index);
  if (result.ec != std::errc{} || result.ptr != end) {
    return std::nullopt;
  }
  return std::pair<std::string, std::size_t>{std::string(register_name.substr(0, digit_start)),
                                              parsed_index};
}

template <std::size_t N>
[[nodiscard]] std::vector<std::string> abi_register_names_from_sequence(
    const std::array<std::string_view, N>& sequence,
    std::size_t start_index,
    std::size_t contiguous_width) {
  if (start_index + contiguous_width > sequence.size()) {
    return {};
  }

  std::vector<std::string> names;
  names.reserve(contiguous_width);
  for (std::size_t index = 0; index < contiguous_width; ++index) {
    names.emplace_back(sequence[start_index + index]);
  }
  return names;
}

[[nodiscard]] std::vector<std::string> contiguous_numeric_register_names(std::string_view register_name,
                                                                         std::size_t contiguous_width) {
  if (const auto split = split_trailing_register_index(register_name); split.has_value()) {
    std::vector<std::string> names;
    names.reserve(contiguous_width);
    for (std::size_t index = 0; index < contiguous_width; ++index) {
      names.push_back(split->first + std::to_string(split->second + index));
    }
    return names;
  }
  return {};
}

[[nodiscard]] std::vector<std::string> call_arg_destination_register_names(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t arg_index,
    std::string_view register_name,
    std::size_t contiguous_width) {
  if (contiguous_width <= 1) {
    return std::vector<std::string>{std::string(register_name)};
  }

  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (reg_class == PreparedRegisterClass::General) {
        constexpr std::array<std::string_view, 6> kX86GeneralArgRegisters = {
            "rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        if (const auto names =
                abi_register_names_from_sequence(kX86GeneralArgRegisters, arg_index, contiguous_width);
            !names.empty()) {
          return names;
        }
      }
      break;
    case c4c::TargetArch::I686:
      return {};
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
    case c4c::TargetArch::Unknown:
      break;
  }

  return contiguous_numeric_register_names(register_name, contiguous_width);
}

[[nodiscard]] std::vector<std::string> call_result_destination_register_names(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::string_view register_name,
    std::size_t contiguous_width) {
  if (contiguous_width <= 1) {
    return std::vector<std::string>{std::string(register_name)};
  }

  switch (target_profile.arch) {
    case c4c::TargetArch::X86_64:
      if (reg_class == PreparedRegisterClass::General) {
        constexpr std::array<std::string_view, 2> kX86GeneralReturnRegisters = {"rax", "rdx"};
        for (std::size_t index = 0; index < kX86GeneralReturnRegisters.size(); ++index) {
          if (kX86GeneralReturnRegisters[index] != register_name) {
            continue;
          }
          if (const auto names = abi_register_names_from_sequence(
                  kX86GeneralReturnRegisters, index, contiguous_width);
              !names.empty()) {
            return names;
          }
          break;
        }
      }
      break;
    case c4c::TargetArch::I686:
      return {};
    case c4c::TargetArch::Aarch64:
    case c4c::TargetArch::Riscv64:
    case c4c::TargetArch::Unknown:
      break;
  }

  return contiguous_numeric_register_names(register_name, contiguous_width);
}

[[nodiscard]] std::size_t align_up(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::size_t call_stack_argument_size_bytes(const bir::CallArgAbiInfo& abi) {
  return align_up(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t call_stack_argument_alignment_bytes(const bir::CallArgAbiInfo& abi) {
  const std::size_t abi_alignment = abi.align_bytes == 0 ? abi.size_bytes : abi.align_bytes;
  return std::min<std::size_t>(std::max<std::size_t>(abi_alignment, 8), 16);
}

[[nodiscard]] std::size_t normalized_value_size(const PreparedRegallocValue& value) {
  return stack_layout::normalize_size(value.type, 0);
}

[[nodiscard]] std::size_t normalized_value_alignment(const PreparedRegallocValue& value) {
  return stack_layout::normalize_alignment(value.type, 0, normalized_value_size(value));
}

[[nodiscard]] PreparedMoveStorageKind assigned_storage_kind(const PreparedRegallocValue& value) {
  if (value.assigned_register.has_value()) {
    return PreparedMoveStorageKind::Register;
  }
  if (value.assigned_stack_slot.has_value()) {
    return PreparedMoveStorageKind::StackSlot;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] bool assigned_storage_matches(const PreparedRegallocValue& lhs,
                                            const PreparedRegallocValue& rhs) {
  const PreparedMoveStorageKind lhs_kind = assigned_storage_kind(lhs);
  const PreparedMoveStorageKind rhs_kind = assigned_storage_kind(rhs);
  if (lhs_kind != rhs_kind) {
    return false;
  }
  switch (lhs_kind) {
    case PreparedMoveStorageKind::None:
      return true;
    case PreparedMoveStorageKind::Register:
      return lhs.assigned_register->occupied_register_names ==
             rhs.assigned_register->occupied_register_names;
    case PreparedMoveStorageKind::StackSlot:
      return lhs.assigned_stack_slot->slot_id == rhs.assigned_stack_slot->slot_id;
  }
  return false;
}

[[nodiscard]] std::string storage_transfer_reason(std::string_view prefix,
                                                  PreparedMoveStorageKind from_kind,
                                                  PreparedMoveStorageKind to_kind) {
  if (from_kind == PreparedMoveStorageKind::StackSlot &&
      to_kind == PreparedMoveStorageKind::Register) {
    return std::string(prefix) + "_stack_to_register";
  }
  if (from_kind == PreparedMoveStorageKind::Register &&
      to_kind == PreparedMoveStorageKind::StackSlot) {
    return std::string(prefix) + "_register_to_stack";
  }
  if (from_kind == PreparedMoveStorageKind::Register &&
      to_kind == PreparedMoveStorageKind::Register) {
    return std::string(prefix) + "_register_to_register";
  }
  if (from_kind == PreparedMoveStorageKind::StackSlot &&
      to_kind == PreparedMoveStorageKind::StackSlot) {
    return std::string(prefix) + "_stack_to_stack";
  }
  return std::string(prefix) + "_storage_resolution";
}

[[nodiscard]] std::string storage_transfer_reason(std::string_view prefix,
                                                  const PreparedRegallocValue& from,
                                                  const PreparedRegallocValue& to) {
  return storage_transfer_reason(prefix, assigned_storage_kind(from), assigned_storage_kind(to));
}

void append_move_resolution_record(PreparedRegallocFunction& regalloc_function,
                                   const PreparedRegallocValue& source,
                                   const PreparedRegallocValue& destination,
                                   std::size_t block_index,
                                   std::size_t instruction_index,
                                   bool uses_cycle_temp_source,
                                   bool coalesced_by_assigned_storage,
                                   std::optional<std::size_t> source_parallel_copy_step_index,
                                   PreparedMoveResolutionOpKind op_kind,
                                   PreparedMoveAuthorityKind authority_kind,
                                   std::string reason,
                                   std::optional<BlockLabelId> source_parallel_copy_predecessor_label =
                                       std::nullopt,
                                   std::optional<BlockLabelId> source_parallel_copy_successor_label =
                                       std::nullopt) {
  if (assigned_storage_kind(source) == PreparedMoveStorageKind::None ||
      assigned_storage_kind(destination) == PreparedMoveStorageKind::None ||
      (op_kind == PreparedMoveResolutionOpKind::Move && !coalesced_by_assigned_storage &&
       assigned_storage_matches(source, destination))) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == source.value_id && move.to_value_id == destination.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::Value &&
               move.destination_storage_kind == assigned_storage_kind(destination) &&
               !move.destination_abi_index.has_value() &&
               !move.destination_register_name.has_value() &&
               !move.destination_stack_offset_bytes.has_value() &&
               move.uses_cycle_temp_source == uses_cycle_temp_source &&
               move.coalesced_by_assigned_storage == coalesced_by_assigned_storage &&
               move.source_parallel_copy_step_index == source_parallel_copy_step_index &&
               !move.source_immediate_i32.has_value() &&
               move.op_kind == op_kind &&
               move.authority_kind == authority_kind &&
               move.source_parallel_copy_predecessor_label ==
                   source_parallel_copy_predecessor_label &&
               move.source_parallel_copy_successor_label ==
                   source_parallel_copy_successor_label &&
               move.block_index == block_index &&
               move.instruction_index == instruction_index;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = source.value_id,
      .to_value_id = destination.value_id,
      .destination_kind = PreparedMoveDestinationKind::Value,
      .destination_storage_kind = assigned_storage_kind(destination),
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::nullopt,
      .destination_stack_offset_bytes = std::nullopt,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = uses_cycle_temp_source,
      .coalesced_by_assigned_storage = coalesced_by_assigned_storage,
      .source_parallel_copy_step_index = source_parallel_copy_step_index,
      .source_immediate_i32 = std::nullopt,
      .op_kind = op_kind,
      .authority_kind = authority_kind,
      .source_parallel_copy_predecessor_label = source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = source_parallel_copy_successor_label,
      .reason = std::move(reason),
  });
}

void append_move_resolution_record(PreparedRegallocFunction& regalloc_function,
                                   PreparedValueId from_value_id,
                                   PreparedValueId to_value_id,
                                   PreparedMoveDestinationKind destination_kind,
                                   PreparedMoveStorageKind from_kind,
                                   PreparedMoveStorageKind to_kind,
                                   std::optional<std::size_t> destination_abi_index,
                                   std::optional<std::string> destination_register_name,
                                   std::size_t destination_contiguous_width,
                                   std::vector<std::string> destination_occupied_register_names,
                                   std::optional<std::size_t> destination_stack_offset_bytes,
                                   std::size_t block_index,
                                   std::size_t instruction_index,
                                   bool uses_cycle_temp_source,
                                   bool coalesced_by_assigned_storage,
                                   std::optional<std::size_t> source_parallel_copy_step_index,
                                   PreparedMoveResolutionOpKind op_kind,
                                   PreparedMoveAuthorityKind authority_kind,
                                   std::string reason,
                                   std::optional<BlockLabelId> source_parallel_copy_predecessor_label =
                                       std::nullopt,
                                   std::optional<BlockLabelId> source_parallel_copy_successor_label =
                                       std::nullopt) {
  if (from_kind == PreparedMoveStorageKind::None || to_kind == PreparedMoveStorageKind::None) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == from_value_id && move.to_value_id == to_value_id &&
               move.destination_kind == destination_kind &&
               move.destination_storage_kind == to_kind &&
               move.destination_abi_index == destination_abi_index &&
               move.destination_register_name == destination_register_name &&
               move.destination_contiguous_width == destination_contiguous_width &&
               move.destination_occupied_register_names == destination_occupied_register_names &&
               move.destination_stack_offset_bytes == destination_stack_offset_bytes &&
               move.block_index == block_index &&
               move.instruction_index == instruction_index &&
               move.uses_cycle_temp_source == uses_cycle_temp_source &&
               move.coalesced_by_assigned_storage == coalesced_by_assigned_storage &&
               move.source_parallel_copy_step_index == source_parallel_copy_step_index &&
               !move.source_immediate_i32.has_value() &&
               move.op_kind == op_kind &&
               move.authority_kind == authority_kind &&
               move.source_parallel_copy_predecessor_label ==
                   source_parallel_copy_predecessor_label &&
               move.source_parallel_copy_successor_label ==
                   source_parallel_copy_successor_label;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = from_value_id,
      .to_value_id = to_value_id,
      .destination_kind = destination_kind,
      .destination_storage_kind = to_kind,
      .destination_abi_index = destination_abi_index,
      .destination_register_name = std::move(destination_register_name),
      .destination_contiguous_width = destination_contiguous_width,
      .destination_occupied_register_names = std::move(destination_occupied_register_names),
      .destination_stack_offset_bytes = destination_stack_offset_bytes,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = uses_cycle_temp_source,
      .coalesced_by_assigned_storage = coalesced_by_assigned_storage,
      .source_parallel_copy_step_index = source_parallel_copy_step_index,
      .source_immediate_i32 = std::nullopt,
      .op_kind = op_kind,
      .authority_kind = authority_kind,
      .source_parallel_copy_predecessor_label = source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = source_parallel_copy_successor_label,
      .reason = std::move(reason),
  });
}

void append_immediate_i32_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const bir::Value& source,
    const PreparedRegallocValue& destination,
    std::size_t block_index,
    std::size_t instruction_index,
    std::optional<std::size_t> source_parallel_copy_step_index,
    PreparedMoveAuthorityKind authority_kind,
    std::string reason,
    std::optional<BlockLabelId> source_parallel_copy_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> source_parallel_copy_successor_label = std::nullopt) {
  if (source.kind != bir::Value::Kind::Immediate || source.type != bir::TypeKind::I32 ||
      assigned_storage_kind(destination) == PreparedMoveStorageKind::None) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == destination.value_id &&
               move.to_value_id == destination.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::Value &&
               move.destination_storage_kind == assigned_storage_kind(destination) &&
               !move.destination_abi_index.has_value() &&
               !move.destination_register_name.has_value() &&
               !move.destination_stack_offset_bytes.has_value() &&
               !move.uses_cycle_temp_source && !move.coalesced_by_assigned_storage &&
               move.source_parallel_copy_step_index == source_parallel_copy_step_index &&
               move.source_immediate_i32 == source.immediate &&
               move.op_kind == PreparedMoveResolutionOpKind::Move &&
               move.authority_kind == authority_kind &&
               move.source_parallel_copy_predecessor_label ==
                   source_parallel_copy_predecessor_label &&
               move.source_parallel_copy_successor_label ==
                   source_parallel_copy_successor_label &&
               move.block_index == block_index &&
               move.instruction_index == instruction_index;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = destination.value_id,
      .to_value_id = destination.value_id,
      .destination_kind = PreparedMoveDestinationKind::Value,
      .destination_storage_kind = assigned_storage_kind(destination),
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::nullopt,
      .destination_stack_offset_bytes = std::nullopt,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = false,
      .coalesced_by_assigned_storage = false,
      .source_parallel_copy_step_index = source_parallel_copy_step_index,
      .source_immediate_i32 = source.immediate,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .authority_kind = authority_kind,
      .source_parallel_copy_predecessor_label = source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = source_parallel_copy_successor_label,
      .reason = std::move(reason),
  });
}

void append_unassigned_return_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    PreparedMoveStorageKind consumed_kind,
    std::optional<std::string> destination_register_name,
    std::size_t destination_contiguous_width,
    std::vector<std::string> destination_occupied_register_names,
    std::size_t block_index,
    std::size_t instruction_index,
    std::string reason) {
  if (assigned_storage_kind(source) != PreparedMoveStorageKind::None ||
      consumed_kind == PreparedMoveStorageKind::None) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == source.value_id &&
               move.to_value_id == source.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::FunctionReturnAbi &&
               move.destination_storage_kind == consumed_kind &&
               !move.destination_abi_index.has_value() &&
               move.destination_register_name == destination_register_name &&
               move.destination_contiguous_width == destination_contiguous_width &&
               move.destination_occupied_register_names == destination_occupied_register_names &&
               !move.destination_stack_offset_bytes.has_value() &&
               move.block_index == block_index &&
               move.instruction_index == instruction_index &&
               !move.uses_cycle_temp_source && !move.coalesced_by_assigned_storage &&
               !move.source_parallel_copy_step_index.has_value() &&
               !move.source_immediate_i32.has_value() &&
               move.op_kind == PreparedMoveResolutionOpKind::Move &&
               move.authority_kind == PreparedMoveAuthorityKind::None;
      });
  if (duplicate != regalloc_function.move_resolution.end()) {
    return;
  }

  regalloc_function.move_resolution.push_back(PreparedMoveResolution{
      .from_value_id = source.value_id,
      .to_value_id = source.value_id,
      .destination_kind = PreparedMoveDestinationKind::FunctionReturnAbi,
      .destination_storage_kind = consumed_kind,
      .destination_abi_index = std::nullopt,
      .destination_register_name = std::move(destination_register_name),
      .destination_contiguous_width = destination_contiguous_width,
      .destination_occupied_register_names = std::move(destination_occupied_register_names),
      .destination_stack_offset_bytes = std::nullopt,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = false,
      .coalesced_by_assigned_storage = false,
      .source_parallel_copy_step_index = std::nullopt,
      .source_immediate_i32 = std::nullopt,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .authority_kind = PreparedMoveAuthorityKind::None,
      .reason = std::move(reason),
  });
}

void expire_completed_assignments(std::vector<ActiveRegisterAssignment>& active,
                                  std::size_t start_point) {
  active.erase(std::remove_if(active.begin(),
                              active.end(),
                              [start_point](const ActiveRegisterAssignment& assignment) {
                                return assignment.end_point < start_point;
                              }),
               active.end());
}

[[nodiscard]] bool assignments_overlap(const ActiveRegisterAssignment& active_assignment,
                                       const PreparedRegisterCandidateSpan& candidate) {
  for (const auto& active_register : active_assignment.occupied_register_names) {
    for (const auto& candidate_register : candidate.occupied_register_names) {
      if (active_register == candidate_register) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<PreparedRegisterCandidateSpan> choose_register_span(
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<PreparedRegisterCandidateSpan>& candidate_spans) {
  for (const auto& candidate : candidate_spans) {
    bool overlap = false;
    for (const auto& assignment : active) {
      if (assignments_overlap(assignment, candidate)) {
        overlap = true;
        break;
      }
    }
    if (!overlap) {
      return candidate;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool has_lower_allocation_rank(const PreparedRegallocValue& lhs,
                                             const PreparedRegallocValue& rhs) {
  if (lhs.spill_weight != rhs.spill_weight) {
    return lhs.spill_weight < rhs.spill_weight;
  }
  if (lhs.priority != rhs.priority) {
    return lhs.priority < rhs.priority;
  }
  if (lhs.live_interval.has_value() != rhs.live_interval.has_value()) {
    return !lhs.live_interval.has_value();
  }
  if (lhs.live_interval.has_value() && rhs.live_interval.has_value() &&
      lhs.live_interval->start_point != rhs.live_interval->start_point) {
    return lhs.live_interval->start_point > rhs.live_interval->start_point;
  }
  return lhs.value_id > rhs.value_id;
}

template <typename CanEvict>
[[nodiscard]] std::optional<std::size_t> choose_eviction_candidate(
    const PreparedRegallocFunction& function,
    const std::vector<ActiveRegisterAssignment>& active,
    const std::vector<PreparedRegisterCandidateSpan>& candidate_spans,
    const PreparedRegallocValue& value,
    CanEvict can_evict) {
  std::optional<std::size_t> weakest_active_index;
  for (std::size_t active_index = 0; active_index < active.size(); ++active_index) {
    const auto& assignment = active[active_index];
    const bool overlaps_any_candidate =
        std::any_of(candidate_spans.begin(),
                    candidate_spans.end(),
                    [&](const PreparedRegisterCandidateSpan& candidate) {
                      return assignments_overlap(assignment, candidate);
                    });
    if (!overlaps_any_candidate || !can_evict(assignment)) {
      continue;
    }

    const auto& active_value = function.values[assignment.value_index];
    if (!has_lower_allocation_rank(active_value, value)) {
      continue;
    }
    if (!weakest_active_index.has_value() ||
        has_lower_allocation_rank(active_value,
                                  function.values[active[*weakest_active_index].value_index])) {
      weakest_active_index = active_index;
    }
  }
  return weakest_active_index;
}

[[nodiscard]] std::optional<PreparedStackSlotAssignment> existing_stack_slot_assignment(
    const PreparedStackLayout& stack_layout,
    const PreparedRegallocValue& value) {
  if (!value.stack_object_id.has_value()) {
    return std::nullopt;
  }
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.object_id != *value.stack_object_id) {
      continue;
    }
    return PreparedStackSlotAssignment{
        .slot_id = slot.slot_id,
        .offset_bytes = slot.offset_bytes,
    };
  }
  return std::nullopt;
}

[[nodiscard]] PreparedStackSlotAssignment allocate_stack_slot(const PreparedRegallocValue& value,
                                                              const PreparedStackLayout& stack_layout,
                                                              PreparedFrameSlotId& next_slot_id,
                                                              std::size_t& next_offset_bytes,
                                                              std::size_t& frame_alignment_bytes) {
  if (const auto existing = existing_stack_slot_assignment(stack_layout, value); existing.has_value()) {
    return *existing;
  }

  const std::size_t size_bytes = normalized_value_size(value);
  const std::size_t align_bytes = normalized_value_alignment(value);
  next_offset_bytes = stack_layout::align_to(next_offset_bytes, align_bytes);
  PreparedStackSlotAssignment slot{
      .slot_id = next_slot_id++,
      .offset_bytes = next_offset_bytes,
  };
  next_offset_bytes += size_bytes;
  frame_alignment_bytes = std::max(frame_alignment_bytes, align_bytes);
  return slot;
}

[[nodiscard]] PreparedValueHome classify_prepared_value_home(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedPointerCarrierMap& pointer_carriers,
    const PreparedRegallocValue& value) {
  PreparedValueHome home{
      .value_id = value.value_id,
      .function_name = value.function_name,
      .value_name = value.value_name,
      .kind = PreparedValueHomeKind::None,
      .register_name = std::nullopt,
      .slot_id = std::nullopt,
      .offset_bytes = std::nullopt,
      .immediate_i32 = std::nullopt,
      .pointer_base_value_name = std::nullopt,
      .pointer_byte_delta = std::nullopt,
  };
  if (function != nullptr && value.value_kind == PreparedValueKind::Parameter) {
    const std::string_view value_name = prepared_value_name(names, value.value_name);
    for (std::size_t param_index = 0; param_index < function->params.size(); ++param_index) {
      const auto& param = function->params[param_index];
      if (param.name != value_name || !param.abi.has_value() || param.is_varargs || param.is_sret ||
          param.is_byval) {
        continue;
      }
      if (const auto register_name =
              call_arg_destination_register_name(target_profile, *param.abi, param_index);
          register_name.has_value()) {
        home.kind = PreparedValueHomeKind::Register;
        home.register_name = *register_name;
      }
      return home;
    }
  }
  if (function != nullptr && value.type == bir::TypeKind::I32) {
    std::unordered_map<std::string_view, const bir::BinaryInst*> named_binaries;
    std::unordered_map<std::string_view, const bir::LoadGlobalInst*> named_global_loads;
    for (const auto& block : function->blocks) {
      for (const auto& inst : block.insts) {
        if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
            binary != nullptr &&
            binary->result.kind == bir::Value::Kind::Named && !binary->result.name.empty()) {
          named_binaries.emplace(binary->result.name, binary);
        } else if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst);
                   load_global != nullptr &&
                   load_global->result.kind == bir::Value::Kind::Named &&
                   !load_global->result.name.empty()) {
          named_global_loads.emplace(load_global->result.name, load_global);
        }
      }
    }
    const bir::Value named_value =
        bir::Value::named(value.type, std::string(prepared_value_name(names, value.value_name)));
    if (const auto computed_value =
            classify_computed_value(names, named_value, *function, named_binaries, named_global_loads);
        computed_value.has_value() &&
        computed_value->base.kind == PreparedComputedBaseKind::ImmediateI32) {
      auto current_value = static_cast<std::int32_t>(computed_value->base.immediate);
      bool supported = true;
      for (const auto& operation : computed_value->operations) {
        const auto immediate = static_cast<std::int32_t>(operation.immediate);
        switch (operation.opcode) {
          case bir::BinaryOpcode::Add:
            current_value = static_cast<std::int32_t>(current_value + immediate);
            break;
          case bir::BinaryOpcode::Mul:
            current_value = static_cast<std::int32_t>(current_value * immediate);
            break;
          case bir::BinaryOpcode::And:
            current_value = static_cast<std::int32_t>(current_value & immediate);
            break;
          case bir::BinaryOpcode::Or:
            current_value = static_cast<std::int32_t>(current_value | immediate);
            break;
          case bir::BinaryOpcode::Xor:
            current_value = static_cast<std::int32_t>(current_value ^ immediate);
            break;
          case bir::BinaryOpcode::Sub:
            current_value = static_cast<std::int32_t>(current_value - immediate);
            break;
          case bir::BinaryOpcode::Shl:
            current_value =
                static_cast<std::int32_t>(static_cast<std::uint32_t>(current_value) << immediate);
            break;
          case bir::BinaryOpcode::LShr:
            current_value = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(current_value) >> immediate);
            break;
          case bir::BinaryOpcode::AShr:
            current_value = static_cast<std::int32_t>(current_value >> immediate);
            break;
          default:
            supported = false;
            break;
        }
        if (!supported) {
          break;
        }
      }
      if (supported) {
        home.kind = PreparedValueHomeKind::RematerializableImmediate;
        home.immediate_i32 = current_value;
        return home;
      }
    }
  }
  if (value.type == bir::TypeKind::Ptr) {
    if (const auto carrier_it = pointer_carriers.find(value.value_name);
        carrier_it != pointer_carriers.end() &&
        (carrier_it->second.base_value_name != value.value_name || carrier_it->second.byte_delta != 0)) {
      home.kind = PreparedValueHomeKind::PointerBasePlusOffset;
      home.pointer_base_value_name = carrier_it->second.base_value_name;
      home.pointer_byte_delta = carrier_it->second.byte_delta;
      if (value.assigned_register.has_value()) {
        home.register_name = value.assigned_register->register_name;
      }
      if (value.assigned_stack_slot.has_value()) {
        home.slot_id = value.assigned_stack_slot->slot_id;
        home.offset_bytes = value.assigned_stack_slot->offset_bytes;
      }
      return home;
    }
  }
  if (value.assigned_register.has_value()) {
    home.kind = PreparedValueHomeKind::Register;
    home.register_name = value.assigned_register->register_name;
    return home;
  }
  if (value.assigned_stack_slot.has_value()) {
    home.kind = PreparedValueHomeKind::StackSlot;
    home.slot_id = value.assigned_stack_slot->slot_id;
    home.offset_bytes = value.assigned_stack_slot->offset_bytes;
    return home;
  }
  return home;
}

[[nodiscard]] std::optional<ValueNameId> prepare_inst_result_value_name(
    PreparedNameTables& names,
    const bir::Inst& inst) {
  return std::visit(
      [&](const auto& typed_inst) -> std::optional<ValueNameId> {
        using InstT = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<InstT, bir::BinaryInst> || std::is_same_v<InstT, bir::SelectInst> ||
                      std::is_same_v<InstT, bir::CastInst> || std::is_same_v<InstT, bir::PhiInst> ||
                      std::is_same_v<InstT, bir::LoadLocalInst> ||
                      std::is_same_v<InstT, bir::LoadGlobalInst>) {
          return prepared_named_value_id(names, typed_inst.result);
        } else if constexpr (std::is_same_v<InstT, bir::CallInst>) {
          if (!typed_inst.result.has_value()) {
            return std::nullopt;
          }
          return prepared_named_value_id(names, *typed_inst.result);
        } else {
          return std::nullopt;
        }
      },
      inst);
}

void update_prepared_pointer_step(std::unordered_map<ValueNameId, std::size_t>& steps,
                                  ValueNameId value_name,
                                  std::size_t step_bytes,
                                  bool* changed) {
  if (value_name == kInvalidValueName || step_bytes == 0) {
    return;
  }
  const auto [it, inserted] = steps.emplace(value_name, step_bytes);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (step_bytes < it->second) {
    it->second = step_bytes;
    if (changed != nullptr) {
      *changed = true;
    }
  }
}

void update_prepared_pointer_slot_step(std::unordered_map<std::string, std::size_t>& steps,
                                       std::string_view slot_name,
                                       std::size_t step_bytes,
                                       bool* changed) {
  if (slot_name.empty() || step_bytes == 0) {
    return;
  }
  const auto [it, inserted] = steps.emplace(std::string(slot_name), step_bytes);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (step_bytes < it->second) {
    it->second = step_bytes;
    if (changed != nullptr) {
      *changed = true;
    }
  }
}

[[nodiscard]] std::optional<PreparedPointerCarrierState> resolve_prepared_pointer_carrier_state(
    ValueNameId value_name,
    const PreparedPointerCarrierMap& pointer_carriers,
    const std::unordered_map<ValueNameId, std::size_t>& direct_step_by_value_name,
    std::string_view slot_name,
    const std::unordered_map<std::string, PreparedPointerCarrierState>& slot_pointer_carriers) {
  if (const auto carrier_it = pointer_carriers.find(value_name); carrier_it != pointer_carriers.end()) {
    return carrier_it->second;
  }
  if (const auto direct_step_it = direct_step_by_value_name.find(value_name);
      direct_step_it != direct_step_by_value_name.end()) {
    return PreparedPointerCarrierState{
        .base_value_name = value_name,
        .byte_delta = 0,
        .step_bytes = direct_step_it->second,
    };
  }
  if (!slot_name.empty()) {
    if (const auto slot_it = slot_pointer_carriers.find(std::string(slot_name));
        slot_it != slot_pointer_carriers.end()) {
      return slot_it->second;
    }
  }
  return std::nullopt;
}

void maybe_update_prepared_pointer_carrier(
    PreparedPointerCarrierMap& pointer_carriers,
    ValueNameId value_name,
    const PreparedPointerCarrierState& candidate,
    bool* changed) {
  if (value_name == kInvalidValueName || candidate.base_value_name == kInvalidValueName ||
      candidate.step_bytes == 0) {
    return;
  }
  const auto [it, inserted] = pointer_carriers.emplace(value_name, candidate);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (it->second.base_value_name == candidate.base_value_name &&
      it->second.byte_delta == candidate.byte_delta && it->second.step_bytes == candidate.step_bytes) {
    return;
  }
  if (candidate.base_value_name == value_name && candidate.byte_delta == 0) {
    if (it->second.base_value_name != value_name || it->second.byte_delta != 0 ||
        candidate.step_bytes < it->second.step_bytes || it->second.step_bytes == 0) {
      it->second = candidate;
      if (changed != nullptr) {
        *changed = true;
      }
    }
    return;
  }
  if (it->second.base_value_name == value_name && it->second.byte_delta == 0) {
    it->second = candidate;
    if (changed != nullptr) {
      *changed = true;
    }
  }
}

void maybe_update_slot_pointer_carrier(
    std::unordered_map<std::string, PreparedPointerCarrierState>& slot_pointer_carriers,
    std::string_view slot_name,
    const PreparedPointerCarrierState& candidate,
    bool* changed) {
  if (slot_name.empty() || candidate.base_value_name == kInvalidValueName || candidate.step_bytes == 0) {
    return;
  }
  const auto [it, inserted] =
      slot_pointer_carriers.emplace(std::string(slot_name), candidate);
  if (inserted) {
    if (changed != nullptr) {
      *changed = true;
    }
    return;
  }
  if (it->second.base_value_name == candidate.base_value_name &&
      it->second.byte_delta == candidate.byte_delta && it->second.step_bytes == candidate.step_bytes) {
    return;
  }
  it->second = candidate;
  if (changed != nullptr) {
    *changed = true;
  }
}

[[nodiscard]] PreparedPointerCarrierMap build_pointer_carrier_map(
    PreparedNameTables& names,
    const bir::Function& function,
    const PreparedAddressingFunction* function_addressing) {
  std::unordered_set<ValueNameId> explicit_pointer_defs;
  explicit_pointer_defs.reserve(function.params.size() + function.blocks.size() * 4U);
  for (const auto& param : function.params) {
    if (param.type != bir::TypeKind::Ptr) {
      continue;
    }
    if (const auto value_name = names.value_names.find(param.name); value_name != kInvalidValueName) {
      explicit_pointer_defs.insert(value_name);
    }
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto value_name = prepare_inst_result_value_name(names, inst);
      if (!value_name.has_value()) {
        continue;
      }
      const auto* result_type = std::visit(
          [&](const auto& typed_inst) -> const bir::TypeKind* {
            using InstT = std::decay_t<decltype(typed_inst)>;
            if constexpr (std::is_same_v<InstT, bir::BinaryInst> || std::is_same_v<InstT, bir::SelectInst> ||
                          std::is_same_v<InstT, bir::CastInst> || std::is_same_v<InstT, bir::PhiInst> ||
                          std::is_same_v<InstT, bir::LoadLocalInst> ||
                          std::is_same_v<InstT, bir::LoadGlobalInst>) {
              return &typed_inst.result.type;
            } else if constexpr (std::is_same_v<InstT, bir::CallInst>) {
              return typed_inst.result.has_value() ? &typed_inst.result->type : nullptr;
            } else {
              return static_cast<const bir::TypeKind*>(nullptr);
            }
          },
          inst);
      if (result_type != nullptr && *result_type == bir::TypeKind::Ptr) {
        explicit_pointer_defs.insert(*value_name);
      }
    }
  }

  std::unordered_map<ValueNameId, std::size_t> direct_step_by_value_name;
  if (function_addressing != nullptr) {
    for (const auto& access : function_addressing->accesses) {
      if (access.address.base_kind != PreparedAddressBaseKind::PointerValue ||
          !access.address.pointer_value_name.has_value()) {
        continue;
      }
      update_prepared_pointer_step(
          direct_step_by_value_name, *access.address.pointer_value_name, access.address.size_bytes, nullptr);
    }
  }

  PreparedPointerCarrierMap pointer_carriers;
  for (const auto& [value_name, step_bytes] : direct_step_by_value_name) {
    maybe_update_prepared_pointer_carrier(
        pointer_carriers,
        value_name,
        PreparedPointerCarrierState{
            .base_value_name = value_name,
            .byte_delta = 0,
            .step_bytes = step_bytes,
        },
        nullptr);
  }
  std::unordered_map<std::string, PreparedPointerCarrierState> slot_pointer_carriers;
  bool changed = true;
  std::size_t remaining_iterations = function.blocks.size() * 4U + 1U;
  while (changed && remaining_iterations-- != 0) {
    changed = false;
    for (const auto& block : function.blocks) {
      std::unordered_map<std::string, PreparedPointerCarrierState> last_loaded_pointer_by_slot;
      std::unordered_map<std::string, ValueNameId> last_loaded_pointer_name_by_slot;
      std::optional<PreparedPointerCarrierState> last_loaded_pointer_state;
      std::optional<ValueNameId> last_loaded_pointer_name;
      for (const auto& inst : block.insts) {
        if (const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
            load != nullptr && load->result.kind == bir::Value::Kind::Named &&
            load->result.type == bir::TypeKind::Ptr) {
          const auto result_name = names.value_names.find(load->result.name);
          if (result_name != kInvalidValueName) {
            if (!load->address.has_value()) {
              if (const auto carrier = resolve_prepared_pointer_carrier_state(
                      result_name, pointer_carriers, direct_step_by_value_name, load->slot_name,
                      slot_pointer_carriers);
                  carrier.has_value()) {
                maybe_update_prepared_pointer_carrier(pointer_carriers, result_name, *carrier, &changed);
                last_loaded_pointer_by_slot[load->slot_name] = *carrier;
                last_loaded_pointer_name_by_slot[load->slot_name] = result_name;
                last_loaded_pointer_state = *carrier;
                last_loaded_pointer_name = result_name;
              }
            } else if (const auto carrier = resolve_prepared_pointer_carrier_state(
                           result_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers);
                       carrier.has_value()) {
              last_loaded_pointer_state = *carrier;
              last_loaded_pointer_name = result_name;
            }
          }
          continue;
        }
        if (const auto* load = std::get_if<bir::LoadGlobalInst>(&inst);
            load != nullptr && load->result.kind == bir::Value::Kind::Named &&
            load->result.type == bir::TypeKind::Ptr) {
          const auto result_name = names.value_names.find(load->result.name);
          if (result_name != kInvalidValueName) {
            if (const auto carrier = resolve_prepared_pointer_carrier_state(
                    result_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers);
                carrier.has_value()) {
              last_loaded_pointer_state = *carrier;
              last_loaded_pointer_name = result_name;
            }
          }
          continue;
        }
        if (const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
            store != nullptr && store->value.kind == bir::Value::Kind::Named &&
            store->value.type == bir::TypeKind::Ptr) {
          const auto stored_value_name = names.value_names.find(store->value.name);
          if (stored_value_name == kInvalidValueName) {
            continue;
          }
          if (const auto stored_value_state = resolve_prepared_pointer_carrier_state(
                  stored_value_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers);
              stored_value_state.has_value() && !store->address.has_value()) {
            maybe_update_prepared_pointer_carrier(
                pointer_carriers, stored_value_name, *stored_value_state, &changed);
            maybe_update_slot_pointer_carrier(
                slot_pointer_carriers, store->slot_name, *stored_value_state, &changed);
          }
          if (explicit_pointer_defs.count(stored_value_name) != 0 ||
              resolve_prepared_pointer_carrier_state(
                  stored_value_name, pointer_carriers, direct_step_by_value_name, {}, slot_pointer_carriers)
                  .has_value()) {
            continue;
          }
          if (!store->address.has_value()) {
            const auto base_it = last_loaded_pointer_by_slot.find(store->slot_name);
            const auto base_name_it = last_loaded_pointer_name_by_slot.find(store->slot_name);
            if (base_it == last_loaded_pointer_by_slot.end() ||
                base_name_it == last_loaded_pointer_name_by_slot.end()) {
              continue;
            }
            auto derived_state = base_it->second;
            derived_state.base_value_name = base_name_it->second;
            derived_state.byte_delta = static_cast<std::int64_t>(derived_state.step_bytes);
            maybe_update_prepared_pointer_carrier(
                pointer_carriers, stored_value_name, derived_state, &changed);
            maybe_update_slot_pointer_carrier(
                slot_pointer_carriers, store->slot_name, derived_state, &changed);
            continue;
          }
          if (store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
              !last_loaded_pointer_state.has_value() || !last_loaded_pointer_name.has_value()) {
            continue;
          }
          auto derived_state = *last_loaded_pointer_state;
          derived_state.base_value_name = *last_loaded_pointer_name;
          derived_state.byte_delta = -static_cast<std::int64_t>(derived_state.step_bytes);
          maybe_update_prepared_pointer_carrier(
              pointer_carriers, stored_value_name, derived_state, &changed);
          continue;
        }
        if (const auto* store = std::get_if<bir::StoreGlobalInst>(&inst);
            store != nullptr && store->value.kind == bir::Value::Kind::Named &&
            store->value.type == bir::TypeKind::Ptr) {
          const auto stored_value_name = names.value_names.find(store->value.name);
          if (stored_value_name == kInvalidValueName ||
              explicit_pointer_defs.count(stored_value_name) != 0 || !store->address.has_value() ||
              store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
              !last_loaded_pointer_state.has_value() || !last_loaded_pointer_name.has_value()) {
            continue;
          }
          auto derived_state = *last_loaded_pointer_state;
          derived_state.base_value_name = *last_loaded_pointer_name;
          derived_state.byte_delta = -static_cast<std::int64_t>(derived_state.step_bytes);
          maybe_update_prepared_pointer_carrier(
              pointer_carriers, stored_value_name, derived_state, &changed);
        }
      }
    }
  }
  return pointer_carriers;
}

[[nodiscard]] PreparedMovePhase classify_prepared_move_phase(const PreparedMoveResolution& move) {
  switch (move.destination_kind) {
    case PreparedMoveDestinationKind::CallArgumentAbi:
      return PreparedMovePhase::BeforeCall;
    case PreparedMoveDestinationKind::CallResultAbi:
      return PreparedMovePhase::AfterCall;
    case PreparedMoveDestinationKind::FunctionReturnAbi:
      return PreparedMovePhase::BeforeReturn;
    case PreparedMoveDestinationKind::Value:
      return move.authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy
                 ? PreparedMovePhase::BlockEntry
                 : PreparedMovePhase::BeforeInstruction;
  }
  return PreparedMovePhase::BeforeInstruction;
}

void append_prepared_move_bundle(PreparedValueLocationFunction& function_locations,
                                 const PreparedMoveResolution& move) {
  const PreparedMovePhase phase = classify_prepared_move_phase(move);
  const auto existing = std::find_if(
      function_locations.move_bundles.begin(),
      function_locations.move_bundles.end(),
      [&](const PreparedMoveBundle& bundle) {
        return bundle.phase == phase && bundle.authority_kind == move.authority_kind &&
               bundle.block_index == move.block_index &&
               bundle.instruction_index == move.instruction_index &&
               bundle.source_parallel_copy_predecessor_label ==
                   move.source_parallel_copy_predecessor_label &&
               bundle.source_parallel_copy_successor_label ==
                   move.source_parallel_copy_successor_label;
      });
  if (existing != function_locations.move_bundles.end()) {
    existing->moves.push_back(move);
    return;
  }
  function_locations.move_bundles.push_back(PreparedMoveBundle{
      .function_name = function_locations.function_name,
      .phase = phase,
      .authority_kind = move.authority_kind,
      .block_index = move.block_index,
      .instruction_index = move.instruction_index,
      .source_parallel_copy_predecessor_label = move.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label = move.source_parallel_copy_successor_label,
      .moves = {move},
  });
}

void append_prepared_abi_binding(PreparedValueLocationFunction& function_locations,
                                 PreparedMovePhase phase,
                                 std::size_t block_index,
                                 std::size_t instruction_index,
                                 PreparedAbiBinding binding) {
  auto existing = std::find_if(
      function_locations.move_bundles.begin(),
      function_locations.move_bundles.end(),
      [&](const PreparedMoveBundle& bundle) {
        return bundle.phase == phase && bundle.block_index == block_index &&
               bundle.instruction_index == instruction_index;
      });
  if (existing == function_locations.move_bundles.end()) {
    function_locations.move_bundles.push_back(PreparedMoveBundle{
        .function_name = function_locations.function_name,
        .phase = phase,
        .block_index = block_index,
        .instruction_index = instruction_index,
        .moves = {},
        .abi_bindings = {},
    });
    existing = std::prev(function_locations.move_bundles.end());
  }

  const auto duplicate = std::find_if(
      existing->abi_bindings.begin(),
      existing->abi_bindings.end(),
      [&](const PreparedAbiBinding& existing_binding) {
        return existing_binding.destination_kind == binding.destination_kind &&
               existing_binding.destination_storage_kind == binding.destination_storage_kind &&
               existing_binding.destination_abi_index == binding.destination_abi_index &&
               existing_binding.destination_register_name == binding.destination_register_name &&
               existing_binding.destination_contiguous_width == binding.destination_contiguous_width &&
               existing_binding.destination_occupied_register_names ==
                   binding.destination_occupied_register_names &&
               existing_binding.destination_stack_offset_bytes ==
                   binding.destination_stack_offset_bytes;
      });
  if (duplicate != existing->abi_bindings.end()) {
    return;
  }
  existing->abi_bindings.push_back(std::move(binding));
}

[[nodiscard]] PreparedMoveStorageKind call_arg_storage_kind(const c4c::TargetProfile& target_profile,
                                                            const bir::CallInst& call,
                                                            std::size_t arg_index);
[[nodiscard]] std::optional<std::size_t> call_arg_destination_stack_offset_bytes(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index);
[[nodiscard]] std::optional<bir::CallArgAbiInfo> resolve_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index);
[[nodiscard]] PreparedMoveStorageKind call_result_storage_kind(const bir::CallInst& call);
[[nodiscard]] const PreparedRegallocValue* find_regalloc_value(
    const PreparedRegallocFunction& function,
    const PreparedNameTables& names,
    std::string_view value_name);

void append_prepared_call_abi_bindings(const PreparedNameTables& names,
                                       const c4c::TargetProfile& target_profile,
                                       const c4c::backend::bir::Function& function,
                                       const PreparedRegallocFunction& regalloc_function,
                                       PreparedValueLocationFunction& function_locations) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }

      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        const PreparedMoveStorageKind destination_storage_kind =
            call_arg_storage_kind(target_profile, *call, arg_index);
        if (destination_storage_kind == PreparedMoveStorageKind::None) {
          continue;
        }
        const auto& arg = call->args[arg_index];
        const auto arg_abi = resolve_call_arg_abi(target_profile, *call, arg_index);
        const auto abi_register_name =
            arg_abi.has_value()
                ? call_arg_destination_register_name(target_profile, *arg_abi, arg_index)
                : std::nullopt;
        const auto stack_offset = call_arg_destination_stack_offset_bytes(target_profile, *call, arg_index);
        const auto* source =
            arg.kind == bir::Value::Kind::Named
                ? find_regalloc_value(regalloc_function, names, arg.name)
                : nullptr;
        const std::size_t destination_contiguous_width =
            source != nullptr ? published_register_group_width(*source) : 1;
        const auto destination_occupied_register_names =
            destination_storage_kind == PreparedMoveStorageKind::Register &&
                    abi_register_name.has_value()
                ? call_arg_destination_register_names(target_profile,
                                                      source != nullptr ? source->register_class
                                                                         : PreparedRegisterClass::None,
                                                      arg_index,
                                                      *abi_register_name,
                                                      destination_contiguous_width)
                : std::vector<std::string>{};
        append_prepared_abi_binding(function_locations,
                                    PreparedMovePhase::BeforeCall,
                                    block_index,
                                    instruction_index,
                                    PreparedAbiBinding{
                                        .destination_kind = PreparedMoveDestinationKind::CallArgumentAbi,
                                        .destination_storage_kind = destination_storage_kind,
                                        .destination_abi_index = arg_index,
                                        .destination_register_name =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? abi_register_name
                                                : std::nullopt,
                                        .destination_contiguous_width =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? destination_contiguous_width
                                                : 1,
                                        .destination_occupied_register_names =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? destination_occupied_register_names
                                                : std::vector<std::string>{},
                                        .destination_stack_offset_bytes =
                                            destination_storage_kind == PreparedMoveStorageKind::StackSlot
                                                ? stack_offset
                                                : std::nullopt,
                                    });
        if (destination_storage_kind == PreparedMoveStorageKind::StackSlot &&
            arg_abi.has_value() &&
            arg_abi->type == bir::TypeKind::Ptr &&
            arg_abi->byval_copy &&
            abi_register_name.has_value()) {
          append_prepared_abi_binding(function_locations,
                                      PreparedMovePhase::BeforeCall,
                                      block_index,
                                      instruction_index,
                                      PreparedAbiBinding{
                                          .destination_kind =
                                              PreparedMoveDestinationKind::CallArgumentAbi,
                                          .destination_storage_kind =
                                              PreparedMoveStorageKind::Register,
                                          .destination_abi_index = arg_index,
                                          .destination_register_name =
                                              abi_register_name,
                                          .destination_contiguous_width = 1,
                                          .destination_occupied_register_names =
                                              std::vector<std::string>{*abi_register_name},
                                          .destination_stack_offset_bytes = std::nullopt,
                                      });
        }
      }

      const PreparedMoveStorageKind result_storage_kind = call_result_storage_kind(*call);
      if (result_storage_kind == PreparedMoveStorageKind::None) {
        continue;
      }
      const auto destination_register_name =
          call->result_abi.has_value()
              ? call_result_destination_register_name(target_profile, *call->result_abi)
              : std::nullopt;
      const auto* result =
          call->result.has_value() && call->result->kind == bir::Value::Kind::Named
              ? find_regalloc_value(regalloc_function, names, call->result->name)
              : nullptr;
      const std::size_t destination_contiguous_width =
          result != nullptr ? published_register_group_width(*result) : 1;
      append_prepared_abi_binding(function_locations,
                                  PreparedMovePhase::AfterCall,
                                  block_index,
                                  instruction_index,
                                  PreparedAbiBinding{
                                      .destination_kind = PreparedMoveDestinationKind::CallResultAbi,
                                      .destination_storage_kind = result_storage_kind,
                                      .destination_abi_index = std::nullopt,
                                      .destination_register_name = destination_register_name,
                                      .destination_contiguous_width =
                                          result_storage_kind == PreparedMoveStorageKind::Register
                                              ? destination_contiguous_width
                                              : 1,
                                      .destination_occupied_register_names =
                                          result_storage_kind == PreparedMoveStorageKind::Register &&
                                                  destination_register_name.has_value()
                                              ? call_result_destination_register_names(
                                                    target_profile,
                                                    result != nullptr ? result->register_class
                                                                      : PreparedRegisterClass::None,
                                                    *destination_register_name,
                                                    destination_contiguous_width)
                                              : std::vector<std::string>{},
                                  });
    }
  }
}

[[nodiscard]] PreparedValueLocationFunction build_prepared_value_location_function(
    PreparedNameTables& names,
    const c4c::TargetProfile& target_profile,
    const c4c::backend::bir::Function* function,
    const PreparedAddressingFunction* function_addressing,
    const PreparedRegallocFunction& regalloc_function) {
  PreparedValueLocationFunction function_locations{
      .function_name = regalloc_function.function_name,
      .value_homes = {},
      .move_bundles = {},
  };
  const auto pointer_carriers =
      function == nullptr ? PreparedPointerCarrierMap{}
                          : build_pointer_carrier_map(names, *function, function_addressing);
  function_locations.value_homes.reserve(regalloc_function.values.size());
  for (const auto& value : regalloc_function.values) {
    function_locations.value_homes.push_back(
        classify_prepared_value_home(names, target_profile, function, pointer_carriers, value));
  }
  for (const auto& move : regalloc_function.move_resolution) {
    append_prepared_move_bundle(function_locations, move);
  }
  if (function != nullptr) {
    append_prepared_call_abi_bindings(
        names, target_profile, *function, regalloc_function, function_locations);
  }
  return function_locations;
}

[[nodiscard]] std::size_t interval_start_sort_key(const PreparedRegallocValue& value) {
  if (!value.live_interval.has_value()) {
    return std::numeric_limits<std::size_t>::max();
  }
  return value.live_interval->start_point;
}

[[nodiscard]] const bir::Function* find_bir_function(const bir::Module& module,
                                                     const PreparedNameTables& names,
                                                     FunctionNameId function_name) {
  const std::string_view function_spelling = prepared_function_name(names, function_name);
  const auto it = std::find_if(module.functions.begin(),
                               module.functions.end(),
                               [function_spelling](const bir::Function& function) {
                                 return function.name == function_spelling;
                               });
  if (it == module.functions.end()) {
    return nullptr;
  }
  return &*it;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value(
    const PreparedRegallocFunction& function,
    const PreparedNameTables& names,
    std::string_view value_name) {
  const ValueNameId value_name_id = names.value_names.find(value_name);
  if (value_name_id == kInvalidValueName) {
    return nullptr;
  }
  const auto it = std::find_if(function.values.begin(),
                               function.values.end(),
                               [value_name_id](const PreparedRegallocValue& value) {
                                 return value.value_name == value_name_id;
                               });
  if (it == function.values.end()) {
    return nullptr;
  }
  return &*it;
}

[[nodiscard]] std::optional<std::size_t> find_instruction_index_for_named_result(
    const bir::Block& block,
    std::string_view value_name) {
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
    bool matches_result = std::visit(
        [&](const auto& inst) {
          using Inst = std::decay_t<decltype(inst)>;
          if constexpr (std::is_same_v<Inst, bir::BinaryInst> || std::is_same_v<Inst, bir::SelectInst> ||
                        std::is_same_v<Inst, bir::CastInst> || std::is_same_v<Inst, bir::LoadLocalInst> ||
                        std::is_same_v<Inst, bir::LoadGlobalInst>) {
            return inst.result.kind == bir::Value::Kind::Named && inst.result.name == value_name;
          } else if constexpr (std::is_same_v<Inst, bir::CallInst>) {
            return inst.result.has_value() && inst.result->kind == bir::Value::Kind::Named &&
                   inst.result->name == value_name;
          } else {
            return false;
          }
        },
        block.insts[instruction_index]);
    if (matches_result) {
      return instruction_index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::string phi_transfer_reason_prefix(const PreparedJoinTransfer& join_transfer,
                                                     bool uses_cycle_temp_source) {
  std::string prefix = join_transfer.kind == PreparedJoinTransferKind::LoopCarry
                           ? "phi_loop_carry"
                           : "phi_join";
  if (uses_cycle_temp_source) {
    prefix += "_cycle_temp";
  }
  return prefix;
}

[[nodiscard]] std::string phi_temp_save_reason(const PreparedJoinTransfer& join_transfer) {
  std::string prefix = phi_transfer_reason_prefix(join_transfer, false);
  prefix += "_save_destination_to_temp";
  return prefix;
}

void append_phi_move_resolution(const PreparedNameTables& names,
                                const bir::Function& function,
                                const PreparedControlFlowFunction& function_cf,
                                PreparedRegallocFunction& regalloc_function) {
  for (const auto& bundle : function_cf.parallel_copy_bundles) {
    const auto block_index = published_prepared_parallel_copy_execution_block_index(
        names, function, bundle);
    if (!block_index.has_value()) {
      continue;
    }

    for (std::size_t step_index = 0; step_index < bundle.steps.size(); ++step_index) {
      const auto& step = bundle.steps[step_index];
      if (step.move_index >= bundle.moves.size()) {
        continue;
      }

      const auto& move = bundle.moves[step.move_index];
      if (move.join_transfer_index >= function_cf.join_transfers.size() ||
          move.destination_value.kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto& join_transfer = function_cf.join_transfers[move.join_transfer_index];

      const auto* destination =
          find_regalloc_value(regalloc_function, names, move.destination_value.name);
      if (destination == nullptr || assigned_storage_kind(*destination) == PreparedMoveStorageKind::None) {
        continue;
      }

      if (step.kind == PreparedParallelCopyStepKind::SaveDestinationToTemp) {
        append_move_resolution_record(regalloc_function,
                                      *destination,
                                      *destination,
                                      *block_index,
                                      0,
                                      false,
                                      false,
                                      step_index,
                                      PreparedMoveResolutionOpKind::SaveDestinationToTemp,
                                      PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                                      phi_temp_save_reason(join_transfer),
                                      bundle.predecessor_label,
                                      bundle.successor_label);
        continue;
      }

      if (step.kind != PreparedParallelCopyStepKind::Move) {
        continue;
      }

      if (move.source_value.kind == bir::Value::Kind::Immediate &&
          move.source_value.type == bir::TypeKind::I32) {
        append_immediate_i32_move_resolution_record(
            regalloc_function,
            move.source_value,
            *destination,
            *block_index,
            0,
            step_index,
            PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
            phi_transfer_reason_prefix(join_transfer, false) + "_immediate_materialization",
            bundle.predecessor_label,
            bundle.successor_label);
        continue;
      }

      if (move.source_value.kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto* source = find_regalloc_value(regalloc_function, names, move.source_value.name);
      if (source == nullptr || assigned_storage_kind(*source) == PreparedMoveStorageKind::None) {
        continue;
      }
      const bool coalesced_by_assigned_storage = assigned_storage_matches(*source, *destination);

      append_move_resolution_record(
          regalloc_function,
          *source,
          *destination,
          *block_index,
          0,
          step.uses_cycle_temp_source,
          coalesced_by_assigned_storage,
          step_index,
          PreparedMoveResolutionOpKind::Move,
          PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
          storage_transfer_reason(
              phi_transfer_reason_prefix(join_transfer, step.uses_cycle_temp_source),
              *source,
              *destination),
          bundle.predecessor_label,
          bundle.successor_label);
    }
  }
}

[[nodiscard]] std::optional<BlockLabelId> resolve_existing_consumer_block_label_id(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Block& block) {
  if (block.label_id != kInvalidBlockLabel) {
    const std::string_view structured_label = bir_names.block_labels.spelling(block.label_id);
    if (!structured_label.empty()) {
      const BlockLabelId prepared_label_id = names.block_labels.find(structured_label);
      if (prepared_label_id != kInvalidBlockLabel) {
        return prepared_label_id;
      }
    }
  }
  return resolve_prepared_block_label_id(names, block.label);
}

void append_consumer_move_resolution(const PreparedNameTables& names,
                                     const bir::NameTables& bir_names,
                                     const bir::Function& function,
                                     const PreparedControlFlowFunction* function_cf,
                                     PreparedRegallocFunction& regalloc_function) {
  auto is_authoritative_select_materialized_join_result =
      [&](const bir::Block& block, const bir::SelectInst& select) {
        if (function_cf == nullptr || select.result.kind != bir::Value::Kind::Named) {
          return false;
        }

        const auto block_label_id = resolve_existing_consumer_block_label_id(names, bir_names, block);
        if (!block_label_id.has_value()) {
          return false;
        }

        const auto* join_transfer =
            find_prepared_join_transfer(names, *function_cf, *block_label_id, select.result.name);
        return join_transfer != nullptr &&
               effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
                   PreparedJoinTransferCarrierKind::SelectMaterialization;
      };

  auto append_for_instruction = [&](const bir::Value& result,
                                    std::initializer_list<const bir::Value*> operands,
                                    std::size_t block_index,
                                    std::size_t instruction_index) {
    if (result.kind != bir::Value::Kind::Named) {
      return;
    }

    const auto* destination = find_regalloc_value(regalloc_function, names, result.name);
    if (destination == nullptr) {
      return;
    }

    for (const bir::Value* operand : operands) {
      if (operand == nullptr || operand->kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto* source = find_regalloc_value(regalloc_function, names, operand->name);
      if (source == nullptr) {
        continue;
      }

      append_move_resolution_record(regalloc_function,
                                    *source,
                                    *destination,
                                    block_index,
                                    instruction_index,
                                    false,
                                    false,
                                    std::nullopt,
                                    PreparedMoveResolutionOpKind::Move,
                                    PreparedMoveAuthorityKind::None,
                                    storage_transfer_reason("consumer", *source, *destination));
    }
  };

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      std::visit(
          [&](const auto& inst) {
            using Inst = std::decay_t<decltype(inst)>;
            if constexpr (std::is_same_v<Inst, bir::BinaryInst>) {
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::SelectInst>) {
              // Select-materialized joins are already owned by published out-of-SSA
              // join-transfer authority. Re-adding them here would regress into
              // consumer-shaped reconstruction.
              if (is_authoritative_select_materialized_join_result(block, inst)) {
                return;
              }
              append_for_instruction(inst.result,
                                     {&inst.lhs, &inst.rhs, &inst.true_value, &inst.false_value},
                                     block_index,
                                     instruction_index);
            } else if constexpr (std::is_same_v<Inst, bir::CastInst>) {
              append_for_instruction(inst.result, {&inst.operand}, block_index, instruction_index);
            }
          },
          block.insts[instruction_index]);
    }
  }
}

[[nodiscard]] std::optional<bir::CallArgAbiInfo> resolve_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index) {
  if (arg_index < call.arg_abi.size()) {
    return call.arg_abi[arg_index];
  }
  if (arg_index >= call.arg_types.size()) {
    return std::nullopt;
  }
  return infer_call_arg_abi(target_profile, call.arg_types[arg_index]);
}

[[nodiscard]] PreparedMoveStorageKind call_arg_storage_kind(const c4c::TargetProfile& target_profile,
                                                            const bir::CallInst& call,
                                                            std::size_t arg_index) {
  const auto abi = resolve_call_arg_abi(target_profile, call, arg_index);
  if (!abi.has_value()) {
    return PreparedMoveStorageKind::None;
  }

  if (target_profile.arch == c4c::TargetArch::X86_64 &&
      abi->type == bir::TypeKind::Ptr &&
      abi->byval_copy &&
      !abi->sret_pointer &&
      call_arg_destination_register_name(target_profile, *abi, arg_index).has_value()) {
    return PreparedMoveStorageKind::Register;
  }

  if (abi->passed_on_stack || abi->byval_copy || abi->sret_pointer ||
      abi->primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (call_arg_destination_register_name(target_profile, *abi, arg_index).has_value()) {
    return PreparedMoveStorageKind::Register;
  }
  if (abi->type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::StackSlot;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] std::optional<std::size_t> call_arg_destination_stack_offset_bytes(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index) {
  if (target_profile.arch != c4c::TargetArch::X86_64 ||
      call_arg_storage_kind(target_profile, call, arg_index) != PreparedMoveStorageKind::StackSlot) {
    return std::nullopt;
  }

  std::size_t next_offset_bytes = 0;
  for (std::size_t candidate_index = 0; candidate_index < call.args.size(); ++candidate_index) {
    if (call_arg_storage_kind(target_profile, call, candidate_index) !=
        PreparedMoveStorageKind::StackSlot) {
      continue;
    }
    const auto abi = resolve_call_arg_abi(target_profile, call, candidate_index);
    if (!abi.has_value()) {
      return std::nullopt;
    }
    next_offset_bytes =
        align_up(next_offset_bytes, call_stack_argument_alignment_bytes(*abi));
    if (candidate_index == arg_index) {
      return next_offset_bytes;
    }
    next_offset_bytes += call_stack_argument_size_bytes(*abi);
  }
  return std::nullopt;
}

[[nodiscard]] PreparedMoveStorageKind call_result_storage_kind(const bir::CallInst& call) {
  if (!call.result.has_value() || !call.result_abi.has_value() || call.result->kind != bir::Value::Kind::Named) {
    return PreparedMoveStorageKind::None;
  }

  const auto& abi = *call.result_abi;
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] std::optional<bir::CallResultAbiInfo> infer_scalar_function_return_abi(
    const bir::Function& function) {
  if (function.return_type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallResultAbiInfo abi{
      .type = function.return_type,
      .primary_class = bir::AbiValueClass::None,
      .secondary_class = bir::AbiValueClass::None,
      .returned_in_memory = false,
  };
  switch (function.return_type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      abi.primary_class = bir::AbiValueClass::Sse;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.returned_in_memory = true;
      return abi;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] PreparedMoveStorageKind function_return_storage_kind(const bir::Function& function) {
  const auto return_abi = function.return_abi.has_value()
                              ? function.return_abi
                              : infer_scalar_function_return_abi(function);
  if (!return_abi.has_value()) {
    return PreparedMoveStorageKind::None;
  }
  const auto& abi = *return_abi;
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

void append_call_arg_move_resolution(const PreparedNameTables& names,
                                     const c4c::TargetProfile& target_profile,
                                     const bir::Function& function,
                                     PreparedRegallocFunction& regalloc_function) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }

      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        const auto& arg = call->args[arg_index];
        if (arg.kind != bir::Value::Kind::Named) {
          continue;
        }

        const auto* source = find_regalloc_value(regalloc_function, names, arg.name);
        if (source == nullptr) {
          continue;
        }

        const PreparedMoveStorageKind consumed_kind =
            call_arg_storage_kind(target_profile, *call, arg_index);
        const PreparedMoveStorageKind source_kind = assigned_storage_kind(*source);
        const auto arg_abi = resolve_call_arg_abi(target_profile, *call, arg_index);
        const auto abi_register_name =
            arg_abi.has_value()
                ? call_arg_destination_register_name(target_profile, *arg_abi, arg_index)
                : std::nullopt;
        const auto stack_offset = call_arg_destination_stack_offset_bytes(target_profile, *call, arg_index);
        const auto destination_register_name =
            consumed_kind == PreparedMoveStorageKind::Register ? abi_register_name : std::nullopt;
        const auto destination_stack_offset =
            consumed_kind == PreparedMoveStorageKind::StackSlot ? stack_offset : std::nullopt;
        const std::size_t destination_contiguous_width =
            consumed_kind == PreparedMoveStorageKind::Register
                ? published_register_group_width(*source)
                : 1;
        const auto destination_occupied_register_names =
            consumed_kind == PreparedMoveStorageKind::Register &&
                    destination_register_name.has_value()
                ? call_arg_destination_register_names(target_profile,
                                                      source->register_class,
                                                      arg_index,
                                                      *destination_register_name,
                                                      destination_contiguous_width)
                : std::vector<std::string>{};
        if (source_kind == PreparedMoveStorageKind::Register &&
            consumed_kind == PreparedMoveStorageKind::Register && source->assigned_register.has_value() &&
            destination_register_name == std::optional<std::string>{source->assigned_register->register_name}) {
          continue;
        }
        append_move_resolution_record(regalloc_function,
                                      source->value_id,
                                      source->value_id,
                                      PreparedMoveDestinationKind::CallArgumentAbi,
                                      source_kind,
                                      consumed_kind,
                                      arg_index,
                                      destination_register_name,
                                      destination_contiguous_width,
                                      destination_occupied_register_names,
                                      destination_stack_offset,
                                      block_index,
                                      instruction_index,
                                      false,
                                      false,
                                      std::nullopt,
                                      PreparedMoveResolutionOpKind::Move,
                                      PreparedMoveAuthorityKind::None,
                                      storage_transfer_reason("call_arg",
                                                              source_kind,
                                                              consumed_kind));
      }
    }
  }
}

void append_call_result_move_resolution(const PreparedNameTables& names,
                                        const c4c::TargetProfile& target_profile,
                                        const bir::Function& function,
                                        PreparedRegallocFunction& regalloc_function) {
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr || !call->result.has_value() || call->result->kind != bir::Value::Kind::Named) {
        continue;
      }

      const auto* result = find_regalloc_value(regalloc_function, names, call->result->name);
      if (result == nullptr) {
        continue;
      }

      const PreparedMoveStorageKind consumed_kind = call_result_storage_kind(*call);
      const PreparedMoveStorageKind source_kind = assigned_storage_kind(*result);
      const auto destination_register_name = call->result_abi.has_value()
                                                   ? call_result_destination_register_name(
                                                       target_profile, *call->result_abi)
                                                 : std::nullopt;
      const std::size_t destination_contiguous_width =
          consumed_kind == PreparedMoveStorageKind::Register
              ? published_register_group_width(*result)
              : 1;
      const auto destination_occupied_register_names =
          consumed_kind == PreparedMoveStorageKind::Register &&
                  destination_register_name.has_value()
              ? call_result_destination_register_names(target_profile,
                                                       result->register_class,
                                                       *destination_register_name,
                                                       destination_contiguous_width)
              : std::vector<std::string>{};
      if (source_kind == PreparedMoveStorageKind::Register &&
          consumed_kind == PreparedMoveStorageKind::Register && result->assigned_register.has_value() &&
          destination_register_name == std::optional<std::string>{result->assigned_register->register_name}) {
        continue;
      }
      append_move_resolution_record(regalloc_function,
                                    result->value_id,
                                    result->value_id,
                                    PreparedMoveDestinationKind::CallResultAbi,
                                    source_kind,
                                    consumed_kind,
                                    std::nullopt,
                                    destination_register_name,
                                    destination_contiguous_width,
                                    destination_occupied_register_names,
                                    std::nullopt,
                                    block_index,
                                    instruction_index,
                                    false,
                                    false,
                                    std::nullopt,
                                    PreparedMoveResolutionOpKind::Move,
                                    PreparedMoveAuthorityKind::None,
                                    storage_transfer_reason("call_result",
                                                            source_kind,
                                                            consumed_kind));
    }
  }
}

void append_return_move_resolution(const PreparedNameTables& names,
                                   const c4c::TargetProfile& target_profile,
                                   const bir::Function& function,
                                   PreparedRegallocFunction& regalloc_function) {
  const PreparedMoveStorageKind consumed_kind = function_return_storage_kind(function);
  if (consumed_kind == PreparedMoveStorageKind::None) {
    return;
  }
  const auto return_abi = function.return_abi.has_value()
                              ? function.return_abi
                              : infer_scalar_function_return_abi(function);

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    if (block.terminator.kind != bir::TerminatorKind::Return || !block.terminator.value.has_value() ||
        block.terminator.value->kind != bir::Value::Kind::Named) {
      continue;
    }

    const auto* source = find_regalloc_value(regalloc_function, names, block.terminator.value->name);
    if (source == nullptr) {
      continue;
    }

    const PreparedMoveStorageKind source_kind = assigned_storage_kind(*source);
    const auto destination_register_name = return_abi.has_value()
                                               ? call_result_destination_register_name(
                                                     target_profile, *return_abi)
                                               : std::nullopt;
    const std::size_t destination_contiguous_width =
        consumed_kind == PreparedMoveStorageKind::Register
            ? published_register_group_width(*source)
            : 1;
    const auto destination_occupied_register_names =
        consumed_kind == PreparedMoveStorageKind::Register &&
                destination_register_name.has_value()
            ? call_result_destination_register_names(target_profile,
                                                     source->register_class,
                                                     *destination_register_name,
                                                     destination_contiguous_width)
            : std::vector<std::string>{};
    const bool coalesced_by_assigned_storage =
        source_kind == PreparedMoveStorageKind::Register &&
        consumed_kind == PreparedMoveStorageKind::Register &&
        source->assigned_register.has_value() &&
        destination_register_name ==
            std::optional<std::string>{source->assigned_register->register_name};
    if (source_kind == PreparedMoveStorageKind::None) {
      append_unassigned_return_move_resolution_record(
          regalloc_function,
          *source,
          consumed_kind,
          destination_register_name,
          destination_contiguous_width,
          destination_occupied_register_names,
          block_index,
          block.insts.size(),
          "return_context_to_register");
      continue;
    }
    append_move_resolution_record(regalloc_function,
                                  source->value_id,
                                  source->value_id,
                                  PreparedMoveDestinationKind::FunctionReturnAbi,
                                  source_kind,
                                  consumed_kind,
                                  std::nullopt,
                                  destination_register_name,
                                  destination_contiguous_width,
                                  destination_occupied_register_names,
                                  std::nullopt,
                                  block_index,
                                  block.insts.size(),
                                  false,
                                  coalesced_by_assigned_storage,
                                  std::nullopt,
                                  PreparedMoveResolutionOpKind::Move,
                                  PreparedMoveAuthorityKind::None,
                                  storage_transfer_reason("return",
                                                          source_kind,
                                                          consumed_kind));
  }
}

void append_spill_reload_ops(const PreparedLivenessFunction& liveness_function,
                             const std::vector<std::optional<std::size_t>>& spill_points,
                             PreparedRegallocFunction& regalloc_function) {
  for (std::size_t value_index = 0; value_index < regalloc_function.values.size(); ++value_index) {
    const auto spill_point = spill_points[value_index];
    const auto& value = regalloc_function.values[value_index];
    if (!spill_point.has_value() || !value.assigned_stack_slot.has_value()) {
      continue;
    }
    const auto& published_register = value.assigned_register.has_value()
                                         ? value.assigned_register
                                         : value.spill_register_authority;

    if (const auto spill_location = locate_program_point(liveness_function, *spill_point);
        spill_location.has_value()) {
      regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
          .value_id = value.value_id,
          .op_kind = PreparedSpillReloadOpKind::Spill,
          .block_index = spill_location->block_index,
          .instruction_index = spill_location->instruction_index,
          .register_bank = register_bank_from_class(value.register_class),
          .register_name = published_register.has_value()
                               ? std::optional<std::string>{published_register->register_name}
                               : std::nullopt,
          .contiguous_width = published_register.has_value()
                                  ? published_register->contiguous_width
                                  : std::max<std::size_t>(value.register_group_width, 1),
          .occupied_register_names = published_register.has_value()
                                         ? published_register->occupied_register_names
                                         : std::vector<std::string>{},
          .slot_id = value.assigned_stack_slot.has_value()
                         ? std::optional<PreparedFrameSlotId>{value.assigned_stack_slot->slot_id}
                         : std::nullopt,
          .stack_offset_bytes = value.assigned_stack_slot.has_value()
                                    ? std::optional<std::size_t>{
                                          value.assigned_stack_slot->offset_bytes}
                                    : std::nullopt,
      });
    }

    std::optional<std::size_t> last_reload_point;
    for (const std::size_t use_point : liveness_function.values[value_index].use_points) {
      if (use_point <= *spill_point || last_reload_point == use_point) {
        continue;
      }
      if (const auto reload_location = locate_program_point(liveness_function, use_point);
          reload_location.has_value()) {
        regalloc_function.spill_reload_ops.push_back(PreparedSpillReloadOp{
            .value_id = value.value_id,
            .op_kind = PreparedSpillReloadOpKind::Reload,
            .block_index = reload_location->block_index,
            .instruction_index = reload_location->instruction_index,
            .register_bank = register_bank_from_class(value.register_class),
            .register_name = published_register.has_value()
                                 ? std::optional<std::string>{published_register->register_name}
                                 : std::nullopt,
            .contiguous_width = published_register.has_value()
                                    ? published_register->contiguous_width
                                    : std::max<std::size_t>(value.register_group_width, 1),
            .occupied_register_names = published_register.has_value()
                                           ? published_register->occupied_register_names
                                           : std::vector<std::string>{},
            .slot_id = value.assigned_stack_slot.has_value()
                           ? std::optional<PreparedFrameSlotId>{value.assigned_stack_slot->slot_id}
                           : std::nullopt,
            .stack_offset_bytes = value.assigned_stack_slot.has_value()
                                      ? std::optional<std::size_t>{
                                            value.assigned_stack_slot->offset_bytes}
                                      : std::nullopt,
        });
        last_reload_point = use_point;
      }
    }
  }
}

}  // namespace

void BirPreAlloc::run_regalloc() {
  prepared_.completed_phases.push_back("regalloc");
  prepared_.regalloc.functions.clear();
  prepared_.regalloc.functions.reserve(prepared_.liveness.functions.size());
  prepared_.value_locations.functions.clear();
  prepared_.value_locations.functions.reserve(prepared_.liveness.functions.size());

  for (const auto& liveness_function : prepared_.liveness.functions) {
    PreparedRegallocFunction regalloc_function{
        .function_name = liveness_function.function_name,
        .values = {},
        .constraints = {},
        .interference = {},
        .move_resolution = {},
        .spill_reload_ops = {},
    };
    regalloc_function.values.reserve(liveness_function.values.size());
    regalloc_function.constraints.reserve(liveness_function.values.size());
    const auto* function =
        find_bir_function(prepared_.module, prepared_.names, liveness_function.function_name);

    for (const auto& liveness_value : liveness_function.values) {
      const PreparedRegisterClass register_class = resolve_register_class(prepared_, liveness_value);
      const std::size_t register_group_width =
          resolve_register_group_width(prepared_, liveness_value);
      const bool eligible_for_register_seed =
          register_class != PreparedRegisterClass::None &&
          liveness_value.value_kind != PreparedValueKind::StackObject;
      const std::size_t base_priority = value_priority(liveness_value);
      const std::size_t weighted_use_priority = weighted_use_score(liveness_function, liveness_value);
      const std::size_t priority = weighted_use_priority +
                                   (base_priority >= liveness_value.use_points.size()
                                        ? base_priority - liveness_value.use_points.size()
                                        : 0U);
      const auto caller_saved_names = caller_saved_registers(prepared_.target_profile, register_class);
      const auto callee_saved_names = callee_saved_registers(prepared_.target_profile, register_class);

      regalloc_function.values.push_back(PreparedRegallocValue{
          .value_id = liveness_value.value_id,
          .stack_object_id = liveness_value.stack_object_id,
          .function_name = liveness_value.function_name,
          .value_name = liveness_value.value_name,
          .type = liveness_value.type,
          .value_kind = liveness_value.value_kind,
          .register_class = register_class,
          .register_group_width = register_group_width,
          .allocation_status = PreparedAllocationStatus::Unallocated,
          .spillable = eligible_for_register_seed && !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .crosses_call = liveness_value.crosses_call,
          .priority = priority,
          .spill_weight = static_cast<double>(priority),
          .live_interval = liveness_value.live_interval,
          .assigned_register = std::nullopt,
          .assigned_stack_slot = std::nullopt,
          .spill_register_authority = std::nullopt,
      });

      if (!eligible_for_register_seed) {
        continue;
      }

      regalloc_function.constraints.push_back(PreparedAllocationConstraint{
          .value_id = liveness_value.value_id,
          .register_class = register_class,
          .register_group_width = register_group_width,
          .requires_register = !liveness_value.requires_home_slot,
          .requires_home_slot = liveness_value.requires_home_slot,
          .cannot_cross_call = liveness_value.crosses_call &&
                               register_class != PreparedRegisterClass::Float,
          .fixed_register_name = std::nullopt,
          .preferred_register_names = materialize_register_names(liveness_value.crosses_call
                                                                    ? callee_saved_names
                                                                    : caller_saved_names),
          .forbidden_register_names = materialize_register_names(liveness_value.crosses_call
                                                                     ? caller_saved_names
                                                                     : std::vector<std::string_view>{}),
      });
    }

    for (std::size_t lhs_index = 0; lhs_index < regalloc_function.values.size(); ++lhs_index) {
      const auto& lhs = regalloc_function.values[lhs_index];
      if (!lhs.live_interval.has_value() || lhs.register_class == PreparedRegisterClass::None) {
        continue;
      }
      for (std::size_t rhs_index = lhs_index + 1U; rhs_index < regalloc_function.values.size(); ++rhs_index) {
        const auto& rhs = regalloc_function.values[rhs_index];
        if (!rhs.live_interval.has_value() || rhs.register_class == PreparedRegisterClass::None) {
          continue;
        }
        if (!intervals_overlap(*lhs.live_interval, *rhs.live_interval)) {
          continue;
        }
        regalloc_function.interference.push_back(PreparedInterferenceEdge{
            .lhs_value_id = lhs.value_id,
            .rhs_value_id = rhs.value_id,
            .reason = "overlapping_live_intervals",
        });
      }
    }

    PreparedFrameSlotId next_slot_id = 0;
    std::size_t next_offset_bytes = prepared_.stack_layout.frame_size_bytes;
    std::size_t frame_alignment_bytes = prepared_.stack_layout.frame_alignment_bytes;
    for (const auto& slot : prepared_.stack_layout.frame_slots) {
      next_slot_id = std::max(next_slot_id, slot.slot_id + 1U);
      next_offset_bytes = std::max(next_offset_bytes, slot.offset_bytes + slot.size_bytes);
      frame_alignment_bytes = std::max(frame_alignment_bytes, slot.align_bytes);
    }

    std::vector<std::size_t> allocation_order(regalloc_function.values.size());
    for (std::size_t index = 0; index < allocation_order.size(); ++index) {
      allocation_order[index] = index;
    }
    std::sort(allocation_order.begin(),
              allocation_order.end(),
              [&regalloc_function](std::size_t lhs_index, std::size_t rhs_index) {
                const auto& lhs = regalloc_function.values[lhs_index];
                const auto& rhs = regalloc_function.values[rhs_index];
                const std::size_t lhs_start = interval_start_sort_key(lhs);
                const std::size_t rhs_start = interval_start_sort_key(rhs);
                if (lhs_start != rhs_start) {
                  return lhs_start < rhs_start;
                }
                if (lhs.priority != rhs.priority) {
                  return lhs.priority > rhs.priority;
                }
                return lhs.value_id < rhs.value_id;
              });

    std::vector<ActiveRegisterAssignment> active_caller_saved_assignments;
    active_caller_saved_assignments.reserve(regalloc_function.values.size());
    std::vector<ActiveRegisterAssignment> active_callee_saved_assignments;
    active_callee_saved_assignments.reserve(regalloc_function.values.size());
    std::vector<std::optional<std::size_t>> spill_points(regalloc_function.values.size(), std::nullopt);
    std::size_t assigned_register_count = 0;
    std::size_t assigned_stack_count = 0;

    auto assign_from_pool = [&](const auto& should_consider,
                                const auto& register_pool_for_value,
                                std::vector<ActiveRegisterAssignment>& active_assignments,
                                const auto& can_evict_assignment) {
      for (const std::size_t value_index : allocation_order) {
        auto& value = regalloc_function.values[value_index];
        if (value.assigned_register.has_value() || value.assigned_stack_slot.has_value() ||
            value.requires_home_slot || !value.live_interval.has_value() ||
            value.register_class == PreparedRegisterClass::None || !should_consider(value)) {
          continue;
        }

        expire_completed_assignments(active_assignments, value.live_interval->start_point);
        const auto candidate_spans = register_pool_for_value(value);
        if (const auto chosen_register = choose_register_span(active_assignments, candidate_spans);
            chosen_register.has_value()) {
          value.assigned_register = PreparedPhysicalRegisterAssignment{
              .reg_class = value.register_class,
              .register_name = chosen_register->register_name,
              .contiguous_width = chosen_register->contiguous_width,
              .occupied_register_names = chosen_register->occupied_register_names,
          };
          value.allocation_status = PreparedAllocationStatus::AssignedRegister;
          active_assignments.push_back(ActiveRegisterAssignment{
              .value_index = value_index,
              .end_point = value.live_interval->end_point,
              .register_name = value.assigned_register->register_name,
              .occupied_register_names = value.assigned_register->occupied_register_names,
          });
          ++assigned_register_count;
          continue;
        }

        const auto eviction_index = choose_eviction_candidate(regalloc_function,
                                                              active_assignments,
                                                              candidate_spans,
                                                              value,
                                                              can_evict_assignment);
        if (!eviction_index.has_value()) {
          continue;
        }

        auto& evicted_value = regalloc_function.values[active_assignments[*eviction_index].value_index];
        spill_points[active_assignments[*eviction_index].value_index] = value.live_interval->start_point;
        value.assigned_register = PreparedPhysicalRegisterAssignment{
            .reg_class = value.register_class,
            .register_name = active_assignments[*eviction_index].register_name,
            .contiguous_width =
                regalloc_function.values[active_assignments[*eviction_index].value_index]
                    .assigned_register->contiguous_width,
            .occupied_register_names =
                regalloc_function.values[active_assignments[*eviction_index].value_index]
                    .assigned_register->occupied_register_names,
        };
        value.allocation_status = PreparedAllocationStatus::AssignedRegister;

        evicted_value.spill_register_authority = evicted_value.assigned_register;
        evicted_value.assigned_register.reset();
        evicted_value.allocation_status = PreparedAllocationStatus::Unallocated;

        active_assignments[*eviction_index] = ActiveRegisterAssignment{
            .value_index = value_index,
            .end_point = value.live_interval->end_point,
            .register_name = value.assigned_register->register_name,
            .occupied_register_names = value.assigned_register->occupied_register_names,
        };
      }
    };

    assign_from_pool([](const PreparedRegallocValue& value) { return value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return callee_saved_register_spans(prepared_.target_profile,
                                                          value.register_class,
                                                          value.register_group_width);
                     },
                     active_callee_saved_assignments,
                     [](const ActiveRegisterAssignment&) { return true; });
    assign_from_pool([](const PreparedRegallocValue& value) { return !value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return caller_saved_register_spans(prepared_.target_profile,
                                                          value.register_class,
                                                          value.register_group_width);
                     },
                     active_caller_saved_assignments,
                     [](const ActiveRegisterAssignment&) { return true; });
    assign_from_pool([](const PreparedRegallocValue& value) { return !value.crosses_call; },
                     [this](const PreparedRegallocValue& value) {
                       return callee_saved_register_spans(prepared_.target_profile,
                                                          value.register_class,
                                                          value.register_group_width);
                     },
                     active_callee_saved_assignments,
                     [&regalloc_function](const ActiveRegisterAssignment& assignment) {
                       return !regalloc_function.values[assignment.value_index].crosses_call;
                     });

    for (const std::size_t value_index : allocation_order) {
      auto& value = regalloc_function.values[value_index];

      if (value.assigned_register.has_value() || value.assigned_stack_slot.has_value()) {
        continue;
      }

      if (value.requires_home_slot) {
        value.assigned_stack_slot = allocate_stack_slot(value,
                                                        prepared_.stack_layout,
                                                        next_slot_id,
                                                        next_offset_bytes,
                                                        frame_alignment_bytes);
        value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
        ++assigned_stack_count;
        continue;
      }

      if (!value.live_interval.has_value() || value.register_class == PreparedRegisterClass::None) {
        if (normalized_value_size(value) != 0U) {
          value.assigned_stack_slot = allocate_stack_slot(value,
                                                          prepared_.stack_layout,
                                                          next_slot_id,
                                                          next_offset_bytes,
                                                          frame_alignment_bytes);
          value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
          ++assigned_stack_count;
        }
        continue;
      }

      value.assigned_stack_slot = allocate_stack_slot(value,
                                                      prepared_.stack_layout,
                                                      next_slot_id,
                                                      next_offset_bytes,
                                                      frame_alignment_bytes);
      value.allocation_status = PreparedAllocationStatus::AssignedStackSlot;
      ++assigned_stack_count;
    }

    append_spill_reload_ops(liveness_function, spill_points, regalloc_function);
    if (function != nullptr) {
      if (const auto* function_cf =
              find_prepared_control_flow_function(prepared_.control_flow, regalloc_function.function_name);
          function_cf != nullptr) {
        append_phi_move_resolution(prepared_.names, *function, *function_cf, regalloc_function);
        append_consumer_move_resolution(
            prepared_.names, prepared_.module.names, *function, function_cf, regalloc_function);
      } else {
        append_consumer_move_resolution(
            prepared_.names, prepared_.module.names, *function, nullptr, regalloc_function);
      }
      append_call_arg_move_resolution(
          prepared_.names, prepared_.target_profile, *function, regalloc_function);
      append_call_result_move_resolution(
          prepared_.names, prepared_.target_profile, *function, regalloc_function);
      append_return_move_resolution(
          prepared_.names, prepared_.target_profile, *function, regalloc_function);
    }

    prepared_.stack_layout.frame_size_bytes =
        std::max(prepared_.stack_layout.frame_size_bytes,
                 stack_layout::align_to(next_offset_bytes, std::max<std::size_t>(frame_alignment_bytes, 1U)));
    prepared_.stack_layout.frame_alignment_bytes =
        std::max(prepared_.stack_layout.frame_alignment_bytes, frame_alignment_bytes);

    prepared_.notes.push_back(PrepareNote{
        .phase = "regalloc",
        .message = "regalloc seeded function '" +
                   std::string(prepared_function_name(prepared_.names, regalloc_function.function_name)) +
                   "' with " +
                   std::to_string(regalloc_function.constraints.size()) +
                   " allocation constraint(s) and " +
                   std::to_string(regalloc_function.interference.size()) +
                   " interference edge(s) from active liveness; assigned " +
                   std::to_string(assigned_register_count) + " register(s) and " +
                   std::to_string(assigned_stack_count) + " stack slot(s)",
    });
    prepared_.value_locations.functions.push_back(build_prepared_value_location_function(
        prepared_.names,
        prepared_.target_profile,
        function,
        find_prepared_addressing_function(prepared_.addressing, regalloc_function.function_name),
        regalloc_function));
    prepared_.regalloc.functions.push_back(std::move(regalloc_function));
  }
}

}  // namespace c4c::backend::prepare
