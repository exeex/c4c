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
  lowered->is_variadic = parsed_is_variadic;
  return true;
}

bool BirFunctionLowerer::lower_runtime_intrinsic_inst(
    const c4c::codegen::lir::LirInst& inst,
    std::vector<bir::Inst>* lowered_insts) const {
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
      return false;
    }
    lowered_insts->push_back(bir::CallInst{
        .callee = std::string(callee_name),
        .args = {*lowered_ap},
        .arg_types = {bir::TypeKind::Ptr},
        .return_type_name = "void",
        .return_type = bir::TypeKind::Void,
    });
    return true;
  };
  const auto lower_va_arg_call =
      [&](const c4c::codegen::lir::LirVaArgOp& va_arg) -> bool {
    if (va_arg.result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }
    const auto lowered_ap = lower_value(va_arg.ap_ptr, bir::TypeKind::Ptr);
    const auto lowered_type = lower_va_result_type(va_arg.type_str.str());
    if (!lowered_ap.has_value() || !lowered_type.has_value()) {
      return false;
    }
    lowered_insts->push_back(bir::CallInst{
        .result = bir::Value::named(*lowered_type, va_arg.result.str()),
        .callee =
            "llvm.va_arg." + std::string(c4c::codegen::lir::trim_lir_arg_text(va_arg.type_str.str())),
        .args = {*lowered_ap},
        .arg_types = {bir::TypeKind::Ptr},
        .return_type = *lowered_type,
    });
    return true;
  };

  if (const auto* va_start = std::get_if<c4c::codegen::lir::LirVaStartOp>(&inst)) {
    return lower_va_list_call("llvm.va_start.p0", va_start->ap_ptr);
  }

  if (const auto* va_end = std::get_if<c4c::codegen::lir::LirVaEndOp>(&inst)) {
    return lower_va_list_call("llvm.va_end.p0", va_end->ap_ptr);
  }

  if (const auto* va_arg = std::get_if<c4c::codegen::lir::LirVaArgOp>(&inst)) {
    return lower_va_arg_call(*va_arg);
  }

  if (const auto* abs = std::get_if<c4c::codegen::lir::LirAbsOp>(&inst)) {
    if (abs->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto value_type = lower_integer_type(abs->int_type.str());
    if (!value_type.has_value() ||
        (*value_type != bir::TypeKind::I32 && *value_type != bir::TypeKind::I64)) {
      return false;
    }

    const auto operand = lower_value(abs->arg, *value_type);
    const auto zero = make_integer_immediate(*value_type, 0);
    if (!operand.has_value() || !zero.has_value()) {
      return false;
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

std::optional<bir::Function> BirFunctionLowerer::lower_decl_function(
    const c4c::codegen::lir::LirFunction& function) {
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

}  // namespace c4c::backend
