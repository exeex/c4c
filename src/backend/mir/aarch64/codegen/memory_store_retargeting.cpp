#include "memory_store_retargeting.hpp"

#include "../abi/abi.hpp"
#include "dispatch_lookup.hpp"
#include "memory.hpp"
#include "../../../prealloc/control_flow.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <optional>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

struct PreparedLocalAddressStoreValue {
  bool has_frame_address = false;
  OperandRecord operand{};
};

std::optional<FrameSlotOperand> make_frame_slot_operand_from_stack_slot(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedFrameSlot& slot) {
  const auto* object = find_stack_object(stack_layout, slot.object_id);
  if (object == nullptr) {
    return std::nullopt;
  }
  return FrameSlotOperand{
      .slot_id = slot.slot_id,
      .object_id = slot.object_id,
      .function_name = slot.function_name,
      .slot_name = object->slot_name,
      .value_name = object->value_name,
      .type = object->type,
      .offset_bytes = slot.offset_bytes,
      .offset_is_prepared_snapshot = true,
      .size_bytes = slot.size_bytes,
      .align_bytes = slot.align_bytes,
      .fixed_location = slot.fixed_location,
  };
}

bool find_prepared_local_address_store_value(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedAddressMaterializationLookups* address_materialization_lookups,
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const bir::StoreLocalInst& store,
    const MemoryInstructionRecord& record,
    PreparedLocalAddressStoreValue& out) {
  if (record.memory_kind != MemoryInstructionKind::Store ||
      record.address.base_kind == MemoryBaseKind::Register ||
      store.value.kind != bir::Value::Kind::Named || store.value.type != bir::TypeKind::Ptr ||
      !record.address.stored_value_name.has_value()) {
    return true;
  }
  if (!record.address.stored_value_id.has_value()) {
    return true;
  }

  const auto prepared_address =
      prepare::find_indexed_prepared_frame_address_offset_for_value_id(
          stack_layout,
          address_materialization_lookups,
          value_home_lookups,
          record.address.block_label,
          *record.address.stored_value_id,
          record.address.instruction_index);
  if (!prepared_address.has_value()) {
    return true;
  }
  const auto* slot = find_frame_slot(stack_layout, prepared_address->frame_slot_id);
  if (slot == nullptr) {
    return false;
  }
  auto operand = make_frame_slot_operand_from_stack_slot(stack_layout, *slot);
  if (!operand.has_value()) {
    return false;
  }
  out.has_frame_address = true;
  out.operand = make_frame_slot_operand(*operand);
  return true;
}

bool rewrite_local_address_store_value(const PreparedLocalAddressStoreValue& prepared,
                                       MemoryInstructionRecord& record) {
  if (!prepared.has_frame_address) {
    return true;
  }
  record.value = prepared.operand;
  return true;
}

}  // namespace

[[nodiscard]] bool store_local_uses_pointer_value_address(
    const bir::StoreLocalInst& store) {
  return store.address.has_value() &&
         store.address->base_kind == bir::MemoryAddress::BaseKind::PointerValue;
}

[[nodiscard]] std::optional<RegisterOperand> prepared_or_emitted_store_value_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  if (value_name.has_value()) {
    if (const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
        emitted.has_value()) {
      return emitted;
    }
  }
  auto prepared = make_named_prepared_result_register(context, value);
  if (prepared.has_value()) {
    prepared->occupied_register_references = {prepared->reg};
    prepared->occupied_registers = {abi::register_name(prepared->reg)};
  }
  return prepared;
}

[[nodiscard]] bool resolve_frame_slot_memory_offset(
    const prepare::PreparedStackLayout& stack_layout,
    MemoryOperand& address) {
  if (address.base_kind != MemoryBaseKind::FrameSlot) {
    return true;
  }
  if (!address.frame_slot_id.has_value()) {
    return false;
  }
  const auto* slot = find_frame_slot(stack_layout, *address.frame_slot_id);
  if (slot == nullptr) {
    return false;
  }
  address.byte_offset += static_cast<std::int64_t>(slot->offset_bytes);
  address.byte_offset_is_prepared_snapshot = true;
  return true;
}

[[nodiscard]] bool apply_stack_layout_to_memory_record(
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedAddressMaterializationLookups* address_materialization_lookups,
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const bir::StoreLocalInst* local_store,
    MemoryInstructionRecord& record) {
  PreparedLocalAddressStoreValue prepared_local_address_store_value{};
  if (local_store != nullptr &&
      !find_prepared_local_address_store_value(
          stack_layout,
          address_materialization_lookups,
          value_home_lookups,
          *local_store,
          record,
          prepared_local_address_store_value)) {
    return false;
  }
  if (!resolve_frame_slot_memory_offset(stack_layout, record.address)) {
    return false;
  }
  if (local_store == nullptr) {
    return true;
  }
  return rewrite_local_address_store_value(prepared_local_address_store_value, record);
}

void retarget_pointer_store_value_to_materialized_address(
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  memory_record->value = make_register_operand(materialized_address);
}

void retarget_store_address_to_materialized_pointer(
    const bir::StoreLocalInst& store,
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  if (!store.address.has_value() ||
      store.address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store) {
    return;
  }

  memory_record->address.base_kind = MemoryBaseKind::Register;
  memory_record->address.base_register = materialized_address;
  memory_record->address.frame_slot_id.reset();
  memory_record->address.symbol_name.reset();
  memory_record->address.symbol_label.clear();
  memory_record->address.pointer_value_name.reset();
  memory_record->address.pointer_value_id.reset();
  memory_record->address.byte_offset = store.address->byte_offset;
  memory_record->address.byte_offset_is_prepared_snapshot = false;
  memory_record->address.size_bytes = store.address->size_bytes;
  memory_record->address.align_bytes = store.address->align_bytes;
  memory_record->address.address_space = store.address->address_space;
  memory_record->address.is_volatile = store.address->is_volatile;
  memory_record->address.can_use_base_plus_offset = true;
}

void retarget_pointer_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      store->value.type != bir::TypeKind::Ptr) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value()) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}

void retarget_store_local_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      !store_local_uses_pointer_value_address(*store) ||
      store->value.kind != bir::Value::Kind::Named) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store) {
    return;
  }
  const auto* current_register =
      memory_record->value.has_value()
          ? std::get_if<RegisterOperand>(&memory_record->value->payload)
          : nullptr;
  if (current_register == nullptr) {
    return;
  }
  const auto value_register =
      prepared_or_emitted_store_value_register(context, store->value, scalar_state);
  if (!value_register.has_value()) {
    return;
  }
  if (register_operands_share_physical_register(*current_register, *value_register)) {
    return;
  }
  memory_record->value = make_register_operand(*value_register);
}

}  // namespace c4c::backend::aarch64::codegen
