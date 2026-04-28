#include "lowering.hpp"

#include <algorithm>

namespace c4c::backend {

namespace {

std::string_view resolve_link_name(const c4c::LinkNameTable& link_names,
                                   c4c::LinkNameId id) {
  if (id == c4c::kInvalidLinkName) return {};
  const std::string_view spelling = link_names.spelling(id);
  return spelling.empty() ? std::string_view{} : spelling;
}

std::string function_name_for_reporting(const c4c::codegen::lir::LirModule& module,
                                        const c4c::codegen::lir::LirFunction& function) {
  const std::string_view resolved_name = resolve_link_name(module.link_names,
                                                           function.link_name_id);
  if (!resolved_name.empty()) {
    return std::string(resolved_name);
  }
  return function.name;
}

std::optional<std::string> parse_byval_pointee_type(std::string_view type_text) {
  constexpr std::string_view kPrefix = "ptr byval(";

  auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type_text);
  if (trimmed.size() <= kPrefix.size() || trimmed.substr(0, kPrefix.size()) != kPrefix) {
    return std::nullopt;
  }

  const auto body = trimmed.substr(kPrefix.size());
  int paren_depth = 1;
  int bracket_depth = 0;
  int brace_depth = 0;
  int angle_depth = 0;
  for (std::size_t index = 0; index < body.size(); ++index) {
    switch (body[index]) {
      case '(':
        if (bracket_depth == 0 && brace_depth == 0 && angle_depth == 0) {
          ++paren_depth;
        }
        break;
      case ')':
        if (bracket_depth == 0 && brace_depth == 0 && angle_depth == 0) {
          --paren_depth;
          if (paren_depth == 0) {
            const auto pointee =
                c4c::codegen::lir::trim_lir_arg_text(body.substr(0, index));
            if (pointee.empty()) {
              return std::nullopt;
            }
            return std::string(pointee);
          }
        }
        break;
      case '[':
        ++bracket_depth;
        break;
      case ']':
        if (bracket_depth > 0) {
          --bracket_depth;
        }
        break;
      case '{':
        ++brace_depth;
        break;
      case '}':
        if (brace_depth > 0) {
          --brace_depth;
        }
        break;
      case '<':
        ++angle_depth;
        break;
      case '>':
        if (angle_depth > 0) {
          --angle_depth;
        }
        break;
      default:
        break;
    }
  }

  return std::nullopt;
}

}  // namespace

using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::compute_call_arg_abi;
using lir_to_bir_detail::compute_function_return_abi;

