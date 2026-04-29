#include "lowering.hpp"

#include <algorithm>

namespace c4c::backend {

namespace {

bool use_float_return_registers(const c4c::TargetProfile& target_profile,
                                bir::TypeKind type) {
  if (!target_profile.has_float_return_registers) {
    return false;
  }
  return type == bir::TypeKind::F32 || type == bir::TypeKind::F64 ||
         type == bir::TypeKind::F128;
}

bool use_float_arg_registers(const c4c::TargetProfile& target_profile,
                             bir::TypeKind type) {
  if (!target_profile.has_float_arg_registers) {
    return false;
  }
  return type == bir::TypeKind::F32 || type == bir::TypeKind::F64 ||
         type == bir::TypeKind::F128;
}

std::optional<bir::CallResultAbiInfo> lower_function_return_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type,
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
      abi.primary_class = use_float_return_registers(target_profile, type)
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
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

std::optional<bir::CallArgAbiInfo> lower_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
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
      abi.primary_class = use_float_arg_registers(target_profile, type)
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
      return abi;
    case bir::TypeKind::F128:
      if (target_profile.arch == c4c::TargetArch::X86_64) {
        abi.primary_class = bir::AbiValueClass::Memory;
        abi.passed_in_register = false;
        abi.passed_on_stack = true;
        return abi;
      }
      abi.primary_class = use_float_arg_registers(target_profile, type)
                              ? bir::AbiValueClass::Sse
                              : bir::AbiValueClass::Integer;
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

bool BirFunctionLowerer::is_void_param_sentinel(const c4c::TypeSpec& type) {
  return type.base == TB_VOID && type.ptr_level == 0 && type.array_rank == 0;
}

std::optional<bir::TypeKind> BirFunctionLowerer::lower_param_type(const c4c::TypeSpec& type) {
  if (type.ptr_level != 0 || type.array_rank != 0) {
    return bir::TypeKind::Ptr;
  }
  if (type.base == TB_BOOL && type.ptr_level == 0 && type.array_rank == 0) {
    return bir::TypeKind::I1;
  }
  return lower_minimal_scalar_type(type);
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

bool BirFunctionLowerer::has_structured_signature_params(
    const c4c::codegen::lir::LirFunction& function) {
  return function.signature_has_void_param_list || function.signature_is_variadic ||
         !function.signature_params.empty() || !function.signature_param_type_refs.empty();
}

std::optional<std::vector<BirFunctionLowerer::ParsedFunctionSignatureParam>>
BirFunctionLowerer::structured_signature_params(
    const c4c::codegen::lir::LirFunction& function) {
  if (function.signature_params.size() != function.signature_param_type_refs.size()) {
    return std::nullopt;
  }

  std::vector<ParsedFunctionSignatureParam> params;
  params.reserve(function.signature_params.size() + (function.signature_is_variadic ? 1u : 0u));
  for (std::size_t index = 0; index < function.signature_params.size(); ++index) {
    params.push_back({
        function.signature_param_type_refs[index].str(),
        function.signature_params[index].name,
        false,
    });
  }
  if (function.signature_is_variadic) {
    params.push_back({"", "...", true});
  }
  return params;
}

std::optional<BirFunctionLowerer::LoweredReturnInfo> BirFunctionLowerer::lower_return_info_from_type(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const c4c::TargetProfile& target_profile,
    const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type_text);
  if (trimmed == "void") {
    return LoweredReturnInfo{};
  }
  if (const auto scalar_type = lower_scalar_or_function_pointer_type(trimmed); scalar_type.has_value()) {
    return LoweredReturnInfo{
        .type = *scalar_type,
        .abi = lir_to_bir_detail::compute_function_return_abi(target_profile, *scalar_type, false),
    };
  }
  if (const auto aggregate_layout =
          lower_byval_aggregate_layout(trimmed, type_decls, structured_layouts);
      aggregate_layout.has_value()) {
    return LoweredReturnInfo{
        .type = bir::TypeKind::Void,
        .size_bytes = aggregate_layout->size_bytes,
        .align_bytes = aggregate_layout->align_bytes,
        .abi = lir_to_bir_detail::compute_function_return_abi(
            target_profile, bir::TypeKind::Void, true),
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
    const auto lowered_type =
        lower_return_info_from_type(ret->type_str,
                                    type_decls_,
                                    context_.target_profile,
                                    &structured_layouts_);
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
    const c4c::codegen::lir::LirFunction& function,
    const TypeDeclMap& type_decls,
    const c4c::TargetProfile& target_profile,
    const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts) {
  if (function.signature_return_type_ref.has_value()) {
    return lower_return_info_from_type(function.signature_return_type_ref->str(),
                                       type_decls,
                                       target_profile,
                                       structured_layouts);
  }

  // Compatibility fallback for hand-built legacy LIR that predates structured
  // signature metadata. Generated LIR should provide signature_return_type_ref.
  const auto line = c4c::codegen::lir::trim_lir_arg_text(function.signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      first_space >= at_pos) {
    return std::nullopt;
  }
  const auto return_type_text =
      c4c::codegen::lir::trim_lir_arg_text(line.substr(first_space + 1, at_pos - first_space - 1));
  return lower_return_info_from_type(return_type_text, type_decls, target_profile, structured_layouts);
}

bool BirFunctionLowerer::lower_function_params_with_layouts(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::TargetProfile& target_profile,
    const std::optional<LoweredReturnInfo>& return_info,
    const TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
    bir::Function* lowered) {
  const auto initial_param_count = lowered->params.size();
  if (return_info.has_value() && return_info->returned_via_sret) {
    lowered->params.push_back(bir::Param{
        .type = bir::TypeKind::Ptr,
        .name = "%ret.sret",
        .size_bytes = return_info->size_bytes,
        .align_bytes = return_info->align_bytes,
        .abi =
            [&]() -> std::optional<bir::CallArgAbiInfo> {
          auto abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, bir::TypeKind::Ptr);
          if (abi.has_value()) {
            abi->sret_pointer = true;
          }
          return abi;
      }(),
        .is_sret = true,
    });
  }

  const bool structured_params_available = has_structured_signature_params(function);
  // Generated LIR carries structured signature params. The text parser remains
  // only for legacy hand-built LIR fixtures that do not populate those fields.
  const auto parsed_params = structured_params_available
                                 ? structured_signature_params(function)
                                 : parse_function_signature_params(function.signature_text);
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
      function.signature_has_void_param_list ||
      (!structured_params_available && function.params.size() == 1 &&
       is_void_param_sentinel(function.params.front().second));
  if (has_void_param_sentinel &&
      (!parsed_params.has_value() ||
       (parsed_params->empty() ||
        (parsed_params->size() == 1 && !parsed_params->front().is_varargs &&
         c4c::codegen::lir::trim_lir_arg_text(parsed_params->front().type) == "void")))) {
    return true;
  }

  if (!structured_params_available && parsed_params.has_value() && !function.params.empty() &&
      !has_void_param_sentinel &&
      parsed_fixed_param_count != function.params.size()) {
    return false;
  }

  if (structured_params_available) {
    if (!parsed_params.has_value()) {
      return false;
    }
    for (const auto& param : *parsed_params) {
      if (param.is_varargs) {
        lowered->is_variadic = true;
        return true;
      }
      const auto layout = lower_byval_aggregate_layout(param.type, type_decls, structured_layouts);
      if (layout.has_value()) {
        if (param.operand.empty()) {
          return false;
        }
        lowered->params.push_back(bir::Param{
            .type = bir::TypeKind::Ptr,
            .name = param.operand,
            .size_bytes = layout->size_bytes,
            .align_bytes = layout->align_bytes,
            .abi =
                [&]() -> std::optional<bir::CallArgAbiInfo> {
              auto abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, bir::TypeKind::Ptr);
              if (abi.has_value()) {
                abi->size_bytes = layout->size_bytes;
                abi->align_bytes = layout->align_bytes;
                abi->byval_copy = true;
              }
              return abi;
            }(),
            .is_byval = true,
        });
        continue;
      }

      const auto lowered_type = lower_scalar_or_function_pointer_type(param.type);
      if (!lowered_type.has_value() || param.operand.empty()) {
        return false;
      }
      lowered->params.push_back(bir::Param{
          .type = *lowered_type,
          .name = param.operand,
          .abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, *lowered_type),
      });
    }
    lowered->is_variadic = parsed_is_variadic;
    return true;
  }

