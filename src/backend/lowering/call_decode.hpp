#pragma once

#include "../ir.hpp"

#include "../../codegen/lir/call_args.hpp"
#include "../../codegen/lir/ir.hpp"

#include <charconv>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using BackendTypedCallArgView = c4c::codegen::lir::LirTypedCallArgView;

struct ParsedBackendTypedCallView {
  std::vector<std::string> owned_param_types;
  std::vector<std::string_view> param_types;
  std::vector<BackendTypedCallArgView> args;
};

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

struct ParsedBackendSingleParamSlotAddFunctionView {
  std::string param_name;
  std::string slot_name;
  const c4c::codegen::lir::LirBinOp* add = nullptr;
};

struct ParsedBackendSingleAddImmFunctionView {
  std::string param_name;
  const c4c::codegen::lir::LirBinOp* add = nullptr;
};

struct ParsedBackendTwoParamAddFunctionView {
  std::string lhs_param_name;
  std::string rhs_param_name;
  const c4c::codegen::lir::LirBinOp* add = nullptr;
};

struct ParsedBackendStructuredTwoParamAddFunctionView {
  std::string lhs_param_name;
  std::string rhs_param_name;
  const BackendBinaryInst* add = nullptr;
};

struct ParsedBackendStructuredZeroArgReturnImmFunctionView {
  std::int64_t return_imm = 0;
};

struct ParsedBackendStructuredSingleAddImmFunctionView {
  std::string param_name;
  const BackendBinaryInst* add = nullptr;
};

struct ParsedBackendStructuredFoldedTwoArgFunctionView {
  std::string lhs_param_name;
  std::string rhs_param_name;
  std::int64_t base_imm = 0;
  const BackendBinaryInst* add = nullptr;
  const BackendBinaryInst* sub = nullptr;
};

struct ParsedBackendSingleHelperMainLirModuleView {
  const c4c::codegen::lir::LirFunction* helper = nullptr;
  const c4c::codegen::lir::LirFunction* main_function = nullptr;
  const c4c::codegen::lir::LirBlock* main_block = nullptr;
  const c4c::codegen::lir::LirRet* main_ret = nullptr;
};

struct ParsedBackendSingleHelperDirectGlobalCallView {
  const c4c::codegen::lir::LirCallOp* call = nullptr;
  std::string_view operand;
  std::string_view callee_name;
};

struct ParsedBackendMinimalTwoArgDirectCallLirModuleView {
  const c4c::codegen::lir::LirFunction* helper = nullptr;
  const c4c::codegen::lir::LirFunction* main_function = nullptr;
  const c4c::codegen::lir::LirCallOp* call = nullptr;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct ParsedBackendMinimalTwoArgDirectCallModuleView {
  const BackendFunction* helper = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendCallInst* call = nullptr;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct ParsedBackendMinimalStructuredDirectCallModuleView {
  const BackendFunction* helper = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendBlock* main_block = nullptr;
  const BackendCallInst* call = nullptr;
  ParsedBackendDirectGlobalTypedCallView parsed_call;
};

struct ParsedBackendMinimalDirectCallModuleView {
  const BackendFunction* helper = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendCallInst* call = nullptr;
  std::int64_t return_imm = 0;
};

struct ParsedBackendMinimalDirectCallAddImmModuleView {
  const BackendFunction* helper = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendCallInst* call = nullptr;
  std::int64_t call_arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct ParsedBackendMinimalFoldedTwoArgDirectCallModuleView {
  const BackendFunction* helper = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendCallInst* call = nullptr;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
  std::int64_t return_imm = 0;
};

struct ParsedBackendMinimalCallCrossingDirectCallModuleView {
  const BackendFunction* helper = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendBinaryInst* source_add = nullptr;
  const BackendCallInst* call = nullptr;
  const BackendBinaryInst* final_add = nullptr;
  std::string_view regalloc_source_value;
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
};

struct ParsedBackendMinimalDeclaredDirectCallModuleView {
  const BackendFunction* callee = nullptr;
  const BackendFunction* main_function = nullptr;
  const BackendBlock* main_block = nullptr;
  const BackendCallInst* call = nullptr;
  ParsedBackendDirectGlobalTypedCallView parsed_call;
  std::vector<ParsedBackendExternCallArg> args;
  bool return_call_result = false;
  std::int64_t return_imm = 0;
};

inline std::optional<ParsedBackendTwoParamAddFunctionView>
parse_backend_two_param_add_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name);

inline std::optional<ParsedBackendStructuredTwoParamAddFunctionView>
parse_backend_structured_two_param_add_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt);

inline std::optional<ParsedBackendStructuredZeroArgReturnImmFunctionView>
parse_backend_structured_zero_arg_return_imm_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt);

inline std::optional<ParsedBackendStructuredSingleAddImmFunctionView>
parse_backend_structured_single_add_imm_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt);

inline std::optional<ParsedBackendStructuredFoldedTwoArgFunctionView>
parse_backend_structured_folded_two_arg_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt);