std::optional<bir::TypeKind> BirFunctionLowerer::lower_minimal_scalar_type(const c4c::TypeSpec& type) {
  if (type.ptr_level != 0 || type.array_rank != 0) {
    return std::nullopt;
  }
  if (type.base == TB_FLOAT) {
    return bir::TypeKind::F32;
  }
  if (type.base == TB_DOUBLE) {
    return bir::TypeKind::F64;
  }
  if (type.base == TB_CHAR || type.base == TB_SCHAR || type.base == TB_UCHAR) {
    return bir::TypeKind::I8;
  }
  if (type.base == TB_INT) {
    return bir::TypeKind::I32;
  }
  if (type.base == TB_LONG || type.base == TB_ULONG || type.base == TB_LONGLONG ||
      type.base == TB_ULONGLONG) {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

std::optional<BirFunctionLowerer::ParsedTypedCall> BirFunctionLowerer::parse_typed_call(
    const c4c::codegen::lir::LirCallOp& call) {
  if (const auto param_types =
          c4c::codegen::lir::parse_lir_call_param_types(call.callee_type_suffix);
      param_types.has_value()) {
    bool saw_varargs = false;
    std::size_t fixed_param_count = 0;
    for (auto type : *param_types) {
      const auto trimmed_type = c4c::codegen::lir::trim_lir_arg_text(type);
      if (trimmed_type == "...") {
        saw_varargs = true;
        break;
      }
      ++fixed_param_count;
    }

    if (saw_varargs) {
      const auto args = c4c::codegen::lir::parse_lir_typed_call_args(call.args_str);
      if (!args.has_value() || args->size() < fixed_param_count) {
        return std::nullopt;
      }

      ParsedTypedCall parsed;
      parsed.is_variadic = true;
      parsed.owned_param_types.reserve(args->size());
      parsed.param_types.reserve(args->size());
      parsed.args.reserve(args->size());
      for (std::size_t index = 0; index < args->size(); ++index) {
        const auto arg_type = c4c::codegen::lir::trim_lir_arg_text((*args)[index].type);
        if (index < fixed_param_count) {
          const auto expected_type = c4c::codegen::lir::trim_lir_arg_text((*param_types)[index]);
          if (expected_type == arg_type) {
            parsed.owned_param_types.push_back(std::string(expected_type));
          } else if (expected_type == "ptr") {
            const auto byval_type = parse_byval_pointee_type(arg_type);
            if (!byval_type.has_value()) {
              return std::nullopt;
            }
            parsed.owned_param_types.push_back(*byval_type);
          } else {
            return std::nullopt;
          }
        } else {
          parsed.owned_param_types.push_back(std::string(arg_type));
        }
        parsed.param_types.push_back(parsed.owned_param_types.back());
        parsed.args.push_back((*args)[index]);
      }
      return parsed;
    }
  }

  if (const auto parsed = c4c::codegen::lir::parse_lir_typed_call_or_infer_params(call);
      parsed.has_value()) {
    ParsedTypedCall view;
    view.param_types = parsed->param_types;
    view.args = parsed->args;
    return view;
  }

  const auto param_types = c4c::codegen::lir::parse_lir_call_param_types(call.callee_type_suffix);
  const auto args = c4c::codegen::lir::parse_lir_typed_call_args(call.args_str);
  if (!param_types.has_value() || !args.has_value() || param_types->size() != args->size()) {
    return std::nullopt;
  }

  ParsedTypedCall parsed;
  parsed.owned_param_types.reserve(param_types->size());
  parsed.param_types.reserve(param_types->size());
  parsed.args = *args;
  for (std::size_t index = 0; index < param_types->size(); ++index) {
    const auto expected_type = c4c::codegen::lir::trim_lir_arg_text((*param_types)[index]);
    const auto arg_type = c4c::codegen::lir::trim_lir_arg_text((*args)[index].type);
    if (expected_type == arg_type) {
      parsed.owned_param_types.push_back(std::string(expected_type));
      parsed.param_types.push_back(parsed.owned_param_types.back());
      continue;
    }
    if (expected_type == "ptr") {
      const auto byval_type = parse_byval_pointee_type(arg_type);
      if (byval_type.has_value()) {
        parsed.owned_param_types.push_back(*byval_type);
        parsed.param_types.push_back(parsed.owned_param_types.back());
        continue;
      }
    }
    return std::nullopt;
  }

  return parsed;
}

std::optional<BirFunctionLowerer::ParsedDirectGlobalTypedCall>
BirFunctionLowerer::parse_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call) {
  const auto symbol_name = c4c::codegen::lir::parse_lir_direct_global_callee(call.callee);
  if (!symbol_name.has_value()) {
    return std::nullopt;
  }

  bool signature_is_variadic = false;
  if (const auto param_types =
          c4c::codegen::lir::parse_lir_call_param_types(call.callee_type_suffix);
      param_types.has_value()) {
    signature_is_variadic =
        std::any_of(param_types->begin(), param_types->end(), [](std::string_view type) {
          return c4c::codegen::lir::trim_lir_arg_text(type) == "...";
        });
  }

  if (const auto parsed = c4c::codegen::lir::parse_lir_direct_global_typed_call(call);
      parsed.has_value()) {
    ParsedDirectGlobalTypedCall lowered;
    lowered.symbol_name = *symbol_name;
    lowered.typed_call.param_types = parsed->typed_call.param_types;
    lowered.typed_call.args = parsed->typed_call.args;
    lowered.is_variadic = signature_is_variadic;
    return lowered;
  }

  const auto callee_type_suffix = c4c::codegen::lir::trim_lir_arg_text(call.callee_type_suffix);
  if (callee_type_suffix.empty()) {
    return std::nullopt;
  }

  const auto param_types = c4c::codegen::lir::parse_lir_call_param_types(callee_type_suffix);
  if (!param_types.has_value()) {
    return std::nullopt;
  }

  bool saw_varargs = false;
  std::size_t fixed_param_count = 0;
  for (auto type : *param_types) {
    const auto trimmed_type = c4c::codegen::lir::trim_lir_arg_text(type);
    if (trimmed_type == "...") {
      saw_varargs = true;
      break;
    }
    ++fixed_param_count;
  }

  if (saw_varargs) {
    const auto args = c4c::codegen::lir::parse_lir_typed_call_args(call.args_str);
    if (!args.has_value() || args->size() < fixed_param_count) {
      return std::nullopt;
    }

    ParsedDirectGlobalTypedCall parsed;
    parsed.symbol_name = *symbol_name;
    parsed.is_variadic = true;
    parsed.typed_call.owned_param_types.reserve(args->size());
    parsed.typed_call.param_types.reserve(args->size());
    parsed.typed_call.args.reserve(args->size());
    for (std::size_t index = 0; index < args->size(); ++index) {
      const auto arg_type = c4c::codegen::lir::trim_lir_arg_text((*args)[index].type);
      if (index < fixed_param_count) {
        const auto expected_type = c4c::codegen::lir::trim_lir_arg_text((*param_types)[index]);
        if (expected_type == arg_type) {
          parsed.typed_call.owned_param_types.push_back(std::string(expected_type));
        } else if (expected_type == "ptr") {
          const auto byval_type = parse_byval_pointee_type(arg_type);
          if (!byval_type.has_value()) {
            return std::nullopt;
          }
          parsed.typed_call.owned_param_types.push_back(*byval_type);
        } else {
          return std::nullopt;
        }
      } else if (const auto byval_type = parse_byval_pointee_type(arg_type);
                 byval_type.has_value()) {
        parsed.typed_call.owned_param_types.push_back(*byval_type);
      } else {
        parsed.typed_call.owned_param_types.push_back(std::string(arg_type));
      }
      parsed.typed_call.param_types.push_back(parsed.typed_call.owned_param_types.back());
      parsed.typed_call.args.push_back((*args)[index]);
    }
    return parsed;
  }

  const auto fixed_args = c4c::codegen::lir::parse_lir_typed_call_args(call.args_str);
  if (!fixed_args.has_value() || param_types->size() != fixed_args->size()) {
    return std::nullopt;
  }

  ParsedDirectGlobalTypedCall fixed_parsed;
  fixed_parsed.symbol_name = *symbol_name;
  fixed_parsed.typed_call.owned_param_types.reserve(param_types->size());
  fixed_parsed.typed_call.param_types.reserve(param_types->size());
  fixed_parsed.typed_call.args = *fixed_args;
  for (std::size_t index = 0; index < param_types->size(); ++index) {
    const auto expected_type = c4c::codegen::lir::trim_lir_arg_text((*param_types)[index]);
    const auto arg_type = c4c::codegen::lir::trim_lir_arg_text((*fixed_args)[index].type);
    if (expected_type == arg_type) {
      fixed_parsed.typed_call.owned_param_types.push_back(std::string(expected_type));
      fixed_parsed.typed_call.param_types.push_back(
          fixed_parsed.typed_call.owned_param_types.back());
      continue;
    }
    if (expected_type == "ptr") {
      const auto byval_type = parse_byval_pointee_type(arg_type);
      if (byval_type.has_value()) {
        fixed_parsed.typed_call.owned_param_types.push_back(*byval_type);
        fixed_parsed.typed_call.param_types.push_back(
            fixed_parsed.typed_call.owned_param_types.back());
        continue;
      }
    }
    return std::nullopt;
  }

  return fixed_parsed;
}
std::optional<bir::Function> BirFunctionLowerer::lower_extern_decl(
    const c4c::codegen::lir::LirExternDecl& decl,
    const c4c::TargetProfile& target_profile,
    const TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts) {
  auto return_info = lower_return_info_from_type(
      decl.return_type_str, type_decls, target_profile, &structured_layouts);
  if (!return_info.has_value()) {
    return_info = lower_return_info_from_type(
        decl.return_type.str(), type_decls, target_profile, &structured_layouts);
  }
  if (!return_info.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = decl.name;
  lowered.link_name_id = decl.link_name_id;
  lowered.return_type = return_info->type;
  lowered.return_size_bytes = return_info->size_bytes;
  lowered.return_align_bytes = return_info->align_bytes;
  lowered.return_abi = return_info->abi;
  lowered.is_declaration = true;
  return lowered;
}

bool BirFunctionLowerer::lower_call_inst(const c4c::codegen::lir::LirCallOp& call,
                                         std::vector<bir::Inst>* lowered_insts) {
  auto& value_aliases = value_aliases_;
  auto& aggregate_value_aliases = aggregate_value_aliases_;
  auto& local_pointer_slots = local_pointer_slots_;
  auto& local_pointer_array_bases = local_pointer_array_bases_;
  auto& local_aggregate_slots = local_aggregate_slots_;
  auto& local_slot_types = local_slot_types_;
  auto& local_slot_pointer_values = local_slot_pointer_values_;
  auto& pointer_value_addresses = pointer_value_addresses_;
  const auto& global_types = global_types_;
  const auto& function_symbols = function_symbols_;
  const auto& type_decls = type_decls_;

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
      const auto slot_size = lir_to_bir_detail::type_size_bytes(slot_type_it->second);
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
  constexpr std::string_view kDirectCallFamily = "direct-call semantic family";
  constexpr std::string_view kIndirectCallFamily = "indirect-call semantic family";
  constexpr std::string_view kCallReturnFamily = "call-return semantic family";
  const auto fail_call_family = [&](std::string_view family) -> bool {
    note_semantic_call_family_failure(family);
    return false;
  };

  const auto return_info =
      lower_return_info_from_type(call.return_type.str(),
                                  type_decls,
                                  context_.target_profile,
                                  &structured_layouts_);
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

  const auto maybe_resolve_direct_calloc_pointer_address =
      [&](std::string_view symbol_name,
          const ParsedTypedCall& typed_call) -> std::optional<PointerAddress> {
    if (symbol_name != "calloc" || return_info->returned_via_sret ||
        return_info->type != bir::TypeKind::Ptr ||
        call.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
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
        .base_value = bir::Value::named(bir::TypeKind::Ptr, call.result.str()),
        .dynamic_element_count = static_cast<std::size_t>(count_value->immediate),
        .dynamic_element_stride_bytes = static_cast<std::size_t>(size_value->immediate),
    };
  };

  if (return_info->returned_via_sret) {
    if (call.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_call_family(kCallReturnFamily);
    }
    sret_slot_name = call.result.str();
    if (!declare_local_aggregate_slots(call.return_type.str(),
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
  if (const auto direct_callee = c4c::codegen::lir::parse_lir_direct_global_callee(call.callee);
      direct_callee.has_value()) {
    call_family = kDirectCallFamily;
    const auto resolved_direct_callee_name =
        [&](std::string_view fallback_name) -> std::string {
      if (call.direct_callee_link_name_id != c4c::kInvalidLinkName) {
        const std::string_view semantic_name =
            context_.lir_module.link_names.spelling(call.direct_callee_link_name_id);
        if (!semantic_name.empty()) {
          return std::string(semantic_name);
        }
      }
      return std::string(fallback_name);
    };
    if (const auto inferred_call = parse_typed_call(call); inferred_call.has_value()) {
      const std::string semantic_direct_callee =
          resolved_direct_callee_name(*direct_callee);
      if (const auto lowered_memory_intrinsic = try_lower_direct_memory_intrinsic_call(
              semantic_direct_callee, *inferred_call, call, *return_info, lowered_insts);
          lowered_memory_intrinsic.has_value()) {
        return *lowered_memory_intrinsic;
      }
      if (!returned_pointer_address.has_value()) {
        returned_pointer_address =
            maybe_resolve_direct_calloc_pointer_address(semantic_direct_callee, *inferred_call);
      }
    }

    if (const auto parsed_call = parse_direct_global_typed_call(call);
        parsed_call.has_value()) {
      const std::string semantic_direct_callee =
          resolved_direct_callee_name(parsed_call->symbol_name);
      if (const auto lowered_memory_intrinsic = try_lower_direct_memory_intrinsic_call(
              semantic_direct_callee, parsed_call->typed_call, call, *return_info, lowered_insts);
          lowered_memory_intrinsic.has_value()) {
        return *lowered_memory_intrinsic;
      }
      if (!returned_pointer_address.has_value()) {
        returned_pointer_address = maybe_resolve_direct_calloc_pointer_address(
            semantic_direct_callee, parsed_call->typed_call);
      }
      callee_name = std::string(parsed_call->symbol_name);
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
              *compute_call_arg_abi(context_.target_profile,
                                                       bir::TypeKind::Ptr));
          continue;
        }
        const auto arg_type =
            lower_scalar_or_function_pointer_type(parsed_call->typed_call.param_types[index]);
        if (!arg_type.has_value()) {
          const auto aggregate_layout =
              lower_byval_aggregate_layout(parsed_call->typed_call.param_types[index],
                                           type_decls,
                                           &structured_layouts_);
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
            *compute_call_arg_abi(context_.target_profile, *arg_type));
      }
    } else if (c4c::codegen::lir::trim_lir_arg_text(call.args_str).empty()) {
      callee_name = std::string(*direct_callee);
    } else {
      return fail_call_family(call_family);
    }
  } else {
    call_family = kIndirectCallFamily;
    const auto parsed_call = parse_typed_call(call);
    if (!parsed_call.has_value()) {
      return fail_call_family(call_family);
    }
    is_variadic_call = parsed_call->is_variadic;
    callee_value = lower_value(call.callee, bir::TypeKind::Ptr, value_aliases);
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
            *compute_call_arg_abi(context_.target_profile,
                                                     bir::TypeKind::Ptr));
        continue;
      }
      const auto arg_type =
          lower_scalar_or_function_pointer_type(parsed_call->param_types[index]);
      if (!arg_type.has_value()) {
        const auto aggregate_layout =
            lower_byval_aggregate_layout(parsed_call->param_types[index],
                                         type_decls,
                                         &structured_layouts_);
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
          *compute_call_arg_abi(context_.target_profile, *arg_type));
    }
    is_indirect_call = true;
  }

  bir::CallInst lowered_call;
  if (!return_info->returned_via_sret && return_info->type != bir::TypeKind::Void) {
    if (call.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_call_family(call_family);
    }
    lowered_call.result = bir::Value::named(return_info->type, call.result.str());
  } else if (call.result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    if (!return_info->returned_via_sret) {
      return fail_call_family(call_family);
    }
  }
  if (is_indirect_call) {
    lowered_call.callee_value = std::move(*callee_value);
  } else {
    lowered_call.callee = std::move(*callee_name);
    lowered_call.callee_link_name_id = call.direct_callee_link_name_id;
  }
  lowered_call.args = std::move(lowered_args);
  lowered_call.arg_types = std::move(lowered_arg_types);
  lowered_call.arg_abi = std::move(lowered_arg_abi);
  if (call.return_type.has_struct_name_id()) {
    const auto structured_return_name =
        context_.lir_module.struct_names.spelling(call.return_type.struct_name_id());
    if (!structured_return_name.empty()) {
      lowered_call.structured_return_type_name = std::string(structured_return_name);
    }
  }
  if (!return_info->returned_via_sret ||
      !lowered_call.structured_return_type_name.has_value()) {
    lowered_call.return_type_name =
        return_info->returned_via_sret ? "void" : std::string(call.return_type.str());
  }
  lowered_call.return_type = return_info->type;
  lowered_call.result_abi = return_info->abi;
  lowered_call.is_indirect = is_indirect_call;
  lowered_call.is_variadic = is_variadic_call;
  if (sret_slot_name.has_value()) {
    lowered_call.sret_storage_name = *sret_slot_name;
  }
  lowered_insts->push_back(std::move(lowered_call));
  if (returned_pointer_address.has_value()) {
    pointer_value_addresses[call.result.str()] = *returned_pointer_address;
  }
  return true;
}

