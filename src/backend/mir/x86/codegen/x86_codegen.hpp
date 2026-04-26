#pragma once

#include "../x86.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace c4c::backend::x86 {

struct PreparedBoundedSameModuleHelperPrefixRender {
  std::string helper_prefix;
  std::unordered_set<std::string_view> helper_names;
  std::unordered_set<std::string_view> helper_string_names;
  std::unordered_set<std::string_view> helper_global_names;
};

[[nodiscard]] inline std::string render_prepared_stack_memory_operand(
    std::size_t byte_offset,
    std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

[[nodiscard]] inline std::string render_prepared_stack_address_expr(std::size_t byte_offset) {
  if (byte_offset == 0) {
    return "[rsp]";
  }
  return "[rsp + " + std::to_string(byte_offset) + "]";
}

[[nodiscard]] inline std::optional<std::string>
select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index) {
  if (function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* before_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
      *function_locations, c4c::backend::prepare::PreparedMovePhase::BeforeCall, block_index,
      instruction_index);
  if (before_call_bundle == nullptr) {
    return std::nullopt;
  }
  for (const auto& move : before_call_bundle->moves) {
    if (move.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        move.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::Register &&
        move.destination_abi_index == std::optional<std::size_t>{arg_index} &&
        move.destination_register_name.has_value()) {
      return *move.destination_register_name;
    }
  }
  for (const auto& binding : before_call_bundle->abi_bindings) {
    if (binding.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        binding.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::Register &&
        binding.destination_abi_index == std::optional<std::size_t>{arg_index} &&
        binding.destination_register_name.has_value()) {
      return *binding.destination_register_name;
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string>
select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index) {
  return select_prepared_call_argument_abi_register_if_supported(
      function_locations, 0, instruction_index, arg_index);
}

[[nodiscard]] inline std::optional<std::size_t>
select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index) {
  if (function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* before_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
      *function_locations, c4c::backend::prepare::PreparedMovePhase::BeforeCall, block_index,
      instruction_index);
  if (before_call_bundle == nullptr) {
    return std::nullopt;
  }
  for (const auto& move : before_call_bundle->moves) {
    if (move.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        move.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::StackSlot &&
        move.destination_abi_index == std::optional<std::size_t>{arg_index} &&
        move.destination_stack_offset_bytes.has_value()) {
      return move.destination_stack_offset_bytes;
    }
  }
  for (const auto& binding : before_call_bundle->abi_bindings) {
    if (binding.destination_kind ==
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
        binding.destination_storage_kind ==
            c4c::backend::prepare::PreparedMoveStorageKind::StackSlot &&
        binding.destination_abi_index == std::optional<std::size_t>{arg_index} &&
        binding.destination_stack_offset_bytes.has_value()) {
      return binding.destination_stack_offset_bytes;
    }
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::size_t>
select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index) {
  return select_prepared_call_argument_abi_stack_offset_if_supported(
      function_locations, 0, instruction_index, arg_index);
}

[[nodiscard]] inline std::optional<std::string>
render_prepared_trivial_defined_function_if_supported(
    const c4c::backend::bir::Function&,
    c4c::TargetArch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&,
    const std::function<std::string(const c4c::backend::bir::Function&)>&) {
  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedBoundedSameModuleHelperPrefixRender>
render_prepared_bounded_same_module_helper_prefix_if_supported(
    const c4c::backend::prepare::PreparedBirModule&,
    const std::vector<const c4c::backend::bir::Function*>&,
    const c4c::backend::bir::Function&,
    c4c::TargetArch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&,
    const std::function<std::string(const c4c::backend::bir::Function&)>&,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>&,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&,
                                                   std::size_t)>&,
    const std::function<std::string(std::string_view)>&,
    const std::function<std::string(std::string_view)>&) {
  return std::nullopt;
}

}  // namespace c4c::backend::x86
