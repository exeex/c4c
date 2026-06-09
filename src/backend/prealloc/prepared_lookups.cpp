#include "prepared_lookups.hpp"

#include "module.hpp"

#include <algorithm>
#include <functional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace c4c::backend::prepare {

PreparedMoveBundleLookups::PreparedMoveBundleLookups() = default;
PreparedMoveBundleLookups::PreparedMoveBundleLookups(const PreparedMoveBundleLookups&) =
    default;
PreparedMoveBundleLookups::PreparedMoveBundleLookups(PreparedMoveBundleLookups&&) =
    default;
PreparedMoveBundleLookups& PreparedMoveBundleLookups::operator=(
    const PreparedMoveBundleLookups&) = default;
PreparedMoveBundleLookups& PreparedMoveBundleLookups::operator=(
    PreparedMoveBundleLookups&&) = default;
PreparedMoveBundleLookups::~PreparedMoveBundleLookups() = default;

namespace {

[[nodiscard]] std::optional<std::size_t> prepared_block_index_by_label(
    const PreparedControlFlowFunction& function,
    BlockLabelId label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].block_label == label) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> prepared_block_successors(
    const PreparedControlFlowFunction& function,
    const PreparedControlFlowBlock& block) {
  std::vector<std::size_t> successors;
  auto append_label = [&](BlockLabelId label) {
    if (label == kInvalidBlockLabel) {
      return;
    }
    const auto index = prepared_block_index_by_label(function, label);
    if (index.has_value() &&
        std::find(successors.begin(), successors.end(), *index) == successors.end()) {
      successors.push_back(*index);
    }
  };

  if (block.terminator_kind == bir::TerminatorKind::Branch) {
    append_label(block.branch_target_label);
  } else if (block.terminator_kind == bir::TerminatorKind::CondBranch) {
    append_label(block.true_label);
    append_label(block.false_label);
  }
  return successors;
}

[[nodiscard]] std::vector<std::vector<bool>> make_prepared_dominance_matrix(
    const PreparedControlFlowFunction& function) {
  const std::size_t count = function.blocks.size();
  std::vector<std::vector<std::size_t>> predecessors(count);
  for (std::size_t index = 0; index < count; ++index) {
    for (const auto successor : prepared_block_successors(function, function.blocks[index])) {
      if (successor < count) {
        predecessors[successor].push_back(index);
      }
    }
  }

  std::vector<std::vector<bool>> dominates(count, std::vector<bool>(count, true));
  if (count != 0) {
    std::fill(dominates.front().begin(), dominates.front().end(), false);
    dominates.front().front() = true;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t index = 1; index < count; ++index) {
      std::vector<bool> next(count, !predecessors[index].empty());
      if (predecessors[index].empty()) {
        std::fill(next.begin(), next.end(), false);
      } else {
        for (const auto predecessor : predecessors[index]) {
          for (std::size_t candidate = 0; candidate < count; ++candidate) {
            next[candidate] = next[candidate] && dominates[predecessor][candidate];
          }
        }
      }
      next[index] = true;
      if (next != dominates[index]) {
        dominates[index] = std::move(next);
        changed = true;
      }
    }
  }
  return dominates;
}

[[nodiscard]] bool prepared_block_dominates(const PreparedControlFlowFunction& function,
                                            std::size_t dominator_index,
                                            std::size_t dominated_index) {
  const auto dominates = make_prepared_dominance_matrix(function);
  return dominated_index < dominates.size() &&
         dominator_index < dominates[dominated_index].size() &&
         dominates[dominated_index][dominator_index];
}

