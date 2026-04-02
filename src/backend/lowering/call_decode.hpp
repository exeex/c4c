#pragma once

#include "../ir.hpp"

#include "../../codegen/lir/call_args.hpp"
#include "../../codegen/lir/ir.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using BackendTypedCallArgView = c4c::codegen::lir::LirTypedCallArgView;
using ParsedBackendTypedCallView = c4c::codegen::lir::ParsedLirTypedCallView;

struct ParsedBackendDirectGlobalTypedCallView {
  std::string_view symbol_name;
  ParsedBackendTypedCallView typed_call;
};

struct ParsedBackendExternCallArg {
  enum class Kind { I32Imm, I64Imm, Ptr };

  Kind kind = Kind::Ptr;
  std::int64_t imm = 0;
  std::string operand;
};

using OwnedBackendTypedCallArg = c4c::codegen::lir::OwnedLirTypedCallArg;

struct ParsedBackendFunctionSignatureParam {
  std::string type;
  std::string operand;
  bool is_varargs = false;
};

struct ParsedBackendSingleLocalTypedCallView {
  const c4c::codegen::lir::LirLoadOp* arg_load = nullptr;
  const c4c::codegen::lir::LirCallOp* call = nullptr;
};

struct ParsedBackendTwoLocalTypedCallView {
  const c4c::codegen::lir::LirLoadOp* arg0_load = nullptr;
  const c4c::codegen::lir::LirLoadOp* arg1_load = nullptr;
  const c4c::codegen::lir::LirCallOp* call = nullptr;
};

inline std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const c4c::codegen::lir::LirCallOp& call) {
  return c4c::codegen::lir::parse_lir_typed_call_or_infer_params(call);
}

bool backend_lir_type_uses_nonminimal_types(std::string_view type_text);
bool backend_lir_global_uses_nonminimal_types(const c4c::codegen::lir::LirGlobal& global);
bool backend_lir_return_uses_nonminimal_types(const c4c::codegen::lir::LirRet& ret);
bool backend_lir_call_uses_nonminimal_types(const c4c::codegen::lir::LirCallOp& call);
bool backend_lir_function_signature_uses_nonminimal_types(std::string_view signature_text);
bool backend_lir_signature_matches(std::string_view signature_text,
                                   std::string_view expected_linkage,
                                   std::string_view expected_return_type,
                                   std::string_view expected_function_name,
                                   std::initializer_list<std::string_view> expected_param_types);
bool backend_lir_is_i32_main_definition(std::string_view signature_text);
bool backend_lir_is_zero_arg_i32_main_definition(std::string_view signature_text);

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call);
std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const BackendCallInst& call);
std::optional<ParsedBackendDirectGlobalTypedCallView> parse_backend_direct_global_typed_call(
    const BackendCallInst& call);
std::optional<ParsedBackendExternCallArg> parse_backend_extern_call_arg(
    std::string_view type,
    std::string_view operand);
bool backend_typed_call_matches_signature(const ParsedBackendTypedCallView& parsed,
                                          const BackendFunctionSignature& signature,
                                          bool allow_extra_varargs = false);
std::optional<std::vector<ParsedBackendExternCallArg>> parse_backend_extern_call_args(
    const ParsedBackendTypedCallView& parsed);
std::optional<std::vector<OwnedBackendTypedCallArg>> parse_backend_owned_typed_call_args(
    std::string_view args_str);
std::optional<std::vector<ParsedBackendFunctionSignatureParam>>
parse_backend_function_signature_params(std::string_view signature_text);
void collect_backend_call_value_names(const c4c::codegen::lir::LirCallOp& call,
                                      std::vector<std::string>& values);

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

inline std::optional<ParsedBackendSingleLocalTypedCallView>
parse_backend_single_local_typed_call(const std::vector<c4c::codegen::lir::LirInst>& insts,
                                      std::string_view slot) {
  using namespace c4c::codegen::lir;

  ParsedBackendSingleLocalTypedCallView parsed;
  if (insts.size() == 3) {
    parsed.arg_load = std::get_if<LirLoadOp>(&insts[1]);
    parsed.call = std::get_if<LirCallOp>(&insts[2]);
    if (parsed.arg_load == nullptr || parsed.call == nullptr ||
        parsed.arg_load->type_str != "i32" || parsed.arg_load->ptr != slot) {
      return std::nullopt;
    }
    return parsed;
  }

  if (insts.size() != 6) {
    return std::nullopt;
  }

  const auto* load_before = std::get_if<LirLoadOp>(&insts[1]);
  const auto* add = std::get_if<LirBinOp>(&insts[2]);
  const auto* store_rewrite = std::get_if<LirStoreOp>(&insts[3]);
  parsed.arg_load = std::get_if<LirLoadOp>(&insts[4]);
  parsed.call = std::get_if<LirCallOp>(&insts[5]);
  if (load_before == nullptr || add == nullptr || store_rewrite == nullptr ||
      parsed.arg_load == nullptr || parsed.call == nullptr ||
      !matches_backend_zero_add_slot_rewrite_with_reload(
          *load_before, *add, *store_rewrite, *parsed.arg_load, slot)) {
    return std::nullopt;
  }
  return parsed;
}

