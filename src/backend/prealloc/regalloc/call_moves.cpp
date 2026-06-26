#include "call_moves.hpp"

#include "call_return_abi.hpp"
#include "classification.hpp"
#include "move_records.hpp"
#include "storage.hpp"
#include "values.hpp"

#include "../target_register_profile.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace c4c::backend::prepare::regalloc_detail {

namespace {

struct RegallocValueNameIndex {
  std::unordered_map<ValueNameId, const PreparedRegallocValue*> by_name;
};

[[nodiscard]] RegallocValueNameIndex make_regalloc_value_name_index(
    const PreparedRegallocFunction& function) {
  RegallocValueNameIndex index;
  index.by_name.reserve(function.values.size());
  for (const auto& value : function.values) {
    if (value.value_name != kInvalidValueName) {
      index.by_name.emplace(value.value_name, &value);
    }
  }
  return index;
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value(
    const RegallocValueNameIndex& index,
    const PreparedNameTables& names,
    std::string_view value_name) {
  const ValueNameId value_name_id = names.value_names.find(value_name);
  if (value_name_id == kInvalidValueName) {
    return nullptr;
  }
  const auto it = index.by_name.find(value_name_id);
  return it == index.by_name.end() ? nullptr : it->second;
}

[[nodiscard]] bool same_register_destination(
    const std::optional<PreparedRegisterPlacement>& destination_placement,
    const PreparedPhysicalRegisterAssignment& assigned_register,
    const std::optional<std::string>& destination_register_name) {
  if (destination_placement.has_value() && assigned_register.placement.has_value()) {
    return *destination_placement == *assigned_register.placement;
  }
    return destination_register_name ==
           std::optional<std::string>{assigned_register.register_name};
  }

[[nodiscard]] std::size_t call_arg_destination_register_width(
    const c4c::TargetProfile& target_profile,
    const PreparedRegallocValue& source,
    const std::optional<bir::CallArgAbiInfo>& arg_abi) {
  if (target_profile.arch == c4c::TargetArch::Aarch64 &&
      arg_abi.has_value() &&
      arg_abi->type == bir::TypeKind::Ptr &&
      arg_abi->byval_copy &&
      arg_abi->passed_in_register &&
      !arg_abi->passed_on_stack &&
      arg_abi->primary_class == bir::AbiValueClass::Integer &&
      arg_abi->size_bytes > 0 &&
      arg_abi->size_bytes <= 16) {
    return std::max<std::size_t>((arg_abi->size_bytes + 7) / 8, 1);
  }
  return published_register_group_width(source);
}

[[nodiscard]] std::optional<PreparedRegisterPlacement> indexed_result_placement(
    const c4c::TargetProfile& target_profile,
    const bir::CallResultAbiInfo& abi,
    std::size_t lane_index) {
  auto placement = call_result_destination_register_placement(target_profile, abi, 1);
  if (placement.has_value()) {
    placement->slot_index = lane_index;
  }
  return placement;
}

[[nodiscard]] PreparedMoveStorageKind return_abi_storage_kind(
    const bir::CallResultAbiInfo& abi) {
  if (abi.returned_in_memory || abi.primary_class == bir::AbiValueClass::Memory) {
    return PreparedMoveStorageKind::StackSlot;
  }
  if (abi.type != bir::TypeKind::Void) {
    return PreparedMoveStorageKind::Register;
  }
  return PreparedMoveStorageKind::None;
}

void append_f128_constant_call_arg_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    PreparedMoveStorageKind consumed_kind,
    std::optional<std::size_t> destination_abi_index,
    std::optional<std::string> destination_register_name,
    std::size_t destination_contiguous_width,
    std::vector<std::string> destination_occupied_register_names,
    std::optional<std::size_t> destination_stack_offset_bytes,
    std::size_t block_index,
    std::size_t instruction_index,
    std::optional<PreparedRegisterPlacement> destination_register_placement) {
  if (!source.constant_f128_payload.has_value() ||
      consumed_kind == PreparedMoveStorageKind::None) {
    return;
  }

  const auto duplicate = std::find_if(
      regalloc_function.move_resolution.begin(),
      regalloc_function.move_resolution.end(),
      [&](const PreparedMoveResolution& move) {
        return move.from_value_id == source.value_id &&
               move.to_value_id == source.value_id &&
               move.destination_kind == PreparedMoveDestinationKind::CallArgumentAbi &&
               move.destination_storage_kind == consumed_kind &&
               move.destination_abi_index == destination_abi_index &&
               move.destination_register_name == destination_register_name &&
               move.destination_contiguous_width == destination_contiguous_width &&
               move.destination_occupied_register_names == destination_occupied_register_names &&
               move.destination_register_placement == destination_register_placement &&
               move.destination_stack_offset_bytes == destination_stack_offset_bytes &&
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
      .destination_kind = PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = consumed_kind,
      .destination_abi_index = destination_abi_index,
      .destination_register_name = std::move(destination_register_name),
      .destination_contiguous_width = destination_contiguous_width,
      .destination_occupied_register_names = std::move(destination_occupied_register_names),
      .destination_stack_offset_bytes = destination_stack_offset_bytes,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .uses_cycle_temp_source = false,
      .coalesced_by_assigned_storage = false,
      .source_parallel_copy_step_index = std::nullopt,
      .source_immediate_i32 = std::nullopt,
      .op_kind = PreparedMoveResolutionOpKind::Move,
      .authority_kind = PreparedMoveAuthorityKind::None,
      .reason = "f128_constant_call_arg_immediate_to_abi",
      .destination_register_placement = std::move(destination_register_placement),
  });
}

void append_unassigned_return_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    PreparedMoveStorageKind consumed_kind,
    std::optional<std::string> destination_register_name,
    std::size_t destination_contiguous_width,
    std::vector<std::string> destination_occupied_register_names,
    std::optional<PreparedRegisterPlacement> destination_register_placement,
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
               move.destination_register_placement == destination_register_placement &&
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
      .destination_register_placement = std::move(destination_register_placement),
  });
}

}  // namespace

