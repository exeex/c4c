#include "publication_plans.hpp"

#include "mir/query.hpp"

#include "module.hpp"
#include "select_chain_lookups.hpp"

#include <algorithm>
#include <functional>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] bool is_prepared_store_global_publication_run_instruction(
    const bir::Inst& inst) {
  return std::get_if<bir::SelectInst>(&inst) != nullptr ||
         std::get_if<bir::LoadGlobalInst>(&inst) != nullptr ||
         std::get_if<bir::BinaryInst>(&inst) != nullptr ||
         std::get_if<bir::CastInst>(&inst) != nullptr ||
         std::get_if<bir::StoreGlobalInst>(&inst) != nullptr;
}

[[nodiscard]] bool prepared_store_global_publication_run_has_prior_store(
    const bir::Block* block,
    std::size_t instruction_index) {
  if (block == nullptr || instruction_index == 0 ||
      instruction_index > block->insts.size()) {
    return false;
  }
  for (std::size_t index = instruction_index; index > 0; --index) {
    const auto& candidate = block->insts[index - 1U];
    if (!is_prepared_store_global_publication_run_instruction(candidate)) {
      return false;
    }
    if (std::get_if<bir::StoreGlobalInst>(&candidate) != nullptr) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] const bir::Value* prepared_source_producer_result_value(
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

[[nodiscard]] bool prepared_source_producer_has_matching_payload(
    const PreparedEdgePublicationSourceProducer& producer) {
  switch (producer.kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local != nullptr;
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary != nullptr;
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Immediate:
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return false;
  }
  return false;
}

[[nodiscard]] bool prepared_store_source_producer_metadata_agrees(
    const PreparedStoreSourcePublicationInputs& inputs) {
  const auto publication_instruction_index =
      inputs.publication_instruction_index.value_or(
          inputs.destination_access != nullptr
              ? inputs.destination_access->inst_index
              : std::size_t{0});
  if (inputs.source_value == nullptr ||
      inputs.destination_access == nullptr ||
      inputs.source_producer == nullptr ||
      inputs.source_producer->kind ==
          PreparedEdgePublicationSourceProducerKind::Unknown ||
      inputs.source_producer->block_label == kInvalidBlockLabel ||
      inputs.source_producer->block_label !=
          inputs.destination_access->block_label ||
      inputs.source_producer->instruction_index >=
          publication_instruction_index ||
      !prepared_source_producer_has_matching_payload(*inputs.source_producer)) {
    return false;
  }

  const bool home_backed_source_agrees =
      inputs.source_home != nullptr &&
      inputs.destination_access->stored_value_name.has_value() &&
      inputs.source_home->value_name != kInvalidValueName &&
      inputs.source_home->value_name ==
          *inputs.destination_access->stored_value_name &&
      inputs.source_home->kind != PreparedValueHomeKind::None;
  const bool byval_load_local_source_agrees =
      inputs.byval_load_local_source &&
      inputs.source_producer->kind ==
          PreparedEdgePublicationSourceProducerKind::LoadLocal;
  if (!home_backed_source_agrees && !byval_load_local_source_agrees) {
    return false;
  }

  const auto* produced_value =
      prepared_source_producer_result_value(*inputs.source_producer);
  return produced_value != nullptr &&
         produced_value->kind == bir::Value::Kind::Named &&
         inputs.source_value->kind == bir::Value::Kind::Named &&
         produced_value->name == inputs.source_value->name &&
         produced_value->type == inputs.source_value->type;
}

[[nodiscard]] bool prepared_source_producer_matches_store_value(
    const PreparedEdgePublicationSourceProducer& producer,
    const bir::Value& value,
    const bir::Inst& inst) {
  const auto* result = prepared_source_producer_result_value(producer);
  if (result == nullptr ||
      result->kind != bir::Value::Kind::Named ||
      value.kind != bir::Value::Kind::Named ||
      result->name != value.name ||
      result->type != value.type) {
    return false;
  }
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

[[nodiscard]] const PreparedEdgePublicationSourceProducer*
find_pending_store_global_source_producer(
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const PreparedStoreSourcePublicationPlan& plan,
    const bir::StoreGlobalInst& store,
    std::size_t instruction_index) {
  if (source_producers == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      plan.source_value_name == kInvalidValueName) {
    return nullptr;
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          source_producers, plan.source_value_name);
  if (producer == nullptr ||
      producer->kind == PreparedEdgePublicationSourceProducerKind::Unknown ||
      producer->block_label != block_label ||
      producer->instruction_index >= instruction_index ||
      producer->instruction_index >= block->insts.size()) {
    return nullptr;
  }
  return prepared_source_producer_matches_store_value(
             *producer, store.value, block->insts[producer->instruction_index])
             ? producer
             : nullptr;
}

[[nodiscard]] const PreparedMemoryAccess* find_publication_memory_access(
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    std::size_t instruction_index) {
  if (addressing == nullptr) {
    return nullptr;
  }
  const auto found =
      std::find_if(addressing->accesses.begin(),
                   addressing->accesses.end(),
                   [block_label, instruction_index](const PreparedMemoryAccess& access) {
                     return access.inst_index == instruction_index &&
                            (block_label == kInvalidBlockLabel ||
                             access.block_label == block_label);
                   });
  return found != addressing->accesses.end() ? &*found : nullptr;
}

[[nodiscard]] const PreparedValueHome* find_publication_source_home(
    const PreparedValueLocationFunction* value_locations,
    std::optional<ValueNameId> value_name) {
  if (value_locations == nullptr ||
      !value_name.has_value() ||
      *value_name == kInvalidValueName) {
    return nullptr;
  }
  const auto found =
      std::find_if(value_locations->value_homes.begin(),
                   value_locations->value_homes.end(),
                   [value_name](const PreparedValueHome& home) {
                     return home.value_name == *value_name;
                   });
  if (found != value_locations->value_homes.end()) {
    return &*found;
  }
  return nullptr;
}

[[nodiscard]] const PreparedMemoryAccess*
find_store_source_publication_access(
    const PreparedNameTables& names,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Value& stored_value,
    std::size_t instruction_index) {
  if (addressing == nullptr) {
    return nullptr;
  }
  if (const auto* exact =
          find_prepared_memory_access(*addressing, block_label, instruction_index);
      exact != nullptr) {
    return exact;
  }

  if (stored_value.kind != bir::Value::Kind::Named || stored_value.name.empty()) {
    return nullptr;
  }
  const auto stored_name = names.value_names.find(stored_value.name);
  if (stored_name == kInvalidValueName) {
    return nullptr;
  }

  const PreparedMemoryAccess* selected = nullptr;
  for (const auto& access : addressing->accesses) {
    if (access.block_label != block_label ||
        access.stored_value_name != std::optional<ValueNameId>{stored_name} ||
        access.result_value_name.has_value()) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &access;
  }
  return selected;
}

[[nodiscard]] PreparedScalarPublicationPlan missing_destination_home(
    const bir::Value* source_value,
    const PreparedBlockEntryPublication* publication) {
  PreparedScalarPublicationPlan plan{
      .status = PreparedScalarPublicationStatus::MissingDestinationHome,
      .current_block_entry_publication = publication,
      .current_block_entry_publication_available =
          publication != nullptr && prepared_block_entry_publication_available(*publication),
  };
  if (source_value != nullptr) {
    plan.source_value = *source_value;
  }
  return plan;
}

[[nodiscard]] PreparedScalarPublicationHookKind hook_kind_from_home(
    PreparedValueHomeKind home_kind) {
  switch (home_kind) {
    case PreparedValueHomeKind::Register:
      return PreparedScalarPublicationHookKind::RegisterHome;
    case PreparedValueHomeKind::StackSlot:
      return PreparedScalarPublicationHookKind::StackSlotHome;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedScalarPublicationHookKind::RematerializableImmediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedScalarPublicationHookKind::PointerBasePlusOffset;
    case PreparedValueHomeKind::None:
      return PreparedScalarPublicationHookKind::None;
  }
  return PreparedScalarPublicationHookKind::None;
}

[[nodiscard]] std::optional<unsigned> integer_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] const PreparedFrameSlot* find_frame_slot(
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedFrameSlot* prepared_frame_slot_for_access(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess* access) {
  if (access == nullptr ||
      access->address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return nullptr;
  }
  return find_frame_slot(stack_layout, *access->address.frame_slot_id);
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

[[nodiscard]] const bir::Value*
prepared_current_block_join_instruction_result_value_ref(const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> const bir::Value* {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result.has_value() ? &*typed_inst.result : nullptr;
        }
        return nullptr;
      },
      inst);
}

[[nodiscard]] bool prepared_load_access_matches_result(
    const PreparedNameTables& names,
    const PreparedMemoryAccess* access,
    const bir::LoadLocalInst& load) {
  const auto result_name = existing_prepared_value_name_id(names, load.result);
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
  const auto stored_name = existing_prepared_value_name_id(names, store.value);
  return stored_name.has_value() ? access->stored_value_name == stored_name
                                 : !access->stored_value_name.has_value();
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

[[nodiscard]] bool prepared_store_access_targets_load_byte(
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccess* load_access,
    const PreparedMemoryAccess* store_access,
    std::optional<std::size_t> load_lane_offset) {
  const auto load_offset = prepared_access_absolute_offset(stack_layout, load_access);
  const auto store_offset = prepared_access_absolute_offset(stack_layout, store_access);
  if (!load_offset.has_value() || !store_offset.has_value()) {
    return false;
  }
  const auto lane_offset =
      load_lane_offset.has_value()
          ? static_cast<std::int64_t>(*load_lane_offset)
          : std::int64_t{0};
  return *store_offset == *load_offset + lane_offset;
}

[[nodiscard]] bool prepared_recovered_store_source_agrees_with_bir(
    const PreparedMemoryAccess* load_access,
    const PreparedMemoryAccess* store_access,
    const bir::Block& block,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index,
    const bir::StoreLocalInst& store,
    std::size_t store_instruction_index) {
  const auto bir_identity =
      mir::find_bir_same_block_load_local_stored_value_source_identity(
          mir::BirSameBlockLoadLocalSourceRequest{
              .block = &block,
              .block_label = block.label,
              .root_value = &load.result,
              .before_instruction_index = load_instruction_index + 1U,
          });
  if (!bir_identity) {
    return true;
  }
  if (bir_identity.load_local != &load ||
      bir_identity.store_local != &store ||
      bir_identity.load_memory_access.instruction_index !=
          load_instruction_index ||
      bir_identity.store_memory_access.instruction_index !=
          store_instruction_index ||
      bir_identity.load_memory_access.node_kind !=
          mir::BirMemoryAccessNodeKind::LoadLocal ||
      bir_identity.store_memory_access.node_kind !=
          mir::BirMemoryAccessNodeKind::StoreLocal ||
      bir_identity.load_memory_access.base_kind !=
          mir::BirMemoryAccessBaseKind::LocalSlot ||
      bir_identity.store_memory_access.base_kind !=
          mir::BirMemoryAccessBaseKind::LocalSlot ||
      bir_identity.load_memory_access.result_value_name != load.result.name ||
      bir_identity.stored_value.name != store.value.name ||
      bir_identity.stored_value.type != store.value.type) {
    return false;
  }
  if (load_access == nullptr || store_access == nullptr ||
      load_access->address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      store_access->address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      !load_access->address.frame_slot_id.has_value() ||
      !store_access->address.frame_slot_id.has_value()) {
    return false;
  }
  return bir_identity.load_memory_access.local_slot_id ==
             static_cast<c4c::SlotNameId>(*load_access->address.frame_slot_id) &&
         bir_identity.store_memory_access.local_slot_id ==
             static_cast<c4c::SlotNameId>(*store_access->address.frame_slot_id) &&
         bir_identity.load_memory_access.byte_offset ==
             load_access->address.byte_offset &&
         bir_identity.store_memory_access.byte_offset ==
             store_access->address.byte_offset &&
         bir_identity.load_memory_access.size_bytes ==
             load_access->address.size_bytes &&
         bir_identity.store_memory_access.size_bytes ==
             store_access->address.size_bytes;
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

[[nodiscard]] std::optional<std::size_t> parse_trailing_dot_offset(
    std::string_view name) {
  const auto dot = name.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= name.size()) {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = dot + 1; index < name.size(); ++index) {
    const char ch = name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

[[nodiscard]] bool is_byval_formal_value_name(
    const PreparedNameTables& names,
    const bir::Function* bir_function,
    ValueNameId value_name) {
  if (bir_function == nullptr) {
    return false;
  }
  const std::string_view name = prepared_value_name(names, value_name);
  if (name.empty()) {
    return false;
  }
  for (const auto& param : bir_function->params) {
    if (param.is_byval && param.name == name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] const PreparedStackObject* find_stack_object(
    const PreparedStackLayout& stack_layout,
    PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::size_t> scalar_type_size_bytes(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::F64:
    case bir::TypeKind::Ptr:
      return std::size_t{8};
    case bir::TypeKind::F128:
      return std::size_t{16};
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool stack_source_size_is_aggregate_width(
    std::size_t source_size_bytes,
    bir::TypeKind source_type,
    bir::TypeKind destination_type) {
  const auto source_scalar_size = scalar_type_size_bytes(source_type);
  const auto destination_scalar_size = scalar_type_size_bytes(destination_type);
  if (source_scalar_size.has_value() && source_size_bytes > *source_scalar_size) {
    return true;
  }
  if (destination_scalar_size.has_value() &&
      source_size_bytes > *destination_scalar_size) {
    return true;
  }
  return !source_scalar_size.has_value() && !destination_scalar_size.has_value() &&
         source_size_bytes > 8;
}

[[nodiscard]] const PreparedStackObject* prepared_stack_object_for_frame_slot(
    const PreparedStackLayout& stack_layout,
    const PreparedFrameSlot* slot) {
  if (slot == nullptr) {
    return nullptr;
  }
  return find_stack_object(stack_layout, slot->object_id);
}

[[nodiscard]] BlockLabelId prepared_block_label_id(const PreparedNameTables& names,
                                                   const bir::Block& block) {
  const BlockLabelId structured = names.block_labels.find(block.label);
  return structured != kInvalidBlockLabel ? structured : kInvalidBlockLabel;
}

[[nodiscard]] const PreparedControlFlowFunction* find_control_flow_function(
    const PreparedControlFlow& control_flow,
    FunctionNameId function_name) {
  for (const auto& function : control_flow.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

void append_store_source_record(
    PreparedStoreSourcePublicationPlans& plans,
    FunctionNameId function_name,
    BlockLabelId block_label,
    std::size_t instruction_index,
    PreparedStoreSourcePublicationPlan plan) {
  plans.records.push_back(PreparedStoreSourcePublicationRecord{
      .function_name = function_name,
      .block_label = block_label,
      .instruction_index = instruction_index,
      .plan = std::move(plan),
  });
}

[[nodiscard]] std::optional<BlockLabelId> prepared_call_block_label(
    const PreparedBirModule& prepared,
    FunctionNameId function_name,
    const PreparedCallPlan& call_plan) {
  const auto* control_flow = find_control_flow_function(prepared.control_flow,
                                                       function_name);
  if (control_flow == nullptr ||
      call_plan.block_index >= control_flow->blocks.size()) {
    return std::nullopt;
  }
  const BlockLabelId block_label =
      control_flow->blocks[call_plan.block_index].block_label;
  return block_label != kInvalidBlockLabel
             ? std::optional<BlockLabelId>{block_label}
             : std::nullopt;
}

[[nodiscard]] const PreparedStoreSourcePublicationRecord*
find_unique_call_argument_value_publication_store_source(
    const PreparedBirModule& prepared,
    FunctionNameId function_name,
    const PreparedCallArgumentPlan& argument) {
  const auto need =
      find_prepared_missing_frame_slot_call_argument_publication_need(argument);
  if (!need.available ||
      need.kind != PreparedMissingFrameSlotCallArgumentPublicationKind::
                       FrameSlotAddress ||
      need.source_selection == nullptr ||
      need.source_selection !=
          (argument.source_selection.has_value() ? &*argument.source_selection
                                                 : nullptr) ||
      need.source_value_id != argument.source_value_id ||
      !need.source_materializes_address ||
      need.may_emit_local_aggregate_address_payload ||
      !argument.source_selection->address_materialization_block_label.has_value() ||
      !argument.source_selection->source_slot_id.has_value() ||
      !argument.source_selection->source_stack_offset_bytes.has_value() ||
      !argument.source_selection->source_size_bytes.has_value() ||
      *argument.source_selection->source_size_bytes != 8) {
    return nullptr;
  }

  const PreparedStoreSourcePublicationRecord* selected = nullptr;
  for (const auto& record : prepared.store_source_publications.records) {
    const auto& plan = record.plan;
    if (record.function_name != function_name ||
        record.block_label !=
            *argument.source_selection->address_materialization_block_label ||
        record.instruction_index > argument.instruction_index ||
        !prepared_store_source_publication_available(plan) ||
        plan.intent != PreparedStoreSourcePublicationIntent::StoreLocalPublication ||
        plan.duplicate_publication ||
        plan.destination_frame_slot_id != argument.source_selection->source_slot_id ||
        plan.destination_stack_offset_bytes !=
            argument.source_selection->source_stack_offset_bytes ||
        plan.destination_size_bytes != 8 ||
        plan.source_value_name == kInvalidValueName ||
        plan.source_value_id == 0 ||
        plan.source_value.type != bir::TypeKind::Ptr) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &record;
  }
  return selected;
}

void populate_call_argument_value_publication_plans(PreparedBirModule& prepared) {
  prepared.call_argument_value_publications.facts.clear();
  for (const auto& function_plan : prepared.call_plans.functions) {
    if (function_plan.function_name == kInvalidFunctionName) {
      continue;
    }
    for (const auto& call_plan : function_plan.calls) {
      const auto call_block =
          prepared_call_block_label(prepared, function_plan.function_name, call_plan);
      if (!call_block.has_value()) {
        continue;
      }
      for (const auto& argument : call_plan.arguments) {
        if (!argument.source_selection.has_value() ||
            !argument.source_value_id.has_value() ||
            !argument.source_selection->source_value_name.has_value() ||
            !argument.source_selection->source_slot_id.has_value() ||
            !argument.source_selection->source_stack_offset_bytes.has_value() ||
            !argument.source_selection->source_size_bytes.has_value()) {
          continue;
        }
        const auto* source =
            find_unique_call_argument_value_publication_store_source(
                prepared, function_plan.function_name, argument);
        if (source == nullptr) {
          continue;
        }
        const auto& plan = source->plan;
        prepared.call_argument_value_publications.facts.push_back(
            PreparedCallArgumentValuePublicationFact{
                .function_name = function_plan.function_name,
                .call_block_label = *call_block,
                .call_instruction_index = call_plan.instruction_index,
                .arg_index = argument.arg_index,
                .argument_value_id = *argument.source_value_id,
                .argument_value_name =
                    *argument.source_selection->source_value_name,
                .argument_object_slot_id =
                    *argument.source_selection->source_slot_id,
                .argument_object_stack_offset_bytes =
                    *argument.source_selection->source_stack_offset_bytes,
                .argument_object_size_bytes =
                    *argument.source_selection->source_size_bytes,
                .source_store_block_label = source->block_label,
                .source_store_instruction_index = source->instruction_index,
                .payload_value_id = plan.source_value_id,
                .payload_value_name = plan.source_value_name,
                .payload_value = plan.source_value,
                .destination_frame_slot_id = *plan.destination_frame_slot_id,
                .destination_stack_offset_bytes =
                    *plan.destination_stack_offset_bytes,
                .destination_size_bytes = plan.destination_size_bytes,
            });
      }
    }
  }
}

}  // namespace

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

[[nodiscard]] bool
prepared_edge_publication_redundant_block_entry_parallel_copy_move(
    const PreparedEdgePublication& publication,
    const PreparedMoveResolution* move) {
  if (move == nullptr ||
      publication.status != PreparedEdgePublicationLookupStatus::Available ||
      publication.move != move ||
      publication.phase != PreparedMovePhase::BlockEntry ||
      !publication.parallel_copy_step_index.has_value() ||
      publication.parallel_copy_step_index != move->source_parallel_copy_step_index) {
    return false;
  }
  return move->authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
         move->destination_kind == PreparedMoveDestinationKind::Value &&
         move->op_kind == PreparedMoveResolutionOpKind::Move &&
         publication.matching_move_redundant_by_assigned_storage;
}

[[nodiscard]] bool
prepared_edge_publication_matches_parallel_copy_move_source(
    const PreparedEdgePublication& publication,
    const PreparedMoveResolution& move,
    const PreparedValueHome& source_home) {
  if (publication.status != PreparedEdgePublicationLookupStatus::Available ||
      publication.move != &move ||
      publication.phase != PreparedMovePhase::BlockEntry ||
      publication.destination_value_id != move.to_value_id ||
      publication.source_home != &source_home ||
      publication.source_home_kind != source_home.kind ||
      source_home.value_id != move.from_value_id ||
      publication.source_value_id !=
          std::optional<PreparedValueId>{move.from_value_id} ||
      publication.source_value_name != source_home.value_name) {
    return false;
  }
  return move.destination_kind == PreparedMoveDestinationKind::Value &&
         move.op_kind == PreparedMoveResolutionOpKind::Move &&
         !move.source_immediate_i32.has_value();
}

[[nodiscard]] PreparedAggregateStackSourceAuthority
prepare_aggregate_stack_source_authority(
    const PreparedEdgePublication* publication) {
  PreparedAggregateStackSourceAuthority authority;
  if (publication == nullptr) {
    return authority;
  }

  authority.destination_value_id = publication->destination_value_id;
  authority.destination_value_name = publication->destination_value_name;
  authority.source_type = publication->source_value.type;
  authority.destination_type = publication->destination_value.type;
  authority.destination_storage_kind = publication->destination_storage_kind;

  if (publication->status != PreparedEdgePublicationLookupStatus::Available) {
    authority.status =
        PreparedAggregateStackSourceAuthorityStatus::UnsupportedPublication;
    return authority;
  }
  if (!publication->source_value_id.has_value()) {
    return authority;
  }
  authority.source_value_id = *publication->source_value_id;
  authority.source_value_name = publication->source_value_name;

  if (publication->source_home == nullptr ||
      publication->source_home_kind != PreparedValueHomeKind::StackSlot) {
    return authority;
  }
  if (publication->destination_home_kind != PreparedValueHomeKind::Register ||
      publication->destination_storage_kind != PreparedMoveStorageKind::Register) {
    authority.status =
        PreparedAggregateStackSourceAuthorityStatus::UnsupportedDestinationStorage;
    return authority;
  }

  authority.source_slot_id = publication->source_home->slot_id;
  authority.source_stack_offset_bytes = publication->source_home->offset_bytes;
  authority.source_stack_size_bytes = publication->source_home->size_bytes;
  authority.source_stack_align_bytes = publication->source_home->align_bytes;

  if (!authority.source_stack_size_bytes.has_value() ||
      !stack_source_size_is_aggregate_width(*authority.source_stack_size_bytes,
                                            publication->source_value.type,
                                            publication->destination_value.type)) {
    authority.status = PreparedAggregateStackSourceAuthorityStatus::Unavailable;
    return authority;
  }

  authority.copy_width_bytes = authority.source_stack_size_bytes;
  if (!authority.source_slot_id.has_value() ||
      !authority.source_stack_offset_bytes.has_value() ||
      !authority.source_stack_align_bytes.has_value()) {
    authority.status =
        PreparedAggregateStackSourceAuthorityStatus::IncompleteConcreteStackSource;
    return authority;
  }

  if (publication->move == nullptr ||
      publication->move->authority_kind !=
          PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      publication->move->destination_kind != PreparedMoveDestinationKind::Value ||
      publication->move->op_kind != PreparedMoveResolutionOpKind::Move ||
      publication->move->from_value_id != authority.source_value_id ||
      publication->move->to_value_id != authority.destination_value_id) {
    authority.status =
        PreparedAggregateStackSourceAuthorityStatus::UnsupportedMoveAuthority;
    return authority;
  }
  if (!publication->move->destination_register_placement.has_value()) {
    authority.status =
        PreparedAggregateStackSourceAuthorityStatus::IncompleteDestinationMapping;
    return authority;
  }
  authority.destination_register_placement =
      publication->move->destination_register_placement;

  authority.status =
      PreparedAggregateStackSourceAuthorityStatus::MissingAggregateCopyAuthority;
  return authority;
}

[[nodiscard]] PreparedTypedStackSourcePublication
prepare_same_width_i32_stack_source_publication(
    const PreparedEdgePublication* publication) {
  PreparedTypedStackSourcePublication prepared;
  prepared.publication = publication;
  if (publication == nullptr) {
    return prepared;
  }

  prepared.source_type = publication->source_value.type;
  prepared.destination_type = publication->destination_value.type;
  prepared.destination_value_id = publication->destination_value_id;
  prepared.destination_value_name = publication->destination_value_name;
  prepared.move = publication->move;

  if (publication->status != PreparedEdgePublicationLookupStatus::Available) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::UnsupportedPublication;
    return prepared;
  }
  if (!publication->source_value_id.has_value()) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::UnsupportedPublication;
    return prepared;
  }
  prepared.source_value_id = *publication->source_value_id;
  prepared.source_value_name = publication->source_value_name;

  if (publication->source_home == nullptr ||
      publication->source_home_kind != PreparedValueHomeKind::StackSlot) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::UnsupportedSourceHome;
    return prepared;
  }
  prepared.source_home = publication->source_home;
  prepared.source_slot_id = publication->source_home->slot_id;
  prepared.source_stack_offset_bytes = publication->source_home->offset_bytes;
  prepared.source_stack_size_bytes = publication->source_home->size_bytes;
  prepared.source_stack_align_bytes = publication->source_home->align_bytes;
  if (!prepared.source_slot_id.has_value() ||
      !prepared.source_stack_offset_bytes.has_value() ||
      !prepared.source_stack_size_bytes.has_value() ||
      *prepared.source_stack_size_bytes != 4) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::MissingConcreteStackSource;
    return prepared;
  }

  if (publication->source_value.type != bir::TypeKind::I32 ||
      publication->destination_value.type != bir::TypeKind::I32) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::MissingSameWidthI32Type;
    return prepared;
  }

  if (publication->destination_storage_kind != PreparedMoveStorageKind::Register) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::UnsupportedDestinationStorage;
    return prepared;
  }
  if (publication->move == nullptr) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::MissingDestinationRegisterPlacement;
    return prepared;
  }
  if (publication->move->authority_kind !=
          PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      publication->move->destination_kind != PreparedMoveDestinationKind::Value ||
      publication->move->op_kind != PreparedMoveResolutionOpKind::Move ||
      publication->move->from_value_id != prepared.source_value_id ||
      publication->move->to_value_id != prepared.destination_value_id) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::UnsupportedMoveAuthority;
    return prepared;
  }
  if (!publication->move->destination_register_placement.has_value()) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::MissingDestinationRegisterPlacement;
    return prepared;
  }

  prepared.destination_register_placement =
      publication->move->destination_register_placement;
  prepared.destination_register_bank =
      prepared.destination_register_placement->bank;
  if (prepared.destination_register_bank != PreparedRegisterBank::Gpr) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::MissingDestinationGprBank;
    return prepared;
  }
  if (!has_prepared_register_placement(*prepared.destination_register_placement)) {
    prepared.status =
        PreparedTypedStackSourcePublicationStatus::MissingDestinationRegisterView;
    return prepared;
  }

  prepared.extension_policy =
      PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension;
  prepared.status = PreparedTypedStackSourcePublicationStatus::Available;
  return prepared;
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

namespace {

[[nodiscard]] bool prepared_edge_publication_has_producer_pointer(
    const PreparedEdgePublication& publication) {
  switch (publication.source_producer_kind) {
    case PreparedEdgePublicationSourceProducerKind::Unknown:
    case PreparedEdgePublicationSourceProducerKind::Immediate:
      return true;
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return publication.source_load_local != nullptr;
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return publication.source_load_global != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return publication.source_cast != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return publication.source_binary != nullptr;
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return publication.source_select != nullptr;
  }
  return false;
}

void copy_prepared_edge_copy_source_fact_fields(
    PreparedEdgeCopySourceFacts& facts,
    const PreparedEdgePublication& publication) {
  facts.publication = &publication;
  facts.predecessor_label = publication.predecessor_label;
  facts.successor_label = publication.successor_label;
  facts.destination_value_id = publication.destination_value_id;
  facts.move = publication.move;
  facts.destination_value = publication.destination_value;
  facts.source_value = publication.source_value;
  facts.resolved_destination_value_id = publication.destination_value_id;
  facts.destination_value_name = publication.destination_value_name;
  facts.source_value_id = publication.source_value_id;
  facts.source_value_name = publication.source_value_name;
  facts.source_value_kind = publication.source_value_kind;
  facts.source_home = publication.source_home;
  facts.source_home_kind = publication.source_home_kind;
  facts.destination_home = publication.destination_home;
  facts.destination_home_kind = publication.destination_home_kind;
  facts.destination_storage_kind = publication.destination_storage_kind;
  facts.source_producer_kind = publication.source_producer_kind;
  facts.source_producer_block_label = publication.source_producer_block_label;
  facts.source_producer_instruction_index =
      publication.source_producer_instruction_index;
  facts.source_load_local = publication.source_load_local;
  facts.source_load_global = publication.source_load_global;
  facts.source_cast = publication.source_cast;
  facts.source_binary = publication.source_binary;
  facts.source_select = publication.source_select;
  facts.source_memory_access_status = publication.source_memory_access_status;
  facts.source_memory_access = publication.source_memory_access;
  facts.source_memory_base_kind = publication.source_memory_base_kind;
  facts.source_memory_frame_slot_id = publication.source_memory_frame_slot_id;
  facts.source_memory_symbol_name = publication.source_memory_symbol_name;
  facts.source_memory_pointer_value_name =
      publication.source_memory_pointer_value_name;
  facts.source_memory_byte_offset = publication.source_memory_byte_offset;
  facts.source_memory_size_bytes = publication.source_memory_size_bytes;
  facts.source_memory_align_bytes = publication.source_memory_align_bytes;
  facts.source_memory_address_space = publication.source_memory_address_space;
  facts.source_memory_is_volatile = publication.source_memory_is_volatile;
  facts.source_memory_can_use_base_plus_offset =
      publication.source_memory_can_use_base_plus_offset;
  facts.source_memory_layout_authority =
      publication.source_memory_layout_authority;
  facts.source_memory_range_verdict = publication.source_memory_range_verdict;
  facts.source_memory_dynamic_array_verdict =
      publication.source_memory_dynamic_array_verdict;
  facts.source_memory_requires_address_materialization =
      publication.source_memory_requires_address_materialization;
}

[[nodiscard]] PreparedEdgeCopySourceFactsStatus
validate_prepared_edge_copy_publication_source_facts(
    const PreparedEdgePublication& publication) {
  if (publication.status != PreparedEdgePublicationLookupStatus::Available) {
    return PreparedEdgeCopySourceFactsStatus::PublicationUnavailable;
  }
  if (publication.source_value_kind == bir::Value::Kind::Named &&
      (publication.source_value_name == kInvalidValueName ||
       !publication.source_value_id.has_value())) {
    return PreparedEdgeCopySourceFactsStatus::MissingSourceValue;
  }
  if (publication.source_value_kind == bir::Value::Kind::Named &&
      publication.source_home == nullptr) {
    return PreparedEdgeCopySourceFactsStatus::MissingSourceHome;
  }
  if (publication.source_producer_kind !=
          PreparedEdgePublicationSourceProducerKind::Unknown &&
      publication.source_producer_kind !=
          PreparedEdgePublicationSourceProducerKind::Immediate &&
      (!publication.source_producer_block_label.has_value() ||
       !publication.source_producer_instruction_index.has_value() ||
       !prepared_edge_publication_has_producer_pointer(publication))) {
    return PreparedEdgeCopySourceFactsStatus::MissingSourceProducer;
  }
  if (publication.source_producer_kind ==
          PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      publication.source_memory_access_status ==
          PreparedEdgePublicationSourceMemoryAccessStatus::MissingPreparedMemoryAccess) {
    return PreparedEdgeCopySourceFactsStatus::MissingSourceMemoryAccess;
  }
  if (publication.source_producer_kind ==
          PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      publication.source_memory_access_status ==
          PreparedEdgePublicationSourceMemoryAccessStatus::IncompletePreparedMemoryAccess) {
    return PreparedEdgeCopySourceFactsStatus::IncompleteSourceMemoryAccess;
  }
  return PreparedEdgeCopySourceFactsStatus::Available;
}

}  // namespace

[[nodiscard]] bool prepared_edge_copy_source_facts_have_materializable_producer(
    const PreparedEdgeCopySourceFacts& facts) {
  return facts.status == PreparedEdgeCopySourceFactsStatus::Available &&
         facts.publication != nullptr &&
         facts.source_producer_kind !=
             PreparedEdgePublicationSourceProducerKind::Unknown &&
         facts.source_producer_kind !=
             PreparedEdgePublicationSourceProducerKind::Immediate;
}

[[nodiscard]] bool prepared_edge_publication_source_home_matches_source(
    const PreparedEdgePublication& publication) {
  return publication.status == PreparedEdgePublicationLookupStatus::Available &&
         publication.source_home != nullptr &&
         publication.source_home->value_name == publication.source_value_name &&
         publication.source_home->kind == publication.source_home_kind;
}

[[nodiscard]] bool prepared_edge_publication_source_memory_matches_access(
    const PreparedEdgePublication& publication,
    const PreparedMemoryAccess& access) {
  return publication.status == PreparedEdgePublicationLookupStatus::Available &&
         access.result_value_name.has_value() &&
         *access.result_value_name == publication.source_value_name &&
         publication.source_memory_base_kind == access.address.base_kind &&
         publication.source_memory_frame_slot_id == access.address.frame_slot_id &&
         publication.source_memory_symbol_name == access.address.symbol_name &&
         publication.source_memory_pointer_value_name ==
             access.address.pointer_value_name &&
         publication.source_memory_byte_offset == access.address.byte_offset &&
         publication.source_memory_size_bytes == access.address.size_bytes &&
         publication.source_memory_align_bytes == access.address.align_bytes &&
         publication.source_memory_address_space == access.address_space &&
         publication.source_memory_is_volatile == access.is_volatile &&
         publication.source_memory_can_use_base_plus_offset ==
             access.address.can_use_base_plus_offset &&
         publication.source_memory_layout_authority ==
             access.address.provenance.layout_authority &&
         publication.source_memory_range_verdict ==
             access.address.provenance.range_verdict &&
         publication.source_memory_dynamic_array_verdict ==
             access.address.provenance.dynamic_array.verdict;
}

[[nodiscard]] bool prepared_value_homes_share_register_name(
    const PreparedValueHome& lhs,
    const PreparedValueHome& rhs) {
  return lhs.kind == PreparedValueHomeKind::Register &&
         rhs.kind == PreparedValueHomeKind::Register &&
         lhs.register_name.has_value() &&
         rhs.register_name.has_value() &&
         *lhs.register_name == *rhs.register_name;
}

[[nodiscard]] bool
prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
    const PreparedMoveResolution& move,
    PreparedValueId value_id) {
  return move.authority_kind == PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
         move.destination_kind == PreparedMoveDestinationKind::Value &&
         move.destination_storage_kind == PreparedMoveStorageKind::Register &&
         move.op_kind == PreparedMoveResolutionOpKind::Move &&
         move.to_value_id == value_id;
}

