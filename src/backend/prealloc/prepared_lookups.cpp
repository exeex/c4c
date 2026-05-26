#include "prepared_lookups.hpp"

#include "module.hpp"

#include <algorithm>
#include <string_view>
#include <variant>

namespace c4c::backend::prepare {
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

[[nodiscard]] PreparedEdgePublicationKey prepared_edge_publication_key(
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id) {
  return PreparedEdgePublicationKey{
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .destination_value_id = destination_value_id,
  };
}

[[nodiscard]] bool operator==(const PreparedEdgePublicationKey& lhs,
                              const PreparedEdgePublicationKey& rhs) {
  return lhs.predecessor_label == rhs.predecessor_label &&
         lhs.successor_label == rhs.successor_label &&
         lhs.destination_value_id == rhs.destination_value_id;
}

[[nodiscard]] std::size_t PreparedEdgePublicationKeyHash::operator()(
    const PreparedEdgePublicationKey& key) const {
  std::size_t seed = std::hash<BlockLabelId>{}(key.predecessor_label);
  seed ^= std::hash<BlockLabelId>{}(key.successor_label) + 0x9e3779b97f4a7c15ULL +
          (seed << 6U) + (seed >> 2U);
  seed ^= std::hash<PreparedValueId>{}(key.destination_value_id) +
          0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  return seed;
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
    lookups.calls_by_position.emplace(
        prepared_call_position_key(call.block_index, call.instruction_index), &call);
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

  for (const auto& join_transfer : function.join_transfers) {
    const auto carrier_kind = effective_prepared_join_transfer_carrier_kind(join_transfer);
    for (const auto& edge_transfer : join_transfer.edge_transfers) {
      PreparedEdgePublication publication{
          .predecessor_label = edge_transfer.predecessor_label,
          .successor_label = edge_transfer.successor_label,
          .destination_value = edge_transfer.destination_value,
          .source_value = edge_transfer.incoming_value,
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
      }

      publication.move = find_edge_publication_move(value_locations,
                                                    publication.predecessor_label,
                                                    publication.successor_label,
                                                    publication.destination_value_id,
                                                    &publication.move_bundle);
      if (publication.move != nullptr) {
        publication.destination_storage_kind = publication.move->destination_storage_kind;
      }
      publication.status = PreparedEdgePublicationLookupStatus::Available;
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

[[nodiscard]] PreparedFunctionLookups make_prepared_function_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function) {
  const auto* call_plans = find_prepared_call_plans(prepared, function.function_name);
  const auto* value_locations =
      find_prepared_value_location_function(prepared, function.function_name);
  auto value_home_lookups = make_prepared_value_home_lookups(value_locations);
  auto edge_publication_lookups = make_prepared_edge_publication_lookups(
      prepared.names, function, value_locations, &value_home_lookups);
  return PreparedFunctionLookups{
      .call_plans = make_prepared_call_plan_lookups(prepared, call_plans, function),
      .address_materializations =
          make_prepared_address_materialization_lookups(prepared, function.function_name),
      .move_bundles = make_prepared_move_bundle_lookups(value_locations),
      .value_homes = std::move(value_home_lookups),
      .edge_publications = std::move(edge_publication_lookups),
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

[[nodiscard]] const std::vector<const PreparedEdgePublication*>*
find_indexed_prepared_edge_publications(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id) {
  if (lookups == nullptr) {
    return nullptr;
  }
  const auto it = lookups->publications_by_edge_destination.find(
      prepared_edge_publication_key(predecessor_label, successor_label, destination_value_id));
  if (it == lookups->publications_by_edge_destination.end()) {
    return nullptr;
  }
  return &it->second;
}

[[nodiscard]] const PreparedEdgePublication* find_unique_indexed_prepared_edge_publication(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id) {
  const auto* publications = find_indexed_prepared_edge_publications(
      lookups, predecessor_label, successor_label, destination_value_id);
  if (publications == nullptr || publications->size() != 1) {
    return nullptr;
  }
  return publications->front();
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
