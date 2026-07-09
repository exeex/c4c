#include "prepared_local_memory_emit.hpp"

#include "prepared_emit_context.hpp"
#include "prepared_frame_emit.hpp"
#include "prepared_global_memory_emit.hpp"
#include "prepared_scalar_emit.hpp"
#include "rv64_line_assembler.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/prepared_contract_verifier.hpp"

#include <algorithm>
#include <limits>

namespace c4c::backend::riscv::codegen {

namespace {

const c4c::backend::prepare::PreparedMemoryAccess* simple_frame_slot_access_for(
    const PreparedCurrentInstructionContext& context,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr) {
    return nullptr;
  }
  const auto size_bytes = rv64_local_memory_size_for_type(value_type);
  if (!size_bytes.has_value()) {
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
      access->address.size_bytes != *size_bytes ||
      access->address.align_bytes > *size_bytes ||
      !access->address.can_use_base_plus_offset ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

bool is_simple_scalar_frame_slot_access(
    const c4c::backend::prepare::PreparedMemoryAccess& access,
    c4c::backend::bir::TypeKind value_type) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  const auto size_bytes = rv64_local_memory_size_for_type(value_type);
  if (!size_bytes.has_value()) {
    return false;
  }
  return access.address_space == bir::AddressSpace::Default &&
         !access.is_volatile &&
         access.address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
         access.address.frame_slot_id.has_value() &&
         access.address.size_bytes == *size_bytes &&
         access.address.align_bytes <= *size_bytes &&
         access.address.can_use_base_plus_offset &&
         fits_signed_12_bit_immediate(access.address.byte_offset);
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_frame_slot_access_for_load(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  if (context.lookups == nullptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.name.empty()) {
    return nullptr;
  }
  if (!rv64_local_memory_size_for_type(load.result.type).has_value()) {
    return nullptr;
  }
  const auto result_value_name = context.names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  if (const auto* access =
          c4c::backend::prepare::find_indexed_prepared_memory_access(
              &context.lookups->memory_accesses,
              context.block_label,
              context.instruction_index);
      access != nullptr &&
      access->result_value_name == std::optional<c4c::ValueNameId>{result_value_name} &&
      is_simple_scalar_frame_slot_access(*access, load.result.type)) {
    return access;
  }

  const c4c::backend::prepare::PreparedMemoryAccess* selected = nullptr;
  for (const auto& entry : context.lookups->memory_accesses.accesses_by_position) {
    const auto* access = entry.second;
    if (access == nullptr ||
        access->block_label != context.block_label ||
        access->result_value_name != std::optional<c4c::ValueNameId>{result_value_name} ||
        !is_simple_scalar_frame_slot_access(*access, load.result.type)) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = access;
  }
  return selected;
}

bool is_simple_pointer_frame_slot_access(
    const c4c::backend::prepare::PreparedMemoryAccess& access) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  return access.address_space == bir::AddressSpace::Default &&
         !access.is_volatile &&
         access.address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
         access.address.frame_slot_id.has_value() &&
         access.address.size_bytes == 8 &&
         access.address.align_bytes >= 8 &&
         access.address.can_use_base_plus_offset &&
         fits_signed_12_bit_immediate(access.address.byte_offset);
}

const c4c::backend::prepare::PreparedMemoryAccess*
simple_pointer_frame_slot_access_for_store(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::StoreLocalInst& store) {
  namespace bir = c4c::backend::bir;
  if (context.lookups == nullptr ||
      store.value.type != bir::TypeKind::Ptr ||
      ((store.value.kind != bir::Value::Kind::Named ||
        store.value.name.empty()) &&
       (store.value.kind != bir::Value::Kind::Immediate ||
        store.value.immediate != 0))) {
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
  if (const auto* access =
          c4c::backend::prepare::find_indexed_prepared_memory_access(
              &context.lookups->memory_accesses,
              context.block_label,
              context.instruction_index);
      access != nullptr &&
      access->stored_value_name == stored_value_name &&
      is_simple_pointer_frame_slot_access(*access)) {
    return access;
  }

  const c4c::backend::prepare::PreparedMemoryAccess* selected = nullptr;
  for (const auto& entry : context.lookups->memory_accesses.accesses_by_position) {
    const auto* access = entry.second;
    if (access == nullptr ||
        access->block_label != context.block_label ||
        access->stored_value_name != stored_value_name ||
        !is_simple_pointer_frame_slot_access(*access)) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = access;
  }
  return selected;
}

const c4c::backend::prepare::PreparedMemoryAccess*
simple_pointer_frame_slot_access_for_load(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  if (context.lookups == nullptr ||
      load.result.type != bir::TypeKind::Ptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.name.empty()) {
    return nullptr;
  }
  const auto result_value_name = context.names.value_names.find(load.result.name);
  if (result_value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  if (const auto* access =
          c4c::backend::prepare::find_indexed_prepared_memory_access(
              &context.lookups->memory_accesses,
              context.block_label,
              context.instruction_index);
      access != nullptr &&
      access->result_value_name == std::optional<c4c::ValueNameId>{result_value_name} &&
      is_simple_pointer_frame_slot_access(*access)) {
    return access;
  }

  const c4c::backend::prepare::PreparedMemoryAccess* selected = nullptr;
  for (const auto& entry : context.lookups->memory_accesses.accesses_by_position) {
    const auto* access = entry.second;
    if (access == nullptr ||
        access->block_label != context.block_label ||
        access->result_value_name != std::optional<c4c::ValueNameId>{result_value_name} ||
        !is_simple_pointer_frame_slot_access(*access)) {
      continue;
    }
    if (selected != nullptr) {
      return nullptr;
    }
    selected = access;
  }
  return selected;
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
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_f32_access_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::F32 ||
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
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
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
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
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
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_i16_access_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::StoreLocalInst& store) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      store.value.type != bir::TypeKind::I16 ||
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
      access->address.size_bytes != 2 ||
      access->address.align_bytes < 2 ||
      access->address.byte_offset < 0 ||
      access->address.byte_offset % 2 != 0 ||
      !access->address.can_use_base_plus_offset ||
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_i16_access_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::LoadLocalInst& load) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      load.result.kind != bir::Value::Kind::Named ||
      load.result.type != bir::TypeKind::I16 ||
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
      access->address.size_bytes != 2 ||
      access->address.align_bytes < 2 ||
      access->address.byte_offset < 0 ||
      access->address.byte_offset % 2 != 0 ||
      !access->address.can_use_base_plus_offset ||
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* simple_pointer_value_f32_access_for(
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::StoreLocalInst& store) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (context.lookups == nullptr ||
      store.value.type != bir::TypeKind::F32 ||
      store.value.kind != bir::Value::Kind::Named ||
      store.value.name.empty()) {
    return nullptr;
  }
  const auto value_name = context.names.value_names.find(store.value.name);
  if (value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto* access = prepare::find_indexed_prepared_memory_access(
      &context.lookups->memory_accesses,
      context.block_label,
      context.instruction_index);
  if (access == nullptr ||
      access->result_value_name.has_value() ||
      access->stored_value_name != std::optional<c4c::ValueNameId>{value_name} ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      access->address.size_bytes != 4 ||
      access->address.align_bytes < 4 ||
      access->address.byte_offset < 0 ||
      access->address.byte_offset % 4 != 0 ||
      !access->address.can_use_base_plus_offset ||
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
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

const c4c::backend::prepare::PreparedAddressMaterialization*
simple_pointer_address_materialization_before_or_at(
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
        materialization->inst_index > context.instruction_index ||
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
    if (selected != nullptr && selected->inst_index == materialization->inst_index) {
      return nullptr;
    }
    if (selected == nullptr || selected->inst_index < materialization->inst_index) {
      selected = materialization;
    }
  }
  return selected;
}

std::optional<std::string> load_pointer_value_base_register(
    std::string& out,
    const PreparedCurrentInstructionContext& context,
    c4c::ValueNameId value_name,
    std::string_view scratch_register) {
  const auto existing_register = prepared_register_for_value_name_id(
      context,
      value_name);
  if (existing_register.has_value()) {
    return existing_register;
  }
  if (context.lookups == nullptr || value_name == c4c::kInvalidValueName ||
      scratch_register.empty()) {
    return std::nullopt;
  }
  const auto value_id_it = context.lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == context.lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it =
      context.lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == context.lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value() ||
      !home.size_bytes.has_value() ||
      *home.size_bytes < 8 ||
      !fits_signed_12_bit_load_offset(*home.offset_bytes)) {
    return std::nullopt;
  }
  out += "    ld " + std::string{scratch_register} + ", " +
         std::to_string(*home.offset_bytes) + "(sp)\n";
  return std::string{scratch_register};
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

std::optional<std::size_t> rv64_scalar_memory_size_for_type_local(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I8:
      return std::size_t{1};
    case c4c::backend::bir::TypeKind::I16:
      return std::size_t{2};
    case c4c::backend::bir::TypeKind::I32:
      return std::size_t{4};
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return std::size_t{8};
    default:
      return std::nullopt;
  }
}

bool rv64_floating_type_local(c4c::backend::bir::TypeKind type) {
  return type == c4c::backend::bir::TypeKind::F32 ||
         type == c4c::backend::bir::TypeKind::F64;
}

std::optional<std::int64_t> integer_immediate_for_value_local(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      break;
    default:
      return std::nullopt;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto report =
      prepare::verify_prepared_rematerializable_integer_immediate_contract(home);
  if (report.owner_class != prepare::PreparedContractOwnerClass::Coherent) {
    return std::nullopt;
  }
  const auto fact = prepare::as_rematerializable_integer_immediate_fact(*home);
  return fact.has_value() ? std::optional<std::int64_t>{fact->signed_value}
                          : std::nullopt;
}

bool is_rv64_null_pointer_value_local(const c4c::backend::bir::Value& value) {
  return value.kind == c4c::backend::bir::Value::Kind::Immediate &&
         value.type == c4c::backend::bir::TypeKind::Ptr &&
         value.immediate == 0 && value.immediate_bits == 0;
}

std::optional<std::uint32_t> gpr_register_number_for_value_local(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  return home == nullptr ? std::nullopt
                         : rv64_prepared_gpr_register_number_for_home(*home);
}

std::optional<std::uint32_t> gpr_register_number_for_value_name_local(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::ValueNameId value_name) {
  if (lookups == nullptr || value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto value_id_it = lookups->value_homes.value_ids.find(value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  return rv64_prepared_gpr_register_number_for_home(*home_it->second);
}

std::optional<std::uint32_t> fpr_register_number_for_value_local(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  namespace prepare = c4c::backend::prepare;

  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->target_register_identity.has_value()) {
    return std::nullopt;
  }
  const auto& identity = *home->target_register_identity;
  if (identity.target_arch != c4c::TargetArch::Riscv64 ||
      identity.bank != prepare::PreparedRegisterBank::Fpr ||
      identity.register_class != prepare::PreparedRegisterClass::Float ||
      identity.physical_index > 31) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(identity.physical_index);
}

std::uint32_t rv64_temporary_gpr_avoiding_local(std::uint32_t reserved_register) {
  constexpr std::uint32_t t1 = 6;
  constexpr std::uint32_t t2 = 7;
  return reserved_register == t1 ? t2 : t1;
}

bool rv64_prepared_gpr_register_is_occupied_local(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::uint32_t candidate) {
  if (lookups == nullptr) {
    return true;
  }
  for (const auto& home_entry : lookups->value_homes.homes_by_id) {
    const auto* home = home_entry.second;
    if (home == nullptr) {
      continue;
    }
    const auto home_register = rv64_prepared_gpr_register_number_for_home(*home);
    if (home_register.has_value() && *home_register == candidate) {
      return true;
    }
  }
  return false;
}

std::optional<std::size_t> prepared_stack_slot_home_absolute_offset_for_value_local(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto* home = prepared_value_home_for(names, lookups, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  const auto size_bytes = rv64_scalar_memory_size_for_type_local(value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  return rv64_prepared_stack_slot_home_absolute_offset(stack_layout,
                                                       *home,
                                                       stack_frame_bytes,
                                                       *size_bytes);
}

std::optional<std::uint32_t> rv64_load_store_funct3_for_size_local(
    std::size_t size_bytes) {
  return rv64_prepared_load_store_funct3_for_size(size_bytes);
}

bool append_rv64_store_register_to_stack_local(RiscvEncodedFragment& fragment,
                                               std::uint32_t source_register,
                                               std::int32_t offset,
                                               std::size_t size_bytes = 4) {
  return append_rv64_prepared_store_register_to_stack(
      fragment, source_register, offset, size_bytes);
}

bool append_rv64_load_stack_to_register_local(RiscvEncodedFragment& fragment,
                                              std::uint32_t destination_register,
                                              std::int32_t offset,
                                              std::size_t size_bytes = 4) {
  return append_rv64_prepared_load_stack_to_register(
      fragment, destination_register, offset, size_bytes);
}

bool append_rv64_store_register_to_base_local(RiscvEncodedFragment& fragment,
                                              std::uint32_t source_register,
                                              std::uint32_t base_register,
                                              std::int32_t offset,
                                              std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_load_store_funct3_for_size_local(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_s_type(0x23,
                                       *funct3,
                                       base_register,
                                       source_register,
                                       offset));
  return true;
}

bool append_rv64_load_base_to_register_local(RiscvEncodedFragment& fragment,
                                             std::uint32_t destination_register,
                                             std::uint32_t base_register,
                                             std::int32_t offset,
                                             std::size_t size_bytes) {
  if (!fits_signed_12_bit_immediate(offset)) {
    return false;
  }
  const auto funct3 = rv64_load_store_funct3_for_size_local(size_bytes);
  if (!funct3.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x03,
                                       destination_register,
                                       *funct3,
                                       base_register,
                                       offset));
  return true;
}

void append_fragment_local(RiscvEncodedFragment& destination,
                           RiscvEncodedFragment source) {
  const auto base_offset = destination.bytes.size();
  destination.bytes.insert(destination.bytes.end(),
                           source.bytes.begin(),
                           source.bytes.end());
  for (auto& label : source.labels) {
    label.offset_bytes += base_offset;
    destination.labels.push_back(std::move(label));
  }
  for (auto& fixup : source.fixups) {
    fixup.offset_bytes += base_offset;
    destination.fixups.push_back(std::move(fixup));
  }
}

std::optional<std::string> prepared_scalar_direct_global_local_symbol(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedMemoryAccess& access) {
  if (!access.address.symbol_name.has_value()) {
    return std::nullopt;
  }
  const std::string_view symbol =
      names.link_names.spelling(*access.address.symbol_name);
  if (symbol.empty()) {
    return std::nullopt;
  }
  return std::string{symbol};
}

bool prepared_scalar_direct_global_local_access_is_supported(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::optional<c4c::ValueNameId> result_value_name,
    std::optional<c4c::ValueNameId> stored_value_name,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr ||
      access->result_value_name != result_value_name ||
      access->stored_value_name != stored_value_name ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      access->address.global_address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      access->address.provenance.layout_authority !=
          bir::MemoryLayoutAuthorityKind::ScalarLayout ||
      !fits_signed_12_bit_immediate(access->address.byte_offset) ||
      !prepare::prepared_global_symbol_memory_has_publication_authority(
          access->address)) {
    return false;
  }
  return prepared_scalar_direct_global_local_symbol(names, *access).has_value();
}

bool append_rv64_gpr_to_fpr_move_local(RiscvEncodedFragment& fragment,
                                       std::uint32_t destination,
                                       std::uint32_t source,
                                       c4c::backend::bir::TypeKind type) {
  std::optional<std::uint32_t> funct7;
  if (type == c4c::backend::bir::TypeKind::F32) {
    funct7 = 0x78;  // fmv.w.x
  } else if (type == c4c::backend::bir::TypeKind::F64) {
    funct7 = 0x79;  // fmv.d.x
  }
  if (!funct7.has_value()) {
    return false;
  }
  rv64_append_le32(fragment.bytes,
                   rv64_encode_r_type(0x53, destination, 0, source, 0, *funct7));
  return true;
}

bool append_rv64_store_fpr_to_base_local(RiscvEncodedFragment& fragment,
                                         std::uint32_t source_register,
                                         std::uint32_t base_register,
                                         std::int32_t offset,
                                         c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset) ||
      !rv64_floating_type_local(type)) {
    return false;
  }
  const std::uint32_t funct3 =
      type == c4c::backend::bir::TypeKind::F32 ? 2 : 3;
  rv64_append_le32(fragment.bytes,
                   rv64_encode_s_type(0x27,
                                       funct3,
                                       base_register,
                                       source_register,
                                       offset));
  return true;
}

bool append_rv64_load_base_to_fpr_local(RiscvEncodedFragment& fragment,
                                        std::uint32_t destination_register,
                                        std::uint32_t base_register,
                                        std::int32_t offset,
                                        c4c::backend::bir::TypeKind type) {
  if (!fits_signed_12_bit_immediate(offset) ||
      !rv64_floating_type_local(type)) {
    return false;
  }
  const std::uint32_t funct3 =
      type == c4c::backend::bir::TypeKind::F32 ? 2 : 3;
  rv64_append_le32(fragment.bytes,
                   rv64_encode_i_type(0x07,
                                       destination_register,
                                       funct3,
                                       base_register,
                                       offset));
  return true;
}

bool append_rv64_stack_offset_address_to_register_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    std::size_t offset) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    rv64_append_le32(fragment.bytes,
                     rv64_encode_i_type(0x13,
                                         destination,
                                         0,
                                         2,
                                         static_cast<std::int32_t>(
                                             signed_offset)));
    return true;
  }
  append_rv64_prepared_load_immediate(fragment, destination, signed_offset);
  append_rv64_prepared_add_registers(fragment, destination, 2, destination);
  return true;
}