inline std::optional<ParsedBackendTwoLocalTypedCallView>
parse_backend_two_local_typed_call(const std::vector<c4c::codegen::lir::LirInst>& insts,
                                   std::string_view slot0,
                                   std::string_view slot1) {
  using namespace c4c::codegen::lir;

  ParsedBackendTwoLocalTypedCallView parsed;
  if (insts.size() == 5) {
    parsed.arg0_load = std::get_if<LirLoadOp>(&insts[2]);
    parsed.arg1_load = std::get_if<LirLoadOp>(&insts[3]);
    parsed.call = std::get_if<LirCallOp>(&insts[4]);
    if (parsed.arg0_load == nullptr || parsed.arg1_load == nullptr ||
        parsed.call == nullptr || parsed.arg0_load->type_str != "i32" ||
        parsed.arg1_load->type_str != "i32" || parsed.arg0_load->ptr != slot0 ||
        parsed.arg1_load->ptr != slot1) {
      return std::nullopt;
    }
    return parsed;
  }

  if (insts.size() == 11) {
    const auto* load0_before = std::get_if<LirLoadOp>(&insts[2]);
    const auto* add0 = std::get_if<LirBinOp>(&insts[3]);
    const auto* store0_rewrite = std::get_if<LirStoreOp>(&insts[4]);
    const auto* load1_before = std::get_if<LirLoadOp>(&insts[5]);
    const auto* add1 = std::get_if<LirBinOp>(&insts[6]);
    const auto* store1_rewrite = std::get_if<LirStoreOp>(&insts[7]);
    parsed.arg0_load = std::get_if<LirLoadOp>(&insts[8]);
    parsed.arg1_load = std::get_if<LirLoadOp>(&insts[9]);
    parsed.call = std::get_if<LirCallOp>(&insts[10]);
    if (load0_before == nullptr || add0 == nullptr || store0_rewrite == nullptr ||
        load1_before == nullptr || add1 == nullptr || store1_rewrite == nullptr ||
        parsed.arg0_load == nullptr || parsed.arg1_load == nullptr ||
        parsed.call == nullptr ||
        !matches_backend_zero_add_slot_rewrite(
            *load0_before, *add0, *store0_rewrite, slot0) ||
        !matches_backend_zero_add_slot_rewrite(
            *load1_before, *add1, *store1_rewrite, slot1) ||
        parsed.arg0_load->type_str != "i32" || parsed.arg0_load->ptr != slot0 ||
        parsed.arg1_load->type_str != "i32" || parsed.arg1_load->ptr != slot1) {
      return std::nullopt;
    }
    return parsed;
  }

  if (insts.size() != 8) {
    return std::nullopt;
  }

  parsed.call = std::get_if<LirCallOp>(&insts[7]);
  if (parsed.call == nullptr) {
    return std::nullopt;
  }

  const auto* load0_before = std::get_if<LirLoadOp>(&insts[2]);
  const auto* add0 = std::get_if<LirBinOp>(&insts[3]);
  const auto* store0_rewrite = std::get_if<LirStoreOp>(&insts[4]);
  parsed.arg0_load = std::get_if<LirLoadOp>(&insts[5]);
  parsed.arg1_load = std::get_if<LirLoadOp>(&insts[6]);
  if (load0_before != nullptr && add0 != nullptr && store0_rewrite != nullptr &&
      parsed.arg0_load != nullptr && parsed.arg1_load != nullptr &&
      matches_backend_zero_add_slot_rewrite_with_reload(
          *load0_before, *add0, *store0_rewrite, *parsed.arg0_load, slot0) &&
      parsed.arg1_load->type_str == "i32" && parsed.arg1_load->ptr == slot1) {
    return parsed;
  }

  const auto* load1_before = std::get_if<LirLoadOp>(&insts[2]);
  const auto* add1 = std::get_if<LirBinOp>(&insts[3]);
  const auto* store1_rewrite = std::get_if<LirStoreOp>(&insts[4]);
  parsed.arg0_load = std::get_if<LirLoadOp>(&insts[5]);
  parsed.arg1_load = std::get_if<LirLoadOp>(&insts[6]);
  if (load1_before == nullptr || add1 == nullptr || store1_rewrite == nullptr ||
      parsed.arg0_load == nullptr || parsed.arg1_load == nullptr ||
      !matches_backend_zero_add_slot_rewrite_with_reload(
          *load1_before, *add1, *store1_rewrite, *parsed.arg1_load, slot1) ||
      parsed.arg0_load->type_str != "i32" || parsed.arg0_load->ptr != slot0) {
    return std::nullopt;
  }
  return parsed;
}
}  // namespace c4c::backend
