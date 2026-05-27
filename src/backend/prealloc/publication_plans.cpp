#include "publication_plans.hpp"

#include "../mir/query.hpp"

namespace c4c::backend::prepare {

namespace mir = c4c::backend::mir;

namespace {

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

[[nodiscard]] std::string_view local_slot_reference_name(
    const bir::NameTables& bir_names,
    std::string_view raw_name,
    SlotNameId slot_id) {
  if (!raw_name.empty()) {
    return raw_name;
  }
  return slot_id != kInvalidSlotName ? bir_names.slot_names.spelling(slot_id)
                                     : std::string_view{};
}

[[nodiscard]] bool local_slot_reference_matches(
    const bir::NameTables& bir_names,
    const bir::LoadLocalInst& load,
    const bir::StoreLocalInst& store) {
  if (load.slot_id != kInvalidSlotName && store.slot_id != kInvalidSlotName &&
      load.slot_id == store.slot_id) {
    return true;
  }
  const auto load_name =
      local_slot_reference_name(bir_names, load.slot_name, load.slot_id);
  const auto store_name =
      local_slot_reference_name(bir_names, store.slot_name, store.slot_id);
  return !load_name.empty() && load_name == store_name;
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

[[nodiscard]] std::string_view prepared_frame_slot_object_name(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id) {
  const auto* slot = find_frame_slot(stack_layout, slot_id);
  const auto* object =
      slot != nullptr ? find_stack_object(stack_layout, slot->object_id) : nullptr;
  return object != nullptr ? prepared_stack_object_name(names, *object)
                           : std::string_view{};
}

[[nodiscard]] std::string_view prepared_load_local_frame_object_name(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    std::size_t load_instruction_index) {
  const auto* access =
      addressing != nullptr
          ? find_prepared_memory_access(*addressing, block_label, load_instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return std::string_view{};
  }
  return prepared_frame_slot_object_name(
      names, stack_layout, *access->address.frame_slot_id);
}

[[nodiscard]] bool value_name_has_slot_prefix(std::string_view value_name,
                                              std::string_view slot_name) {
  return !slot_name.empty() && value_name.size() > slot_name.size() &&
         value_name.substr(0, slot_name.size()) == slot_name &&
         value_name[slot_name.size()] == '.';
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

[[nodiscard]] bool store_local_targets_logical_slot(
    const bir::NameTables& bir_names,
    const bir::StoreLocalInst& store,
    std::string_view slot_name) {
  const auto store_name =
      local_slot_reference_name(bir_names, store.slot_name, store.slot_id);
  if (!store_name.empty() && store_name == slot_name) {
    return true;
  }
  return store.value.kind == bir::Value::Kind::Named &&
         value_name_has_slot_prefix(store.value.name, slot_name);
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

[[nodiscard]] bool dependency_is_load_global(
    const mir::DependencyTraversalRecord& record) {
  return record.kind == mir::SameBlockProducerKind::LoadGlobal;
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

std::optional<PreparedRecoveredStoreSourcePublication>
find_prepared_recovered_narrow_store_source_for_wide_local_load(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
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
  const auto load_slot_name =
      local_slot_reference_name(bir_names, load.slot_name, load.slot_id);
  const auto load_frame_object_name =
      prepared_load_local_frame_object_name(
          names, stack_layout, addressing, block_label, load_instruction_index);
  const auto load_lane_offset =
      load.result.kind == bir::Value::Kind::Named
          ? parse_trailing_dot_offset(load.result.name)
          : std::nullopt;
  for (std::size_t index = load_instruction_index; index > 0; --index) {
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&block->insts[index - 1]);
    if (store == nullptr ||
        (!local_slot_reference_matches(bir_names, load, *store) &&
         !store_local_targets_logical_slot(bir_names, *store, load_slot_name) &&
         !store_local_targets_logical_slot(bir_names, *store, load_frame_object_name) &&
         !(load_lane_offset.has_value() &&
           store->value.kind == bir::Value::Kind::Named &&
           parse_trailing_dot_offset(store->value.name) == load_lane_offset))) {
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
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  PreparedDirectGlobalSelectChainDependency dependency;
  if (block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return dependency;
  }
  const auto root = mir::find_same_block_named_producer_record(
      block, value.name, before_instruction_index);
  if (!root) {
    return dependency;
  }
  dependency.contains_direct_global_load = mir::select_chain_contains_dependency(
      block, value, before_instruction_index, dependency_is_load_global);
  if (!dependency.contains_direct_global_load) {
    return dependency;
  }
  dependency.root_is_select = root.kind == mir::SameBlockProducerKind::Select;
  dependency.root_instruction_index = root.instruction_index;
  return dependency;
}

PreparedStoreSourceDirectGlobalSelectChainDependency
find_prepared_store_source_direct_global_select_chain_dependency(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  return find_prepared_direct_global_select_chain_dependency(
      block, value, before_instruction_index);
}

}  // namespace c4c::backend::prepare
