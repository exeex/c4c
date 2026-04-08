#pragma once

#include "../bir.hpp"

#include "../../codegen/lir/call_args_ops.hpp"
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

struct ParsedBackendTypedCallView {
  std::vector<std::string> owned_param_types;
  std::vector<std::string_view> param_types;
  std::vector<c4c::codegen::lir::LirTypedCallArgView> args;
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

struct ParsedBirMinimalDirectCallModuleView {
  const bir::Function* helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* call = nullptr;
  std::int64_t return_imm = 0;
};

struct ParsedBirMinimalVoidDirectCallImmReturnModuleView {
  const bir::Function* helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* call = nullptr;
  std::int64_t return_imm = 0;
};

struct ParsedBirMinimalDeclaredDirectCallModuleView {
  const bir::Function* callee = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* call = nullptr;
  std::vector<ParsedBackendExternCallArg> args;
  bool return_call_result = false;
  std::int64_t return_imm = 0;
};

struct ParsedBirMinimalTwoArgDirectCallModuleView {
  const bir::Function* helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* call = nullptr;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct ParsedBirMinimalDirectCallAddImmModuleView {
  const bir::Function* helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* call = nullptr;
  std::int64_t call_arg_imm = 0;
  std::int64_t add_imm = 0;
};

struct ParsedBirMinimalDirectCallIdentityArgModuleView {
  const bir::Function* helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* call = nullptr;
  std::int64_t call_arg_imm = 0;
};

struct ParsedBirMinimalDualIdentityDirectCallSubModuleView {
  const bir::Function* lhs_helper = nullptr;
  const bir::Function* rhs_helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::Block* main_block = nullptr;
  const bir::CallInst* lhs_call = nullptr;
  const bir::CallInst* rhs_call = nullptr;
  const bir::BinaryInst* sub = nullptr;
  std::int64_t lhs_call_arg_imm = 0;
  std::int64_t rhs_call_arg_imm = 0;
};

struct ParsedBirMinimalCallCrossingDirectCallModuleView {
  const bir::Function* helper = nullptr;
  const bir::Function* main_function = nullptr;
  const bir::BinaryInst* source_add = nullptr;
  const bir::CallInst* call = nullptr;
  const bir::BinaryInst* final_add = nullptr;
  std::string_view regalloc_source_value;
  std::int64_t source_imm = 0;
  std::int64_t helper_add_imm = 0;
};

inline std::optional<ParsedBackendTwoParamAddFunctionView>
parse_backend_two_param_add_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name);

inline std::optional<ParsedBackendSingleAddImmFunctionView>
parse_backend_single_add_imm_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name = std::nullopt);

