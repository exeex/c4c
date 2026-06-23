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

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_ptr_access_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::Ptr ||
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
      access->address.size_bytes != 8 ||
      access->address.align_bytes < 8 ||
      access->address.byte_offset < 0 ||
      access->address.byte_offset % 8 != 0 ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_i32_access_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::StoreLocalInst& store) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      store.value.type != bir::TypeKind::I32 ||
      (store.value.kind == bir::Value::Kind::Named && store.value.name.empty())) {
    return nullptr;
  }
  std::optional<c4c::ValueNameId> stored_value_name;
  if (store.value.kind == bir::Value::Kind::Named) {
    const auto value_name = context.names.value_names.find(store.value.name);
    if (value_name == c4c::kInvalidValueName) {
      return nullptr;
    }
    stored_value_name = value_name;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &context.lookups->memory_accesses,
      context.block_label,
      context.instruction_index);
  if (access == nullptr ||
      access->result_value_name.has_value() ||
      access->stored_value_name != stored_value_name ||
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
simple_pointer_address_materialization_at(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value,
    std::size_t materialization_instruction_index) {
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
        materialization->inst_index != materialization_instruction_index ||
        (materialization->kind != prepare::PreparedAddressMaterializationKind::FrameSlot &&
         materialization->kind != prepare::PreparedAddressMaterializationKind::DirectGlobal &&
         materialization->kind != prepare::PreparedAddressMaterializationKind::StringConstant) ||
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
    if (materialization->kind == prepare::PreparedAddressMaterializationKind::StringConstant &&
        (!materialization->text_name.has_value() ||
         materialization->is_thread_local ||
         materialization->has_tls_address_space ||
         !fits_signed_12_bit_immediate(materialization->byte_offset))) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = materialization;
  }
  return selected;
}

const c4c::backend::prepare::PreparedAddressMaterialization*
simple_pointer_address_materialization_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Value& value) {
  return simple_pointer_address_materialization_at(
      context,
      value,
      context.instruction_index);
}

const c4c::backend::prepare::PreparedStoreSourcePublicationPlan*
simple_store_source_publication_for(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::StoreLocalInst& store) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (store.value.kind != bir::Value::Kind::Named ||
      store.value.name.empty()) {
    return nullptr;
  }
  const auto source_value_name = context.names.value_names.find(store.value.name);
  if (source_value_name == c4c::kInvalidValueName) {
    return nullptr;
  }

  const prepare::PreparedStoreSourcePublicationPlan* selected = nullptr;
  for (const auto& record : prepared.store_source_publications.records) {
    if (record.function_name != function_name ||
        record.block_label != context.block_label ||
        record.instruction_index != context.instruction_index ||
        record.plan.status != prepare::PreparedStoreSourcePublicationStatus::Available ||
        record.plan.intent !=
            prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication ||
        record.plan.source_value_name != source_value_name) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = &record.plan;
  }
  return selected;
}

std::optional<std::string> emit_riscv_string_constant_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedAddressMaterialization& materialization,
    std::string_view destination_register) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (destination_register.empty() ||
      materialization.kind != prepare::PreparedAddressMaterializationKind::StringConstant ||
      materialization.address_space != bir::AddressSpace::Default ||
      materialization.is_thread_local ||
      materialization.has_tls_address_space ||
      !materialization.text_name.has_value() ||
      !fits_signed_12_bit_immediate(materialization.byte_offset)) {
    return std::nullopt;
  }

  const std::string_view label = prepared.names.texts.lookup(*materialization.text_name);
  if (label.empty()) {
    return std::nullopt;
  }

  std::string out = "    lla " + std::string{destination_register} + ", " +
                    std::string{label} + "\n";
  if (materialization.byte_offset != 0) {
    out += "    addi " + std::string{destination_register} + ", " +
           std::string{destination_register} + ", " +
           std::to_string(materialization.byte_offset) + "\n";
  }
  return out;
}