[[nodiscard]] bool
prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
    const PreparedMoveResolution& move,
    const PreparedValueHome& source_home,
    const PreparedValueHome& destination_home) {
  return prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
             move, destination_home.value_id) &&
         !move.source_immediate_i32.has_value() &&
         move.from_value_id == source_home.value_id &&
         move.from_value_id != move.to_value_id &&
         prepared_value_homes_share_register_name(source_home, destination_home);
}

[[nodiscard]] PreparedEdgeCopySourceFacts prepare_edge_copy_source_facts(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    PreparedValueId destination_value_id) {
  PreparedEdgeCopySourceFacts facts{
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .destination_value_id = destination_value_id,
  };
  if (lookups == nullptr) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingPreparedLookups;
    return facts;
  }
  if (predecessor_label == kInvalidBlockLabel) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel;
    return facts;
  }
  if (successor_label == kInvalidBlockLabel) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingSuccessorLabel;
    return facts;
  }
  if (destination_value_id == PreparedValueId{0}) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingDestinationValue;
    return facts;
  }
  const auto* publications = find_indexed_prepared_edge_publications(
      lookups, predecessor_label, successor_label, destination_value_id);
  if (publications == nullptr || publications->empty()) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingPublication;
    return facts;
  }
  if (publications->size() != 1) {
    facts.status = PreparedEdgeCopySourceFactsStatus::AmbiguousPublication;
    return facts;
  }
  const auto* publication = publications->front();
  if (publication == nullptr) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingPublication;
    return facts;
  }
  copy_prepared_edge_copy_source_fact_fields(facts, *publication);
  if (publication->predecessor_label != predecessor_label ||
      publication->successor_label != successor_label ||
      publication->destination_value_id != destination_value_id) {
    facts.status = PreparedEdgeCopySourceFactsStatus::EdgeMismatch;
    return facts;
  }
  facts.status =
      validate_prepared_edge_copy_publication_source_facts(*publication);
  return facts;
}