bool BirFunctionLowerer::lower_runtime_intrinsic_inst(
    const c4c::codegen::lir::LirInst& inst,
    const ValueMap& value_aliases,
    std::vector<bir::Inst>* lowered_insts) {
  const auto fail_runtime_family = [&](std::string_view family) -> bool {
    note_runtime_intrinsic_family_failure(family);
    return false;
  };
  const auto lower_va_result_type = [](std::string_view type_text) -> std::optional<bir::TypeKind> {
    if (const auto lowered = lower_scalar_or_function_pointer_type(type_text); lowered.has_value()) {
      return lowered;
    }
    const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type_text);
    if (trimmed == "float") {
      return bir::TypeKind::F32;
    }
    if (trimmed == "double") {
      return bir::TypeKind::F64;
    }
    return std::nullopt;
  };
  const auto lower_va_list_call =
      [&](std::string_view callee_name,
          const c4c::codegen::lir::LirOperand& ap_ptr) -> bool {
    const auto lowered_ap = lower_value(ap_ptr, bir::TypeKind::Ptr);
    if (!lowered_ap.has_value()) {
      return fail_runtime_family("variadic runtime family");
    }
    // Runtime helpers are synthesized BIR compatibility calls rather than
    // user/extern symbols, so callee_link_name_id intentionally stays invalid.
    lowered_insts->push_back(bir::CallInst{
        .callee = std::string(callee_name),
        .args = {*lowered_ap},
        .arg_types = {bir::TypeKind::Ptr},
        .arg_abi = {*compute_call_arg_abi(context_.target_profile, bir::TypeKind::Ptr)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    return true;
  };
  const auto lower_va_arg_call =
      [&](const c4c::codegen::lir::LirVaArgOp& va_arg) -> bool {
    if (va_arg.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_runtime_family("variadic runtime family");
    }
    const auto lowered_ap = lower_value(va_arg.ap_ptr, bir::TypeKind::Ptr);
    const auto lowered_type = lower_va_result_type(va_arg.type_str.str());
    if (!lowered_ap.has_value()) {
      return fail_runtime_family("variadic runtime family");
    }
    if (!lowered_type.has_value()) {
      const auto aggregate_layout =
          lower_byval_aggregate_layout(va_arg.type_str.str(), type_decls_, &structured_layouts_);
      if (!aggregate_layout.has_value()) {
        return fail_runtime_family("variadic runtime family");
      }
      if (!declare_local_aggregate_slots(
              va_arg.type_str.str(), va_arg.result.str(), aggregate_layout->align_bytes)) {
        return fail_runtime_family("variadic runtime family");
      }
      aggregate_value_aliases_[va_arg.result.str()] = va_arg.result.str();
      // Runtime helpers are synthesized BIR compatibility calls rather than
      // user/extern symbols, so callee_link_name_id intentionally stays invalid.
      lowered_insts->push_back(bir::CallInst{
          .callee = "llvm.va_arg.aggregate",
          .args = {bir::Value::named(bir::TypeKind::Ptr, va_arg.result.str()), *lowered_ap},
          .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
          .arg_abi =
              {bir::CallArgAbiInfo{
                   .type = bir::TypeKind::Ptr,
                   .size_bytes = aggregate_layout->size_bytes,
                   .align_bytes = aggregate_layout->align_bytes,
                   .primary_class = bir::AbiValueClass::Memory,
                   .sret_pointer = true,
               },
               *compute_call_arg_abi(context_.target_profile, bir::TypeKind::Ptr)},
          .return_type_name = "void",
          .return_type = bir::TypeKind::Void,
      });
      return true;
    }
    lowered_insts->push_back(bir::CallInst{
        .result = bir::Value::named(*lowered_type, va_arg.result.str()),
        .callee =
            "llvm.va_arg." + std::string(c4c::codegen::lir::trim_lir_arg_text(va_arg.type_str.str())),
        .args = {*lowered_ap},
        .arg_types = {bir::TypeKind::Ptr},
        .arg_abi = {*compute_call_arg_abi(context_.target_profile, bir::TypeKind::Ptr)},
        .return_type = *lowered_type,
        .result_abi = compute_function_return_abi(context_.target_profile, *lowered_type, false),
    });
    return true;
  };
  const auto lower_inline_asm_call =
      [&](const c4c::codegen::lir::LirInlineAsmOp& inline_asm) -> bool {
    const auto return_type_text =
        std::string(c4c::codegen::lir::trim_lir_arg_text(inline_asm.ret_type.str()));
    // Inline asm is represented as a runtime placeholder, not a link-name
    // symbol. Its callee_link_name_id intentionally stays invalid.
    bir::CallInst lowered_call{
        .callee = "llvm.inline_asm",
        .return_type_name = return_type_text,
        .return_type = bir::TypeKind::Void,
        .inline_asm = bir::InlineAsmMetadata{
            .asm_text = inline_asm.asm_text,
            .constraints = inline_asm.constraints,
            .args_text = inline_asm.args_str,
            .side_effects = inline_asm.side_effects,
        },
    };

    if (return_type_text != "void") {
      if (inline_asm.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return fail_runtime_family("inline-asm placeholder family");
      }
      const auto lowered_type = lower_scalar_or_function_pointer_type(return_type_text);
      if (!lowered_type.has_value()) {
        return fail_runtime_family("inline-asm placeholder family");
      }
      lowered_call.result = bir::Value::named(*lowered_type, inline_asm.result.str());
      lowered_call.result_abi =
          compute_function_return_abi(context_.target_profile, *lowered_type, false);
      lowered_call.return_type = *lowered_type;
    } else if (!inline_asm.result.empty()) {
      return fail_runtime_family("inline-asm placeholder family");
    }

    if (!inline_asm.args_str.empty()) {
      auto lowered_args = std::vector<bir::Value>{};
      auto lowered_arg_types = std::vector<bir::TypeKind>{};
      for (const auto raw_item :
           lir_to_bir_detail::split_top_level_initializer_items(inline_asm.args_str)) {
        const auto item = c4c::codegen::lir::trim_lir_arg_text(raw_item);
        if (item.empty()) {
          continue;
        }
        const auto parsed_operand = lir_to_bir_detail::parse_typed_operand(item);
        if (!parsed_operand.has_value()) {
          lowered_args.clear();
          lowered_arg_types.clear();
          break;
        }
        const auto arg_type = lower_scalar_or_function_pointer_type(parsed_operand->type_text);
        if (!arg_type.has_value()) {
          lowered_args.clear();
          lowered_arg_types.clear();
          break;
        }
        const auto lowered_arg =
            BirFunctionLowerer::lower_value(parsed_operand->operand, *arg_type, value_aliases);
        if (!lowered_arg.has_value()) {
          lowered_args.clear();
          lowered_arg_types.clear();
          break;
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*lowered_arg);
      }
      if (!lowered_args.empty()) {
        lowered_call.args = std::move(lowered_args);
        lowered_call.arg_types = std::move(lowered_arg_types);
        lowered_call.inline_asm->args_text.clear();
      }
    }

    lowered_insts->push_back(std::move(lowered_call));
    return true;
  };
  const auto lower_fabs_intrinsic_call =
      [&](const c4c::codegen::lir::LirCallOp& call) -> std::optional<bool> {
    const auto parsed_callee = c4c::codegen::lir::parse_lir_call_callee(call.callee.str());
    if (!parsed_callee.has_value() ||
        parsed_callee->kind != c4c::codegen::lir::LirCallCalleeKind::DirectIntrinsic) {
      return std::nullopt;
    }
    const bool is_fabs_family = parsed_callee->symbol_name == "llvm.fabs.float" ||
                                parsed_callee->symbol_name == "llvm.fabs.double" ||
                                parsed_callee->symbol_name == "llvm.fabs.x86_fp80" ||
                                parsed_callee->symbol_name == "llvm.fabs.f128";
    if (!is_fabs_family) {
      return std::nullopt;
    }

    const auto parsed_call = parse_typed_call(call);
    if (!parsed_call.has_value() || parsed_call->args.size() != 1 ||
        parsed_call->param_types.size() != 1) {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    bir::TypeKind value_type = bir::TypeKind::Void;
    const auto param_type = c4c::codegen::lir::trim_lir_arg_text(parsed_call->param_types[0]);
    const auto return_type = c4c::codegen::lir::trim_lir_arg_text(call.return_type.str());
    if (parsed_callee->symbol_name == "llvm.fabs.float" && param_type == "float" &&
        return_type == "float") {
      value_type = bir::TypeKind::F32;
    } else if (parsed_callee->symbol_name == "llvm.fabs.double" && param_type == "double" &&
               return_type == "double") {
      value_type = bir::TypeKind::F64;
    } else if ((parsed_callee->symbol_name == "llvm.fabs.x86_fp80" ||
                parsed_callee->symbol_name == "llvm.fabs.f128") &&
               (param_type == "x86_fp80" || param_type == "f128") &&
               (return_type == "x86_fp80" || return_type == "f128")) {
      value_type = bir::TypeKind::F128;
    } else {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    if (call.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    const auto lowered_arg = lower_value(
        c4c::codegen::lir::LirOperand(std::string(parsed_call->args[0].operand)),
        value_type,
        value_aliases);
    if (!lowered_arg.has_value()) {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    lowered_insts->push_back(bir::CallInst{
        .result = bir::Value::named(value_type, call.result.str()),
        .callee = std::string(parsed_callee->symbol_name),
        .args = {*lowered_arg},
        .arg_types = {value_type},
        .arg_abi = {*compute_call_arg_abi(context_.target_profile, value_type)},
        .return_type = value_type,
        .result_abi = compute_function_return_abi(context_.target_profile, value_type, false),
    });
    return true;
  };

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {
    if (const auto lowered_fabs = lower_fabs_intrinsic_call(*call); lowered_fabs.has_value()) {
      return *lowered_fabs;
    }
  }

  if (const auto* va_start = std::get_if<c4c::codegen::lir::LirVaStartOp>(&inst)) {
    return lower_va_list_call("llvm.va_start.p0", va_start->ap_ptr);
  }

  if (const auto* va_end = std::get_if<c4c::codegen::lir::LirVaEndOp>(&inst)) {
    return lower_va_list_call("llvm.va_end.p0", va_end->ap_ptr);
  }

  if (const auto* va_copy = std::get_if<c4c::codegen::lir::LirVaCopyOp>(&inst)) {
    const auto lowered_dst = lower_value(va_copy->dst_ptr, bir::TypeKind::Ptr);
    const auto lowered_src = lower_value(va_copy->src_ptr, bir::TypeKind::Ptr);
    if (!lowered_dst.has_value() || !lowered_src.has_value()) {
      return fail_runtime_family("variadic runtime family");
    }
    lowered_insts->push_back(bir::CallInst{
        .callee = "llvm.va_copy.p0.p0",
        .args = {*lowered_dst, *lowered_src},
        .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
        .arg_abi = {*compute_call_arg_abi(context_.target_profile, bir::TypeKind::Ptr),
                    *compute_call_arg_abi(context_.target_profile, bir::TypeKind::Ptr)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    return true;
  }

  if (const auto* va_arg = std::get_if<c4c::codegen::lir::LirVaArgOp>(&inst)) {
    return lower_va_arg_call(*va_arg);
  }

  if (const auto* inline_asm = std::get_if<c4c::codegen::lir::LirInlineAsmOp>(&inst)) {
    return lower_inline_asm_call(*inline_asm);
  }

  if (const auto* stacksave = std::get_if<c4c::codegen::lir::LirStackSaveOp>(&inst)) {
    if (stacksave->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_runtime_family("stack-state runtime family");
    }
    lowered_insts->push_back(bir::CallInst{
        .result = bir::Value::named(bir::TypeKind::Ptr, stacksave->result.str()),
        .callee = "llvm.stacksave",
        .return_type = bir::TypeKind::Ptr,
        .result_abi = compute_function_return_abi(context_.target_profile, bir::TypeKind::Ptr, false),
    });
    return true;
  }

  if (const auto* stackrestore = std::get_if<c4c::codegen::lir::LirStackRestoreOp>(&inst)) {
    const auto lowered_saved_ptr = lower_value(stackrestore->saved_ptr, bir::TypeKind::Ptr);
    if (!lowered_saved_ptr.has_value()) {
      return fail_runtime_family("stack-state runtime family");
    }
    lowered_insts->push_back(bir::CallInst{
        .callee = "llvm.stackrestore",
        .args = {*lowered_saved_ptr},
        .arg_types = {bir::TypeKind::Ptr},
        .arg_abi = {*compute_call_arg_abi(context_.target_profile, bir::TypeKind::Ptr)},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    return true;
  }

  if (const auto* abs = std::get_if<c4c::codegen::lir::LirAbsOp>(&inst)) {
    if (abs->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    const auto value_type = lower_integer_type(abs->int_type.str());
    if (!value_type.has_value() ||
        (*value_type != bir::TypeKind::I32 && *value_type != bir::TypeKind::I64)) {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    const auto operand = lower_value(abs->arg, *value_type);
    const auto zero = make_integer_immediate(*value_type, 0);
    if (!operand.has_value() || !zero.has_value()) {
      return fail_runtime_family("absolute-value intrinsic family");
    }

    const std::string result_name = abs->result.str();
    const auto negated_value = bir::Value::named(*value_type, result_name + ".neg");
    lowered_insts->push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Sub,
        .result = negated_value,
        .operand_type = *value_type,
        .lhs = *zero,
        .rhs = *operand,
    });
    lowered_insts->push_back(bir::SelectInst{
        .predicate = bir::BinaryOpcode::Slt,
        .result = bir::Value::named(*value_type, result_name),
        .compare_type = *value_type,
        .lhs = *operand,
        .rhs = *zero,
        .true_value = negated_value,
        .false_value = *operand,
    });
    return true;
  }

  return false;
}

void BirFunctionLowerer::note_semantic_call_family_failure(std::string_view family) {
  const std::string function_name = function_name_for_reporting(context_.lir_module, function_);
  context_.note("function",
                "semantic lir_to_bir function '" + function_name +
                    "' failed in semantic call family '" + std::string(family) + "'");
}

void BirFunctionLowerer::note_runtime_intrinsic_family_failure(std::string_view family) {
  const std::string function_name = function_name_for_reporting(context_.lir_module, function_);
  context_.note("function",
                "semantic lir_to_bir function '" + function_name +
                    "' failed in runtime/intrinsic family '" + std::string(family) + "'");
}

std::optional<bir::Function> BirFunctionLowerer::lower_decl_function(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::TargetProfile& target_profile,
    const TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts) {
  bir::Function lowered;
  lowered.name = function.name;
  lowered.link_name_id = function.link_name_id;
  auto return_info = lower_signature_return_info(
      function.signature_text, type_decls, target_profile, &structured_layouts);
  if (!return_info.has_value()) {
    lowered.return_type = lower_param_type(function.return_type).value_or(bir::TypeKind::Void);
    lowered.return_abi = compute_function_return_abi(target_profile, lowered.return_type, false);
  } else {
    lowered.return_type = return_info->type;
    lowered.return_size_bytes = return_info->size_bytes;
    lowered.return_align_bytes = return_info->align_bytes;
    lowered.return_abi = return_info->abi;
  }
  if (!lower_function_params_with_layouts(function,
                                          target_profile,
                                          return_info,
                                          type_decls,
                                          &structured_layouts,
                                          &lowered)) {
    return std::nullopt;
  }
  lowered.is_declaration = true;
  return lowered;
}

}  // namespace c4c::backend
