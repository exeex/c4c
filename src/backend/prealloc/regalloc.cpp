#include "prealloc.hpp"
#include "regalloc/assignment.hpp"
#include "regalloc/call_moves.hpp"
#include "regalloc/call_return_abi.hpp"
#include "regalloc/classification.hpp"
#include "regalloc/consumer_moves.hpp"
#include "regalloc/intervals.hpp"
#include "regalloc/phi_moves.hpp"
#include "regalloc/runtime_helpers.hpp"
#include "regalloc/spill_reload.hpp"
#include "regalloc/stack_slots.hpp"
#include "regalloc/storage.hpp"
#include "regalloc/value_homes.hpp"
#include "regalloc/values.hpp"
#include "dynamic_stack.hpp"
#include "target_register_profile.hpp"
#include "stack_layout/stack_layout.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

namespace {

using regalloc_detail::append_f128_constant_values_for_function;
using regalloc_detail::append_call_arg_move_resolution;
using regalloc_detail::append_call_result_move_resolution;
using regalloc_detail::append_consumer_move_resolution;
using regalloc_detail::append_f128_runtime_helper_mappings;
using regalloc_detail::append_i128_runtime_helper_mappings;
using regalloc_detail::append_phi_move_resolution;
using regalloc_detail::append_return_move_resolution;
using regalloc_detail::append_spill_reload_ops;
using regalloc_detail::ActiveRegisterAssignment;
using regalloc_detail::choose_eviction_candidate;
using regalloc_detail::choose_register_span;
using regalloc_detail::build_prepared_value_homes;
using regalloc_detail::call_arg_destination_register_names;
using regalloc_detail::call_arg_destination_stack_offset_bytes;
using regalloc_detail::call_arg_storage_kind;
using regalloc_detail::call_result_destination_register_names;
using regalloc_detail::call_result_storage_kind;
using regalloc_detail::f128_call_arg_destination_placement;
using regalloc_detail::find_regalloc_value;
using regalloc_detail::interval_start_sort_key;
using regalloc_detail::intervals_overlap;
using regalloc_detail::materialize_register_names;
using regalloc_detail::materialize_register_placements;
using regalloc_detail::allocate_stack_slot;
using regalloc_detail::normalized_value_size;
using regalloc_detail::published_register_group_width;
using regalloc_detail::resolve_call_arg_abi;
using regalloc_detail::resolve_register_class;
using regalloc_detail::resolve_register_group_width;
using regalloc_detail::value_priority;
using regalloc_detail::weighted_use_score;

void expire_completed_assignments(std::vector<ActiveRegisterAssignment>& active,
                                  std::size_t start_point,
                                  bool preserve_call_boundary_pressure) {
  active.erase(std::remove_if(active.begin(),
                              active.end(),
                              [start_point, preserve_call_boundary_pressure](
                                  const ActiveRegisterAssignment& assignment) {
                                if (assignment.end_point < start_point) {
                                  return true;
                                }
                                return !preserve_call_boundary_pressure &&
                                       assignment.end_point == start_point;
                              }),
               active.end());
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

[[nodiscard]] PreparedFrameSlotId next_frame_slot_id(const PreparedStackLayout& stack_layout) {
  PreparedFrameSlotId next = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    next = std::max(next, slot.slot_id + 1U);
  }
  return next;
}

[[nodiscard]] bool function_has_dynamic_stack_operation(const bir::Function* function) {
  if (function == nullptr) {
    return false;
  }
  for (const auto& block : function->blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<bir::CallInst>(&inst);
      if (call == nullptr) {
        continue;
      }
      if (call->callee == "llvm.stacksave" || call->callee == "llvm.stackrestore" ||
          is_dynamic_alloca_call(call->callee)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] PreparedObjectId next_stack_object_id(const PreparedStackLayout& stack_layout) {
  PreparedObjectId next = 0;
  for (const auto& object : stack_layout.objects) {
    next = std::max(next, object.object_id + 1U);
  }
  return next;
}

[[nodiscard]] bool has_frame_slot_id(const PreparedStackLayout& stack_layout,
                                     PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::size_t function_frame_extent(const PreparedStackLayout& stack_layout,
                                                FunctionNameId function_name) {
  std::size_t extent = 0;
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name == function_name) {
      extent = std::max(extent, slot.offset_bytes + slot.size_bytes);
    }
  }
  return extent;
}

[[nodiscard]] std::size_t function_frame_alignment(const PreparedStackLayout& stack_layout,
                                                   FunctionNameId function_name) {
  std::size_t alignment = 1;
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.function_name == function_name) {
      alignment = std::max(alignment, slot.align_bytes);
    }
  }
  return alignment;
}

