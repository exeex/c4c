#include "lir_to_bir.hpp"

#include "call_decode.hpp"

namespace c4c::backend::lir_to_bir_detail {

namespace {

bool is_void_param_sentinel(const c4c::TypeSpec& type) {
  return type.base == TB_VOID && type.ptr_level == 0 && type.array_rank == 0;
}

}  // namespace

std::optional<bir::TypeKind> lower_param_type(const c4c::TypeSpec& type) {
  if (type.base == TB_BOOL && type.ptr_level == 0 && type.array_rank == 0) {
    return bir::TypeKind::I1;
  }
  return backend_lir_lower_minimal_scalar_type(type);
}

std::optional<LoweredReturnInfo> lower_return_info_from_type(std::string_view type_text,
                                                             const TypeDeclMap& type_decls) {
  const auto trimmed = c4c::codegen::lir::trim_lir_arg_text(type_text);
  if (trimmed == "void") {
    return LoweredReturnInfo{};
  }
  if (const auto scalar_type = lower_integer_type(trimmed); scalar_type.has_value()) {
    return LoweredReturnInfo{
        .type = *scalar_type,
    };
  }
  if (const auto aggregate_layout = lower_byval_aggregate_layout(trimmed, type_decls);
      aggregate_layout.has_value()) {
    return LoweredReturnInfo{
        .type = bir::TypeKind::Void,
        .size_bytes = aggregate_layout->size_bytes,
        .align_bytes = aggregate_layout->align_bytes,
        .returned_via_sret = true,
    };
  }
  return std::nullopt;
}

std::optional<LoweredReturnInfo> infer_function_return_info(
    const c4c::codegen::lir::LirFunction& function,
    const TypeDeclMap& type_decls) {
  std::optional<LoweredReturnInfo> return_info;
  for (const auto& block : function.blocks) {
    const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator);
    if (ret == nullptr) {
      continue;
    }
    const auto lowered_type = lower_return_info_from_type(ret->type_str, type_decls);
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

std::optional<LoweredReturnInfo> lower_signature_return_info(std::string_view signature_text,
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

std::optional<bir::Function> lower_extern_decl(const c4c::codegen::lir::LirExternDecl& decl) {
  auto return_type = lower_integer_type(decl.return_type_str);
  if (!return_type.has_value()) {
    return_type = lower_integer_type(decl.return_type.str());
  }
  if (!return_type.has_value() &&
      (c4c::codegen::lir::trim_lir_arg_text(decl.return_type_str) == "void" ||
       c4c::codegen::lir::trim_lir_arg_text(decl.return_type.str()) == "void")) {
    return_type = bir::TypeKind::Void;
  }
  if (!return_type.has_value()) {
    return std::nullopt;
  }

  bir::Function lowered;
  lowered.name = decl.name;
  lowered.return_type = *return_type;
  lowered.is_declaration = true;
  return lowered;
}

bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
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

  const auto parsed_params = parse_backend_function_signature_params(function.signature_text);
  const bool has_void_param_sentinel =
      function.params.size() == 1 && is_void_param_sentinel(function.params.front().second);
  if (has_void_param_sentinel &&
      (!parsed_params.has_value() ||
       (parsed_params->size() == 1 && !parsed_params->front().is_varargs &&
        c4c::codegen::lir::trim_lir_arg_text(parsed_params->front().type) == "void"))) {
    return true;
  }

  if (parsed_params.has_value() && !function.params.empty() && !has_void_param_sentinel &&
      parsed_params->size() != function.params.size()) {
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
      return false;
    }
    const auto lowered_type = lower_integer_type(param.type);
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
  return true;
}

std::optional<bir::Function> lower_decl_function(const c4c::codegen::lir::LirFunction& function) {
  bir::Function lowered;
  lowered.name = function.name;
  auto return_info = lower_signature_return_info(function.signature_text, TypeDeclMap{});
  if (!return_info.has_value()) {
    lowered.return_type = lower_param_type(function.return_type).value_or(bir::TypeKind::Void);
  } else {
    lowered.return_type = return_info->type;
    lowered.return_size_bytes = return_info->size_bytes;
    lowered.return_align_bytes = return_info->align_bytes;
  }
  if (!lower_function_params(function, return_info, TypeDeclMap{}, &lowered)) {
    return std::nullopt;
  }
  lowered.is_declaration = true;
  return lowered;
}

}  // namespace c4c::backend::lir_to_bir_detail