[[nodiscard]] const PreparedEdgePublication*
find_unique_indexed_block_entry_parallel_copy_edge_publication(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    const PreparedMoveResolution& move) {
  if (predecessor_label == kInvalidBlockLabel ||
      successor_label == kInvalidBlockLabel ||
      move.destination_kind != PreparedMoveDestinationKind::Value ||
      move.op_kind != PreparedMoveResolutionOpKind::Move ||
      (move.source_parallel_copy_predecessor_label.has_value() &&
       *move.source_parallel_copy_predecessor_label != predecessor_label) ||
      (move.source_parallel_copy_successor_label.has_value() &&
       *move.source_parallel_copy_successor_label != successor_label)) {
    return nullptr;
  }

  const auto* publication =
      find_unique_indexed_prepared_edge_publication(lookups,
                                                    predecessor_label,
                                                    successor_label,
                                                    move.to_value_id);
  if (publication == nullptr ||
      publication->status != PreparedEdgePublicationLookupStatus::Available ||
      publication->phase != PreparedMovePhase::BlockEntry ||
      publication->predecessor_label != predecessor_label ||
      publication->successor_label != successor_label ||
      publication->destination_value_id != move.to_value_id) {
    return nullptr;
  }
  return publication;
}

[[nodiscard]] PreparedEdgeCopySourceFacts
prepare_block_entry_parallel_copy_edge_source_facts(
    const PreparedEdgePublicationLookups* lookups,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label,
    const PreparedMoveResolution& move) {
  PreparedEdgeCopySourceFacts facts{
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .destination_value_id = move.to_value_id,
      .move = &move,
  };
  if (predecessor_label == kInvalidBlockLabel) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel;
    return facts;
  }
  if (successor_label == kInvalidBlockLabel) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MissingSuccessorLabel;
    return facts;
  }
  if (move.destination_kind != PreparedMoveDestinationKind::Value ||
      move.op_kind != PreparedMoveResolutionOpKind::Move) {
    facts.status = PreparedEdgeCopySourceFactsStatus::UnsupportedMove;
    return facts;
  }
  if ((move.source_parallel_copy_predecessor_label.has_value() &&
       *move.source_parallel_copy_predecessor_label != predecessor_label) ||
      (move.source_parallel_copy_successor_label.has_value() &&
       *move.source_parallel_copy_successor_label != successor_label)) {
    facts.status = PreparedEdgeCopySourceFactsStatus::MoveEdgeMismatch;
    return facts;
  }

  facts = prepare_edge_copy_source_facts(
      lookups, predecessor_label, successor_label, move.to_value_id);
  facts.move = &move;
  if (facts.status != PreparedEdgeCopySourceFactsStatus::Available) {
    return facts;
  }
  if (facts.publication == nullptr ||
      facts.publication->phase != PreparedMovePhase::BlockEntry) {
    facts.status = PreparedEdgeCopySourceFactsStatus::PublicationUnavailable;
    return facts;
  }
  if (facts.publication->move != &move) {
    facts.status = PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch;
    return facts;
  }
  if (move.source_immediate_i32.has_value()) {
    if (facts.source_value_kind != bir::Value::Kind::Immediate ||
        facts.source_value.immediate != *move.source_immediate_i32) {
      facts.status = PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch;
    }
    return facts;
  }
  if (!facts.source_value_id.has_value() ||
      *facts.source_value_id != move.from_value_id) {
    facts.status = PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch;
  }
  return facts;
}

