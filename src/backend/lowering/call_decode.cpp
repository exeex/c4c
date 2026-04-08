#include "call_decode.hpp"

#include <charconv>
#include <limits>
#include <vector>

namespace c4c::backend {

namespace {

std::optional<std::int64_t> parse_backend_i64(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
}

bool backend_text_uses_nonminimal_types(std::string_view text) {
  return text.find("float") != std::string_view::npos ||
         text.find("double") != std::string_view::npos ||
         text.find("fp128") != std::string_view::npos ||
         text.find("i64") != std::string_view::npos ||
         text.find("i128") != std::string_view::npos;
}

bool parse_backend_signature_head(std::string_view signature_text,
                                  std::string_view* linkage,
                                  std::string_view* return_type,
                                  std::string_view* function_name) {
  const auto line = c4c::codegen::lir::trim_lir_arg_text(signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  const auto paren_open = line.find('(', at_pos);
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      paren_open == std::string_view::npos || first_space >= at_pos || at_pos + 1 >= paren_open) {
    return false;
  }

  *linkage = c4c::codegen::lir::trim_lir_arg_text(line.substr(0, first_space));
  *return_type = c4c::codegen::lir::trim_lir_arg_text(
      line.substr(first_space + 1, at_pos - first_space - 1));
  *function_name = line.substr(at_pos + 1, paren_open - at_pos - 1);
  return true;
}

}  // namespace

bool backend_lir_type_uses_nonminimal_types(std::string_view type_text) {
  return backend_text_uses_nonminimal_types(type_text);
}

bool backend_lir_global_uses_nonminimal_types(const c4c::codegen::lir::LirGlobal& global) {
  return backend_lir_type_uses_nonminimal_types(global.llvm_type) ||
         global.init_text.find("double") != std::string::npos ||
         global.init_text.find("float") != std::string::npos;
}

bool backend_lir_return_uses_nonminimal_types(const c4c::codegen::lir::LirRet& ret) {
  return backend_lir_type_uses_nonminimal_types(ret.type_str);
}

bool backend_lir_call_uses_nonminimal_types(const c4c::codegen::lir::LirCallOp& call) {
  if (backend_lir_type_uses_nonminimal_types(call.return_type)) {
    return true;
  }

  const auto parsed = parse_backend_typed_call(call);
  if (!parsed.has_value()) {
    return backend_lir_type_uses_nonminimal_types(call.callee_type_suffix) ||
           backend_lir_type_uses_nonminimal_types(call.args_str);
  }

  for (std::string_view type : parsed->param_types) {
    if (backend_lir_type_uses_nonminimal_types(type)) {
      return true;
    }
  }
  for (const auto& arg : parsed->args) {
    if (backend_lir_type_uses_nonminimal_types(arg.type)) {
      return true;
    }
  }
  return false;
}

bool backend_lir_function_signature_uses_nonminimal_types(std::string_view signature_text) {
  const auto line = c4c::codegen::lir::trim_lir_arg_text(signature_text);
  const auto first_space = line.find(' ');
  const auto at_pos = line.find('@');
  if (first_space == std::string_view::npos || at_pos == std::string_view::npos ||
      first_space >= at_pos) {
    return backend_lir_type_uses_nonminimal_types(signature_text);
  }

  if (backend_lir_type_uses_nonminimal_types(
          line.substr(first_space + 1, at_pos - first_space - 1))) {
    return true;
  }

  const auto params = parse_backend_function_signature_params(line);
  if (!params.has_value()) {
    return backend_lir_type_uses_nonminimal_types(signature_text);
  }
  for (const auto& param : *params) {
    if (!param.is_varargs && backend_lir_type_uses_nonminimal_types(param.type)) {
      return true;
    }
  }
  return false;
}

bool backend_lir_signature_matches(std::string_view signature_text,
                                   std::string_view expected_linkage,
                                   std::string_view expected_return_type,
                                   std::string_view expected_function_name,
                                   std::initializer_list<std::string_view> expected_param_types) {
  std::string_view linkage;
  std::string_view return_type;
  std::string_view function_name;
  if (!parse_backend_signature_head(signature_text, &linkage, &return_type, &function_name) ||
      linkage != expected_linkage || return_type != expected_return_type ||
      function_name != expected_function_name) {
    return false;
  }

  const auto params = parse_backend_function_signature_params(signature_text);
  if (!params.has_value() || params->size() != expected_param_types.size()) {
    return false;
  }

  std::size_t index = 0;
  for (std::string_view expected_type : expected_param_types) {
    if ((*params)[index].is_varargs ||
        c4c::codegen::lir::trim_lir_arg_text((*params)[index].type) !=
            c4c::codegen::lir::trim_lir_arg_text(expected_type)) {
      return false;
    }
    ++index;
  }
  return true;
}

bool backend_lir_is_i32_definition(std::string_view signature_text) {
  std::string_view linkage;
  std::string_view return_type;
  std::string_view function_name;
  if (!parse_backend_signature_head(signature_text, &linkage, &return_type, &function_name)) {
    return false;
  }

  return linkage == "define" && return_type == "i32" && !function_name.empty();
}

bool backend_lir_is_zero_arg_i32_definition(std::string_view signature_text) {
  if (!backend_lir_is_i32_definition(signature_text)) {
    return false;
  }

  const auto params = parse_backend_function_signature_params(signature_text);
  return params.has_value() && params->empty();
}

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call) {
  using namespace c4c::codegen::lir;

  const auto symbol_name = parse_lir_direct_global_callee(call.callee);
  if (!symbol_name.has_value()) {
    return std::nullopt;
  }

  if (const auto parsed = parse_lir_direct_global_typed_call(call);
      parsed.has_value()) {
    return parsed;
  }

  const auto callee_type_suffix = trim_lir_arg_text(call.callee_type_suffix);
  if (callee_type_suffix.empty()) {
    return std::nullopt;
  }

  const auto param_types = parse_lir_call_param_types(callee_type_suffix);
  if (!param_types.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string_view> fixed_param_types;
  bool saw_varargs = false;
  for (auto type : *param_types) {
    const auto trimmed_type = trim_lir_arg_text(type);
    if (trimmed_type == "...") {
      saw_varargs = true;
      break;
    }
    fixed_param_types.push_back(trimmed_type);
  }

  if (!saw_varargs) {
    return std::nullopt;
  }

  const auto args = parse_lir_typed_call_args(call.args_str);
  if (!args.has_value() || args->size() < fixed_param_types.size()) {
    return std::nullopt;
  }

  ParsedLirTypedCallView parsed;
  parsed.param_types.reserve(fixed_param_types.size());
  parsed.args.reserve(fixed_param_types.size());
  for (std::size_t index = 0; index < fixed_param_types.size(); ++index) {
    if (fixed_param_types[index] != (*args)[index].type) {
      return std::nullopt;
    }
    parsed.param_types.push_back(fixed_param_types[index]);
    parsed.args.push_back((*args)[index]);
  }
  return ParsedLirDirectGlobalTypedCallView{
      *symbol_name,
      std::move(parsed),
  };
}

std::optional<ParsedBackendExternCallArg> parse_backend_extern_call_arg(
    std::string_view type,
    std::string_view operand) {
  using namespace c4c::codegen::lir;

  const auto normalized_type = trim_lir_arg_text(type);
  const auto normalized_operand = trim_lir_arg_text(operand);
  if (normalized_type.empty() || normalized_operand.empty()) {
    return std::nullopt;
  }

  if (normalized_type == "i32") {
    const auto imm = parse_backend_i64(normalized_operand);
    if (!imm.has_value() ||
        *imm < std::numeric_limits<std::int32_t>::min() ||
        *imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }
    return ParsedBackendExternCallArg{ParsedBackendExternCallArg::Kind::I32Imm, *imm, {}};
  }

  if (normalized_type == "i64") {
    const auto imm = parse_backend_i64(normalized_operand);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return ParsedBackendExternCallArg{ParsedBackendExternCallArg::Kind::I64Imm, *imm, {}};
  }

  const bool is_ptr_type =
      normalized_type == "ptr" || (normalized_type.size() > 1 && normalized_type.back() == '*');
  if (!is_ptr_type) {
    return std::nullopt;
  }

  const auto imm = parse_backend_i64(normalized_operand);
  if (imm.has_value()) {
    return ParsedBackendExternCallArg{ParsedBackendExternCallArg::Kind::I64Imm, *imm, {}};
  }

  if (normalized_operand.front() != '@') {
    return std::nullopt;
  }

  return ParsedBackendExternCallArg{
      ParsedBackendExternCallArg::Kind::Ptr,
      0,
      std::string(normalized_operand),
  };
}

std::optional<std::vector<ParsedBackendExternCallArg>> parse_backend_extern_call_args(
    const ParsedBackendTypedCallView& parsed) {
  std::vector<ParsedBackendExternCallArg> args;
  args.reserve(parsed.args.size());
  for (std::size_t index = 0; index < parsed.args.size(); ++index) {
    auto parsed_arg = parse_backend_extern_call_arg(parsed.param_types[index],
                                                    parsed.args[index].operand);
    if (!parsed_arg.has_value()) {
      return std::nullopt;
    }
    args.push_back(*parsed_arg);
  }
  return args;
}

std::optional<std::vector<c4c::codegen::lir::OwnedLirTypedCallArg>>
parse_backend_owned_typed_call_args(
    std::string_view args_str) {
  const auto parsed = c4c::codegen::lir::parse_lir_typed_call_args(args_str);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  std::vector<c4c::codegen::lir::OwnedLirTypedCallArg> owned;
  owned.reserve(parsed->size());
  for (const auto& arg : *parsed) {
    owned.push_back({std::string(arg.type), std::string(arg.operand)});
  }
  return owned;
}

std::optional<std::vector<c4c::codegen::lir::OwnedLirTypedCallArg>>
parse_backend_owned_typed_call_args(
    const c4c::codegen::lir::LirCallOp& call) {
  const auto parsed = c4c::codegen::lir::parse_lir_typed_call_args(call.args_str);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  std::vector<c4c::codegen::lir::OwnedLirTypedCallArg> owned;
  owned.reserve(parsed->size());
  for (const auto& arg : *parsed) {
    owned.push_back({std::string(arg.type), std::string(arg.operand)});
  }
  return owned;
}

std::optional<std::vector<ParsedBackendFunctionSignatureParam>>
parse_backend_function_signature_params(std::string_view signature_text) {
  const auto paren_open = signature_text.find('(');
  const auto paren_close = signature_text.rfind(')');
  if (paren_open == std::string_view::npos || paren_close == std::string_view::npos ||
      paren_close < paren_open) {
    return std::nullopt;
  }

  const auto params_text = signature_text.substr(paren_open + 1, paren_close - paren_open - 1);
  std::vector<ParsedBackendFunctionSignatureParam> params;
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

void collect_backend_call_value_names(const c4c::codegen::lir::LirCallOp& call,
                                      std::vector<std::string>& values) {
  c4c::codegen::lir::collect_lir_value_names_from_call(call, values);
}

}  // namespace c4c::backend