bool append_rv64_store_register_to_stack_offset_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t source_register,
    std::size_t offset,
    std::size_t size_bytes) {
  return append_rv64_prepared_store_register_to_stack_offset(
      fragment, source_register, offset, size_bytes);
}

bool append_rv64_load_stack_offset_to_register_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination_register,
    std::size_t offset,
    std::size_t size_bytes) {
  return append_rv64_prepared_load_stack_offset_to_register(
      fragment, destination_register, offset, size_bytes);
}

bool append_rv64_store_fpr_to_stack_offset_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t source_register,
    std::size_t offset,
    c4c::backend::bir::TypeKind type) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    return append_rv64_store_fpr_to_base_local(
        fragment,
        source_register,
        2,
        static_cast<std::int32_t>(signed_offset),
        type);
  }
  constexpr std::uint32_t scratch = 5;  // t0
  append_rv64_prepared_load_immediate(fragment, scratch, signed_offset);
  append_rv64_prepared_add_registers(fragment, scratch, 2, scratch);
  return append_rv64_store_fpr_to_base_local(fragment, source_register, scratch, 0, type);
}

bool append_rv64_load_stack_offset_to_fpr_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination_register,
    std::size_t offset,
    c4c::backend::bir::TypeKind type) {
  if (offset >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return false;
  }
  const auto signed_offset = static_cast<std::int64_t>(offset);
  if (fits_signed_12_bit_immediate(signed_offset)) {
    return append_rv64_load_base_to_fpr_local(
        fragment,
        destination_register,
        2,
        static_cast<std::int32_t>(signed_offset),
        type);
  }
  constexpr std::uint32_t scratch = 5;  // t0
  append_rv64_prepared_load_immediate(fragment, scratch, signed_offset);
  append_rv64_prepared_add_registers(fragment, scratch, 2, scratch);
  return append_rv64_load_base_to_fpr_local(
      fragment, destination_register, scratch, 0, type);
}