[[nodiscard]] bool route5_join_source_record_agrees_with_prepared_fact(
    const PreparedNameTables& names,
    const PreparedCurrentBlockJoinParallelCopySourceFact& fact,
    const bir::Route5CurrentBlockJoinSourceRecord& route5_join_source) {
  if (fact.status != PreparedEdgeCopySourceFactsStatus::Available ||
      fact.publication == nullptr ||
      route5_join_source.status != bir::Route5PublicationStatus::Available ||
      !route5_join_source) {
    return false;
  }
  if (route5_join_source.predecessor_label_id != fact.predecessor_label ||
      route5_join_source.successor_label_id != fact.successor_label) {
    return false;
  }
  if (route5_join_source.destination_value_name !=
          prepared_value_name(names, fact.destination_value_name) ||
      route5_join_source.destination_value_type !=
          fact.publication->destination_value.type) {
    return false;
  }
  if (fact.destination_home == nullptr ||
      fact.destination_home->value_id != fact.destination_value_id ||
      fact.destination_home->value_name != fact.destination_value_name) {
    return false;
  }
  if (fact.immediate_source) {
    return route5_join_source.source_value_kind == bir::Value::Kind::Immediate &&
           route5_join_source.source_value.value_kind ==
               bir::Value::Kind::Immediate &&
           route5_join_source.source_value.integer_constant ==
               fact.publication->source_value.immediate &&
           route5_join_source.source_value_type ==
               fact.publication->source_value.type;
  }
  if (!fact.source_value_id.has_value() ||
      fact.source_home == nullptr ||
      fact.source_home->value_id != *fact.source_value_id ||
      fact.source_home->value_name != fact.source_value_name) {
    return false;
  }
  return route5_join_source.source_value_kind == bir::Value::Kind::Named &&
         route5_join_source.source_value_name ==
             prepared_value_name(names, fact.source_value_name) &&
         route5_join_source.source_value_type == fact.publication->source_value.type &&
         route5_join_source.source_producer_instruction != nullptr &&
         route5_join_source.source_producer_instruction_index.has_value();
}

void attach_route5_current_block_join_source_if_agrees(
    const PreparedNameTables& names,
    const bir::Route5EdgeJoinSourceIndex* route5_edge_join_sources,
    const bir::Block* successor_block,
    PreparedCurrentBlockJoinParallelCopySourceFact& fact) {
  if (route5_edge_join_sources == nullptr ||
      successor_block == nullptr ||
      fact.publication == nullptr ||
      fact.status != PreparedEdgeCopySourceFactsStatus::Available ||
      fact.immediate_source ||
      !fact.source_is_incoming_expression ||
      !fact.destination_is_source_value ||
      fact.source_is_source_value ||
      fact.source_home_is_stack) {
    return;
  }
  const auto route5_join_source = bir::route5_find_current_block_join_source(
      *route5_edge_join_sources,
      *successor_block,
      fact.publication->destination_value,
      fact.publication->source_value);
  fact.route5_join_source_status = route5_join_source.status;
  if (!route5_join_source_record_agrees_with_prepared_fact(names,
                                                           fact,
                                                           route5_join_source)) {
    return;
  }
  const bir::Route5CurrentBlockJoinSourceRecord* unique_agreeing_record = nullptr;
  for (const auto& candidate : route5_edge_join_sources->join_records) {
    if (candidate.successor_block != successor_block &&
        (candidate.successor_label_id == kInvalidBlockLabel ||
         successor_block->label_id == kInvalidBlockLabel ||
         candidate.successor_label_id != successor_block->label_id)) {
      continue;
    }
    if (!route5_join_source_record_agrees_with_prepared_fact(names,
                                                             fact,
                                                             candidate)) {
      continue;
    }
    if (unique_agreeing_record != nullptr) {
      fact.route5_join_source_status = bir::Route5PublicationStatus::NoMatch;
      return;
    }
    unique_agreeing_record = &candidate;
  }
  if (unique_agreeing_record == nullptr) {
    return;
  }
  fact.route5_join_source = unique_agreeing_record;
  fact.route5_join_source_agrees = true;
}

[[nodiscard]] PreparedCurrentBlockJoinParallelCopySourceFacts
prepare_current_block_join_parallel_copy_source_facts(
    const PreparedCurrentBlockJoinParallelCopySourceQueryInputs& inputs) {
  PreparedCurrentBlockJoinParallelCopySourceFacts result;
  if (inputs.names == nullptr) {
    result.status = PreparedCurrentBlockJoinParallelCopySourceStatus::MissingNames;
    return result;
  }
  if (inputs.value_locations == nullptr) {
    result.status =
        PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations;
    return result;
  }
  if (inputs.edge_publications == nullptr) {
    result.status =
        PreparedCurrentBlockJoinParallelCopySourceStatus::MissingEdgePublicationLookups;
    return result;
  }
  if (inputs.block == nullptr) {
    result.status = PreparedCurrentBlockJoinParallelCopySourceStatus::MissingBlock;
    return result;
  }
  if (inputs.successor_label == kInvalidBlockLabel) {
    result.status =
        PreparedCurrentBlockJoinParallelCopySourceStatus::MissingSuccessorLabel;
    return result;
  }

  result.status = PreparedCurrentBlockJoinParallelCopySourceStatus::Available;

  auto append_value_id = [](std::vector<PreparedValueId>& values,
                            PreparedValueId value_id) {
    if (value_id != PreparedValueId{0} &&
        std::find(values.begin(), values.end(), value_id) == values.end()) {
      values.push_back(value_id);
    }
  };
  auto append_value_name = [](std::vector<ValueNameId>& values,
                              ValueNameId value_name) {
    if (value_name != kInvalidValueName &&
        std::find(values.begin(), values.end(), value_name) == values.end()) {
      values.push_back(value_name);
    }
  };
  auto append_source_value = [&](const PreparedValueHome* home) {
    if (home == nullptr) {
      return;
    }
    append_value_id(result.source_value_ids, home->value_id);
    append_value_name(result.source_value_names, home->value_name);
  };
  auto append_incoming_expression = [&](const PreparedValueHome* home) {
    if (home == nullptr) {
      return;
    }
    append_value_id(result.incoming_expression_value_ids, home->value_id);
    append_value_name(result.incoming_expression_value_names, home->value_name);
  };

  for (const auto& bundle : inputs.value_locations->move_bundles) {
    if (bundle.phase != PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<BlockLabelId>{inputs.successor_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      PreparedCurrentBlockJoinParallelCopySourceFact fact{
          .bundle = &bundle,
          .move = &move,
          .predecessor_label =
              bundle.source_parallel_copy_predecessor_label.value_or(kInvalidBlockLabel),
          .successor_label = inputs.successor_label,
          .destination_value_id = move.to_value_id,
          .destination_storage_kind = move.destination_storage_kind,
          .destination_register_name = move.destination_register_name,
          .immediate_source = move.source_immediate_i32.has_value(),
      };
      if (bundle.source_parallel_copy_predecessor_label == std::nullopt) {
        fact.status = PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel;
        result.facts.push_back(fact);
        continue;
      }
      if (move.destination_kind != PreparedMoveDestinationKind::Value ||
          move.op_kind != PreparedMoveResolutionOpKind::Move) {
        fact.status = PreparedEdgeCopySourceFactsStatus::UnsupportedMove;
        result.facts.push_back(fact);
        continue;
      }

      const auto source_facts =
          prepare_block_entry_parallel_copy_edge_source_facts(
              inputs.edge_publications,
              *bundle.source_parallel_copy_predecessor_label,
              inputs.successor_label,
              move);
      fact.status = source_facts.status;
      fact.publication = source_facts.publication;
      fact.source_value_id = source_facts.source_value_id;
      fact.source_value_name = source_facts.source_value_name;
      fact.source_home = source_facts.source_home;
      fact.destination_home = source_facts.destination_home;
      fact.source_home_kind = source_facts.source_home_kind;
      fact.destination_home_kind = source_facts.destination_home_kind;
      fact.destination_value_name = source_facts.destination_value_name;
      fact.destination_storage_kind = source_facts.destination_storage_kind;
      fact.destination_register_name = move.destination_register_name;

      if (fact.destination_home == nullptr) {
        fact.destination_home =
            find_indexed_prepared_value_home(inputs.value_home_lookups,
                                             inputs.value_locations,
                                             move.to_value_id);
        if (fact.destination_home != nullptr) {
          fact.destination_home_kind = fact.destination_home->kind;
          fact.destination_value_name = fact.destination_home->value_name;
        }
      }
      if (!move.source_immediate_i32.has_value() && fact.source_home == nullptr) {
        fact.source_home =
            find_indexed_prepared_value_home(inputs.value_home_lookups,
                                             inputs.value_locations,
                                             move.from_value_id);
        if (fact.source_home != nullptr) {
          fact.source_home_kind = fact.source_home->kind;
          fact.source_value_id = fact.source_home->value_id;
          fact.source_value_name = fact.source_home->value_name;
        }
      }

      if (fact.status == PreparedEdgeCopySourceFactsStatus::Available) {
        if (!move.source_immediate_i32.has_value()) {
          fact.source_is_incoming_expression = fact.source_home != nullptr;
          append_incoming_expression(fact.source_home);
        }
        fact.destination_is_source_value =
            prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
                move, move.to_value_id);
        if (fact.destination_is_source_value) {
          append_source_value(fact.destination_home);
        }
        fact.source_home_is_stack =
            fact.source_home != nullptr &&
            fact.source_home->kind == PreparedValueHomeKind::StackSlot;
        fact.source_shares_destination_register =
            fact.source_home != nullptr &&
            fact.destination_home != nullptr &&
            prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
                move, *fact.source_home, *fact.destination_home);
        fact.source_is_source_value =
            fact.destination_is_source_value &&
            !move.source_immediate_i32.has_value() &&
            move.from_value_id != move.to_value_id &&
            (fact.source_shares_destination_register || fact.source_home_is_stack);
        if (fact.source_is_source_value) {
          append_source_value(fact.source_home);
        }
        attach_route5_current_block_join_source_if_agrees(
            *inputs.names, inputs.route5_edge_join_sources, inputs.block, fact);
      }

      result.facts.push_back(fact);
    }
  }

  std::vector<ValueNameId> pending_expression_names =
      result.incoming_expression_value_names;
  std::vector<ValueNameId> processed_expression_names;
  auto append_operand = [&](const bir::Value& value) {
    const auto value_name = existing_prepared_value_name_id(*inputs.names, value);
    if (value_name.has_value()) {
      pending_expression_names.push_back(*value_name);
    }
  };
  while (!pending_expression_names.empty()) {
    const auto value_name = pending_expression_names.back();
    pending_expression_names.pop_back();
    if (std::find(processed_expression_names.begin(),
                  processed_expression_names.end(),
                  value_name) != processed_expression_names.end()) {
      continue;
    }
    processed_expression_names.push_back(value_name);
    append_value_name(result.incoming_expression_value_names, value_name);
    const bir::Inst* producer = nullptr;
    for (const auto& inst : inputs.block->insts) {
      const auto* inst_result =
          prepared_current_block_join_instruction_result_value_ref(inst);
      if (inst_result == nullptr) {
        continue;
      }
      const auto inst_result_name =
          existing_prepared_value_name_id(*inputs.names, *inst_result);
      if (inst_result_name == std::optional<ValueNameId>{value_name}) {
        producer = &inst;
        break;
      }
    }
    if (producer == nullptr) {
      continue;
    }
    std::visit(
        [&](const auto& typed_inst) {
          using T = std::decay_t<decltype(typed_inst)>;
          if constexpr (std::is_same_v<T, bir::BinaryInst>) {
            append_operand(typed_inst.lhs);
            append_operand(typed_inst.rhs);
          } else if constexpr (std::is_same_v<T, bir::CastInst>) {
            append_operand(typed_inst.operand);
          } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
            append_operand(typed_inst.lhs);
            append_operand(typed_inst.rhs);
            append_operand(typed_inst.true_value);
            append_operand(typed_inst.false_value);
          }
        },
        *producer);
  }

  for (const auto value_name : result.incoming_expression_value_names) {
    const auto value_id = find_indexed_prepared_value_id(inputs.value_home_lookups,
                                                        inputs.regalloc,
                                                        inputs.value_locations,
                                                        value_name);
    if (value_id.has_value()) {
      append_value_id(result.incoming_expression_value_ids, *value_id);
    }
  }
  return result;
}

bool prepared_scalar_publication_available(
    const PreparedScalarPublicationPlan& plan) {
  return plan.status == PreparedScalarPublicationStatus::Available;
}

PreparedStorageEncodingKind prepared_storage_encoding_from_value_home_kind(
    PreparedValueHomeKind home_kind) {
  switch (home_kind) {
    case PreparedValueHomeKind::Register:
      return PreparedStorageEncodingKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedStorageEncodingKind::FrameSlot;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedStorageEncodingKind::Immediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedStorageEncodingKind::ComputedAddress;
    case PreparedValueHomeKind::None:
      return PreparedStorageEncodingKind::None;
  }
  return PreparedStorageEncodingKind::None;
}

PreparedStorageEncodingKind prepared_publication_storage_encoding_from_home(
    PreparedValueHomeKind home_kind) {
  return prepared_storage_encoding_from_value_home_kind(home_kind);
}

PreparedScalarPublicationPlan plan_prepared_scalar_publication(
    const PreparedScalarPublicationInputs& inputs) {
  if (inputs.destination_home == nullptr) {
    return missing_destination_home(inputs.source_value,
                                    inputs.current_block_entry_publication);
  }

  const PreparedValueHome& home = *inputs.destination_home;
  PreparedScalarPublicationPlan plan{
      .status = PreparedScalarPublicationStatus::Available,
      .hook_kind = hook_kind_from_home(home.kind),
      .source_value_id = home.value_id,
      .source_value_name = home.value_name,
      .destination_home = &home,
      .destination_home_kind = home.kind,
      .storage_encoding = prepared_storage_encoding_from_value_home_kind(home.kind),
      .current_block_entry_publication = inputs.current_block_entry_publication,
      .current_block_entry_publication_available =
          inputs.current_block_entry_publication != nullptr &&
          prepared_block_entry_publication_available(*inputs.current_block_entry_publication),
      .slot_id = home.slot_id,
      .stack_offset_bytes = home.offset_bytes,
      .size_bytes = home.size_bytes,
      .align_bytes = home.align_bytes,
      .immediate_i32 = home.immediate_i32,
      .immediate_f128 = home.immediate_f128,
      .pointer_base_value_name = home.pointer_base_value_name,
      .pointer_base_symbol_name = home.pointer_base_symbol_name,
      .pointer_byte_delta = home.pointer_byte_delta,
  };

  if (inputs.source_value == nullptr) {
    plan.status = PreparedScalarPublicationStatus::MissingSourceValue;
    return plan;
  }
  plan.source_value = *inputs.source_value;

  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return plan;
    case PreparedValueHomeKind::StackSlot:
      if (!home.slot_id.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingStackSlot;
      }
      return plan;
    case PreparedValueHomeKind::RematerializableImmediate:
      if (!home.immediate_i32.has_value() && !home.immediate_f128.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingImmediatePayload;
      }
      return plan;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      if (!home.pointer_base_value_name.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingPointerBase;
      } else if (!home.pointer_byte_delta.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingPointerDelta;
      }
      return plan;
    case PreparedValueHomeKind::None:
      plan.status = PreparedScalarPublicationStatus::UnsupportedHomeKind;
      return plan;
  }
  plan.status = PreparedScalarPublicationStatus::UnsupportedHomeKind;
  return plan;
}

