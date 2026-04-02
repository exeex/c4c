#pragma once

#include "../ir.hpp"

#include "../../codegen/lir/call_args.hpp"
#include "../../codegen/lir/ir.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace c4c::backend {

using BackendTypedCallArgView = c4c::codegen::lir::LirTypedCallArgView;
using ParsedBackendTypedCallView = c4c::codegen::lir::ParsedLirTypedCallView;

struct ParsedBackendDirectGlobalTypedCallView {
  std::string_view symbol_name;
  ParsedBackendTypedCallView typed_call;
};

inline std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const c4c::codegen::lir::LirCallOp& call) {
  return c4c::codegen::lir::parse_lir_typed_call_or_infer_params(call);
}

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call);
std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const BackendCallInst& call);
std::optional<ParsedBackendDirectGlobalTypedCallView> parse_backend_direct_global_typed_call(
    const BackendCallInst& call);

template <std::size_t N>
inline bool backend_typed_call_matches(
    const ParsedBackendTypedCallView& parsed,
    const std::array<std::string_view, N>& expected_types) {
  if (parsed.param_types.size() != expected_types.size() ||
      parsed.args.size() != expected_types.size()) {
    return false;
  }
  for (std::size_t index = 0; index < expected_types.size(); ++index) {
    if (c4c::codegen::lir::trim_lir_arg_text(parsed.param_types[index]) !=
        c4c::codegen::lir::trim_lir_arg_text(expected_types[index])) {
      return false;
    }
  }
  return true;
}

template <typename Call>
inline std::optional<std::string_view> parse_backend_single_typed_call_operand(
    const Call& call,
    std::string_view expected_type) {
  const auto parsed = parse_backend_typed_call(call);
  if (!parsed.has_value() ||
      !backend_typed_call_matches(*parsed, std::array<std::string_view, 1>{expected_type})) {
    return std::nullopt;
  }
  return parsed->args.front().operand;
}

template <typename Call>
inline std::optional<std::pair<std::string_view, std::string_view>>
parse_backend_two_typed_call_operands(const Call& call,
                                      std::string_view expected_type0,
                                      std::string_view expected_type1) {
  const auto parsed = parse_backend_typed_call(call);
  if (!parsed.has_value() ||
      !backend_typed_call_matches(
          *parsed, std::array<std::string_view, 2>{expected_type0, expected_type1})) {
    return std::nullopt;
  }
  return std::make_pair(parsed->args[0].operand, parsed->args[1].operand);
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
  return std::make_pair(parsed->typed_call.args[0].operand,
                        parsed->typed_call.args[1].operand);
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

inline bool matches_backend_zero_add_slot_rewrite(
    const c4c::codegen::lir::LirLoadOp& load,
    const c4c::codegen::lir::LirBinOp& add,
    const c4c::codegen::lir::LirStoreOp& store,
    std::string_view slot) {
  using namespace c4c::codegen::lir;

  if (load.type_str != "i32" || load.ptr != slot ||
      add.opcode.typed() != LirBinaryOpcode::Add || add.type_str != "i32" ||
      add.result.empty() || store.type_str != "i32" || store.ptr != slot ||
      store.val != add.result) {
    return false;
  }

  const bool add_zero_on_rhs = add.lhs == load.result && add.rhs == "0";
  const bool add_zero_on_lhs = add.lhs == "0" && add.rhs == load.result;
  return add_zero_on_rhs || add_zero_on_lhs;
}

inline bool matches_backend_zero_add_slot_rewrite_with_reload(
    const c4c::codegen::lir::LirLoadOp& load,
    const c4c::codegen::lir::LirBinOp& add,
    const c4c::codegen::lir::LirStoreOp& store,
    const c4c::codegen::lir::LirLoadOp& reload,
    std::string_view slot) {
  if (reload.type_str != "i32" || reload.ptr != slot) {
    return false;
  }
  return matches_backend_zero_add_slot_rewrite(load, add, store, slot);
}
}  // namespace c4c::backend