[[nodiscard]] bool prior_stack_preserved_reaches_call(
    const std::vector<PreparedPriorPreservedValueEntry>& prior_entries,
    const std::vector<std::vector<bool>>& dominates,
    const PreparedCallPlan& call) {
  for (const auto& prior : prior_entries) {
    if (prior.preserved == nullptr ||
        prior.preserved->route != PreparedCallPreservationRoute::StackSlot) {
      continue;
    }
    if (prior.block_index == call.block_index) {
      if (prior.instruction_index < call.instruction_index) {
        return true;
      }
      continue;
    }
    if (call.block_index < dominates.size() &&
        prior.block_index < dominates[call.block_index].size() &&
        dominates[call.block_index][prior.block_index]) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<ValueNameId> existing_prepared_value_name_id(
    const PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == kInvalidValueName) {
    return std::nullopt;
  }
  return value_name;
}

[[nodiscard]] PreparedMoveStorageKind prepared_storage_kind_for_home(
    PreparedValueHomeKind kind) {
  switch (kind) {
    case PreparedValueHomeKind::Register:
      return PreparedMoveStorageKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedMoveStorageKind::StackSlot;
    case PreparedValueHomeKind::None:
    case PreparedValueHomeKind::RematerializableImmediate:
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedMoveStorageKind::None;
  }
  return PreparedMoveStorageKind::None;
}

[[nodiscard]] const PreparedMoveResolution* find_edge_publication_move(
    const PreparedValueLocationFunction* value_locations,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id,
    const PreparedMoveBundle** move_bundle) {
  if (move_bundle != nullptr) {
    *move_bundle = nullptr;
  }
  if (value_locations == nullptr ||
      predecessor_label == kInvalidBlockLabel ||
      successor_label == kInvalidBlockLabel) {
    return nullptr;
  }

  const std::optional<BlockLabelId> expected_predecessor{predecessor_label};
  const std::optional<BlockLabelId> expected_successor{successor_label};
  for (const auto& bundle : value_locations->move_bundles) {
    if (bundle.phase != PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_predecessor_label != expected_predecessor ||
        bundle.source_parallel_copy_successor_label != expected_successor) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.destination_kind == PreparedMoveDestinationKind::Value &&
          move.to_value_id == destination_value_id) {
        if (move_bundle != nullptr) {
          *move_bundle = &bundle;
        }
        return &move;
      }
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedParallelCopyStep* find_edge_publication_parallel_copy_step(
    const PreparedParallelCopyBundle* bundle,
    const PreparedMoveResolution* move,
    std::size_t* step_index) {
  if (step_index != nullptr) {
    *step_index = 0;
  }
  if (bundle == nullptr || move == nullptr ||
      !move->source_parallel_copy_step_index.has_value()) {
    return nullptr;
  }
  const auto index = *move->source_parallel_copy_step_index;
  if (index >= bundle->steps.size()) {
    return nullptr;
  }
  if (step_index != nullptr) {
    *step_index = index;
  }
  return &bundle->steps[index];
}

[[nodiscard]] const bir::Function* prepared_bir_function(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  if (function.function_name == kInvalidFunctionName) {
    return nullptr;
  }
  const std::string_view function_name =
      prepared_function_name(prepared.names, function.function_name);
  if (function_name.empty()) {
    return nullptr;
  }
  for (const auto& bir_function : prepared.module.functions) {
    if (std::string_view{bir_function.name} == function_name) {
      return &bir_function;
    }
  }
  return nullptr;
}

[[nodiscard]] const bir::Block* prepared_bir_block(
    const PreparedBirModule& prepared,
    const bir::Function& function,
    const PreparedControlFlowBlock& block) {
  if (block.block_label == kInvalidBlockLabel) {
    return nullptr;
  }
  const std::string_view block_label =
      prepared_block_label(prepared.names, block.block_label);
  if (block_label.empty()) {
    return nullptr;
  }
  for (const auto& bir_block : function.blocks) {
    if (bir_block.label_id != kInvalidBlockLabel &&
        prepared.module.names.block_labels.spelling(bir_block.label_id) == block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] BlockLabelId prepared_bir_block_label_id(const PreparedBirModule& prepared,
                                                       const bir::Block& block) {
  if (block.label_id != kInvalidBlockLabel) {
    const std::string_view label =
        prepared.module.names.block_labels.spelling(block.label_id);
    if (!label.empty()) {
      const BlockLabelId prepared_label = prepared.names.block_labels.find(label);
      if (prepared_label != kInvalidBlockLabel) {
        return prepared_label;
      }
    }
  }
  return prepared.names.block_labels.find(block.label);
}

void append_prepared_after_call_result_lane_binding(
    std::vector<PreparedAfterCallResultLaneBinding>& bindings,
    const PreparedNameTables& names,
    const PreparedValueHomeLookups& value_home_lookups,
    const PreparedMoveBundle& bundle,
    const PreparedAbiBinding& abi_binding,
    std::size_t lane_index,
    const bir::Value& value) {
  const auto value_name = existing_prepared_value_name_id(names, value);
  if (!value_name.has_value()) {
    return;
  }
  const auto value_id_it = value_home_lookups.value_ids.find(*value_name);
  if (value_id_it == value_home_lookups.value_ids.end()) {
    return;
  }
  bindings.push_back(PreparedAfterCallResultLaneBinding{
      .move_bundle = &bundle,
      .abi_binding = &abi_binding,
      .value_id = value_id_it->second,
      .value_name = *value_name,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .lane_index = lane_index,
  });
}

void publish_prepared_after_call_result_lane_bindings(
    PreparedMoveBundleLookups& lookups,
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups& value_home_lookups) {
  const auto* bir_function = prepared_bir_function(prepared, function);
  if (bir_function == nullptr || value_locations == nullptr) {
    return;
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size();
       ++block_index) {
    const auto* bir_block =
        prepared_bir_block(prepared, *bir_function, function.blocks[block_index]);
    if (bir_block == nullptr) {
      continue;
    }
    for (std::size_t instruction_index = 0;
         instruction_index < bir_block->insts.size();
         ++instruction_index) {
      const auto* call = std::get_if<bir::CallInst>(&bir_block->insts[instruction_index]);
      if (call == nullptr || !call->result.has_value()) {
        continue;
      }
      const auto* bundle =
          find_indexed_prepared_move_bundle(&lookups,
                                            value_locations,
                                            PreparedMovePhase::AfterCall,
                                            block_index,
                                            instruction_index);
      if (bundle == nullptr) {
        continue;
      }
      for (const auto& binding : bundle->abi_bindings) {
        if (binding.destination_kind != PreparedMoveDestinationKind::CallResultAbi ||
            binding.destination_storage_kind != PreparedMoveStorageKind::Register ||
            !binding.destination_abi_index.has_value()) {
          continue;
        }
        const std::size_t lane_index = *binding.destination_abi_index;
        if (lane_index == 0) {
          append_prepared_after_call_result_lane_binding(
              lookups.after_call_result_lane_bindings,
              prepared.names,
              value_home_lookups,
              *bundle,
              binding,
              lane_index,
              *call->result);
        }
        if (lane_index >= call->result_lanes.size()) {
          continue;
        }
        const auto& lane = call->result_lanes[lane_index];
        if (lane_index == 0 &&
            lane.kind == call->result->kind &&
            lane.name == call->result->name &&
            lane.type == call->result->type) {
          continue;
        }
        append_prepared_after_call_result_lane_binding(
            lookups.after_call_result_lane_bindings,
            prepared.names,
            value_home_lookups,
            *bundle,
            binding,
            lane_index,
            lane);
      }
    }
  }

  lookups.after_call_result_lane_bindings_by_position_and_value.reserve(
      lookups.after_call_result_lane_bindings.size());
  for (const auto& binding : lookups.after_call_result_lane_bindings) {
    const auto key =
        prepared_after_call_result_lane_position_key(binding.block_index,
                                                     binding.instruction_index,
                                                     binding.value_id);
    const auto [it, inserted] =
        lookups.after_call_result_lane_bindings_by_position_and_value.emplace(key,
                                                                              &binding);
    if (!inserted) {
      it->second = nullptr;
    }
  }
}

void apply_source_producer_fact(
    PreparedEdgePublication& publication,
    const PreparedEdgePublicationSourceProducerLookups& source_producers) {
  if (publication.source_value_kind == bir::Value::Kind::Immediate) {
    publication.source_producer_kind =
        PreparedEdgePublicationSourceProducerKind::Immediate;
    return;
  }
  if (publication.source_value_name == kInvalidValueName) {
    return;
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(&source_producers,
                                                             publication.source_value_name);
  if (producer == nullptr) {
    return;
  }
  if (producer->kind == PreparedEdgePublicationSourceProducerKind::Unknown) {
    return;
  }
  publication.source_producer_kind = producer->kind;
  if (producer->block_label != kInvalidBlockLabel) {
    publication.source_producer_block_label = producer->block_label;
  }
  publication.source_producer_instruction_index = producer->instruction_index;
  publication.source_load_local = producer->load_local;
  publication.source_load_global = producer->load_global;
  publication.source_cast = producer->cast;
  publication.source_binary = producer->binary;
  publication.source_select = producer->select;
}

[[nodiscard]] bool prepared_address_has_complete_base(const PreparedAddress& address) {
  switch (address.base_kind) {
    case PreparedAddressBaseKind::FrameSlot:
      return address.frame_slot_id.has_value();
    case PreparedAddressBaseKind::GlobalSymbol:
      return address.symbol_name.has_value();
    case PreparedAddressBaseKind::PointerValue:
      return address.pointer_value_name.has_value();
    case PreparedAddressBaseKind::StringConstant:
      return true;
    case PreparedAddressBaseKind::None:
      return false;
  }
  return false;
}

void copy_source_memory_access_fact(PreparedEdgePublication& publication,
                                    const PreparedMemoryAccess& access) {
  publication.source_memory_access = &access;
  publication.source_memory_base_kind = access.address.base_kind;
  publication.source_memory_frame_slot_id = access.address.frame_slot_id;
  publication.source_memory_symbol_name = access.address.symbol_name;
  publication.source_memory_pointer_value_name = access.address.pointer_value_name;
  publication.source_memory_byte_offset = access.address.byte_offset;
  publication.source_memory_size_bytes = access.address.size_bytes;
  publication.source_memory_align_bytes = access.address.align_bytes;
  publication.source_memory_address_space = access.address_space;
  publication.source_memory_is_volatile = access.is_volatile;
  publication.source_memory_can_use_base_plus_offset =
      access.address.can_use_base_plus_offset;
  publication.source_memory_requires_address_materialization =
      !access.address.can_use_base_plus_offset;
}

void apply_source_memory_access_fact(
    PreparedEdgePublication& publication,
    const PreparedAddressingFunction* function_addressing) {
  publication.source_memory_access_status =
      PreparedEdgePublicationSourceMemoryAccessStatus::Unavailable;
  if (publication.source_producer_kind !=
      PreparedEdgePublicationSourceProducerKind::LoadLocal) {
    return;
  }
  if (!publication.source_producer_block_label.has_value() ||
      !publication.source_producer_instruction_index.has_value() ||
      function_addressing == nullptr) {
    publication.source_memory_access_status =
        PreparedEdgePublicationSourceMemoryAccessStatus::MissingPreparedMemoryAccess;
    return;
  }

  const auto* access =
      find_prepared_memory_access(*function_addressing,
                                  *publication.source_producer_block_label,
                                  *publication.source_producer_instruction_index);
  if (access == nullptr) {
    publication.source_memory_access_status =
        PreparedEdgePublicationSourceMemoryAccessStatus::MissingPreparedMemoryAccess;
    return;
  }
  copy_source_memory_access_fact(publication, *access);
  const bool result_matches =
      access->result_value_name.has_value() &&
      access->result_value_name == publication.source_value_name;
  if (!result_matches || !prepared_address_has_complete_base(access->address) ||
      access->address.size_bytes == 0 || access->address.align_bytes == 0) {
    publication.source_memory_access_status =
        PreparedEdgePublicationSourceMemoryAccessStatus::IncompletePreparedMemoryAccess;
    return;
  }

  publication.source_memory_access_status =
      PreparedEdgePublicationSourceMemoryAccessStatus::Available;
}

[[nodiscard]] bool prepared_value_spelling_matches(const bir::Value& value,
                                                   std::string_view spelling) {
  return value.kind == bir::Value::Kind::Named && value.name == spelling;
}

[[nodiscard]] bool prepared_non_call_instruction_uses_value(const bir::Inst& inst,
                                                            std::string_view spelling) {
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return prepared_value_spelling_matches(binary->lhs, spelling) ||
           prepared_value_spelling_matches(binary->rhs, spelling);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
    return prepared_value_spelling_matches(select->lhs, spelling) ||
           prepared_value_spelling_matches(select->rhs, spelling) ||
           prepared_value_spelling_matches(select->true_value, spelling) ||
           prepared_value_spelling_matches(select->false_value, spelling);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    return prepared_value_spelling_matches(cast->operand, spelling);
  }
  if (const auto* store_global = std::get_if<bir::StoreGlobalInst>(&inst)) {
    return prepared_value_spelling_matches(store_global->value, spelling);
  }
  if (const auto* store_local = std::get_if<bir::StoreLocalInst>(&inst)) {
    return prepared_value_spelling_matches(store_local->value, spelling);
  }
  return false;
}

[[nodiscard]] bool prepared_terminator_uses_value(const bir::Terminator& terminator,
                                                  std::string_view spelling) {
  if (terminator.value.has_value() &&
      prepared_value_spelling_matches(*terminator.value, spelling)) {
    return true;
  }
  for (const auto& lane : terminator.return_lanes) {
    if (prepared_value_spelling_matches(lane, spelling)) {
      return true;
    }
  }
  return terminator.kind == bir::TerminatorKind::CondBranch &&
         prepared_value_spelling_matches(terminator.condition, spelling);
}

[[nodiscard]] bool prepared_branch_condition_uses_value(
    const PreparedControlFlowFunction& function,
    const PreparedControlFlowBlock& block,
    std::string_view spelling) {
  const auto* condition = find_prepared_branch_condition(function, block.block_label);
  if (condition == nullptr) {
    return false;
  }
  return prepared_value_spelling_matches(condition->condition_value, spelling) ||
         (condition->lhs.has_value() &&
          prepared_value_spelling_matches(*condition->lhs, spelling)) ||
         (condition->rhs.has_value() &&
          prepared_value_spelling_matches(*condition->rhs, spelling));
}

[[nodiscard]] bool prepared_block_entry_has_non_call_use(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function,
    const PreparedControlFlowBlock& block,
    const bir::Block& bir_block,
    ValueNameId value_name) {
  if (value_name == kInvalidValueName) {
    return false;
  }
  const std::string_view spelling = prepared_value_name(prepared.names, value_name);
  if (spelling.empty()) {
    return false;
  }
  for (const auto& inst : bir_block.insts) {
    if (std::holds_alternative<bir::CallInst>(inst)) {
      return false;
    }
    if (prepared_non_call_instruction_uses_value(inst, spelling)) {
      return true;
    }
  }
  return prepared_terminator_uses_value(bir_block.terminator, spelling) ||
         prepared_branch_condition_uses_value(function, block, spelling);
}

void collect_block_entry_republication_effects(
    PreparedCallPlanLookups& lookups,
    const PreparedBirModule& prepared,
    const PreparedCallPlansFunction& call_plans,
    const PreparedControlFlowFunction& function,
    const std::vector<std::vector<bool>>& dominates) {
  const auto* bir_function = prepared_bir_function(prepared, function);
  if (bir_function == nullptr) {
    return;
  }

  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    const auto& block = function.blocks[block_index];
    const auto* bir_block = prepared_bir_block(prepared, *bir_function, block);
    if (bir_block == nullptr) {
      continue;
    }

    std::vector<PreparedCallBoundaryEffectPlan> selected;
    for (const auto& call : call_plans.calls) {
      if (call.block_index == block_index ||
          block_index >= dominates.size() ||
          call.block_index >= dominates[block_index].size() ||
          !dominates[block_index][call.block_index]) {
        continue;
      }
      const auto effects = plan_prepared_call_boundary_effects(call, nullptr, nullptr);
      for (auto effect : effects) {
        if (effect.effect_kind !=
                PreparedCallBoundaryEffectKind::PreservationRepublication ||
            effect.preservation_route !=
                PreparedCallPreservationRoute::CalleeSavedRegister ||
            !effect.destination.value_id.has_value() ||
            !prepared_block_entry_has_non_call_use(
                prepared, function, block, *bir_block, effect.destination.value_name)) {
          continue;
        }
        effect.phase = PreparedMovePhase::BlockEntry;
        effect.block_index = block_index;
        effect.instruction_index = 0;
        effect.reason = "callee_saved_preservation_home_block_entry_republication";
        auto existing = std::find_if(
            selected.begin(),
            selected.end(),
            [&](const PreparedCallBoundaryEffectPlan& candidate) {
              return candidate.destination.value_id == effect.destination.value_id;
            });
        if (existing == selected.end()) {
          effect.order_index = selected.size();
          selected.push_back(std::move(effect));
        } else {
          effect.order_index = existing->order_index;
          *existing = std::move(effect);
        }
      }
    }
    if (!selected.empty()) {
      lookups.block_entry_republication_effects_by_block.emplace(block_index,
                                                                 std::move(selected));
    }
  }
}

[[nodiscard]] bool prepared_frame_address_object_is_addressable(
    const PreparedStackLayout& stack_layout,
    const PreparedFrameSlot& slot) {
  const auto* object = find_stack_object_by_id(stack_layout, slot.object_id);
  return object != nullptr &&
         (object->address_exposed || object->permanent_home_slot);
}

[[nodiscard]] const PreparedFrameSlot* prepared_frame_slot_for_access(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess* access) {
  if (access == nullptr ||
      access->address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return nullptr;
  }
  return find_frame_slot_by_id(stack_layout, *access->address.frame_slot_id);
}

[[nodiscard]] std::optional<std::int64_t> prepared_access_absolute_offset(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess* access) {
  const auto* slot = prepared_frame_slot_for_access(stack_layout, access);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return static_cast<std::int64_t>(slot->offset_bytes) +
         access->address.byte_offset;
}

[[nodiscard]] bool prepared_access_ranges_overlap(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess* lhs,
    const PreparedMemoryAccess* rhs) {
  const auto lhs_offset = prepared_access_absolute_offset(stack_layout, lhs);
  const auto rhs_offset = prepared_access_absolute_offset(stack_layout, rhs);
  if (!lhs_offset.has_value() || !rhs_offset.has_value() ||
      lhs == nullptr || rhs == nullptr) {
    return true;
  }
  const auto lhs_end = *lhs_offset +
                       static_cast<std::int64_t>(lhs->address.size_bytes);
  const auto rhs_end = *rhs_offset +
                       static_cast<std::int64_t>(rhs->address.size_bytes);
  return *lhs_offset < rhs_end && *rhs_offset < lhs_end;
}

[[nodiscard]] bool prepared_access_ranges_match(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess* lhs,
    const PreparedMemoryAccess* rhs) {
  const auto lhs_offset = prepared_access_absolute_offset(stack_layout, lhs);
  const auto rhs_offset = prepared_access_absolute_offset(stack_layout, rhs);
  return lhs_offset.has_value() &&
         rhs_offset.has_value() &&
         *lhs_offset == *rhs_offset &&
         lhs != nullptr &&
         rhs != nullptr &&
         lhs->address.size_bytes == rhs->address.size_bytes;
}

[[nodiscard]] std::optional<ValueNameId> prepared_existing_value_name_id(
    const PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  return value_name != kInvalidValueName ? std::optional<ValueNameId>{value_name}
                                         : std::nullopt;
}

[[nodiscard]] bool prepared_load_access_matches_result(
    const PreparedNameTables& names,
    const PreparedMemoryAccess* access,
    const bir::LoadLocalInst& load) {
  const auto result_name = prepared_existing_value_name_id(names, load.result);
  return result_name.has_value() &&
         access != nullptr &&
         access->result_value_name == result_name;
}

[[nodiscard]] bool prepared_global_load_access_matches_result(
    const PreparedNameTables& names,
    const PreparedMemoryAccess* access,
    const bir::LoadGlobalInst& load) {
  const auto result_name = prepared_existing_value_name_id(names, load.result);
  return result_name.has_value() &&
         access != nullptr &&
         access->result_value_name == result_name;
}

[[nodiscard]] bool prepared_store_access_matches_value(
    const PreparedNameTables& names,
    const PreparedMemoryAccess* access,
    const bir::StoreLocalInst& store) {
  if (access == nullptr) {
    return false;
  }
  const auto stored_name = prepared_existing_value_name_id(names, store.value);
  return stored_name.has_value() ? access->stored_value_name == stored_name
                                 : !access->stored_value_name.has_value();
}

[[nodiscard]] const bir::Value* prepared_lookup_source_producer_result(
    const PreparedEdgePublicationSourceProducer& producer) {
  switch (producer.kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local != nullptr ? &producer.load_local->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global != nullptr ? &producer.load_global->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast != nullptr ? &producer.cast->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary != nullptr ? &producer.binary->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select != nullptr ? &producer.select->result : nullptr;
    case PreparedEdgePublicationSourceProducerKind::Immediate:
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return nullptr;
  }
  return nullptr;
}

[[nodiscard]] bool prepared_source_producer_matches_instruction(
    const PreparedEdgePublicationSourceProducer& producer,
    const bir::Inst& inst) {
  switch (producer.kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local == std::get_if<bir::LoadLocalInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global == std::get_if<bir::LoadGlobalInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast == std::get_if<bir::CastInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary == std::get_if<bir::BinaryInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select == std::get_if<bir::SelectInst>(&inst);
    case PreparedEdgePublicationSourceProducerKind::Immediate:
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return false;
  }
  return false;
}

[[nodiscard]] const bir::Value* prepared_instruction_result_value_ref(
    const bir::Inst& inst) {
  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&inst)) {
    return &load_local->result;
  }
  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst)) {
    return &load_global->result;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    return &cast->result;
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return &binary->result;
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
    return &select->result;
  }
  if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
    return call->result.has_value() ? &*call->result : nullptr;
  }
  return nullptr;
}

