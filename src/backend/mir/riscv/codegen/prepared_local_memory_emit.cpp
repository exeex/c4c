#include "prepared_local_memory_emit.hpp"

#include "prepared_frame_emit.hpp"
#include "prepared_scalar_emit.hpp"

#include "../../../prealloc/addressing.hpp"

namespace c4c::backend::riscv::codegen {

namespace {

const c4c::backend::prepare::PreparedMemoryAccess* simple_frame_slot_access_for(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr ||
      (value_type != bir::TypeKind::I8 && value_type != bir::TypeKind::I32)) {
    return nullptr;
  }
  const std::size_t size_bytes = value_type == bir::TypeKind::I8 ? 1 : 4;
  const std::size_t align_bytes = value_type == bir::TypeKind::I8 ? 1 : 4;
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      block_label,
      instruction_index);
  if (access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes < align_bytes ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_frame_slot_access_for(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || value_type != bir::TypeKind::Ptr) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      block_label,
      instruction_index);
  if (access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      access->address.size_bytes != 8 ||
      access->address.align_bytes < 8 ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_i32_access_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::I32 ||
      load.result.name.empty()) {
    return nullptr;
  }
  const auto result_value_name = names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &lookups->memory_accesses,
      block_label,
      instruction_index);
  if (access == nullptr ||
      access->result_value_name != std::optional<c4c::ValueNameId>{result_value_name} ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      access->address.size_bytes != 4 ||
      access->address.align_bytes < 4 ||
      access->address.byte_offset < 0 ||
      access->address.byte_offset % 4 != 0 ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedAddressMaterialization*
simple_frame_slot_address_materialization_for(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.type != bir::TypeKind::Ptr ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups->address_materializations,
          block_label);
  if (materializations == nullptr) {
    return nullptr;
  }

  const prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->inst_index != instruction_index ||
        materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
        materialization->address_space != bir::AddressSpace::Default ||
        materialization->result_value_name != std::optional<c4c::ValueNameId>{value_name} ||
        !materialization->frame_slot_id.has_value() ||
        !fits_signed_12_bit_immediate(materialization->byte_offset)) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = materialization;
  }
  return selected;
}

bool fits_i32_bytewise_stack_offsets(std::int64_t stack_offset) {
  return fits_signed_12_bit_immediate(stack_offset) &&
         fits_signed_12_bit_immediate(stack_offset + 1) &&
         fits_signed_12_bit_immediate(stack_offset + 2) &&
         fits_signed_12_bit_immediate(stack_offset + 3);
}

}  // namespace

std::optional<std::string> emit_i32_store_to_stack_offset(std::string_view source_register,
                                                          std::int64_t stack_offset) {
  if (stack_offset % 4 == 0) {
    return "    sw " + std::string(source_register) + ", " +
           std::to_string(stack_offset) + "(sp)\n";
  }
  if (!fits_i32_bytewise_stack_offsets(stack_offset)) {
    return std::nullopt;
  }
  std::string out;
  out += "    sb " + std::string(source_register) + ", " +
         std::to_string(stack_offset) + "(sp)\n";
  out += "    srli t2, " + std::string(source_register) + ", 8\n";
  out += "    sb t2, " + std::to_string(stack_offset + 1) + "(sp)\n";
  out += "    srli t2, " + std::string(source_register) + ", 16\n";
  out += "    sb t2, " + std::to_string(stack_offset + 2) + "(sp)\n";
  out += "    srli t2, " + std::string(source_register) + ", 24\n";
  out += "    sb t2, " + std::to_string(stack_offset + 3) + "(sp)\n";
  return out;
}

std::optional<std::string> emit_i32_load_from_stack_offset(std::string_view destination_register,
                                                           std::int64_t stack_offset) {
  if (stack_offset % 4 == 0) {
    return "    lw " + std::string(destination_register) + ", " +
           std::to_string(stack_offset) + "(sp)\n";
  }
  if (!fits_i32_bytewise_stack_offsets(stack_offset)) {
    return std::nullopt;
  }
  std::string out;
  out += "    lbu " + std::string(destination_register) + ", " +
         std::to_string(stack_offset) + "(sp)\n";
  out += "    lbu t2, " + std::to_string(stack_offset + 1) + "(sp)\n";
  out += "    slli t2, t2, 8\n";
  out += "    or " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", t2\n";
  out += "    lbu t2, " + std::to_string(stack_offset + 2) + "(sp)\n";
  out += "    slli t2, t2, 16\n";
  out += "    or " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", t2\n";
  out += "    lbu t2, " + std::to_string(stack_offset + 3) + "(sp)\n";
  out += "    slli t2, t2, 24\n";
  out += "    or " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", t2\n";
  out += "    slli " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", 32\n";
  out += "    srai " + std::string(destination_register) + ", " +
         std::string(destination_register) + ", 32\n";
  return out;
}