void publish_regalloc_stack_slots(PreparedStackLayout& stack_layout,
                                  const PreparedRegallocFunction& function) {
  PreparedObjectId next_object_id = next_stack_object_id(stack_layout);
  for (const auto& value : function.values) {
    if (!value.assigned_stack_slot.has_value() ||
        has_frame_slot_id(stack_layout, value.assigned_stack_slot->slot_id)) {
      continue;
    }
    const PreparedObjectId object_id = next_object_id++;
    const std::size_t size_bytes = value.assigned_stack_slot->size_bytes.value_or(0);
    const std::size_t align_bytes = value.assigned_stack_slot->align_bytes.value_or(1);
    stack_layout.objects.push_back(PreparedStackObject{
        .object_id = object_id,
        .function_name = function.function_name,
        .slot_name = std::nullopt,
        .value_name = value.value_name,
        .source_kind =
            value.requires_home_slot ? "regalloc.home_slot" : "regalloc.spill_slot",
        .type = value.type,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .address_exposed = false,
        .requires_home_slot = value.requires_home_slot,
        .permanent_home_slot = value.requires_home_slot,
    });
    stack_layout.frame_slots.push_back(PreparedFrameSlot{
        .slot_id = value.assigned_stack_slot->slot_id,
        .object_id = object_id,
        .function_name = function.function_name,
        .offset_bytes = value.assigned_stack_slot->offset_bytes,
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .fixed_location = false,
    });
  }
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
               existing_binding.destination_register_placement ==
                   binding.destination_register_placement &&
               existing_binding.destination_stack_offset_bytes ==
                   binding.destination_stack_offset_bytes;
      });
  if (duplicate != existing->abi_bindings.end()) {
    return;
  }
  existing->abi_bindings.push_back(std::move(binding));
}

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
        const auto abi_register_placement =
            destination_storage_kind == PreparedMoveStorageKind::Register && arg_abi.has_value()
                ? f128_call_arg_destination_placement(
                      call_arg_destination_register_placement(target_profile,
                                                             *arg_abi,
                                                             arg_index,
                                                             destination_contiguous_width),
                      arg.type)
                : std::nullopt;
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
                                        .destination_register_placement =
                                            destination_storage_kind == PreparedMoveStorageKind::Register
                                                ? abi_register_placement
                                                : std::nullopt,
                                    });
        if (arg_abi.has_value() &&
            arg_abi->type == bir::TypeKind::Ptr &&
            arg_abi->byval_copy &&
            abi_register_name.has_value()) {
          if (destination_storage_kind == PreparedMoveStorageKind::Register &&
              stack_offset.has_value()) {
            append_prepared_abi_binding(
                function_locations,
                PreparedMovePhase::BeforeCall,
                block_index,
                instruction_index,
                PreparedAbiBinding{
                    .destination_kind = PreparedMoveDestinationKind::CallArgumentAbi,
                    .destination_storage_kind = PreparedMoveStorageKind::StackSlot,
                    .destination_abi_index = arg_index,
                    .destination_register_name = std::nullopt,
                    .destination_contiguous_width = 1,
                    .destination_occupied_register_names = {},
                    .destination_stack_offset_bytes = stack_offset,
                    .destination_register_placement = std::nullopt,
                });
          }
          const auto byval_register_placement =
              call_arg_destination_register_placement(target_profile, *arg_abi, arg_index, 1);
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
                                          .destination_register_placement =
                                              byval_register_placement,
                                      });
        }
      }

      const PreparedMoveStorageKind result_storage_kind = call_result_storage_kind(*call);
      if (result_storage_kind == PreparedMoveStorageKind::None) {
        continue;
      }
      if (!call->result_lanes.empty()) {
        const auto base_destination_register_name =
            call->result_abi.has_value()
                ? call_result_destination_register_name(target_profile, *call->result_abi)
                : std::nullopt;
        const auto destination_register_names =
            result_storage_kind == PreparedMoveStorageKind::Register &&
                    base_destination_register_name.has_value() && call->result_abi.has_value()
                ? call_result_destination_register_names(
                      target_profile,
                      PreparedRegisterClass::Float,
                      *base_destination_register_name,
                      std::max<std::size_t>(call->result_abi->register_count,
                                            call->result_lanes.size()))
                : std::vector<std::string>{};
        for (std::size_t lane_index = 0; lane_index < call->result_lanes.size(); ++lane_index) {
          const auto& lane = call->result_lanes[lane_index];
          if (lane.kind != bir::Value::Kind::Named) {
            continue;
          }
          const auto destination_register_name =
              lane_index < destination_register_names.size()
                  ? std::optional<std::string>{destination_register_names[lane_index]}
                  : base_destination_register_name;
          auto destination_register_placement =
              result_storage_kind == PreparedMoveStorageKind::Register &&
                      call->result_abi.has_value()
                  ? call_result_destination_register_placement(target_profile, *call->result_abi, 1)
                  : std::nullopt;
          if (destination_register_placement.has_value()) {
            destination_register_placement->slot_index = lane_index;
          }
          append_prepared_abi_binding(
              function_locations,
              PreparedMovePhase::AfterCall,
              block_index,
              instruction_index,
              PreparedAbiBinding{
                  .destination_kind = PreparedMoveDestinationKind::CallResultAbi,
                  .destination_storage_kind = result_storage_kind,
                  .destination_abi_index = lane_index,
                  .destination_register_name = destination_register_name,
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names =
                      result_storage_kind == PreparedMoveStorageKind::Register &&
                              destination_register_name.has_value()
                          ? std::vector<std::string>{*destination_register_name}
                          : std::vector<std::string>{},
                  .destination_stack_offset_bytes = std::nullopt,
                  .destination_register_placement = destination_register_placement,
              });
        }
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
      const auto destination_register_placement =
          result_storage_kind == PreparedMoveStorageKind::Register &&
                  call->result_abi.has_value()
              ? call_result_destination_register_placement(target_profile,
                                                           *call->result_abi,
                                                           destination_contiguous_width)
              : std::nullopt;
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
                                      .destination_stack_offset_bytes = std::nullopt,
                                      .destination_register_placement =
                                          destination_register_placement,
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
  function_locations.value_homes = build_prepared_value_homes(names,
                                                             target_profile,
                                                             function,
                                                             function_addressing,
                                                             regalloc_function);
  for (const auto& move : regalloc_function.move_resolution) {
    append_prepared_move_bundle(function_locations, move);
  }
  if (function != nullptr) {
    append_prepared_call_abi_bindings(
        names, target_profile, *function, regalloc_function, function_locations);
  }
  return function_locations;
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

}  // namespace