[[nodiscard]] bool prepared_result_matches_value_name(
    const PreparedNameTables& names,
    const bir::Value& result,
    bir::TypeKind value_type,
    ValueNameId value_name) {
  if (result.kind != bir::Value::Kind::Named ||
      result.name.empty() ||
      result.type != value_type ||
      value_name == kInvalidValueName) {
    return false;
  }
  return names.value_names.find(result.name) == value_name;
}

[[nodiscard]] bool prepared_result_matches_value_name(
    const PreparedNameTables& names,
    const bir::Value& result,
    ValueNameId value_name) {
  if (result.kind != bir::Value::Kind::Named ||
      result.name.empty() ||
      value_name == kInvalidValueName) {
    return false;
  }
  return names.value_names.find(result.name) == value_name;
}

[[nodiscard]] std::optional<PreparedSameBlockScalarProducer>
prepared_same_block_source_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    ValueNameId value_name,
    bir::TypeKind value_type,
    std::size_t before_instruction_index) {
  if (source_producers == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      value_name == kInvalidValueName) {
    return std::nullopt;
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          source_producers, value_name);
  if (producer == nullptr ||
      producer->block_label != block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= block->insts.size()) {
    return std::nullopt;
  }
  const auto& inst = block->insts[producer->instruction_index];
  if (!prepared_source_producer_matches_instruction(*producer, inst)) {
    return std::nullopt;
  }
  const auto* producer_result = prepared_lookup_source_producer_result(*producer);
  const auto* instruction_result = prepared_instruction_result_value_ref(inst);
  if (producer_result == nullptr ||
      instruction_result == nullptr ||
      producer_result != instruction_result ||
      !prepared_result_matches_value_name(names,
                                          *instruction_result,
                                          value_type,
                                          value_name)) {
    return std::nullopt;
  }
  return PreparedSameBlockScalarProducer{
      .producer = *producer,
      .instruction = &inst,
      .instruction_index = producer->instruction_index,
      .value_name = value_name,
  };
}

[[nodiscard]] std::optional<std::int64_t> evaluate_prepared_same_block_integer_constant(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  if (depth > 4U ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = existing_prepared_value_name_id(names, value);
  const auto producer =
      value_name.has_value()
          ? prepared_same_block_source_producer(names,
                                                source_producers,
                                                block_label,
                                                block,
                                                *value_name,
                                                value.type,
                                                before_instruction_index)
          : std::nullopt;
  if (!producer.has_value() ||
      producer->producer.kind != PreparedEdgePublicationSourceProducerKind::Binary ||
      producer->producer.binary == nullptr) {
    return std::nullopt;
  }
  const auto* binary = producer->producer.binary;
  const auto lhs =
      evaluate_prepared_same_block_integer_constant(names,
                                                    source_producers,
                                                    block_label,
                                                    block,
                                                    binary->lhs,
                                                    producer->instruction_index,
                                                    depth + 1U);
  const auto rhs =
      evaluate_prepared_same_block_integer_constant(names,
                                                    source_producers,
                                                    block_label,
                                                    block,
                                                    binary->rhs,
                                                    producer->instruction_index,
                                                    depth + 1U);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  const auto lhs_value = *lhs;
  const auto rhs_value = *rhs;
  switch (binary->opcode) {
    case bir::BinaryOpcode::Add:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) +
                                       static_cast<std::uint64_t>(rhs_value));
    case bir::BinaryOpcode::Sub:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) -
                                       static_cast<std::uint64_t>(rhs_value));
    case bir::BinaryOpcode::Mul:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) *
                                       static_cast<std::uint64_t>(rhs_value));
    case bir::BinaryOpcode::And:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) &
                                       static_cast<std::uint64_t>(rhs_value));
    case bir::BinaryOpcode::Or:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) |
                                       static_cast<std::uint64_t>(rhs_value));
    case bir::BinaryOpcode::Xor:
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) ^
                                       static_cast<std::uint64_t>(rhs_value));
    case bir::BinaryOpcode::Shl:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value)
                                       << static_cast<unsigned>(rhs_value));
    case bir::BinaryOpcode::LShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) >>
                                       static_cast<unsigned>(rhs_value));
    case bir::BinaryOpcode::AShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return lhs_value >> static_cast<unsigned>(rhs_value);
    case bir::BinaryOpcode::SDiv:
      return rhs_value != 0 ? std::optional<std::int64_t>{lhs_value / rhs_value}
                            : std::nullopt;
    case bir::BinaryOpcode::UDiv:
      return rhs_value != 0
                 ? std::optional<std::int64_t>{static_cast<std::int64_t>(
                       static_cast<std::uint64_t>(lhs_value) /
                       static_cast<std::uint64_t>(rhs_value))}
                 : std::nullopt;
    case bir::BinaryOpcode::SRem:
      return rhs_value != 0 ? std::optional<std::int64_t>{lhs_value % rhs_value}
                            : std::nullopt;
    case bir::BinaryOpcode::URem:
      return rhs_value != 0
                 ? std::optional<std::int64_t>{static_cast<std::int64_t>(
                       static_cast<std::uint64_t>(lhs_value) %
                       static_cast<std::uint64_t>(rhs_value))}
                 : std::nullopt;
    case bir::BinaryOpcode::Eq:
      return lhs_value == rhs_value ? 1 : 0;
    case bir::BinaryOpcode::Ne:
      return lhs_value != rhs_value ? 1 : 0;
    case bir::BinaryOpcode::Slt:
      return lhs_value < rhs_value ? 1 : 0;
    case bir::BinaryOpcode::Sle:
      return lhs_value <= rhs_value ? 1 : 0;
    case bir::BinaryOpcode::Sgt:
      return lhs_value > rhs_value ? 1 : 0;
    case bir::BinaryOpcode::Sge:
      return lhs_value >= rhs_value ? 1 : 0;
    case bir::BinaryOpcode::Ult:
      return static_cast<std::uint64_t>(lhs_value) <
                     static_cast<std::uint64_t>(rhs_value)
                 ? 1
                 : 0;
    case bir::BinaryOpcode::Ule:
      return static_cast<std::uint64_t>(lhs_value) <=
                     static_cast<std::uint64_t>(rhs_value)
                 ? 1
                 : 0;
    case bir::BinaryOpcode::Ugt:
      return static_cast<std::uint64_t>(lhs_value) >
                     static_cast<std::uint64_t>(rhs_value)
                 ? 1
                 : 0;
    case bir::BinaryOpcode::Uge:
      return static_cast<std::uint64_t>(lhs_value) >=
                     static_cast<std::uint64_t>(rhs_value)
                 ? 1
                 : 0;
  }
  return std::nullopt;
}

}  // namespace

[[nodiscard]] std::size_t prepared_call_position_key(std::size_t block_index,
                                                     std::size_t instruction_index) {
  return (block_index << 32U) ^ instruction_index;
}

[[nodiscard]] std::size_t prepared_move_bundle_position_key(PreparedMovePhase phase,
                                                            std::size_t block_index,
                                                            std::size_t instruction_index) {
  return (static_cast<std::size_t>(phase) << 56U) ^
         (block_index << 32U) ^
         instruction_index;
}