void append_call_arg_move_resolution(const PreparedNameTables& names,
                                     const c4c::TargetProfile& target_profile,
                                     const bir::Function& function,
                                     PreparedRegallocFunction& regalloc_function) {
  const auto value_name_index = make_regalloc_value_name_index(regalloc_function);
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }

      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        const auto& arg = call->args[arg_index];
        const bool f128_constant_arg = is_f128_immediate_constant(arg);
        const auto* source =
            arg.kind == bir::Value::Kind::Named
                ? find_regalloc_value(value_name_index, names, arg.name)
                : f128_constant_arg
                      ? find_f128_constant_regalloc_value(regalloc_function, arg)
                      : nullptr;
        if (source == nullptr) {
          continue;
        }

        const PreparedMoveStorageKind consumed_kind =
            call_arg_storage_kind(target_profile, *call, arg_index);
        const PreparedMoveStorageKind source_kind = assigned_storage_kind(*source);
        const auto arg_abi = resolve_call_arg_abi(target_profile, *call, arg_index);
        const auto abi_register_index =
            call_arg_abi_register_index(target_profile, *call, arg_index);
        const auto abi_register_name =
            arg_abi.has_value() && abi_register_index.has_value()
                ? call_arg_destination_register_name(target_profile, *arg_abi, *abi_register_index)
                : std::nullopt;
        const auto stack_offset = call_arg_destination_stack_offset_bytes(target_profile, *call, arg_index);
        const auto destination_register_name =
            consumed_kind == PreparedMoveStorageKind::Register ? abi_register_name : std::nullopt;
        const auto destination_stack_offset =
            consumed_kind == PreparedMoveStorageKind::StackSlot ? stack_offset : std::nullopt;
          const std::size_t destination_contiguous_width =
              consumed_kind == PreparedMoveStorageKind::Register
                  ? call_arg_destination_register_width(target_profile, *source, arg_abi)
                  : 1;
        auto destination_register_placement =
            consumed_kind == PreparedMoveStorageKind::Register && arg_abi.has_value() &&
                    abi_register_index.has_value()
                ? f128_call_arg_destination_placement(
                      call_arg_destination_register_placement(target_profile,
                                                             *arg_abi,
                                                             *abi_register_index,
                                                             destination_contiguous_width),
                      arg.type)
                : std::nullopt;
        if (destination_register_placement.has_value() &&
            destination_register_placement->bank == PreparedRegisterBank::None &&
            arg_abi.has_value() &&
            arg_abi->type == bir::TypeKind::I16 &&
            arg_abi->primary_class == bir::AbiValueClass::Integer) {
          destination_register_placement->bank = PreparedRegisterBank::Gpr;
        }
          const auto destination_occupied_register_names =
              consumed_kind == PreparedMoveStorageKind::Register &&
                      destination_register_name.has_value() && abi_register_index.has_value()
                  ? call_arg_destination_register_names(target_profile,
                                                        source->register_class,
                                                        *abi_register_index,
                                                        *destination_register_name,
                                                        destination_contiguous_width)
                  : std::vector<std::string>{};
          const bool aarch64_byval_register_lanes =
              target_profile.arch == c4c::TargetArch::Aarch64 &&
              arg_abi.has_value() &&
              arg_abi->type == bir::TypeKind::Ptr &&
              arg_abi->byval_copy &&
              arg_abi->passed_in_register &&
              !arg_abi->passed_on_stack &&
              arg_abi->primary_class == bir::AbiValueClass::Integer &&
              arg_abi->size_bytes > 0 &&
              arg_abi->size_bytes <= 16 &&
              (consumed_kind == PreparedMoveStorageKind::Register ||
               consumed_kind == PreparedMoveStorageKind::StackSlot);
        if (f128_constant_arg) {
          append_f128_constant_call_arg_move_resolution_record(
              regalloc_function,
              *source,
              consumed_kind,
              arg_index,
              destination_register_name,
              destination_contiguous_width,
              destination_occupied_register_names,
              destination_stack_offset,
              block_index,
              instruction_index,
              destination_register_placement);
          continue;
        }
        if (source_kind == PreparedMoveStorageKind::Register &&
            consumed_kind == PreparedMoveStorageKind::Register && source->assigned_register.has_value() &&
            same_register_destination(destination_register_placement,
                                      *source->assigned_register,
                                      destination_register_name)) {
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
                                        aarch64_byval_register_lanes
                                            ? std::string{
                                                  "call_arg_byval_aggregate_register_lanes"}
                                            : storage_transfer_reason("call_arg",
                                                                      source_kind,
                                                                      consumed_kind),
                                        std::nullopt,
                                        std::nullopt,
                                        destination_register_placement);
      }
    }
  }
}