bool prepared_store_source_publication_available(
    const PreparedStoreSourcePublicationPlan& plan) {
  return plan.status == PreparedStoreSourcePublicationStatus::Available;
}

bool prepared_store_global_publication_has_authority(
    const PreparedStoreSourcePublicationPlan& plan) {
  if (!prepared_store_source_publication_available(plan) ||
      plan.intent != PreparedStoreSourcePublicationIntent::StoreGlobalPublication ||
      plan.destination_access == nullptr ||
      plan.destination_base_kind != PreparedAddressBaseKind::GlobalSymbol ||
      !prepared_global_symbol_memory_has_publication_authority(
          plan.destination_access->address)) {
    return false;
  }

  if (plan.source_value.kind == bir::Value::Kind::Immediate) {
    return plan.source_producer_kind ==
           PreparedEdgePublicationSourceProducerKind::Immediate;
  }

  if (plan.source_value.kind != bir::Value::Kind::Named ||
      plan.source_value_name == kInvalidValueName ||
      plan.source_home == nullptr ||
      plan.source_home_kind == PreparedValueHomeKind::None ||
      plan.source_home->value_name != plan.source_value_name ||
      plan.source_producer_kind ==
          PreparedEdgePublicationSourceProducerKind::Unknown ||
      plan.source_producer_kind ==
          PreparedEdgePublicationSourceProducerKind::Immediate ||
      !plan.source_producer_block_label.has_value() ||
      !plan.source_producer_instruction_index.has_value()) {
    return false;
  }

  switch (plan.source_producer_kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return plan.source_load_local != nullptr;
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return plan.source_load_global != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return plan.source_cast != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Binary:
      return plan.source_binary != nullptr;
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return plan.source_select != nullptr;
    case PreparedEdgePublicationSourceProducerKind::Unknown:
    case PreparedEdgePublicationSourceProducerKind::Immediate:
      return false;
  }
  return false;
}

PreparedDirectGlobalReturnAuthority
plan_prepared_direct_global_return_authority(
    const PreparedDirectGlobalReturnAuthorityInputs& inputs) {
  PreparedDirectGlobalReturnAuthority authority{
      .return_value = inputs.return_value,
      .value_home = inputs.value_home,
      .before_return_move = inputs.before_return_move,
  };

  if (inputs.names == nullptr) {
    authority.status = PreparedDirectGlobalReturnAuthorityStatus::MissingNames;
    return authority;
  }
  if (inputs.return_value == nullptr) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::MissingReturnValue;
    return authority;
  }
  if (inputs.return_value->kind != bir::Value::Kind::Named ||
      inputs.return_value->type != bir::TypeKind::Ptr) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::UnsupportedReturnValue;
    return authority;
  }
  if (inputs.return_value->pointer_symbol_link_name_id == kInvalidLinkName) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::MissingGlobalIdentity;
    return authority;
  }
  authority.global_symbol_name =
      inputs.return_value->pointer_symbol_link_name_id;

  if (inputs.value_home == nullptr) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::MissingValueHome;
    return authority;
  }
  authority.value_id = inputs.value_home->value_id;
  authority.value_name = inputs.value_home->value_name;
  if (inputs.value_home->function_name == kInvalidFunctionName ||
      inputs.value_home->value_name == kInvalidValueName ||
      inputs.value_home->value_id == 0 ||
      prepared_value_name(*inputs.names, inputs.value_home->value_name) !=
          inputs.return_value->name) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::HomeValueMismatch;
    return authority;
  }
  if (inputs.value_home->kind != PreparedValueHomeKind::Register ||
      !inputs.value_home->register_name.has_value()) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::UnsupportedHomeKind;
    return authority;
  }

  if (inputs.before_return_move == nullptr) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::MissingReturnMove;
    return authority;
  }
  if (inputs.before_return_move->block_index != inputs.block_index ||
      inputs.before_return_move->instruction_index != inputs.instruction_index ||
      inputs.before_return_move->from_value_id != inputs.value_home->value_id) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::ReturnMoveMismatch;
    return authority;
  }
  if (inputs.before_return_move->destination_kind !=
          PreparedMoveDestinationKind::FunctionReturnAbi ||
      inputs.before_return_move->destination_storage_kind !=
          PreparedMoveStorageKind::Register ||
      !inputs.before_return_move->destination_register_name.has_value() ||
      !inputs.before_return_move->destination_register_placement.has_value() ||
      !has_prepared_register_placement(
          *inputs.before_return_move->destination_register_placement) ||
      inputs.before_return_move->destination_register_placement->bank !=
          PreparedRegisterBank::Gpr) {
    authority.status =
        PreparedDirectGlobalReturnAuthorityStatus::UnsupportedReturnDestination;
    return authority;
  }

  authority.return_bank =
      inputs.before_return_move->destination_register_placement->bank;
  authority.status = PreparedDirectGlobalReturnAuthorityStatus::Available;
  return authority;
}

bool prepared_direct_global_return_authority_available(
    const PreparedDirectGlobalReturnAuthority& authority) {
  return authority.status ==
         PreparedDirectGlobalReturnAuthorityStatus::Available;
}

[[nodiscard]] bool prepared_pointer_value_is_null_immediate(
    const bir::Value& value) {
  return value.kind == bir::Value::Kind::Immediate &&
         value.type == bir::TypeKind::Ptr &&
         value.immediate == 0 &&
         value.immediate_bits == 0 &&
         !value.f128_payload.has_value();
}

[[nodiscard]] bool prepared_home_names_value(
    const PreparedNameTables& names,
    const PreparedValueHome& home,
    const bir::Value& value) {
  return value.kind == bir::Value::Kind::Named &&
         home.function_name != kInvalidFunctionName &&
         home.value_name != kInvalidValueName &&
         home.value_id != 0 &&
         prepared_value_name(names, home.value_name) == value.name;
}

[[nodiscard]] bool prepared_home_has_gpr_compatible_register(
    const PreparedValueHome& home) {
  return (home.kind == PreparedValueHomeKind::Register ||
          home.kind == PreparedValueHomeKind::PointerBasePlusOffset) &&
         home.register_name.has_value();
}

[[nodiscard]] bool prepared_pointer_branch_predicate_is_supported(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

[[nodiscard]] PreparedFusedPointerBranchPublicationStatus
validate_prepared_pointer_branch_operand_home(
    const PreparedNameTables& names,
    const bir::Value& value,
    const PreparedValueHome* home,
    PreparedFusedPointerBranchPublicationStatus missing_home_status) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return prepared_pointer_value_is_null_immediate(value)
               ? PreparedFusedPointerBranchPublicationStatus::Available
               : PreparedFusedPointerBranchPublicationStatus::UnsupportedOperand;
  }
  if (value.kind != bir::Value::Kind::Named || value.type != bir::TypeKind::Ptr ||
      value.name.empty()) {
    return PreparedFusedPointerBranchPublicationStatus::UnsupportedOperand;
  }
  if (home == nullptr) {
    return missing_home_status;
  }
  if (!prepared_home_names_value(names, *home, value)) {
    return PreparedFusedPointerBranchPublicationStatus::HomeValueMismatch;
  }
  if (!prepared_home_has_gpr_compatible_register(*home)) {
    return PreparedFusedPointerBranchPublicationStatus::UnsupportedOperandHome;
  }
  return PreparedFusedPointerBranchPublicationStatus::Available;
}

PreparedFusedPointerBranchPublication
plan_prepared_fused_pointer_branch_publication(
    const PreparedFusedPointerBranchPublicationInputs& inputs) {
  PreparedFusedPointerBranchPublication publication{
      .branch_condition = inputs.branch_condition,
      .terminator = inputs.terminator,
      .condition_home = inputs.condition_home,
      .lhs_home = inputs.lhs_home,
      .rhs_home = inputs.rhs_home,
  };

  if (inputs.names == nullptr) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::MissingNames;
    return publication;
  }
  if (inputs.branch_condition == nullptr) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::MissingBranchCondition;
    return publication;
  }
  if (inputs.terminator == nullptr) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::MissingTerminator;
    return publication;
  }
  if (inputs.terminator->kind != bir::TerminatorKind::CondBranch) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::UnsupportedTerminator;
    return publication;
  }
  if (inputs.branch_condition->kind !=
          PreparedBranchConditionKind::FusedCompare ||
      !inputs.branch_condition->can_fuse_with_branch ||
      !inputs.branch_condition->predicate.has_value() ||
      !inputs.branch_condition->compare_type.has_value() ||
      !inputs.branch_condition->lhs.has_value() ||
      !inputs.branch_condition->rhs.has_value()) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::UnsupportedBranchCondition;
    return publication;
  }
  publication.predicate = *inputs.branch_condition->predicate;
  if (!prepared_pointer_branch_predicate_is_supported(publication.predicate)) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::UnsupportedPredicate;
    return publication;
  }
  if (*inputs.branch_condition->compare_type != bir::TypeKind::Ptr ||
      inputs.branch_condition->lhs->type != bir::TypeKind::Ptr ||
      inputs.branch_condition->rhs->type != bir::TypeKind::Ptr) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::UnsupportedCompareType;
    return publication;
  }
  if (inputs.terminator->condition.kind != bir::Value::Kind::Named ||
      inputs.branch_condition->condition_value.kind != bir::Value::Kind::Named ||
      inputs.terminator->condition.name !=
          inputs.branch_condition->condition_value.name) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::ConditionMismatch;
    return publication;
  }
  if (inputs.branch_condition->true_label == kInvalidBlockLabel ||
      inputs.branch_condition->false_label == kInvalidBlockLabel ||
      prepared_block_label(*inputs.names, inputs.branch_condition->true_label) !=
          inputs.terminator->true_label ||
      prepared_block_label(*inputs.names, inputs.branch_condition->false_label) !=
          inputs.terminator->false_label) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::TargetMismatch;
    return publication;
  }
  if (inputs.condition_home == nullptr) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::MissingConditionHome;
    return publication;
  }
  if (inputs.terminator->condition.type != bir::TypeKind::I32 ||
      !prepared_home_names_value(*inputs.names,
                                 *inputs.condition_home,
                                 inputs.terminator->condition)) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::HomeValueMismatch;
    return publication;
  }
  if (!prepared_home_has_gpr_compatible_register(*inputs.condition_home)) {
    publication.status =
        PreparedFusedPointerBranchPublicationStatus::UnsupportedConditionHome;
    return publication;
  }

  const auto lhs_status = validate_prepared_pointer_branch_operand_home(
      *inputs.names,
      *inputs.branch_condition->lhs,
      inputs.lhs_home,
      PreparedFusedPointerBranchPublicationStatus::MissingLhsHome);
  if (lhs_status != PreparedFusedPointerBranchPublicationStatus::Available) {
    publication.status = lhs_status;
    return publication;
  }
  const auto rhs_status = validate_prepared_pointer_branch_operand_home(
      *inputs.names,
      *inputs.branch_condition->rhs,
      inputs.rhs_home,
      PreparedFusedPointerBranchPublicationStatus::MissingRhsHome);
  if (rhs_status != PreparedFusedPointerBranchPublicationStatus::Available) {
    publication.status = rhs_status;
    return publication;
  }

  publication.status =
      PreparedFusedPointerBranchPublicationStatus::Available;
  return publication;
}

bool prepared_fused_pointer_branch_publication_available(
    const PreparedFusedPointerBranchPublication& publication) {
  return publication.status ==
         PreparedFusedPointerBranchPublicationStatus::Available;
}

namespace {

[[nodiscard]] const bir::Value* dependency_operand_for_role(
    const bir::BinaryInst& binary,
    PreparedDependencyOperandRole role) {
  switch (role) {
    case PreparedDependencyOperandRole::Lhs:
      return &binary.lhs;
    case PreparedDependencyOperandRole::Rhs:
      return &binary.rhs;
  }
  return nullptr;
}

[[nodiscard]] bool prepared_stack_object_matches_dependency_home(
    const PreparedStackObject& object,
    const PreparedValueHome& home,
    bir::TypeKind dependency_type) {
  return home.kind == PreparedValueHomeKind::StackSlot &&
         home.function_name == object.function_name &&
         object.value_name.has_value() &&
         *object.value_name == home.value_name &&
         home.size_bytes.has_value() &&
         object.size_bytes == *home.size_bytes &&
         home.align_bytes.has_value() &&
         object.align_bytes == *home.align_bytes &&
         object.type == dependency_type &&
         object.type == bir::TypeKind::Ptr &&
         object.size_bytes == 8 &&
         object.align_bytes >= 8 &&
         !object.address_exposed &&
         !object.permanent_home_slot;
}

[[nodiscard]] bool prepared_dependency_cast_source_home_is_consumable(
    const PreparedValueHome& home) {
  if (home.kind == PreparedValueHomeKind::Register &&
      home.register_name.has_value()) {
    return true;
  }
  return home.kind == PreparedValueHomeKind::RematerializableImmediate &&
         home.immediate_i32.has_value();
}

[[nodiscard]] bool prepared_inttoptr_cast_width_supported(
    const bir::CastInst& cast) {
  return cast.opcode == bir::CastOpcode::IntToPtr &&
         cast.result.type == bir::TypeKind::Ptr &&
         (cast.operand.type == bir::TypeKind::I32 ||
          cast.operand.type == bir::TypeKind::I64);
}

}  // namespace

