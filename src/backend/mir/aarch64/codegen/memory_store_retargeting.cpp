#include "memory_store_retargeting.hpp"

#include "../abi/abi.hpp"
#include "dispatch_lookup.hpp"
#include "../../../prealloc/control_flow.hpp"

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