inline std::optional<std::string_view> parse_backend_single_identity_function(
    const c4c::codegen::lir::LirFunction& function,
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
bool backend_lir_is_i32_definition(std::string_view signature_text);
bool backend_lir_is_zero_arg_i32_definition(std::string_view signature_text);

std::optional<c4c::codegen::lir::ParsedLirDirectGlobalTypedCallView>
parse_backend_direct_global_typed_call(const c4c::codegen::lir::LirCallOp& call);
std::optional<ParsedBackendExternCallArg> parse_backend_extern_call_arg(
    std::string_view type,
    std::string_view operand);
std::optional<std::vector<ParsedBackendExternCallArg>> parse_backend_extern_call_args(
    const ParsedBackendTypedCallView& parsed);
std::optional<std::vector<c4c::codegen::lir::OwnedLirTypedCallArg>>
parse_backend_owned_typed_call_args(
    std::string_view args_str);
std::optional<std::vector<c4c::codegen::lir::OwnedLirTypedCallArg>>
parse_backend_owned_typed_call_args(
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
parse_backend_single_helper_zero_arg_caller_lir_module(
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
    if (!function.is_declaration &&
        backend_lir_signature_matches(
            function.signature_text, "define", "i32", function.name, {})) {
      if (main_fn != nullptr) {
        return std::nullopt;
      }
      main_fn = &function;
    } else {
      if (helper != nullptr) {
        return std::nullopt;
      }
      helper = &function;
    }
  }

  if (helper == nullptr || main_fn == nullptr || helper->is_declaration ||
      main_fn->is_declaration ||
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

inline std::optional<std::int64_t> parse_backend_lir_zero_arg_return_imm_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name) {
  const std::string_view function_name =
      expected_name.has_value() ? *expected_name : std::string_view(function.name);
  if (function.is_declaration ||
      !backend_lir_signature_matches(function.signature_text, "define", "i32", function_name, {}) ||
      function.entry.value != 0 || function.blocks.size() != 1 || !function.alloca_insts.empty() ||
      !function.stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  const auto* ret = std::get_if<c4c::codegen::lir::LirRet>(&block.terminator);
  if (block.label != "entry" || !block.insts.empty() || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32") {
    return std::nullopt;
  }

  return parse_backend_i64_literal(*ret->value_str);
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

inline std::optional<ParsedBirMinimalDirectCallModuleView>
parse_bir_minimal_direct_call_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const bir::Function& caller,
         const bir::Function& helper) -> std::optional<ParsedBirMinimalDirectCallModuleView> {
    if (caller.is_declaration || helper.is_declaration ||
        caller.return_type != bir::TypeKind::I32 ||
        helper.return_type != bir::TypeKind::I32 || !caller.params.empty() ||
        !helper.params.empty() || !caller.local_slots.empty() ||
        !helper.local_slots.empty() || caller.blocks.size() != 1 ||
        helper.blocks.size() != 1 || caller.name.empty() || helper.name.empty() ||
        caller.name == helper.name) {
      return std::nullopt;
    }

    const auto helper_return_imm = bir::parse_i32_return_immediate(helper);
    if (!helper_return_imm.has_value()) {
      return std::nullopt;
    }

    const auto& main_block = caller.blocks.front();
    if (main_block.label != "entry" || main_block.insts.size() != 1 ||
        caller.return_type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<bir::CallInst>(&main_block.insts.front());
    if (call == nullptr || call->callee != helper.name) {
      return std::nullopt;
    }
    if (call->result.has_value()) {
      if (!main_block.terminator.value.has_value() ||
          main_block.terminator.value->kind != bir::Value::Kind::Named ||
          main_block.terminator.value->name != call->result->name) {
        return std::nullopt;
      }
    } else if (main_block.terminator.value.has_value()) {
      return std::nullopt;
    }

    return ParsedBirMinimalDirectCallModuleView{
        &helper,
        &caller,
        &main_block,
        call,
        *helper_return_imm,
    };
  };

  if (const auto parsed = try_match(module.functions[0], module.functions[1]); parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions[1], module.functions[0]);
}

inline std::optional<ParsedBirMinimalDeclaredDirectCallModuleView>
parse_bir_minimal_declared_direct_call_module(const bir::Module& module) {
  if (module.functions.size() != 2) {
    return std::nullopt;
  }

  const bir::Function* main_fn = nullptr;
  const bir::Function* callee = nullptr;
  for (const auto& function : module.functions) {
    if (function.is_declaration) {
      if (callee != nullptr || function.return_type != bir::TypeKind::I32 ||
          function.params.size() > 8 || !function.local_slots.empty() || !function.blocks.empty()) {
        return std::nullopt;
      }
      for (const auto& param : function.params) {
        if (param.name.empty() ||
            (param.type != bir::TypeKind::I32 && param.type != bir::TypeKind::I64)) {
          return std::nullopt;
        }
      }
      callee = &function;
      continue;
    }

    if (main_fn != nullptr || function.return_type != bir::TypeKind::I32 ||
        !function.params.empty() || !function.local_slots.empty() || function.blocks.size() != 1) {
      return std::nullopt;
    }
    main_fn = &function;
  }

  if (callee == nullptr || main_fn == nullptr || callee->name.empty() || main_fn->name.empty() ||
      callee->name == main_fn->name) {
    return std::nullopt;
  }

  const auto& main_block = main_fn->blocks.front();
  if (main_block.label != "entry" || main_block.insts.size() != 1 ||
      !main_block.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto* call = std::get_if<bir::CallInst>(&main_block.insts.front());
  if (call == nullptr || call->callee != callee->name || !call->result.has_value() ||
      call->result->kind != bir::Value::Kind::Named ||
      call->result->type != bir::TypeKind::I32) {
    return std::nullopt;
  }

  if (call->return_type_name != "i32" || call->args.size() != callee->params.size()) {
    return std::nullopt;
  }

  std::vector<ParsedBackendExternCallArg> parsed_args;
  parsed_args.reserve(call->args.size());
  for (std::size_t index = 0; index < call->args.size(); ++index) {
    const auto& param = callee->params[index];
    const auto& arg = call->args[index];
    if (arg.type != param.type) {
      return std::nullopt;
    }

    if (arg.kind == bir::Value::Kind::Immediate) {
      if (arg.type == bir::TypeKind::I32) {
        parsed_args.push_back(ParsedBackendExternCallArg{
            .kind = ParsedBackendExternCallArg::Kind::I32Imm,
            .imm = arg.immediate,
        });
        continue;
      }
      if (arg.type == bir::TypeKind::I64) {
        parsed_args.push_back(ParsedBackendExternCallArg{
            .kind = ParsedBackendExternCallArg::Kind::I64Imm,
            .imm = arg.immediate,
        });
        continue;
      }
      return std::nullopt;
    }

    if (arg.kind != bir::Value::Kind::Named || arg.name.empty() ||
        arg.type != bir::TypeKind::I64) {
      return std::nullopt;
    }

    bool known_symbol = false;
    for (const auto& global : module.globals) {
      if (global.name == arg.name) {
        known_symbol = true;
        break;
      }
    }
    if (!known_symbol) {
      for (const auto& string_constant : module.string_constants) {
        if (string_constant.name == arg.name) {
          known_symbol = true;
          break;
        }
      }
    }
    if (!known_symbol) {
      for (const auto& function : module.functions) {
        if (function.name == arg.name) {
          known_symbol = true;
          break;
        }
      }
    }
    if (!known_symbol) {
      return std::nullopt;
    }

    parsed_args.push_back(ParsedBackendExternCallArg{
        .kind = ParsedBackendExternCallArg::Kind::Ptr,
        .operand = "@" + arg.name,
    });
  }

  const bool return_call_result =
      main_block.terminator.value->kind == bir::Value::Kind::Named &&
      main_block.terminator.value->type == bir::TypeKind::I32 &&
      main_block.terminator.value->name == call->result->name;
  std::int64_t return_imm = 0;
  if (!return_call_result) {
    if (main_block.terminator.value->kind != bir::Value::Kind::Immediate ||
        main_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }
    return_imm = main_block.terminator.value->immediate;
  }

  return ParsedBirMinimalDeclaredDirectCallModuleView{
      callee,
      main_fn,
      &main_block,
      call,
      std::move(parsed_args),
      return_call_result,
      return_imm,
  };
}

inline std::optional<ParsedBirMinimalTwoArgDirectCallModuleView>
parse_bir_minimal_two_arg_direct_call_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const bir::Function& caller,
         const bir::Function& helper) -> std::optional<ParsedBirMinimalTwoArgDirectCallModuleView> {
    if (caller.is_declaration || helper.is_declaration ||
        caller.return_type != bir::TypeKind::I32 || helper.return_type != bir::TypeKind::I32 ||
        !caller.params.empty() || !caller.local_slots.empty() || !helper.local_slots.empty() ||
        caller.blocks.size() != 1 || helper.blocks.size() != 1 || caller.name.empty() ||
        helper.name.empty() || caller.name == helper.name || helper.params.size() != 2 ||
        helper.params[0].type != bir::TypeKind::I32 || helper.params[1].type != bir::TypeKind::I32 ||
        helper.params[0].name.empty() || helper.params[1].name.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    if (helper_block.label != "entry" || helper_block.insts.size() != 1 ||
        !helper_block.terminator.value.has_value() ||
        helper_block.terminator.value->kind != bir::Value::Kind::Named ||
        helper_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* add = std::get_if<bir::BinaryInst>(&helper_block.insts.front());
    if (add == nullptr || add->opcode != bir::BinaryOpcode::Add ||
        add->result.kind != bir::Value::Kind::Named || add->result.type != bir::TypeKind::I32 ||
        add->lhs.kind != bir::Value::Kind::Named || add->rhs.kind != bir::Value::Kind::Named ||
        add->lhs.type != bir::TypeKind::I32 || add->rhs.type != bir::TypeKind::I32 ||
        helper_block.terminator.value->name != add->result.name) {
      return std::nullopt;
    }

    const bool lhs_rhs_match =
        add->lhs.name == helper.params[0].name && add->rhs.name == helper.params[1].name;
    const bool rhs_lhs_match =
        add->lhs.name == helper.params[1].name && add->rhs.name == helper.params[0].name;
    if (!lhs_rhs_match && !rhs_lhs_match) {
      return std::nullopt;
    }

    const auto& main_block = caller.blocks.front();
    if (main_block.label != "entry" || main_block.insts.size() != 1 ||
        !main_block.terminator.value.has_value() ||
        main_block.terminator.value->kind != bir::Value::Kind::Named ||
        main_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<bir::CallInst>(&main_block.insts.front());
    if (call == nullptr || call->callee != helper.name || !call->result.has_value() ||
        call->result->kind != bir::Value::Kind::Named || call->result->type != bir::TypeKind::I32 ||
        main_block.terminator.value->name != call->result->name || call->return_type_name != "i32" ||
        call->args.size() != 2 || call->args[0].kind != bir::Value::Kind::Immediate ||
        call->args[1].kind != bir::Value::Kind::Immediate ||
        call->args[0].type != bir::TypeKind::I32 || call->args[1].type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    return ParsedBirMinimalTwoArgDirectCallModuleView{
        &helper,
        &caller,
        &main_block,
        call,
        call->args[0].immediate,
        call->args[1].immediate,
    };
  };

  if (const auto parsed = try_match(module.functions[0], module.functions[1]); parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions[1], module.functions[0]);
}

inline std::optional<ParsedBirMinimalDirectCallAddImmModuleView>
parse_bir_minimal_direct_call_add_imm_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const bir::Function& caller,
         const bir::Function& helper) -> std::optional<ParsedBirMinimalDirectCallAddImmModuleView> {
    if (caller.is_declaration || helper.is_declaration ||
        caller.return_type != bir::TypeKind::I32 || helper.return_type != bir::TypeKind::I32 ||
        !caller.params.empty() || !caller.local_slots.empty() || !helper.local_slots.empty() ||
        caller.blocks.size() != 1 || helper.blocks.size() != 1 || caller.name.empty() ||
        helper.name.empty() || caller.name == helper.name || helper.params.size() != 1 ||
        helper.params.front().type != bir::TypeKind::I32 || helper.params.front().name.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    if (helper_block.label != "entry" || helper_block.insts.size() != 1 ||
        !helper_block.terminator.value.has_value() ||
        helper_block.terminator.value->kind != bir::Value::Kind::Named ||
        helper_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* add = std::get_if<bir::BinaryInst>(&helper_block.insts.front());
    if (add == nullptr || add->opcode != bir::BinaryOpcode::Add ||
        add->result.kind != bir::Value::Kind::Named || add->result.type != bir::TypeKind::I32 ||
        helper_block.terminator.value->name != add->result.name) {
      return std::nullopt;
    }

    std::optional<std::int64_t> add_imm;
    const auto& param_name = helper.params.front().name;
    const bool lhs_param_rhs_imm =
        add->lhs.kind == bir::Value::Kind::Named && add->lhs.type == bir::TypeKind::I32 &&
        add->lhs.name == param_name && add->rhs.kind == bir::Value::Kind::Immediate &&
        add->rhs.type == bir::TypeKind::I32;
    const bool lhs_imm_rhs_param =
        add->lhs.kind == bir::Value::Kind::Immediate && add->lhs.type == bir::TypeKind::I32 &&
        add->rhs.kind == bir::Value::Kind::Named && add->rhs.type == bir::TypeKind::I32 &&
        add->rhs.name == param_name;
    if (lhs_param_rhs_imm) {
      add_imm = add->rhs.immediate;
    } else if (lhs_imm_rhs_param) {
      add_imm = add->lhs.immediate;
    } else {
      return std::nullopt;
    }

    const auto& main_block = caller.blocks.front();
    if (main_block.label != "entry" || main_block.insts.size() != 1 ||
        !main_block.terminator.value.has_value() ||
        main_block.terminator.value->kind != bir::Value::Kind::Named ||
        main_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<bir::CallInst>(&main_block.insts.front());
    if (call == nullptr || call->callee != helper.name || !call->result.has_value() ||
        call->result->kind != bir::Value::Kind::Named || call->result->type != bir::TypeKind::I32 ||
        main_block.terminator.value->name != call->result->name || call->return_type_name != "i32" ||
        call->args.size() != 1 || call->args.front().kind != bir::Value::Kind::Immediate ||
        call->args.front().type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    return ParsedBirMinimalDirectCallAddImmModuleView{
        &helper,
        &caller,
        &main_block,
        call,
        call->args.front().immediate,
        *add_imm,
    };
  };

  if (const auto parsed = try_match(module.functions[0], module.functions[1]); parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions[1], module.functions[0]);
}

inline std::optional<ParsedBirMinimalDirectCallIdentityArgModuleView>
parse_bir_minimal_direct_call_identity_arg_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const bir::Function& caller,
         const bir::Function& helper)
      -> std::optional<ParsedBirMinimalDirectCallIdentityArgModuleView> {
    if (caller.is_declaration || helper.is_declaration ||
        caller.return_type != bir::TypeKind::I32 || helper.return_type != bir::TypeKind::I32 ||
        !caller.params.empty() || !caller.local_slots.empty() || !helper.local_slots.empty() ||
        caller.blocks.size() != 1 || helper.blocks.size() != 1 || caller.name.empty() ||
        helper.name.empty() || caller.name == helper.name || helper.params.size() != 1 ||
        helper.params.front().type != bir::TypeKind::I32 || helper.params.front().name.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    if (helper_block.label != "entry" || !helper_block.insts.empty() ||
        !helper_block.terminator.value.has_value() ||
        helper_block.terminator.value->kind != bir::Value::Kind::Named ||
        helper_block.terminator.value->type != bir::TypeKind::I32 ||
        helper_block.terminator.value->name != helper.params.front().name) {
      return std::nullopt;
    }

    const auto& main_block = caller.blocks.front();
    if (main_block.label != "entry" || main_block.insts.size() != 1 ||
        !main_block.terminator.value.has_value() ||
        main_block.terminator.value->kind != bir::Value::Kind::Named ||
        main_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<bir::CallInst>(&main_block.insts.front());
    if (call == nullptr || call->callee != helper.name || !call->result.has_value() ||
        call->result->kind != bir::Value::Kind::Named || call->result->type != bir::TypeKind::I32 ||
        main_block.terminator.value->name != call->result->name || call->return_type_name != "i32" ||
        call->args.size() != 1 || call->args.front().kind != bir::Value::Kind::Immediate ||
        call->args.front().type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    return ParsedBirMinimalDirectCallIdentityArgModuleView{
        &helper,
        &caller,
        &main_block,
        call,
        call->args.front().immediate,
    };
  };

  if (const auto parsed = try_match(module.functions[0], module.functions[1]); parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions[1], module.functions[0]);
}

inline std::optional<ParsedBirMinimalDualIdentityDirectCallSubModuleView>
parse_bir_minimal_dual_identity_direct_call_sub_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 3) {
    return std::nullopt;
  }

  const bir::Function* main_fn = nullptr;
  std::vector<const bir::Function*> helpers;
  helpers.reserve(2);
  for (const auto& function : module.functions) {
    if (!function.is_declaration && function.return_type == bir::TypeKind::I32 &&
        function.params.empty() && function.local_slots.empty() && function.blocks.size() == 1) {
      const auto& block = function.blocks.front();
      if (block.label == "entry" && block.insts.size() == 3 &&
          block.terminator.value.has_value() &&
          block.terminator.value->kind == bir::Value::Kind::Named &&
          block.terminator.value->type == bir::TypeKind::I32) {
        if (main_fn != nullptr) {
          return std::nullopt;
        }
        main_fn = &function;
        continue;
      }
    }
    helpers.push_back(&function);
  }

  if (main_fn == nullptr || helpers.size() != 2) {
    return std::nullopt;
  }

  for (const auto* helper : helpers) {
    if (helper == nullptr || helper->is_declaration || helper->return_type != bir::TypeKind::I32 ||
        helper->params.size() != 1 || helper->params.front().type != bir::TypeKind::I32 ||
        helper->params.front().name.empty() || !helper->local_slots.empty() ||
        helper->blocks.size() != 1) {
      return std::nullopt;
    }
    const auto& block = helper->blocks.front();
    if (block.label != "entry" || !block.insts.empty() || !block.terminator.value.has_value() ||
        block.terminator.value->kind != bir::Value::Kind::Named ||
        block.terminator.value->type != bir::TypeKind::I32 ||
        block.terminator.value->name != helper->params.front().name) {
      return std::nullopt;
    }
  }

  const auto& main_block = main_fn->blocks.front();
  const auto* lhs_call = std::get_if<bir::CallInst>(&main_block.insts[0]);
  const auto* rhs_call = std::get_if<bir::CallInst>(&main_block.insts[1]);
  const auto* sub = std::get_if<bir::BinaryInst>(&main_block.insts[2]);
  if (lhs_call == nullptr || rhs_call == nullptr || sub == nullptr ||
      sub->opcode != bir::BinaryOpcode::Sub ||
      sub->result.kind != bir::Value::Kind::Named || sub->result.type != bir::TypeKind::I32 ||
      !lhs_call->result.has_value() || !rhs_call->result.has_value() ||
      lhs_call->result->kind != bir::Value::Kind::Named ||
      lhs_call->result->type != bir::TypeKind::I32 ||
      rhs_call->result->kind != bir::Value::Kind::Named ||
      rhs_call->result->type != bir::TypeKind::I32 ||
      sub->lhs.kind != bir::Value::Kind::Named || sub->lhs.type != bir::TypeKind::I32 ||
      sub->rhs.kind != bir::Value::Kind::Named || sub->rhs.type != bir::TypeKind::I32 ||
      sub->lhs.name != lhs_call->result->name || sub->rhs.name != rhs_call->result->name ||
      main_block.terminator.value->name != sub->result.name) {
    return std::nullopt;
  }

  const bir::Function* lhs_helper = nullptr;
  const bir::Function* rhs_helper = nullptr;
  for (const auto* helper : helpers) {
    if (helper->name == lhs_call->callee) {
      lhs_helper = helper;
    }
    if (helper->name == rhs_call->callee) {
      rhs_helper = helper;
    }
  }

  if (lhs_helper == nullptr || rhs_helper == nullptr || lhs_helper == rhs_helper ||
      lhs_call->return_type_name != "i32" || rhs_call->return_type_name != "i32" ||
      lhs_call->args.size() != 1 || rhs_call->args.size() != 1 ||
      lhs_call->args.front().kind != bir::Value::Kind::Immediate ||
      lhs_call->args.front().type != bir::TypeKind::I32 ||
      rhs_call->args.front().kind != bir::Value::Kind::Immediate ||
      rhs_call->args.front().type != bir::TypeKind::I32) {
    return std::nullopt;
  }

  return ParsedBirMinimalDualIdentityDirectCallSubModuleView{
      lhs_helper,
      rhs_helper,
      main_fn,
      &main_block,
      lhs_call,
      rhs_call,
      sub,
      lhs_call->args.front().immediate,
      rhs_call->args.front().immediate,
  };
}

inline std::optional<ParsedBirMinimalCallCrossingDirectCallModuleView>
parse_bir_minimal_call_crossing_direct_call_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const bir::Function& caller,
         const bir::Function& helper)
      -> std::optional<ParsedBirMinimalCallCrossingDirectCallModuleView> {
    if (caller.is_declaration || helper.is_declaration ||
        caller.return_type != bir::TypeKind::I32 || helper.return_type != bir::TypeKind::I32 ||
        !caller.params.empty() || !caller.local_slots.empty() || !helper.local_slots.empty() ||
        caller.blocks.size() != 1 || helper.blocks.size() != 1 || caller.name.empty() ||
        helper.name.empty() || caller.name == helper.name || helper.params.size() != 1 ||
        helper.params.front().type != bir::TypeKind::I32 || helper.params.front().name.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    if (helper_block.label != "entry" || helper_block.insts.size() != 1 ||
        !helper_block.terminator.value.has_value() ||
        helper_block.terminator.value->kind != bir::Value::Kind::Named ||
        helper_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* helper_add = std::get_if<bir::BinaryInst>(&helper_block.insts.front());
    if (helper_add == nullptr || helper_add->opcode != bir::BinaryOpcode::Add ||
        helper_add->result.kind != bir::Value::Kind::Named ||
        helper_add->result.type != bir::TypeKind::I32 ||
        helper_block.terminator.value->name != helper_add->result.name) {
      return std::nullopt;
    }

    std::optional<std::int64_t> helper_add_imm;
    const auto& helper_param = helper.params.front().name;
    const bool lhs_param_rhs_imm =
        helper_add->lhs.kind == bir::Value::Kind::Named &&
        helper_add->lhs.type == bir::TypeKind::I32 &&
        helper_add->lhs.name == helper_param &&
        helper_add->rhs.kind == bir::Value::Kind::Immediate &&
        helper_add->rhs.type == bir::TypeKind::I32;
    const bool lhs_imm_rhs_param =
        helper_add->lhs.kind == bir::Value::Kind::Immediate &&
        helper_add->lhs.type == bir::TypeKind::I32 &&
        helper_add->rhs.kind == bir::Value::Kind::Named &&
        helper_add->rhs.type == bir::TypeKind::I32 &&
        helper_add->rhs.name == helper_param;
    if (lhs_param_rhs_imm) {
      helper_add_imm = helper_add->rhs.immediate;
    } else if (lhs_imm_rhs_param) {
      helper_add_imm = helper_add->lhs.immediate;
    } else {
      return std::nullopt;
    }

    const auto& main_block = caller.blocks.front();
    if (main_block.label != "entry" || main_block.insts.size() != 3 ||
        !main_block.terminator.value.has_value() ||
        main_block.terminator.value->kind != bir::Value::Kind::Named ||
        main_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* source_add = std::get_if<bir::BinaryInst>(&main_block.insts[0]);
    const auto* call = std::get_if<bir::CallInst>(&main_block.insts[1]);
    const auto* final_add = std::get_if<bir::BinaryInst>(&main_block.insts[2]);
    if (source_add == nullptr || call == nullptr || final_add == nullptr ||
        source_add->opcode != bir::BinaryOpcode::Add ||
        source_add->result.kind != bir::Value::Kind::Named ||
        source_add->result.type != bir::TypeKind::I32 ||
        source_add->lhs.kind != bir::Value::Kind::Immediate ||
        source_add->lhs.type != bir::TypeKind::I32 ||
        source_add->rhs.kind != bir::Value::Kind::Immediate ||
        source_add->rhs.type != bir::TypeKind::I32 || call->callee != helper.name ||
        !call->result.has_value() || call->result->kind != bir::Value::Kind::Named ||
        call->result->type != bir::TypeKind::I32 || call->return_type_name != "i32" ||
        call->args.size() != 1 || call->args.front().kind != bir::Value::Kind::Named ||
        call->args.front().type != bir::TypeKind::I32 ||
        call->args.front().name != source_add->result.name ||
        final_add->opcode != bir::BinaryOpcode::Add ||
        final_add->result.kind != bir::Value::Kind::Named ||
        final_add->result.type != bir::TypeKind::I32 ||
        final_add->lhs.kind != bir::Value::Kind::Named ||
        final_add->lhs.type != bir::TypeKind::I32 ||
        final_add->rhs.kind != bir::Value::Kind::Named ||
        final_add->rhs.type != bir::TypeKind::I32 ||
        final_add->lhs.name != source_add->result.name ||
        final_add->rhs.name != call->result->name ||
        main_block.terminator.value->name != final_add->result.name) {
      return std::nullopt;
    }

    return ParsedBirMinimalCallCrossingDirectCallModuleView{
        &helper,
        &caller,
        source_add,
        call,
        final_add,
        source_add->result.name,
        source_add->lhs.immediate + source_add->rhs.immediate,
        *helper_add_imm,
    };
  };

  if (const auto parsed = try_match(module.functions[0], module.functions[1]); parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions[1], module.functions[0]);
}

inline std::optional<ParsedBirMinimalVoidDirectCallImmReturnModuleView>
parse_bir_minimal_void_direct_call_imm_return_module(const bir::Module& module) {
  if (!module.globals.empty() || !module.string_constants.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const bir::Function& caller,
         const bir::Function& helper)
      -> std::optional<ParsedBirMinimalVoidDirectCallImmReturnModuleView> {
    if (caller.is_declaration || helper.is_declaration ||
        caller.return_type != bir::TypeKind::I32 || helper.return_type != bir::TypeKind::Void ||
        !caller.params.empty() || !helper.params.empty() || !caller.local_slots.empty() ||
        !helper.local_slots.empty() || caller.blocks.size() != 1 || helper.blocks.size() != 1 ||
        caller.name.empty() || helper.name.empty() || caller.name == helper.name) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    if (helper_block.label != "entry" || !helper_block.insts.empty() ||
        helper_block.terminator.value.has_value()) {
      return std::nullopt;
    }

    const auto& main_block = caller.blocks.front();
    if (main_block.label != "entry" || main_block.insts.size() != 1 ||
        !main_block.terminator.value.has_value() ||
        main_block.terminator.value->kind != bir::Value::Kind::Immediate ||
        main_block.terminator.value->type != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<bir::CallInst>(&main_block.insts.front());
    if (call == nullptr || call->result.has_value() || call->callee != helper.name ||
        call->return_type_name != "void" || !call->args.empty()) {
      return std::nullopt;
    }

    return ParsedBirMinimalVoidDirectCallImmReturnModuleView{
        &helper,
        &caller,
        &main_block,
        call,
        main_block.terminator.value->immediate,
    };
  };

  if (const auto parsed = try_match(module.functions[0], module.functions[1]); parsed.has_value()) {
    return parsed;
  }
  return try_match(module.functions[1], module.functions[0]);
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
    std::optional<std::string_view> expected_name) {
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

inline std::optional<std::string_view> parse_backend_single_identity_function(
    const c4c::codegen::lir::LirFunction& function,
    std::optional<std::string_view> expected_name) {
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
  if (block.label != "entry" || !block.insts.empty() || ret == nullptr ||
      !ret->value_str.has_value() || ret->type_str != "i32" ||
      *ret->value_str != helper_params->front().operand) {
    return std::nullopt;
  }

  return helper_params->front().operand;
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