bool append_rv64_move_value_to_register_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  if (is_rv64_null_pointer_value_local(value)) {
    append_rv64_prepared_load_immediate(fragment, destination, 0);
    return true;
  }
  const auto immediate = integer_immediate_for_value_local(names, lookups, value);
  if (immediate.has_value()) {
    append_rv64_prepared_load_immediate(fragment, destination, *immediate);
    return true;
  }
  const auto source = gpr_register_number_for_value_local(names, lookups, value);
  if (source.has_value()) {
    append_rv64_prepared_move(fragment, destination, *source);
    return true;
  }
  const auto stack_offset =
      prepared_stack_slot_home_absolute_offset_for_value_local(stack_layout,
                                                               names,
                                                               lookups,
                                                               value,
                                                               stack_frame_bytes);
  if (stack_offset.has_value()) {
    const auto size_bytes = rv64_scalar_memory_size_for_type_local(value.type);
    if (!size_bytes.has_value()) {
      return false;
    }
    return append_rv64_load_stack_offset_to_register_local(fragment,
                                                          destination,
                                                          *stack_offset,
                                                          *size_bytes);
  }
  return false;
}

std::optional<std::uint32_t> append_rv64_prepare_floating_value_for_store_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t scratch_fpr,
    std::uint32_t scratch_gpr,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value) {
  if (!rv64_floating_type_local(value.type)) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    if (value.immediate_bits >
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
      return std::nullopt;
    }
    append_rv64_prepared_load_immediate(
        fragment,
        scratch_gpr,
        static_cast<std::int64_t>(value.immediate_bits));
    if (!append_rv64_gpr_to_fpr_move_local(
            fragment, scratch_fpr, scratch_gpr, value.type)) {
      return std::nullopt;
    }
    return scratch_fpr;
  }
  return fpr_register_number_for_value_local(names, lookups, value);
}

const c4c::backend::prepare::PreparedVariadicVaListField*
rv64_variadic_va_list_overflow_arg_area_field_local(
    const c4c::backend::prepare::PreparedVariadicEntryPlanFunction& entry_plan) {
  namespace prepare = c4c::backend::prepare;

  for (const auto& field : entry_plan.va_list_layout.fields) {
    if (field.kind == prepare::PreparedVariadicVaListFieldKind::OverflowArgArea) {
      return &field;
    }
  }
  return nullptr;
}

bool rv64_variadic_helper_free_entry_contract_is_complete_local(
    const c4c::backend::prepare::PreparedVariadicEntryPlanFunction& entry_plan) {
  const auto* overflow_arg_area =
      rv64_variadic_va_list_overflow_arg_area_field_local(entry_plan);
  return !entry_plan.register_save_area.required &&
         entry_plan.overflow_area.required &&
         entry_plan.overflow_area.align_bytes == std::optional<std::size_t>{8} &&
         entry_plan.va_list_layout.required &&
         entry_plan.va_list_layout.size_bytes == std::optional<std::size_t>{8} &&
         entry_plan.va_list_layout.align_bytes == std::optional<std::size_t>{8} &&
         entry_plan.va_list_layout.fields.size() == 1 &&
         overflow_arg_area != nullptr && overflow_arg_area->offset_bytes == 0 &&
         overflow_arg_area->size_bytes == 8;
}

std::optional<std::int32_t> rv64_va_start_destination_load_offset_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::bir::LoadLocalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr || load.result.type != bir::TypeKind::Ptr ||
      load.slot_name.empty() ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.byte_offset != 0 ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes) {
    return std::nullopt;
  }

  const auto value_name = prepared.names.value_names.find(load.slot_name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(prepared, function_name);
  if (entry_plan == nullptr ||
      !rv64_variadic_helper_free_entry_contract_is_complete_local(*entry_plan)) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* selected = nullptr;
  const prepare::PreparedVariadicVaStartOperandHomes* selected_payload = nullptr;
  for (const auto& homes : entry_plan->helper_operand_homes) {
    const auto* payload =
        prepare::find_prepared_variadic_va_start_operand_homes(homes);
    if (homes.helper != prepare::PreparedVariadicEntryHelperKind::VaStart ||
        homes.block_index != block_index ||
        homes.instruction_index >= instruction_index ||
        payload == nullptr ||
        payload->destination_va_list_address.kind !=
            prepare::PreparedValueHomeKind::Register ||
        payload->destination_va_list_address.value_name != value_name ||
        payload->destination_va_list.kind != prepare::PreparedValueHomeKind::StackSlot ||
        !payload->destination_va_list.size_bytes.has_value() ||
        *payload->destination_va_list.size_bytes != size_bytes) {
      continue;
    }
    if (selected == nullptr ||
        homes.instruction_index > selected->instruction_index) {
      selected = &homes;
      selected_payload = payload;
    }
  }
  if (selected == nullptr || selected_payload == nullptr) {
    return std::nullopt;
  }
  return rv64_prepared_stack_slot_home_offset(prepared.stack_layout,
                                             selected_payload->destination_va_list,
                                             stack_frame_bytes,
                                             size_bytes);
}

}  // namespace

std::optional<std::size_t> prepared_frame_slot_absolute_byte_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr || access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      access->address.byte_offset < 0) {
    return std::nullopt;
  }
  std::size_t slot_offset = 0;
  if (*access->address.frame_slot_id != 0 || !stack_layout.frame_slots.empty()) {
    const auto slot_it =
        std::find_if(stack_layout.frame_slots.begin(),
                     stack_layout.frame_slots.end(),
                     [&](const prepare::PreparedFrameSlot& slot) {
                       return slot.slot_id == *access->address.frame_slot_id &&
                              (access->function_name == c4c::kInvalidFunctionName ||
                               slot.function_name == c4c::kInvalidFunctionName ||
                               slot.function_name == access->function_name);
                     });
    if (slot_it == stack_layout.frame_slots.end()) {
      return std::nullopt;
    }
    const auto slot_byte_offset =
        static_cast<std::size_t>(access->address.byte_offset);
    if (slot_byte_offset > slot_it->size_bytes ||
        slot_it->size_bytes - slot_byte_offset < size_bytes) {
      return std::nullopt;
    }
    slot_offset = slot_it->offset_bytes;
  }
  if (slot_offset > std::numeric_limits<std::size_t>::max() -
                        static_cast<std::size_t>(access->address.byte_offset)) {
    return std::nullopt;
  }
  const auto offset =
      slot_offset + static_cast<std::size_t>(access->address.byte_offset);
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < size_bytes ||
      offset > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return offset;
}