PreparedDependencyOperandAuthority plan_prepared_dependency_operand_authority(
    const PreparedDependencyOperandAuthorityInputs& inputs) {
  PreparedDependencyOperandAuthority authority{
      .policy = inputs.policy,
      .operand_role = inputs.operand_role,
      .publication = inputs.publication,
      .dependency_home = inputs.dependency_home,
      .dependency_stack_object = inputs.dependency_stack_object,
      .cast_producer = inputs.cast_producer,
      .cast_source_home = inputs.cast_source_home,
  };

  if (inputs.names == nullptr) {
    authority.status = PreparedDependencyOperandAuthorityStatus::MissingNames;
    return authority;
  }
  if (inputs.publication == nullptr) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::MissingPublication;
    return authority;
  }
  authority.destination_value_id = inputs.publication->destination_value_id;
  authority.destination_value_name = inputs.publication->destination_value_name;
  authority.edge_source_value_id = inputs.publication->source_value_id;
  authority.edge_source_value_name = inputs.publication->source_value_name;

  if (inputs.publication->status !=
          PreparedEdgePublicationLookupStatus::Available ||
      inputs.publication->phase != PreparedMovePhase::BlockEntry ||
      inputs.publication->parallel_copy_execution_site !=
          PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      inputs.publication->destination_storage_kind !=
          PreparedMoveStorageKind::Register) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::UnsupportedPublication;
    return authority;
  }
  if (inputs.publication->source_producer_kind !=
      PreparedEdgePublicationSourceProducerKind::Binary) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::UnsupportedSourceProducer;
    return authority;
  }
  if (inputs.publication->source_binary == nullptr) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::MissingSourceProducer;
    return authority;
  }
  if (inputs.publication->source_binary->result !=
      inputs.publication->source_value) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::UnsupportedSourceProducer;
    return authority;
  }

  if (inputs.dependency_operand == nullptr) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::MissingDependencyOperand;
    return authority;
  }
  const auto* expected_operand =
      dependency_operand_for_role(*inputs.publication->source_binary,
                                  inputs.operand_role);
  if (expected_operand == nullptr || *expected_operand != *inputs.dependency_operand) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::OperandMismatch;
    return authority;
  }
  authority.dependency_type = inputs.dependency_operand->type;

  if (inputs.dependency_home == nullptr) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::MissingDependencyHome;
    return authority;
  }
  authority.dependency_value_id = inputs.dependency_home->value_id;
  authority.dependency_value_name = inputs.dependency_home->value_name;
  if (!prepared_home_names_value(*inputs.names, *inputs.dependency_home,
                                 *inputs.dependency_operand)) {
    authority.status =
        PreparedDependencyOperandAuthorityStatus::HomeValueMismatch;
    return authority;
  }

  if (inputs.dependency_home->kind == PreparedValueHomeKind::StackSlot) {
    authority.dependency_slot_id = inputs.dependency_home->slot_id;
    authority.dependency_stack_offset_bytes =
        inputs.dependency_home->offset_bytes;
    authority.dependency_stack_size_bytes = inputs.dependency_home->size_bytes;
    authority.dependency_stack_align_bytes = inputs.dependency_home->align_bytes;
    if (inputs.dependency_stack_object == nullptr) {
      authority.status =
          PreparedDependencyOperandAuthorityStatus::MissingStackObject;
      return authority;
    }
    if (!prepared_stack_object_matches_dependency_home(
            *inputs.dependency_stack_object, *inputs.dependency_home,
            inputs.dependency_operand->type)) {
      authority.status =
          PreparedDependencyOperandAuthorityStatus::StackObjectMismatch;
      return authority;
    }
  }

  switch (inputs.policy) {
    case PreparedDependencyOperandMaterializationPolicy::None:
      authority.status =
          PreparedDependencyOperandAuthorityStatus::MissingPolicy;
      return authority;
    case PreparedDependencyOperandMaterializationPolicy::LoadFromStackSlot:
      if (inputs.dependency_home->kind != PreparedValueHomeKind::StackSlot) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::StackObjectMismatch;
        return authority;
      }
      if (!inputs.stack_slot_fresh_at_edge) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::MissingStackFreshness;
        return authority;
      }
      if (!inputs.stack_slot_clobber_safe_at_edge) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::MissingStackClobberSafety;
        return authority;
      }
      break;
    case PreparedDependencyOperandMaterializationPolicy::
        RematerializeCastFromSource:
      if (inputs.cast_producer == nullptr) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::MissingCastProducer;
        return authority;
      }
      if (inputs.cast_producer->kind !=
              PreparedEdgePublicationSourceProducerKind::Cast ||
          inputs.cast_producer->cast == nullptr) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::UnsupportedCastProducer;
        return authority;
      }
      if (inputs.cast_producer->cast->result != *inputs.dependency_operand) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::CastResultMismatch;
        return authority;
      }
      if (!prepared_inttoptr_cast_width_supported(*inputs.cast_producer->cast)) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::UnsupportedCastWidth;
        return authority;
      }
      if (inputs.cast_source_home == nullptr) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::MissingCastSourceHome;
        return authority;
      }
      authority.cast_source_value_id = inputs.cast_source_home->value_id;
      authority.cast_source_value_name = inputs.cast_source_home->value_name;
      authority.cast_source_type = inputs.cast_producer->cast->operand.type;
      if (!prepared_home_names_value(*inputs.names, *inputs.cast_source_home,
                                     inputs.cast_producer->cast->operand)) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::CastSourceMismatch;
        return authority;
      }
      if (!prepared_dependency_cast_source_home_is_consumable(
              *inputs.cast_source_home)) {
        authority.status =
            PreparedDependencyOperandAuthorityStatus::UnsupportedCastSourceHome;
        return authority;
      }
      break;
  }

  authority.status = PreparedDependencyOperandAuthorityStatus::Available;
  return authority;
}

bool prepared_dependency_operand_authority_available(
    const PreparedDependencyOperandAuthority& authority) {
  return authority.status == PreparedDependencyOperandAuthorityStatus::Available;
}

namespace {

[[nodiscard]] const PreparedStackObject* dependency_stack_object_for_home(
    const PreparedStackLayout& stack_layout,
    const PreparedValueHome* home) {
  if (home == nullptr || home->kind != PreparedValueHomeKind::StackSlot ||
      !home->slot_id.has_value()) {
    return nullptr;
  }
  const auto* slot = find_frame_slot_by_id(stack_layout, *home->slot_id);
  return prepared_stack_object_for_frame_slot(stack_layout, slot);
}

[[nodiscard]] const PreparedValueHome* value_home_for_named_value(
    const PreparedNameTables& names,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups,
    const bir::Value& value) {
  const auto value_name = existing_prepared_value_name_id(names, value);
  if (!value_name.has_value()) {
    return nullptr;
  }
  const auto value_id =
      find_indexed_prepared_value_id(value_home_lookups,
                                     nullptr,
                                     value_locations,
                                     *value_name);
  if (!value_id.has_value()) {
    return nullptr;
  }
  return find_indexed_prepared_value_home(value_home_lookups,
                                          value_locations,
                                          *value_id);
}

void clear_temporary_authority_pointers(
    PreparedDependencyOperandAuthority& authority) {
  authority.publication = nullptr;
  authority.dependency_home = nullptr;
  authority.dependency_stack_object = nullptr;
  authority.cast_producer = nullptr;
  authority.cast_source_home = nullptr;
}

PreparedDependencyOperandAuthorityRecord make_dependency_operand_authority_record(
    FunctionNameId function_name,
    const PreparedEdgePublication& publication,
    PreparedDependencyOperandRole role,
    const bir::Value& dependency_operand,
    const PreparedValueHome* dependency_home,
    const PreparedStackObject* dependency_stack_object,
    const PreparedEdgePublicationSourceProducer* cast_producer,
    const PreparedValueHome* cast_source_home,
    PreparedDependencyOperandMaterializationPolicy policy,
    const PreparedNameTables& names) {
  PreparedDependencyOperandAuthorityRecord record{
      .function_name = function_name,
      .predecessor_label = publication.predecessor_label,
      .successor_label = publication.successor_label,
      .source_producer_kind = publication.source_producer_kind,
      .source_producer_block_label = publication.source_producer_block_label,
      .source_producer_instruction_index =
          publication.source_producer_instruction_index,
      .cast_producer_block_label =
          cast_producer != nullptr && cast_producer->block_label != kInvalidBlockLabel
              ? std::optional<BlockLabelId>{cast_producer->block_label}
              : std::nullopt,
      .cast_producer_instruction_index =
          cast_producer != nullptr
              ? std::optional<std::size_t>{cast_producer->instruction_index}
              : std::nullopt,
      .cast_source_home_kind =
          cast_source_home != nullptr ? cast_source_home->kind
                                      : PreparedValueHomeKind::None,
      .cast_source_register_name =
          cast_source_home != nullptr ? cast_source_home->register_name
                                      : std::nullopt,
      .cast_source_immediate_i32 =
          cast_source_home != nullptr ? cast_source_home->immediate_i32
                                      : std::nullopt,
      .authority = plan_prepared_dependency_operand_authority({
          .names = &names,
          .publication = &publication,
          .dependency_operand = &dependency_operand,
          .operand_role = role,
          .dependency_home = dependency_home,
          .dependency_stack_object = dependency_stack_object,
          .cast_producer = cast_producer,
          .cast_source_home = cast_source_home,
          .policy = policy,
      }),
  };
  clear_temporary_authority_pointers(record.authority);
  return record;
}

void collect_dependency_operand_authorities_for_role(
    PreparedDependencyOperandAuthorityRecords& records,
    const PreparedBirModule& prepared,
    FunctionNameId function_name,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups,
    const PreparedEdgePublicationSourceProducerLookups& source_producers,
    const PreparedEdgePublication& publication,
    PreparedDependencyOperandRole role) {
  if (publication.source_binary == nullptr) {
    return;
  }
  const auto* dependency_operand =
      dependency_operand_for_role(*publication.source_binary, role);
  if (dependency_operand == nullptr ||
      dependency_operand->kind != bir::Value::Kind::Named ||
      dependency_operand->type != bir::TypeKind::Ptr) {
    return;
  }
  const auto dependency_name =
      existing_prepared_value_name_id(prepared.names, *dependency_operand);
  if (!dependency_name.has_value()) {
    return;
  }
  const auto* dependency_home =
      value_home_for_named_value(prepared.names,
                                 value_locations,
                                 value_home_lookups,
                                 *dependency_operand);
  if (dependency_home == nullptr ||
      dependency_home->kind != PreparedValueHomeKind::StackSlot) {
    return;
  }
  const auto* dependency_stack_object =
      dependency_stack_object_for_home(prepared.stack_layout, dependency_home);
  const auto* cast_producer =
      find_indexed_prepared_edge_publication_source_producer(&source_producers,
                                                             *dependency_name);
  const PreparedValueHome* cast_source_home = nullptr;
  if (cast_producer != nullptr && cast_producer->cast != nullptr) {
    cast_source_home =
        value_home_for_named_value(prepared.names,
                                   value_locations,
                                   value_home_lookups,
                                   cast_producer->cast->operand);
  }

  records.records.push_back(make_dependency_operand_authority_record(
      function_name,
      publication,
      role,
      *dependency_operand,
      dependency_home,
      dependency_stack_object,
      cast_producer,
      cast_source_home,
      PreparedDependencyOperandMaterializationPolicy::RematerializeCastFromSource,
      prepared.names));
  records.records.push_back(make_dependency_operand_authority_record(
      function_name,
      publication,
      role,
      *dependency_operand,
      dependency_home,
      dependency_stack_object,
      nullptr,
      nullptr,
      PreparedDependencyOperandMaterializationPolicy::LoadFromStackSlot,
      prepared.names));
}

}  // namespace

PreparedDependencyOperandAuthorityRecords
collect_prepared_dependency_operand_authorities(
    const PreparedBirModule& prepared) {
  PreparedDependencyOperandAuthorityRecords records;
  for (const auto& function : prepared.control_flow.functions) {
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function.function_name);
    const auto value_home_lookups =
        make_prepared_value_home_lookups(value_locations);
    const auto edge_publications = make_prepared_edge_publication_lookups(
        prepared, function, value_locations, &value_home_lookups);
    const auto source_producers =
        make_prepared_edge_publication_source_producer_lookups(prepared,
                                                               function);
    for (const auto& publication : edge_publications.publications) {
      if (publication.status != PreparedEdgePublicationLookupStatus::Available ||
          publication.source_producer_kind !=
              PreparedEdgePublicationSourceProducerKind::Binary ||
          publication.source_binary == nullptr) {
        continue;
      }
      collect_dependency_operand_authorities_for_role(
          records,
          prepared,
          function.function_name,
          value_locations,
          &value_home_lookups,
          source_producers,
          publication,
          PreparedDependencyOperandRole::Lhs);
      collect_dependency_operand_authorities_for_role(
          records,
          prepared,
          function.function_name,
          value_locations,
          &value_home_lookups,
          source_producers,
          publication,
          PreparedDependencyOperandRole::Rhs);
    }
  }
  return records;
}

