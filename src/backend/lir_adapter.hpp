#pragma once

#include "../codegen/lir/call_args.hpp"
#include "../codegen/lir/ir.hpp"

#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend {

enum class LirAdapterErrorKind {
  Unsupported,
  Malformed,
};

class LirAdapterError : public std::invalid_argument {
 public:
  LirAdapterError(LirAdapterErrorKind kind, const std::string& message)
      : std::invalid_argument(message), kind_(kind) {}

  LirAdapterErrorKind kind() const noexcept { return kind_; }

 private:
  LirAdapterErrorKind kind_;
};

struct BackendParam {
  std::string type_str;
  std::string name;
};

struct BackendFunctionSignature {
  std::string linkage;
  std::string return_type;
  std::string name;
  std::vector<BackendParam> params;
  bool is_vararg = false;
};

enum class BackendBinaryOpcode {
  Add,
  Sub,
  Mul,
  SDiv,
  SRem,
};

struct BackendBinaryInst {
  BackendBinaryOpcode opcode = BackendBinaryOpcode::Add;
  std::string result;
  std::string type_str;
  std::string lhs;
  std::string rhs;
};

using BackendCallArg = c4c::codegen::lir::OwnedLirTypedCallArg;

struct BackendCallInst {
  std::string result;
  std::string return_type;
  std::string callee;
  std::vector<std::string> param_types;
  std::vector<BackendCallArg> args;
  bool render_callee_type_suffix = false;
};

using BackendInst = std::variant<BackendBinaryInst, BackendCallInst>;

struct BackendReturn {
  std::optional<std::string> value;
  std::string type_str;
};

struct BackendBlock {
  std::string label;
  std::vector<BackendInst> insts;
  BackendReturn terminator;
};

struct BackendFunction {
  BackendFunctionSignature signature;
  bool is_declaration = false;
  std::vector<BackendBlock> blocks;
};

struct BackendModule {
  std::string target_triple;
  std::string data_layout;
  std::vector<std::string> type_decls;
  std::vector<BackendFunction> functions;
};

using BackendTypedCallArgView = c4c::codegen::lir::LirTypedCallArgView;
using ParsedBackendTypedCallView = c4c::codegen::lir::ParsedLirTypedCallView;

struct ParsedBackendDirectGlobalTypedCallView {
  std::string_view symbol_name;
  ParsedBackendTypedCallView typed_call;
};

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call);
std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const BackendCallInst& call);
std::optional<ParsedBackendDirectGlobalTypedCallView> parse_backend_direct_global_typed_call(
    const BackendCallInst& call);

template <std::size_t N>
inline bool backend_typed_call_has_param_types(
    const ParsedBackendTypedCallView& parsed,
    const std::array<std::string_view, N>& expected_types) {
  if (parsed.param_types.size() != expected_types.size()) {
    return false;
  }
  for (std::size_t index = 0; index < expected_types.size(); ++index) {
    if (parsed.param_types[index] != expected_types[index]) {
      return false;
    }
  }
  return true;
}

template <typename DirectCallView, std::size_t N>
inline bool backend_direct_global_typed_call_matches(
    const DirectCallView& parsed,
    std::string_view expected_symbol_name,
    const std::array<std::string_view, N>& expected_types) {
  if (parsed.symbol_name != expected_symbol_name ||
      parsed.typed_call.param_types.size() != expected_types.size()) {
    return false;
  }
  for (std::size_t index = 0; index < expected_types.size(); ++index) {
    if (c4c::codegen::lir::trim_lir_arg_text(parsed.typed_call.param_types[index]) !=
        c4c::codegen::lir::trim_lir_arg_text(expected_types[index])) {
      return false;
    }
  }
  return true;
}

template <typename Call>
inline std::optional<std::string_view> parse_backend_direct_global_single_typed_call_operand(
    const Call& call,
    std::string_view expected_symbol_name,
    std::string_view expected_type) {
  const auto parsed = parse_backend_direct_global_typed_call(call);
  if (!parsed.has_value() ||
      !backend_direct_global_typed_call_matches(
          *parsed, expected_symbol_name, std::array<std::string_view, 1>{expected_type})) {
    return std::nullopt;
  }
  return parsed->typed_call.args.front().operand;
}

template <typename Call>
inline std::optional<std::pair<std::string_view, std::string_view>>
parse_backend_direct_global_two_typed_call_operands(
    const Call& call,
    std::string_view expected_symbol_name,
    std::string_view expected_type0,
    std::string_view expected_type1) {
  const auto parsed = parse_backend_direct_global_typed_call(call);
  if (!parsed.has_value() ||
      !backend_direct_global_typed_call_matches(
          *parsed,
          expected_symbol_name,
          std::array<std::string_view, 2>{expected_type0, expected_type1})) {
    return std::nullopt;
  }
  return std::pair<std::string_view, std::string_view>{
      parsed->typed_call.args[0].operand,
      parsed->typed_call.args[1].operand,
  };
}

template <typename Call>
inline std::optional<std::string_view> parse_backend_zero_arg_direct_global_typed_call(
    const Call& call) {
  const auto parsed = parse_backend_direct_global_typed_call(call);
  if (!parsed.has_value() || !parsed->typed_call.param_types.empty() ||
      !parsed->typed_call.args.empty()) {
    return std::nullopt;
  }
  return parsed->symbol_name;
}

BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module);
std::string render_module(const BackendModule& module);

}  // namespace c4c::backend