void append_call_result_move_resolution(const PreparedNameTables& names,
                                        const c4c::TargetProfile& target_profile,
                                        const bir::Function& function,
                                        PreparedRegallocFunction& regalloc_function) {
  const auto value_name_index = make_regalloc_value_name_index(regalloc_function);
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size(); ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&block.insts[instruction_index]);
      if (call == nullptr || !call->result.has_value() || call->result->kind != bir::Value::Kind::Named) {
        continue;
      }

      const PreparedMoveStorageKind consumed_kind = call_result_storage_kind(*call);
      const bool uses_explicit_result_lanes = !call->result_lanes.empty();
      const std::vector<bir::Value> result_lanes =
          uses_explicit_result_lanes ? call->result_lanes
                                     : std::vector<bir::Value>{*call->result};
      const auto base_destination_register_name =
          call->result_abi.has_value()
              ? call_result_destination_register_name(target_profile, *call->result_abi)
              : std::nullopt;
      const auto destination_register_names =
          uses_explicit_result_lanes &&
                  consumed_kind == PreparedMoveStorageKind::Register &&
                  base_destination_register_name.has_value() && call->result_abi.has_value()
              ? call_result_destination_register_names(target_profile,
                                                       call->result_abi->type == bir::TypeKind::F128
                                                           ? PreparedRegisterClass::Vector
                                                           : PreparedRegisterClass::Float,
                                                       *base_destination_register_name,
                                                       std::max<std::size_t>(
                                                           call->result_abi->register_count,
                                                           result_lanes.size()))
              : std::vector<std::string>{};
      for (std::size_t lane_index = 0; lane_index < result_lanes.size(); ++lane_index) {
        if (result_lanes[lane_index].kind != bir::Value::Kind::Named) {
          continue;
        }
        const auto* result =
            find_regalloc_value(value_name_index, names, result_lanes[lane_index].name);
        if (result == nullptr) {
          continue;
        }
        const PreparedMoveStorageKind source_kind = assigned_storage_kind(*result);
        const auto destination_register_name =
            uses_explicit_result_lanes && lane_index < destination_register_names.size()
                ? std::optional<std::string>{destination_register_names[lane_index]}
                : base_destination_register_name;
        const std::size_t destination_contiguous_width =
            uses_explicit_result_lanes ? 1 : published_register_group_width(*result);
        const auto destination_register_placement =
            consumed_kind == PreparedMoveStorageKind::Register && call->result_abi.has_value()
                ? (uses_explicit_result_lanes
                       ? indexed_result_placement(target_profile, *call->result_abi, lane_index)
                       : call_result_destination_register_placement(target_profile,
                                                                    *call->result_abi,
                                                                    destination_contiguous_width))
                : std::nullopt;
        const auto destination_occupied_register_names =
            consumed_kind == PreparedMoveStorageKind::Register &&
                    destination_register_name.has_value()
                ? (uses_explicit_result_lanes
                       ? std::vector<std::string>{*destination_register_name}
                       : call_result_destination_register_names(target_profile,
                                                                result->register_class,
                                                                *destination_register_name,
                                                                destination_contiguous_width))
                : std::vector<std::string>{};
        if (source_kind == PreparedMoveStorageKind::Register &&
            consumed_kind == PreparedMoveStorageKind::Register &&
            result->assigned_register.has_value() &&
            same_register_destination(destination_register_placement,
                                      *result->assigned_register,
                                      destination_register_name)) {
          continue;
        }
        append_move_resolution_record(regalloc_function,
                                      result->value_id,
                                      result->value_id,
                                      PreparedMoveDestinationKind::CallResultAbi,
                                      source_kind,
                                      consumed_kind,
                                      uses_explicit_result_lanes
                                          ? std::optional<std::size_t>{lane_index}
                                          : std::nullopt,
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
                                                              consumed_kind),
                                      std::nullopt,
                                      std::nullopt,
                                      destination_register_placement);
      }
    }
  }
}