[[nodiscard]] std::size_t prepared_call_argument_position_key(
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index) {
  std::size_t seed = block_index;
  seed ^= instruction_index + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  seed ^= abi_index + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  return seed;
}

[[nodiscard]] std::size_t prepared_after_call_result_lane_position_key(
    std::size_t block_index,
    std::size_t instruction_index,
    PreparedValueId value_id) {
  std::size_t seed = block_index;
  seed ^= instruction_index + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  seed ^= static_cast<std::size_t>(value_id) + 0x9e3779b97f4a7c15ULL +
          (seed << 6U) + (seed >> 2U);
  return seed;
}

[[nodiscard]] std::size_t prepared_before_return_abi_move_source_bank_key(
    std::size_t block_index,
    PreparedValueId source_value_id,
    PreparedRegisterBank destination_bank) {
  return (static_cast<std::size_t>(destination_bank) << 56U) ^
         (block_index << 32U) ^
         static_cast<std::size_t>(source_value_id);
}

[[nodiscard]] std::size_t prepared_return_chain_value_key(
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId value_name) {
  std::size_t seed = block_index;
  seed ^= instruction_index + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  seed ^= std::hash<ValueNameId>{}(value_name) + 0x9e3779b97f4a7c15ULL +
          (seed << 6U) + (seed >> 2U);
  return seed;
}

[[nodiscard]] std::size_t prepared_memory_access_position_key(
    BlockLabelId block_label,
    std::size_t instruction_index) {
  return (static_cast<std::size_t>(block_label) << 32U) ^ instruction_index;
}

[[nodiscard]] bool prepared_prior_preserved_value_entry_position_less(
    const PreparedPriorPreservedValueEntry& lhs,
    const PreparedPriorPreservedValueEntry& rhs) {
  if (lhs.block_index != rhs.block_index) {
    return lhs.block_index < rhs.block_index;
  }
  return lhs.instruction_index < rhs.instruction_index;
}

[[nodiscard]] bool prepared_prior_preserved_value_entry_same_position(
    const PreparedPriorPreservedValueEntry& lhs,
    const PreparedPriorPreservedValueEntry& rhs) {
  return lhs.block_index == rhs.block_index &&
         lhs.instruction_index == rhs.instruction_index;
}

[[nodiscard]] bool prepared_prior_preserved_value_entry_reaches_call(
    const PreparedPriorPreservedValueEntry& entry,
    const PreparedControlFlowFunction* control_flow,
    const PreparedCallPlan& current_call_plan) {
  if (entry.block_index == current_call_plan.block_index) {
    return entry.instruction_index < current_call_plan.instruction_index;
  }
  return control_flow != nullptr &&
         prepared_block_dominates(*control_flow, entry.block_index,
                                  current_call_plan.block_index);
}

[[nodiscard]] bool prepared_prior_preserved_value_has_complete_source(
    const PreparedCallPreservedValue& preserved) {
  switch (preserved.route) {
    case PreparedCallPreservationRoute::Unknown:
      return false;
    case PreparedCallPreservationRoute::CalleeSavedRegister:
      return preserved.register_name.has_value() &&
             preserved.register_bank.has_value() &&
             preserved.contiguous_width != 0 &&
             !preserved.occupied_register_names.empty() &&
             preserved.register_placement.has_value();
    case PreparedCallPreservationRoute::StackSlot:
      return preserved.value_name != kInvalidValueName &&
             preserved.slot_id.has_value() &&
             preserved.stack_offset_bytes.has_value() &&
             preserved.stack_size_bytes.has_value() &&
             *preserved.stack_size_bytes != 0;
  }
  return false;
}

[[nodiscard]] PreparedCallPlanLookups make_prepared_call_plan_lookups(
    const PreparedBirModule& prepared,
    const PreparedCallPlansFunction* call_plans,
    const PreparedControlFlowFunction& function) {
  PreparedCallPlanLookups lookups;
  if (call_plans == nullptr) {
    return lookups;
  }
  lookups.calls_by_position.reserve(call_plans->calls.size());
  lookups.first_stack_preserved_by_call_index.resize(call_plans->calls.size());
  const auto dominates = make_prepared_dominance_matrix(function);
  for (std::size_t call_index = 0; call_index < call_plans->calls.size(); ++call_index) {
    const auto& call = call_plans->calls[call_index];
    const auto call_position_key =
        prepared_call_position_key(call.block_index, call.instruction_index);
    lookups.calls_by_position.emplace(call_position_key, &call);
    if (call.outgoing_stack_argument_area.has_value()) {
      lookups.outgoing_stack_argument_areas_by_position.emplace(
          call_position_key, &*call.outgoing_stack_argument_area);
    }
    for (const auto& argument : call.arguments) {
      if (argument.source_encoding != PreparedStorageEncodingKind::Immediate ||
          !argument.source_literal.has_value()) {
        continue;
      }
      const auto key = prepared_call_argument_position_key(
          call.block_index, call.instruction_index, argument.arg_index);
      const auto [it, inserted] =
          lookups.immediate_arguments_by_position_and_abi.emplace(key, &argument);
      if (!inserted) {
        it->second = nullptr;
      }
    }
    for (const auto& preserved : call.preserved_values) {
      const bool stack_preserved =
          preserved.route == PreparedCallPreservationRoute::StackSlot &&
          preserved.value_name != kInvalidValueName &&
          preserved.slot_id.has_value() &&
          preserved.stack_offset_bytes.has_value() &&
          preserved.stack_size_bytes.has_value() &&
          *preserved.stack_size_bytes != 0;
      if (stack_preserved) {
        const auto has_reaching_prior =
            preserved.value_id < lookups.prior_preserved_by_value.size() &&
            prior_stack_preserved_reaches_call(
                lookups.prior_preserved_by_value[preserved.value_id], dominates, call);
        if (!has_reaching_prior) {
          lookups.first_stack_preserved_by_call_index[call_index].push_back(&preserved);
        }
      }
      if (preserved.route != PreparedCallPreservationRoute::Unknown) {
        if (preserved.value_id >= lookups.prior_preserved_by_value.size()) {
          lookups.prior_preserved_by_value.resize(preserved.value_id + 1U);
        }
        lookups.prior_preserved_by_value[preserved.value_id].push_back(
            PreparedPriorPreservedValueEntry{
                .block_index = call.block_index,
                .instruction_index = call.instruction_index,
                .preserved = &preserved,
            });
      }
    }
  }
  for (auto& entries : lookups.prior_preserved_by_value) {
    std::sort(entries.begin(), entries.end(),
              prepared_prior_preserved_value_entry_position_less);
  }
  collect_block_entry_republication_effects(
      lookups, prepared, *call_plans, function, dominates);
  return lookups;
}

[[nodiscard]] PreparedAddressMaterializationLookups
make_prepared_address_materialization_lookups(const PreparedBirModule& prepared,
                                              FunctionNameId function_name) {
  PreparedAddressMaterializationLookups lookups;
  const auto* addressing = find_prepared_addressing(prepared, function_name);
  if (addressing == nullptr) {
    return lookups;
  }
  lookups.materializations_by_block.reserve(addressing->address_materializations.size());
  for (const auto& materialization : addressing->address_materializations) {
    lookups.materializations_by_block[materialization.block_label].push_back(
        &materialization);
  }
  return lookups;
}

[[nodiscard]] PreparedMemoryAccessLookups make_prepared_memory_access_lookups(
    const PreparedAddressingFunction* addressing,
    const PreparedValueHomeLookups* value_home_lookups) {
  PreparedMemoryAccessLookups lookups;
  if (addressing == nullptr) {
    return lookups;
  }
  lookups.accesses_by_position.reserve(addressing->accesses.size());
  lookups.accesses_by_result_value_name.reserve(addressing->accesses.size());
  lookups.accesses_by_result_value_id.reserve(addressing->accesses.size());
  for (const auto& access : addressing->accesses) {
    if (access.block_label != kInvalidBlockLabel) {
      lookups.accesses_by_position.emplace(
          prepared_memory_access_position_key(access.block_label, access.inst_index),
          &access);
    }
    if (!access.result_value_name.has_value() ||
        *access.result_value_name == kInvalidValueName) {
      continue;
    }
    lookups.accesses_by_result_value_name[*access.result_value_name].push_back(&access);
    if (value_home_lookups == nullptr) {
      continue;
    }
    const auto value_id = find_indexed_prepared_value_id(
        value_home_lookups, nullptr, nullptr, *access.result_value_name);
    if (value_id.has_value()) {
      lookups.accesses_by_result_value_id[*value_id].push_back(&access);
    }
  }
  return lookups;
}

[[nodiscard]] PreparedMoveBundleLookups make_prepared_move_bundle_lookups(
    const PreparedValueLocationFunction* value_locations) {
  PreparedMoveBundleLookups lookups;
  if (value_locations == nullptr) {
    return lookups;
  }
  lookups.bundles_by_position.reserve(value_locations->move_bundles.size());
  for (const auto& bundle : value_locations->move_bundles) {
    lookups.bundles_by_position.emplace(
        prepared_move_bundle_position_key(bundle.phase, bundle.block_index,
                                          bundle.instruction_index),
        &bundle);
    if (bundle.phase == PreparedMovePhase::BeforeCall) {
      for (const auto& move : bundle.moves) {
        if (move.op_kind != PreparedMoveResolutionOpKind::Move ||
            move.destination_kind != PreparedMoveDestinationKind::CallArgumentAbi ||
            !move.destination_abi_index.has_value()) {
          continue;
        }
        const auto key = prepared_call_argument_position_key(
            bundle.block_index, bundle.instruction_index, *move.destination_abi_index);
        const auto [it, inserted] =
            lookups.before_call_argument_moves_by_position_and_abi.emplace(key, &move);
        if (!inserted) {
          it->second = nullptr;
        }
      }
    }
    if (bundle.phase != PreparedMovePhase::BeforeReturn) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.from_value_id == PreparedValueId{0} ||
          move.destination_kind != PreparedMoveDestinationKind::FunctionReturnAbi ||
          move.destination_storage_kind != PreparedMoveStorageKind::Register ||
          move.op_kind != PreparedMoveResolutionOpKind::Move ||
          !move.destination_register_placement.has_value() ||
          move.destination_register_placement->bank == PreparedRegisterBank::None) {
        continue;
      }
      const auto key = prepared_before_return_abi_move_source_bank_key(
          bundle.block_index,
          move.from_value_id,
          move.destination_register_placement->bank);
      const auto [it, inserted] =
          lookups.before_return_abi_moves_by_source_and_bank.emplace(key, &move);
      if (!inserted) {
        it->second = nullptr;
      }
    }
  }
  return lookups;
}