std::optional<std::string> emit_riscv_string_constant_access_address_materialization(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedMemoryAccess& access,
    std::string_view destination_register) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (destination_register.empty() ||
      access.address_space != bir::AddressSpace::Default ||
      access.is_volatile ||
      access.address.base_kind != prepare::PreparedAddressBaseKind::StringConstant ||
      !access.address.symbol_name.has_value() ||
      !access.address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access.address.byte_offset)) {
    return std::nullopt;
  }

  const std::string_view label =
      prepared.names.link_names.spelling(*access.address.symbol_name);
  if (label.empty()) {
    return std::nullopt;
  }

  std::string out = "    lla " + std::string{destination_register} + ", " +
                    std::string{label} + "\n";
  if (access.address.byte_offset != 0) {
    out += "    addi " + std::string{destination_register} + ", " +
           std::string{destination_register} + ", " +
           std::to_string(access.address.byte_offset) + "\n";
  }
  return out;
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
    if (access == nullptr) {
      return std::nullopt;
    }
    const auto destination_stack_offset =
        simple_frame_slot_sp_offset_for(prepared, function_name, *access);
    if (!destination_stack_offset.has_value()) {
      return std::nullopt;
    }

    std::string out;
    if (materialization == nullptr) {
      const auto* store_source = simple_store_source_publication_for(
          prepared,
          function_name,
          context,
          store);
      if (store_source == nullptr) {
        return std::nullopt;
      }
      const auto source_register = prepared_pointer_register_for_value(context, store.value);
      if (!source_register.has_value()) {
        return std::nullopt;
      }
      out += "    sd " + *source_register + ", " +
             std::to_string(*destination_stack_offset) + "(sp)\n";
      return out;
    }

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
    } else if (materialization->kind ==
               c4c::backend::prepare::PreparedAddressMaterializationKind::StringConstant) {
      const auto materialized =
          emit_riscv_string_constant_address_materialization(
              prepared,
              *materialization,
              "t1");
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

  if (const auto* pointer_access = simple_pointer_value_i32_access_for(
          context,
          store);
      pointer_access != nullptr) {
    const auto base_register = prepared_register_for_value_name_id(
        context,
        *pointer_access->address.pointer_value_name);
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const std::string source_register = *base_register == "t1" ? "t3" : "t1";
    std::string out;
    if (!emit_move_to_register(
            out,
            source_register,
            context.names,
            context.lookups,
            store.value)) {
      return std::nullopt;
    }
    out += "    sw " + source_register + ", " +
           std::to_string(pointer_access->address.byte_offset) + "(" +
           *base_register + ")\n";
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
    const auto* materialization = simple_pointer_address_materialization_for(
        context,
        load.result);
    if (materialization != nullptr &&
        materialization->kind ==
            c4c::backend::prepare::PreparedAddressMaterializationKind::StringConstant) {
      const auto destination_register =
          prepared_pointer_register_for_value(context, load.result);
      if (!destination_register.has_value()) {
        return std::nullopt;
      }
      return emit_riscv_string_constant_address_materialization(
          prepared,
          *materialization,
          *destination_register);
    }

    const auto* string_access =
        context.lookups == nullptr
            ? nullptr
            : c4c::backend::prepare::find_indexed_prepared_memory_access(
                  &context.lookups->memory_accesses,
                  context.block_label,
                  context.instruction_index);
    const auto string_result_value_name =
        load.result.kind == c4c::backend::bir::Value::Kind::Named
            ? context.names.value_names.find(load.result.name)
            : c4c::kInvalidValueName;
    if (string_access != nullptr &&
        string_access->result_value_name ==
            std::optional<c4c::ValueNameId>{string_result_value_name} &&
        string_result_value_name != c4c::kInvalidValueName &&
        string_access->address.size_bytes == 8 &&
        string_access->address.align_bytes >= 8 &&
        string_access->address.base_kind ==
            c4c::backend::prepare::PreparedAddressBaseKind::StringConstant) {
      const auto destination_register =
          prepared_pointer_register_for_value(context, load.result);
      if (!destination_register.has_value()) {
        return std::nullopt;
      }
      return emit_riscv_string_constant_access_address_materialization(
          prepared,
          *string_access,
          *destination_register);
    }

    if (const auto* pointer_access = simple_pointer_value_ptr_access_for(
            context,
            load);
        pointer_access != nullptr) {
      const auto base_register = prepared_register_for_value_name_id(
          context,
          *pointer_access->address.pointer_value_name);
      const auto destination_register =
          prepared_pointer_register_for_value(context, load.result);
      if (!base_register.has_value() || !destination_register.has_value()) {
        return std::nullopt;
      }
      return "    ld " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
    }

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

  if (load.result.type != c4c::backend::bir::TypeKind::I8 &&
      load.result.type != c4c::backend::bir::TypeKind::I32) {
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

  if (load.result.type == c4c::backend::bir::TypeKind::I8) {
    const auto* pointer_access =
        context.lookups == nullptr
            ? nullptr
            : c4c::backend::prepare::find_indexed_prepared_memory_access(
                  &context.lookups->memory_accesses,
                  context.block_label,
                  context.instruction_index);
    if (pointer_access != nullptr &&
        pointer_access->address.base_kind ==
            c4c::backend::prepare::PreparedAddressBaseKind::PointerValue &&
        pointer_access->address.pointer_value_name.has_value() &&
        pointer_access->address.size_bytes == 1) {
      const auto base_register = prepared_register_for_value_name_id(
          context,
          *pointer_access->address.pointer_value_name);
      const auto destination_register =
          prepared_register_for_value(context, load.result);
      if (!base_register.has_value() || !destination_register.has_value() ||
          !fits_signed_12_bit_immediate(pointer_access->address.byte_offset)) {
        return std::nullopt;
      }
      return "    lb " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
    }
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
  if (load.result.type == c4c::backend::bir::TypeKind::I8) {
    if (!destination_register.has_value()) {
      return std::nullopt;
    }
    return "    lb " + *destination_register + ", " +
           std::to_string(*stack_offset) + "(sp)\n";
  }
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