std::optional<std::pair<std::uint32_t, std::int32_t>>
prepared_pointer_value_base_offset(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  const auto base_register =
      gpr_register_number_for_value_name_local(lookups,
                                               *access->address.pointer_value_name);
  if (!base_register.has_value()) {
    return std::nullopt;
  }
  return std::pair<std::uint32_t, std::int32_t>{
      *base_register,
      static_cast<std::int32_t>(access->address.byte_offset)};
}

Rv64LargeSelectedPointerOffsetMaterializationStatus
rv64_large_selected_pointer_offset_materialization_status(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      !prepare::prepared_pointer_value_local_memory_required_authority_available(*access) ||
      fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return Rv64LargeSelectedPointerOffsetMaterializationStatus::NotApplicable;
  }
  const auto base_register =
      gpr_register_number_for_value_name_local(lookups,
                                               *access->address.pointer_value_name);
  if (!base_register.has_value()) {
    return Rv64LargeSelectedPointerOffsetMaterializationStatus::NotApplicable;
  }
  const auto& provenance = access->address.provenance;
  const auto& requested = provenance.requested_range;
  if (provenance.range_verdict == bir::MemoryRangeVerdict::ProvenOutOfBounds ||
      !requested.available ||
      requested.overflowed ||
      !requested.end_available ||
      requested.begin != access->address.byte_offset ||
      requested.size_bytes != size_bytes ||
      requested.end < requested.begin ||
      requested.end - requested.begin != static_cast<std::int64_t>(size_bytes)) {
    return Rv64LargeSelectedPointerOffsetMaterializationStatus::NotApplicable;
  }
  if (!access->address.rv64_large_selected_pointer_offset_scratch_clobber_authority) {
    return Rv64LargeSelectedPointerOffsetMaterializationStatus::
        MissingScratchClobberAuthority;
  }
  constexpr std::uint32_t scratch_register = 31;  // t6
  if (*base_register == scratch_register ||
      rv64_prepared_gpr_register_is_occupied_local(lookups, scratch_register)) {
    return Rv64LargeSelectedPointerOffsetMaterializationStatus::
        MalformedScratchClobberAuthority;
  }
  return Rv64LargeSelectedPointerOffsetMaterializationStatus::Available;
}

std::optional<std::size_t> prepared_pointer_value_stack_home_base_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > size_bytes ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  const auto value_id_it =
      lookups->value_homes.value_ids.find(*access->address.pointer_value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
      !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
      home.value_name != *access->address.pointer_value_name ||
      *home.size_bytes < 8 || *home.align_bytes > 8) {
    return std::nullopt;
  }
  const auto frame_slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *home.slot_id &&
                            slot.function_name == home.function_name;
                   });
  if (frame_slot_it == stack_layout.frame_slots.end() ||
      frame_slot_it->offset_bytes != *home.offset_bytes ||
      frame_slot_it->size_bytes != *home.size_bytes ||
      frame_slot_it->align_bytes != *home.align_bytes) {
    return std::nullopt;
  }
  const auto object_it =
      std::find_if(stack_layout.objects.begin(),
                   stack_layout.objects.end(),
                   [&](const prepare::PreparedStackObject& object) {
                     return object.object_id == frame_slot_it->object_id &&
                            object.function_name == home.function_name;
                   });
  if (object_it == stack_layout.objects.end() ||
      object_it->value_name != home.value_name ||
      object_it->size_bytes != *home.size_bytes ||
      object_it->align_bytes != *home.align_bytes ||
      object_it->source_kind == "byval_param" ||
      object_it->source_kind == "sret_param") {
    return std::nullopt;
  }
  if (*home.offset_bytes > stack_frame_bytes ||
      stack_frame_bytes - *home.offset_bytes < 8) {
    return std::nullopt;
  }
  return *home.offset_bytes;
}

std::optional<std::pair<std::uint32_t, std::int32_t>>
materialize_prepared_pointer_value_base_offset(
    RiscvEncodedFragment& fragment,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes,
    std::uint32_t scratch_register) {
  const auto base_register =
      prepared_pointer_value_base_offset(lookups, access, size_bytes);
  if (base_register.has_value()) {
    return base_register;
  }
  if (rv64_large_selected_pointer_offset_materialization_status(
          lookups,
          access,
          size_bytes) ==
      Rv64LargeSelectedPointerOffsetMaterializationStatus::Available) {
    const auto large_base_register =
        gpr_register_number_for_value_name_local(lookups,
                                                 *access->address.pointer_value_name);
    if (!large_base_register.has_value()) {
      return std::nullopt;
    }
    constexpr std::uint32_t large_offset_scratch_register = 31;  // t6
    append_rv64_prepared_load_immediate(fragment,
                                        large_offset_scratch_register,
                                        access->address.byte_offset);
    append_rv64_prepared_add_registers(fragment,
                                       large_offset_scratch_register,
                                       *large_base_register,
                                       large_offset_scratch_register);
    return std::pair<std::uint32_t, std::int32_t>{
        large_offset_scratch_register,
        0};
  }
  const auto stack_home_offset =
      prepared_pointer_value_stack_home_base_offset(stack_layout,
                                                   lookups,
                                                   access,
                                                   stack_frame_bytes,
                                                   size_bytes);
  if (!stack_home_offset.has_value() ||
      !append_rv64_load_stack_offset_to_register_local(fragment,
                                                      scratch_register,
                                                      *stack_home_offset,
                                                      8)) {
    return std::nullopt;
  }
  return std::pair<std::uint32_t, std::int32_t>{
      scratch_register,
      static_cast<std::int32_t>(access->address.byte_offset)};
}

std::optional<std::int32_t> prepared_byval_stack_slot_pointer_access_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > 8 ||
      access->address.byte_offset < 0 ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  if (!prepare::prepared_stack_home_local_memory_has_authority(
          stack_layout,
          &lookups->value_homes,
          *access,
          prepare::PreparedStackHomeLocalMemoryRole::ByvalParam)) {
    return std::nullopt;
  }
  const auto value_id_it =
      lookups->value_homes.value_ids.find(*access->address.pointer_value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
      !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
      home.value_name != *access->address.pointer_value_name ||
      access->address.align_bytes > *home.align_bytes ||
      *home.align_bytes > 8) {
    return std::nullopt;
  }
  const auto frame_slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *home.slot_id &&
                            slot.function_name == home.function_name;
                   });
  if (frame_slot_it == stack_layout.frame_slots.end() ||
      frame_slot_it->offset_bytes != *home.offset_bytes ||
      frame_slot_it->size_bytes != *home.size_bytes ||
      frame_slot_it->align_bytes != *home.align_bytes) {
    return std::nullopt;
  }
  const auto object_it =
      std::find_if(stack_layout.objects.begin(),
                   stack_layout.objects.end(),
                   [&](const prepare::PreparedStackObject& object) {
                     return object.object_id == frame_slot_it->object_id &&
                            object.function_name == home.function_name;
                   });
  if (object_it == stack_layout.objects.end() ||
      object_it->value_name != home.value_name ||
      object_it->source_kind != "byval_param" ||
      object_it->size_bytes != *home.size_bytes ||
      object_it->align_bytes != *home.align_bytes ||
      !object_it->address_exposed || !object_it->requires_home_slot ||
      !object_it->permanent_home_slot) {
    return std::nullopt;
  }
  const auto byte_offset =
      static_cast<std::size_t>(access->address.byte_offset);
  if (byte_offset > *home.size_bytes ||
      *home.size_bytes - byte_offset < size_bytes ||
      *home.offset_bytes > std::numeric_limits<std::size_t>::max() - byte_offset) {
    return std::nullopt;
  }
  const auto offset = *home.offset_bytes + byte_offset;
  if (offset > stack_frame_bytes || stack_frame_bytes - offset < size_bytes ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(offset))) {
    return std::nullopt;
  }
  return static_cast<std::int32_t>(offset);
}

