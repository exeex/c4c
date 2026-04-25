#include "../lowering.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using DynamicGlobalAggregateArrayAccess = BirFunctionLowerer::DynamicGlobalAggregateArrayAccess;
using DynamicGlobalPointerArrayAccess = BirFunctionLowerer::DynamicGlobalPointerArrayAccess;
using DynamicGlobalScalarArrayAccess = BirFunctionLowerer::DynamicGlobalScalarArrayAccess;
using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using DynamicLocalPointerArrayAccess = BirFunctionLowerer::DynamicLocalPointerArrayAccess;
using GlobalAddress = BirFunctionLowerer::GlobalAddress;
using LocalPointerArrayBase = BirFunctionLowerer::LocalPointerArrayBase;
using PointerAddress = BirFunctionLowerer::PointerAddress;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::type_size_bytes;

bool BirFunctionLowerer::lower_scalar_or_local_memory_inst(
    const c4c::codegen::lir::LirInst& inst,
    std::vector<bir::Inst>* lowered_insts) {
  auto& value_aliases = value_aliases_;
  auto& compare_exprs = compare_exprs_;
  auto& local_slot_types = local_slot_types_;
  auto& local_pointer_slots = local_pointer_slots_;
  auto& local_array_slots = local_array_slots_;
  auto& local_pointer_array_bases = local_pointer_array_bases_;
  auto& dynamic_local_pointer_arrays = dynamic_local_pointer_arrays_;
  auto& dynamic_local_aggregate_arrays = dynamic_local_aggregate_arrays_;
  auto& dynamic_pointer_value_arrays = dynamic_pointer_value_arrays_;
  auto& local_aggregate_slots = local_aggregate_slots_;
  auto& local_aggregate_field_slots = local_aggregate_field_slots_;
  auto& local_pointer_value_aliases = local_pointer_value_aliases_;
  auto& local_scalar_slot_values = local_scalar_slot_values_;
  auto& loaded_local_scalar_immediates = loaded_local_scalar_immediates_;
  auto& local_indirect_pointer_slots = local_indirect_pointer_slots_;
  auto& pointer_value_addresses = pointer_value_addresses_;
  auto& pointer_address_ints = pointer_address_ints_;
  auto& local_pointer_slot_addresses = local_pointer_slot_addresses_;
  auto& local_address_slots = local_address_slots_;
  auto& local_slot_address_slots = local_slot_address_slots_;
  auto& local_slot_pointer_values = local_slot_pointer_values_;
  auto& global_address_slots = global_address_slots_;
  auto& global_pointer_slots = global_pointer_slots_;
  auto& global_pointer_value_slots = global_pointer_value_slots_;
  auto& addressed_global_pointer_value_slots = addressed_global_pointer_value_slots_;
  auto& dynamic_global_pointer_arrays = dynamic_global_pointer_arrays_;
  auto& dynamic_global_aggregate_arrays = dynamic_global_aggregate_arrays_;
  auto& dynamic_global_scalar_arrays = dynamic_global_scalar_arrays_;
  auto& global_object_pointer_slots = global_object_pointer_slots_;
  auto& global_address_ints = global_address_ints_;
  auto& global_object_address_ints = global_object_address_ints_;
  const auto& aggregate_params = aggregate_params_;
  const auto& global_types = global_types_;
  auto* lowered_function = &lowered_function_;
  const auto fail_scalar_cast = [&]() {
    note_function_lowering_family_failure("scalar-cast semantic family");
    return false;
  };
  const auto fail_scalar_binop = [&]() {
    note_function_lowering_family_failure("scalar-binop semantic family");
    return false;
  };
  const auto fail_alloca = [&]() {
    note_function_lowering_family_failure("alloca local-memory semantic family");
    return false;
  };
  const auto fail_gep = [&]() {
    note_function_lowering_family_failure("gep local-memory semantic family");
    return false;
  };
  const auto fail_store = [&]() {
    note_function_lowering_family_failure("store local-memory semantic family");
    return false;
  };
  const auto fail_load = [&]() {
    note_function_lowering_family_failure("load local-memory semantic family");
    return false;
  };
  const auto resolve_runtime_pointer_address =
      [&](std::string_view operand_name) -> std::optional<PointerAddress> {
    const auto operand = std::string(operand_name);
    if (const auto addressed_ptr_it = pointer_value_addresses.find(operand);
        addressed_ptr_it != pointer_value_addresses.end()) {
      return addressed_ptr_it->second;
    }

    if (const auto local_slot_ptr_it = local_slot_pointer_values.find(operand);
        local_slot_ptr_it != local_slot_pointer_values.end()) {
      if (local_slot_ptr_it->second.byte_offset < 0) {
        return std::nullopt;
      }

      const std::string base_slot =
          local_slot_ptr_it->second.array_element_slots.empty()
              ? local_slot_ptr_it->second.slot_name
              : local_slot_ptr_it->second.array_element_slots.front();
      return PointerAddress{
          .base_value = bir::Value::named(bir::TypeKind::Ptr, base_slot),
          .value_type = local_slot_ptr_it->second.value_type,
          .byte_offset = static_cast<std::size_t>(local_slot_ptr_it->second.byte_offset),
          .storage_type_text = local_slot_ptr_it->second.storage_type_text,
          .type_text = local_slot_ptr_it->second.type_text,
      };
    }

    const auto local_ptr_it = local_pointer_slots.find(operand);
    if (local_ptr_it == local_pointer_slots.end()) {
      return std::nullopt;
    }

    const auto slot_type_it = local_slot_types.find(local_ptr_it->second);
    if (slot_type_it == local_slot_types.end()) {
      return std::nullopt;
    }

    std::string base_slot = local_ptr_it->second;
    std::size_t byte_offset = 0;
    if (const auto array_base_it = local_pointer_array_bases.find(operand);
        array_base_it != local_pointer_array_bases.end()) {
      if (array_base_it->second.element_slots.empty()) {
        return std::nullopt;
      }
      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return std::nullopt;
      }
      base_slot = array_base_it->second.element_slots.front();
      byte_offset = array_base_it->second.base_index * slot_size;
    }

    const auto type_text = render_type(slot_type_it->second);
    return PointerAddress{
        .base_value = bir::Value::named(bir::TypeKind::Ptr, std::move(base_slot)),
        .value_type = slot_type_it->second,
        .byte_offset = byte_offset,
        .storage_type_text = type_text,
        .type_text = type_text,
    };
  };
  const auto clear_local_scalar_slot_values = [&]() { local_scalar_slot_values.clear(); };
  const auto erase_local_scalar_slot_value = [&](std::string_view ptr_name) {
    const auto ptr_it = local_pointer_slots.find(std::string(ptr_name));
    if (ptr_it != local_pointer_slots.end()) {
      local_scalar_slot_values.erase(ptr_it->second);
    }
  };
  const auto publish_exact_local_pointer_owner =
      [&](std::string_view result_name, std::string_view slot_name) {
        value_aliases[std::string(result_name)] =
            bir::Value::named(bir::TypeKind::Ptr, std::string(slot_name));
      };
  if (std::holds_alternative<c4c::codegen::lir::LirCmpOp>(inst)) {
    const auto scalar_result =
        lower_scalar_family_inst(inst, value_aliases, compare_exprs, lowered_insts);
    return scalar_result.value_or(false);
  }

  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    if (cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_scalar_cast();
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::PtrToInt &&
        cast->to_type.str() == "i64") {
      if (cast->operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
        const std::string global_name = cast->operand.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it != global_types.end() && global_it->second.supports_linear_addressing) {
          global_address_ints[cast->result.str()] = GlobalAddress{
              .global_name = global_name,
              .value_type = global_it->second.value_type,
              .byte_offset = 0,
          };
          return true;
        }
        if (global_it != global_types.end() && global_it->second.supports_direct_value &&
            global_it->second.value_type == bir::TypeKind::Ptr) {
          global_object_address_ints[cast->result.str()] = GlobalAddress{
              .global_name = global_name,
              .value_type = bir::TypeKind::Ptr,
              .byte_offset = 0,
          };
          return true;
        }
      }

      if (cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto global_object_it = global_object_pointer_slots.find(cast->operand.str());
        if (global_object_it != global_object_pointer_slots.end()) {
          global_object_address_ints[cast->result.str()] = global_object_it->second;
          return true;
        }

        const auto global_ptr_it = global_pointer_slots.find(cast->operand.str());
        if (global_ptr_it != global_pointer_slots.end()) {
          global_address_ints[cast->result.str()] = global_ptr_it->second;
          return true;
        }

        const auto pointer_addr = resolve_runtime_pointer_address(cast->operand.str());
        if (pointer_addr.has_value()) {
          pointer_address_ints[cast->result.str()] = *pointer_addr;
          return true;
        }
      }
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::IntToPtr &&
        cast->from_type.str() == "i64" && cast->to_type.str() == "ptr" &&
        cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto pointer_addr_it = pointer_address_ints.find(cast->operand.str());
      if (pointer_addr_it != pointer_address_ints.end()) {
        pointer_value_addresses[cast->result.str()] = pointer_addr_it->second;
        return true;
      }

      const auto global_object_addr_it = global_object_address_ints.find(cast->operand.str());
      if (global_object_addr_it != global_object_address_ints.end()) {
        global_object_pointer_slots[cast->result.str()] = global_object_addr_it->second;
        return true;
      }

      const auto global_addr_it = global_address_ints.find(cast->operand.str());
      if (global_addr_it != global_address_ints.end()) {
        global_pointer_slots[cast->result.str()] = global_addr_it->second;
        return true;
      }
    }

    const auto scalar_result =
        lower_scalar_family_inst(inst, value_aliases, compare_exprs, lowered_insts);
    return scalar_result.value_or(false) ? true : fail_scalar_cast();
  }

  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    if (bin->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_scalar_binop();
    }

    const auto opcode = lower_scalar_binary_opcode(bin->opcode);
    const auto value_type = lower_scalar_or_function_pointer_type(bin->type_str.str());
    if (!opcode.has_value() || !value_type.has_value() || *value_type == bir::TypeKind::Ptr) {
      return fail_scalar_binop();
    }

    if (*opcode == bir::BinaryOpcode::Sub && *value_type == bir::TypeKind::I64 &&
        bin->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        bin->rhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto lhs_pointer_addr_it = pointer_address_ints.find(bin->lhs.str());
      const auto rhs_pointer_addr_it = pointer_address_ints.find(bin->rhs.str());
      if (lhs_pointer_addr_it != pointer_address_ints.end() &&
          rhs_pointer_addr_it != pointer_address_ints.end() &&
          lhs_pointer_addr_it->second.base_value == rhs_pointer_addr_it->second.base_value) {
        value_aliases[bin->result.str()] = bir::Value::immediate_i64(
            static_cast<std::int64_t>(lhs_pointer_addr_it->second.byte_offset) -
            static_cast<std::int64_t>(rhs_pointer_addr_it->second.byte_offset));
        return true;
      }

      const auto lhs_addr_it = global_address_ints.find(bin->lhs.str());
      const auto rhs_addr_it = global_address_ints.find(bin->rhs.str());
      if (lhs_addr_it != global_address_ints.end() && rhs_addr_it != global_address_ints.end() &&
          lhs_addr_it->second.global_name == rhs_addr_it->second.global_name) {
        const auto lhs_offset = static_cast<std::int64_t>(lhs_addr_it->second.byte_offset);
        const auto rhs_offset = static_cast<std::int64_t>(rhs_addr_it->second.byte_offset);
        value_aliases[bin->result.str()] = bir::Value::immediate_i64(lhs_offset - rhs_offset);
        return true;
      }
    }

    const auto scalar_result =
        lower_scalar_family_inst(inst, value_aliases, compare_exprs, lowered_insts);
    return scalar_result.value_or(false) ? true : fail_scalar_binop();
  }

  if (lower_runtime_intrinsic_inst(inst, value_aliases, lowered_insts)) {
    return true;
  }

  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    return lower_local_memory_alloca_inst(*alloca, lowered_insts) ? true : fail_alloca();
  }

  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
    return lower_memory_gep_inst(*gep, lowered_insts) ? true : fail_gep();
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    return lower_memory_store_inst(*store, lowered_insts) ? true : fail_store();
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    return lower_memory_load_inst(*load, lowered_insts) ? true : fail_load();
  }

  if (const auto* memcpy = std::get_if<c4c::codegen::lir::LirMemcpyOp>(&inst)) {
    return lower_memory_memcpy_inst(*memcpy, lowered_insts);
  }

  if (const auto* memset = std::get_if<c4c::codegen::lir::LirMemsetOp>(&inst)) {
    return lower_memory_memset_inst(*memset, lowered_insts);
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    return lower_call_inst(*call, lowered_insts);
  }

  return false;
}
}  // namespace c4c::backend