inline ParsedBackendTypedCallView make_backend_typed_call_view(
    const c4c::codegen::lir::ParsedLirTypedCallView& parsed) {
  ParsedBackendTypedCallView view;
  view.param_types = parsed.param_types;
  view.args = parsed.args;
  return view;
}

inline std::optional<ParsedBackendTypedCallView> parse_backend_typed_call(
    const c4c::codegen::lir::LirCallOp& call) {
  const auto parsed = c4c::codegen::lir::parse_lir_typed_call_or_infer_params(call);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return make_backend_typed_call_view(*parsed);
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
std::optional<std::vector<OwnedBackendTypedCallArg>> parse_backend_owned_typed_call_args(
    const c4c::codegen::lir::LirCallOp& call);
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

inline std::optional<std::int64_t> parse_backend_i64_literal(std::string_view text) {
  std::int64_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc() || result.ptr != end) {
    return std::nullopt;
  }
  return value;
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

inline std::optional<ParsedBackendSingleHelperMainLirModuleView>
parse_backend_single_helper_zero_arg_main_lir_module(
    const c4c::codegen::lir::LirModule& module,
    std::size_t expected_main_inst_count) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const LirFunction* main_fn = nullptr;
  const LirFunction* helper = nullptr;
  for (const auto& function : module.functions) {
    if (function.name == "main") {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
      continue;
    }

    if (helper != nullptr) {
      return std::nullopt;
    }
    helper = &function;
  }

  if (helper == nullptr || main_fn == nullptr || helper->is_declaration ||
      main_fn->is_declaration ||
      !backend_lir_is_zero_arg_i32_main_definition(main_fn->signature_text) ||
      main_fn->entry.value != 0 || main_fn->blocks.size() != 1 || !main_fn->alloca_insts.empty() ||
      !main_fn->stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
  if (main_block.label != "entry" || main_block.insts.size() != expected_main_inst_count ||
      main_ret == nullptr || !main_ret->value_str.has_value() || main_ret->type_str != "i32") {
    return std::nullopt;
  }

  return ParsedBackendSingleHelperMainLirModuleView{
      helper,
      main_fn,
      &main_block,
      main_ret,
  };
}

inline std::optional<ParsedBackendStructuredTwoParamAddFunctionView>
parse_backend_structured_two_param_add_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name) {
  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.signature.name);
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      function.signature.name != function_name ||
      backend_signature_return_scalar_type(function.signature) != BackendScalarType::I32 ||
      function.signature.params.size() != 2 || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lhs_param = function.signature.params[0];
  const auto& rhs_param = function.signature.params[1];
  if (backend_param_scalar_type(lhs_param) != BackendScalarType::I32 ||
      backend_param_scalar_type(rhs_param) != BackendScalarType::I32 ||
      lhs_param.name.empty() || rhs_param.name.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      backend_return_scalar_type(block.terminator) != BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* add = std::get_if<BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != BackendBinaryOpcode::Add ||
      backend_binary_value_type(*add) != BackendScalarType::I32 || add->result.empty() ||
      *block.terminator.value != add->result || add->lhs != lhs_param.name ||
      add->rhs != rhs_param.name) {
    return std::nullopt;
  }

  return ParsedBackendStructuredTwoParamAddFunctionView{
      lhs_param.name,
      rhs_param.name,
      add,
  };
}

inline std::optional<ParsedBackendStructuredZeroArgReturnImmFunctionView>
parse_backend_structured_zero_arg_return_imm_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name) {
  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.signature.name);
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      function.signature.name != function_name ||
      backend_signature_return_scalar_type(function.signature) != BackendScalarType::I32 ||
      !function.signature.params.empty() || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.terminator.value.has_value() ||
      backend_return_scalar_type(block.terminator) != BackendScalarType::I32 ||
      !block.insts.empty()) {
    return std::nullopt;
  }

  const auto return_imm = parse_backend_i64_literal(*block.terminator.value);
  if (!return_imm.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendStructuredZeroArgReturnImmFunctionView{*return_imm};
}

inline std::optional<ParsedBackendStructuredSingleAddImmFunctionView>
parse_backend_structured_single_add_imm_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name) {
  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.signature.name);
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      function.signature.name != function_name ||
      backend_signature_return_scalar_type(function.signature) != BackendScalarType::I32 ||
      function.signature.params.size() != 1 || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& param = function.signature.params.front();
  if (backend_param_scalar_type(param) != BackendScalarType::I32 || param.name.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 1 ||
      !block.terminator.value.has_value() ||
      backend_return_scalar_type(block.terminator) != BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* add = std::get_if<BackendBinaryInst>(&block.insts.front());
  if (add == nullptr || add->opcode != BackendBinaryOpcode::Add ||
      backend_binary_value_type(*add) != BackendScalarType::I32 || add->result.empty() ||
      *block.terminator.value != add->result || add->lhs != param.name ||
      !parse_backend_i64_literal(add->rhs).has_value()) {
    return std::nullopt;
  }

  return ParsedBackendStructuredSingleAddImmFunctionView{param.name, add};
}