std::optional<PreparedSretStackPointerAccess>
prepared_sret_stack_slot_pointer_access(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes,
    std::size_t size_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || access == nullptr ||
      access->address_space != bir::AddressSpace::Default ||
      access->is_volatile ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.size_bytes != size_bytes ||
      access->address.align_bytes > 8 ||
      !fits_signed_12_bit_immediate(access->address.byte_offset)) {
    return std::nullopt;
  }
  if (!prepare::prepared_stack_home_local_memory_has_authority(
          stack_layout,
          &lookups->value_homes,
          *access,
          prepare::PreparedStackHomeLocalMemoryRole::SretParam)) {
    return std::nullopt;
  }
  const auto value_id_it =
      lookups->value_homes.value_ids.find(*access->address.pointer_value_name);
  if (value_id_it == lookups->value_homes.value_ids.end()) {
    return std::nullopt;
  }
  const auto home_it = lookups->value_homes.homes_by_id.find(value_id_it->second);
  if (home_it == lookups->value_homes.homes_by_id.end() ||
      home_it->second == nullptr) {
    return std::nullopt;
  }
  const auto& home = *home_it->second;
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.slot_id.has_value() || !home.offset_bytes.has_value() ||
      !home.size_bytes.has_value() || !home.align_bytes.has_value() ||
      home.value_name != *access->address.pointer_value_name ||
      access->address.align_bytes > *home.align_bytes ||
      *home.align_bytes > 8) {
    return std::nullopt;
  }
  const auto frame_slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *home.slot_id &&
                            slot.function_name == home.function_name;
                   });
  if (frame_slot_it == stack_layout.frame_slots.end() ||
      frame_slot_it->offset_bytes != *home.offset_bytes ||
      frame_slot_it->size_bytes != *home.size_bytes ||
      frame_slot_it->align_bytes != *home.align_bytes) {
    return std::nullopt;
  }
  const auto object_it =
      std::find_if(stack_layout.objects.begin(),
                   stack_layout.objects.end(),
                   [&](const prepare::PreparedStackObject& object) {
                     return object.object_id == frame_slot_it->object_id &&
                            object.function_name == home.function_name;
                   });
  if (object_it == stack_layout.objects.end() ||
      object_it->value_name != home.value_name ||
      object_it->source_kind != "sret_param" ||
      object_it->type != bir::TypeKind::Ptr ||
      !object_it->address_exposed || !object_it->requires_home_slot ||
      !object_it->permanent_home_slot) {
    return std::nullopt;
  }
  if (*home.offset_bytes > stack_frame_bytes ||
      stack_frame_bytes - *home.offset_bytes < 8 ||
      !fits_signed_12_bit_immediate(static_cast<std::int64_t>(*home.offset_bytes))) {
    return std::nullopt;
  }
  return PreparedSretStackPointerAccess{
      .pointer_home_offset = static_cast<std::int32_t>(*home.offset_bytes),
      .pointee_offset = static_cast<std::int32_t>(access->address.byte_offset),
  };
}

std::optional<std::size_t> rv64_local_memory_size_for_type(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return std::size_t{4};
    case c4c::backend::bir::TypeKind::F64:
      return std::size_t{8};
    default:
      return rv64_scalar_memory_size_for_type_local(type);
  }
}

std::optional<std::size_t> prepared_frame_slot_address_materialization_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr || value.type != bir::TypeKind::Ptr ||
      value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups->address_materializations,
          block_label);
  if (materializations == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto* materialization : *materializations) {
    if (materialization == nullptr ||
        materialization->inst_index != instruction_index ||
        materialization->kind !=
            prepare::PreparedAddressMaterializationKind::FrameSlot ||
        materialization->result_value_name != value_name ||
        !materialization->frame_slot_id.has_value() ||
        materialization->byte_offset < 0 ||
        materialization->address_space != bir::AddressSpace::Default ||
        materialization->is_thread_local ||
        materialization->has_tls_address_space ||
        materialization->tls_model !=
            prepare::PreparedTlsMaterializationModel::None ||
        materialization->tls_thread_pointer_register !=
            prepare::PreparedTlsThreadPointerRegister::None ||
        materialization->tls_high_relocation !=
            prepare::PreparedTlsRelocationKind::None ||
        materialization->tls_low_relocation !=
            prepare::PreparedTlsRelocationKind::None) {
      continue;
    }
    if (selected != nullptr) {
      return std::nullopt;
    }
    selected = materialization;
  }
  if (selected == nullptr) {
    return std::nullopt;
  }

  const auto slot_it =
      std::find_if(stack_layout.frame_slots.begin(),
                   stack_layout.frame_slots.end(),
                   [&](const prepare::PreparedFrameSlot& slot) {
                     return slot.slot_id == *selected->frame_slot_id &&
                            (selected->function_name == c4c::kInvalidFunctionName ||
                             slot.function_name == c4c::kInvalidFunctionName ||
                             slot.function_name == selected->function_name);
                   });
  if (slot_it == stack_layout.frame_slots.end() ||
      selected->byte_offset < static_cast<std::int64_t>(slot_it->offset_bytes)) {
    return std::nullopt;
  }
  const auto offset = static_cast<std::size_t>(selected->byte_offset);
  if (slot_it->offset_bytes >
          std::numeric_limits<std::size_t>::max() - slot_it->size_bytes ||
      offset > slot_it->offset_bytes + slot_it->size_bytes ||
      offset > stack_frame_bytes ||
      offset > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    return std::nullopt;
  }
  return offset;
}