void append_return_move_resolution(const PreparedNameTables& names,
                                   const c4c::TargetProfile& target_profile,
                                   const bir::Function& function,
                                   PreparedRegallocFunction& regalloc_function) {
  const auto value_name_index = make_regalloc_value_name_index(regalloc_function);
  auto return_abi = function.return_abi;
  PreparedMoveStorageKind consumed_kind = function_return_storage_kind(function);
  if (consumed_kind == PreparedMoveStorageKind::None && !return_abi.has_value()) {
    return_abi = direct_bir_function_return_move_repair(function);
    if (return_abi.has_value()) {
      consumed_kind = return_abi_storage_kind(*return_abi);
    }
  }
  if (consumed_kind == PreparedMoveStorageKind::None) {
    return;
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    if (block.terminator.kind != bir::TerminatorKind::Return || !block.terminator.value.has_value() ||
        block.terminator.value->kind != bir::Value::Kind::Named) {
      continue;
    }

    const bool uses_explicit_return_lanes = !block.terminator.return_lanes.empty();
    const std::vector<bir::Value> return_lanes =
        uses_explicit_return_lanes ? block.terminator.return_lanes
                                   : std::vector<bir::Value>{*block.terminator.value};
    const auto base_destination_register_name =
        return_abi.has_value()
            ? call_result_destination_register_name(target_profile, *return_abi)
            : std::nullopt;
    const auto destination_register_names =
        uses_explicit_return_lanes &&
                consumed_kind == PreparedMoveStorageKind::Register &&
                base_destination_register_name.has_value() && return_abi.has_value()
            ? call_result_destination_register_names(target_profile,
                                                     PreparedRegisterClass::Float,
                                                     *base_destination_register_name,
                                                     std::max<std::size_t>(
                                                         return_abi->register_count,
                                                         return_lanes.size()))
            : std::vector<std::string>{};
    for (std::size_t lane_index = 0; lane_index < return_lanes.size(); ++lane_index) {
      if (return_lanes[lane_index].kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto* source =
          find_regalloc_value(value_name_index, names, return_lanes[lane_index].name);
      if (source == nullptr) {
        continue;
      }

      const PreparedMoveStorageKind source_kind = assigned_storage_kind(*source);
      const auto destination_register_name =
          uses_explicit_return_lanes && lane_index < destination_register_names.size()
              ? std::optional<std::string>{destination_register_names[lane_index]}
              : base_destination_register_name;
      const std::size_t destination_contiguous_width =
          uses_explicit_return_lanes ? 1 : published_register_group_width(*source);
      const auto destination_register_placement =
          consumed_kind == PreparedMoveStorageKind::Register && return_abi.has_value()
              ? (uses_explicit_return_lanes
                     ? indexed_result_placement(target_profile, *return_abi, lane_index)
                     : call_result_destination_register_placement(target_profile,
                                                                  *return_abi,
                                                                  destination_contiguous_width))
              : std::nullopt;
      const auto destination_occupied_register_names =
          consumed_kind == PreparedMoveStorageKind::Register &&
                  destination_register_name.has_value()
              ? (uses_explicit_return_lanes
                     ? std::vector<std::string>{*destination_register_name}
                     : call_result_destination_register_names(target_profile,
                                                              source->register_class,
                                                              *destination_register_name,
                                                              destination_contiguous_width))
              : std::vector<std::string>{};
      const bool coalesced_by_assigned_storage =
          source_kind == PreparedMoveStorageKind::Register &&
          consumed_kind == PreparedMoveStorageKind::Register &&
          source->assigned_register.has_value() &&
          same_register_destination(destination_register_placement,
                                    *source->assigned_register,
                                    destination_register_name);
      if (source_kind == PreparedMoveStorageKind::None) {
        append_unassigned_return_move_resolution_record(
            regalloc_function,
            *source,
            consumed_kind,
            destination_register_name,
            destination_contiguous_width,
            destination_occupied_register_names,
            destination_register_placement,
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
                                    uses_explicit_return_lanes
                                        ? std::optional<std::size_t>{lane_index}
                                        : std::nullopt,
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
                                                            consumed_kind),
                                    std::nullopt,
                                    std::nullopt,
                                    destination_register_placement);
    }
  }
}

}  // namespace c4c::backend::prepare::regalloc_detail