PreparedSelectEdgeSourceProducerPlacement
plan_prepared_select_edge_source_producer_placement(
    const PreparedSelectEdgeSourceProducerPlacementInputs& inputs) {
  PreparedSelectEdgeSourceProducerPlacement placement{
      .placement_kind = inputs.placement_kind,
      .publication = inputs.publication,
      .move_bundle = inputs.move_bundle,
      .move_bundle_block_label = inputs.move_bundle_block_label,
  };
  if (inputs.publication == nullptr) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::MissingPublication;
    return placement;
  }
  placement.predecessor_label = inputs.publication->predecessor_label;
  placement.successor_label = inputs.publication->successor_label;
  placement.destination_value_id = inputs.publication->destination_value_id;
  placement.destination_value_name = inputs.publication->destination_value_name;
  placement.source_value_id = inputs.publication->source_value_id;
  placement.source_value_name = inputs.publication->source_value_name;
  placement.source_producer_block_label =
      inputs.publication->source_producer_block_label;
  placement.source_producer_instruction_index =
      inputs.publication->source_producer_instruction_index;

  if (inputs.move_bundle == nullptr) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::MissingMoveBundle;
    return placement;
  }
  placement.move_bundle_instruction_index = inputs.move_bundle->instruction_index;
  placement.move_count = inputs.move_bundle->moves.size();

  if (inputs.placement_kind !=
      PreparedSelectEdgeSourceProducerPlacementKind::
          PredecessorEdgeConsumedSuppression) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedPlacementKind;
    return placement;
  }
  if (inputs.publication->status !=
          PreparedEdgePublicationLookupStatus::Available ||
      inputs.publication->phase != PreparedMovePhase::BlockEntry ||
      inputs.publication->parallel_copy_execution_site !=
          PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      inputs.publication->carrier_kind !=
          PreparedJoinTransferCarrierKind::SelectMaterialization) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedPublication;
    return placement;
  }
  if (inputs.publication->source_producer_kind !=
      PreparedEdgePublicationSourceProducerKind::Binary) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedSourceProducer;
    return placement;
  }
  if (inputs.publication->source_binary == nullptr ||
      !inputs.publication->source_value_id.has_value()) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::MissingSourceProducer;
    return placement;
  }
  if (inputs.publication->source_binary->result !=
      inputs.publication->source_value) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedSourceProducer;
    return placement;
  }
  if (!inputs.publication->source_producer_block_label.has_value() ||
      !inputs.publication->source_producer_instruction_index.has_value() ||
      !inputs.move_bundle_block_label.has_value()) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::MissingProducerSite;
    return placement;
  }
  if (*inputs.publication->source_producer_block_label !=
          *inputs.move_bundle_block_label ||
      *inputs.publication->source_producer_instruction_index !=
          inputs.move_bundle->instruction_index) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::
            MoveBundleProducerMismatch;
    return placement;
  }
  if (inputs.move_bundle->phase != PreparedMovePhase::BeforeInstruction ||
      inputs.move_bundle->authority_kind != PreparedMoveAuthorityKind::None) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedMoveBundle;
    return placement;
  }
  if (inputs.move_bundle->moves.empty()) {
    placement.status =
        PreparedSelectEdgeSourceProducerPlacementStatus::MissingMove;
    return placement;
  }

  for (const auto& move : inputs.move_bundle->moves) {
    if (move.destination_kind != PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != PreparedMoveStorageKind::Register ||
        move.op_kind != PreparedMoveResolutionOpKind::Move ||
        move.authority_kind != PreparedMoveAuthorityKind::None ||
        move.block_index != inputs.move_bundle->block_index ||
        move.instruction_index != inputs.move_bundle->instruction_index) {
      placement.status =
          PreparedSelectEdgeSourceProducerPlacementStatus::UnsupportedMove;
      return placement;
    }
    if (move.to_value_id != *inputs.publication->source_value_id) {
      placement.status =
          PreparedSelectEdgeSourceProducerPlacementStatus::
              MoveDestinationMismatch;
      return placement;
    }
  }

  placement.status =
      PreparedSelectEdgeSourceProducerPlacementStatus::Available;
  return placement;
}

bool prepared_select_edge_source_producer_placement_available(
    const PreparedSelectEdgeSourceProducerPlacement& placement) {
  return placement.status ==
         PreparedSelectEdgeSourceProducerPlacementStatus::Available;
}

bool prepared_select_edge_source_producer_placement_matches_move_bundle(
    const PreparedSelectEdgeSourceProducerPlacementRecord& record,
    FunctionNameId function_name,
    std::optional<BlockLabelId> move_bundle_block_label,
    const PreparedMoveBundle& move_bundle) {
  if (record.function_name != function_name ||
      move_bundle.function_name != function_name ||
      !prepared_select_edge_source_producer_placement_available(
          record.placement) ||
      record.placement.placement_kind !=
          PreparedSelectEdgeSourceProducerPlacementKind::
              PredecessorEdgeConsumedSuppression ||
      record.placement.move_bundle_instruction_index !=
          move_bundle.instruction_index ||
      record.placement.move_count != move_bundle.moves.size() ||
      !record.placement.source_value_id.has_value() ||
      move_bundle.phase != PreparedMovePhase::BeforeInstruction ||
      move_bundle.authority_kind != PreparedMoveAuthorityKind::None ||
      move_bundle.moves.empty()) {
    return false;
  }
  if (!record.placement.move_bundle_block_label.has_value() ||
      !move_bundle_block_label.has_value() ||
      *record.placement.move_bundle_block_label != *move_bundle_block_label) {
    return false;
  }
  for (const auto& move : move_bundle.moves) {
    if (move.destination_kind != PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != PreparedMoveStorageKind::Register ||
        move.op_kind != PreparedMoveResolutionOpKind::Move ||
        move.authority_kind != PreparedMoveAuthorityKind::None ||
        move.block_index != move_bundle.block_index ||
        move.instruction_index != move_bundle.instruction_index ||
        move.to_value_id != *record.placement.source_value_id) {
      return false;
    }
  }
  return true;
}

namespace {

void clear_temporary_placement_pointers(
    PreparedSelectEdgeSourceProducerPlacement& placement) {
  placement.publication = nullptr;
  placement.move_bundle = nullptr;
}

[[nodiscard]] std::optional<BlockLabelId> prepared_block_label_for_bundle(
    const PreparedControlFlowFunction& function,
    const PreparedMoveBundle& bundle) {
  if (bundle.block_index >= function.blocks.size()) {
    return std::nullopt;
  }
  const auto block_label = function.blocks[bundle.block_index].block_label;
  if (block_label == kInvalidBlockLabel) {
    return std::nullopt;
  }
  return block_label;
}

}  // namespace

PreparedSelectEdgeSourceProducerPlacementRecords
collect_prepared_select_edge_source_producer_placements(
    const PreparedBirModule& prepared) {
  PreparedSelectEdgeSourceProducerPlacementRecords records;
  for (const auto& function : prepared.control_flow.functions) {
    const auto* value_locations =
        find_prepared_value_location_function(prepared, function.function_name);
    if (value_locations == nullptr) {
      continue;
    }
    const auto value_home_lookups =
        make_prepared_value_home_lookups(value_locations);
    const auto edge_publications = make_prepared_edge_publication_lookups(
        prepared, function, value_locations, &value_home_lookups);
    for (const auto& bundle : value_locations->move_bundles) {
      const auto bundle_block_label =
          prepared_block_label_for_bundle(function, bundle);
      for (const auto& publication : edge_publications.publications) {
        auto placement = plan_prepared_select_edge_source_producer_placement({
            .publication = &publication,
            .move_bundle = &bundle,
            .placement_kind =
                PreparedSelectEdgeSourceProducerPlacementKind::
                    PredecessorEdgeConsumedSuppression,
            .move_bundle_block_label = bundle_block_label,
        });
        if (!prepared_select_edge_source_producer_placement_available(
                placement)) {
          continue;
        }
        clear_temporary_placement_pointers(placement);
        records.records.push_back(PreparedSelectEdgeSourceProducerPlacementRecord{
            .function_name = function.function_name,
            .placement = placement,
        });
      }
    }
  }
  return records;
}

void publish_store_global_immediate_source_if_authorized(
    PreparedStoreSourcePublicationPlan& plan) {
  if (plan.intent != PreparedStoreSourcePublicationIntent::StoreGlobalPublication ||
      plan.source_value.kind != bir::Value::Kind::Immediate ||
      plan.destination_access == nullptr ||
      !prepared_global_symbol_memory_has_publication_authority(
          plan.destination_access->address)) {
    return;
  }
  plan.source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Immediate;
}

PreparedStoreSourcePublicationPlan plan_prepared_store_source_publication(
    const PreparedStoreSourcePublicationInputs& inputs) {
  PreparedStoreSourcePublicationPlan plan{
      .intent = inputs.intent,
      .destination_access = inputs.destination_access,
      .pending_publication = inputs.pending_publication,
      .stack_homes_only = inputs.stack_homes_only,
      .pointer_store_writeback = inputs.pointer_store_writeback,
      .duplicate_publication = inputs.duplicate_publication,
  };

  if (inputs.source_value == nullptr) {
    plan.status = PreparedStoreSourcePublicationStatus::MissingSourceValue;
    return plan;
  }
  plan.source_value = *inputs.source_value;

  if (inputs.destination_access == nullptr) {
    plan.status = PreparedStoreSourcePublicationStatus::MissingDestinationAccess;
    return plan;
  }

  const PreparedMemoryAccess& access = *inputs.destination_access;
  plan.status = PreparedStoreSourcePublicationStatus::Available;
  plan.destination_base_kind = access.address.base_kind;
  plan.destination_frame_slot_id = access.address.frame_slot_id;
  plan.destination_pointer_value_name = access.address.pointer_value_name;
  plan.destination_byte_offset = access.address.byte_offset;
  plan.destination_size_bytes = access.address.size_bytes;
  plan.destination_align_bytes = access.address.align_bytes;
  plan.destination_can_use_base_plus_offset = access.address.can_use_base_plus_offset;
  plan.destination_is_volatile = access.is_volatile;

  plan.destination_frame_slot = inputs.destination_frame_slot;
  if (inputs.destination_frame_slot != nullptr) {
    plan.destination_frame_slot_id = inputs.destination_frame_slot->slot_id;
    plan.destination_object_id = inputs.destination_frame_slot->object_id;
    plan.destination_stack_offset_bytes = inputs.destination_frame_slot->offset_bytes;
    plan.destination_stack_size_bytes = inputs.destination_frame_slot->size_bytes;
    plan.destination_stack_align_bytes = inputs.destination_frame_slot->align_bytes;
  }
  plan.destination_stack_object = inputs.destination_stack_object;
  if (!plan.destination_object_id.has_value() &&
      inputs.destination_stack_object != nullptr) {
    plan.destination_object_id = inputs.destination_stack_object->object_id;
  }

  plan.source_home = inputs.source_home;
  if (inputs.source_home != nullptr) {
    const PreparedValueHome& home = *inputs.source_home;
    plan.source_value_id = home.value_id;
    plan.source_value_name = home.value_name;
    plan.source_home_kind = home.kind;
    plan.source_storage_encoding =
        prepared_storage_encoding_from_value_home_kind(home.kind);
    plan.source_slot_id = home.slot_id;
    plan.source_stack_offset_bytes = home.offset_bytes;
    plan.source_size_bytes = home.size_bytes;
    plan.source_align_bytes = home.align_bytes;
    plan.source_pointer_base_value_name = home.pointer_base_value_name;
    plan.source_pointer_base_symbol_name = home.pointer_base_symbol_name;
    plan.source_pointer_byte_delta = home.pointer_byte_delta;
  } else if (access.stored_value_name.has_value()) {
    plan.source_value_name = *access.stored_value_name;
  }

  if (inputs.recovered_source_value != nullptr) {
    plan.recovered_source_value = *inputs.recovered_source_value;
  }
  plan.recovered_source_instruction_index =
      inputs.recovered_source_instruction_index;
  plan.byval_load_local_source = inputs.byval_load_local_source;
  plan.direct_global_select_chain_source =
      inputs.direct_global_select_chain_source;
  plan.direct_global_select_chain_root_is_select =
      inputs.direct_global_select_chain_root_is_select;
  plan.direct_global_select_chain_root_instruction_index =
      inputs.direct_global_select_chain_root_instruction_index;

  if (prepared_store_source_producer_metadata_agrees(inputs)) {
    plan.source_producer_kind = inputs.source_producer->kind;
    if (inputs.source_producer->block_label != kInvalidBlockLabel) {
      plan.source_producer_block_label = inputs.source_producer->block_label;
    }
    plan.source_producer_instruction_index =
        inputs.source_producer->instruction_index;
    plan.source_load_local = inputs.source_producer->load_local;
    plan.source_load_global = inputs.source_producer->load_global;
    plan.source_cast = inputs.source_producer->cast;
    plan.source_binary = inputs.source_producer->binary;
    plan.source_select = inputs.source_producer->select;
  }

  plan.pointer_base_home = inputs.pointer_base_home;
  if (inputs.pointer_base_home != nullptr) {
    plan.pointer_base_home_kind = inputs.pointer_base_home->kind;
    plan.pointer_base_slot_id = inputs.pointer_base_home->slot_id;
    plan.pointer_base_stack_offset_bytes = inputs.pointer_base_home->offset_bytes;
  }

  return plan;
}

PreparedStoreSourcePublicationPlan plan_prepared_store_global_publication(
    const PreparedValueLocationFunction* value_locations,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::StoreGlobalInst& store,
    std::size_t instruction_index,
    bool pending_publication,
    bool stack_homes_only) {
  const auto* access =
      find_publication_memory_access(addressing, block_label, instruction_index);
  const auto* source_home =
      find_publication_source_home(value_locations,
                                   access != nullptr ? access->stored_value_name
                                                     : std::nullopt);
  const bool duplicate_publication =
      !pending_publication && source_home != nullptr &&
      source_home->kind == PreparedValueHomeKind::StackSlot;
  auto plan = plan_prepared_store_source_publication({
      .source_value = &store.value,
      .destination_access = access,
      .source_home = source_home,
      .intent = PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .pending_publication = pending_publication,
      .stack_homes_only = stack_homes_only,
      .duplicate_publication = duplicate_publication,
  });
  publish_store_global_immediate_source_if_authorized(plan);
  return plan;
}