void BirPreAlloc::run_regalloc() {
  prepared_.completed_phases.push_back("regalloc");
  prepared_.regalloc.functions.clear();
  prepared_.regalloc.functions.reserve(prepared_.liveness.functions.size());
  prepared_.value_locations.functions.clear();
  prepared_.value_locations.functions.reserve(prepared_.liveness.functions.size());
  prepared_.i128_runtime_helpers.functions.clear();

  PreparedValueId next_synthetic_value_id = 0;
  for (const auto& liveness_function : prepared_.liveness.functions) {
    for (const auto& value : liveness_function.values) {
      next_synthetic_value_id = std::max(next_synthetic_value_id, value.value_id + 1U);
    }
  }

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
    const bool has_dynamic_stack = function_has_dynamic_stack_operation(function);

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
      const auto caller_saved_spans =
          caller_saved_register_spans(prepared_.target_profile, register_class, register_group_width);
      const auto callee_saved_spans =
          callee_saved_register_spans(prepared_.target_profile, register_class, register_group_width);

      regalloc_function.values.push_back(PreparedRegallocValue{
          .value_id = liveness_value.value_id,
          .stack_object_id = liveness_value.stack_object_id,
          .function_name = liveness_value.function_name,
          .value_name = liveness_value.value_name,
          .type = liveness_value.type,
          .constant_f128_payload = std::nullopt,
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
          .fixed_register_placement = std::nullopt,
          .preferred_register_placements =
              materialize_register_placements(liveness_value.crosses_call ? callee_saved_spans
                                                                           : caller_saved_spans),
          .forbidden_register_placements =
              liveness_value.crosses_call
                  ? materialize_register_placements(caller_saved_spans)
                  : std::vector<PreparedRegisterPlacement>{},
      });
    }

    append_f128_constant_values_for_function(regalloc_function.values,
                                             prepared_.names,
                                             function,
                                             liveness_function.function_name,
                                             next_synthetic_value_id);

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

    PreparedFrameSlotId next_slot_id = next_frame_slot_id(prepared_.stack_layout);
    std::size_t next_offset_bytes =
        function_frame_extent(prepared_.stack_layout, regalloc_function.function_name);
    std::size_t frame_alignment_bytes =
        function_frame_alignment(prepared_.stack_layout, regalloc_function.function_name);

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

        const bool starts_at_call_point =
            std::find(liveness_function.call_points.begin(),
                      liveness_function.call_points.end(),
                      value.live_interval->start_point) != liveness_function.call_points.end();
        expire_completed_assignments(active_assignments,
                                     value.live_interval->start_point,
                                     starts_at_call_point || !has_dynamic_stack);
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

    publish_regalloc_stack_slots(prepared_.stack_layout, regalloc_function);

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
      append_i128_runtime_helper_mappings(prepared_.names,
                                          *function,
                                          regalloc_function,
                                          prepared_.i128_runtime_helpers);
      append_f128_runtime_helper_mappings(prepared_.names,
                                          *function,
                                          regalloc_function,
                                          prepared_.f128_runtime_helpers);
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