  for (std::size_t index = 0; index < function.params.size() && !has_void_param_sentinel; ++index) {
    const auto& param = function.params[index];
    const auto lowered_type = lower_param_type(param.second);
    if (!lowered_type.has_value()) {
      if (!parsed_params.has_value() || index >= parsed_params->size()) {
        return false;
      }
      const auto layout =
          lower_byval_aggregate_layout((*parsed_params)[index].type, type_decls, structured_layouts);
      if (!layout.has_value() || param.first.empty()) {
        return false;
      }
      lowered->params.push_back(bir::Param{
          .type = bir::TypeKind::Ptr,
          .name = param.first,
          .size_bytes = layout->size_bytes,
          .align_bytes = layout->align_bytes,
          .abi =
              [&]() -> std::optional<bir::CallArgAbiInfo> {
            auto abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, bir::TypeKind::Ptr);
            if (abi.has_value()) {
              abi->size_bytes = layout->size_bytes;
              abi->align_bytes = layout->align_bytes;
              abi->byval_copy = true;
            }
            return abi;
          }(),
          .is_byval = true,
      });
      continue;
    }
    lowered->params.push_back(bir::Param{
        .type = *lowered_type,
        .name = param.first,
        .abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, *lowered_type),
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
          .abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, *lowered_type),
      });
      continue;
    }
    const auto layout = lower_byval_aggregate_layout(param.type, type_decls, structured_layouts);
    if (!layout.has_value() || param.operand.empty()) {
      return false;
    }
    lowered->params.push_back(bir::Param{
        .type = bir::TypeKind::Ptr,
        .name = param.operand,
        .size_bytes = layout->size_bytes,
        .align_bytes = layout->align_bytes,
        .abi =
            [&]() -> std::optional<bir::CallArgAbiInfo> {
          auto abi = lir_to_bir_detail::compute_call_arg_abi(target_profile, bir::TypeKind::Ptr);
          if (abi.has_value()) {
            abi->size_bytes = layout->size_bytes;
            abi->align_bytes = layout->align_bytes;
            abi->byval_copy = true;
          }
          return abi;
        }(),
        .is_byval = true,
    });
  }
  lowered->is_variadic = parsed_is_variadic;
  return true;
}

bool BirFunctionLowerer::lower_function_params(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::TargetProfile& target_profile,
    const std::optional<LoweredReturnInfo>& return_info,
    const TypeDeclMap& type_decls,
    bir::Function* lowered) const {
  return lower_function_params_with_layouts(function,
                                            target_profile,
                                            return_info,
                                            type_decls,
                                            &structured_layouts_,
                                            lowered);
}

bool BirFunctionLowerer::lower_function_params_fallback(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::TargetProfile& target_profile,
    const std::optional<LoweredReturnInfo>& return_info,
    const TypeDeclMap& type_decls,
    bir::Function* lowered) {
  return lower_function_params_with_layouts(function,
                                            target_profile,
                                            return_info,
                                            type_decls,
                                            nullptr,
                                            lowered);
}

std::optional<bir::CallArgAbiInfo> lir_to_bir_detail::compute_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type) {
  return lower_call_arg_abi(target_profile, type);
}

std::optional<bir::CallResultAbiInfo> lir_to_bir_detail::compute_function_return_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type,
    bool returned_via_sret) {
  return lower_function_return_abi(target_profile, type, returned_via_sret);
}

}  // namespace c4c::backend