[[nodiscard]] bool prepared_return_chain_binary_opcode_is_scalar_publication(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::URem:
      return true;
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

[[nodiscard]] bool prepared_value_matches_name(const bir::Value& value,
                                               std::string_view name) {
  return value.kind == bir::Value::Kind::Named && value.name == name;
}

void publish_prepared_return_chain(
    PreparedReturnChainLookups& lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId chain_value_name,
    ValueNameId terminal_value_name,
    ValueNameId next_operand_value_name) {
  if (chain_value_name == kInvalidValueName ||
      terminal_value_name == kInvalidValueName) {
    return;
  }
  const auto key =
      prepared_return_chain_value_key(block_index, instruction_index, chain_value_name);
  const auto [it, inserted] =
      lookups.terminal_return_values_by_chain_value.emplace(key, terminal_value_name);
  if (!inserted && it->second != terminal_value_name) {
    it->second = kInvalidValueName;
  }
  if (next_operand_value_name == kInvalidValueName) {
    return;
  }
  const auto [next_it, next_inserted] =
      lookups.next_operand_values_by_chain_value.emplace(key, next_operand_value_name);
  if (!next_inserted && next_it->second != next_operand_value_name) {
    next_it->second = kInvalidValueName;
  }
}

[[nodiscard]] PreparedReturnChainLookups make_prepared_return_chain_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  PreparedReturnChainLookups lookups;
  const auto* bir_function = prepared_bir_function(prepared, function);
  if (bir_function == nullptr) {
    return lookups;
  }
  const auto block_count = std::min(function.blocks.size(), bir_function->blocks.size());
  for (std::size_t block_index = 0; block_index < block_count; ++block_index) {
    const auto* bir_block =
        prepared_bir_block(prepared, *bir_function, function.blocks[block_index]);
    if (bir_block == nullptr ||
        bir_block->terminator.kind != bir::TerminatorKind::Return ||
        !bir_block->terminator.value.has_value() ||
        bir_block->terminator.value->kind != bir::Value::Kind::Named) {
      continue;
    }
    const auto terminal_value_name =
        existing_prepared_value_name_id(prepared.names, *bir_block->terminator.value);
    if (!terminal_value_name.has_value()) {
      continue;
    }

    for (std::size_t instruction_index = 0;
         instruction_index < bir_block->insts.size();
         ++instruction_index) {
      const auto* binary =
          std::get_if<bir::BinaryInst>(&bir_block->insts[instruction_index]);
      if (binary == nullptr ||
          !prepared_return_chain_binary_opcode_is_scalar_publication(binary->opcode) ||
          binary->result.kind != bir::Value::Kind::Named) {
        continue;
      }
      const auto chain_value_name =
          existing_prepared_value_name_id(prepared.names, binary->result);
      if (!chain_value_name.has_value()) {
        continue;
      }

      std::string current_name = binary->result.name;
      ValueNameId next_operand_value_name = kInvalidValueName;
      bool reaches_return = false;
      for (std::size_t next_index = instruction_index + 1;
           next_index < bir_block->insts.size();
           ++next_index) {
        const auto* next_binary =
            std::get_if<bir::BinaryInst>(&bir_block->insts[next_index]);
        if (next_binary == nullptr ||
            !prepared_return_chain_binary_opcode_is_scalar_publication(
                next_binary->opcode)) {
          break;
        }
        const bool consumes_current =
            prepared_value_matches_name(next_binary->lhs, current_name) ||
            prepared_value_matches_name(next_binary->rhs, current_name);
        if (!consumes_current ||
            next_binary->result.kind != bir::Value::Kind::Named) {
          break;
        }
        if (next_index == instruction_index + 1) {
          const bir::Value& other_operand =
              prepared_value_matches_name(next_binary->lhs, current_name)
                  ? next_binary->rhs
                  : next_binary->lhs;
          if (const auto other_name =
                  existing_prepared_value_name_id(prepared.names, other_operand);
              other_name.has_value()) {
            next_operand_value_name = *other_name;
          }
        }
        current_name = next_binary->result.name;
      }
      reaches_return =
          prepared_value_matches_name(*bir_block->terminator.value, current_name);
      if (reaches_return) {
        publish_prepared_return_chain(lookups,
                                      block_index,
                                      instruction_index,
                                      *chain_value_name,
                                      *terminal_value_name,
                                      next_operand_value_name);
      }
    }
  }
  return lookups;
}

[[nodiscard]] PreparedValueHomeLookups make_prepared_value_home_lookups(
    const PreparedValueLocationFunction* value_locations) {
  PreparedValueHomeLookups lookups;
  if (value_locations == nullptr) {
    return lookups;
  }
  lookups.homes_by_id.reserve(value_locations->value_homes.size());
  lookups.value_ids.reserve(value_locations->value_homes.size());
  for (const auto& home : value_locations->value_homes) {
    lookups.homes_by_id.emplace(home.value_id, &home);
    if (home.value_name != kInvalidValueName) {
      lookups.value_ids.emplace(home.value_name, home.value_id);
    }
  }
  return lookups;
}

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedBirModule* prepared,
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups) {
  PreparedEdgePublicationLookups lookups;
  const auto local_value_home_lookups =
      value_home_lookups == nullptr
          ? make_prepared_value_home_lookups(value_locations)
          : PreparedValueHomeLookups{};
  const auto* value_homes =
      value_home_lookups == nullptr ? &local_value_home_lookups : value_home_lookups;
  const auto source_producers =
      prepared != nullptr
          ? make_prepared_edge_publication_source_producer_lookups(*prepared, function)
          : PreparedEdgePublicationSourceProducerLookups{};
  const auto* function_addressing =
      prepared != nullptr ? find_prepared_addressing(*prepared, function.function_name)
                          : nullptr;

  for (const auto& join_transfer : function.join_transfers) {
    const auto carrier_kind = effective_prepared_join_transfer_carrier_kind(join_transfer);
    for (const auto& edge_transfer : join_transfer.edge_transfers) {
      PreparedEdgePublication publication{
          .predecessor_label = edge_transfer.predecessor_label,
          .successor_label = edge_transfer.successor_label,
          .destination_value = edge_transfer.destination_value,
          .source_value = edge_transfer.incoming_value,
          .source_value_kind = edge_transfer.incoming_value.kind,
          .phase = PreparedMovePhase::BlockEntry,
          .carrier_kind = carrier_kind,
          .join_transfer = &join_transfer,
          .edge_transfer = &edge_transfer,
          .parallel_copy_bundle =
              find_published_parallel_copy_bundle_for_edge_transfer(function, edge_transfer),
      };

      if (publication.predecessor_label == kInvalidBlockLabel) {
        publication.status = PreparedEdgePublicationLookupStatus::MissingPredecessorLabel;
        lookups.publications.push_back(std::move(publication));
        continue;
      }
      if (publication.successor_label == kInvalidBlockLabel) {
        publication.status = PreparedEdgePublicationLookupStatus::MissingSuccessorLabel;
        lookups.publications.push_back(std::move(publication));
        continue;
      }

      const auto destination_name =
          existing_prepared_value_name_id(names, edge_transfer.destination_value);
      if (!destination_name.has_value()) {
        publication.status = PreparedEdgePublicationLookupStatus::MissingDestinationValue;
        lookups.publications.push_back(std::move(publication));
        continue;
      }
      publication.destination_value_name = *destination_name;

      const auto destination_value_id =
          find_indexed_prepared_value_id(value_homes,
                                        nullptr,
                                        value_locations,
                                        publication.destination_value_name);
      if (!destination_value_id.has_value()) {
        publication.status = PreparedEdgePublicationLookupStatus::MissingDestinationHome;
        lookups.publications.push_back(std::move(publication));
        continue;
      }
      publication.destination_value_id = *destination_value_id;
      publication.destination_home =
          find_indexed_prepared_value_home(value_homes,
                                           value_locations,
                                           publication.destination_value_id);
      if (publication.destination_home == nullptr) {
        publication.status = PreparedEdgePublicationLookupStatus::MissingDestinationHome;
        lookups.publications.push_back(std::move(publication));
        continue;
      }
      publication.destination_home_kind = publication.destination_home->kind;
      publication.destination_storage_kind =
          prepared_storage_kind_for_home(publication.destination_home->kind);

      if (const auto source_name =
              existing_prepared_value_name_id(names, edge_transfer.incoming_value);
          source_name.has_value()) {
        publication.source_value_name = *source_name;
        publication.source_value_id =
            find_indexed_prepared_value_id(value_homes,
                                          nullptr,
                                          value_locations,
                                          *source_name);
        if (publication.source_value_id.has_value()) {
          publication.source_home =
              find_indexed_prepared_value_home(value_homes,
                                               value_locations,
                                               *publication.source_value_id);
          if (publication.source_home != nullptr) {
            publication.source_home_kind = publication.source_home->kind;
          }
        }
      }
      apply_source_producer_fact(publication, source_producers);
      apply_source_memory_access_fact(publication, function_addressing);
      if (publication.source_value_id.has_value()) {
        publication.source_and_destination_same_value_id =
            *publication.source_value_id == publication.destination_value_id;
      }
      if (publication.parallel_copy_bundle != nullptr) {
        publication.parallel_copy_bundle_has_cycle =
            publication.parallel_copy_bundle->has_cycle;
        publication.parallel_copy_execution_site =
            publication.parallel_copy_bundle->execution_site;
        publication.parallel_copy_execution_block_label =
            publication.parallel_copy_bundle->execution_block_label;
      }

      publication.move = find_edge_publication_move(value_locations,
                                                    publication.predecessor_label,
                                                    publication.successor_label,
                                                    publication.destination_value_id,
                                                    &publication.move_bundle);
      if (publication.move != nullptr) {
        publication.destination_storage_kind = publication.move->destination_storage_kind;
        publication.matching_move_coalesced_by_assigned_storage =
            publication.move->coalesced_by_assigned_storage;
        publication.matching_move_redundant_by_assigned_storage =
            publication.move->coalesced_by_assigned_storage ||
            publication.source_and_destination_same_value_id;
        std::size_t step_index = 0;
        const auto* step = find_edge_publication_parallel_copy_step(
            publication.parallel_copy_bundle, publication.move, &step_index);
        if (step != nullptr) {
          publication.parallel_copy_step_index = step_index;
          publication.parallel_copy_step_kind = step->kind;
          publication.parallel_copy_step_uses_cycle_temp_source =
              step->uses_cycle_temp_source;
        } else if (publication.move->source_parallel_copy_step_index.has_value()) {
          publication.parallel_copy_step_index =
              publication.move->source_parallel_copy_step_index;
          publication.parallel_copy_step_uses_cycle_temp_source =
              publication.move->uses_cycle_temp_source;
        }
      }
      publication.status = PreparedEdgePublicationLookupStatus::Available;
      publication.aggregate_stack_source_authority =
          prepare_aggregate_stack_source_authority(&publication);
      lookups.publications.push_back(std::move(publication));
    }
  }

  for (const auto& publication : lookups.publications) {
    if (publication.status != PreparedEdgePublicationLookupStatus::Available) {
      continue;
    }
    lookups.publications_by_edge_destination
        [prepared_edge_publication_key(publication.predecessor_label,
                                       publication.successor_label,
                                       publication.destination_value_id)]
            .push_back(&publication);
  }
  return lookups;
}

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups) {
  return make_prepared_edge_publication_lookups(
      nullptr, names, function, value_locations, value_home_lookups);
}

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups) {
  return make_prepared_edge_publication_lookups(
      &prepared, prepared.names, function, value_locations, value_home_lookups);
}

