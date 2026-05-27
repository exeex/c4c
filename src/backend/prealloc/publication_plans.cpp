#include "publication_plans.hpp"

#include <algorithm>
#include <unordered_set>
#include <variant>

namespace c4c::backend::prepare {

namespace {

[[nodiscard]] const bir::Value* prepared_source_producer_result(
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

[[nodiscard]] bool prepared_source_producer_matches_indexed_instruction(
    const PreparedEdgePublicationSourceProducer& producer,
    const bir::Inst& inst,
    const bir::Value& value) {
  const auto* result = prepared_source_producer_result(producer);
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

[[nodiscard]] const PreparedEdgePublicationSourceProducer*
prepared_select_chain_source_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (source_producers == nullptr ||
      block_label == kInvalidBlockLabel ||
      block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name = resolve_prepared_value_name_id(names, value.name);
  if (!value_name.has_value()) {
    return nullptr;
  }
  const auto* producer =
      find_indexed_prepared_edge_publication_source_producer(
          source_producers, *value_name);
  if (producer == nullptr ||
      producer->block_label != block_label ||
      producer->instruction_index >= before_instruction_index ||
      producer->instruction_index >= block->insts.size()) {
    return nullptr;
  }
  const auto& inst = block->insts[producer->instruction_index];
  return prepared_source_producer_matches_indexed_instruction(*producer, inst, value)
             ? producer
             : nullptr;
}

[[nodiscard]] std::optional<bool>
prepared_select_chain_contains_direct_global_load(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  if (depth > 64U) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return false;
  }
  const auto* producer = prepared_select_chain_source_producer(
      names, source_producers, block_label, block, value, before_instruction_index);
  if (producer == nullptr) {
    return std::nullopt;
  }
  switch (producer->kind) {
    case PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return true;
    case PreparedEdgePublicationSourceProducerKind::LoadLocal:
    case PreparedEdgePublicationSourceProducerKind::Immediate:
      return false;
    case PreparedEdgePublicationSourceProducerKind::Cast:
      return producer->cast != nullptr
                 ? prepared_select_chain_contains_direct_global_load(
                       names,
                       source_producers,
                       block_label,
                       block,
                       producer->cast->operand,
                       producer->instruction_index,
                       depth + 1U)
                 : std::nullopt;
    case PreparedEdgePublicationSourceProducerKind::Binary: {
      if (producer->binary == nullptr) {
        return std::nullopt;
      }
      const auto lhs = prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->binary->lhs,
          producer->instruction_index,
          depth + 1U);
      if (!lhs.has_value() || *lhs) {
        return lhs;
      }
      return prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->binary->rhs,
          producer->instruction_index,
          depth + 1U);
    }
    case PreparedEdgePublicationSourceProducerKind::SelectMaterialization: {
      if (producer->select == nullptr) {
        return std::nullopt;
      }
      const auto true_value = prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->select->true_value,
          producer->instruction_index,
          depth + 1U);
      if (!true_value.has_value() || *true_value) {
        return true_value;
      }
      return prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          producer->select->false_value,
          producer->instruction_index,
          depth + 1U);
    }
    case PreparedEdgePublicationSourceProducerKind::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
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

}  // namespace

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

  if (inputs.source_producer != nullptr &&
      inputs.source_producer->kind != PreparedEdgePublicationSourceProducerKind::Unknown) {
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
  return plan_prepared_store_source_publication({
      .source_value = &store.value,
      .destination_access = access,
      .source_home = source_home,
      .intent = PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .pending_publication = pending_publication,
      .stack_homes_only = stack_homes_only,
      .duplicate_publication = duplicate_publication,
  });
}

std::vector<PreparedStoreGlobalPublicationCandidate>
plan_pending_prepared_store_global_publications(
    const PreparedValueLocationFunction* value_locations,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Block* block,
    std::size_t instruction_index) {
  std::vector<PreparedStoreGlobalPublicationCandidate> candidates;
  if (block == nullptr ||
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
  return access != nullptr &&
         access->address.base_kind == PreparedAddressBaseKind::PointerValue &&
         access->address.pointer_value_name.has_value() &&
         access->address.can_use_base_plus_offset &&
         is_byval_formal_value_name(
             names, bir_function, *access->address.pointer_value_name);
}

PreparedDirectGlobalSelectChainDependency
find_prepared_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  PreparedDirectGlobalSelectChainDependency dependency;
  const auto* root = prepared_select_chain_source_producer(
      names, source_producers, block_label, block, value, before_instruction_index);
  if (root == nullptr) {
    return dependency;
  }
  const auto contains_direct_global_load =
      prepared_select_chain_contains_direct_global_load(
          names,
          source_producers,
          block_label,
          block,
          value,
          before_instruction_index);
  if (!contains_direct_global_load.has_value() || !*contains_direct_global_load) {
    return dependency;
  }
  dependency.contains_direct_global_load = true;
  dependency.root_is_select =
      root->kind == PreparedEdgePublicationSourceProducerKind::SelectMaterialization;
  dependency.root_instruction_index = root->instruction_index;
  return dependency;
}

PreparedStoreSourceDirectGlobalSelectChainDependency
find_prepared_store_source_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  return find_prepared_direct_global_select_chain_dependency(
      names,
      source_producers,
      block_label,
      block,
      value,
      before_instruction_index);
}

PreparedScalarSelectChainMaterialization
find_prepared_scalar_select_chain_materialization(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  PreparedScalarSelectChainMaterialization materialization;
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return materialization;
  }
  const auto value_name = resolve_prepared_value_name_id(names, value.name);
  if (!value_name.has_value()) {
    return materialization;
  }
  auto dependency =
      find_prepared_direct_global_select_chain_dependency(
          names,
          source_producers,
          block_label,
          block,
          value,
          before_instruction_index);
  if (!dependency.contains_direct_global_load ||
      !dependency.root_instruction_index.has_value()) {
    return materialization;
  }
  materialization.available = true;
  materialization.root_value_name = *value_name;
  materialization.direct_global_dependency = dependency;
  return materialization;
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
  const auto* producer = prepared_select_chain_source_producer(
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