std::optional<std::string> emit_riscv_simple_store_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::StoreLocalInst& store,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if (store.value.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto* access = simple_pointer_frame_slot_access_for(
        lookups,
        block_label,
        instruction_index,
        store.value.type);
    const auto* materialization = simple_frame_slot_address_materialization_for(
        names,
        lookups,
        block_label,
        instruction_index,
        store.value);
    if (access == nullptr || materialization == nullptr) {
      return std::nullopt;
    }
    const auto destination_stack_offset =
        simple_frame_slot_sp_offset_for(prepared, function_name, *access);
    const auto source_stack_offset =
        simple_frame_slot_sp_offset_for(prepared, function_name, *materialization);
    if (!destination_stack_offset.has_value() || !source_stack_offset.has_value()) {
      return std::nullopt;
    }

    std::string out;
    out += "    addi t1, sp, " + std::to_string(*source_stack_offset) + "\n";
    out += "    sd t1, " + std::to_string(*destination_stack_offset) + "(sp)\n";
    return out;
  }

  const auto* access = simple_frame_slot_access_for(
      lookups,
      block_label,
      instruction_index,
      store.value.type);
  if (access == nullptr) {
    return std::nullopt;
  }
  const auto stack_offset =
      simple_frame_slot_sp_offset_for(prepared, function_name, *access);
  if (!stack_offset.has_value()) {
    return std::nullopt;
  }

  std::string out;
  if (!emit_move_to_register(out, "t1", names, lookups, store.value)) {
    return std::nullopt;
  }
  if (store.value.type == c4c::backend::bir::TypeKind::I8) {
    out += "    sb t1, " + std::to_string(*stack_offset) + "(sp)\n";
    return out;
  }
  const auto stored = emit_i32_store_to_stack_offset("t1", *stack_offset);
  if (!stored.has_value()) {
    return std::nullopt;
  }
  out += *stored;
  return out;
}

std::optional<std::string> emit_riscv_simple_load_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::LoadLocalInst& load,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups) {
  if (load.result.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto* access = simple_pointer_frame_slot_access_for(
        lookups,
        block_label,
        instruction_index,
        load.result.type);
    if (access == nullptr) {
      return std::nullopt;
    }
    const auto stack_offset =
        simple_frame_slot_sp_offset_for(prepared, function_name, *access);
    if (!stack_offset.has_value()) {
      return std::nullopt;
    }
    const auto destination_register =
        prepared_pointer_register_for_value(names, lookups, load.result);
    if (!destination_register.has_value()) {
      return std::nullopt;
    }

    return "    ld " + *destination_register + ", " +
           std::to_string(*stack_offset) + "(sp)\n";
  }

  if (load.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  if (const auto* pointer_access = simple_pointer_value_i32_access_for(
          names,
          lookups,
          block_label,
          instruction_index,
          load);
      pointer_access != nullptr) {
    const auto base_register = prepared_register_for_value_name_id(
        lookups,
        *pointer_access->address.pointer_value_name);
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const auto destination_register =
        prepared_register_for_value(names, lookups, load.result);
    if (destination_register.has_value()) {
      return "    lw " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
    }

    const auto* destination_home = prepared_value_home_for(names, lookups, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        destination_home->size_bytes != std::optional<std::size_t>{4}) {
      return std::nullopt;
    }
    std::string out = "    lw t3, " +
                      std::to_string(pointer_access->address.byte_offset) + "(" +
                      *base_register + ")\n";
    const auto stored = emit_i32_store_to_stack_offset(
        "t3",
        static_cast<std::int64_t>(*destination_home->offset_bytes));
    if (!stored.has_value()) {
      return std::nullopt;
    }
    out += *stored;
    return out;
  }

  const auto* access = simple_frame_slot_access_for(
      lookups,
      block_label,
      instruction_index,
      load.result.type);
  if (access == nullptr) {
    return std::nullopt;
  }
  const auto stack_offset =
      simple_frame_slot_sp_offset_for(prepared, function_name, *access);
  if (!stack_offset.has_value()) {
    return std::nullopt;
  }
  const auto destination_register = prepared_register_for_value(names, lookups, load.result);
  if (destination_register.has_value()) {
    return emit_i32_load_from_stack_offset(*destination_register, *stack_offset);
  }

  const auto* destination_home = prepared_value_home_for(names, lookups, load.result);
  if (destination_home == nullptr ||
      destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !destination_home->offset_bytes.has_value() ||
      destination_home->size_bytes != std::optional<std::size_t>{4}) {
    return std::nullopt;
  }
  auto out = emit_i32_load_from_stack_offset("t3", *stack_offset);
  if (!out.has_value()) {
    return std::nullopt;
  }
  const auto stored = emit_i32_store_to_stack_offset(
      "t3",
      static_cast<std::int64_t>(*destination_home->offset_bytes));
  if (!stored.has_value()) {
    return std::nullopt;
  }
  *out += *stored;
  return out;
}

}  // namespace c4c::backend::riscv::codegen