[[nodiscard]] PreparedFunctionLookups make_prepared_function_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  const auto* call_plans = find_prepared_call_plans(prepared, function.function_name);
  const auto* value_locations =
      find_prepared_value_location_function(prepared, function.function_name);
  const auto* addressing = find_prepared_addressing(prepared, function.function_name);
  auto value_home_lookups = make_prepared_value_home_lookups(value_locations);
  auto memory_access_lookups =
      make_prepared_memory_access_lookups(addressing, &value_home_lookups);
  auto move_bundle_lookups = make_prepared_move_bundle_lookups(value_locations);
  publish_prepared_after_call_result_lane_bindings(move_bundle_lookups,
                                                   prepared,
                                                   function,
                                                   value_locations,
                                                   value_home_lookups);
  auto source_producer_lookups =
      make_prepared_edge_publication_source_producer_lookups(prepared, function);
  auto edge_publication_lookups = make_prepared_edge_publication_lookups(
      prepared, function, value_locations, &value_home_lookups);
  return PreparedFunctionLookups{
      .call_plans = make_prepared_call_plan_lookups(prepared, call_plans, function),
      .address_materializations =
          make_prepared_address_materialization_lookups(prepared, function.function_name),
      .memory_accesses = std::move(memory_access_lookups),
      .move_bundles = std::move(move_bundle_lookups),
      .return_chains = make_prepared_return_chain_lookups(prepared, function),
      .value_homes = std::move(value_home_lookups),
      .edge_publications = std::move(edge_publication_lookups),
      .edge_publication_source_producers = std::move(source_producer_lookups),
  };
}

[[nodiscard]] const PreparedCallPlan* find_indexed_prepared_call_plan(
    const PreparedCallPlanLookups* lookups,
    const PreparedCallPlansFunction* call_plans,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (lookups != nullptr) {
    const auto it =
        lookups->calls_by_position.find(prepared_call_position_key(block_index,
                                                                   instruction_index));
    if (it != lookups->calls_by_position.end()) {
      return it->second;
    }
    return nullptr;
  }
  if (call_plans == nullptr) {
    return nullptr;
  }
  for (const auto& call : call_plans->calls) {
    if (call.block_index == block_index && call.instruction_index == instruction_index) {
      return &call;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedCallArgumentPlan*
find_indexed_prepared_immediate_call_argument(
    const PreparedCallPlanLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->immediate_arguments_by_position_and_abi.find(
      prepared_call_argument_position_key(block_index, instruction_index, abi_index));
  return it != lookups->immediate_arguments_by_position_and_abi.end()
             ? it->second
             : nullptr;
}

[[nodiscard]] const std::vector<const PreparedAddressMaterialization*>*
find_indexed_prepared_address_materializations(
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->materializations_by_block.find(block_label);
  if (it == lookups->materializations_by_block.end()) {
    return nullptr;
  }
  return &it->second;
}

[[nodiscard]] const std::vector<const PreparedMemoryAccess*>*
find_indexed_prepared_memory_accesses_by_result_value_name(
    const PreparedMemoryAccessLookups* lookups,
    ValueNameId result_value_name) {
  if (lookups == nullptr || result_value_name == kInvalidValueName) {
    return nullptr;
  }
  const auto it = lookups->accesses_by_result_value_name.find(result_value_name);
  if (it == lookups->accesses_by_result_value_name.end()) {
    return nullptr;
  }
  return &it->second;
}

[[nodiscard]] const PreparedMemoryAccess* find_indexed_prepared_memory_access(
    const PreparedMemoryAccessLookups* lookups,
    BlockLabelId block_label,
    std::size_t instruction_index) {
  if (lookups == nullptr || block_label == kInvalidBlockLabel) {
    return nullptr;
  }
  const auto it = lookups->accesses_by_position.find(
      prepared_memory_access_position_key(block_label, instruction_index));
  if (it == lookups->accesses_by_position.end()) {
    return nullptr;
  }
  return it->second;
}

[[nodiscard]] const PreparedMemoryAccess*
find_unique_indexed_prepared_memory_access_by_result_value_name(
    const PreparedMemoryAccessLookups* lookups,
    ValueNameId result_value_name) {
  const auto* accesses =
      find_indexed_prepared_memory_accesses_by_result_value_name(lookups,
                                                                 result_value_name);
  if (accesses == nullptr || accesses->size() != 1) {
    return nullptr;
  }
  return accesses->front();
}

[[nodiscard]] const std::vector<const PreparedMemoryAccess*>*
find_indexed_prepared_memory_accesses_by_result_value_id(
    const PreparedMemoryAccessLookups* lookups,
    PreparedValueId result_value_id) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->accesses_by_result_value_id.find(result_value_id);
  if (it == lookups->accesses_by_result_value_id.end()) {
    return nullptr;
  }
  return &it->second;
}

[[nodiscard]] const PreparedMemoryAccess*
find_unique_indexed_prepared_memory_access_by_result_value_id(
    const PreparedMemoryAccessLookups* lookups,
    PreparedValueId result_value_id) {
  const auto* accesses =
      find_indexed_prepared_memory_accesses_by_result_value_id(lookups,
                                                               result_value_id);
  if (accesses == nullptr || accesses->size() != 1) {
    return nullptr;
  }
  return accesses->front();
}

[[nodiscard]] std::vector<const PreparedAddressMaterialization*>
collect_prepared_address_materializations_for_block(
    const PreparedAddressingFunction& addressing,
    BlockLabelId block_label) {
  std::vector<const PreparedAddressMaterialization*> materializations;
  for (const auto& materialization : addressing.address_materializations) {
    if (materialization.block_label == block_label) {
      materializations.push_back(&materialization);
    }
  }
  return materializations;
}

std::optional<PreparedFrameAddressOffset>
find_indexed_prepared_frame_address_offset_for_value(
    const PreparedStackLayout& stack_layout,
    const PreparedAddressMaterializationLookups* lookups,
    BlockLabelId block_label,
    ValueNameId value_name,
    std::optional<std::size_t> before_or_at_instruction_index) {
  if (lookups == nullptr || block_label == kInvalidBlockLabel ||
      value_name == kInvalidValueName) {
    return std::nullopt;
  }
  const auto* materializations =
      find_indexed_prepared_address_materializations(lookups, block_label);
  if (materializations == nullptr) {
    return std::nullopt;
  }

  const PreparedAddressMaterialization* selected = nullptr;
  const PreparedFrameSlot* selected_slot = nullptr;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->kind != PreparedAddressMaterializationKind::FrameSlot ||
        materialization->result_value_name != std::optional<ValueNameId>{value_name} ||
        !materialization->frame_slot_id.has_value() ||
        (before_or_at_instruction_index.has_value() &&
         materialization->inst_index > *before_or_at_instruction_index)) {
      continue;
    }
    const auto* slot =
        find_frame_slot_by_id(stack_layout, *materialization->frame_slot_id);
    if (slot == nullptr || materialization->byte_offset < 0 ||
        !prepared_frame_address_object_is_addressable(stack_layout, *slot)) {
      continue;
    }
    if (selected != nullptr &&
        selected->inst_index == materialization->inst_index) {
      if (selected->frame_slot_id == materialization->frame_slot_id &&
          selected->byte_offset == materialization->byte_offset) {
        continue;
      }
      return std::nullopt;
    }
    if (selected == nullptr || selected->inst_index < materialization->inst_index) {
      selected = materialization;
      selected_slot = slot;
    }
  }
  if (selected == nullptr || selected_slot == nullptr ||
      !selected->frame_slot_id.has_value()) {
    return std::nullopt;
  }
  if (selected->byte_offset < 0) {
    return std::nullopt;
  }
  return PreparedFrameAddressOffset{
      .materialization = selected,
      .frame_slot_id = *selected->frame_slot_id,
      .object_id = selected_slot->object_id,
      .stack_offset_bytes = static_cast<std::size_t>(selected->byte_offset),
      .materialization_byte_offset = selected->byte_offset,
  };
}

std::optional<PreparedFrameAddressOffset>
find_indexed_prepared_frame_address_offset_for_value_id(
    const PreparedStackLayout& stack_layout,
    const PreparedAddressMaterializationLookups* lookups,
    const PreparedValueHomeLookups* value_home_lookups,
    BlockLabelId block_label,
    PreparedValueId value_id,
    std::optional<std::size_t> before_or_at_instruction_index) {
  if (lookups == nullptr || block_label == kInvalidBlockLabel) {
    return std::nullopt;
  }
  const auto* materializations =
      find_indexed_prepared_address_materializations(lookups, block_label);
  if (materializations == nullptr) {
    return std::nullopt;
  }

  const PreparedAddressMaterialization* selected = nullptr;
  const PreparedFrameSlot* selected_slot = nullptr;
  for (const auto* materialization : *materializations) {
    std::optional<PreparedValueId> materialization_value_id =
        materialization != nullptr ? materialization->result_value_id : std::nullopt;
    if (!materialization_value_id.has_value() &&
        materialization != nullptr &&
        materialization->result_value_name.has_value() &&
        value_home_lookups != nullptr) {
      const auto it =
          value_home_lookups->value_ids.find(*materialization->result_value_name);
      if (it != value_home_lookups->value_ids.end()) {
        materialization_value_id = it->second;
      }
    }
    if (materialization == nullptr ||
        materialization->kind != PreparedAddressMaterializationKind::FrameSlot ||
        materialization_value_id != std::optional<PreparedValueId>{value_id} ||
        !materialization->frame_slot_id.has_value() ||
        (before_or_at_instruction_index.has_value() &&
         materialization->inst_index > *before_or_at_instruction_index)) {
      continue;
    }
    const auto* slot =
        find_frame_slot_by_id(stack_layout, *materialization->frame_slot_id);
    if (slot == nullptr || materialization->byte_offset < 0 ||
        !prepared_frame_address_object_is_addressable(stack_layout, *slot)) {
      continue;
    }
    if (selected != nullptr &&
        selected->inst_index == materialization->inst_index) {
      if (selected->frame_slot_id == materialization->frame_slot_id &&
          selected->byte_offset == materialization->byte_offset) {
        continue;
      }
      return std::nullopt;
    }
    if (selected == nullptr || selected->inst_index < materialization->inst_index) {
      selected = materialization;
      selected_slot = slot;
    }
  }
  if (selected == nullptr || selected_slot == nullptr ||
      !selected->frame_slot_id.has_value() || selected->byte_offset < 0) {
    return std::nullopt;
  }
  return PreparedFrameAddressOffset{
      .materialization = selected,
      .frame_slot_id = *selected->frame_slot_id,
      .object_id = selected_slot->object_id,
      .stack_offset_bytes = static_cast<std::size_t>(selected->byte_offset),
      .materialization_byte_offset = selected->byte_offset,
  };
}