inline std::optional<ParsedBackendStructuredFoldedTwoArgFunctionView>
parse_backend_structured_folded_two_arg_function(
    const BackendFunction& function,
    std::optional<std::string_view> expected_name) {
  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.signature.name);
  if (function.is_declaration || !backend_function_is_definition(function.signature) ||
      function.signature.name != function_name ||
      backend_signature_return_scalar_type(function.signature) != BackendScalarType::I32 ||
      function.signature.params.size() != 2 || function.signature.is_vararg ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& lhs_param = function.signature.params[0];
  const auto& rhs_param = function.signature.params[1];
  if (backend_param_scalar_type(lhs_param) != BackendScalarType::I32 ||
      backend_param_scalar_type(rhs_param) != BackendScalarType::I32 ||
      lhs_param.name.empty() || rhs_param.name.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || block.insts.size() != 2 ||
      !block.terminator.value.has_value() ||
      backend_return_scalar_type(block.terminator) != BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* add = std::get_if<BackendBinaryInst>(&block.insts[0]);
  const auto* sub = std::get_if<BackendBinaryInst>(&block.insts[1]);
  if (add == nullptr || sub == nullptr ||
      add->opcode != BackendBinaryOpcode::Add ||
      sub->opcode != BackendBinaryOpcode::Sub ||
      backend_binary_value_type(*add) != BackendScalarType::I32 ||
      backend_binary_value_type(*sub) != BackendScalarType::I32 ||
      add->result.empty() || sub->result.empty() ||
      add->rhs != lhs_param.name || sub->lhs != add->result ||
      sub->rhs != rhs_param.name || *block.terminator.value != sub->result) {
    return std::nullopt;
  }

  const auto base_imm = parse_backend_i64_literal(add->lhs);
  if (!base_imm.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendStructuredFoldedTwoArgFunctionView{
      lhs_param.name,
      rhs_param.name,
      *base_imm,
      add,
      sub,
  };
}

inline std::optional<ParsedBackendSingleHelperDirectGlobalCallView>
parse_backend_single_helper_direct_global_call(
    const ParsedBackendSingleHelperMainLirModuleView& module,
    std::size_t call_inst_index) {
  using namespace c4c::codegen::lir;

  if (module.helper == nullptr || module.main_block == nullptr ||
      call_inst_index >= module.main_block->insts.size()) {
    return std::nullopt;
  }

  const auto* call = std::get_if<LirCallOp>(&module.main_block->insts[call_inst_index]);
  const auto call_operand =
      call == nullptr
          ? std::nullopt
          : parse_backend_direct_global_single_typed_call_operand(
                *call, module.helper->name, "i32");
  if (call == nullptr || !call_operand.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendSingleHelperDirectGlobalCallView{
      call,
      *call_operand,
      module.helper->name,
  };
}

inline std::optional<ParsedBackendMinimalStructuredDirectCallModuleView>
parse_backend_minimal_structured_direct_call_module(const BackendModule& module,
                                                    std::size_t expected_arg_count) {
  if (module.functions.size() != 2) {
    return std::nullopt;
  }

  const BackendFunction* main_fn = nullptr;
  const BackendFunction* helper = nullptr;
  for (const auto& function : module.functions) {
    if (function.signature.name == "main") {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
      continue;
    }
    if (helper != nullptr) {
      return std::nullopt;
    }
    helper = &function;
  }

  if (helper == nullptr || main_fn == nullptr || helper->is_declaration || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      !backend_function_is_definition(helper->signature) ||
      backend_signature_return_scalar_type(main_fn->signature) != BackendScalarType::I32 ||
      backend_signature_return_scalar_type(helper->signature) != BackendScalarType::I32 ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      helper->signature.is_vararg || helper->blocks.size() != 1 || main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      backend_return_scalar_type(main_block.terminator) != BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* call = std::get_if<BackendCallInst>(&main_block.insts.front());
  const auto parsed_call =
      call == nullptr ? std::nullopt : parse_backend_direct_global_typed_call(*call);
  if (call == nullptr || !parsed_call.has_value() ||
      !backend_typed_call_matches_signature(parsed_call->typed_call, helper->signature) ||
      backend_call_return_type_kind(*call) != BackendValueTypeKind::Scalar ||
      backend_call_return_scalar_type(*call) != BackendScalarType::I32 ||
      call->result.empty() || parsed_call->symbol_name.empty() || parsed_call->symbol_name == "main" ||
      parsed_call->symbol_name != helper->signature.name ||
      parsed_call->typed_call.args.size() != expected_arg_count ||
      *main_block.terminator.value != call->result) {
    return std::nullopt;
  }

  return ParsedBackendMinimalStructuredDirectCallModuleView{
      helper,
      main_fn,
      &main_block,
      call,
      *parsed_call,
  };
}

inline std::optional<ParsedBackendMinimalTwoArgDirectCallModuleView>
parse_backend_minimal_two_arg_direct_call_module(const BackendModule& module) {
  if (!module.globals.empty() || !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto parsed = parse_backend_minimal_structured_direct_call_module(module, 2);
  if (!parsed.has_value() ||
      !parse_backend_structured_two_param_add_function(*parsed->helper, std::nullopt).has_value()) {
    return std::nullopt;
  }

  const auto lhs_call_arg_imm = parse_backend_i64_literal(parsed->call->args[0].operand);
  const auto rhs_call_arg_imm = parse_backend_i64_literal(parsed->call->args[1].operand);
  if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalTwoArgDirectCallModuleView{
      parsed->helper,
      parsed->main_function,
      parsed->call,
      *lhs_call_arg_imm,
      *rhs_call_arg_imm,
  };
}

inline std::optional<ParsedBackendMinimalDirectCallModuleView>
parse_backend_minimal_direct_call_module(const BackendModule& module) {
  if (!module.globals.empty() || !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto parsed = parse_backend_minimal_structured_direct_call_module(module, 0);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto helper_shape =
      parse_backend_structured_zero_arg_return_imm_function(*parsed->helper, std::nullopt);
  if (!helper_shape.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalDirectCallModuleView{
      parsed->helper,
      parsed->main_function,
      parsed->call,
      helper_shape->return_imm,
  };
}

inline std::optional<ParsedBackendMinimalDirectCallAddImmModuleView>
parse_backend_minimal_direct_call_add_imm_module(const BackendModule& module) {
  if (!module.globals.empty() || !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto parsed = parse_backend_minimal_structured_direct_call_module(module, 1);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto helper_shape =
      parse_backend_structured_single_add_imm_function(*parsed->helper, std::nullopt);
  if (!helper_shape.has_value()) {
    return std::nullopt;
  }

  const auto call_arg_imm = parse_backend_i64_literal(parsed->call->args.front().operand);
  const auto add_imm = parse_backend_i64_literal(helper_shape->add->rhs);
  if (!call_arg_imm.has_value() || !add_imm.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalDirectCallAddImmModuleView{
      parsed->helper,
      parsed->main_function,
      parsed->call,
      *call_arg_imm,
      *add_imm,
  };
}

inline std::optional<ParsedBackendMinimalFoldedTwoArgDirectCallModuleView>
parse_backend_minimal_folded_two_arg_direct_call_module(const BackendModule& module) {
  if (!module.globals.empty() || !module.string_constants.empty()) {
    return std::nullopt;
  }

  const auto parsed = parse_backend_minimal_structured_direct_call_module(module, 2);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto helper_shape =
      parse_backend_structured_folded_two_arg_function(*parsed->helper, std::nullopt);
  if (!helper_shape.has_value()) {
    return std::nullopt;
  }

  const auto lhs_call_arg_imm = parse_backend_i64_literal(parsed->call->args[0].operand);
  const auto rhs_call_arg_imm = parse_backend_i64_literal(parsed->call->args[1].operand);
  if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalFoldedTwoArgDirectCallModuleView{
      parsed->helper,
      parsed->main_function,
      parsed->call,
      *lhs_call_arg_imm,
      *rhs_call_arg_imm,
      helper_shape->base_imm + *lhs_call_arg_imm - *rhs_call_arg_imm,
  };
}

inline std::optional<ParsedBackendMinimalCallCrossingDirectCallModuleView>
parse_backend_minimal_call_crossing_direct_call_module(const BackendModule& module) {
  if (module.functions.size() != 2) {
    return std::nullopt;
  }

  const BackendFunction* main_fn = nullptr;
  const BackendFunction* helper = nullptr;
  for (const auto& function : module.functions) {
    if (function.signature.name == "main") {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
      continue;
    }
    if (helper != nullptr) {
      return std::nullopt;
    }
    helper = &function;
  }

  if (helper == nullptr || main_fn == nullptr || helper->is_declaration || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      backend_signature_return_scalar_type(main_fn->signature) != BackendScalarType::I32 ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto helper_shape =
      parse_backend_structured_single_add_imm_function(*helper, std::nullopt);
  if (!helper_shape.has_value()) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 3 ||
      !main_block.terminator.value.has_value() ||
      backend_return_scalar_type(main_block.terminator) != BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* source_add = std::get_if<BackendBinaryInst>(&main_block.insts[0]);
  const auto* call = std::get_if<BackendCallInst>(&main_block.insts[1]);
  const auto* final_add = std::get_if<BackendBinaryInst>(&main_block.insts[2]);
  const auto parsed_call =
      call == nullptr ? std::nullopt : parse_backend_direct_global_typed_call(*call);
  if (source_add == nullptr || call == nullptr || final_add == nullptr || !parsed_call.has_value() ||
      source_add->opcode != BackendBinaryOpcode::Add ||
      backend_binary_value_type(*source_add) != BackendScalarType::I32 ||
      parsed_call->symbol_name != helper->signature.name ||
      !backend_typed_call_matches(parsed_call->typed_call, std::array<std::string_view, 1>{"i32"}) ||
      call->result.empty() || call->args.front().operand != source_add->result ||
      final_add->opcode != BackendBinaryOpcode::Add ||
      backend_binary_value_type(*final_add) != BackendScalarType::I32 ||
      final_add->lhs != source_add->result || final_add->rhs != call->result ||
      *main_block.terminator.value != final_add->result) {
    return std::nullopt;
  }

  const auto lhs_imm = parse_backend_i64_literal(source_add->lhs);
  const auto rhs_imm = parse_backend_i64_literal(source_add->rhs);
  const auto add_imm = parse_backend_i64_literal(helper_shape->add->rhs);
  if (!lhs_imm.has_value() || !rhs_imm.has_value() || !add_imm.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalCallCrossingDirectCallModuleView{
      helper,
      main_fn,
      source_add,
      call,
      final_add,
      source_add->result,
      *lhs_imm + *rhs_imm,
      *add_imm,
  };
}

inline std::optional<ParsedBackendMinimalDeclaredDirectCallModuleView>
parse_backend_minimal_declared_direct_call_module(const BackendModule& module) {
  if (module.functions.size() < 2) {
    return std::nullopt;
  }

  const BackendFunction* main_fn = nullptr;
  for (const auto& function : module.functions) {
    if (function.signature.name == "main") {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
    }
  }

  if (main_fn == nullptr || main_fn->is_declaration ||
      !backend_function_is_definition(main_fn->signature) ||
      backend_signature_return_scalar_type(main_fn->signature) != BackendScalarType::I32 ||
      !main_fn->signature.params.empty() || main_fn->signature.is_vararg ||
      main_fn->blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value() ||
      backend_return_scalar_type(main_block.terminator) != BackendScalarType::I32) {
    return std::nullopt;
  }

  const auto* call = std::get_if<BackendCallInst>(&main_block.insts.front());
  const auto parsed_call =
      call == nullptr ? std::nullopt : parse_backend_direct_global_typed_call(*call);
  if (call == nullptr || !parsed_call.has_value() ||
      backend_call_return_type_kind(*call) != BackendValueTypeKind::Scalar ||
      backend_call_return_scalar_type(*call) != BackendScalarType::I32 ||
      parsed_call->symbol_name.empty() || parsed_call->symbol_name == "main" ||
      parsed_call->typed_call.args.size() > 6) {
    return std::nullopt;
  }

  const BackendFunction* callee = nullptr;
  for (const auto& function : module.functions) {
    if (function.signature.name == parsed_call->symbol_name) {
      if (callee != nullptr) {
        return std::nullopt;
      }
      callee = &function;
    }
  }

  if (callee == nullptr || !callee->is_declaration ||
      !backend_function_is_declaration(callee->signature) ||
      backend_signature_return_scalar_type(callee->signature) != BackendScalarType::I32 ||
      !backend_typed_call_matches_signature(parsed_call->typed_call,
                                            callee->signature,
                                            callee->signature.is_vararg)) {
    return std::nullopt;
  }

  const bool return_call_result =
      !call->result.empty() && *main_block.terminator.value == call->result;
  std::int64_t return_imm = 0;
  if (!return_call_result) {
    const auto parsed_return = parse_backend_i64_literal(*main_block.terminator.value);
    if (!parsed_return.has_value() ||
        *parsed_return < std::numeric_limits<std::int32_t>::min() ||
        *parsed_return > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }
    return_imm = *parsed_return;
  }

  auto args = parse_backend_extern_call_args(parsed_call->typed_call);
  if (!args.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalDeclaredDirectCallModuleView{
      callee,
      main_fn,
      &main_block,
      call,
      *parsed_call,
      std::move(*args),
      return_call_result,
      return_imm,
  };
}

inline std::optional<std::pair<std::int64_t, bool>>
parse_backend_single_local_i32_slot_call_operand_imm(
    const std::vector<c4c::codegen::lir::LirInst>& insts,
    std::string_view slot,
    std::string_view lhs_call_operand,
    std::string_view rhs_call_operand) {
  using namespace c4c::codegen::lir;

  if (insts.size() < 2) {
    return std::nullopt;
  }

  const auto* store = std::get_if<LirStoreOp>(&insts.front());
  if (store == nullptr || store->type_str != "i32" || store->ptr != slot) {
    return std::nullopt;
  }

  const auto stored_imm = parse_backend_i64_literal(store->val);
  if (!stored_imm.has_value()) {
    return std::nullopt;
  }

  std::string last_load_result;
  std::string rewritten_result;
  bool saw_rewrite = false;
  bool committed_rewrite = false;
  bool matched_lhs_operand = false;
  bool matched_rhs_operand = false;

  for (std::size_t inst_index = 1; inst_index < insts.size(); ++inst_index) {
    const auto& inst = insts[inst_index];

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str != "i32" || load->ptr != slot) {
        return std::nullopt;
      }
      last_load_result = load->result;
      if (committed_rewrite || !saw_rewrite) {
        if (load->result == lhs_call_operand) {
          matched_lhs_operand = true;
        }
        if (load->result == rhs_call_operand) {
          matched_rhs_operand = true;
        }
      }
      continue;
    }

    if (const auto* rewrite = std::get_if<LirBinOp>(&inst)) {
      const auto rewrite_opcode = rewrite->opcode.typed();
      if (saw_rewrite || committed_rewrite || rewrite_opcode != LirBinaryOpcode::Add ||
          rewrite->type_str != "i32" || rewrite->lhs != last_load_result ||
          rewrite->rhs != "0") {
        return std::nullopt;
      }
      rewritten_result = rewrite->result;
      saw_rewrite = true;
      continue;
    }

    if (const auto* rewrite_store = std::get_if<LirStoreOp>(&inst)) {
      if (!saw_rewrite || committed_rewrite || rewrite_store->type_str != "i32" ||
          rewrite_store->ptr != slot || rewrite_store->val != rewritten_result) {
        return std::nullopt;
      }
      committed_rewrite = true;
      continue;
    }

    return std::nullopt;
  }

  if (matched_lhs_operand) {
    return std::pair<std::int64_t, bool>{*stored_imm, true};
  }
  if (matched_rhs_operand) {
    return std::pair<std::int64_t, bool>{*stored_imm, false};
  }
  return std::nullopt;
}

inline std::optional<std::pair<std::int64_t, std::int64_t>>
parse_backend_two_local_i32_rewrite_call_operands_imm(
    const std::vector<c4c::codegen::lir::LirInst>& insts,
    std::string_view lhs_slot,
    std::string_view rhs_slot,
    std::string_view lhs_call_operand,
    std::string_view rhs_call_operand) {
  using namespace c4c::codegen::lir;

  struct SlotState {
    std::string slot_name;
    std::int64_t stored_imm = 0;
    bool initialized = false;
    std::string last_load_result;
    std::string rewritten_result;
    bool matched_call_operand = false;
  };

  if (insts.size() < 3) {
    return std::nullopt;
  }

  SlotState lhs_state{std::string(lhs_slot)};
  SlotState rhs_state{std::string(rhs_slot)};
  auto match_slot = [&](std::string_view ptr) -> SlotState* {
    if (ptr == lhs_state.slot_name) return &lhs_state;
    if (ptr == rhs_state.slot_name) return &rhs_state;
    return nullptr;
  };

  for (std::size_t inst_index = 0; inst_index < 2; ++inst_index) {
    const auto* store = std::get_if<LirStoreOp>(&insts[inst_index]);
    if (store == nullptr || store->type_str != "i32") {
      return std::nullopt;
    }

    auto* slot = match_slot(store->ptr);
    if (slot == nullptr || slot->initialized) {
      return std::nullopt;
    }

    const auto stored_imm = parse_backend_i64_literal(store->val);
    if (!stored_imm.has_value()) {
      return std::nullopt;
    }

    slot->stored_imm = *stored_imm;
    slot->initialized = true;
  }

  for (std::size_t inst_index = 2; inst_index < insts.size(); ++inst_index) {
    const auto& inst = insts[inst_index];

    if (const auto* load = std::get_if<LirLoadOp>(&inst)) {
      if (load->type_str != "i32") {
        return std::nullopt;
      }
      auto* slot = match_slot(load->ptr);
      if (slot == nullptr || !slot->initialized) {
        return std::nullopt;
      }
      slot->last_load_result = load->result;
      if ((slot == &lhs_state && load->result == lhs_call_operand) ||
          (slot == &rhs_state && load->result == rhs_call_operand)) {
        slot->matched_call_operand = true;
      }
      continue;
    }

    if (const auto* rewrite = std::get_if<LirBinOp>(&inst)) {
      const auto rewrite_opcode = rewrite->opcode.typed();
      if (rewrite_opcode != LirBinaryOpcode::Add || rewrite->type_str != "i32" ||
          rewrite->rhs != "0") {
        return std::nullopt;
      }

      SlotState* matched_slot = nullptr;
      for (auto* slot : {&lhs_state, &rhs_state}) {
        if (slot->last_load_result == rewrite->lhs) {
          matched_slot = slot;
          break;
        }
      }
      if (matched_slot == nullptr) {
        return std::nullopt;
      }
      matched_slot->rewritten_result = rewrite->result;
      continue;
    }

    if (const auto* rewrite_store = std::get_if<LirStoreOp>(&inst)) {
      if (rewrite_store->type_str != "i32") {
        return std::nullopt;
      }
      auto* slot = match_slot(rewrite_store->ptr);
      if (slot == nullptr || rewrite_store->val != slot->rewritten_result) {
        return std::nullopt;
      }
      continue;
    }

    return std::nullopt;
  }

  if (!lhs_state.matched_call_operand || !rhs_state.matched_call_operand) {
    return std::nullopt;
  }

  return std::pair<std::int64_t, std::int64_t>{lhs_state.stored_imm, rhs_state.stored_imm};
}

inline std::optional<ParsedBackendMinimalTwoArgDirectCallLirModuleView>
parse_backend_minimal_two_arg_direct_call_lir_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const LirFunction* main_fn = nullptr;
  const LirFunction* helper = nullptr;
  for (const auto& function : module.functions) {
    if (function.name == "main") {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
      continue;
    }
    if (helper != nullptr) {
      return std::nullopt;
    }
    helper = &function;
  }

  if (helper == nullptr || main_fn == nullptr || helper->is_declaration || main_fn->is_declaration ||
      !backend_lir_is_zero_arg_i32_main_definition(main_fn->signature_text) ||
      helper->entry.value != 0 || main_fn->entry.value != 0 || helper->blocks.size() != 1 ||
      main_fn->blocks.size() != 1 || !helper->alloca_insts.empty() ||
      !helper->stack_objects.empty() || !main_fn->stack_objects.empty()) {
    return std::nullopt;
  }

  if (!parse_backend_two_param_add_function(*helper, std::nullopt).has_value()) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
  if (main_block.label != "entry" || main_ret == nullptr || !main_ret->value_str.has_value() ||
      main_ret->type_str != "i32" || main_block.insts.empty()) {
    return std::nullopt;
  }

  const auto* call = std::get_if<LirCallOp>(&main_block.insts.back());
  const auto call_operands =
      call == nullptr
          ? std::nullopt
          : parse_backend_direct_global_two_typed_call_operands(
                *call, helper->name, "i32", "i32");
  if (call == nullptr || call->result.empty() || call->result != *main_ret->value_str ||
      !call_operands.has_value()) {
    return std::nullopt;
  }

  if (main_fn->alloca_insts.empty()) {
    if (main_block.insts.size() != 1) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_backend_i64_literal(call_operands->first);
    const auto rhs_call_arg_imm = parse_backend_i64_literal(call_operands->second);
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }

    return ParsedBackendMinimalTwoArgDirectCallLirModuleView{
        helper,
        main_fn,
        call,
        *lhs_call_arg_imm,
        *rhs_call_arg_imm,
    };
  }

  const std::vector<LirInst> slot_prefix(main_block.insts.begin(), main_block.insts.end() - 1);
  if (main_fn->alloca_insts.size() == 1) {
    const auto* alloca = std::get_if<LirAllocaOp>(&main_fn->alloca_insts.front());
    if (alloca == nullptr || alloca->type_str != "i32" || !alloca->count.empty()) {
      return std::nullopt;
    }

    const auto matched_local =
        parse_backend_single_local_i32_slot_call_operand_imm(
            slot_prefix, alloca->result, call_operands->first, call_operands->second);
    if (!matched_local.has_value()) {
      return std::nullopt;
    }

    if (matched_local->second) {
      const auto rhs_call_arg_imm = parse_backend_i64_literal(call_operands->second);
      if (!rhs_call_arg_imm.has_value()) {
        return std::nullopt;
      }
      return ParsedBackendMinimalTwoArgDirectCallLirModuleView{
          helper,
          main_fn,
          call,
          matched_local->first,
          *rhs_call_arg_imm,
      };
    }

    const auto lhs_call_arg_imm = parse_backend_i64_literal(call_operands->first);
    if (!lhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }
    return ParsedBackendMinimalTwoArgDirectCallLirModuleView{
        helper,
        main_fn,
        call,
        *lhs_call_arg_imm,
        matched_local->first,
    };
  }

  if (main_fn->alloca_insts.size() != 2) {
    return std::nullopt;
  }

  const auto* lhs_alloca = std::get_if<LirAllocaOp>(&main_fn->alloca_insts[0]);
  const auto* rhs_alloca = std::get_if<LirAllocaOp>(&main_fn->alloca_insts[1]);
  if (lhs_alloca == nullptr || rhs_alloca == nullptr || lhs_alloca->type_str != "i32" ||
      rhs_alloca->type_str != "i32" || !lhs_alloca->count.empty() || !rhs_alloca->count.empty()) {
    return std::nullopt;
  }

  const auto matched_slots = parse_backend_two_local_i32_rewrite_call_operands_imm(
      slot_prefix, lhs_alloca->result, rhs_alloca->result, call_operands->first,
      call_operands->second);
  if (!matched_slots.has_value()) {
    return std::nullopt;
  }

  return ParsedBackendMinimalTwoArgDirectCallLirModuleView{
      helper,
      main_fn,
      call,
      matched_slots->first,
      matched_slots->second,
  };
}

inline std::optional<std::string_view> parse_backend_single_helper_call_crossing_source_value(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  const auto parsed = parse_backend_single_helper_zero_arg_main_lir_module(module, 3);
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& insts = parsed->main_block->insts;
  const auto* source_add = std::get_if<LirBinOp>(&insts[0]);
  const auto* final_add = std::get_if<LirBinOp>(&insts[2]);
  const auto source_add_opcode = source_add == nullptr ? std::nullopt : source_add->opcode.typed();
  const auto final_add_opcode = final_add == nullptr ? std::nullopt : final_add->opcode.typed();
  const auto parsed_call = parse_backend_single_helper_direct_global_call(*parsed, 1);
  if (source_add == nullptr || final_add == nullptr || !parsed_call.has_value() ||
      source_add_opcode != LirBinaryOpcode::Add || source_add->type_str != "i32" ||
      parsed_call->operand != source_add->result ||
      final_add_opcode != LirBinaryOpcode::Add || final_add->type_str != "i32" ||
      final_add->lhs != source_add->result || final_add->rhs != parsed_call->call->result ||
      *parsed->main_ret->value_str != final_add->result) {
    return std::nullopt;
  }

  return std::string_view(source_add->result);
}

inline std::optional<ParsedBackendSingleParamSlotAddFunctionView>
parse_backend_single_param_slot_add_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt) {
  using namespace c4c::codegen::lir;

  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.name);
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function_name,
                                     {"i32"}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || function.alloca_insts.size() != 2 ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto helper_params = parse_backend_function_signature_params(function.signature_text);
  if (!helper_params.has_value() || helper_params->size() != 1 || helper_params->front().is_varargs ||
      c4c::codegen::lir::trim_lir_arg_text(helper_params->front().type) != "i32" ||
      helper_params->front().operand.empty()) {
    return std::nullopt;
  }

  const auto* alloca = std::get_if<LirAllocaOp>(&function.alloca_insts[0]);
  const auto* arg_store = std::get_if<LirStoreOp>(&function.alloca_insts[1]);
  if (alloca == nullptr || arg_store == nullptr || alloca->result.empty() ||
      alloca->type_str != "i32" || !alloca->count.empty() || arg_store->type_str != "i32" ||
      arg_store->val != helper_params->front().operand || arg_store->ptr != alloca->result) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (block.label != "entry" || block.insts.size() != 4 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* load_before = std::get_if<LirLoadOp>(&block.insts[0]);
  const auto* add = std::get_if<LirBinOp>(&block.insts[1]);
  const auto* store_rewrite = std::get_if<LirStoreOp>(&block.insts[2]);
  const auto* reload = std::get_if<LirLoadOp>(&block.insts[3]);
  const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
  if (load_before == nullptr || add == nullptr || store_rewrite == nullptr || reload == nullptr ||
      load_before->type_str != "i32" || load_before->ptr != alloca->result ||
      add_opcode != LirBinaryOpcode::Add || add->type_str != "i32" ||
      add->lhs != load_before->result || store_rewrite->type_str != "i32" ||
      store_rewrite->val != add->result || store_rewrite->ptr != alloca->result ||
      reload->type_str != "i32" || reload->ptr != alloca->result ||
      *ret->value_str != reload->result) {
    return std::nullopt;
  }

  return ParsedBackendSingleParamSlotAddFunctionView{
      helper_params->front().operand,
      alloca->result,
      add,
  };
}

inline std::optional<ParsedBackendSingleAddImmFunctionView>
parse_backend_single_add_imm_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt) {
  if (const auto parsed =
          parse_backend_single_param_slot_add_function(function, expected_name);
      parsed.has_value()) {
    return ParsedBackendSingleAddImmFunctionView{
        std::move(parsed->param_name),
        parsed->add,
    };
  }

  using namespace c4c::codegen::lir;

  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.name);
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function_name,
                                     {"i32"}) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto helper_params = parse_backend_function_signature_params(function.signature_text);
  if (!helper_params.has_value() || helper_params->size() != 1 || helper_params->front().is_varargs ||
      c4c::codegen::lir::trim_lir_arg_text(helper_params->front().type) != "i32" ||
      helper_params->front().operand.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (block.label != "entry" || block.insts.size() != 1 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<LirBinOp>(&block.insts.front());
  const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
  if (add == nullptr || add_opcode != LirBinaryOpcode::Add || add->type_str != "i32" ||
      add->lhs != helper_params->front().operand || *ret->value_str != add->result) {
    return std::nullopt;
  }

  return ParsedBackendSingleAddImmFunctionView{
      helper_params->front().operand,
      add,
  };
}

inline std::optional<ParsedBackendTwoParamAddFunctionView>
parse_backend_two_param_add_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt) {
  using namespace c4c::codegen::lir;

  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.name);
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function_name,
                                     {"i32", "i32"}) ||
      function.entry.value != 0 || function.blocks.size() != 1 ||
      !function.alloca_insts.empty() || !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto helper_params = parse_backend_function_signature_params(function.signature_text);
  if (!helper_params.has_value() || helper_params->size() != 2 || (*helper_params)[0].is_varargs ||
      (*helper_params)[1].is_varargs ||
      c4c::codegen::lir::trim_lir_arg_text((*helper_params)[0].type) != "i32" ||
      c4c::codegen::lir::trim_lir_arg_text((*helper_params)[1].type) != "i32" ||
      (*helper_params)[0].operand.empty() || (*helper_params)[1].operand.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<LirRet>(&block.terminator);
  if (block.label != "entry" || block.insts.size() != 1 || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  const auto* add = std::get_if<LirBinOp>(&block.insts.front());
  const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
  if (add == nullptr || add_opcode != LirBinaryOpcode::Add || add->type_str != "i32" ||
      add->lhs != (*helper_params)[0].operand || add->rhs != (*helper_params)[1].operand ||
      *ret->value_str != add->result) {
    return std::nullopt;
  }

  return ParsedBackendTwoParamAddFunctionView{
      std::move((*helper_params)[0].operand),
      std::move((*helper_params)[1].operand),
      add,
  };
}
}  // namespace c4c::backend
