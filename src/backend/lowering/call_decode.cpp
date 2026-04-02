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

}  // namespace

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

std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const BackendCallInst& call) {
  bool can_borrow_existing_types = true;
  for (std::size_t index = 0; index < call.param_types.size(); ++index) {
    if (call.param_types[index].empty()) {
      can_borrow_existing_types = false;
      break;
    }
  }
  if (can_borrow_existing_types) {
    return c4c::codegen::lir::borrow_lir_typed_call(call.param_types, call.args);
  }

  std::vector<std::string> param_types;
  param_types.reserve(call.args.size());
  for (std::size_t index = 0; index < call.args.size(); ++index) {
    param_types.push_back(render_backend_call_param_type(call, index));
  }
  return c4c::codegen::lir::borrow_lir_typed_call(param_types, call.args);
}

std::optional<ParsedBackendDirectGlobalTypedCallView> parse_backend_direct_global_typed_call(
    const BackendCallInst& call) {
  if (call.callee.kind != BackendCallCalleeKind::DirectGlobal) {
    return std::nullopt;
  }
  const auto typed_call = parse_backend_typed_call(call);
  if (!typed_call.has_value()) {
    return std::nullopt;
  }
  return ParsedBackendDirectGlobalTypedCallView{call.callee.symbol_name, std::move(*typed_call)};
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

}  // namespace c4c::backend