PreparedCurrentBlockEntryPublication
find_prepared_current_block_entry_publication(
    const PreparedCurrentBlockEntryPublicationQueryInputs& query,
    PreparedValueId destination_value_id) {
  PreparedCurrentBlockEntryPublication result{
      .destination_value_id = destination_value_id,
  };
  if (query.value_locations == nullptr) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingValueLocations;
    return result;
  }
  if (query.successor_label == kInvalidBlockLabel) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingSuccessorLabel;
    return result;
  }
  if (destination_value_id == PreparedValueId{0}) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingDestinationValue;
    return result;
  }

  const auto* destination_home =
      find_indexed_prepared_value_home(query.value_home_lookups,
                                       query.value_locations,
                                       destination_value_id);
  result.destination_home = destination_home;
  if (destination_home != nullptr) {
    result.destination_value_name = destination_home->value_name;
  } else {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingDestinationHome;
    return result;
  }

  std::optional<PreparedBlockEntryPublication> first_matching_publication;
  for (const auto& publication :
       collect_prepared_block_entry_publications(query.value_locations,
                                                 query.successor_label)) {
    if (publication.destination_value_id != destination_value_id) {
      continue;
    }
    if (!first_matching_publication.has_value()) {
      first_matching_publication = publication;
    }
    if (prepared_block_entry_publication_available(publication)) {
      result.status = PreparedCurrentBlockEntryPublicationStatus::Available;
      result.publication = publication;
      result.destination_home = publication.home;
      result.destination_value_name = publication.destination_value_name;
      return result;
    }
  }

  if (first_matching_publication.has_value()) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::PublicationUnavailable;
    result.publication = *first_matching_publication;
    result.destination_home = first_matching_publication->home;
    result.destination_value_name = first_matching_publication->destination_value_name;
    return result;
  }

  result.status = PreparedCurrentBlockEntryPublicationStatus::MissingPublication;
  return result;
}

PreparedCurrentBlockEntryPublication
find_prepared_current_block_entry_publication(
    const PreparedCurrentBlockEntryPublicationQueryInputs& query,
    const bir::Value& destination_value) {
  PreparedCurrentBlockEntryPublication result;
  if (query.names == nullptr) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingNames;
    return result;
  }
  const auto destination_value_name =
      existing_prepared_value_name_id(*query.names, destination_value);
  if (!destination_value_name.has_value()) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingDestinationValue;
    return result;
  }
  result.destination_value_name = *destination_value_name;
  if (query.value_locations == nullptr) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingValueLocations;
    return result;
  }
  const auto destination_value_id =
      find_indexed_prepared_value_id(query.value_home_lookups,
                                     query.regalloc,
                                     query.value_locations,
                                     *destination_value_name);
  if (!destination_value_id.has_value()) {
    result.status = PreparedCurrentBlockEntryPublicationStatus::MissingDestinationHome;
    return result;
  }
  return find_prepared_current_block_entry_publication(query, *destination_value_id);
}

[[nodiscard]] const PreparedMoveBundle* find_indexed_prepared_move_bundle(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (lookups != nullptr) {
    const auto it =
        lookups->bundles_by_position.find(prepared_move_bundle_position_key(
            phase, block_index, instruction_index));
    if (it != lookups->bundles_by_position.end()) {
      return it->second;
    }
    return nullptr;
  }
  return value_locations == nullptr ? nullptr
                                    : find_prepared_move_bundle(*value_locations,
                                                                phase,
                                                                block_index,
                                                                instruction_index);
}

[[nodiscard]] const PreparedMoveResolution*
find_indexed_prepared_before_call_argument_move(
    const PreparedMoveBundleLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->before_call_argument_moves_by_position_and_abi.find(
      prepared_call_argument_position_key(block_index, instruction_index, abi_index));
  return it != lookups->before_call_argument_moves_by_position_and_abi.end()
             ? it->second
             : nullptr;
}

[[nodiscard]] const PreparedAfterCallResultLaneBinding*
find_indexed_prepared_after_call_result_lane_binding(
    const PreparedMoveBundleLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    PreparedValueId value_id) {
  if (lookups == nullptr || value_id == PreparedValueId{0}) {
    return nullptr;
  }
  const auto it =
      lookups->after_call_result_lane_bindings_by_position_and_value.find(
          prepared_after_call_result_lane_position_key(block_index,
                                                       instruction_index,
                                                       value_id));
  return it != lookups->after_call_result_lane_bindings_by_position_and_value.end()
             ? it->second
             : nullptr;
}

[[nodiscard]] bool prepared_move_targets_destination_bank(
    const PreparedMoveResolution& move,
    PreparedRegisterBank destination_bank) {
  return destination_bank != PreparedRegisterBank::None &&
         move.destination_register_placement.has_value() &&
         move.destination_register_placement->bank == destination_bank;
}

[[nodiscard]] const PreparedMoveResolution*
find_prepared_before_return_abi_move_by_source_and_destination_bank(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    std::size_t block_index,
    PreparedValueId source_value_id,
    PreparedRegisterBank destination_bank) {
  if (source_value_id == PreparedValueId{0} ||
      destination_bank == PreparedRegisterBank::None) {
    return nullptr;
  }
  if (lookups != nullptr) {
    const auto it =
        lookups->before_return_abi_moves_by_source_and_bank.find(
            prepared_before_return_abi_move_source_bank_key(block_index,
                                                            source_value_id,
                                                            destination_bank));
    return it != lookups->before_return_abi_moves_by_source_and_bank.end()
               ? it->second
               : nullptr;
  }
  if (value_locations == nullptr) {
    return nullptr;
  }

  const PreparedMoveResolution* selected = nullptr;
  for (const auto& bundle : value_locations->move_bundles) {
    if (bundle.phase != PreparedMovePhase::BeforeReturn ||
        bundle.block_index != block_index) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.from_value_id != source_value_id ||
          move.destination_kind != PreparedMoveDestinationKind::FunctionReturnAbi ||
          move.destination_storage_kind != PreparedMoveStorageKind::Register ||
          move.op_kind != PreparedMoveResolutionOpKind::Move ||
          !prepared_move_targets_destination_bank(move, destination_bank)) {
        continue;
      }
      if (selected != nullptr) {
        return nullptr;
      }
      selected = &move;
    }
  }
  return selected;
}

[[nodiscard]] ValueNameId find_prepared_return_chain_terminal_value(
    const PreparedReturnChainLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId value_name) {
  if (lookups == nullptr || value_name == kInvalidValueName) {
    return kInvalidValueName;
  }
  const auto it = lookups->terminal_return_values_by_chain_value.find(
      prepared_return_chain_value_key(block_index, instruction_index, value_name));
  return it != lookups->terminal_return_values_by_chain_value.end()
             ? it->second
             : kInvalidValueName;
}

[[nodiscard]] ValueNameId find_prepared_return_chain_next_operand_value(
    const PreparedReturnChainLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId value_name) {
  if (lookups == nullptr || value_name == kInvalidValueName) {
    return kInvalidValueName;
  }
  const auto it = lookups->next_operand_values_by_chain_value.find(
      prepared_return_chain_value_key(block_index, instruction_index, value_name));
  return it != lookups->next_operand_values_by_chain_value.end()
             ? it->second
             : kInvalidValueName;
}

PreparedCurrentBlockPublicationConsumption
find_prepared_current_block_publication_consumption(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (source_producers == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      value_name == kInvalidValueName) {
    return {};
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          source_producers, value_name);
  if (producer == nullptr ||
      producer->block_label != block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= block->insts.size()) {
    return {};
  }
  const auto& inst = block->insts[producer->instruction_index];
  if (!prepared_source_producer_matches_instruction(*producer, inst)) {
    return {};
  }
  const auto* producer_result = prepared_lookup_source_producer_result(*producer);
  const auto* instruction_result = prepared_instruction_result_value_ref(inst);
  if (producer_result == nullptr ||
      instruction_result == nullptr ||
      producer_result != instruction_result ||
      !prepared_result_matches_value_name(names, *instruction_result, value_name)) {
    return {};
  }
  return PreparedCurrentBlockPublicationConsumption{
      .available = true,
      .source_producer = producer,
      .instruction = &inst,
      .produced_value = instruction_result,
      .instruction_index = producer->instruction_index,
      .value_name = value_name,
      .source_producer_kind = producer->kind,
  };
}

std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    ValueNameId value_name,
    bir::TypeKind value_type,
    std::size_t before_instruction_index) {
  return prepared_same_block_source_producer(names,
                                            source_producers,
                                            block_label,
                                            block,
                                            value_name,
                                            value_type,
                                            before_instruction_index);
}

std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto value_name = existing_prepared_value_name_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  return find_prepared_same_block_scalar_producer(names,
                                                 source_producers,
                                                 block_label,
                                                 block,
                                                 *value_name,
                                                 value.type,
                                                 before_instruction_index);
}

std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedSameBlockValueMaterializationQuery& query,
    const bir::Value& value) {
  if (query.names == nullptr) {
    return std::nullopt;
  }
  return find_prepared_same_block_scalar_producer(*query.names,
                                                 query.source_producers,
                                                 query.block_label,
                                                 query.block,
                                                 value,
                                                 query.before_instruction_index);
}

std::optional<std::int64_t> evaluate_prepared_same_block_integer_constant(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  return evaluate_prepared_same_block_integer_constant(names,
                                                       source_producers,
                                                       block_label,
                                                       block,
                                                       value,
                                                       before_instruction_index,
                                                       0U);
}

std::optional<std::int64_t> evaluate_prepared_same_block_integer_constant(
    const PreparedSameBlockValueMaterializationQuery& query,
    const bir::Value& value) {
  if (query.names == nullptr) {
    return std::nullopt;
  }
  return evaluate_prepared_same_block_integer_constant(*query.names,
                                                       query.source_producers,
                                                       query.block_label,
                                                       query.block,
                                                       value,
                                                       query.before_instruction_index,
                                                       0U);
}

std::optional<PreparedFusedCompareOperandProducer>
find_prepared_fused_compare_operand_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return PreparedFusedCompareOperandProducer{
        .kind = PreparedEdgePublicationSourceProducerKind::Immediate,
        .integer_constant = value.immediate,
    };
  }
  const auto value_name = existing_prepared_value_name_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto producer =
      find_prepared_same_block_scalar_producer(names,
                                              source_producers,
                                              block_label,
                                              block,
                                              *value_name,
                                              value.type,
                                              before_instruction_index);
  if (!producer.has_value()) {
    return std::nullopt;
  }
  PreparedFusedCompareOperandProducer result{
      .kind = producer->producer.kind,
      .instruction = producer->instruction,
      .instruction_index = producer->instruction_index,
      .value_name = *value_name,
      .cast = producer->producer.cast,
      .load_local = producer->producer.load_local,
      .load_global = producer->producer.load_global,
      .binary = producer->producer.binary,
      .select = producer->producer.select,
  };
  result.integer_constant =
      evaluate_prepared_same_block_integer_constant(names,
                                                    source_producers,
                                                    block_label,
                                                    block,
                                                    value,
                                                    before_instruction_index,
                                                    0U);
  return result;
}

