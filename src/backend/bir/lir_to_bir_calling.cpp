#include "lir_to_bir.hpp"

#include <algorithm>

namespace c4c::backend {

namespace {

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

std::optional<bir::CallResultAbiInfo> lower_function_return_abi(bir::TypeKind type,
                                                                bool returned_via_sret) {
  if (returned_via_sret) {
    return bir::CallResultAbiInfo{
        .type = bir::TypeKind::Void,
        .primary_class = bir::AbiValueClass::Memory,
        .returned_in_memory = true,
    };
  }

  bir::CallResultAbiInfo abi{
      .type = type,
  };
  switch (type) {
    case bir::TypeKind::Void:
      return std::nullopt;
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      abi.primary_class = bir::AbiValueClass::Sse;
      return abi;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.returned_in_memory = true;
      return abi;
  }
  return std::nullopt;
}

std::optional<bir::CallArgAbiInfo> lower_call_arg_abi(bir::TypeKind type) {
  if (type == bir::TypeKind::Void) {
    return std::nullopt;
  }

  bir::CallArgAbiInfo abi{
      .type = type,
      .size_bytes = lir_to_bir_detail::type_size_bytes(type),
      .align_bytes = lir_to_bir_detail::type_size_bytes(type),
      .passed_in_register = true,
  };
  switch (type) {
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      abi.primary_class = bir::AbiValueClass::Sse;
      return abi;
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      abi.primary_class = bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::I128:
      abi.primary_class = bir::AbiValueClass::Memory;
      abi.passed_in_register = false;
      abi.passed_on_stack = true;
      return abi;
    case bir::TypeKind::Void:
      return std::nullopt;
  }
  return std::nullopt;
}

}  // namespace

using lir_to_bir_detail::lower_integer_type;

bool BirFunctionLowerer::is_void_param_sentinel(const c4c::TypeSpec& type) {
  return type.base == TB_VOID && type.ptr_level == 0 && type.array_rank == 0;
}

std::optional<bir::TypeKind> BirFunctionLowerer::lower_param_type(const c4c::TypeSpec& type) {
  if (type.base == TB_BOOL && type.ptr_level == 0 && type.array_rank == 0) {
    return bir::TypeKind::I1;
  }
  return lower_minimal_scalar_type(type);
}

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

  if (!saw_varargs) {
    return std::nullopt;
  }

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
    parsed.typed_call.owned_param_types.push_back(
        std::string(c4c::codegen::lir::trim_lir_arg_text((*args)[index].type)));
    parsed.typed_call.param_types.push_back(parsed.typed_call.owned_param_types.back());
    parsed.typed_call.args.push_back((*args)[index]);
  }
  return parsed;
}

std::optional<std::vector<BirFunctionLowerer::ParsedFunctionSignatureParam>>
BirFunctionLowerer::parse_function_signature_params(std::string_view signature_text) {
  const auto paren_open = signature_text.find('(');
  const auto paren_close = signature_text.rfind(')');
  if (paren_open == std::string_view::npos || paren_close == std::string_view::npos ||
      paren_close < paren_open) {
    return std::nullopt;
  }

  const auto params_text = signature_text.substr(paren_open + 1, paren_close - paren_open - 1);
  std::vector<ParsedFunctionSignatureParam> params;
  bool parse_failed = false;
  c4c::codegen::lir::for_each_lir_top_level_segment(
      params_text, ',', [&](std::string_view raw_param) {
        const auto param = c4c::codegen::lir::trim_lir_arg_text(raw_param);
        if (param.empty()) {
          return;
        }
        if (param == "...") {
          params.push_back({"", "...", true});
          return;
        }

        const auto parsed = c4c::codegen::lir::parse_lir_typed_call_arg(param);
        if (!parsed.has_value()) {
          parse_failed = true;
          return;
        }
        params.push_back({std::string(parsed->type), std::string(parsed->operand), false});
      });
  if (parse_failed) {
    return std::nullopt;
  }
  return params;
}

std::optional<BirFunctionLowerer::LoweredReturnInfo> BirFunctionLowerer::lower_return_info_from_type(
    std::string_view type_text,
    const TypeDeclMap& type_decls) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type_text);
  if (trimmed == "void") {
    return LoweredReturnInfo{};
  }
  if (const auto scalar_type = lower_scalar_or_function_pointer_type(trimmed); scalar_type.has_value()) {
    return LoweredReturnInfo{
        .type = *scalar_type,
        .abi = lower_function_return_abi(*scalar_type, false),
    };
  }
  if (const auto aggregate_layout = lower_byval_aggregate_layout(trimmed, type_decls);
      aggregate_layout.has_value()) {
    return LoweredReturnInfo{
        .type = bir::TypeKind::Void,
        .size_bytes = aggregate_layout->size_bytes,
        .align_bytes = aggregate_layout->align_bytes,
        .abi = lower_function_return_abi(bir::TypeKind::Void, true),
        .returned_via_sret = true,
    };
  }
  return std::nullopt;
}