bool append_rv64_materialize_or_move_store_value_local(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes) {
  const auto frame_slot_address_offset =
      prepared_frame_slot_address_materialization_offset(stack_layout,
                                                         names,
                                                         lookups,
                                                         block_label,
                                                         instruction_index,
                                                         value,
                                                         stack_frame_bytes);
  if (frame_slot_address_offset.has_value()) {
    return append_rv64_stack_offset_address_to_register_local(
        fragment, destination, *frame_slot_address_offset);
  }
  return append_rv64_move_value_to_register_local(fragment,
                                                  destination,
                                                  stack_layout,
                                                  names,
                                                  lookups,
                                                  value,
                                                  stack_frame_bytes);
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_store_local(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const c4c::backend::bir::StoreLocalInst& store,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (rv64_floating_type_local(store.value.type)) {
    constexpr std::uint32_t scratch_fpr = 0;  // ft0
    const auto size_bytes = rv64_local_memory_size_for_type(store.value.type);
    if (!size_bytes.has_value()) {
      return std::nullopt;
    }
    const auto offset =
        prepared_frame_slot_absolute_byte_offset(stack_layout,
                                                 access,
                                                 stack_frame_bytes,
                                                 *size_bytes);
    if (offset.has_value()) {
      RiscvEncodedFragment fragment;
      const auto source_fpr = append_rv64_prepare_floating_value_for_store_local(
          fragment, scratch_fpr, 6, names, lookups, store.value);
      if (!source_fpr.has_value() ||
          !append_rv64_store_fpr_to_stack_offset_local(
              fragment, *source_fpr, *offset, store.value.type)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto sret_pointer =
        prepared_sret_stack_slot_pointer_access(stack_layout,
                                                lookups,
                                                access,
                                                stack_frame_bytes,
                                                *size_bytes);
    if (sret_pointer.has_value()) {
      RiscvEncodedFragment fragment;
      if (!append_rv64_load_stack_to_register_local(fragment,
                                                   7,
                                                   sret_pointer->pointer_home_offset,
                                                   8)) {
        return std::nullopt;
      }
      const auto source_fpr = append_rv64_prepare_floating_value_for_store_local(
          fragment, scratch_fpr, 6, names, lookups, store.value);
      if (!source_fpr.has_value() ||
          !append_rv64_store_fpr_to_base_local(fragment,
                                              *source_fpr,
                                              7,
                                              sret_pointer->pointee_offset,
                                              store.value.type)) {
        return std::nullopt;
      }
      return fragment;
    }
    RiscvEncodedFragment fragment;
    const auto pointer_base =
        materialize_prepared_pointer_value_base_offset(fragment,
                                                      stack_layout,
                                                      lookups,
                                                      access,
                                                      stack_frame_bytes,
                                                      *size_bytes,
                                                      7);
    if (!pointer_base.has_value()) {
      return std::nullopt;
    }
    const auto source_fpr = append_rv64_prepare_floating_value_for_store_local(
        fragment,
        scratch_fpr,
        rv64_temporary_gpr_avoiding_local(pointer_base->first),
        names,
        lookups,
        store.value);
    if (!source_fpr.has_value() ||
        !append_rv64_store_fpr_to_base_local(fragment,
                                            *source_fpr,
                                            pointer_base->first,
                                            pointer_base->second,
                                            store.value.type)) {
      return std::nullopt;
    }
    return fragment;
  }

  const auto size_bytes = rv64_scalar_memory_size_for_type_local(store.value.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  std::optional<c4c::ValueNameId> stored_value_name;
  if (store.value.kind == c4c::backend::bir::Value::Kind::Named) {
    const auto value_name = names.value_names.find(store.value.name);
    if (value_name == c4c::kInvalidValueName) {
      return std::nullopt;
    }
    stored_value_name = value_name;
  }
  if (prepared_scalar_direct_global_local_access_is_supported(names,
                                                             access,
                                                             std::nullopt,
                                                             stored_value_name,
                                                             *size_bytes)) {
    const auto symbol =
        prepared_scalar_direct_global_local_symbol(names, *access);
    if (!symbol.has_value()) {
      return std::nullopt;
    }
    RiscvEncodedFragment fragment;
    if (!append_rv64_materialize_or_move_store_value_local(fragment,
                                                          6,
                                                          stack_layout,
                                                          names,
                                                          lookups,
                                                          block_label,
                                                          instruction_index,
                                                          store.value,
                                                          stack_frame_bytes)) {
      return std::nullopt;
    }
    append_fragment_local(
        fragment,
        make_rv64_pcrel_address_fragment(
            5,
            *symbol,
            ".Lpcrel_hi_global_local_store_" +
                std::to_string(access->function_name) + "_" +
                std::to_string(access->block_label) + "_" +
                std::to_string(access->inst_index),
            RiscvObjectFixupTargetKind::Object,
            0));
    if (!append_rv64_store_register_to_base_local(
            fragment,
            6,
            5,
            static_cast<std::int32_t>(access->address.byte_offset),
            *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  const auto offset =
      prepared_frame_slot_absolute_byte_offset(stack_layout,
                                               access,
                                               stack_frame_bytes,
                                               *size_bytes);
  if (!offset.has_value()) {
    const auto byval_offset =
        prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                        lookups,
                                                        access,
                                                        stack_frame_bytes,
                                                        *size_bytes);
    if (byval_offset.has_value()) {
      RiscvEncodedFragment fragment;
      if (!append_rv64_materialize_or_move_store_value_local(fragment,
                                                            6,
                                                            stack_layout,
                                                            names,
                                                            lookups,
                                                            block_label,
                                                            instruction_index,
                                                            store.value,
                                                            stack_frame_bytes) ||
          !append_rv64_store_register_to_stack_local(fragment,
                                                     6,
                                                     *byval_offset,
                                                     *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto sret_pointer =
        prepared_sret_stack_slot_pointer_access(stack_layout,
                                                lookups,
                                                access,
                                                stack_frame_bytes,
                                                *size_bytes);
    if (sret_pointer.has_value()) {
      RiscvEncodedFragment fragment;
      if (!append_rv64_load_stack_to_register_local(fragment,
                                                   7,
                                                   sret_pointer->pointer_home_offset,
                                                   8) ||
          !append_rv64_move_value_to_register_local(fragment,
                                                   6,
                                                   stack_layout,
                                                   names,
                                                   lookups,
                                                   store.value,
                                                   stack_frame_bytes) ||
          !append_rv64_store_register_to_base_local(fragment,
                                                   6,
                                                   7,
                                                   sret_pointer->pointee_offset,
                                                   *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    RiscvEncodedFragment fragment;
    const auto pointer_base =
        materialize_prepared_pointer_value_base_offset(fragment,
                                                      stack_layout,
                                                      lookups,
                                                      access,
                                                      stack_frame_bytes,
                                                      *size_bytes,
                                                      7);
    if (!pointer_base.has_value()) {
      return std::nullopt;
    }
    const std::uint32_t value_register =
        rv64_temporary_gpr_avoiding_local(pointer_base->first);
    if (!append_rv64_materialize_or_move_store_value_local(fragment,
                                                          value_register,
                                                          stack_layout,
                                                          names,
                                                          lookups,
                                                          block_label,
                                                          instruction_index,
                                                          store.value,
                                                          stack_frame_bytes) ||
        !append_rv64_store_register_to_base_local(fragment,
                                                 value_register,
                                                 pointer_base->first,
                                                 pointer_base->second,
                                                 *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  RiscvEncodedFragment fragment;
  if (!append_rv64_materialize_or_move_store_value_local(fragment,
                                                        6,
                                                        stack_layout,
                                                        names,
                                                        lookups,
                                                        block_label,
                                                        instruction_index,
                                                        store.value,
                                                        stack_frame_bytes)) {
    return std::nullopt;
  }
  if (!append_rv64_store_register_to_stack_offset_local(
          fragment, 6, *offset, *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<RiscvEncodedFragment> fragment_for_prepared_load_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::LoadLocalInst& load,
    const c4c::backend::prepare::PreparedMemoryAccess* access,
    std::size_t stack_frame_bytes) {
  if (load.result.type == c4c::backend::bir::TypeKind::Ptr &&
      access != nullptr &&
      access->address_space == c4c::backend::bir::AddressSpace::Default &&
      !access->is_volatile &&
      access->address.base_kind ==
          c4c::backend::prepare::PreparedAddressBaseKind::StringConstant &&
      access->address.size_bytes == 8 &&
      access->address.align_bytes == 8 &&
      fits_signed_12_bit_immediate(access->address.byte_offset) &&
      c4c::backend::prepare::prepared_string_constant_label_pointer_has_authority(
          access->address)) {
    const auto destination =
        gpr_register_number_for_value_local(names, lookups, load.result);
    const auto destination_offset =
        prepared_stack_slot_home_absolute_offset_for_value_local(stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 load.result,
                                                                 stack_frame_bytes);
    if (!destination.has_value() && !destination_offset.has_value()) {
      return std::nullopt;
    }
    const std::string_view label =
        access->address.symbol_name.has_value()
            ? prepared.names.link_names.spelling(*access->address.symbol_name)
            : std::string_view{};
    if (label.empty()) {
      return std::nullopt;
    }
    const std::uint32_t destination_register = destination.value_or(6);
    RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
        destination_register,
        std::string{label},
        ".Lpcrel_hi_string_local_load_" + std::to_string(access->function_name) +
            "_" + std::to_string(access->block_label) + "_" +
            std::to_string(access->inst_index),
        RiscvObjectFixupTargetKind::Object,
        access->address.byte_offset);
    if (destination_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset_local(fragment,
                                                         destination_register,
                                                         *destination_offset,
                                                         8)) {
      return std::nullopt;
    }
    return fragment;
  }
  if (rv64_floating_type_local(load.result.type)) {
    const auto size_bytes = rv64_local_memory_size_for_type(load.result.type);
    if (!size_bytes.has_value()) {
      return std::nullopt;
    }
    const auto destination =
        fpr_register_number_for_value_local(names, lookups, load.result);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    const auto offset =
        prepared_frame_slot_absolute_byte_offset(stack_layout,
                                                 access,
                                                 stack_frame_bytes,
                                                 *size_bytes);
    RiscvEncodedFragment fragment;
    if (offset.has_value()) {
      if (!append_rv64_load_stack_offset_to_fpr_local(
              fragment, *destination, *offset, load.result.type)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto byval_offset =
        prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                        lookups,
                                                        access,
                                                        stack_frame_bytes,
                                                        *size_bytes);
    if (byval_offset.has_value()) {
      if (!append_rv64_load_stack_offset_to_fpr_local(
              fragment, *destination, *byval_offset, load.result.type)) {
        return std::nullopt;
      }
      return fragment;
    }
    const auto pointer_base =
        materialize_prepared_pointer_value_base_offset(fragment,
                                                      stack_layout,
                                                      lookups,
                                                      access,
                                                      stack_frame_bytes,
                                                      *size_bytes,
                                                      6);
    if (!pointer_base.has_value() ||
        !append_rv64_load_base_to_fpr_local(fragment,
                                           *destination,
                                           pointer_base->first,
                                           pointer_base->second,
                                           load.result.type)) {
      return std::nullopt;
    }
    return fragment;
  }

  const auto size_bytes = rv64_scalar_memory_size_for_type_local(load.result.type);
  if (!size_bytes.has_value()) {
    return std::nullopt;
  }
  if (load.result.kind == c4c::backend::bir::Value::Kind::Named &&
      !load.result.name.empty()) {
    const auto result_value_name = names.value_names.find(load.result.name);
    if (result_value_name != c4c::kInvalidValueName &&
        prepared_scalar_direct_global_local_access_is_supported(
            names,
            access,
            result_value_name,
            std::nullopt,
            *size_bytes)) {
      const auto symbol =
          prepared_scalar_direct_global_local_symbol(names, *access);
      const auto destination = gpr_register_number_for_value_local(
          names,
          lookups,
          load.result);
      const auto destination_offset =
          prepared_stack_slot_home_absolute_offset_for_value_local(stack_layout,
                                                                   names,
                                                                   lookups,
                                                                   load.result,
                                                                   stack_frame_bytes);
      if (!symbol.has_value() ||
          (!destination.has_value() && !destination_offset.has_value())) {
        return std::nullopt;
      }
      const std::uint32_t destination_register = destination.value_or(6);
      RiscvEncodedFragment fragment = make_rv64_pcrel_address_fragment(
          destination_register,
          *symbol,
          ".Lpcrel_hi_global_local_load_" +
              std::to_string(access->function_name) + "_" +
              std::to_string(access->block_label) + "_" +
              std::to_string(access->inst_index),
          RiscvObjectFixupTargetKind::Object,
          0);
      if (!append_rv64_load_base_to_register_local(
              fragment,
              destination_register,
              destination_register,
              static_cast<std::int32_t>(access->address.byte_offset),
              *size_bytes)) {
        return std::nullopt;
      }
      if (destination_offset.has_value() &&
          !append_rv64_store_register_to_stack_offset_local(fragment,
                                                           destination_register,
                                                           *destination_offset,
                                                           *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
  }
  const auto va_start_destination_offset =
      rv64_va_start_destination_load_offset_local(prepared,
                                                  function_name,
                                                  block_index,
                                                  instruction_index,
                                                  load,
                                                  access,
                                                  stack_frame_bytes,
                                                  *size_bytes);
  std::optional<std::size_t> offset;
  if (va_start_destination_offset.has_value()) {
    offset = static_cast<std::size_t>(*va_start_destination_offset);
  } else {
    offset = prepared_frame_slot_absolute_byte_offset(stack_layout,
                                                      access,
                                                      stack_frame_bytes,
                                                      *size_bytes);
  }
  if (!offset.has_value()) {
    const auto byval_offset =
        prepared_byval_stack_slot_pointer_access_offset(stack_layout,
                                                        lookups,
                                                        access,
                                                        stack_frame_bytes,
                                                        *size_bytes);
    if (byval_offset.has_value()) {
      const auto destination =
          gpr_register_number_for_value_local(names, lookups, load.result);
      const auto destination_offset =
          prepared_stack_slot_home_absolute_offset_for_value_local(stack_layout,
                                                                   names,
                                                                   lookups,
                                                                   load.result,
                                                                   stack_frame_bytes);
      if (!destination.has_value() && !destination_offset.has_value()) {
        return std::nullopt;
      }
      const std::uint32_t destination_register = destination.value_or(6);
      RiscvEncodedFragment fragment;
      if (!append_rv64_load_stack_to_register_local(fragment,
                                                   destination_register,
                                                   *byval_offset,
                                                   *size_bytes)) {
        return std::nullopt;
      }
      if (destination_offset.has_value() &&
          !append_rv64_store_register_to_stack_offset_local(fragment,
                                                           destination_register,
                                                           *destination_offset,
                                                           *size_bytes)) {
        return std::nullopt;
      }
      return fragment;
    }
    RiscvEncodedFragment fragment;
    const auto pointer_base =
        materialize_prepared_pointer_value_base_offset(fragment,
                                                      stack_layout,
                                                      lookups,
                                                      access,
                                                      stack_frame_bytes,
                                                      *size_bytes,
                                                      7);
    if (!pointer_base.has_value()) {
      return std::nullopt;
    }
    const auto destination = gpr_register_number_for_value_local(
        names,
        lookups,
        load.result);
    const auto destination_offset =
        prepared_stack_slot_home_absolute_offset_for_value_local(stack_layout,
                                                                 names,
                                                                 lookups,
                                                                 load.result,
                                                                 stack_frame_bytes);
    if (!destination.has_value() && !destination_offset.has_value()) {
      return std::nullopt;
    }
    const std::uint32_t destination_register = destination.value_or(6);
    if (!append_rv64_load_base_to_register_local(fragment,
                                                destination_register,
                                                pointer_base->first,
                                                pointer_base->second,
                                                *size_bytes)) {
      return std::nullopt;
    }
    if (destination_offset.has_value() &&
        !append_rv64_store_register_to_stack_offset_local(fragment,
                                                         destination_register,
                                                         *destination_offset,
                                                         *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  const auto destination = gpr_register_number_for_value_local(
      names,
      lookups,
      load.result);
  RiscvEncodedFragment fragment;
  if (destination.has_value()) {
    if (!append_rv64_load_stack_offset_to_register_local(fragment,
                                                        *destination,
                                                        *offset,
                                                        *size_bytes)) {
      return std::nullopt;
    }
    return fragment;
  }
  const auto destination_offset =
      prepared_stack_slot_home_absolute_offset_for_value_local(stack_layout,
                                                               names,
                                                               lookups,
                                                               load.result,
                                                               stack_frame_bytes);
  if (!destination_offset.has_value() ||
      !append_rv64_load_stack_offset_to_register_local(fragment,
                                                      6,
                                                      *offset,
                                                      *size_bytes) ||
      !append_rv64_store_register_to_stack_offset_local(
          fragment, 6, *destination_offset, *size_bytes)) {
    return std::nullopt;
  }
  return fragment;
}

std::optional<std::string> emit_riscv_simple_store_local(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::StoreLocalInst& store,
    const PreparedCurrentInstructionContext& context) {
  if (store.value.type == c4c::backend::bir::TypeKind::Ptr) {
    const auto* access = simple_pointer_frame_slot_access_for_store(
        context,
        store);
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
    if (store.value.kind == c4c::backend::bir::Value::Kind::Immediate &&
        store.value.immediate == 0) {
      out += "    sd zero, " + std::to_string(*destination_stack_offset) + "(sp)\n";
      return out;
    }
    if (materialization == nullptr) {
      const auto source_register = prepared_pointer_register_for_value(context, store.value);
      if (source_register.has_value()) {
        out += "    sd " + *source_register + ", " +
               std::to_string(*destination_stack_offset) + "(sp)\n";
        return out;
      }
      if (emit_move_to_register(out,
                                "t1",
                                context.names,
                                context.lookups,
                                store.value)) {
        out += "    sd t1, " + std::to_string(*destination_stack_offset) + "(sp)\n";
        return out;
      }
      materialization = simple_pointer_address_materialization_before_or_at(
          context,
          store.value);
      if (materialization == nullptr) {
        return std::nullopt;
      }
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
      const auto materialization_register =
          prepared_pointer_register_for_value(context, store.value).value_or("t1");
      const auto materialized =
          emit_riscv_direct_global_address_materialization(
              prepared,
              *materialization,
              materialization_register);
      if (!materialized.has_value()) {
        return std::nullopt;
      }
      out += *materialized;
      out += "    sd " + materialization_register + ", " +
             std::to_string(*destination_stack_offset) + "(sp)\n";
      return out;
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
    std::string out;
    const auto base_register = load_pointer_value_base_register(
        out,
        context,
        *pointer_access->address.pointer_value_name,
        "t3");
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const std::string source_register = *base_register == "t1" ? "t3" : "t1";
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

  if (const auto* pointer_access = simple_pointer_value_i16_access_for(
          context,
          store);
      pointer_access != nullptr) {
    std::string out;
    const auto base_register = load_pointer_value_base_register(
        out,
        context,
        *pointer_access->address.pointer_value_name,
        "t3");
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const std::string source_register = *base_register == "t1" ? "t3" : "t1";
    if (!emit_move_to_register(
            out,
            source_register,
            context.names,
            context.lookups,
            store.value)) {
      return std::nullopt;
    }
    out += "    sh " + source_register + ", " +
           std::to_string(pointer_access->address.byte_offset) + "(" +
           *base_register + ")\n";
    return out;
  }

  if (const auto* pointer_access = simple_pointer_value_f32_access_for(
          context,
          store);
      pointer_access != nullptr) {
    std::string out;
    const auto base_register = load_pointer_value_base_register(
        out,
        context,
        *pointer_access->address.pointer_value_name,
        "t3");
    const auto source_register = prepared_register_for_value(context, store.value);
    if (!base_register.has_value() || !source_register.has_value()) {
      return std::nullopt;
    }
    out += "    fsw " + *source_register + ", " +
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
  if (store.value.type == c4c::backend::bir::TypeKind::F32 ||
      store.value.type == c4c::backend::bir::TypeKind::F64) {
    const auto source_register = prepared_register_for_value(context, store.value);
    if (!source_register.has_value()) {
      return std::nullopt;
    }
    out += std::string{store.value.type == c4c::backend::bir::TypeKind::F32
                           ? "    fsw "
                           : "    fsd "} +
           *source_register + ", " + std::to_string(*stack_offset) + "(sp)\n";
    return out;
  }

  if (!emit_move_to_register(out, "t1", context.names, context.lookups, store.value)) {
    return std::nullopt;
  }
  if (store.value.type == c4c::backend::bir::TypeKind::I8) {
    out += "    sb t1, " + std::to_string(*stack_offset) + "(sp)\n";
    return out;
  }
  if (store.value.type == c4c::backend::bir::TypeKind::I16) {
    out += "    sh t1, " + std::to_string(*stack_offset) + "(sp)\n";
    return out;
  }
  if (store.value.type == c4c::backend::bir::TypeKind::I64) {
    out += "    sd t1, " + std::to_string(*stack_offset) + "(sp)\n";
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
      std::string out;
      const auto base_register = load_pointer_value_base_register(
          out,
          context,
          *pointer_access->address.pointer_value_name,
          "t3");
      const auto destination_register =
          prepared_pointer_register_for_value(context, load.result);
      if (!base_register.has_value()) {
        return std::nullopt;
      }
      if (destination_register.has_value()) {
        out += "    ld " + *destination_register + ", " +
               std::to_string(pointer_access->address.byte_offset) + "(" +
               *base_register + ")\n";
        return out;
      }
      const auto* destination_home = prepared_value_home_for(context, load.result);
      if (destination_home == nullptr ||
          destination_home->kind !=
              c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
          !destination_home->offset_bytes.has_value() ||
          destination_home->size_bytes != std::optional<std::size_t>{8} ||
          !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
        return std::nullopt;
      }
      const std::string destination_scratch = *base_register == "t3" ? "t1" : "t3";
      out += "    ld " + destination_scratch + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
      out += "    sd " + destination_scratch + ", " +
             std::to_string(*destination_home->offset_bytes) + "(sp)\n";
      return out;
    }

    const auto* access = simple_pointer_frame_slot_access_for_load(
        context,
        load);
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

  if (!rv64_local_memory_size_for_type(load.result.type).has_value()) {
    return std::nullopt;
  }

  if (const auto* pointer_access = simple_pointer_value_f32_access_for(
          context,
          load);
      pointer_access != nullptr) {
    std::string out;
    const auto base_register = load_pointer_value_base_register(
        out,
        context,
        *pointer_access->address.pointer_value_name,
        "t3");
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const auto destination_register =
        prepared_register_for_value(context, load.result);
    if (destination_register.has_value()) {
      out += "    flw " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
      return out;
    }

    const auto* destination_home = prepared_value_home_for(context, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        destination_home->size_bytes != std::optional<std::size_t>{4} ||
        !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    out += "    flw ft0, " +
           std::to_string(pointer_access->address.byte_offset) + "(" +
           *base_register + ")\n";
    out += "    fsw ft0, " +
           std::to_string(*destination_home->offset_bytes) + "(sp)\n";
    return out;
  }

  if (const auto* pointer_access = simple_pointer_value_i16_access_for(
          context,
          load);
      pointer_access != nullptr) {
    std::string out;
    const auto base_register = load_pointer_value_base_register(
        out,
        context,
        *pointer_access->address.pointer_value_name,
        "t3");
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const auto destination_register =
        prepared_register_for_value(context, load.result);
    if (destination_register.has_value()) {
      out += "    lh " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
      return out;
    }

    const auto* destination_home = prepared_value_home_for(context, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        destination_home->size_bytes != std::optional<std::size_t>{2} ||
        !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    const std::string destination_scratch = *base_register == "t3" ? "t1" : "t3";
    out += "    lh " + destination_scratch + ", " +
           std::to_string(pointer_access->address.byte_offset) + "(" +
           *base_register + ")\n";
    out += "    sh " + destination_scratch + ", " +
           std::to_string(*destination_home->offset_bytes) + "(sp)\n";
    return out;
  }

  if (const auto* pointer_access = simple_pointer_value_i32_access_for(
          context,
          load);
      pointer_access != nullptr) {
    std::string out;
    const auto base_register = load_pointer_value_base_register(
        out,
        context,
        *pointer_access->address.pointer_value_name,
        "t3");
    if (!base_register.has_value()) {
      return std::nullopt;
    }
    const auto destination_register =
        prepared_register_for_value(context, load.result);
    if (destination_register.has_value()) {
      out += "    lw " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
      return out;
    }

    const auto* destination_home = prepared_value_home_for(context, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
          destination_home->size_bytes != std::optional<std::size_t>{4}) {
      return std::nullopt;
    }
    const std::string destination_scratch = *base_register == "t3" ? "t1" : "t3";
    out += "    lw " + destination_scratch + ", " +
           std::to_string(pointer_access->address.byte_offset) + "(" +
           *base_register + ")\n";
    const auto stored = emit_i32_store_to_stack_offset(
        destination_scratch,
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
        pointer_access->address.size_bytes == 1 &&
        pointer_access->address.can_use_base_plus_offset) {
      std::string out;
      const auto base_register = load_pointer_value_base_register(
          out,
          context,
          *pointer_access->address.pointer_value_name,
          "t3");
      const auto destination_register =
          prepared_register_for_value(context, load.result);
      if (!base_register.has_value() || !destination_register.has_value() ||
          !fits_signed_12_bit_immediate(pointer_access->address.byte_offset)) {
        return std::nullopt;
      }
      out += "    lb " + *destination_register + ", " +
             std::to_string(pointer_access->address.byte_offset) + "(" +
             *base_register + ")\n";
      return out;
    }
  }

  const auto* access = simple_frame_slot_access_for_load(context, load);
  if (access == nullptr) {
    access = simple_frame_slot_access_for(
        context,
        load.result.type);
  }
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
  if (load.result.type == c4c::backend::bir::TypeKind::I16) {
    if (destination_register.has_value()) {
      return "    lh " + *destination_register + ", " +
             std::to_string(*stack_offset) + "(sp)\n";
    }
    const auto* destination_home = prepared_value_home_for(context, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        destination_home->size_bytes != std::optional<std::size_t>{2} ||
        !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    return "    lh t3, " + std::to_string(*stack_offset) + "(sp)\n"
           "    sh t3, " + std::to_string(*destination_home->offset_bytes) + "(sp)\n";
  }
  if (load.result.type == c4c::backend::bir::TypeKind::F32 ||
      load.result.type == c4c::backend::bir::TypeKind::F64) {
    const auto size_bytes =
        load.result.type == c4c::backend::bir::TypeKind::F32 ? 4 : 8;
    if (destination_register.has_value()) {
      return std::string{load.result.type == c4c::backend::bir::TypeKind::F32
                             ? "    flw "
                             : "    fld "} +
             *destination_register + ", " +
             std::to_string(*stack_offset) + "(sp)\n";
    }
    const auto* destination_home = prepared_value_home_for(context, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        destination_home->size_bytes != std::optional<std::size_t>{size_bytes} ||
        !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    return std::string{load.result.type == c4c::backend::bir::TypeKind::F32
                           ? "    flw ft0, "
                           : "    fld ft0, "} +
           std::to_string(*stack_offset) + "(sp)\n" +
           std::string{load.result.type == c4c::backend::bir::TypeKind::F32
                           ? "    fsw ft0, "
                           : "    fsd ft0, "} +
           std::to_string(*destination_home->offset_bytes) + "(sp)\n";
  }
  if (load.result.type == c4c::backend::bir::TypeKind::I64) {
    if (destination_register.has_value()) {
      return "    ld " + *destination_register + ", " +
             std::to_string(*stack_offset) + "(sp)\n";
    }
    const auto* destination_home = prepared_value_home_for(context, load.result);
    if (destination_home == nullptr ||
        destination_home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
        !destination_home->offset_bytes.has_value() ||
        destination_home->size_bytes != std::optional<std::size_t>{8} ||
        !fits_signed_12_bit_load_offset(*destination_home->offset_bytes)) {
      return std::nullopt;
    }
    return "    ld t3, " + std::to_string(*stack_offset) + "(sp)\n"
           "    sd t3, " + std::to_string(*destination_home->offset_bytes) + "(sp)\n";
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