std::optional<PreparedFusedCompareOperandProducerFacts>
find_prepared_fused_compare_operand_producer_facts(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const PreparedBranchCondition& branch_condition,
    std::size_t before_instruction_index) {
  if (branch_condition.kind != PreparedBranchConditionKind::FusedCompare ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  PreparedFusedCompareOperandProducerFacts facts{
      .lhs = find_prepared_fused_compare_operand_producer(names,
                                                          source_producers,
                                                          block_label,
                                                          block,
                                                          *branch_condition.lhs,
                                                          before_instruction_index),
      .rhs = find_prepared_fused_compare_operand_producer(names,
                                                          source_producers,
                                                          block_label,
                                                          block,
                                                          *branch_condition.rhs,
                                                          before_instruction_index),
  };
  if (!facts.lhs.has_value() && !facts.rhs.has_value()) {
    return std::nullopt;
  }
  return facts;
}

std::optional<PreparedMaterializedConditionProducer>
find_prepared_materialized_condition_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& condition_value,
    std::size_t before_instruction_index) {
  if (condition_value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto condition_value_name =
      existing_prepared_value_name_id(names, condition_value);
  if (!condition_value_name.has_value()) {
    return std::nullopt;
  }
  const auto producer =
      find_prepared_same_block_scalar_producer(names,
                                              source_producers,
                                              block_label,
                                              block,
                                              *condition_value_name,
                                              condition_value.type,
                                              before_instruction_index);
  if (!producer.has_value() ||
      producer->producer.kind != PreparedEdgePublicationSourceProducerKind::Binary ||
      producer->producer.binary == nullptr) {
    return std::nullopt;
  }
  return PreparedMaterializedConditionProducer{
      .binary = producer->producer.binary,
      .instruction_index = producer->instruction_index,
      .condition_value_name = *condition_value_name,
  };
}

std::optional<PreparedSameBlockGlobalLoadAccess>
find_prepared_global_load_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global) {
  if (addressing == nullptr || block_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto* access =
      find_prepared_memory_access(*addressing, block_label, instruction_index);
  if (!prepared_global_load_access_matches_result(
          names,
          access,
          load_global) ||
      access->address.base_kind != PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }

  return PreparedSameBlockGlobalLoadAccess{
      .load_global = &load_global,
      .access = access,
  };
}

std::optional<PreparedSameBlockGlobalLoadAccess>
find_prepared_same_block_global_load_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    const PreparedSameBlockScalarProducer& producer) {
  if (producer.producer.kind != PreparedEdgePublicationSourceProducerKind::LoadGlobal ||
      producer.producer.load_global == nullptr) {
    return std::nullopt;
  }
  return find_prepared_global_load_access(names,
                                          addressing,
                                          producer.producer.block_label,
                                          producer.producer.instruction_index,
                                          *producer.producer.load_global);
}

std::optional<PreparedSameBlockLoadLocalStoredValueSource>
find_prepared_same_block_load_local_stored_value_source(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto value_name = prepared_existing_value_name_id(names, value);
  const auto producer_context =
      value_name.has_value()
          ? find_prepared_same_block_scalar_producer(names,
                                                    source_producers,
                                                    block_label,
                                                    block,
                                                    *value_name,
                                                    value.type,
                                                    before_instruction_index)
          : std::nullopt;
  const auto* producer = producer_context.has_value()
                             ? &producer_context->producer
                             : nullptr;
  if (addressing == nullptr ||
      producer == nullptr ||
      producer->kind != PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      producer->load_local == nullptr ||
      producer->block_label == kInvalidBlockLabel ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= (block != nullptr ? block->insts.size()
                                                       : std::size_t{0})) {
    return std::nullopt;
  }

  const auto* load_access =
      find_prepared_memory_access(*addressing,
                                  producer->block_label,
                                  producer->instruction_index);
  if (!prepared_load_access_matches_result(names, load_access, *producer->load_local) ||
      prepared_frame_slot_for_access(stack_layout, load_access) == nullptr) {
    return std::nullopt;
  }

  for (std::size_t index = producer->instruction_index; index > 0; --index) {
    const auto store_index = index - 1;
    const auto* store = std::get_if<bir::StoreLocalInst>(&block->insts[store_index]);
    if (store == nullptr) {
      continue;
    }
    const auto* store_access =
        find_prepared_memory_access(*addressing, producer->block_label, store_index);
    if (store_access == nullptr ||
        !prepared_store_access_matches_value(names, store_access, *store)) {
      if (prepared_access_ranges_overlap(stack_layout, load_access, store_access)) {
        return std::nullopt;
      }
      continue;
    }
    if (!prepared_access_ranges_overlap(stack_layout, load_access, store_access)) {
      continue;
    }
    if (!prepared_access_ranges_match(stack_layout, load_access, store_access)) {
      return std::nullopt;
    }
    return PreparedSameBlockLoadLocalStoredValueSource{
        .stored_value = store->value,
        .store_instruction_index = store_index,
        .load_producer = producer,
        .load_access = load_access,
        .store_access = store_access,
    };
  }

  return std::nullopt;
}

[[nodiscard]] const PreparedCallPreservedValue*
find_latest_indexed_prior_preserved_value(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id) {
  if (value_id >= lookups.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = lookups.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const PreparedPriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(entries.begin(),
                             entries.end(),
                             current,
                             prepared_prior_preserved_value_entry_position_less);
  if (it == entries.begin()) {
    return nullptr;
  }
  --it;
  return it->preserved;
}

[[nodiscard]] const PreparedCallPreservedValue*
find_dominating_indexed_prior_preserved_value(
    const PreparedCallPlanLookups& lookups,
    const PreparedControlFlowFunction* control_flow,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id) {
  if (value_id >= lookups.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = lookups.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const PreparedPriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(entries.begin(),
                             entries.end(),
                             current,
                             prepared_prior_preserved_value_entry_position_less);
  while (it != entries.begin()) {
    --it;
    if (it->block_index == current_call_plan.block_index) {
      if (it->instruction_index < current_call_plan.instruction_index) {
        return it->preserved;
      }
      continue;
    }
    if (control_flow != nullptr &&
        prepared_block_dominates(*control_flow, it->block_index,
                                 current_call_plan.block_index)) {
      return it->preserved;
    }
  }
  return nullptr;
}

[[nodiscard]] PreparedPriorPreservedValueLookupResult
find_unique_indexed_prior_preserved_value_source(
    const PreparedCallPlanLookups& lookups,
    const PreparedControlFlowFunction* control_flow,
    const PreparedCallPlan& current_call_plan,
    PreparedValueId value_id) {
  if (value_id >= lookups.prior_preserved_by_value.size()) {
    return {};
  }
  const auto& entries = lookups.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return {};
  }
  const PreparedPriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(entries.begin(),
                             entries.end(),
                             current,
                             prepared_prior_preserved_value_entry_position_less);
  const PreparedPriorPreservedValueEntry* selected = nullptr;
  while (it != entries.begin()) {
    --it;
    if (!prepared_prior_preserved_value_entry_reaches_call(
            *it, control_flow, current_call_plan)) {
      continue;
    }
    if (selected == nullptr) {
      selected = &*it;
      continue;
    }
    if (prepared_prior_preserved_value_entry_same_position(*selected, *it)) {
      return PreparedPriorPreservedValueLookupResult{
          .status = PreparedPriorPreservedValueLookupStatus::Ambiguous,
          .entry = selected,
          .preserved = selected->preserved,
      };
    }
    break;
  }
  if (selected == nullptr || selected->preserved == nullptr) {
    return {};
  }
  if (!prepared_prior_preserved_value_has_complete_source(*selected->preserved)) {
    return PreparedPriorPreservedValueLookupResult{
        .status = PreparedPriorPreservedValueLookupStatus::InvalidPreservation,
        .entry = selected,
        .preserved = selected->preserved,
    };
  }
  return PreparedPriorPreservedValueLookupResult{
      .status = PreparedPriorPreservedValueLookupStatus::Found,
      .entry = selected,
      .preserved = selected->preserved,
  };
}

[[nodiscard]] const PreparedCallPreservedValue*
find_latest_indexed_prior_stack_preserved_value_before_instruction(
    const PreparedCallPlanLookups& lookups,
    PreparedValueId value_id,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (value_id >= lookups.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = lookups.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const PreparedPriorPreservedValueEntry current{
      .block_index = block_index,
      .instruction_index = instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(entries.begin(),
                             entries.end(),
                             current,
                             prepared_prior_preserved_value_entry_position_less);
  while (it != entries.begin()) {
    --it;
    if (it->block_index != block_index) {
      break;
    }
    if (it->preserved != nullptr &&
        it->preserved->route == PreparedCallPreservationRoute::StackSlot) {
      return it->preserved;
    }
  }
  return nullptr;
}

[[nodiscard]] const std::vector<const PreparedCallPreservedValue*>*
first_indexed_stack_preserved_values_for_call(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlansFunction& call_plans,
    const PreparedCallPlan& current_call_plan) {
  if (call_plans.calls.empty()) {
    return nullptr;
  }
  const auto* begin = call_plans.calls.data();
  const auto* end = begin + call_plans.calls.size();
  const auto* current = &current_call_plan;
  if (current >= begin && current < end) {
    return &lookups.first_stack_preserved_by_call_index
                [static_cast<std::size_t>(current - begin)];
  }
  for (std::size_t call_index = 0; call_index < call_plans.calls.size(); ++call_index) {
    const auto& call = call_plans.calls[call_index];
    if (call.block_index == current_call_plan.block_index &&
        call.instruction_index == current_call_plan.instruction_index) {
      return &lookups.first_stack_preserved_by_call_index[call_index];
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedOutgoingStackArgumentArea*
find_indexed_prepared_outgoing_stack_argument_area(
    const PreparedCallPlanLookups& lookups,
    const PreparedCallPlansFunction* call_plans,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (call_plans == nullptr) {
    return nullptr;
  }
  const auto area_it = lookups.outgoing_stack_argument_areas_by_position.find(
      prepared_call_position_key(block_index, instruction_index));
  return area_it != lookups.outgoing_stack_argument_areas_by_position.end()
             ? area_it->second
             : nullptr;
}

[[nodiscard]] const std::vector<PreparedCallBoundaryEffectPlan>*
indexed_block_entry_republication_effects_for_block(
    const PreparedCallPlanLookups& lookups,
    std::size_t block_index) {
  const auto it = lookups.block_entry_republication_effects_by_block.find(block_index);
  if (it == lookups.block_entry_republication_effects_by_block.end()) {
    return nullptr;
  }
  return &it->second;
}

}  // namespace c4c::backend::prepare