std::optional<BirFunctionLowerer::LoweredReturnInfo> BirFunctionLowerer::infer_function_return_info()
    const {
  std::optional<LoweredReturnInfo> return_info;
  for (const auto& block : function_.blocks) {
    const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator);
    if (ret == nullptr) {
      continue;
    }
    const auto lowered_type = lower_return_info_from_type(ret->type_str, type_decls_);
    if (!lowered_type.has_value()) {
      return std::nullopt;
    }
    if (!return_info.has_value()) {
      return_info = lowered_type;
      continue;
    }
    if (return_info->type != lowered_type->type ||
        return_info->size_bytes != lowered_type->size_bytes ||
        return_info->align_bytes != lowered_type->align_bytes ||
        return_info->returned_via_sret != lowered_type->returned_via_sret) {
      return std::nullopt;
    }
  }
  return return_info;
}

std::optional<BirFunctionLowerer::LoweredReturnInfo> BirFunctionLowerer::lower_signature_return_info(
    std::string_view signature_text,
    const TypeDeclMap& type_decls) {
  const auto line = c4c::codegen::lir::trim_lir_arg_text(signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      first_space >= at_pos) {
    return std::nullopt;
  }
  const auto return_type_text =
      c4c::codegen::lir::trim_lir_arg_text(line.substr(first_space + 1, at_pos - first_space - 1));
  return lower_return_info_from_type(return_type_text, type_decls);
}

