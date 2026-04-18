#include "lir_to_bir.hpp"

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
using GlobalPointerSlotKey = BirFunctionLowerer::GlobalPointerSlotKey;
using LocalArraySlots = BirFunctionLowerer::LocalArraySlots;
using LocalPointerArrayBase = BirFunctionLowerer::LocalPointerArrayBase;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

static bool can_address_scalar_subobject(std::int64_t byte_offset,
                                         bir::TypeKind stored_type,
                                         std::string_view type_text,
                                         bir::TypeKind access_type,
                                         const BirFunctionLowerer::TypeDeclMap& type_decls,
                                         bool allow_opaque_ptr_base) {
  if (byte_offset < 0) {
    return false;
  }
  if (stored_type == access_type) {
    return true;
  }
  if (stored_type != bir::TypeKind::Void) {
    return false;
  }
  if (allow_opaque_ptr_base && byte_offset == 0 && (type_text.empty() || type_text == "ptr")) {
    return true;
  }

  const auto access_size = type_size_bytes(access_type);
  if (access_size == 0) {
    return false;
  }
  const auto view_layout = compute_aggregate_type_layout(type_text, type_decls);
  if (view_layout.kind == BirFunctionLowerer::AggregateTypeLayout::Kind::Invalid ||
      view_layout.size_bytes == 0) {
    return false;
  }
  return static_cast<std::size_t>(byte_offset) + access_size <= view_layout.size_bytes;
}

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
  auto& local_indirect_pointer_slots = local_indirect_pointer_slots_;
  auto& pointer_value_addresses = pointer_value_addresses_;
  auto& local_pointer_slot_addresses = local_pointer_slot_addresses_;
  auto& local_address_slots = local_address_slots_;
  auto& local_slot_address_slots = local_slot_address_slots_;
  auto& local_slot_pointer_values = local_slot_pointer_values_;
  auto& global_address_slots = global_address_slots_;
  auto& addressed_global_pointer_slots = addressed_global_pointer_slots_;
  auto& global_pointer_slots = global_pointer_slots_;
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
        if (global_it == global_types.end()) {
          return fail_scalar_cast();
        }
        if (global_it->second.supports_linear_addressing) {
          global_address_ints[cast->result.str()] = GlobalAddress{
              .global_name = global_name,
              .value_type = global_it->second.value_type,
              .byte_offset = 0,
          };
          return true;
        }
        if (!global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return fail_scalar_cast();
        }
        global_object_address_ints[cast->result.str()] = GlobalAddress{
            .global_name = global_name,
            .value_type = bir::TypeKind::Ptr,
            .byte_offset = 0,
        };
        return true;
      }

      if (cast->operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return fail_scalar_cast();
      }

      const auto global_object_it = global_object_pointer_slots.find(cast->operand.str());
      if (global_object_it != global_object_pointer_slots.end()) {
        global_object_address_ints[cast->result.str()] = global_object_it->second;
        return true;
      }

      const auto global_ptr_it = global_pointer_slots.find(cast->operand.str());
      if (global_ptr_it == global_pointer_slots.end()) {
        return fail_scalar_cast();
      }
      global_address_ints[cast->result.str()] = global_ptr_it->second;
      return true;
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::IntToPtr &&
        cast->from_type.str() == "i64" && cast->to_type.str() == "ptr" &&
        cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto global_object_addr_it = global_object_address_ints.find(cast->operand.str());
      if (global_object_addr_it != global_object_address_ints.end()) {
        global_object_pointer_slots[cast->result.str()] = global_object_addr_it->second;
        return true;
      }

      const auto global_addr_it = global_address_ints.find(cast->operand.str());
      if (global_addr_it == global_address_ints.end()) {
        return fail_scalar_cast();
      }
      global_pointer_slots[cast->result.str()] = global_addr_it->second;
      return true;
    }

    const auto opcode = lower_cast_opcode(cast->kind);
    const auto from_type = lower_integer_type(cast->from_type.str());
    const auto to_type = lower_integer_type(cast->to_type.str());
    if (!opcode.has_value() || !from_type.has_value() || !to_type.has_value()) {
      return fail_scalar_cast();
    }

    const auto operand = lower_value(cast->operand, *from_type, value_aliases);
    if (!operand.has_value()) {
      return fail_scalar_cast();
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
    const auto value_type = lower_integer_type(bin->type_str.str());
    if (!opcode.has_value() || !value_type.has_value()) {
      return fail_scalar_binop();
    }

    if (*opcode == bir::BinaryOpcode::Sub && *value_type == bir::TypeKind::I64 &&
        bin->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        bin->rhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
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

    const auto lhs = lower_value(bin->lhs, *value_type, value_aliases);
    const auto rhs = lower_value(bin->rhs, *value_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return fail_scalar_binop();
    }

    if (*value_type == bir::TypeKind::I64 && lhs->kind == bir::Value::Kind::Immediate &&
        rhs->kind == bir::Value::Kind::Immediate) {
      const auto folded = fold_i64_binary_immediates(*opcode, lhs->immediate, rhs->immediate);
      if (folded.has_value()) {
        value_aliases[bin->result.str()] = *folded;
        return true;
      }
    }

    lowered_insts->push_back(bir::BinaryInst{
        .opcode = *opcode,
        .result = bir::Value::named(*value_type, bin->result.str()),
        .operand_type = *value_type,
        .lhs = *lhs,
        .rhs = *rhs,
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

    std::string resolved_slot;
    std::optional<LocalPointerArrayBase> resolved_array_base;
    if (gep->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = gep->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
        return fail_gep();
      }
      const auto resolved_address = resolve_global_gep_address(
          global_name, global_it->second.type_text, *gep, value_aliases, type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          global_name, global_it->second.type_text, 0, false, *gep, value_aliases, type_decls);
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
          local_aggregate_slots[gep->result.str()] = LocalAggregateSlots{
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = std::move(resolved_target->type_text),
              .base_byte_offset = static_cast<std::size_t>(resolved_target->byte_offset),
              .leaf_slots = aggregate_it->second.leaf_slots,
          };
          const auto leaf_it = aggregate_it->second.leaf_slots.find(
              static_cast<std::size_t>(resolved_target->byte_offset));
          if (leaf_it != aggregate_it->second.leaf_slots.end()) {
            local_slot_pointer_values[gep->result.str()] = LocalSlotAddress{
                .slot_name = leaf_it->second,
                .value_type = bir::TypeKind::Void,
                .byte_offset = 0,
                .storage_type_text = aggregate_it->second.storage_type_text,
                .type_text = local_aggregate_slots[gep->result.str()].type_text,
            };
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

    if (const auto array_it = local_array_slots.find(gep->ptr.str()); array_it != local_array_slots.end()) {
      if (gep->indices.size() != 2) {
        return fail_gep();
      }
      const auto base_index = parse_typed_operand(gep->indices[0]);
      const auto elem_index = parse_typed_operand(gep->indices[1]);
      if (!base_index.has_value() || !elem_index.has_value()) {
        return fail_gep();
      }
      const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
      const auto elem_imm = resolve_index_operand(elem_index->operand, value_aliases);
      if (!base_imm.has_value() || *base_imm != 0) {
        return fail_gep();
      }
      if (elem_imm.has_value()) {
        if (*elem_imm < 0 ||
            static_cast<std::size_t>(*elem_imm) >= array_it->second.element_slots.size()) {
          return fail_gep();
        }
        resolved_slot = array_it->second.element_slots[static_cast<std::size_t>(*elem_imm)];
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = array_it->second.element_slots,
            .base_index = static_cast<std::size_t>(*elem_imm),
        };
      } else {
        const auto elem_value = lower_typed_index_value(*elem_index, value_aliases);
        if (!elem_value.has_value() || array_it->second.element_type != bir::TypeKind::Ptr) {
          return fail_gep();
        }
        dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
            .element_slots = array_it->second.element_slots,
            .index = *elem_value,
        };
        return true;
      }
    } else if (const auto array_base_it = local_pointer_array_bases.find(gep->ptr.str());
               array_base_it != local_pointer_array_bases.end()) {
      if (gep->indices.empty() || gep->indices.size() > 2) {
        return fail_gep();
      }
      std::size_t index_pos = 0;
      if (gep->indices.size() == 2) {
        const auto base_index = parse_typed_operand(gep->indices[0]);
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
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
      if (index_imm.has_value()) {
        const auto final_index = static_cast<std::int64_t>(array_base_it->second.base_index) + *index_imm;
        if (final_index < 0) {
          if (array_base_it->second.element_slots.empty()) {
            return fail_gep();
          }
          const auto base_slot = array_base_it->second.element_slots.front();
          const auto slot_type_it = local_slot_types.find(base_slot);
          if (slot_type_it == local_slot_types.end()) {
            return fail_gep();
          }
          const auto slot_size = type_size_bytes(slot_type_it->second);
          if (slot_size == 0) {
            return fail_gep();
          }
          local_slot_pointer_values[gep->result.str()] = LocalSlotAddress{
              .slot_name = base_slot,
              .value_type = slot_type_it->second,
              .byte_offset = final_index * static_cast<std::int64_t>(slot_size),
              .storage_type_text = render_type(slot_type_it->second),
              .type_text = render_type(slot_type_it->second),
              .array_element_slots = array_base_it->second.element_slots,
          };
          return true;
        }
        if (static_cast<std::size_t>(final_index) > array_base_it->second.element_slots.size()) {
          return fail_gep();
        }
        if (static_cast<std::size_t>(final_index) == array_base_it->second.element_slots.size()) {
          if (array_base_it->second.element_slots.empty()) {
            return fail_gep();
          }
          const auto base_slot = array_base_it->second.element_slots.front();
          const auto slot_type_it = local_slot_types.find(base_slot);
          if (slot_type_it == local_slot_types.end()) {
            return fail_gep();
          }
          const auto slot_size = type_size_bytes(slot_type_it->second);
          if (slot_size == 0) {
            return fail_gep();
          }
          local_slot_pointer_values[gep->result.str()] = LocalSlotAddress{
              .slot_name = base_slot,
              .value_type = slot_type_it->second,
              .byte_offset = final_index * static_cast<std::int64_t>(slot_size),
              .storage_type_text = render_type(slot_type_it->second),
              .type_text = render_type(slot_type_it->second),
              .array_element_slots = array_base_it->second.element_slots,
              .array_base_index = static_cast<std::size_t>(final_index),
          };
          local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
              .element_slots = array_base_it->second.element_slots,
              .base_index = static_cast<std::size_t>(final_index),
          };
          return true;
        }
        resolved_slot =
            array_base_it->second.element_slots[static_cast<std::size_t>(final_index)];
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = array_base_it->second.element_slots,
            .base_index = static_cast<std::size_t>(final_index),
        };
      } else {
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value() || array_base_it->second.base_index != 0) {
          return fail_gep();
        }
        if (array_base_it->second.element_slots.empty()) {
          return fail_gep();
        }
        const auto slot_type_it = local_slot_types.find(array_base_it->second.element_slots.front());
        if (slot_type_it == local_slot_types.end()) {
          return fail_gep();
        }
        if (slot_type_it->second == bir::TypeKind::Ptr) {
          dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
              .element_slots = array_base_it->second.element_slots,
              .index = *index_value,
          };
          return true;
        }

        const auto element_size = type_size_bytes(slot_type_it->second);
        if (element_size == 0) {
          return fail_gep();
        }

        std::optional<std::string> element_type_text;
        switch (slot_type_it->second) {
          case bir::TypeKind::I1:
            element_type_text = "i1";
            break;
          case bir::TypeKind::I8:
            element_type_text = "i8";
            break;
          case bir::TypeKind::I32:
            element_type_text = "i32";
            break;
          case bir::TypeKind::I64:
            element_type_text = "i64";
            break;
          default:
            return fail_gep();
        }

        DynamicLocalAggregateArrayAccess access{
            .element_type_text = *element_type_text,
            .byte_offset = 0,
            .element_count = array_base_it->second.element_slots.size(),
            .element_stride_bytes = element_size,
        };
        for (std::size_t element_index = 0;
             element_index < array_base_it->second.element_slots.size();
             ++element_index) {
          const auto& slot_name = array_base_it->second.element_slots[element_index];
          const auto element_slot_type_it = local_slot_types.find(slot_name);
          if (element_slot_type_it == local_slot_types.end() ||
              element_slot_type_it->second != slot_type_it->second) {
            return fail_gep();
          }
          access.leaf_slots.emplace(element_index * element_size, slot_name);
        }
        dynamic_local_aggregate_arrays[gep->result.str()] = std::move(access);
        return true;
      }
    } else if (const auto global_ptr_it = global_pointer_slots.find(gep->ptr.str());
               global_ptr_it != global_pointer_slots.end()) {
      auto base_address = global_ptr_it->second;
      if (const auto honest_base = resolve_honest_pointer_base(base_address, global_types);
          honest_base.has_value()) {
        base_address = *honest_base;
      }
      const auto resolved_address = resolve_relative_global_gep_address(
          base_address, gep->element_type.str(), *gep, value_aliases, type_decls);
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
          value_aliases,
          type_decls);
      if (!dynamic_array.has_value()) {
        const auto dynamic_aggregate = resolve_global_dynamic_aggregate_array_access(
            base_address, gep->element_type.str(), *gep, value_aliases, global_types, type_decls);
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
          const auto resolved_target = resolve_relative_gep_target(
              addressed_ptr_it != pointer_value_addresses.end() &&
                      !addressed_ptr_it->second.type_text.empty()
                  ? addressed_ptr_it->second.type_text
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
            return true;
          }

          if (addressed_ptr_it != pointer_value_addresses.end() &&
              (gep->indices.size() == 1 || gep->indices.size() == 2) &&
              !addressed_ptr_it->second.storage_type_text.empty() &&
              !addressed_ptr_it->second.type_text.empty()) {
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
        }
      }
      if (ptr_it == local_pointer_slots.end() || gep->indices.size() != 1) {
        return fail_gep();
      }
      const auto slot_it = local_slot_types.find(ptr_it->second);
      if (slot_it == local_slot_types.end()) {
        return fail_gep();
      }

      const auto parsed_index = parse_typed_operand(gep->indices.front());
      if (!parsed_index.has_value()) {
        return fail_gep();
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);

      const auto dot = ptr_it->second.rfind('.');
      if (dot == std::string::npos) {
        if (!index_imm.has_value() || *index_imm != 0) {
          return fail_gep();
        }
        resolved_slot = ptr_it->second;
      } else {
        const auto base_name = ptr_it->second.substr(0, dot);
        const auto base_array_it = local_array_slots.find(base_name);
        const auto base_offset = parse_i64(std::string_view(ptr_it->second).substr(dot + 1));
        if (!base_offset.has_value()) {
          return fail_gep();
        }
        if (base_array_it != local_array_slots.end()) {
          if (index_imm.has_value()) {
            const auto final_index = *base_offset + *index_imm;
            if (final_index < 0 ||
                static_cast<std::size_t>(final_index) >= base_array_it->second.element_slots.size()) {
              return fail_gep();
            }
            resolved_slot =
                base_array_it->second.element_slots[static_cast<std::size_t>(final_index)];
            resolved_array_base = LocalPointerArrayBase{
                .element_slots = base_array_it->second.element_slots,
                .base_index = static_cast<std::size_t>(final_index),
            };
          } else {
            const auto parsed_index = parse_typed_operand(gep->indices.front());
            if (!parsed_index.has_value()) {
              return fail_gep();
            }
            const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
            if (!index_value.has_value() || *base_offset != 0 ||
                base_array_it->second.element_type != bir::TypeKind::Ptr) {
              return fail_gep();
            }
            dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
                .element_slots = base_array_it->second.element_slots,
                .index = *index_value,
            };
            return true;
          }
        } else if (const auto base_aggregate_it = local_aggregate_slots.find(base_name);
                   base_aggregate_it != local_aggregate_slots.end()) {
          const auto slot_size = type_size_bytes(slot_it->second);
          if (slot_size == 0) {
            return fail_gep();
          }
          if (!index_imm.has_value()) {
            const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
            if (!index_value.has_value() || *base_offset < 0) {
              return fail_gep();
            }

            const auto dynamic_access = build_dynamic_local_aggregate_array_access(
                slot_it->second,
                static_cast<std::size_t>(*base_offset),
                base_aggregate_it->second,
                *index_value,
                type_decls);
            if (!dynamic_access.has_value()) {
              return fail_gep();
            }

            dynamic_local_aggregate_arrays[gep->result.str()] = std::move(*dynamic_access);
            return true;
          }
          const auto final_byte_offset =
              *base_offset + *index_imm * static_cast<std::int64_t>(slot_size);
          if (final_byte_offset < 0) {
            return fail_gep();
          }
          const auto leaf_it =
              base_aggregate_it->second.leaf_slots.find(static_cast<std::size_t>(final_byte_offset));
          if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
            return fail_gep();
          }
          resolved_slot = leaf_it->second;
        } else {
          return fail_gep();
        }
      }
    }

    if (resolved_array_base.has_value()) {
      local_pointer_array_bases[gep->result.str()] = std::move(*resolved_array_base);
    }
    local_pointer_slots[gep->result.str()] = std::move(resolved_slot);
    return true;
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

    const auto append_addressed_store =
        [&](std::string_view scratch_prefix, const bir::MemoryAddress& address) -> bool {
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return fail_store();
      }
      const std::string scratch_slot = std::string(scratch_prefix) + ".addr";
      if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
        return fail_store();
      }
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = scratch_slot,
          .value = *value,
          .address = address,
      });
      return true;
    };

    const auto append_dynamic_pointer_array_store =
        [&](std::string_view scratch_prefix,
            const DynamicPointerValueArrayAccess& access) -> bool {
      if (access.element_type != *value_type || access.element_count == 0) {
        return fail_store();
      }

      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return fail_store();
      }

      for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
        const std::string element_name =
            std::string(scratch_prefix) + ".elt" + std::to_string(element_index);
        const std::string load_slot = element_name + ".load.addr";
        if (!ensure_local_scratch_slot(load_slot, *value_type, slot_size)) {
          return fail_store();
        }
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, element_name),
            .slot_name = load_slot,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = access.base_value,
                    .byte_offset = static_cast<std::int64_t>(
                        access.byte_offset + element_index * access.element_stride_bytes),
                    .size_bytes = slot_size,
                    .align_bytes = slot_size,
                },
        });

        bir::Value stored_value = *value;
        if (access.element_count > 1) {
          const auto compare_rhs = make_index_immediate(access.index.type, element_index);
          if (!compare_rhs.has_value()) {
            return fail_store();
          }
          const std::string select_name =
              std::string(scratch_prefix) + ".store" + std::to_string(element_index);
          lowered_insts->push_back(bir::SelectInst{
              .predicate = bir::BinaryOpcode::Eq,
              .result = bir::Value::named(*value_type, select_name),
              .compare_type = access.index.type,
              .lhs = access.index,
              .rhs = *compare_rhs,
              .true_value = *value,
              .false_value = bir::Value::named(*value_type, element_name),
          });
          stored_value = bir::Value::named(*value_type, select_name);
        }

        const std::string store_slot = element_name + ".store.addr";
        if (!ensure_local_scratch_slot(store_slot, *value_type, slot_size)) {
          return fail_store();
        }
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = store_slot,
            .value = stored_value,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = access.base_value,
                    .byte_offset = static_cast<std::int64_t>(
                        access.byte_offset + element_index * access.element_stride_bytes),
                    .size_bytes = slot_size,
                    .align_bytes = slot_size,
                },
        });
      }
      return true;
    };

    if (const auto addressed_ptr_it = pointer_value_addresses.find(store->ptr.str());
        addressed_ptr_it != pointer_value_addresses.end()) {
      if (!can_address_scalar_subobject(static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                                        addressed_ptr_it->second.value_type,
                                        addressed_ptr_it->second.type_text,
                                        *value_type,
                                        type_decls,
                                        true)) {
        return fail_store();
      }
      if (!append_addressed_store(
              store->ptr.str(),
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value = addressed_ptr_it->second.base_value,
                  .byte_offset = static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                  .size_bytes = type_size_bytes(*value_type),
                  .align_bytes = type_size_bytes(*value_type),
              })) {
        return fail_store();
      }
      return true;
    }

    if (const auto local_slot_ptr_it = local_slot_pointer_values.find(store->ptr.str());
        local_slot_ptr_it != local_slot_pointer_values.end()) {
      const auto direct_slot_type_it = local_slot_types.find(local_slot_ptr_it->second.slot_name);
      const bool exact_local_slot_type = local_slot_ptr_it->second.value_type == *value_type;
      const bool can_use_direct_local_slot =
          direct_slot_type_it != local_slot_types.end() &&
          direct_slot_type_it->second == *value_type;
      if (!can_address_scalar_subobject(local_slot_ptr_it->second.byte_offset,
                                        local_slot_ptr_it->second.value_type,
                                        local_slot_ptr_it->second.type_text,
                                        *value_type,
                                        type_decls,
                                        false)) {
        return fail_store();
      }
      if (exact_local_slot_type && can_use_direct_local_slot &&
          local_slot_ptr_it->second.byte_offset == 0) {
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = local_slot_ptr_it->second.slot_name,
            .value = *value,
        });
        return true;
      }
      if (!append_addressed_store(
              store->ptr.str(),
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = local_slot_ptr_it->second.slot_name,
                  .byte_offset = static_cast<std::int64_t>(local_slot_ptr_it->second.byte_offset),
                  .size_bytes = type_size_bytes(*value_type),
                  .align_bytes = type_size_bytes(*value_type),
              })) {
        return fail_store();
      }
      return true;
    }

    if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays.find(store->ptr.str());
        dynamic_ptr_it != dynamic_pointer_value_arrays.end()) {
      if (!append_dynamic_pointer_array_store(store->ptr.str(), dynamic_ptr_it->second)) {
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
      return true;
    }

    if (store->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = store->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return fail_store();
      }
      if (*value_type == bir::TypeKind::Ptr) {
        global_address_slots[global_name] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_name,
          .value = *value,
      });
      return true;
    }

    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto global_object_it = global_object_pointer_slots.find(store->ptr.str());
          global_object_it != global_object_pointer_slots.end()) {
        const auto global_it = global_types.find(global_object_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != *value_type) {
          return fail_store();
        }
        global_address_slots[global_object_it->second.global_name] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
        lowered_insts->push_back(bir::StoreGlobalInst{
            .global_name = global_object_it->second.global_name,
            .value = *value,
            .byte_offset = global_object_it->second.byte_offset,
        });
        return true;
      }
    }

    if (const auto global_ptr_it = global_pointer_slots.find(store->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (global_ptr_it->second.value_type != *value_type) {
        return fail_store();
      }
      if (*value_type == bir::TypeKind::Ptr) {
        addressed_global_pointer_slots[make_global_pointer_slot_key(global_ptr_it->second)] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_ptr_it->second.global_name,
          .value = *value,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(store->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      return fail_store();
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return fail_store();
    }

    const bool tracks_pointer_value_slot =
        local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
        is_local_array_element_slot(ptr_it->second, local_array_slots);
    if (tracks_pointer_value_slot) {
      if (*value_type == bir::TypeKind::Ptr) {
        const auto pointer_alias = resolve_local_aggregate_pointer_value_alias(
            store->val, value_aliases, local_aggregate_slots, function_symbols);
        if (pointer_alias.has_value()) {
          local_pointer_value_aliases[ptr_it->second] = *pointer_alias;
        } else {
          local_pointer_value_aliases.erase(ptr_it->second);
        }
      } else {
        local_pointer_value_aliases.erase(ptr_it->second);
      }
    }

    if (*value_type == bir::TypeKind::I64) {
      local_pointer_slot_addresses.erase(ptr_it->second);
      local_indirect_pointer_slots.erase(ptr_it->second);
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto global_addr_it = global_address_ints.find(store->val.str());
        if (global_addr_it != global_address_ints.end()) {
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = global_addr_it->second;
          return true;
        }
      }
      local_slot_address_slots.erase(ptr_it->second);
      local_address_slots.erase(ptr_it->second);
    } else if (*value_type == bir::TypeKind::Ptr) {
      bool stored_pointer_value_address = false;
      bool stored_local_slot_address = false;
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::Global) {
        const std::string global_name = store->val.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it == global_types.end()) {
          if (!is_known_function_symbol(global_name, function_symbols)) {
            return fail_store();
          }
          local_pointer_slot_addresses.erase(ptr_it->second);
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = GlobalAddress{
              .global_name = global_name,
              .value_type = bir::TypeKind::Ptr,
              .byte_offset = 0,
          };
          local_indirect_pointer_slots.erase(ptr_it->second);
          if (!tracks_pointer_value_slot) {
            return true;
          }
        }
        if (global_it == global_types.end()) {
          stored_local_slot_address = false;
        } else if (!global_it->second.supports_linear_addressing) {
          return fail_store();
        } else {
          local_pointer_slot_addresses.erase(ptr_it->second);
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = GlobalAddress{
              .global_name = global_name,
              .value_type = global_it->second.value_type,
              .byte_offset = 0,
          };
          local_indirect_pointer_slots.erase(ptr_it->second);
          if (!tracks_pointer_value_slot) {
            return true;
          }
        }
      }
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto local_slot_ptr_val_it = local_slot_pointer_values.find(store->val.str());
        if (local_slot_ptr_val_it != local_slot_pointer_values.end()) {
          local_pointer_slot_addresses.erase(ptr_it->second);
          auto stored_address = local_slot_ptr_val_it->second;
          if (stored_address.array_element_slots.empty()) {
            if (const auto array_base_it = local_pointer_array_bases.find(store->val.str());
                array_base_it != local_pointer_array_bases.end()) {
              stored_address.array_element_slots = array_base_it->second.element_slots;
              stored_address.array_base_index = array_base_it->second.base_index;
            } else if (const auto local_aggregate_it = local_aggregate_slots.find(store->val.str());
                       local_aggregate_it != local_aggregate_slots.end()) {
              if (const auto array_slots = collect_local_scalar_array_slots(
                      local_aggregate_it->second.type_text, type_decls, local_aggregate_it->second);
                  array_slots.has_value()) {
                stored_address.array_element_slots = *array_slots;
              }
            }
          }
          local_slot_address_slots[ptr_it->second] = std::move(stored_address);
          local_address_slots.erase(ptr_it->second);
          local_indirect_pointer_slots.erase(ptr_it->second);
          stored_local_slot_address = true;
        }
        if (const auto pointer_value_it = pointer_value_addresses.find(store->val.str());
            pointer_value_it != pointer_value_addresses.end()) {
          local_pointer_slot_addresses[ptr_it->second] = pointer_value_it->second;
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots.erase(ptr_it->second);
          local_indirect_pointer_slots.erase(ptr_it->second);
          stored_pointer_value_address = true;
        }
        const auto local_ptr_it = local_pointer_slots.find(store->val.str());
        if (local_ptr_it != local_pointer_slots.end() &&
            local_slot_ptr_val_it == local_slot_pointer_values.end()) {
          const auto source_slot_it = local_slot_types.find(local_ptr_it->second);
          if (source_slot_it == local_slot_types.end()) {
            return fail_store();
          }
          local_pointer_slot_addresses.erase(ptr_it->second);
          LocalSlotAddress stored_address{
              .slot_name = local_ptr_it->second,
              .value_type = source_slot_it->second,
              .byte_offset = 0,
              .storage_type_text = render_type(source_slot_it->second),
              .type_text = render_type(source_slot_it->second),
          };
          if (const auto array_base_it = local_pointer_array_bases.find(store->val.str());
              array_base_it != local_pointer_array_bases.end()) {
            const auto slot_size = type_size_bytes(source_slot_it->second);
            if (slot_size == 0 || array_base_it->second.element_slots.empty() ||
                array_base_it->second.base_index >= array_base_it->second.element_slots.size()) {
              return fail_store();
            }
            stored_address.slot_name = array_base_it->second.element_slots.front();
            stored_address.byte_offset = array_base_it->second.base_index * slot_size;
            stored_address.array_element_slots = array_base_it->second.element_slots;
            stored_address.array_base_index = array_base_it->second.base_index;
          }
          local_slot_address_slots[ptr_it->second] = std::move(stored_address);
          local_address_slots.erase(ptr_it->second);
          local_indirect_pointer_slots.erase(ptr_it->second);
          stored_local_slot_address = true;
        } else if (const auto local_aggregate_it = local_aggregate_slots.find(store->val.str());
                   local_aggregate_it != local_aggregate_slots.end() &&
                   local_slot_ptr_val_it == local_slot_pointer_values.end()) {
          const auto leaf_it =
              local_aggregate_it->second.leaf_slots.find(local_aggregate_it->second.base_byte_offset);
          if (leaf_it == local_aggregate_it->second.leaf_slots.end()) {
            return fail_store();
          }
          local_pointer_slot_addresses.erase(ptr_it->second);
          const auto target_layout = compute_aggregate_type_layout(
              local_aggregate_it->second.type_text, type_decls);
          auto stored_address = LocalSlotAddress{
              .slot_name = leaf_it->second,
              .value_type = target_layout.kind == AggregateTypeLayout::Kind::Scalar
                                ? target_layout.scalar_type
                                : bir::TypeKind::Void,
              .byte_offset = 0,
              .storage_type_text = local_aggregate_it->second.storage_type_text,
              .type_text = local_aggregate_it->second.type_text,
          };
          if (const auto array_slots = collect_local_scalar_array_slots(
                  local_aggregate_it->second.type_text, type_decls, local_aggregate_it->second);
              array_slots.has_value()) {
            stored_address.array_element_slots = *array_slots;
          }
          local_slot_address_slots[ptr_it->second] = std::move(stored_address);
          local_address_slots.erase(ptr_it->second);
          local_indirect_pointer_slots.erase(ptr_it->second);
          stored_local_slot_address = true;
        }
        const auto global_ptr_it = global_pointer_slots.find(store->val.str());
        if (global_ptr_it != global_pointer_slots.end()) {
          local_pointer_slot_addresses.erase(ptr_it->second);
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = global_ptr_it->second;
          local_indirect_pointer_slots.insert(ptr_it->second);
          return true;
        }
      }
      if (!stored_local_slot_address && !stored_pointer_value_address) {
        local_pointer_slot_addresses.erase(ptr_it->second);
        local_slot_address_slots.erase(ptr_it->second);
        local_address_slots.erase(ptr_it->second);
        local_indirect_pointer_slots.erase(ptr_it->second);
      }
    }

    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = ptr_it->second,
        .value = *value,
    });
    return true;
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
      if (!aggregate_layout.has_value() ||
          load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return fail_load();
      }
      if (local_aggregate_slots.find(load->ptr.str()) == local_aggregate_slots.end()) {
        return fail_load();
      }
      aggregate_value_aliases[load->result.str()] = load->ptr.str();
      return true;
    }

    if (load->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = load->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return fail_load();
      }
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto runtime_it = global_address_slots.find(global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *runtime_it->second;
          }
        } else if (global_it->second.known_global_address.has_value()) {
          global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load->result.str(), global_it->second, global_types, global_object_pointer_slots);
        }
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_name,
      });
      return true;
    }

    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto global_object_it = global_object_pointer_slots.find(load->ptr.str());
          global_object_it != global_object_pointer_slots.end()) {
        const auto global_it = global_types.find(global_object_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return fail_load();
        }
        if (const auto runtime_it = global_address_slots.find(global_object_it->second.global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *runtime_it->second;
          }
        } else if (global_it->second.known_global_address.has_value()) {
          global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load->result.str(), global_it->second, global_types, global_object_pointer_slots);
        }
        lowered_insts->push_back(bir::LoadGlobalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .global_name = global_object_it->second.global_name,
            .byte_offset = global_object_it->second.byte_offset,
        });
        return true;
      }
    }

    if (const auto global_ptr_it = global_pointer_slots.find(load->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (*value_type != bir::TypeKind::Ptr) {
        if (const auto honest_address = resolve_honest_addressed_global_access(
                global_ptr_it->second, *value_type, global_types);
            honest_address.has_value()) {
          lowered_insts->push_back(bir::LoadGlobalInst{
              .result = bir::Value::named(*value_type, load->result.str()),
              .global_name = honest_address->global_name,
              .byte_offset = honest_address->byte_offset,
          });
          return true;
        }
      }

      if (global_ptr_it->second.value_type != *value_type) {
        if (global_ptr_it->second.value_type != bir::TypeKind::Ptr ||
            global_ptr_it->second.byte_offset != 0) {
          return fail_load();
        }
        const auto global_it = global_types.find(global_ptr_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return fail_load();
        }
        lowered_insts->push_back(bir::LoadGlobalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .global_name = global_ptr_it->second.global_name,
        });
        return true;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        const auto addressed_it =
            addressed_global_pointer_slots.find(make_global_pointer_slot_key(global_ptr_it->second));
        if (addressed_it != addressed_global_pointer_slots.end()) {
          if (addressed_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *addressed_it->second;
          }
        } else {
          const auto global_it = global_types.find(global_ptr_it->second.global_name);
          if (global_it == global_types.end()) {
            return fail_load();
          }
          if (global_ptr_it->second.byte_offset == 0 &&
              global_it->second.supports_direct_value &&
              global_it->second.value_type == bir::TypeKind::Ptr) {
            if (const auto runtime_it = global_address_slots.find(global_ptr_it->second.global_name);
                runtime_it != global_address_slots.end()) {
              if (runtime_it->second.has_value()) {
                global_pointer_slots[load->result.str()] = *runtime_it->second;
              }
            } else if (global_it->second.known_global_address.has_value()) {
              global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
              record_pointer_global_object_alias(
                  load->result.str(), global_it->second, global_types, global_object_pointer_slots);
            }
          }
          const auto pointer_init_it =
              global_it->second.pointer_initializer_offsets.find(global_ptr_it->second.byte_offset);
          if (pointer_init_it != global_it->second.pointer_initializer_offsets.end()) {
            global_pointer_slots[load->result.str()] = pointer_init_it->second;
          }
        }
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_ptr_it->second.global_name,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    if (const auto addressed_ptr_it = pointer_value_addresses.find(load->ptr.str());
        addressed_ptr_it != pointer_value_addresses.end()) {
      if (!can_address_scalar_subobject(static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                                        addressed_ptr_it->second.value_type,
                                        addressed_ptr_it->second.type_text,
                                        *value_type,
                                        type_decls,
                                        true)) {
        return fail_load();
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return fail_load();
      }
      const std::string scratch_slot = load->result.str() + ".addr";
      if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
        return fail_load();
      }
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .slot_name = std::move(scratch_slot),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value = addressed_ptr_it->second.base_value,
                  .byte_offset = static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                  .size_bytes = slot_size,
                  .align_bytes = slot_size,
              },
      });
      return true;
    }

    if (const auto local_slot_ptr_it = local_slot_pointer_values.find(load->ptr.str());
        local_slot_ptr_it != local_slot_pointer_values.end()) {
      const auto direct_slot_type_it = local_slot_types.find(local_slot_ptr_it->second.slot_name);
      const bool exact_local_slot_type = local_slot_ptr_it->second.value_type == *value_type;
      const bool can_use_direct_local_slot =
          direct_slot_type_it != local_slot_types.end() &&
          direct_slot_type_it->second == *value_type;
      if (!can_address_scalar_subobject(local_slot_ptr_it->second.byte_offset,
                                        local_slot_ptr_it->second.value_type,
                                        local_slot_ptr_it->second.type_text,
                                        *value_type,
                                        type_decls,
                                        false)) {
        return fail_load();
      }
      if (*value_type == bir::TypeKind::Ptr && exact_local_slot_type && can_use_direct_local_slot &&
          local_slot_ptr_it->second.byte_offset == 0) {
        if (const auto nested_local_slot_it =
                local_slot_address_slots.find(local_slot_ptr_it->second.slot_name);
            nested_local_slot_it != local_slot_address_slots.end()) {
          local_slot_pointer_values[load->result.str()] = nested_local_slot_it->second;
        }
        if (const auto nested_global_it = local_address_slots.find(local_slot_ptr_it->second.slot_name);
            nested_global_it != local_address_slots.end()) {
          const bool preserve_loaded_pointer_provenance =
              local_indirect_pointer_slots.find(local_slot_ptr_it->second.slot_name) !=
              local_indirect_pointer_slots.end();
          if (!preserve_loaded_pointer_provenance) {
            if (nested_global_it->second.byte_offset == 0 &&
                is_known_function_symbol(nested_global_it->second.global_name, function_symbols)) {
              value_aliases[load->result.str()] =
                  bir::Value::named(bir::TypeKind::Ptr, "@" + nested_global_it->second.global_name);
              global_pointer_slots[load->result.str()] = nested_global_it->second;
            } else if (const auto honest_base =
                           resolve_honest_pointer_base(nested_global_it->second, global_types);
                       honest_base.has_value() && honest_base->byte_offset == 0) {
              value_aliases[load->result.str()] =
                  bir::Value::named(bir::TypeKind::Ptr, "@" + honest_base->global_name);
              global_pointer_slots[load->result.str()] = *honest_base;
            }
          }
          if (global_pointer_slots.find(load->result.str()) == global_pointer_slots.end()) {
            global_pointer_slots[load->result.str()] = nested_global_it->second;
          }
        }
      }
      if (exact_local_slot_type && can_use_direct_local_slot &&
          local_slot_ptr_it->second.byte_offset == 0) {
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .slot_name = local_slot_ptr_it->second.slot_name,
        });
        return true;
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return fail_load();
      }
      const std::string scratch_slot = load->result.str() + ".addr";
      if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
        return fail_load();
      }
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .slot_name = std::move(scratch_slot),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = local_slot_ptr_it->second.slot_name,
                  .byte_offset = static_cast<std::int64_t>(local_slot_ptr_it->second.byte_offset),
                  .size_bytes = slot_size,
                  .align_bytes = slot_size,
              },
      });
      return true;
    }

    if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays.find(load->ptr.str());
        dynamic_ptr_it != dynamic_pointer_value_arrays.end()) {
      if (dynamic_ptr_it->second.element_type != *value_type ||
          dynamic_ptr_it->second.element_count == 0) {
        return fail_load();
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return fail_load();
      }

      std::vector<bir::Value> element_values;
      element_values.reserve(dynamic_ptr_it->second.element_count);
      for (std::size_t element_index = 0;
           element_index < dynamic_ptr_it->second.element_count;
           ++element_index) {
        const std::string element_name =
            load->result.str() + ".elt" + std::to_string(element_index);
        const std::string scratch_slot = element_name + ".addr";
        if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
          return fail_load();
        }
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, element_name),
            .slot_name = scratch_slot,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = dynamic_ptr_it->second.base_value,
                    .byte_offset = static_cast<std::int64_t>(
                        dynamic_ptr_it->second.byte_offset +
                        element_index * dynamic_ptr_it->second.element_stride_bytes),
                    .size_bytes = slot_size,
                    .align_bytes = slot_size,
                },
        });
        element_values.push_back(bir::Value::named(*value_type, element_name));
      }

      const auto selected_value = synthesize_value_array_selects(
          load->result.str(), element_values, dynamic_ptr_it->second.index, lowered_insts);
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
      if (global_scalar_it->second.element_type != *value_type ||
          global_scalar_it->second.outer_element_count == 0 ||
          global_scalar_it->second.element_count == 0) {
        return fail_load();
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return fail_load();
      }

      std::vector<bir::Value> outer_values;
      outer_values.reserve(global_scalar_it->second.outer_element_count);
      for (std::size_t outer_index = 0;
           outer_index < global_scalar_it->second.outer_element_count;
           ++outer_index) {
        std::vector<bir::Value> element_values;
        element_values.reserve(global_scalar_it->second.element_count);
        for (std::size_t element_index = 0;
             element_index < global_scalar_it->second.element_count;
             ++element_index) {
          const std::string element_name =
              load->result.str() + ".outer" + std::to_string(outer_index) + ".elt" +
              std::to_string(element_index);
          lowered_insts->push_back(bir::LoadGlobalInst{
              .result = bir::Value::named(*value_type, element_name),
              .global_name = global_scalar_it->second.global_name,
              .byte_offset =
                  global_scalar_it->second.byte_offset +
                  outer_index * global_scalar_it->second.outer_element_stride_bytes +
                  element_index * global_scalar_it->second.element_stride_bytes,
              .align_bytes = slot_size,
          });
          element_values.push_back(bir::Value::named(*value_type, element_name));
        }

        bir::Value outer_value;
        if (element_values.size() == 1) {
          outer_value = element_values.front();
        } else {
          const auto selected_inner = synthesize_value_array_selects(
              load->result.str() + ".outer" + std::to_string(outer_index),
              element_values,
              global_scalar_it->second.index,
              lowered_insts);
          if (!selected_inner.has_value()) {
            return fail_load();
          }
          outer_value = *selected_inner;
        }
        outer_values.push_back(std::move(outer_value));
      }

      bir::Value selected_value;
      if (outer_values.size() == 1) {
        selected_value = outer_values.front();
      } else {
        const auto selected_outer = synthesize_value_array_selects(
            load->result.str(), outer_values, global_scalar_it->second.outer_index, lowered_insts);
        if (!selected_outer.has_value()) {
          return fail_load();
        }
        selected_value = *selected_outer;
      }
      if (selected_value.kind == bir::Value::Kind::Named &&
          selected_value.name == load->result.str()) {
        return true;
      }
      value_aliases[load->result.str()] = selected_value;
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

    const auto ptr_it = local_pointer_slots.find(load->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto local_array_it = dynamic_local_pointer_arrays.find(load->ptr.str());
            local_array_it != dynamic_local_pointer_arrays.end()) {
          const auto element_values = collect_local_pointer_values(
              local_array_it->second.element_slots, local_pointer_value_aliases);
          if (!element_values.has_value()) {
            return fail_load();
          }
          const auto selected_value = synthesize_pointer_array_selects(
              load->result.str(), *element_values, local_array_it->second.index, lowered_insts);
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
        if (const auto global_array_it = dynamic_global_pointer_arrays.find(load->ptr.str());
            global_array_it != dynamic_global_pointer_arrays.end()) {
          const auto element_values =
              collect_global_array_pointer_values(global_array_it->second, global_types);
          if (!element_values.has_value()) {
            return fail_load();
          }
          const auto selected_value = synthesize_pointer_array_selects(
              load->result.str(), *element_values, global_array_it->second.index, lowered_insts);
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
      }
      return fail_load();
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return fail_load();
    }

    const bool tracks_pointer_value_slot =
        local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
        is_local_array_element_slot(ptr_it->second, local_array_slots);
    if (*value_type == bir::TypeKind::Ptr && tracks_pointer_value_slot) {
      const auto pointer_alias = local_pointer_value_aliases.find(ptr_it->second);
      if (pointer_alias != local_pointer_value_aliases.end()) {
        value_aliases[load->result.str()] = pointer_alias->second;
        if (const auto addr_it = local_address_slots.find(ptr_it->second);
            addr_it != local_address_slots.end()) {
          global_pointer_slots[load->result.str()] = addr_it->second;
        }
        return true;
      }
    }

    if (*value_type == bir::TypeKind::I64) {
      if (const auto addr_it = local_address_slots.find(ptr_it->second);
          addr_it != local_address_slots.end()) {
        global_address_ints[load->result.str()] = addr_it->second;
        return true;
      }
    } else if (*value_type == bir::TypeKind::Ptr) {
      if (const auto local_slot_it = local_slot_address_slots.find(ptr_it->second);
          local_slot_it != local_slot_address_slots.end()) {
        local_slot_pointer_values[load->result.str()] = local_slot_it->second;
        const auto loaded_layout =
            compute_aggregate_type_layout(local_slot_it->second.type_text, type_decls);
        if (loaded_layout.kind == AggregateTypeLayout::Kind::Array &&
            !local_slot_it->second.array_element_slots.empty() &&
            local_slot_it->second.byte_offset >= 0) {
          const auto element_layout =
              compute_aggregate_type_layout(loaded_layout.element_type_text, type_decls);
          if (element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
              element_layout.size_bytes != 0) {
            LocalAggregateSlots aggregate_view{
                .storage_type_text = local_slot_it->second.storage_type_text,
                .type_text = local_slot_it->second.type_text,
                .base_byte_offset =
                    static_cast<std::size_t>(local_slot_it->second.byte_offset),
            };
            for (std::size_t index = 0; index < local_slot_it->second.array_element_slots.size();
                 ++index) {
              aggregate_view.leaf_slots.emplace(index * element_layout.size_bytes,
                                                local_slot_it->second.array_element_slots[index]);
            }
            local_aggregate_slots[load->result.str()] = std::move(aggregate_view);
          }
        }
        if (!local_slot_it->second.array_element_slots.empty() &&
            local_slot_it->second.byte_offset >= 0 &&
            !(loaded_layout.kind == AggregateTypeLayout::Kind::Array &&
              local_slot_it->second.array_element_slots.size() > loaded_layout.array_count)) {
          local_pointer_array_bases[load->result.str()] = LocalPointerArrayBase{
              .element_slots = local_slot_it->second.array_element_slots,
              .base_index = local_slot_it->second.array_base_index,
          };
        }
      }
      if (const auto addr_it = local_address_slots.find(ptr_it->second);
          addr_it != local_address_slots.end()) {
        const bool preserve_loaded_pointer_provenance =
            local_indirect_pointer_slots.find(ptr_it->second) != local_indirect_pointer_slots.end();
        if (!preserve_loaded_pointer_provenance) {
          if (addr_it->second.byte_offset == 0 &&
              is_known_function_symbol(addr_it->second.global_name, function_symbols)) {
            value_aliases[load->result.str()] =
                bir::Value::named(bir::TypeKind::Ptr, "@" + addr_it->second.global_name);
            global_pointer_slots[load->result.str()] = addr_it->second;
            return true;
          }
          if (const auto honest_base = resolve_honest_pointer_base(addr_it->second, global_types);
              honest_base.has_value() && honest_base->byte_offset == 0) {
            value_aliases[load->result.str()] =
                bir::Value::named(bir::TypeKind::Ptr, "@" + honest_base->global_name);
            global_pointer_slots[load->result.str()] = *honest_base;
            return true;
          }
        }
        global_pointer_slots[load->result.str()] = addr_it->second;
      }
      if (const auto pointer_slot_it = local_pointer_slot_addresses.find(ptr_it->second);
          pointer_slot_it != local_pointer_slot_addresses.end()) {
        pointer_value_addresses[load->result.str()] = pointer_slot_it->second;
      }
    }

    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(*value_type, load->result.str()),
        .slot_name = ptr_it->second,
    });
    return true;
  }

  const auto try_lower_immediate_local_memset =
      [&](std::string_view dst_operand, std::uint8_t fill_byte, std::size_t fill_size_bytes) -> bool {
    struct LocalMemsetArrayView {
      bir::TypeKind element_type = bir::TypeKind::Void;
      std::vector<std::string> element_slots;
      std::size_t base_index = 0;
    };
    struct LocalMemsetScalarSlot {
      std::string slot_name;
      std::size_t size_bytes = 0;
    };

    const auto fill_leaf_slots =
        [&](const auto& leaf_slots, std::size_t total_size_bytes) -> bool {
      if (fill_size_bytes > total_size_bytes) {
        return false;
      }

      std::size_t covered_bytes = 0;
      for (const auto& [byte_offset, slot_name] : leaf_slots) {
        if (covered_bytes == fill_size_bytes) {
          break;
        }
        if (byte_offset != covered_bytes) {
          return false;
        }

        const auto slot_type_it = local_slot_types.find(slot_name);
        if (slot_type_it == local_slot_types.end()) {
          return false;
        }
        const auto slot_size = type_size_bytes(slot_type_it->second);
        if (slot_size == 0 || byte_offset + slot_size > fill_size_bytes) {
          return false;
        }

        const auto fill_value =
            lower_repeated_byte_initializer_value(slot_type_it->second, fill_byte);
        if (!fill_value.has_value()) {
          return false;
        }
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = slot_name,
            .value = *fill_value,
        });
        covered_bytes += slot_size;
      }
      return covered_bytes == fill_size_bytes;
    };

    const auto resolve_local_memset_array_view =
        [&](std::string_view operand) -> std::optional<LocalMemsetArrayView> {
      if (const auto array_it = local_array_slots.find(std::string(operand));
          array_it != local_array_slots.end()) {
        return LocalMemsetArrayView{
            .element_type = array_it->second.element_type,
            .element_slots = array_it->second.element_slots,
            .base_index = 0,
        };
      }
      if (const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
          array_base_it != local_pointer_array_bases.end() &&
          array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
        const auto slot_type_it =
            local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
        if (slot_type_it != local_slot_types.end()) {
          return LocalMemsetArrayView{
              .element_type = slot_type_it->second,
              .element_slots = array_base_it->second.element_slots,
              .base_index = array_base_it->second.base_index,
          };
        }
      }

      const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
      if (ptr_slot_it == local_pointer_slots.end()) {
        return std::nullopt;
      }
      const auto dot = ptr_slot_it->second.rfind('.');
      if (dot == std::string::npos) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const std::string base_name = ptr_slot_it->second.substr(0, dot);
      const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
      if (!base_offset.has_value() || *base_offset < 0) {
        return std::nullopt;
      }

      if (const auto base_array_it = local_array_slots.find(base_name);
          base_array_it != local_array_slots.end()) {
        const auto base_index = static_cast<std::size_t>(*base_offset);
        if (base_index >= base_array_it->second.element_slots.size() ||
            base_array_it->second.element_type != slot_type_it->second) {
          return std::nullopt;
        }
        return LocalMemsetArrayView{
            .element_type = slot_type_it->second,
            .element_slots = base_array_it->second.element_slots,
            .base_index = base_index,
        };
      }

      const auto base_aggregate_it = local_aggregate_slots.find(base_name);
      if (base_aggregate_it == local_aggregate_slots.end()) {
        return std::nullopt;
      }

      const auto extent = find_repeated_aggregate_extent_at_offset(
          base_aggregate_it->second.storage_type_text,
          static_cast<std::size_t>(*base_offset),
          render_type(slot_type_it->second),
          type_decls);
      if (!extent.has_value()) {
        return std::nullopt;
      }

      std::vector<std::string> element_slots;
      element_slots.reserve(extent->element_count);
      for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
        const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
            static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
        if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
          return std::nullopt;
        }
        element_slots.push_back(leaf_it->second);
      }

      return LocalMemsetArrayView{
          .element_type = slot_type_it->second,
          .element_slots = std::move(element_slots),
          .base_index = 0,
      };
    };
    const auto resolve_local_memset_scalar_slot =
        [&](std::string_view operand) -> std::optional<LocalMemsetScalarSlot> {
      const auto ptr_it = local_pointer_slots.find(std::string(operand));
      if (ptr_it == local_pointer_slots.end()) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return std::nullopt;
      }

      return LocalMemsetScalarSlot{
          .slot_name = ptr_it->second,
          .size_bytes = slot_size,
      };
    };

    if (const auto aggregate_it = local_aggregate_slots.find(std::string(dst_operand));
        aggregate_it != local_aggregate_slots.end()) {
      const auto aggregate_layout =
          lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
      if (!aggregate_layout.has_value()) {
        return false;
      }
      return fill_leaf_slots(
          collect_sorted_leaf_slots(aggregate_it->second), aggregate_layout->size_bytes);
    }

    const auto array_view = resolve_local_memset_array_view(dst_operand);
    if (array_view.has_value()) {
      if (array_view->base_index > array_view->element_slots.size()) {
        return false;
      }

      const auto element_size = type_size_bytes(array_view->element_type);
      if (element_size == 0) {
        return false;
      }

      const auto target_count = array_view->element_slots.size() - array_view->base_index;
      std::vector<std::pair<std::size_t, std::string>> leaf_slots;
      leaf_slots.reserve(target_count);
      for (std::size_t index = 0; index < target_count; ++index) {
        leaf_slots.emplace_back(index * element_size,
                                array_view->element_slots[array_view->base_index + index]);
      }
      return fill_leaf_slots(leaf_slots, target_count * element_size);
    }

    const auto scalar_slot = resolve_local_memset_scalar_slot(dst_operand);
    if (!scalar_slot.has_value()) {
      return false;
    }

    std::vector<std::pair<std::size_t, std::string>> leaf_slots;
    leaf_slots.emplace_back(0, scalar_slot->slot_name);
    return fill_leaf_slots(leaf_slots, scalar_slot->size_bytes);
  };

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

    const std::string dst_operand(memcpy->dst.str());
    const std::string src_operand(memcpy->src.str());
    struct LocalMemcpyArrayView {
      bir::TypeKind element_type = bir::TypeKind::Void;
      std::vector<std::string> element_slots;
      std::size_t base_index = 0;
    };
    struct LocalMemcpyLeaf {
      std::size_t byte_offset = 0;
      bir::TypeKind type = bir::TypeKind::Void;
      std::string slot_name;
    };
    struct LocalMemcpyLeafView {
      std::size_t size_bytes = 0;
      std::vector<LocalMemcpyLeaf> leaves;
    };
    struct LocalMemcpyScalarSlot {
      std::string slot_name;
      bir::TypeKind type = bir::TypeKind::Void;
      std::size_t size_bytes = 0;
      std::size_t align_bytes = 0;
    };
    const auto resolve_local_memcpy_array_view =
        [&](std::string_view operand) -> std::optional<LocalMemcpyArrayView> {
      if (const auto array_it = local_array_slots.find(std::string(operand));
          array_it != local_array_slots.end()) {
        return LocalMemcpyArrayView{
            .element_type = array_it->second.element_type,
            .element_slots = array_it->second.element_slots,
            .base_index = 0,
        };
      }
      const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
      if (array_base_it != local_pointer_array_bases.end() &&
          array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
        const auto slot_type_it =
            local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
        if (slot_type_it != local_slot_types.end()) {
          return LocalMemcpyArrayView{
              .element_type = slot_type_it->second,
              .element_slots = array_base_it->second.element_slots,
              .base_index = array_base_it->second.base_index,
          };
        }
      }

      const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
      if (ptr_slot_it == local_pointer_slots.end()) {
        return std::nullopt;
      }
      const auto dot = ptr_slot_it->second.rfind('.');
      if (dot == std::string::npos) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const std::string base_name = ptr_slot_it->second.substr(0, dot);
      const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
      if (!base_offset.has_value() || *base_offset < 0) {
        return std::nullopt;
      }

      if (const auto base_array_it = local_array_slots.find(base_name);
          base_array_it != local_array_slots.end()) {
        const auto base_index = static_cast<std::size_t>(*base_offset);
        if (base_index >= base_array_it->second.element_slots.size() ||
            base_array_it->second.element_type != slot_type_it->second) {
          return std::nullopt;
        }
        return LocalMemcpyArrayView{
            .element_type = slot_type_it->second,
            .element_slots = base_array_it->second.element_slots,
            .base_index = base_index,
        };
      }

      const auto base_aggregate_it = local_aggregate_slots.find(base_name);
      if (base_aggregate_it == local_aggregate_slots.end()) {
        return std::nullopt;
      }

      const auto extent = find_repeated_aggregate_extent_at_offset(
          base_aggregate_it->second.storage_type_text,
          static_cast<std::size_t>(*base_offset),
          render_type(slot_type_it->second),
          type_decls);
      if (!extent.has_value()) {
        return std::nullopt;
      }

      std::vector<std::string> element_slots;
      element_slots.reserve(extent->element_count);
      for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
        const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
            static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
        if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
          return std::nullopt;
        }
        element_slots.push_back(leaf_it->second);
      }

      return LocalMemcpyArrayView{
          .element_type = slot_type_it->second,
          .element_slots = std::move(element_slots),
          .base_index = 0,
      };
    };
    const auto resolve_local_memcpy_leaf_view =
        [&](std::string_view operand) -> std::optional<LocalMemcpyLeafView> {
      if (const auto aggregate_it = local_aggregate_slots.find(std::string(operand));
          aggregate_it != local_aggregate_slots.end()) {
        const auto aggregate_layout =
            lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
        if (!aggregate_layout.has_value()) {
          return std::nullopt;
        }
        LocalMemcpyLeafView view{
            .size_bytes = aggregate_layout->size_bytes,
        };
        const auto leaf_slots = collect_sorted_leaf_slots(aggregate_it->second);
        view.leaves.reserve(leaf_slots.size());
        for (const auto& [byte_offset, slot_name] : leaf_slots) {
          const auto slot_type_it = local_slot_types.find(slot_name);
          if (slot_type_it == local_slot_types.end()) {
            return std::nullopt;
          }
          view.leaves.push_back(LocalMemcpyLeaf{
              .byte_offset = byte_offset,
              .type = slot_type_it->second,
              .slot_name = slot_name,
          });
        }
        return view;
      }

      const auto array_view = resolve_local_memcpy_array_view(operand);
      if (!array_view.has_value() || array_view->base_index > array_view->element_slots.size()) {
        return std::nullopt;
      }

      const auto element_size = type_size_bytes(array_view->element_type);
      if (element_size == 0) {
        return std::nullopt;
      }

      const auto count = array_view->element_slots.size() - array_view->base_index;
      LocalMemcpyLeafView view{
          .size_bytes = count * element_size,
      };
      view.leaves.reserve(count);
      for (std::size_t index = 0; index < count; ++index) {
        const auto& slot_name = array_view->element_slots[array_view->base_index + index];
        const auto slot_type_it = local_slot_types.find(slot_name);
        if (slot_type_it == local_slot_types.end() || slot_type_it->second != array_view->element_type) {
          return std::nullopt;
        }
        view.leaves.push_back(LocalMemcpyLeaf{
            .byte_offset = index * element_size,
            .type = array_view->element_type,
            .slot_name = slot_name,
        });
      }
      return view;
    };
    const auto resolve_local_memcpy_scalar_slot =
        [&](std::string_view operand) -> std::optional<LocalMemcpyScalarSlot> {
      const auto ptr_it = local_pointer_slots.find(std::string(operand));
      if (ptr_it == local_pointer_slots.end()) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return std::nullopt;
      }

      auto slot_align = slot_size;
      if (const auto slot_it = std::find_if(lowered_function->local_slots.begin(),
                                            lowered_function->local_slots.end(),
                                            [&](const bir::LocalSlot& slot) {
                                              return slot.name == ptr_it->second;
                                            });
          slot_it != lowered_function->local_slots.end() && slot_it->align_bytes != 0) {
        slot_align = slot_it->align_bytes;
      }

      return LocalMemcpyScalarSlot{
          .slot_name = ptr_it->second,
          .type = slot_type_it->second,
          .size_bytes = slot_size,
          .align_bytes = slot_align,
      };
    };
    const auto append_local_leaf_copy = [&](const LocalMemcpyLeafView& source_view,
                                            const LocalMemcpyLeafView& target_view) -> bool {
      const auto collect_requested_prefix =
          [&](const LocalMemcpyLeafView& view) -> std::optional<std::vector<LocalMemcpyLeaf>> {
        if (requested_size > view.size_bytes) {
          return std::nullopt;
        }

        std::vector<LocalMemcpyLeaf> prefix;
        prefix.reserve(view.leaves.size());
        std::size_t covered_bytes = 0;
        for (const auto& leaf : view.leaves) {
          if (covered_bytes == requested_size) {
            break;
          }
          if (leaf.byte_offset != covered_bytes) {
            return std::nullopt;
          }

          const auto leaf_size = type_size_bytes(leaf.type);
          if (leaf_size == 0 || leaf.byte_offset + leaf_size > requested_size) {
            return std::nullopt;
          }

          prefix.push_back(leaf);
          covered_bytes += leaf_size;
        }

        if (covered_bytes != requested_size) {
          return std::nullopt;
        }
        return prefix;
      };

      const auto source_prefix = collect_requested_prefix(source_view);
      const auto target_prefix = collect_requested_prefix(target_view);
      if (!source_prefix.has_value() || !target_prefix.has_value() ||
          source_prefix->size() != target_prefix->size()) {
        return false;
      }

      for (std::size_t index = 0; index < target_prefix->size(); ++index) {
        const auto& source_leaf = (*source_prefix)[index];
        const auto& target_leaf = (*target_prefix)[index];
        if (source_leaf.byte_offset != target_leaf.byte_offset || source_leaf.type != target_leaf.type) {
          return false;
        }
        const std::string copy_name =
            dst_operand + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(target_leaf.type, copy_name),
            .slot_name = source_leaf.slot_name,
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = target_leaf.slot_name,
            .value = bir::Value::named(target_leaf.type, copy_name),
        });
      }
      return true;
    };
    const auto append_leaf_view_to_scalar_slot =
        [&](const LocalMemcpyLeafView& source_view,
            const LocalMemcpyScalarSlot& target_slot) -> bool {
      if (requested_size != target_slot.size_bytes) {
        return false;
      }

      std::size_t covered_bytes = 0;
      for (const auto& source_leaf : source_view.leaves) {
        if (covered_bytes == requested_size) {
          break;
        }
        const auto leaf_size = type_size_bytes(source_leaf.type);
        if (leaf_size == 0 || source_leaf.byte_offset != covered_bytes ||
            source_leaf.byte_offset + leaf_size > requested_size) {
          return false;
        }

        const std::string copy_name =
            target_slot.slot_name + ".memcpy.copy." + std::to_string(source_leaf.byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(source_leaf.type, copy_name),
            .slot_name = source_leaf.slot_name,
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = source_leaf.slot_name,
            .value = bir::Value::named(source_leaf.type, copy_name),
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                    .base_name = target_slot.slot_name,
                    .byte_offset = static_cast<std::int64_t>(source_leaf.byte_offset),
                    .size_bytes = leaf_size,
                    .align_bytes = std::min(target_slot.align_bytes, leaf_size),
                },
        });
        covered_bytes += leaf_size;
      }
      return covered_bytes == requested_size;
    };
    const auto append_scalar_slot_to_leaf_view =
        [&](const LocalMemcpyScalarSlot& source_slot,
            const LocalMemcpyLeafView& target_view) -> bool {
      if (requested_size != source_slot.size_bytes) {
        return false;
      }

      std::size_t covered_bytes = 0;
      for (const auto& target_leaf : target_view.leaves) {
        if (covered_bytes == requested_size) {
          break;
        }
        const auto leaf_size = type_size_bytes(target_leaf.type);
        if (leaf_size == 0 || target_leaf.byte_offset != covered_bytes ||
            target_leaf.byte_offset + leaf_size > requested_size) {
          return false;
        }

        const std::string copy_name =
            target_leaf.slot_name + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(target_leaf.type, copy_name),
            .slot_name = target_leaf.slot_name,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                    .base_name = source_slot.slot_name,
                    .byte_offset = static_cast<std::int64_t>(target_leaf.byte_offset),
                    .size_bytes = leaf_size,
                    .align_bytes = std::min(source_slot.align_bytes, leaf_size),
                },
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = target_leaf.slot_name,
            .value = bir::Value::named(target_leaf.type, copy_name),
        });
        covered_bytes += leaf_size;
      }
      return covered_bytes == requested_size;
    };
    const auto target_view = resolve_local_memcpy_leaf_view(dst_operand);
    if (target_view.has_value()) {
      const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
      if (source_view.has_value() && append_local_leaf_copy(*source_view, *target_view)) {
        return true;
      }
      const auto source_scalar_slot = resolve_local_memcpy_scalar_slot(src_operand);
      return source_scalar_slot.has_value() &&
             append_scalar_slot_to_leaf_view(*source_scalar_slot, *target_view);
    }

    if (const auto target_scalar_slot = resolve_local_memcpy_scalar_slot(dst_operand);
        target_scalar_slot.has_value()) {
      const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
      return source_view.has_value() &&
             append_leaf_view_to_scalar_slot(*source_view, *target_scalar_slot);
    }

    return false;
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

    return try_lower_immediate_local_memset(
        memset->dst.str(),
        static_cast<std::uint8_t>(fill_value->immediate & 0xff),
        static_cast<std::size_t>(fill_size->immediate));
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
    bool is_indirect_call = false;
    bool is_variadic_call = false;
    std::optional<std::string> sret_slot_name;

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
      struct LocalMemcpyArrayView {
        bir::TypeKind element_type = bir::TypeKind::Void;
        std::vector<std::string> element_slots;
        std::size_t base_index = 0;
      };
      struct LocalMemcpyLeaf {
        std::size_t byte_offset = 0;
        bir::TypeKind type = bir::TypeKind::Void;
        std::string slot_name;
      };
      struct LocalMemcpyLeafView {
        std::size_t size_bytes = 0;
        std::vector<LocalMemcpyLeaf> leaves;
      };
      struct LocalMemcpyScalarSlot {
        std::string slot_name;
        bir::TypeKind type = bir::TypeKind::Void;
        std::size_t size_bytes = 0;
        std::size_t align_bytes = 0;
      };
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
      const auto resolve_local_memcpy_array_view =
          [&](std::string_view operand) -> std::optional<LocalMemcpyArrayView> {
        if (const auto array_it = local_array_slots.find(std::string(operand));
            array_it != local_array_slots.end()) {
          return LocalMemcpyArrayView{
              .element_type = array_it->second.element_type,
              .element_slots = array_it->second.element_slots,
              .base_index = 0,
          };
        }
        const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
        if (array_base_it != local_pointer_array_bases.end() &&
            array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
          const auto slot_type_it =
              local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
          if (slot_type_it != local_slot_types.end()) {
            return LocalMemcpyArrayView{
                .element_type = slot_type_it->second,
                .element_slots = array_base_it->second.element_slots,
                .base_index = array_base_it->second.base_index,
            };
          }
        }

        const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
        if (ptr_slot_it == local_pointer_slots.end()) {
          return std::nullopt;
        }
        const auto dot = ptr_slot_it->second.rfind('.');
        if (dot == std::string::npos) {
          return std::nullopt;
        }

        const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
        if (slot_type_it == local_slot_types.end()) {
          return std::nullopt;
        }

        const std::string base_name = ptr_slot_it->second.substr(0, dot);
        const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
        if (!base_offset.has_value() || *base_offset < 0) {
          return std::nullopt;
        }

        if (const auto base_array_it = local_array_slots.find(base_name);
            base_array_it != local_array_slots.end()) {
          const auto base_index = static_cast<std::size_t>(*base_offset);
          if (base_index >= base_array_it->second.element_slots.size() ||
              base_array_it->second.element_type != slot_type_it->second) {
            return std::nullopt;
          }
          return LocalMemcpyArrayView{
              .element_type = slot_type_it->second,
              .element_slots = base_array_it->second.element_slots,
              .base_index = base_index,
          };
        }

        const auto base_aggregate_it = local_aggregate_slots.find(base_name);
        if (base_aggregate_it == local_aggregate_slots.end()) {
          return std::nullopt;
        }

        const auto extent = find_repeated_aggregate_extent_at_offset(
            base_aggregate_it->second.storage_type_text,
            static_cast<std::size_t>(*base_offset),
            render_type(slot_type_it->second),
            type_decls);
        if (!extent.has_value()) {
          return std::nullopt;
        }

        std::vector<std::string> element_slots;
        element_slots.reserve(extent->element_count);
        for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
          const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
              static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
          if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
            return std::nullopt;
          }
          element_slots.push_back(leaf_it->second);
        }

        return LocalMemcpyArrayView{
            .element_type = slot_type_it->second,
            .element_slots = std::move(element_slots),
            .base_index = 0,
        };
      };
      const auto resolve_local_memcpy_leaf_view =
          [&](std::string_view operand) -> std::optional<LocalMemcpyLeafView> {
        if (const auto aggregate_it = local_aggregate_slots.find(std::string(operand));
            aggregate_it != local_aggregate_slots.end()) {
          const auto aggregate_layout =
              lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
          if (!aggregate_layout.has_value()) {
            return std::nullopt;
          }
          LocalMemcpyLeafView view{
              .size_bytes = aggregate_layout->size_bytes,
          };
          const auto leaf_slots = collect_sorted_leaf_slots(aggregate_it->second);
          view.leaves.reserve(leaf_slots.size());
          for (const auto& [byte_offset, slot_name] : leaf_slots) {
            const auto slot_type_it = local_slot_types.find(slot_name);
            if (slot_type_it == local_slot_types.end()) {
              return std::nullopt;
            }
            view.leaves.push_back(LocalMemcpyLeaf{
                .byte_offset = byte_offset,
                .type = slot_type_it->second,
                .slot_name = slot_name,
            });
          }
          return view;
        }

        const auto array_view = resolve_local_memcpy_array_view(operand);
        if (!array_view.has_value() || array_view->base_index > array_view->element_slots.size()) {
          return std::nullopt;
        }

        const auto element_size = type_size_bytes(array_view->element_type);
        if (element_size == 0) {
          return std::nullopt;
        }

        const auto count = array_view->element_slots.size() - array_view->base_index;
        LocalMemcpyLeafView view{
            .size_bytes = count * element_size,
        };
        view.leaves.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
          const auto& slot_name = array_view->element_slots[array_view->base_index + index];
          const auto slot_type_it = local_slot_types.find(slot_name);
          if (slot_type_it == local_slot_types.end() || slot_type_it->second != array_view->element_type) {
            return std::nullopt;
          }
          view.leaves.push_back(LocalMemcpyLeaf{
              .byte_offset = index * element_size,
              .type = slot_type_it->second,
              .slot_name = slot_name,
          });
        }
        return view;
      };
      const auto resolve_local_memcpy_scalar_slot =
          [&](std::string_view operand) -> std::optional<LocalMemcpyScalarSlot> {
        const auto ptr_it = local_pointer_slots.find(std::string(operand));
        if (ptr_it == local_pointer_slots.end()) {
          return std::nullopt;
        }

        const auto slot_type_it = local_slot_types.find(ptr_it->second);
        if (slot_type_it == local_slot_types.end()) {
          return std::nullopt;
        }

        const auto slot_size = type_size_bytes(slot_type_it->second);
        if (slot_size == 0) {
          return std::nullopt;
        }

        auto slot_align = slot_size;
        if (const auto slot_it = std::find_if(lowered_function->local_slots.begin(),
                                              lowered_function->local_slots.end(),
                                              [&](const bir::LocalSlot& slot) {
                                                return slot.name == ptr_it->second;
                                              });
            slot_it != lowered_function->local_slots.end() && slot_it->align_bytes != 0) {
          slot_align = slot_it->align_bytes;
        }

        return LocalMemcpyScalarSlot{
            .slot_name = ptr_it->second,
            .type = slot_type_it->second,
            .size_bytes = slot_size,
            .align_bytes = slot_align,
        };
      };
      const auto append_local_leaf_copy = [&](const LocalMemcpyLeafView& source_view,
                                              const LocalMemcpyLeafView& target_view) -> bool {
        const auto collect_requested_prefix =
            [&](const LocalMemcpyLeafView& view) -> std::optional<std::vector<LocalMemcpyLeaf>> {
          if (requested_size > view.size_bytes) {
            return std::nullopt;
          }

          std::vector<LocalMemcpyLeaf> prefix;
          prefix.reserve(view.leaves.size());
          std::size_t covered_bytes = 0;
          for (const auto& leaf : view.leaves) {
            if (covered_bytes == requested_size) {
              break;
            }
            if (leaf.byte_offset != covered_bytes) {
              return std::nullopt;
            }

            const auto leaf_size = type_size_bytes(leaf.type);
            if (leaf_size == 0 || leaf.byte_offset + leaf_size > requested_size) {
              return std::nullopt;
            }

            prefix.push_back(leaf);
            covered_bytes += leaf_size;
          }

          if (covered_bytes != requested_size) {
            return std::nullopt;
          }
          return prefix;
        };

        const auto source_prefix = collect_requested_prefix(source_view);
        const auto target_prefix = collect_requested_prefix(target_view);
        if (!source_prefix.has_value() || !target_prefix.has_value() ||
            source_prefix->size() != target_prefix->size()) {
          return false;
        }

        for (std::size_t index = 0; index < target_prefix->size(); ++index) {
          const auto& source_leaf = (*source_prefix)[index];
          const auto& target_leaf = (*target_prefix)[index];
          if (source_leaf.byte_offset != target_leaf.byte_offset || source_leaf.type != target_leaf.type) {
            return false;
          }
          const std::string copy_name =
              dst_operand + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(target_leaf.type, copy_name),
              .slot_name = source_leaf.slot_name,
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = target_leaf.slot_name,
              .value = bir::Value::named(target_leaf.type, copy_name),
          });
        }
        return true;
      };
      const auto append_leaf_view_to_scalar_slot =
          [&](const LocalMemcpyLeafView& source_view,
              const LocalMemcpyScalarSlot& target_slot) -> bool {
        if (requested_size != target_slot.size_bytes) {
          return false;
        }

        std::size_t covered_bytes = 0;
        for (const auto& source_leaf : source_view.leaves) {
          if (covered_bytes == requested_size) {
            break;
          }
          const auto leaf_size = type_size_bytes(source_leaf.type);
          if (leaf_size == 0 || source_leaf.byte_offset != covered_bytes ||
              source_leaf.byte_offset + leaf_size > requested_size) {
            return false;
          }

          const std::string copy_name =
              target_slot.slot_name + ".memcpy.copy." + std::to_string(source_leaf.byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(source_leaf.type, copy_name),
              .slot_name = source_leaf.slot_name,
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = source_leaf.slot_name,
              .value = bir::Value::named(source_leaf.type, copy_name),
              .address =
                  bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                      .base_name = target_slot.slot_name,
                      .byte_offset = static_cast<std::int64_t>(source_leaf.byte_offset),
                      .size_bytes = leaf_size,
                      .align_bytes = std::min(target_slot.align_bytes, leaf_size),
                  },
          });
          covered_bytes += leaf_size;
        }
        return covered_bytes == requested_size;
      };
      const auto append_scalar_slot_to_leaf_view =
          [&](const LocalMemcpyScalarSlot& source_slot,
              const LocalMemcpyLeafView& target_view) -> bool {
        if (requested_size != source_slot.size_bytes) {
          return false;
        }

        std::size_t covered_bytes = 0;
        for (const auto& target_leaf : target_view.leaves) {
          if (covered_bytes == requested_size) {
            break;
          }
          const auto leaf_size = type_size_bytes(target_leaf.type);
          if (leaf_size == 0 || target_leaf.byte_offset != covered_bytes ||
              target_leaf.byte_offset + leaf_size > requested_size) {
            return false;
          }

          const std::string copy_name =
              target_leaf.slot_name + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(target_leaf.type, copy_name),
              .slot_name = target_leaf.slot_name,
              .address =
                  bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                      .base_name = source_slot.slot_name,
                      .byte_offset = static_cast<std::int64_t>(target_leaf.byte_offset),
                      .size_bytes = leaf_size,
                      .align_bytes = std::min(source_slot.align_bytes, leaf_size),
                  },
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = target_leaf.slot_name,
              .value = bir::Value::named(target_leaf.type, copy_name),
          });
          covered_bytes += leaf_size;
        }
        return covered_bytes == requested_size;
      };
      const auto target_view = resolve_local_memcpy_leaf_view(dst_operand);
      if (target_view.has_value()) {
        const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
        if (source_view.has_value() && append_local_leaf_copy(*source_view, *target_view)) {
          return alias_memcpy_result();
        }
        const auto source_scalar_slot = resolve_local_memcpy_scalar_slot(src_operand);
        if (!source_scalar_slot.has_value() ||
            !append_scalar_slot_to_leaf_view(*source_scalar_slot, *target_view)) {
          return fail_memcpy_family();
        }
        return alias_memcpy_result();
      }

      if (const auto target_scalar_slot = resolve_local_memcpy_scalar_slot(dst_operand);
          target_scalar_slot.has_value()) {
        const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
        if (!source_view.has_value() ||
            !append_leaf_view_to_scalar_slot(*source_view, *target_scalar_slot)) {
          return fail_memcpy_family();
        }
        return alias_memcpy_result();
      }

      return fail_memcpy_family();
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
      if (!try_lower_immediate_local_memset(
              dst_operand,
              static_cast<std::uint8_t>(fill_value->immediate & 0xff),
              static_cast<std::size_t>(fill_size->immediate))) {
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
        callee_name = std::move(semantic_direct_callee);
        is_variadic_call = parsed_call->is_variadic;
        lowered_args.reserve(parsed_call->typed_call.args.size());
        lowered_arg_types.reserve(parsed_call->typed_call.param_types.size());
        lowered_arg_abi.reserve(parsed_call->typed_call.param_types.size());
        for (std::size_t index = 0; index < parsed_call->typed_call.args.size(); ++index) {
          const auto trimmed_param_type =
              c4c::codegen::lir::trim_lir_arg_text(parsed_call->typed_call.param_types[index]);
          if (trimmed_param_type == "ptr") {
            const auto arg = lower_call_pointer_arg_value(
                c4c::codegen::lir::LirOperand(
                    std::string(parsed_call->typed_call.args[index].operand)),
                value_aliases,
                local_aggregate_slots,
                global_types,
                function_symbols);
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
            const std::string operand_text(parsed_call->typed_call.args[index].operand);
            const auto aggregate_alias_it = aggregate_value_aliases.find(operand_text);
            if (aggregate_alias_it == aggregate_value_aliases.end() ||
                local_aggregate_slots.find(aggregate_alias_it->second) ==
                    local_aggregate_slots.end()) {
              return fail_call_family(call_family);
            }
            lowered_arg_types.push_back(bir::TypeKind::Ptr);
            lowered_args.push_back(
                bir::Value::named(bir::TypeKind::Ptr, aggregate_alias_it->second));
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
    } else if (call->callee.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      call_family = kIndirectCallFamily;
      const auto parsed_call = parse_typed_call(*call);
      if (!parsed_call.has_value()) {
        return fail_call_family(call_family);
      }
      is_variadic_call = parsed_call->is_variadic;
      callee_value = lower_value(call->callee, bir::TypeKind::Ptr, value_aliases);
      if (!callee_value.has_value()) {
        return fail_call_family(call_family);
      }
      lowered_args.reserve(parsed_call->args.size());
      lowered_arg_types.reserve(parsed_call->param_types.size());
      lowered_arg_abi.reserve(parsed_call->param_types.size());
      for (std::size_t index = 0; index < parsed_call->args.size(); ++index) {
        const auto trimmed_param_type =
            c4c::codegen::lir::trim_lir_arg_text(parsed_call->param_types[index]);
        if (trimmed_param_type == "ptr") {
          const auto arg = lower_call_pointer_arg_value(
              c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)),
              value_aliases,
              local_aggregate_slots,
              global_types,
              function_symbols);
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
          const std::string operand_text(parsed_call->args[index].operand);
          const auto aggregate_alias_it = aggregate_value_aliases.find(operand_text);
          if (aggregate_alias_it == aggregate_value_aliases.end() ||
              local_aggregate_slots.find(aggregate_alias_it->second) == local_aggregate_slots.end()) {
            return fail_call_family(call_family);
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(bir::Value::named(bir::TypeKind::Ptr, aggregate_alias_it->second));
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
    } else {
      return false;
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
    return true;
  }

  return false;
}
}  // namespace c4c::backend