std::vector<PreparedStoreGlobalPublicationCandidate>
plan_pending_prepared_store_global_publications(
    const PreparedValueLocationFunction* value_locations,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Block* block,
    std::size_t instruction_index,
    const PreparedEdgePublicationSourceProducerLookups* source_producers) {
  std::vector<PreparedStoreGlobalPublicationCandidate> candidates;
  if (source_producers == nullptr ||
      block == nullptr ||
      prepared_store_global_publication_run_has_prior_store(block, instruction_index)) {
    return candidates;
  }
  std::unordered_set<ValueNameId> published_source_names;
  for (std::size_t index = instruction_index; index < block->insts.size(); ++index) {
    const auto& candidate = block->insts[index];
    if (!is_prepared_store_global_publication_run_instruction(candidate)) {
      break;
    }
    const auto* store = std::get_if<bir::StoreGlobalInst>(&candidate);
    if (store == nullptr) {
      continue;
    }
    auto plan = plan_prepared_store_global_publication(value_locations,
                                                       addressing,
                                                       block_label,
                                                       *store,
                                                       index,
                                                       true,
                                                       true);
    if (!prepared_store_source_publication_available(plan)) {
      continue;
    }
    const auto* source_producer =
        find_pending_store_global_source_producer(
            source_producers, block_label, block, plan, *store, index);
    if (source_producer == nullptr) {
      continue;
    }
    plan = plan_prepared_store_source_publication({
        .source_value = &store->value,
        .destination_access = plan.destination_access,
        .source_home = plan.source_home,
        .intent = PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
        .pending_publication = true,
        .stack_homes_only = true,
        .duplicate_publication = plan.duplicate_publication,
        .source_producer = source_producer,
    });
    if (plan.source_value_name != kInvalidValueName) {
      const auto [_, inserted] = published_source_names.insert(plan.source_value_name);
      if (!inserted) {
        plan.duplicate_publication = true;
      }
    }
    candidates.push_back(PreparedStoreGlobalPublicationCandidate{
        .instruction_index = index,
        .store_source = plan,
    });
  }
  return candidates;
}

void populate_store_source_publication_plans(PreparedBirModule& prepared) {
  prepared.store_source_publications.records.clear();
  prepared.call_argument_value_publications.facts.clear();

  for (const auto& function : prepared.module.functions) {
    const FunctionNameId function_name = prepared.names.function_names.find(function.name);
    if (function_name == kInvalidFunctionName) {
      continue;
    }

    const auto* value_locations =
        find_prepared_value_location_function(prepared, function_name);
    const auto* addressing = find_prepared_addressing(prepared, function_name);
    const auto* function_cf =
        find_control_flow_function(prepared.control_flow, function_name);
    std::optional<PreparedEdgePublicationSourceProducerLookups>
        source_producer_lookups;
    const PreparedEdgePublicationSourceProducerLookups* source_producers = nullptr;
    if (function_cf != nullptr) {
      source_producer_lookups =
          make_prepared_edge_publication_source_producer_lookups(prepared, *function_cf);
      source_producers = &*source_producer_lookups;
    }

    for (const auto& block : function.blocks) {
      const BlockLabelId block_label = prepared_block_label_id(prepared.names, block);
      if (block_label == kInvalidBlockLabel) {
        continue;
      }

      for (std::size_t inst_index = 0; inst_index < block.insts.size(); ++inst_index) {
        const auto* store_local =
            std::get_if<bir::StoreLocalInst>(&block.insts[inst_index]);
        const auto* store_global =
            std::get_if<bir::StoreGlobalInst>(&block.insts[inst_index]);
        if (store_local == nullptr && store_global == nullptr) {
          continue;
        }

        const bir::Value& source_value =
            store_local != nullptr ? store_local->value : store_global->value;
        const auto* access =
            find_store_source_publication_access(
                prepared.names, addressing, block_label, source_value, inst_index);
        const auto* source_home =
            find_publication_source_home(value_locations,
                                         access != nullptr ? access->stored_value_name
                                                           : std::nullopt);
        const auto* source_producer =
            find_prepared_select_chain_source_producer(prepared.names,
                                                       source_producers,
                                                       block_label,
                                                       &block,
                                                       source_value,
                                                       inst_index);
        const auto direct_global_select_chain =
            find_prepared_store_source_direct_global_select_chain_dependency(
                prepared.names,
                source_producers,
                block_label,
                &block,
                source_value,
                inst_index);
        const auto* destination_slot =
            prepared_frame_slot_for_access(prepared.stack_layout, access);
        const auto* destination_object =
            prepared_stack_object_for_frame_slot(prepared.stack_layout, destination_slot);

        std::optional<PreparedRecoveredStoreSourcePublication> recovered_source;
        bool byval_load_local_source = false;
        if (store_local != nullptr) {
          if (source_producer != nullptr && source_producer->load_local != nullptr) {
            recovered_source =
                find_prepared_recovered_narrow_store_source_for_wide_local_load(
                    prepared.names,
                    prepared.module.names,
                    prepared.stack_layout,
                    addressing,
                    block_label,
                    &block,
                    *source_producer->load_local,
                    source_producer->instruction_index);
          }
          byval_load_local_source =
              prepared_store_source_load_local_is_byval_formal_pointer_source(
                  prepared.names, &function, addressing, source_producer);
        }

        const bool duplicate_publication =
            store_global != nullptr && source_home != nullptr &&
            source_home->kind == PreparedValueHomeKind::StackSlot;
        const bir::Value* recovered_value =
            recovered_source.has_value() ? &recovered_source->stored_value : nullptr;
        auto plan = plan_prepared_store_source_publication({
            .source_value = &source_value,
            .destination_access = access,
            .source_home = source_home,
            .destination_frame_slot = destination_slot,
            .destination_stack_object = destination_object,
            .recovered_source_value = recovered_value,
            .recovered_source_instruction_index =
                recovered_source.has_value()
                    ? std::optional<std::size_t>{recovered_source->instruction_index}
                    : std::nullopt,
            .byval_load_local_source = byval_load_local_source,
            .direct_global_select_chain_source =
                direct_global_select_chain.contains_direct_global_load,
            .direct_global_select_chain_root_is_select =
                direct_global_select_chain.root_is_select,
            .direct_global_select_chain_root_instruction_index =
                direct_global_select_chain.root_instruction_index,
            .intent =
                store_local != nullptr
                    ? PreparedStoreSourcePublicationIntent::StoreLocalPublication
                    : PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
            .stack_homes_only = store_global != nullptr,
            .duplicate_publication = duplicate_publication,
            .source_producer = source_producer,
            .publication_instruction_index = inst_index,
        });
        if (store_global != nullptr) {
          publish_store_global_immediate_source_if_authorized(plan);
        }
        append_store_source_record(prepared.store_source_publications,
                                   function_name,
                                   block_label,
                                   inst_index,
                                   std::move(plan));
      }
    }
  }
  populate_call_argument_value_publication_plans(prepared);
}

PreparedFixedFormalStoreSourcePublication
plan_prepared_fixed_formal_store_source_publication(
    const PreparedFormalPublicationInputs& formal_inputs,
    const PreparedStoreSourcePublicationInputs& store_inputs) {
  PreparedFixedFormalStoreSourcePublication result{
      .store_source = plan_prepared_store_source_publication(store_inputs),
  };
  if (!prepared_store_source_publication_available(result.store_source) ||
      result.store_source.intent !=
          PreparedStoreSourcePublicationIntent::StoreLocalPublication ||
      result.store_source.source_home == nullptr ||
      result.store_source.source_value_name == kInvalidValueName) {
    return result;
  }

  const auto formal_plans = plan_prepared_formal_publications(formal_inputs);
  for (const auto& formal : formal_plans) {
    if (!prepared_formal_publication_available(formal) ||
        formal.value_name != result.store_source.source_value_name ||
        formal.value_id != std::optional<PreparedValueId>{
                               result.store_source.source_value_id} ||
        formal.home != result.store_source.source_home ||
        formal.type != result.store_source.source_value.type ||
        formal.formal == nullptr ||
        formal.formal->is_byval) {
      continue;
    }
    if (result.fixed_formal_source) {
      result.fixed_formal_source = false;
      result.formal_publication = PreparedFormalPublicationPlan{};
      return result;
    }
    result.formal_publication = formal;
    result.fixed_formal_source = true;
  }
  return result;
}

std::optional<PreparedRecoveredStoreSourcePublication>
find_prepared_recovered_narrow_store_source_for_wide_local_load(
    const PreparedNameTables& names,
    const bir::NameTables&,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index) {
  if (block == nullptr) {
    return std::nullopt;
  }
  const auto load_bits = integer_bit_width(load.result.type);
  if (!load_bits.has_value()) {
    return std::nullopt;
  }
  const auto* load_access =
      addressing != nullptr
          ? find_prepared_memory_access(
                *addressing, block_label, load_instruction_index)
          : nullptr;
  if (!prepared_load_access_matches_result(names, load_access, load) ||
      prepared_frame_slot_for_access(stack_layout, load_access) == nullptr) {
    return std::nullopt;
  }
  const auto load_lane_offset =
      load.result.kind == bir::Value::Kind::Named
          ? parse_trailing_dot_offset(load.result.name)
          : std::nullopt;
  for (std::size_t index = load_instruction_index; index > 0; --index) {
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&block->insts[index - 1]);
    const auto* store_access =
        store != nullptr && addressing != nullptr
            ? find_prepared_memory_access(*addressing, block_label, index - 1)
            : nullptr;
    if (store == nullptr ||
        !prepared_store_access_matches_value(names, store_access, *store) ||
        !prepared_store_access_targets_load_byte(
            stack_layout, load_access, store_access, load_lane_offset)) {
      continue;
    }
    const auto store_bits = integer_bit_width(store->value.type);
    if (!store_bits.has_value() || *store_bits != 8U || *store_bits >= *load_bits) {
      return std::nullopt;
    }
    if (!prepared_recovered_store_source_agrees_with_bir(
            load_access,
            store_access,
            *block,
            load,
            load_instruction_index,
            *store,
            index - 1U)) {
      return std::nullopt;
    }
    return PreparedRecoveredStoreSourcePublication{
        .stored_value = store->value,
        .instruction_index = index - 1,
    };
  }
  return std::nullopt;
}

bool prepared_store_source_load_local_is_byval_formal_pointer_source(
    const PreparedNameTables& names,
    const bir::Function* bir_function,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducer* source_producer) {
  if (addressing == nullptr || source_producer == nullptr ||
      source_producer->kind != PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      source_producer->load_local == nullptr ||
      source_producer->block_label == kInvalidBlockLabel) {
    return false;
  }
  const auto* access = find_prepared_memory_access(
      *addressing, source_producer->block_label, source_producer->instruction_index);
  if (access == nullptr ||
      access->address.base_kind != PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto load_result_name =
      existing_prepared_value_name_id(names, source_producer->load_local->result);
  return load_result_name.has_value() &&
         access->result_value_name == load_result_name &&
         is_byval_formal_value_name(
             names, bir_function, *access->address.pointer_value_name);
}

std::optional<PreparedScalarLoadLocalSourceProducer>
find_prepared_same_block_load_local_source_producer(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedMemoryAccessLookups* memory_accesses,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto* producer = find_prepared_select_chain_source_producer(
      names, source_producers, block_label, block, value, before_instruction_index);
  if (memory_accesses == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }

  const bir::LoadLocalInst* load_local = nullptr;
  const PreparedEdgePublicationSourceProducer* source_producer = nullptr;
  std::size_t producer_instruction_index = 0;
  BlockLabelId producer_block_label = block_label;
  const PreparedMemoryAccess* load_access = nullptr;
  if (producer != nullptr &&
      producer->kind == PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      producer->load_local != nullptr) {
    load_local = producer->load_local;
    source_producer = producer;
    producer_instruction_index = producer->instruction_index;
    producer_block_label = producer->block_label;
  } else {
    const auto value_name = resolve_prepared_value_name_id(names, value.name);
    if (!value_name.has_value()) {
      return std::nullopt;
    }
    load_access =
        find_unique_indexed_prepared_memory_access_by_result_value_name(
            memory_accesses, *value_name);
    if (load_access == nullptr ||
        load_access->block_label != block_label ||
        load_access->inst_index >= before_instruction_index ||
        load_access->inst_index >= block->insts.size()) {
      return std::nullopt;
    }
    load_local = std::get_if<bir::LoadLocalInst>(&block->insts[load_access->inst_index]);
    if (load_local == nullptr ||
        !prepared_load_access_matches_result(names, load_access, *load_local)) {
      return std::nullopt;
    }
    producer_instruction_index = load_access->inst_index;
    producer_block_label = load_access->block_label;
  }

  if (load_access == nullptr) {
    load_access =
        find_indexed_prepared_memory_access(memory_accesses,
                                            producer_block_label,
                                            producer_instruction_index);
    if ((load_access == nullptr ||
         !prepared_load_access_matches_result(names, load_access, *load_local)) &&
        load_local != nullptr) {
      const auto load_result_name =
          existing_prepared_value_name_id(names, load_local->result);
      if (load_result_name.has_value()) {
        load_access =
            find_unique_indexed_prepared_memory_access_by_result_value_name(
                memory_accesses, *load_result_name);
      }
    }
  }
  const auto load_access_matches =
      load_access != nullptr &&
      producer_instruction_index < block->insts.size() &&
      load_local != nullptr &&
      prepared_load_access_matches_result(names, load_access, *load_local);
  if (load_local == nullptr ||
      producer_block_label != block_label ||
      producer_instruction_index >= before_instruction_index ||
      producer_instruction_index >= block->insts.size() ||
      !load_access_matches ||
      !prepared_load_access_matches_result(names, load_access, *load_local) ||
      prepared_frame_slot_for_access(stack_layout, load_access) == nullptr) {
    return std::nullopt;
  }

  const auto limit =
      block != nullptr ? std::min(before_instruction_index, block->insts.size())
                       : std::size_t{0};
  for (std::size_t index = producer_instruction_index + 1; index < limit; ++index) {
    const auto* store = std::get_if<bir::StoreLocalInst>(&block->insts[index]);
    if (store == nullptr) {
      continue;
    }
    const auto* store_access =
        find_indexed_prepared_memory_access(memory_accesses, producer_block_label, index);
    if (store_access == nullptr ||
        !prepared_store_access_matches_value(names, store_access, *store) ||
        prepared_access_ranges_overlap(stack_layout, load_access, store_access)) {
      return std::nullopt;
    }
  }

  return PreparedScalarLoadLocalSourceProducer{
      .producer = source_producer,
      .source_access = load_access,
  };
}

}  // namespace c4c::backend::prepare