std::optional<bir::Function> BirFunctionLowerer::lower_extern_decl(
    const c4c::codegen::lir::LirExternDecl& decl) {
  auto return_info = lower_return_info_from_type(decl.return_type_str, TypeDeclMap{});
  if (!return_info.has_value()) {
    return_info = lower_return_info_from_type(decl.return_type.str(), TypeDeclMap{});
  }
  if (!return_info.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = decl.name;
  lowered.return_type = return_info->type;
  lowered.return_size_bytes = return_info->size_bytes;
  lowered.return_align_bytes = return_info->align_bytes;
  lowered.return_abi = return_info->abi;
  lowered.is_declaration = true;
  return lowered;
}

bool BirFunctionLowerer::lower_function_params(
    const c4c::codegen::lir::LirFunction& function,
    const std::optional<LoweredReturnInfo>& return_info,
    const TypeDeclMap& type_decls,
    bir::Function* lowered) {
  const auto initial_param_count = lowered->params.size();
  if (return_info.has_value() && return_info->returned_via_sret) {
    lowered->params.push_back(bir::Param{
        .type = bir::TypeKind::Ptr,
        .name = "%ret.sret",
        .size_bytes = return_info->size_bytes,
        .align_bytes = return_info->align_bytes,
        .is_sret = true,
    });
  }

  const auto parsed_params = parse_function_signature_params(function.signature_text);
  const auto parsed_fixed_param_count = [&]() -> std::size_t {
    if (!parsed_params.has_value()) {
      return 0;
    }
    std::size_t count = 0;
    for (const auto& param : *parsed_params) {
      if (param.is_varargs) {
        break;
      }
      ++count;
    }
    return count;
  }();
  const bool parsed_is_variadic =
      parsed_params.has_value() &&
      std::any_of(parsed_params->begin(),
                  parsed_params->end(),
                  [](const ParsedFunctionSignatureParam& param) { return param.is_varargs; });
  const bool has_void_param_sentinel =
      function.params.size() == 1 && is_void_param_sentinel(function.params.front().second);
  if (has_void_param_sentinel &&
      (!parsed_params.has_value() ||
       (parsed_params->size() == 1 && !parsed_params->front().is_varargs &&
        c4c::codegen::lir::trim_lir_arg_text(parsed_params->front().type) == "void"))) {
    return true;
  }

  if (parsed_params.has_value() && !function.params.empty() && !has_void_param_sentinel &&
      parsed_fixed_param_count != function.params.size()) {
    return false;
  }

  for (std::size_t index = 0; index < function.params.size() && !has_void_param_sentinel;
       ++index) {
    const auto& param = function.params[index];
    const auto lowered_type = lower_param_type(param.second);
    if (!lowered_type.has_value()) {
      if (!parsed_params.has_value() || index >= parsed_params->size()) {
        return false;
      }
      const auto layout = lower_byval_aggregate_layout((*parsed_params)[index].type, type_decls);
      if (!layout.has_value() || param.first.empty()) {
        return false;
      }
      lowered->params.push_back(bir::Param{
          .type = bir::TypeKind::Ptr,
          .name = param.first,
          .size_bytes = layout->size_bytes,
          .align_bytes = layout->align_bytes,
          .is_byval = true,
      });
      continue;
    }
    lowered->params.push_back(bir::Param{
        .type = *lowered_type,
        .name = param.first,
    });
  }

  if (lowered->params.size() > initial_param_count + (return_info.has_value() &&
                                                      return_info->returned_via_sret
                                                  ? 1u
                                                  : 0u)) {
    lowered->is_variadic = parsed_is_variadic;
    return true;
  }

  if (!has_void_param_sentinel && !function.params.empty()) {
    return true;
  }

  if (!parsed_params.has_value()) {
    return true;
  }

  if (parsed_params->size() == 1 && !parsed_params->front().is_varargs &&
      c4c::codegen::lir::trim_lir_arg_text(parsed_params->front().type) == "void") {
    return true;
  }

  for (const auto& param : *parsed_params) {
    if (param.is_varargs) {
      lowered->is_variadic = true;
      return true;
    }
    const auto lowered_type = lower_scalar_or_function_pointer_type(param.type);
    if (lowered_type.has_value()) {
      if (param.operand.empty()) {
        return false;
      }
      lowered->params.push_back(bir::Param{
          .type = *lowered_type,
          .name = param.operand,
      });
      continue;
    }
    const auto layout = lower_byval_aggregate_layout(param.type, type_decls);
    if (!layout.has_value() || param.operand.empty()) {
      return false;
    }
    lowered->params.push_back(bir::Param{
        .type = bir::TypeKind::Ptr,
        .name = param.operand,
        .size_bytes = layout->size_bytes,
        .align_bytes = layout->align_bytes,
        .is_byval = true,
    });
  }
  lowered->is_variadic = parsed_is_variadic;
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
    lowered_insts->push_back(bir::CallInst{
        .callee = std::string(callee_name),
        .args = {*lowered_ap},
        .arg_types = {bir::TypeKind::Ptr},
        .arg_abi = {*lower_call_arg_abi(bir::TypeKind::Ptr)},
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
      const auto aggregate_layout = lower_byval_aggregate_layout(va_arg.type_str.str(), type_decls_);
      if (!aggregate_layout.has_value()) {
        return fail_runtime_family("variadic runtime family");
      }
      if (!declare_local_aggregate_slots(
              va_arg.type_str.str(), va_arg.result.str(), aggregate_layout->align_bytes)) {
        return fail_runtime_family("variadic runtime family");
      }
      aggregate_value_aliases_[va_arg.result.str()] = va_arg.result.str();
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
               bir::CallArgAbiInfo{
                   .type = bir::TypeKind::Ptr,
               }},
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
        .arg_abi = {*lower_call_arg_abi(bir::TypeKind::Ptr)},
        .return_type = *lowered_type,
        .result_abi = lower_function_return_abi(*lowered_type, false),
    });
    return true;
  };
  const auto lower_inline_asm_call =
      [&](const c4c::codegen::lir::LirInlineAsmOp& inline_asm) -> bool {
    const auto return_type_text =
        std::string(c4c::codegen::lir::trim_lir_arg_text(inline_asm.ret_type.str()));
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
      lowered_call.result_abi = lower_function_return_abi(*lowered_type, false);
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
        .arg_abi = {*lower_call_arg_abi(value_type)},
        .return_type = value_type,
        .result_abi = lower_function_return_abi(value_type, false),
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
        .arg_abi = {*lower_call_arg_abi(bir::TypeKind::Ptr),
                    *lower_call_arg_abi(bir::TypeKind::Ptr)},
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
        .result_abi = lower_function_return_abi(bir::TypeKind::Ptr, false),
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
        .arg_abi = {*lower_call_arg_abi(bir::TypeKind::Ptr)},
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
  context_.note("function",
                "semantic lir_to_bir function '" + function_.name +
                    "' failed in semantic call family '" + std::string(family) + "'");
}

void BirFunctionLowerer::note_runtime_intrinsic_family_failure(std::string_view family) {
  context_.note("function",
                "semantic lir_to_bir function '" + function_.name +
                    "' failed in runtime/intrinsic family '" + std::string(family) + "'");
}

std::optional<bir::Function> BirFunctionLowerer::lower_decl_function(
    const c4c::codegen::lir::LirFunction& function) {
  bir::Function lowered;
  lowered.name = function.name;
  auto return_info = lower_signature_return_info(function.signature_text, TypeDeclMap{});
  if (!return_info.has_value()) {
    lowered.return_type = lower_param_type(function.return_type).value_or(bir::TypeKind::Void);
    lowered.return_abi = lower_function_return_abi(lowered.return_type, false);
  } else {
    lowered.return_type = return_info->type;
    lowered.return_size_bytes = return_info->size_bytes;
    lowered.return_align_bytes = return_info->align_bytes;
    lowered.return_abi = return_info->abi;
  }
  if (!lower_function_params(function, return_info, TypeDeclMap{}, &lowered)) {
    return std::nullopt;
  }
  lowered.is_declaration = true;
  return lowered;
}

}  // namespace c4c::backend
