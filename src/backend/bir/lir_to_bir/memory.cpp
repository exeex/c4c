#include "../lir_to_bir.hpp"

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
using LocalArraySlots = BirFunctionLowerer::LocalArraySlots;
using LocalPointerArrayBase = BirFunctionLowerer::LocalPointerArrayBase;
using PointerAddress = BirFunctionLowerer::PointerAddress;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

bool BirFunctionLowerer::lower_scalar_or_local_memory_inst(
    const c4c::codegen::lir::LirInst& inst,
    std::vector<bir::Inst>* lowered_insts) {
  auto& value_aliases = value_aliases_;
  auto& compare_exprs = compare_exprs_;
  auto& aggregate_value_aliases = aggregate_value_aliases_;
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
  const auto& function_symbols = function_symbols_;
  const auto& type_decls = type_decls_;
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
  if (lower_scalar_compare_inst(inst, value_aliases, compare_exprs, lowered_insts)) {
    return true;
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

    const auto opcode = lower_cast_opcode(cast->kind);
    const auto from_type = lower_scalar_or_function_pointer_type(cast->from_type.str());
    const auto to_type = lower_scalar_or_function_pointer_type(cast->to_type.str());
    if (!opcode.has_value() || !from_type.has_value() || !to_type.has_value()) {
      return fail_scalar_cast();
    }

    const auto operand = lower_value(cast->operand, *from_type, value_aliases);
    if (!operand.has_value()) {
      return fail_scalar_cast();
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::Bitcast &&
        operand->type == *to_type) {
      value_aliases[cast->result.str()] = *operand;
      return true;
    }

    if (const auto folded = fold_integer_cast(cast->kind, *operand, *to_type);
        folded.has_value()) {
      value_aliases[cast->result.str()] = *folded;
      return true;
    }

    lowered_insts->push_back(bir::CastInst{
        .opcode = *opcode,
        .result = bir::Value::named(*to_type, cast->result.str()),
        .operand = *operand,
    });
    value_aliases[cast->result.str()] = bir::Value::named(*to_type, cast->result.str());
    return true;
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

    const auto operands = lower_scalar_binop_operands(*bin, *value_type, value_aliases);
    if (!operands.has_value()) {
      return fail_scalar_binop();
    }
    const auto& [lhs, rhs] = *operands;

    if (*value_type == bir::TypeKind::I64 && lhs.kind == bir::Value::Kind::Immediate &&
        rhs.kind == bir::Value::Kind::Immediate) {
      const auto folded = fold_i64_binary_immediates(*opcode, lhs.immediate, rhs.immediate);
      if (folded.has_value()) {
        value_aliases[bin->result.str()] = *folded;
        return true;
      }
    }

    lowered_insts->push_back(bir::BinaryInst{
        .opcode = *opcode,
        .result = bir::Value::named(*value_type, bin->result.str()),
        .operand_type = *value_type,
        .lhs = lhs,
        .rhs = rhs,
    });
    return true;
  }

  if (lower_runtime_intrinsic_inst(inst, value_aliases, lowered_insts)) {
    return true;
  }

  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    if (alloca->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_alloca();
    }

    const std::string slot_name = alloca->result.str();
    if (!alloca->count.str().empty()) {
      if (!context_.options.preserve_dynamic_alloca) {
        return fail_alloca();
      }
      const auto element_type = lower_scalar_or_function_pointer_type(alloca->type_str.str());
      const auto lowered_count = lower_value(alloca->count, bir::TypeKind::I64, value_aliases);
      if (!element_type.has_value() || !lowered_count.has_value()) {
        return fail_alloca();
      }
      const auto type_text =
          std::string(c4c::codegen::lir::trim_lir_arg_text(alloca->type_str.str()));
      lowered_insts->push_back(bir::CallInst{
          .result = bir::Value::named(bir::TypeKind::Ptr, slot_name),
          .callee = "llvm.dynamic_alloca." + type_text,
          .args = {*lowered_count},
          .arg_types = {bir::TypeKind::I64},
          .return_type = bir::TypeKind::Ptr,
      });
      pointer_value_addresses[slot_name] = PointerAddress{
          .base_value = bir::Value::named(bir::TypeKind::Ptr, slot_name),
          .value_type = *element_type,
          .byte_offset = 0,
          .storage_type_text = type_text,
          .type_text = type_text,
      };
      return true;
    }

    const auto slot_type = lower_scalar_or_function_pointer_type(alloca->type_str.str());
    if (local_slot_types.find(slot_name) != local_slot_types.end() ||
        local_array_slots.find(slot_name) != local_array_slots.end()) {
      return fail_alloca();
    }

    if (slot_type.has_value()) {
      local_slot_types.emplace(slot_name, *slot_type);
      local_pointer_slots.emplace(slot_name, slot_name);
      lowered_function->local_slots.push_back(bir::LocalSlot{
          .name = slot_name,
          .type = *slot_type,
          .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
      });
      return true;
    }

    const auto array_type = parse_local_array_type(alloca->type_str.str());
    if (array_type.has_value()) {
      LocalArraySlots array_slots{.element_type = array_type->second};
      array_slots.element_slots.reserve(array_type->first);
      for (std::size_t index = 0; index < array_type->first; ++index) {
        const std::string element_slot = slot_name + "." + std::to_string(index);
        local_slot_types.emplace(element_slot, array_type->second);
        local_pointer_slots.emplace(element_slot, element_slot);
        array_slots.element_slots.push_back(element_slot);
        lowered_function->local_slots.push_back(bir::LocalSlot{
            .name = element_slot,
            .type = array_type->second,
            .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
        });
      }
      local_array_slots.emplace(slot_name, std::move(array_slots));
      return true;
    }

    if (!declare_local_aggregate_slots(alloca->type_str.str(),
                                       slot_name,
                                       alloca->align > 0
                                           ? static_cast<std::size_t>(alloca->align)
                                           : 0)) {
      return fail_alloca();
    }
    return true;
  }

  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
    if (gep->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return fail_gep();
    }

    ValueMap gep_index_value_aliases = value_aliases;
    for (const auto& [name, immediate] : loaded_local_scalar_immediates) {
      gep_index_value_aliases.try_emplace(name, immediate);
    }

    std::string resolved_slot;
    if (gep->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = gep->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
        return fail_gep();
      }
      const auto resolved_address = resolve_global_gep_address(
          global_name, global_it->second.type_text, *gep, gep_index_value_aliases, type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          global_name,
          global_it->second.type_text,
          0,
          false,
          *gep,
          gep_index_value_aliases,
          type_decls);
      if (!dynamic_array.has_value()) {
        return fail_gep();
      }
      dynamic_global_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    }

    if (const auto aggregate_it = local_aggregate_slots.find(gep->ptr.str());
        aggregate_it != local_aggregate_slots.end()) {
      bool established_aggregate_subobject = false;
      const auto resolved_target = resolve_local_aggregate_gep_target(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (resolved_target.has_value()) {
        const auto target_layout =
            compute_aggregate_type_layout(resolved_target->type_text, type_decls);
        if (resolved_target->byte_offset >= 0 &&
            (target_layout.kind == AggregateTypeLayout::Kind::Struct ||
             target_layout.kind == AggregateTypeLayout::Kind::Array)) {
          LocalAggregateSlots subobject_slots{
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = resolved_target->type_text,
              .base_byte_offset = static_cast<std::size_t>(resolved_target->byte_offset),
              .leaf_slots = aggregate_it->second.leaf_slots,
          };
          std::optional<std::vector<std::string>> scalar_array_slots;
          if (target_layout.kind == AggregateTypeLayout::Kind::Array) {
            scalar_array_slots = collect_local_scalar_array_slots(
                subobject_slots.type_text, type_decls, subobject_slots);
          }
          const auto leaf_it = aggregate_it->second.leaf_slots.find(
              static_cast<std::size_t>(resolved_target->byte_offset));
          if (scalar_array_slots.has_value() && leaf_it != aggregate_it->second.leaf_slots.end()) {
            local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
                .element_slots = *scalar_array_slots,
                .base_index = 0,
            };
            auto subobject_address = LocalSlotAddress{
                .slot_name = leaf_it->second,
                .value_type = bir::TypeKind::Void,
                .byte_offset = 0,
                .storage_type_text = aggregate_it->second.storage_type_text,
                .type_text = subobject_slots.type_text,
            };
            if (const auto array_base_it = local_pointer_array_bases.find(gep->result.str());
                array_base_it != local_pointer_array_bases.end()) {
              subobject_address.array_element_slots = array_base_it->second.element_slots;
              subobject_address.array_base_index = array_base_it->second.base_index;
            }
            local_slot_pointer_values[gep->result.str()] = std::move(subobject_address);
            publish_exact_local_pointer_owner(gep->result.str(), leaf_it->second);
            return true;
          }
          local_aggregate_slots[gep->result.str()] = std::move(subobject_slots);
          if (leaf_it != aggregate_it->second.leaf_slots.end()) {
            auto subobject_address = LocalSlotAddress{
                .slot_name = leaf_it->second,
                .value_type = bir::TypeKind::Void,
                .byte_offset = 0,
                .storage_type_text = aggregate_it->second.storage_type_text,
                .type_text = local_aggregate_slots[gep->result.str()].type_text,
            };
            if (scalar_array_slots.has_value()) {
              local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
                  .element_slots = *scalar_array_slots,
                  .base_index = 0,
              };
            }
            if (const auto array_base_it = local_pointer_array_bases.find(gep->result.str());
                array_base_it != local_pointer_array_bases.end()) {
              subobject_address.array_element_slots = array_base_it->second.element_slots;
              subobject_address.array_base_index = array_base_it->second.base_index;
            }
            local_slot_pointer_values[gep->result.str()] = std::move(subobject_address);
            publish_exact_local_pointer_owner(gep->result.str(), leaf_it->second);
          }
          established_aggregate_subobject = true;
        }
      }
      const auto pointer_array_slots = resolve_local_aggregate_pointer_array_slots(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (pointer_array_slots.has_value()) {
        const auto resolved_slot = resolve_local_aggregate_gep_slot(
            aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
        if (resolved_slot.has_value()) {
          local_pointer_slots[gep->result.str()] = *resolved_slot;
          publish_exact_local_pointer_owner(gep->result.str(), *resolved_slot);
        }
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = std::move(*pointer_array_slots),
            .base_index = 0,
        };
        return true;
      }
      const auto resolved_slot = resolve_local_aggregate_gep_slot(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (resolved_slot.has_value()) {
        const auto scalar_array_slots = collect_local_scalar_array_slots(
            aggregate_it->second.type_text, type_decls, aggregate_it->second);
        if (resolved_target.has_value()) {
          const auto target_layout =
              compute_aggregate_type_layout(resolved_target->type_text, type_decls);
          const auto slot_type_it = local_slot_types.find(*resolved_slot);
          if (target_layout.kind == AggregateTypeLayout::Kind::Scalar &&
              slot_type_it != local_slot_types.end() &&
              slot_type_it->second != target_layout.scalar_type) {
            local_slot_pointer_values[gep->result.str()] = LocalSlotAddress{
                .slot_name = *resolved_slot,
                .value_type = target_layout.scalar_type,
                .byte_offset = 0,
                .storage_type_text = aggregate_it->second.storage_type_text,
                .type_text = resolved_target->type_text,
            };
            publish_exact_local_pointer_owner(gep->result.str(), *resolved_slot);
            return true;
          }
          if (scalar_array_slots.has_value() && !scalar_array_slots->empty()) {
            const auto array_layout =
                compute_aggregate_type_layout(aggregate_it->second.type_text, type_decls);
            const auto element_layout =
                compute_aggregate_type_layout(array_layout.element_type_text, type_decls);
            const auto relative_byte_offset =
                resolved_target->byte_offset -
                static_cast<std::int64_t>(aggregate_it->second.base_byte_offset);
            if (array_layout.kind == AggregateTypeLayout::Kind::Array &&
                element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
                element_layout.size_bytes != 0 && relative_byte_offset >= 0 &&
                relative_byte_offset % static_cast<std::int64_t>(element_layout.size_bytes) == 0) {
              const auto base_index = static_cast<std::size_t>(
                  relative_byte_offset / static_cast<std::int64_t>(element_layout.size_bytes));
              if (base_index < scalar_array_slots->size()) {
                local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
                    .element_slots = *scalar_array_slots,
                    .base_index = base_index,
                };
              }
            }
          }
        }
        local_pointer_slots[gep->result.str()] = *resolved_slot;
        publish_exact_local_pointer_owner(gep->result.str(), *resolved_slot);
        return true;
      }
      if (established_aggregate_subobject) {
        return true;
      }
      const auto dynamic_array = resolve_local_aggregate_dynamic_pointer_array_access(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (dynamic_array.has_value()) {
        dynamic_local_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
        return true;
      }
      const auto dynamic_aggregate = resolve_local_dynamic_aggregate_array_access(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (!dynamic_aggregate.has_value()) {
        return fail_gep();
      }
      const auto element_layout =
          compute_aggregate_type_layout(dynamic_aggregate->element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Struct ||
          element_layout.kind == AggregateTypeLayout::Kind::Array) {
        std::vector<bir::Value> element_values;
        element_values.reserve(dynamic_aggregate->element_count);
        for (std::size_t element_index = 0;
             element_index < dynamic_aggregate->element_count;
             ++element_index) {
          const std::string element_name =
              gep->result.str() + ".elt" + std::to_string(element_index);
          local_aggregate_slots[element_name] = LocalAggregateSlots{
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = dynamic_aggregate->element_type_text,
              .base_byte_offset =
                  dynamic_aggregate->byte_offset +
                  element_index * dynamic_aggregate->element_stride_bytes,
              .leaf_slots = aggregate_it->second.leaf_slots,
          };
          element_values.push_back(bir::Value::named(bir::TypeKind::Ptr, element_name));
        }
        const auto selected_value = synthesize_pointer_array_selects(
            gep->result.str(), element_values, dynamic_aggregate->index, lowered_insts);
        if (!selected_value.has_value()) {
          return fail_gep();
        }
        if (selected_value->kind != bir::Value::Kind::Named ||
            selected_value->name != gep->result.str()) {
          value_aliases[gep->result.str()] = *selected_value;
        }
      }
      dynamic_local_aggregate_arrays[gep->result.str()] = std::move(*dynamic_aggregate);
      return true;
    }

    if (const auto handled = try_lower_local_array_slot_gep(*gep,
                                                            value_aliases,
                                                            local_array_slots,
                                                            &local_pointer_slots,
                                                            &local_pointer_array_bases,
                                                            &dynamic_local_pointer_arrays);
        handled.has_value()) {
      if (!*handled) {
        return fail_gep();
      }
      if (local_pointer_slots.find(gep->result.str()) == local_pointer_slots.end()) {
        return true;
      }
      resolved_slot = local_pointer_slots[gep->result.str()];
    } else if (const auto handled = try_lower_local_pointer_array_base_gep(*gep,
                                                                           value_aliases,
                                                                           local_slot_types,
                                                                           &local_pointer_slots,
                                                                           &local_pointer_array_bases,
                                                                           &dynamic_local_pointer_arrays,
                                                                           &dynamic_local_aggregate_arrays,
                                                                           &local_slot_pointer_values);
               handled.has_value()) {
      if (!*handled) {
        return fail_gep();
      }
      if (local_pointer_slots.find(gep->result.str()) == local_pointer_slots.end()) {
        return true;
      }
      resolved_slot = local_pointer_slots[gep->result.str()];
    } else if (const auto global_ptr_it = global_pointer_slots.find(gep->ptr.str());
               global_ptr_it != global_pointer_slots.end()) {
      auto base_address = global_ptr_it->second;
      if (const auto honest_base = resolve_honest_pointer_base(base_address, global_types);
          honest_base.has_value()) {
        base_address = *honest_base;
      }
      const auto resolved_address = resolve_relative_global_gep_address(
          base_address,
          gep->element_type.str(),
          *gep,
          gep_index_value_aliases,
          type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          base_address.global_name,
          gep->element_type.str(),
          base_address.byte_offset,
          true,
          *gep,
          gep_index_value_aliases,
          type_decls);
      if (!dynamic_array.has_value()) {
        const auto dynamic_aggregate = resolve_global_dynamic_aggregate_array_access(
            base_address,
            gep->element_type.str(),
            *gep,
            gep_index_value_aliases,
            global_types,
            type_decls);
        if (dynamic_aggregate.has_value()) {
          dynamic_global_aggregate_arrays[gep->result.str()] = std::move(*dynamic_aggregate);
          return true;
        }
        if (gep->indices.size() != 1 || base_address.value_type != bir::TypeKind::Ptr) {
          return fail_gep();
        }
        const auto parsed_index = parse_typed_operand(gep->indices.front());
        if (!parsed_index.has_value()) {
          return fail_gep();
        }
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value()) {
          return fail_gep();
        }
        const auto global_it = global_types.find(base_address.global_name);
        if (global_it == global_types.end()) {
          return fail_gep();
        }
        const auto array_length = find_pointer_array_length_at_offset(
            global_it->second.type_text, base_address.byte_offset, type_decls);
        if (!array_length.has_value()) {
          return fail_gep();
        }
        dynamic_global_pointer_arrays[gep->result.str()] = DynamicGlobalPointerArrayAccess{
            .global_name = base_address.global_name,
            .byte_offset = base_address.byte_offset,
            .element_count = *array_length,
            .index = *index_value,
        };
        return true;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    } else if (const auto global_aggregate_it = dynamic_global_aggregate_arrays.find(gep->ptr.str());
               global_aggregate_it != dynamic_global_aggregate_arrays.end()) {
      if (const auto aggregate_target = resolve_relative_gep_target(
              global_aggregate_it->second.element_type_text, 0, *gep, value_aliases, type_decls);
          aggregate_target.has_value() && aggregate_target->byte_offset >= 0) {
        const auto target_layout =
            compute_aggregate_type_layout(aggregate_target->type_text, type_decls);
        if (target_layout.kind == AggregateTypeLayout::Kind::Struct ||
            target_layout.kind == AggregateTypeLayout::Kind::Array) {
          dynamic_global_aggregate_arrays[gep->result.str()] = DynamicGlobalAggregateArrayAccess{
              .global_name = global_aggregate_it->second.global_name,
              .element_type_text = std::move(aggregate_target->type_text),
              .byte_offset =
                  global_aggregate_it->second.byte_offset +
                  static_cast<std::size_t>(aggregate_target->byte_offset),
              .element_count = global_aggregate_it->second.element_count,
              .element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
              .index = global_aggregate_it->second.index,
          };
          return true;
        }
        if (target_layout.kind == AggregateTypeLayout::Kind::Scalar &&
            target_layout.scalar_type != bir::TypeKind::Void) {
          std::size_t element_count = 1;
          std::size_t element_stride_bytes = 0;
          if (const auto extent = find_repeated_aggregate_extent_at_offset(
                  global_aggregate_it->second.element_type_text,
                  static_cast<std::size_t>(aggregate_target->byte_offset),
                  aggregate_target->type_text,
                  type_decls);
              extent.has_value()) {
            element_count = extent->element_count;
            element_stride_bytes = extent->element_stride_bytes;
          }
          const auto zero_index = make_index_immediate(bir::TypeKind::I64, 0);
          if (!zero_index.has_value()) {
            return fail_gep();
          }
          dynamic_global_scalar_arrays[gep->result.str()] = DynamicGlobalScalarArrayAccess{
              .global_name = global_aggregate_it->second.global_name,
              .element_type = target_layout.scalar_type,
              .byte_offset =
                  global_aggregate_it->second.byte_offset +
                  static_cast<std::size_t>(aggregate_target->byte_offset),
              .outer_element_count = global_aggregate_it->second.element_count,
              .outer_element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
              .outer_index = global_aggregate_it->second.index,
              .element_count = element_count,
              .element_stride_bytes = element_stride_bytes,
              .index = *zero_index,
          };
          return true;
        }
      }
      const auto element_leaf = resolve_relative_global_gep_address(
          GlobalAddress{},
          global_aggregate_it->second.element_type_text,
          *gep,
          value_aliases,
          type_decls);
      if (!element_leaf.has_value() || element_leaf->value_type != bir::TypeKind::Ptr) {
        return fail_gep();
      }
      dynamic_global_pointer_arrays[gep->result.str()] = DynamicGlobalPointerArrayAccess{
          .global_name = global_aggregate_it->second.global_name,
          .byte_offset = global_aggregate_it->second.byte_offset + element_leaf->byte_offset,
          .element_count = global_aggregate_it->second.element_count,
          .element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
          .index = global_aggregate_it->second.index,
      };
      return true;
    } else if (const auto global_scalar_it = dynamic_global_scalar_arrays.find(gep->ptr.str());
               global_scalar_it != dynamic_global_scalar_arrays.end()) {
      if (gep->indices.empty() || gep->indices.size() > 2 ||
          global_scalar_it->second.element_count == 0 ||
          global_scalar_it->second.element_stride_bytes == 0) {
        return fail_gep();
      }
      std::size_t index_pos = 0;
      if (gep->indices.size() == 2) {
        const auto base_index = parse_typed_operand(gep->indices.front());
        if (!base_index.has_value()) {
          return fail_gep();
        }
        const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
        if (!base_imm.has_value() || *base_imm != 0) {
          return fail_gep();
        }
        index_pos = 1;
      }
      const auto parsed_index = parse_typed_operand(gep->indices[index_pos]);
      if (!parsed_index.has_value()) {
        return fail_gep();
      }
      const auto zero_index = make_index_immediate(bir::TypeKind::I64, 0);
      if (!zero_index.has_value()) {
        return fail_gep();
      }
      if (const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
          index_imm.has_value()) {
        if (*index_imm < 0 ||
            static_cast<std::size_t>(*index_imm) >= global_scalar_it->second.element_count) {
          return fail_gep();
        }
        dynamic_global_scalar_arrays[gep->result.str()] = DynamicGlobalScalarArrayAccess{
            .global_name = global_scalar_it->second.global_name,
            .element_type = global_scalar_it->second.element_type,
            .byte_offset =
                global_scalar_it->second.byte_offset +
                static_cast<std::size_t>(*index_imm) *
                    global_scalar_it->second.element_stride_bytes,
            .outer_element_count = global_scalar_it->second.outer_element_count,
            .outer_element_stride_bytes = global_scalar_it->second.outer_element_stride_bytes,
            .outer_index = global_scalar_it->second.outer_index,
            .element_count =
                global_scalar_it->second.element_count - static_cast<std::size_t>(*index_imm),
            .element_stride_bytes = global_scalar_it->second.element_stride_bytes,
            .index = *zero_index,
        };
        return true;
      }
      const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
      if (!index_value.has_value()) {
        return fail_gep();
      }
      dynamic_global_scalar_arrays[gep->result.str()] = DynamicGlobalScalarArrayAccess{
          .global_name = global_scalar_it->second.global_name,
          .element_type = global_scalar_it->second.element_type,
          .byte_offset = global_scalar_it->second.byte_offset,
          .outer_element_count = global_scalar_it->second.outer_element_count,
          .outer_element_stride_bytes = global_scalar_it->second.outer_element_stride_bytes,
          .outer_index = global_scalar_it->second.outer_index,
          .element_count = global_scalar_it->second.element_count,
          .element_stride_bytes = global_scalar_it->second.element_stride_bytes,
          .index = *index_value,
      };
      return true;
    } else if (const auto handled_dynamic_local_aggregate_gep =
                   try_lower_dynamic_local_aggregate_gep_projection(*gep,
                                                                    dynamic_local_aggregate_arrays,
                                                                    value_aliases,
                                                                    type_decls,
                                                                    &dynamic_local_pointer_arrays);
               handled_dynamic_local_aggregate_gep.has_value()) {
      if (!*handled_dynamic_local_aggregate_gep) {
        return fail_gep();
      }
      return true;
    } else if (const auto handled_local_slot_gep =
                   try_lower_local_slot_pointer_gep(
                       *gep, value_aliases, type_decls, local_slot_types, &local_slot_pointer_values);
               handled_local_slot_gep.has_value()) {
      if (!*handled_local_slot_gep) {
        return fail_gep();
      }
      return true;
    } else {
      const auto addressed_ptr_it = pointer_value_addresses.find(gep->ptr.str());
      const auto ptr_it = local_pointer_slots.find(gep->ptr.str());
      if (addressed_ptr_it != pointer_value_addresses.end() || ptr_it == local_pointer_slots.end()) {
        const auto base_pointer = addressed_ptr_it != pointer_value_addresses.end()
                                      ? std::optional<bir::Value>(addressed_ptr_it->second.base_value)
                                      : lower_value(gep->ptr, bir::TypeKind::Ptr, value_aliases);
        if (base_pointer.has_value()) {
          const auto base_byte_offset = addressed_ptr_it != pointer_value_addresses.end()
                                            ? addressed_ptr_it->second.byte_offset
                                            : 0;
          auto addressed_base_type_text =
              addressed_ptr_it != pointer_value_addresses.end()
                  ? addressed_ptr_it->second.type_text
                  : std::string{};
          if (addressed_base_type_text.empty() &&
              base_pointer->kind == bir::Value::Kind::Named &&
              base_pointer->type == bir::TypeKind::Ptr) {
            if (const auto aggregate_param_it = aggregate_params.find(base_pointer->name);
                aggregate_param_it != aggregate_params.end()) {
              addressed_base_type_text = aggregate_param_it->second.type_text;
            }
          }
          const auto resolved_target = resolve_relative_gep_target(
              !addressed_base_type_text.empty()
                  ? addressed_base_type_text
                  : std::string(c4c::codegen::lir::trim_lir_arg_text(gep->element_type.str())),
              base_byte_offset,
              *gep,
              value_aliases,
              type_decls);
          if (resolved_target.has_value() && resolved_target->byte_offset >= 0) {
            const auto leaf_layout =
                compute_aggregate_type_layout(resolved_target->type_text, type_decls);
            auto storage_type_text =
                addressed_ptr_it != pointer_value_addresses.end()
                    ? addressed_ptr_it->second.storage_type_text
                    : std::string(c4c::codegen::lir::trim_lir_arg_text(gep->element_type.str()));
            if (leaf_layout.kind == AggregateTypeLayout::Kind::Struct ||
                leaf_layout.kind == AggregateTypeLayout::Kind::Array) {
              storage_type_text = resolved_target->type_text;
            }
            pointer_value_addresses[gep->result.str()] = PointerAddress{
                .base_value = *base_pointer,
                .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                                  ? leaf_layout.scalar_type
                                  : bir::TypeKind::Void,
                .byte_offset = static_cast<std::size_t>(resolved_target->byte_offset),
                .storage_type_text = std::move(storage_type_text),
                .type_text = std::move(resolved_target->type_text),
            };
            if (resolved_target->byte_offset == base_byte_offset) {
              value_aliases[gep->result.str()] = *base_pointer;
            }
            return true;
          }

          if (addressed_ptr_it != pointer_value_addresses.end() &&
              (gep->indices.size() == 1 || gep->indices.size() == 2) &&
              ((addressed_ptr_it->second.dynamic_element_count != 0 &&
                addressed_ptr_it->second.dynamic_element_stride_bytes != 0) ||
               (!addressed_ptr_it->second.storage_type_text.empty() &&
                !addressed_ptr_it->second.type_text.empty()))) {
            std::size_t dynamic_index_pos = 0;
            if (gep->indices.size() == 2) {
              const auto base_index = parse_typed_operand(gep->indices.front());
              if (!base_index.has_value()) {
                return fail_gep();
              }
              const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
              if (!base_imm.has_value() || *base_imm != 0) {
                return fail_gep();
              }
              dynamic_index_pos = 1;
            }
            const auto parsed_index = parse_typed_operand(gep->indices[dynamic_index_pos]);
            if (parsed_index.has_value() &&
                !resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
              const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
              if (lowered_index.has_value()) {
                const auto dynamic_element_type =
                    lower_scalar_or_function_pointer_type(gep->element_type.str());
                if (addressed_ptr_it->second.dynamic_element_count != 0 &&
                    addressed_ptr_it->second.dynamic_element_stride_bytes != 0 &&
                    dynamic_element_type.has_value() &&
                    type_size_bytes(*dynamic_element_type) ==
                        addressed_ptr_it->second.dynamic_element_stride_bytes) {
                  dynamic_pointer_value_arrays[gep->result.str()] = DynamicPointerValueArrayAccess{
                      .base_value = addressed_ptr_it->second.base_value,
                      .element_type = *dynamic_element_type,
                      .byte_offset = addressed_ptr_it->second.byte_offset,
                      .element_count = addressed_ptr_it->second.dynamic_element_count,
                      .element_stride_bytes =
                          addressed_ptr_it->second.dynamic_element_stride_bytes,
                      .index = *lowered_index,
                  };
                  return true;
                }
                const auto element_layout = compute_aggregate_type_layout(
                    addressed_ptr_it->second.type_text, type_decls);
                if (element_layout.kind == AggregateTypeLayout::Kind::Array) {
                  const auto array_element_layout = compute_aggregate_type_layout(
                      element_layout.element_type_text, type_decls);
                  if (array_element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
                      array_element_layout.scalar_type != bir::TypeKind::Void &&
                      array_element_layout.size_bytes != 0) {
                    dynamic_pointer_value_arrays[gep->result.str()] = DynamicPointerValueArrayAccess{
                        .base_value = addressed_ptr_it->second.base_value,
                        .element_type = array_element_layout.scalar_type,
                        .byte_offset = addressed_ptr_it->second.byte_offset,
                        .element_count = element_layout.array_count,
                        .element_stride_bytes = array_element_layout.size_bytes,
                        .index = *lowered_index,
                    };
                    return true;
                  }
                }
                const auto extent = find_repeated_aggregate_extent_at_offset(
                    addressed_ptr_it->second.storage_type_text,
                    addressed_ptr_it->second.byte_offset,
                    addressed_ptr_it->second.type_text,
                    type_decls);
                if (extent.has_value() &&
                    element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
                    element_layout.scalar_type != bir::TypeKind::Void) {
                  dynamic_pointer_value_arrays[gep->result.str()] = DynamicPointerValueArrayAccess{
                      .base_value = addressed_ptr_it->second.base_value,
                      .element_type = element_layout.scalar_type,
                      .byte_offset = addressed_ptr_it->second.byte_offset,
                      .element_count = extent->element_count,
                      .element_stride_bytes = extent->element_stride_bytes,
                      .index = *lowered_index,
                  };
                  return true;
                }
              }
            }
          }

          std::size_t raw_index_pos = 0;
          if (gep->indices.size() == 2) {
            const auto base_index = parse_typed_operand(gep->indices.front());
            if (!base_index.has_value()) {
              return fail_gep();
            }
            const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
            if (!base_imm.has_value() || *base_imm != 0) {
              return fail_gep();
            }
            raw_index_pos = 1;
          }
          if (addressed_ptr_it == pointer_value_addresses.end() &&
              (gep->indices.size() == 1 || gep->indices.size() == 2) &&
              c4c::codegen::lir::trim_lir_arg_text(gep->element_type.str()) == "i8") {
            const auto parsed_index = parse_typed_operand(gep->indices[raw_index_pos]);
            if (!parsed_index.has_value()) {
              return fail_gep();
            }
            auto raw_offset = lower_typed_index_value(*parsed_index, value_aliases);
            if (!raw_offset.has_value()) {
              return fail_gep();
            }
            if (raw_offset->kind == bir::Value::Kind::Immediate &&
                raw_offset->type != bir::TypeKind::I64) {
              raw_offset = bir::Value::immediate_i64(raw_offset->immediate);
            }
            if (raw_offset->kind == bir::Value::Kind::Named &&
                raw_offset->type != bir::TypeKind::I64) {
              return fail_gep();
            }
            if (raw_offset->kind == bir::Value::Kind::Immediate && raw_offset->immediate == 0) {
              value_aliases[gep->result.str()] = *base_pointer;
              return true;
            }
            lowered_insts->push_back(bir::BinaryInst{
                .opcode = bir::BinaryOpcode::Add,
                .result = bir::Value::named(bir::TypeKind::Ptr, gep->result.str()),
                .operand_type = bir::TypeKind::Ptr,
                .lhs = *base_pointer,
                .rhs = *raw_offset,
            });
            value_aliases[gep->result.str()] = bir::Value::named(bir::TypeKind::Ptr,
                                                                 gep->result.str());
            return true;
          }
        }
      }
      const auto handled_local_pointer_slot_base_gep = try_lower_local_pointer_slot_base_gep(
          *gep,
          value_aliases,
          type_decls,
          local_slot_types,
          local_array_slots,
          local_aggregate_slots,
          &local_pointer_slots,
          &local_pointer_array_bases,
          &dynamic_local_pointer_arrays,
          &dynamic_local_aggregate_arrays);
      if (!handled_local_pointer_slot_base_gep.has_value() || !*handled_local_pointer_slot_base_gep) {
        return fail_gep();
      }
      return true;
    }

    if (!resolved_slot.empty()) {
      local_pointer_slots[gep->result.str()] = std::move(resolved_slot);
      return true;
    }
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    if (store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
        store->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
      return fail_store();
    }

    const auto value_type = lower_scalar_or_function_pointer_type(store->type_str.str());
    if (!value_type.has_value()) {
      const auto aggregate_layout = lower_byval_aggregate_layout(store->type_str.str(), type_decls);
      if (!aggregate_layout.has_value() ||
          store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
          store->val.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return fail_store();
      }

      const auto target_aggregate_it = local_aggregate_slots.find(store->ptr.str());
      if (target_aggregate_it == local_aggregate_slots.end()) {
        return fail_store();
      }

      if (const auto source_param_it = aggregate_params.find(store->val.str());
          source_param_it != aggregate_params.end()) {
        clear_local_scalar_slot_values();
        const auto leaf_slots = collect_sorted_leaf_slots(target_aggregate_it->second);
        for (const auto& [byte_offset, slot_name] : leaf_slots) {
          const auto slot_type_it = local_slot_types.find(slot_name);
          if (slot_type_it == local_slot_types.end()) {
            return fail_store();
          }
          const auto slot_size = type_size_bytes(slot_type_it->second);
          if (slot_size == 0) {
            return fail_store();
          }

          const std::string temp_name =
              store->ptr.str() + ".byval.copy." + std::to_string(byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(slot_type_it->second, temp_name),
              .slot_name = slot_name,
              .address =
                  bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                      .base_value = bir::Value::named(bir::TypeKind::Ptr, store->val.str()),
                      .byte_offset = static_cast<std::int64_t>(byte_offset),
                      .size_bytes = slot_size,
                      .align_bytes = std::max(slot_size, source_param_it->second.layout.align_bytes),
                  },
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = slot_name,
              .value = bir::Value::named(slot_type_it->second, temp_name),
          });
        }
        return true;
      }

      const auto source_alias_it = aggregate_value_aliases.find(store->val.str());
      if (source_alias_it == aggregate_value_aliases.end()) {
        return fail_store();
      }
      clear_local_scalar_slot_values();
      const auto source_aggregate_it = local_aggregate_slots.find(source_alias_it->second);
      if (source_aggregate_it == local_aggregate_slots.end()) {
        return fail_store();
      }
      if (!append_local_aggregate_copy_from_slots(source_aggregate_it->second,
                                                  target_aggregate_it->second,
                                                  store->ptr.str() + ".aggregate.copy",
                                                  lowered_insts)) {
        return fail_store();
      }
      return true;
    }

    std::optional<bir::Value> value;
    if (*value_type == bir::TypeKind::Ptr &&
        store->val.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = store->val.str().substr(1);
      if (is_known_function_symbol(global_name, function_symbols)) {
        value = bir::Value::named(bir::TypeKind::Ptr, "@" + global_name);
      }
    }
    if (!value.has_value()) {
      value = lower_value(store->val, *value_type, value_aliases);
    }
    if (!value.has_value()) {
      return fail_store();
    }

    if (const auto pointer_store = try_lower_pointer_provenance_store(store->ptr.str(),
                                                                      *value_type,
                                                                      *value,
                                                                      type_decls,
                                                                      local_slot_types,
                                                                      local_slot_pointer_values,
                                                                      pointer_value_addresses,
                                                                      lowered_insts);
        pointer_store.has_value()) {
      clear_local_scalar_slot_values();
      return *pointer_store ? true : fail_store();
    }

    if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays.find(store->ptr.str());
        dynamic_ptr_it != dynamic_pointer_value_arrays.end()) {
      clear_local_scalar_slot_values();
      if (!append_dynamic_pointer_value_array_store(
              store->ptr.str(), *value_type, *value, dynamic_ptr_it->second, lowered_insts)) {
        return fail_store();
      }
      return true;
    }

    bool handled_dynamic_local_aggregate_store = false;
    if (!try_lower_dynamic_local_aggregate_store(store->ptr.str(),
                                                 *value_type,
                                                 *value,
                                                 dynamic_local_aggregate_arrays,
                                                 type_decls,
                                                 local_slot_types,
                                                 lowered_insts,
                                                 &handled_dynamic_local_aggregate_store)) {
        return fail_store();
      }
    if (handled_dynamic_local_aggregate_store) {
      clear_local_scalar_slot_values();
      return true;
    }

    if (const auto global_store = try_lower_global_provenance_store(*store,
                                                                    *value_type,
                                                                    *value,
                                                                    type_decls,
                                                                    global_types,
                                                                    function_symbols,
                                                                    global_pointer_slots,
                                                                    global_object_pointer_slots,
                                                                    pointer_value_addresses,
                                                                    &global_address_slots,
                                                                    &addressed_global_pointer_slots_,
                                                                    &global_pointer_value_slots,
                                                                    &addressed_global_pointer_value_slots,
                                                                    lowered_insts);
        global_store.has_value()) {
      clear_local_scalar_slot_values();
      return *global_store ? true : fail_store();
    }

    const auto local_slot_store = try_lower_local_slot_store(store->ptr.str(),
                                                             store->val,
                                                             *value_type,
                                                             *value,
                                                             value_aliases,
                                                             type_decls,
                                                             global_types,
                                                             function_symbols,
                                                             local_pointer_slots,
                                                             local_slot_types,
                                                             local_aggregate_field_slots,
                                                             local_array_slots,
                                                             local_aggregate_slots,
                                                             local_pointer_array_bases,
                                                             local_slot_pointer_values,
                                                             pointer_value_addresses,
                                                             global_pointer_slots,
                                                             global_address_ints,
                                                             &local_pointer_value_aliases,
                                                             &local_indirect_pointer_slots,
                                                             &local_pointer_slot_addresses,
                                                             &local_slot_address_slots,
                                                             &local_address_slots,
                                                             lowered_insts);
    if (local_slot_store == LocalSlotStoreResult::NotHandled) {
      return fail_store();
    }
    if (local_slot_store == LocalSlotStoreResult::Lowered) {
      const auto ptr_it = local_pointer_slots.find(store->ptr.str());
      if (ptr_it != local_pointer_slots.end() && *value_type != bir::TypeKind::Ptr &&
          value->kind == bir::Value::Kind::Immediate) {
        local_scalar_slot_values[ptr_it->second] = *value;
      } else {
        erase_local_scalar_slot_value(store->ptr.str());
      }
      return true;
    }
    erase_local_scalar_slot_value(store->ptr.str());
    return fail_store();
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    if (load->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         load->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return fail_load();
    }

    const auto value_type = lower_scalar_or_function_pointer_type(load->type_str.str());
    if (!value_type.has_value()) {
      const auto aggregate_layout = lower_byval_aggregate_layout(load->type_str.str(), type_decls);
      if (!aggregate_layout.has_value()) {
        return fail_load();
      }
      if (load->ptr.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        if (local_aggregate_slots.find(load->ptr.str()) == local_aggregate_slots.end()) {
          return fail_load();
        }
        aggregate_value_aliases[load->result.str()] = load->ptr.str();
        return true;
      }
      if (load->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
        return fail_load();
      }

      const std::string global_name = load->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing ||
          global_it->second.storage_size_bytes < aggregate_layout->size_bytes) {
        return fail_load();
      }

      if (!declare_local_aggregate_slots(
              load->type_str.str(), load->result.str(), aggregate_layout->align_bytes)) {
        return fail_load();
      }
      const auto aggregate_it = local_aggregate_slots.find(load->result.str());
      if (aggregate_it == local_aggregate_slots.end()) {
        return fail_load();
      }

      const auto leaf_slots = collect_sorted_leaf_slots(aggregate_it->second);
      for (const auto& [byte_offset, slot_name] : leaf_slots) {
        const auto slot_type_it = local_slot_types.find(slot_name);
        if (slot_type_it == local_slot_types.end()) {
          return fail_load();
        }
        const auto slot_size = type_size_bytes(slot_type_it->second);
        if (slot_size == 0) {
          return fail_load();
        }

        const std::string temp_name =
            load->result.str() + ".global.aggregate.load." + std::to_string(byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(slot_type_it->second, temp_name),
            .slot_name = slot_name,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                    .base_name = global_name,
                    .byte_offset = static_cast<std::int64_t>(byte_offset),
                    .size_bytes = slot_size,
                    .align_bytes = std::max(slot_size, aggregate_layout->align_bytes),
                },
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = slot_name,
            .value = bir::Value::named(slot_type_it->second, temp_name),
        });
      }
      aggregate_value_aliases[load->result.str()] = load->result.str();
      return true;
    }

    loaded_local_scalar_immediates.erase(load->result.str());
    if (load->ptr.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        *value_type != bir::TypeKind::Ptr) {
      const auto ptr_it = local_pointer_slots.find(load->ptr.str());
      if (ptr_it != local_pointer_slots.end()) {
        const auto slot_value_it = local_scalar_slot_values.find(ptr_it->second);
        if (slot_value_it != local_scalar_slot_values.end() &&
            slot_value_it->second.type == *value_type) {
          loaded_local_scalar_immediates[load->result.str()] = slot_value_it->second;
        }
      }
    }

    if (const auto global_load = try_lower_global_provenance_load(*load,
                                                                  *value_type,
                                                                  global_types,
                                                                  type_decls,
                                                                  global_address_slots,
                                                                  addressed_global_pointer_slots_,
                                                                  global_pointer_value_slots,
                                                                  addressed_global_pointer_value_slots,
                                                                  &global_pointer_slots,
                                                                  &global_object_pointer_slots,
                                                                  &pointer_value_addresses,
                                                                  lowered_insts);
        global_load.has_value()) {
      return *global_load ? true : fail_load();
    }

    if (const auto pointer_load = try_lower_pointer_provenance_load(load->result.str(),
                                                                    load->ptr.str(),
                                                                    *value_type,
                                                                    type_decls,
                                                                    local_slot_types,
                                                                    local_indirect_pointer_slots,
                                                                    local_address_slots,
                                                                    local_slot_address_slots,
                                                                    global_types,
                                                                    function_symbols,
                                                                    &value_aliases,
                                                                    &local_slot_pointer_values,
                                                                    &global_pointer_slots,
                                                                    pointer_value_addresses,
                                                                    lowered_insts);
        pointer_load.has_value()) {
      return *pointer_load ? true : fail_load();
    }

    if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays.find(load->ptr.str());
        dynamic_ptr_it != dynamic_pointer_value_arrays.end()) {
      const auto selected_value = load_dynamic_pointer_value_array_value(
          load->result.str(), *value_type, dynamic_ptr_it->second, lowered_insts);
      if (!selected_value.has_value()) {
        return fail_load();
      }
      if (selected_value->kind == bir::Value::Kind::Named &&
          selected_value->name == load->result.str()) {
        return true;
      }
      value_aliases[load->result.str()] = *selected_value;
      return true;
    }

    if (const auto global_scalar_it = dynamic_global_scalar_arrays.find(load->ptr.str());
        global_scalar_it != dynamic_global_scalar_arrays.end()) {
      const auto selected_value = load_dynamic_global_scalar_array_value(
          load->result.str(), *value_type, global_scalar_it->second, lowered_insts);
      if (!selected_value.has_value()) {
        return fail_load();
      }
      if (selected_value->kind == bir::Value::Kind::Named &&
          selected_value->name == load->result.str()) {
        return true;
      }
      value_aliases[load->result.str()] = *selected_value;
      return true;
    }

    bool handled_dynamic_local_aggregate_load = false;
    if (!try_lower_dynamic_local_aggregate_load(load->result.str(),
                                                load->ptr.str(),
                                                *value_type,
                                                dynamic_local_aggregate_arrays,
                                                type_decls,
                                                local_slot_types,
                                                &value_aliases,
                                                lowered_insts,
                                                &handled_dynamic_local_aggregate_load)) {
        return fail_load();
      }
    if (handled_dynamic_local_aggregate_load) {
      return true;
    }

    const auto local_slot_load = try_lower_local_slot_load(load->result.str(),
                                                           load->ptr.str(),
                                                           *value_type,
                                                           local_pointer_slots,
                                                           local_slot_types,
                                                           local_aggregate_field_slots,
                                                           local_array_slots,
                                                           local_pointer_value_aliases,
                                                           type_decls,
                                                           local_indirect_pointer_slots,
                                                           local_address_slots,
                                                           local_slot_address_slots,
                                                           local_pointer_slot_addresses,
                                                           global_types,
                                                           function_symbols,
                                                           &value_aliases,
                                                           &local_slot_pointer_values,
                                                           &local_aggregate_slots,
                                                           &local_pointer_array_bases,
                                                           &global_pointer_slots,
                                                           &pointer_value_addresses,
                                                           &global_address_ints,
                                                           lowered_insts);
    if (local_slot_load == LocalSlotLoadResult::NotHandled) {
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto dynamic_pointer_array_load = try_lower_dynamic_pointer_array_load(
                load->result.str(),
                load->ptr.str(),
                dynamic_local_pointer_arrays,
                dynamic_global_pointer_arrays,
                local_pointer_value_aliases,
                global_types,
                &value_aliases,
                lowered_insts);
            dynamic_pointer_array_load.has_value()) {
          return *dynamic_pointer_array_load ? true : fail_load();
        }
      }
      return fail_load();
    }
    return local_slot_load == LocalSlotLoadResult::Lowered ? true : fail_load();
  }

  if (const auto* memcpy = std::get_if<c4c::codegen::lir::LirMemcpyOp>(&inst)) {
    if (memcpy->dst.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        memcpy->src.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        memcpy->is_volatile) {
      return false;
    }

    const auto copy_size = lower_value(memcpy->size, bir::TypeKind::I64, value_aliases);
    if (!copy_size.has_value() || copy_size->kind != bir::Value::Kind::Immediate ||
        copy_size->immediate < 0) {
      return false;
    }
    const auto requested_size = static_cast<std::size_t>(copy_size->immediate);

    return try_lower_immediate_local_memcpy(memcpy->dst.str(),
                                            memcpy->src.str(),
                                            requested_size,
                                            *lowered_function,
                                            type_decls,
                                            local_slot_types,
                                            local_pointer_slots,
                                            local_array_slots,
                                            local_pointer_array_bases,
                                            local_aggregate_slots,
                                            lowered_insts);
  }

  if (const auto* memset = std::get_if<c4c::codegen::lir::LirMemsetOp>(&inst)) {
    if (memset->dst.kind() != c4c::codegen::lir::LirOperandKind::SsaValue || memset->is_volatile) {
      return false;
    }
    const auto fill_value = lower_value(memset->byte_val, bir::TypeKind::I8, value_aliases);
    const auto fill_size = lower_value(memset->size, bir::TypeKind::I64, value_aliases);
    if (!fill_value.has_value() || fill_value->kind != bir::Value::Kind::Immediate ||
        !fill_size.has_value() ||
        fill_size->kind != bir::Value::Kind::Immediate || fill_size->immediate < 0) {
      return false;
    }

    return try_lower_immediate_local_memset(memset->dst.str(),
                                            static_cast<std::uint8_t>(fill_value->immediate & 0xff),
                                            static_cast<std::size_t>(fill_size->immediate),
                                            type_decls,
                                            local_slot_types,
                                            local_pointer_slots,
                                            local_array_slots,
                                            local_pointer_array_bases,
                                            local_aggregate_slots,
                                            lowered_insts);
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    constexpr std::string_view kDirectCallFamily = "direct-call semantic family";
    constexpr std::string_view kIndirectCallFamily = "indirect-call semantic family";
    constexpr std::string_view kCallReturnFamily = "call-return semantic family";
    const auto fail_call_family = [&](std::string_view family) -> bool {
      note_semantic_call_family_failure(family);
      return false;
    };

    const auto return_info =
        lower_return_info_from_type(call->return_type.str(), type_decls, context_.target_profile);
    if (!return_info.has_value()) {
      return fail_call_family(kCallReturnFamily);
    }

    std::vector<bir::Value> lowered_args;
    std::vector<bir::TypeKind> lowered_arg_types;
    std::vector<bir::CallArgAbiInfo> lowered_arg_abi;
    std::optional<std::string> callee_name;
    std::optional<bir::Value> callee_value;
    std::optional<PointerAddress> returned_pointer_address;
    bool is_indirect_call = false;
    bool is_variadic_call = false;
    std::optional<std::string> sret_slot_name;
    const auto lower_public_pointer_call_arg_value =
        [&](const c4c::codegen::lir::LirOperand& operand) -> std::optional<bir::Value> {
      if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        if (const auto resolved_address = resolve_runtime_pointer_address(operand.str());
            resolved_address.has_value() && resolved_address->byte_offset == 0 &&
            resolved_address->base_value.kind == bir::Value::Kind::Named &&
            resolved_address->base_value.type == bir::TypeKind::Ptr) {
          return resolved_address->base_value;
        }
      }
      return lower_call_pointer_arg_value(
          operand, value_aliases, local_aggregate_slots, global_types, function_symbols);
    };
    const auto lower_byval_call_arg_value =
        [&](const c4c::codegen::lir::LirOperand& operand,
            const AggregateTypeLayout& aggregate_layout) -> std::optional<bir::Value> {
      if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto aggregate_alias_it = aggregate_value_aliases.find(operand.str());
        if (aggregate_alias_it == aggregate_value_aliases.end() ||
            local_aggregate_slots.find(aggregate_alias_it->second) ==
                local_aggregate_slots.end()) {
          return std::nullopt;
        }
        return bir::Value::named(bir::TypeKind::Ptr, aggregate_alias_it->second);
      }

      if (operand.kind() != c4c::codegen::lir::LirOperandKind::Global) {
        return std::nullopt;
      }

      const std::string global_name = operand.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing ||
          global_it->second.storage_size_bytes < aggregate_layout.size_bytes) {
        return std::nullopt;
      }
      return bir::Value::named(bir::TypeKind::Ptr, "@" + global_name);
    };

    const auto try_lower_direct_memcpy_call =
        [&](std::string_view symbol_name,
            const ParsedTypedCall& typed_call) -> std::optional<bool> {
      if (symbol_name != "memcpy") {
        return std::nullopt;
      }
      const auto fail_memcpy_family = [&]() -> std::optional<bool> {
        note_runtime_intrinsic_family_failure("memcpy runtime family");
        return false;
      };
      if (typed_call.args.size() != 3 || typed_call.param_types.size() != 3 ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[2]) != "i64") {
        return fail_memcpy_family();
      }

      const auto copy_size =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[2].operand)),
                      bir::TypeKind::I64,
                      value_aliases);
      if (!copy_size.has_value() || copy_size->kind != bir::Value::Kind::Immediate ||
          copy_size->immediate < 0) {
        return fail_memcpy_family();
      }
      const auto requested_size = static_cast<std::size_t>(copy_size->immediate);

      const std::string dst_operand(typed_call.args[0].operand);
      const std::string src_operand(typed_call.args[1].operand);
      const auto alias_memcpy_result = [&]() -> bool {
        if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
          if (return_info->returned_via_sret || return_info->type != bir::TypeKind::Ptr) {
            note_runtime_intrinsic_family_failure("memcpy runtime family");
            return false;
          }
          value_aliases[call->result.str()] = bir::Value::named(bir::TypeKind::Ptr, dst_operand);
          return true;
        }
        if (return_info->type != bir::TypeKind::Void) {
          note_runtime_intrinsic_family_failure("memcpy runtime family");
          return false;
        }
        return true;
      };
      if (!try_lower_immediate_local_memcpy(dst_operand,
                                            src_operand,
                                            requested_size,
                                            *lowered_function,
                                            type_decls,
                                            local_slot_types,
                                            local_pointer_slots,
                                            local_array_slots,
                                            local_pointer_array_bases,
                                            local_aggregate_slots,
                                            lowered_insts)) {
        return fail_memcpy_family();
      }
      return alias_memcpy_result();
    };
    const auto try_lower_direct_memset_call =
        [&](std::string_view symbol_name,
            const ParsedTypedCall& typed_call) -> std::optional<bool> {
      if (symbol_name != "memset") {
        return std::nullopt;
      }
      const auto fail_memset_family = [&]() -> std::optional<bool> {
        note_runtime_intrinsic_family_failure("memset runtime family");
        return false;
      };
      if (typed_call.args.size() != 3 || typed_call.param_types.size() != 3 ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[2]) != "i64") {
        return fail_memset_family();
      }

      const auto fill_type =
          lower_integer_type(c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]));
      if (!fill_type.has_value()) {
        return fail_memset_family();
      }

      const auto fill_value =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[1].operand)),
                      *fill_type,
                      value_aliases);
      const auto fill_size =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[2].operand)),
                      bir::TypeKind::I64,
                      value_aliases);
      if (!fill_value.has_value() || fill_value->kind != bir::Value::Kind::Immediate ||
          !fill_size.has_value() ||
          fill_size->kind != bir::Value::Kind::Immediate || fill_size->immediate < 0) {
        return fail_memset_family();
      }

      const std::string dst_operand(typed_call.args[0].operand);
      if (!try_lower_immediate_local_memset(dst_operand,
                                            static_cast<std::uint8_t>(fill_value->immediate & 0xff),
                                            static_cast<std::size_t>(fill_size->immediate),
                                            type_decls,
                                            local_slot_types,
                                            local_pointer_slots,
                                            local_array_slots,
                                            local_pointer_array_bases,
                                            local_aggregate_slots,
                                            lowered_insts)) {
        return fail_memset_family();
      }
      if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        if (return_info->returned_via_sret || return_info->type != bir::TypeKind::Ptr) {
          return fail_memset_family();
        }
        value_aliases[call->result.str()] = bir::Value::named(bir::TypeKind::Ptr, dst_operand);
        return true;
      }
      if (return_info->type != bir::TypeKind::Void) {
        return fail_memset_family();
      }
      return true;
    };
    const auto maybe_resolve_direct_calloc_pointer_address =
        [&](std::string_view symbol_name,
            const ParsedTypedCall& typed_call) -> std::optional<PointerAddress> {
      if (symbol_name != "calloc" || return_info->returned_via_sret ||
          return_info->type != bir::TypeKind::Ptr ||
          call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
          typed_call.args.size() != 2 || typed_call.param_types.size() != 2) {
        return std::nullopt;
      }
      const auto count_type =
          lower_integer_type(c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]));
      const auto size_type =
          lower_integer_type(c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]));
      if (!count_type.has_value() || !size_type.has_value()) {
        return std::nullopt;
      }
      const auto count_value =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[0].operand)),
                      *count_type,
                      value_aliases);
      const auto size_value =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[1].operand)),
                      *size_type,
                      value_aliases);
      if (!count_value.has_value() || count_value->kind != bir::Value::Kind::Immediate ||
          !size_value.has_value() || size_value->kind != bir::Value::Kind::Immediate ||
          count_value->immediate <= 0 || size_value->immediate <= 0) {
        return std::nullopt;
      }
      return PointerAddress{
          .base_value = bir::Value::named(bir::TypeKind::Ptr, call->result.str()),
          .dynamic_element_count = static_cast<std::size_t>(count_value->immediate),
          .dynamic_element_stride_bytes = static_cast<std::size_t>(size_value->immediate),
      };
    };

    if (return_info->returned_via_sret) {
      if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return fail_call_family(kCallReturnFamily);
      }
      sret_slot_name = call->result.str();
      if (!declare_local_aggregate_slots(call->return_type.str(),
                                         *sret_slot_name,
                                         return_info->align_bytes)) {
        return fail_call_family(kCallReturnFamily);
      }
      aggregate_value_aliases[*sret_slot_name] = *sret_slot_name;
      lowered_arg_types.push_back(bir::TypeKind::Ptr);
      lowered_args.push_back(bir::Value::named(bir::TypeKind::Ptr, *sret_slot_name));
      lowered_arg_abi.push_back(bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = return_info->size_bytes,
          .align_bytes = return_info->align_bytes,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      });
    }

    std::string_view call_family = kCallReturnFamily;
    if (const auto direct_callee = c4c::codegen::lir::parse_lir_direct_global_callee(call->callee);
        direct_callee.has_value()) {
      call_family = kDirectCallFamily;
      const auto resolved_direct_callee_name =
          [&](std::string_view fallback_name) -> std::string {
        if (call->direct_callee_link_name_id != c4c::kInvalidLinkName) {
          const std::string_view semantic_name =
              context_.lir_module.link_names.spelling(call->direct_callee_link_name_id);
          if (!semantic_name.empty()) {
            return std::string(semantic_name);
          }
        }
        return std::string(fallback_name);
      };
      if (const auto inferred_call = parse_typed_call(*call); inferred_call.has_value()) {
        const std::string semantic_direct_callee =
            resolved_direct_callee_name(*direct_callee);
        if (const auto lowered_memset =
                try_lower_direct_memset_call(semantic_direct_callee, *inferred_call);
            lowered_memset.has_value()) {
          return *lowered_memset;
        }
        if (const auto lowered_memcpy =
                try_lower_direct_memcpy_call(semantic_direct_callee, *inferred_call);
            lowered_memcpy.has_value()) {
          return *lowered_memcpy;
        }
        if (!returned_pointer_address.has_value()) {
          returned_pointer_address =
              maybe_resolve_direct_calloc_pointer_address(semantic_direct_callee, *inferred_call);
        }
      }

      if (const auto parsed_call = parse_direct_global_typed_call(*call);
          parsed_call.has_value()) {
        const std::string semantic_direct_callee =
            resolved_direct_callee_name(parsed_call->symbol_name);
        if (const auto lowered_memset =
                try_lower_direct_memset_call(semantic_direct_callee, parsed_call->typed_call);
            lowered_memset.has_value()) {
          return *lowered_memset;
        }
        if (const auto lowered_memcpy =
                try_lower_direct_memcpy_call(semantic_direct_callee, parsed_call->typed_call);
            lowered_memcpy.has_value()) {
          return *lowered_memcpy;
        }
        if (!returned_pointer_address.has_value()) {
          returned_pointer_address = maybe_resolve_direct_calloc_pointer_address(
              semantic_direct_callee, parsed_call->typed_call);
        }
        callee_name = std::move(semantic_direct_callee);
        is_variadic_call = parsed_call->is_variadic;
        lowered_args.reserve(parsed_call->typed_call.args.size());
        lowered_arg_types.reserve(parsed_call->typed_call.param_types.size());
        lowered_arg_abi.reserve(parsed_call->typed_call.param_types.size());
        for (std::size_t index = 0; index < parsed_call->typed_call.args.size(); ++index) {
          const auto trimmed_param_type =
              c4c::codegen::lir::trim_lir_arg_text(parsed_call->typed_call.param_types[index]);
          if (trimmed_param_type == "ptr") {
            const auto arg = lower_public_pointer_call_arg_value(
                c4c::codegen::lir::LirOperand(
                    std::string(parsed_call->typed_call.args[index].operand)));
            if (!arg.has_value()) {
              return fail_call_family(call_family);
            }
            lowered_arg_types.push_back(bir::TypeKind::Ptr);
            lowered_args.push_back(*arg);
            lowered_arg_abi.push_back(
                *lir_to_bir_detail::compute_call_arg_abi(context_.target_profile,
                                                         bir::TypeKind::Ptr));
            continue;
          }
          const auto arg_type =
              lower_scalar_or_function_pointer_type(parsed_call->typed_call.param_types[index]);
          if (!arg_type.has_value()) {
            const auto aggregate_layout =
                lower_byval_aggregate_layout(parsed_call->typed_call.param_types[index], type_decls);
            if (!aggregate_layout.has_value()) {
              return fail_call_family(call_family);
            }
            const auto arg = lower_byval_call_arg_value(
                c4c::codegen::lir::LirOperand(
                    std::string(parsed_call->typed_call.args[index].operand)),
                *aggregate_layout);
            if (!arg.has_value()) {
              return fail_call_family(call_family);
            }
            lowered_arg_types.push_back(bir::TypeKind::Ptr);
            lowered_args.push_back(*arg);
            lowered_arg_abi.push_back(bir::CallArgAbiInfo{
                .type = bir::TypeKind::Ptr,
                .size_bytes = aggregate_layout->size_bytes,
                .align_bytes = aggregate_layout->align_bytes,
                .primary_class = bir::AbiValueClass::Memory,
                .byval_copy = true,
            });
            continue;
          }
          const auto arg =
              lower_value(c4c::codegen::lir::LirOperand(
                              std::string(parsed_call->typed_call.args[index].operand)),
                          *arg_type,
                          value_aliases);
          if (!arg.has_value()) {
            return fail_call_family(call_family);
          }
          lowered_arg_types.push_back(*arg_type);
          lowered_args.push_back(*arg);
          lowered_arg_abi.push_back(
              *lir_to_bir_detail::compute_call_arg_abi(context_.target_profile, *arg_type));
        }
      } else if (c4c::codegen::lir::trim_lir_arg_text(call->args_str).empty()) {
        callee_name = resolved_direct_callee_name(*direct_callee);
      } else {
        return fail_call_family(call_family);
      }
    } else {
      call_family = kIndirectCallFamily;
      const auto parsed_call = parse_typed_call(*call);
      if (!parsed_call.has_value()) {
        return fail_call_family(call_family);
      }
      is_variadic_call = parsed_call->is_variadic;
      callee_value = lower_value(call->callee, bir::TypeKind::Ptr, value_aliases);
      if (!callee_value.has_value()) {
        return false;
      }
      lowered_args.reserve(parsed_call->args.size());
      lowered_arg_types.reserve(parsed_call->param_types.size());
      lowered_arg_abi.reserve(parsed_call->param_types.size());
      for (std::size_t index = 0; index < parsed_call->args.size(); ++index) {
        const auto trimmed_param_type =
            c4c::codegen::lir::trim_lir_arg_text(parsed_call->param_types[index]);
        if (trimmed_param_type == "ptr") {
          const auto arg = lower_public_pointer_call_arg_value(
              c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)));
          if (!arg.has_value()) {
            return fail_call_family(call_family);
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(*arg);
          lowered_arg_abi.push_back(
              *lir_to_bir_detail::compute_call_arg_abi(context_.target_profile,
                                                       bir::TypeKind::Ptr));
          continue;
        }
        const auto arg_type =
            lower_scalar_or_function_pointer_type(parsed_call->param_types[index]);
        if (!arg_type.has_value()) {
          const auto aggregate_layout =
              lower_byval_aggregate_layout(parsed_call->param_types[index], type_decls);
          if (!aggregate_layout.has_value()) {
            return fail_call_family(call_family);
          }
          const auto arg = lower_byval_call_arg_value(
              c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)),
              *aggregate_layout);
          if (!arg.has_value()) {
            return fail_call_family(call_family);
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(*arg);
          lowered_arg_abi.push_back(bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = aggregate_layout->size_bytes,
              .align_bytes = aggregate_layout->align_bytes,
              .primary_class = bir::AbiValueClass::Memory,
              .byval_copy = true,
          });
          continue;
        }
        const auto arg =
            lower_value(c4c::codegen::lir::LirOperand(
                            std::string(parsed_call->args[index].operand)),
                        *arg_type,
                        value_aliases);
        if (!arg.has_value()) {
          return fail_call_family(call_family);
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*arg);
        lowered_arg_abi.push_back(
            *lir_to_bir_detail::compute_call_arg_abi(context_.target_profile, *arg_type));
      }
      is_indirect_call = true;
    }

    bir::CallInst lowered_call;
    if (!return_info->returned_via_sret && return_info->type != bir::TypeKind::Void) {
      if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return fail_call_family(call_family);
      }
      lowered_call.result = bir::Value::named(return_info->type, call->result.str());
    } else if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      if (!return_info->returned_via_sret) {
        return fail_call_family(call_family);
      }
    }
    if (is_indirect_call) {
      lowered_call.callee_value = std::move(*callee_value);
    } else {
      lowered_call.callee = std::move(*callee_name);
    }
    lowered_call.args = std::move(lowered_args);
    lowered_call.arg_types = std::move(lowered_arg_types);
    lowered_call.arg_abi = std::move(lowered_arg_abi);
    lowered_call.return_type_name =
        return_info->returned_via_sret ? "void" : std::string(call->return_type.str());
    lowered_call.return_type = return_info->type;
    lowered_call.result_abi = return_info->abi;
    lowered_call.is_indirect = is_indirect_call;
    lowered_call.is_variadic = is_variadic_call;
    if (sret_slot_name.has_value()) {
      lowered_call.sret_storage_name = *sret_slot_name;
    }
    lowered_insts->push_back(std::move(lowered_call));
    if (returned_pointer_address.has_value()) {
      pointer_value_addresses[call->result.str()] = *returned_pointer_address;
    }
    return true;
  }

  return false;
}
}  // namespace c4c::backend
