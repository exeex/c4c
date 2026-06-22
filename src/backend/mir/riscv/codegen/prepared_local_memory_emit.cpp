#include "prepared_local_memory_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_global_memory_emit.hpp"
#include "prepared_scalar_emit.hpp"

#include "../../../prealloc/addressing.hpp"

namespace c4c::backend::riscv::codegen {

namespace {

const c4c::backend::prepare::PreparedMemoryAccess* simple_frame_slot_access_for(
    const PreparedCurrentInstructionContext& context,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      (value_type != bir::TypeKind::I8 && value_type != bir::TypeKind::I32)) {
    return nullptr;
  }
  const std::size_t size_bytes = value_type == bir::TypeKind::I8 ? 1 : 4;
  const std::size_t align_bytes = value_type == bir::TypeKind::I8 ? 1 : 4;
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &context.lookups->memory_accesses,
      context.block_label,
      context.instruction_index);
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
    const PreparedCurrentInstructionContext& context,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr || value_type != bir::TypeKind::Ptr) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &context.lookups->memory_accesses,
      context.block_label,
      context.instruction_index);
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
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::I32 ||
      load.result.name.empty()) {
    return nullptr;
  }
  const auto result_value_name = context.names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &context.lookups->memory_accesses,
      context.block_label,
      context.instruction_index);
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
simple_pointer_address_materialization_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.type != bir::TypeKind::Ptr ||
      value.name.empty()) {
    return nullptr;
  }
  const auto value_name = context.names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* materializations =
      prepare::find_indexed_prepared_address_materializations(
          &context.lookups->address_materializations,
          context.block_label);
  if (materializations == nullptr) {
    return nullptr;
  }

  const prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->inst_index != context.instruction_index ||
        (materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot &&
         materialization->kind != prepare::PreparedAddressMaterializationKind::DirectGlobal) ||
        materialization->address_space != bir::AddressSpace::Default ||
        materialization->result_value_name != std::optional<c4c::ValueNameId>{value_name}) {
      continue;
    }
    if (materialization->kind == prepare::PreparedAddressMaterializationKind::FrameSlot &&
        (!materialization->frame_slot_id.has_value() ||
         !fits_signed_12_bit_immediate(materialization->byte_offset))) {
      continue;
    }
    if (materialization->kind == prepare::PreparedAddressMaterializationKind::DirectGlobal &&
        (!materialization->symbol_name.has_value() ||
         materialization->address_materialization_policy !=
             bir::GlobalAddressMaterializationPolicy::Direct ||
         materialization->byte_offset != 0 ||
         materialization->is_thread_local ||
         materialization->has_tls_address_space)) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = materialization;
  }
  return selected;
}

}  // namespace

std::optional<std::string> emit_riscv_simple_store_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::StoreLocalInst& store,
    const PreparedCurrentInstructionContext& context) {
  if (store.value.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto* access = simple_pointer_frame_slot_access_for(
        context,
        store.value.type);
    const auto* materialization = simple_pointer_address_materialization_for(
        context,
        store.value);
    if (access == nullptr || materialization == nullptr) {
      return std::nullopt;
    }
    const auto destination_stack_offset =
        simple_frame_slot_sp_offset_for(prepared, function_name, *access);
    if (!destination_stack_offset.has_value()) {
      return std::nullopt;
    }

    std::string out;
    if (materialization->kind ==
        c4c::backend::prepare::PreparedAddressMaterializationKind::FrameSlot) {
      const auto source_stack_offset =
          simple_frame_slot_sp_offset_for(prepared, function_name, *materialization);
      if (!source_stack_offset.has_value()) {
        return std::nullopt;
      }
      out += "    addi t1, sp, " + std::to_string(*source_stack_offset) + "\n";
    } else if (materialization->kind ==
               c4c::backend::prepare::PreparedAddressMaterializationKind::DirectGlobal) {
      const auto materialized =
          emit_riscv_direct_global_address_materialization(prepared, *materialization, "t1");
      if (!materialized.has_value()) {
        return std::nullopt;
      }
      out += *materialized;
    } else {
      return std::nullopt;
    }
    out += "    sd t1, " + std::to_string(*destination_stack_offset) + "(sp)\n";
    return out;
  }

  const auto* access = simple_frame_slot_access_for(
      context,
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
  if (!emit_move_to_register(out, "t1", context.names, context.lookups, store.value)) {
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
    const PreparedCurrentInstructionContext& context) {
  if (load.result.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto* access = simple_pointer_frame_slot_access_for(
        context,
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
        prepared_pointer_register_for_value(context, load.result);
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
          context,
          load);
      pointer_access != nullptr) {
    const auto base_register = prepared_register_for_value_name_id(
        context,
        *pointer_access->address.pointer_value_name);
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const auto destination_register =
        prepared_register_for_value(context, load.result);
    if (destination_register.has_value()) {
      return "    lw " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
    }

    const auto* destination_home = prepared_value_home_for(context, load.result);
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
      context,
      load.result.type);
  if (access == nullptr) {
    return std::nullopt;
  }
  const auto stack_offset =
      simple_frame_slot_sp_offset_for(prepared, function_name, *access);
  if (!stack_offset.has_value()) {
    return std::nullopt;
  }
  const auto destination_register = prepared_register_for_value(context, load.result);
  if (destination_register.has_value()) {
    return emit_i32_load_from_stack_offset(*destination_register, *stack_offset);
  }

  const auto* destination_home = prepared_value_home_for(context, load.result);
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
