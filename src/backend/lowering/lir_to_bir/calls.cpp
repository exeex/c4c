// Translated scaffold from:
// - ref/claudes-c-compiler/src/ir/lowering/expr_calls.rs
// - ref/claudes-c-compiler/src/backend/call_abi.rs
// - ref/claudes-c-compiler/src/backend/x86/codegen/calls.rs
//
// This file is the backend-owned home for call lowering into BIR.  It is a
// translation outline only: it captures the control-flow split, ABI metadata
// shape, and result-handling stages that need to move out of lir_to_bir.cpp.
// The file is now wired into the build as a scaffold, but not yet used as the
// owning correctness path.

#include "passes.hpp"
#include "../../../codegen/lir/call_args.hpp"
#include "../../../codegen/lir/ir.hpp"
#include "../call_decode.hpp"

#include <initializer_list>
#include <limits>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace c4c::backend::lir_to_bir {

struct CallAbiPolicy {
  std::size_t max_int_regs = 0;
  std::size_t max_float_regs = 0;
  bool align_i128_pairs = false;
  bool f128_in_fp_regs = false;
  bool f128_in_gp_pairs = false;
  bool variadic_floats_in_gp = false;
  bool large_struct_by_ref = false;
  bool use_sysv_struct_classification = false;
  bool use_riscv_float_struct_classification = false;
  bool allow_struct_split_reg_stack = false;
  bool align_struct_pairs = false;
  bool sret_uses_dedicated_reg = false;
};

struct StructReturnClassification {
  std::optional<std::size_t> sret_size;
  std::optional<std::size_t> two_reg_size;
};

struct LoweredCallAbi {
  bir::CallInst call;
  std::vector<bir::CallArgAbiInfo> arg_abi;
  std::optional<bir::CallResultAbiInfo> result_abi;
};

// Rust expr_calls.rs::classify_struct_return
// 32-bit targets keep all struct returns in memory; 64-bit targets split the
// ABI into sret (>16 bytes), two-register returns (9-16 bytes), or scalar.
[[maybe_unused]] static StructReturnClassification classify_struct_return(std::size_t size,
                                                                          std::size_t ptr_size) {
  StructReturnClassification classification;
  if (ptr_size <= 4) {
    classification.sret_size = size;
    return classification;
  }
  if (size > 16) {
    classification.sret_size = size;
  } else if (size > 8) {
    classification.two_reg_size = size;
  }
  return classification;
}

// Rust expr_calls.rs::lower_function_call
// This is the top-level lowering sketch for a call expression:
// 1. Normalize the callee expression:
//    - strip no-op deref layers around function pointers
//    - fold (&func)(...) into a direct call form
//    - resolve _Generic selections to the chosen expression
// 2. Resolve builtin/error/noreturn behavior before ABI work.
// 3. Lower arguments and attach ABI metadata for each operand.
// 4. Emit a BIR call node with direct or indirect callee shape.
// 5. Post-process sret, two-register struct returns, complex returns, and
//    integer narrowing of the final result.
//
// The concrete implementation should feed bir::CallInst plus
// bir::CallArgAbiInfo / bir::CallResultAbiInfo, not backend-specific assembly
// details.
[[maybe_unused]] static LoweredCallAbi lower_function_call_scaffold(
    const c4c::codegen::lir::LirCallOp& call) {
  LoweredCallAbi lowered{};
  lowered.call.return_type = bir::TypeKind::Void;
  lowered.call.callee = std::string(c4c::codegen::lir::trim_lir_arg_text(call.callee));
  lowered.call.is_indirect = !call.callee.empty() && call.callee.str().front() != '@';
  lowered.call.is_variadic = false;
  lowered.call.is_noreturn = false;

  // The real lowering pass will:
  // - decode the typed call via backend::call_decode helpers
  // - compute call_arg_abi metadata from target ABI policy
  // - set calling_convention / result_abi / callee_value as needed
  // - carry through the parsed argument list and their widened IR types
  return lowered;
}

// Rust expr_calls.rs::lower_call_arguments
// Scaffolding notes for the argument-lowering phase:
// - preserve per-argument typed operands
// - apply implicit casts for prototyped parameters
// - apply default argument promotions for variadic and unprototyped calls
// - carry struct size/alignment/classification so the target backend can
//   decide register-vs-stack placement without re-parsing the call text
// - carry complex-number decomposition hints for targets that split values
[[maybe_unused]] static std::vector<bir::CallArgAbiInfo> lower_call_arguments_scaffold(
    const c4c::codegen::lir::LirCallOp& call) {
  (void)call;
  return {};
}

// Rust expr_calls.rs::emit_call_instruction
// This stage decides whether the callee is:
// - a direct global symbol
// - a function-pointer variable
// - a general indirect expression
//
// In the BIR translation, this becomes a bir::CallInst with a populated
// callee symbol or callee_value, plus ABI metadata describing each arg.
[[maybe_unused]] static LoweredCallAbi emit_call_instruction_scaffold(
    const c4c::codegen::lir::LirCallOp& call,
    const CallAbiPolicy& abi_policy) {
  (void)abi_policy;
  return lower_function_call_scaffold(call);
}

// Rust expr_calls.rs::maybe_narrow_call_result
// After a widened backend operation returns, narrow the result back to the
// source integer width when the call return type is a smaller scalar integer.
[[maybe_unused]] static std::optional<bir::Value> maybe_narrow_call_result_scaffold(
    const bir::Value& value,
    bir::TypeKind return_type) {
  (void)value;
  (void)return_type;
  return std::nullopt;
}

// Rust expr_calls.rs::is_function_variadic
// This backend-owned query should remain a metadata lookup, not a codegen
// decision.  The actual implementation will consult parsed function signatures
// and the known builtins that are known to be variadic.
[[maybe_unused]] static bool is_function_variadic_scaffold(std::string_view name) {
  (void)name;
  return false;
}

// Rust expr_calls.rs::get_func_ptr_return_ir_type
// The real version walks through function-pointer expressions, strips no-op
// dereferences, and extracts the effective return type from the callee CType.
[[maybe_unused]] static bir::TypeKind get_func_ptr_return_ir_type_scaffold(
    const c4c::codegen::lir::LirCallOp& call) {
  (void)call;
  return bir::TypeKind::Void;
}

// Backend ABI metadata bridge:
// The eventual goal is to make this file the canonical translation point from
// LIR call syntax into BIR call metadata:
// - bir::CallInst for the call itself
// - bir::CallArgAbiInfo for each argument
// - bir::CallResultAbiInfo for return classification
// - bir::CallingConv / bir::AbiValueClass for the target ABI contract
//
// That lets the backend reason about calls without rereading LIR text or
// re-deriving the same ABI facts in multiple codegen passes.

}  // namespace c4c::backend::lir_to_bir

namespace c4c::backend {

namespace {

std::optional<bir::TypeKind> lower_declared_function_param_type(const c4c::TypeSpec& type) {
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

}  // namespace

std::optional<std::vector<bir::Param>> lower_call_params_from_type_texts(
    const std::vector<std::string_view>& param_types) {
  std::vector<bir::Param> params;
  params.reserve(param_types.size());
  for (std::size_t index = 0; index < param_types.size(); ++index) {
    const auto type = lir_to_bir::legalize_call_arg_type(param_types[index]);
    if (!type.has_value()) {
      return std::nullopt;
    }
    params.push_back(bir::Param{
        .type = *type,
        .name = "%arg" + std::to_string(index),
        .size_bytes = lir_to_bir::legalize_type_size_bytes(*type),
        .align_bytes = lir_to_bir::legalize_type_align_bytes(*type),
        .is_varargs = false,
        .is_sret = false,
        .is_byval = false,
    });
  }
  return params;
}

std::optional<std::vector<bir::Param>> lower_function_params(
    const c4c::codegen::lir::LirFunction& lir_function) {
  std::vector<bir::Param> params;
  if (!lir_function.params.empty()) {
    if (lir_function.params.size() > 2) {
      return std::nullopt;
    }
    params.reserve(lir_function.params.size());
    for (const auto& [param_name, param_type] : lir_function.params) {
      const auto lowered_type = lower_declared_function_param_type(param_type);
      if (!lowered_type.has_value() || param_name.empty()) {
        return std::nullopt;
      }
      params.push_back(bir::Param{*lowered_type, param_name});
    }
    return params;
  }

  const auto parsed_params = parse_backend_function_signature_params(lir_function.signature_text);
  if (!parsed_params.has_value() || parsed_params->size() > 2) {
    return std::nullopt;
  }

  params.reserve(parsed_params->size());
  for (const auto& param : *parsed_params) {
    const auto lowered_type = lir_to_bir::legalize_call_arg_type(param.type);
    if (param.is_varargs || !lowered_type.has_value() || param.operand.empty()) {
      return std::nullopt;
    }
    params.push_back(bir::Param{*lowered_type, param.operand});
  }
  return params;
}

bir::CallingConv default_calling_convention_for_target(std::string_view target_triple) {
  if (target_triple.find("windows") != std::string_view::npos ||
      target_triple.find("mingw") != std::string_view::npos) {
    return bir::CallingConv::Win64;
  }
  if (target_triple.find("x86_64") != std::string_view::npos ||
      target_triple.find("amd64") != std::string_view::npos) {
    return bir::CallingConv::SysV;
  }
  return bir::CallingConv::C;
}

bool function_signature_is_variadic(std::string_view signature_text) {
  const auto params = parse_backend_function_signature_params(signature_text);
  if (!params.has_value()) {
    return false;
  }
  for (const auto& param : *params) {
    if (param.is_varargs) {
      return true;
    }
  }
  return false;
}

bir::CallInst make_direct_call_inst(std::string callee,
                                    bir::CallingConv calling_convention,
                                    bool is_variadic,
                                    bir::TypeKind return_type,
                                    std::string return_type_name,
                                    std::optional<bir::Value> result,
                                    std::vector<bir::Value> args) {
  auto classify_scalar_arg_abi = [](bir::TypeKind type) {
    return bir::CallArgAbiInfo{
        .type = type,
        .size_bytes = lir_to_bir::legalize_type_size_bytes(type),
        .align_bytes = lir_to_bir::legalize_type_align_bytes(type),
        .primary_class = bir::AbiValueClass::Integer,
        .secondary_class = bir::AbiValueClass::None,
        .passed_in_register = true,
        .passed_on_stack = false,
        .byval_copy = false,
    };
  };
  auto classify_scalar_result_abi = [](bir::TypeKind type) {
    return bir::CallResultAbiInfo{
        .type = type,
        .primary_class = bir::AbiValueClass::Integer,
        .secondary_class = bir::AbiValueClass::None,
        .returned_in_memory = false,
    };
  };

  bir::CallInst call;
  call.result = std::move(result);
  call.callee = std::move(callee);
  call.return_type = return_type;
  call.return_type_name = std::move(return_type_name);
  call.arg_types.reserve(args.size());
  call.arg_abi.reserve(args.size());
  for (const auto& arg : args) {
    call.arg_types.push_back(arg.type);
    call.arg_abi.push_back(classify_scalar_arg_abi(arg.type));
  }
  call.args = std::move(args);
  call.result_abi = classify_scalar_result_abi(return_type);
  call.is_indirect = false;
  call.calling_convention = calling_convention;
  call.is_variadic = is_variadic;
  call.is_noreturn = false;
  return call;
}

std::optional<std::vector<bir::Value>> lower_direct_call_args(
    const std::vector<ParsedBackendExternCallArg>& args) {
  std::vector<bir::Value> lowered_args;
  lowered_args.reserve(args.size());
  for (const auto& arg : args) {
    switch (arg.kind) {
      case ParsedBackendExternCallArg::Kind::I32Imm:
        lowered_args.push_back(bir::Value::immediate_i32(static_cast<std::int32_t>(arg.imm)));
        break;
      case ParsedBackendExternCallArg::Kind::I64Imm:
        lowered_args.push_back(bir::Value::immediate_i64(arg.imm));
        break;
      case ParsedBackendExternCallArg::Kind::Ptr:
        if (arg.operand.empty() || arg.operand.front() != '@') {
          return std::nullopt;
        }
        lowered_args.push_back(bir::Value::named(bir::TypeKind::I64, arg.operand.substr(1)));
        break;
    }
  }
  return lowered_args;
}

namespace {

std::optional<bir::TypeKind> lower_minimal_scalar_type(const c4c::TypeSpec& type) {
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

std::optional<bir::TypeKind> lower_scalar_type_text(std::string_view text) {
  if (text == "i8") {
    return bir::TypeKind::I8;
  }
  if (text == "i32") {
    return bir::TypeKind::I32;
  }
  if (text == "i64") {
    return bir::TypeKind::I64;
  }
  return std::nullopt;
}

bool matches_minimal_i32_function_signature(
    const c4c::codegen::lir::LirFunction& function,
    std::initializer_list<std::string_view> signature_param_types) {
  if (!function.params.empty()) {
    const auto lowered_return_type = lower_minimal_scalar_type(function.return_type);
    if (lowered_return_type != bir::TypeKind::I32 ||
        function.params.size() != signature_param_types.size()) {
      return false;
    }

    for (const auto& [param_name, param_type] : function.params) {
      if (param_name.empty() || lower_minimal_scalar_type(param_type) != bir::TypeKind::I32) {
        return false;
      }
    }
    return true;
  }

  return backend_lir_signature_matches(
      function.signature_text, "define", "i32", function.name, signature_param_types);
}

std::optional<bir::TypeKind> lower_scalar_type(
    const c4c::codegen::lir::LirTypeRef& type) {
  if (type.kind() != c4c::codegen::lir::LirTypeKind::Integer) {
    return std::nullopt;
  }

  const auto integer_width = type.integer_bit_width();
  if (!integer_width.has_value()) {
    return std::nullopt;
  }

  switch (*integer_width) {
    case 8:
      return bir::TypeKind::I8;
    case 32:
      return bir::TypeKind::I32;
    case 64:
      return bir::TypeKind::I64;
    default:
      return std::nullopt;
  }
}

unsigned scalar_type_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I8:
      return 8;
    case bir::TypeKind::I32:
      return 32;
    case bir::TypeKind::I64:
      return 64;
    case bir::TypeKind::Void:
      return 0;
  }
  return 0;
}

bool lir_function_returns_integer_width(const c4c::codegen::lir::LirFunction& function,
                                        unsigned bit_width) {
  const auto lowered_type = lower_minimal_scalar_type(function.return_type);
  return lowered_type.has_value() && scalar_type_bit_width(*lowered_type) == bit_width;
}

bool lir_function_matches_minimal_no_param_integer_return(
    const c4c::codegen::lir::LirFunction& function,
    unsigned bit_width) {
  if (!function.params.empty()) {
    return false;
  }
  if (lir_function_returns_integer_width(function, bit_width)) {
    return true;
  }
  return backend_lir_signature_matches(
      function.signature_text, "define", "i32", function.name, {});
}

std::optional<bir::TypeKind> lower_function_return_type(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirRet& ret) {
  return lir_to_bir::legalize_function_return_type(function, ret);
}

std::string decode_call_llvm_byte_string(std::string_view text) {
  std::string bytes;
  bytes.reserve(text.size());
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text[index] == '\\' && index + 2 < text.size()) {
      auto hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      const int hi = hex_val(text[index + 1]);
      const int lo = hex_val(text[index + 2]);
      if (hi >= 0 && lo >= 0) {
        bytes.push_back(static_cast<char>(hi * 16 + lo));
        index += 2;
        continue;
      }
    }
    bytes.push_back(text[index]);
  }
  return bytes;
}

}  // namespace

std::optional<bir::Module> try_lower_minimal_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }
  if (module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const c4c::codegen::lir::LirFunction& main_function,
         const c4c::codegen::lir::LirFunction& helper)
      -> std::optional<std::tuple<std::string, std::string, std::string, std::int64_t>> {
    using namespace c4c::codegen::lir;

    if (main_function.is_declaration || helper.is_declaration ||
        !lir_function_matches_minimal_no_param_integer_return(main_function, 32) ||
        !backend_lir_function_matches_zero_arg_integer_return(helper, 32) ||
        main_function.entry.value != 0 || helper.entry.value != 0 ||
        main_function.blocks.size() != 1 || helper.blocks.size() != 1 ||
        !main_function.alloca_insts.empty() || !helper.alloca_insts.empty() ||
        !main_function.stack_objects.empty() || !helper.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto helper_return_imm =
        parse_backend_lir_zero_arg_return_imm_function(helper, std::nullopt);
    if (!helper_return_imm.has_value()) {
      return std::nullopt;
    }

    const auto& main_block = main_function.blocks.front();
    const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
    if (main_block.label != "entry" || main_block.insts.size() != 1 || main_ret == nullptr ||
        !main_ret->value_str.has_value() ||
        lower_function_return_type(main_function, *main_ret) != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&main_block.insts.front());
    const auto callee_name =
        call == nullptr ? std::nullopt : parse_backend_zero_arg_direct_global_typed_call(*call);
    if (call == nullptr || !callee_name.has_value() || *callee_name != helper.name ||
        call->result.str().empty() || *main_ret->value_str != call->result.str()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::string, std::string, std::int64_t>{
        helper.name,
        main_function.name,
        call->result.str(),
        *helper_return_imm,
    };
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [helper_name, main_name, call_result, return_imm] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_name;
  helper.return_type = bir::TypeKind::I32;

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(return_imm));
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = main_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(make_direct_call_inst(
      helper_name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, call_result),
      {}));
  main_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, call_result);
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_void_direct_call_imm_return_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (!module.globals.empty() || module.functions.size() != 2) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& caller,
         const LirFunction& helper)
      -> std::optional<std::tuple<std::string, std::string, std::int64_t>> {
    if (caller.is_declaration || helper.is_declaration ||
        !lir_function_matches_minimal_no_param_integer_return(caller, 32) ||
        !backend_lir_signature_matches(helper.signature_text, "define", "void", helper.name, {}) ||
        caller.entry.value != 0 || helper.entry.value != 0 ||
        caller.blocks.size() != 1 || helper.blocks.size() != 1 ||
        !caller.alloca_insts.empty() || !helper.alloca_insts.empty() ||
        !caller.stack_objects.empty() || !helper.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
    if (helper_block.label != "entry" || !helper_block.insts.empty() || helper_ret == nullptr ||
        helper_ret->value_str.has_value() || helper_ret->type_str != "void") {
      return std::nullopt;
    }

    const auto& caller_block = caller.blocks.front();
    const auto* caller_ret = std::get_if<LirRet>(&caller_block.terminator);
    if (caller_block.label != "entry" || caller_block.insts.size() != 1 || caller_ret == nullptr ||
        !caller_ret->value_str.has_value() ||
        lower_function_return_type(caller, *caller_ret) != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&caller_block.insts.front());
    const auto callee_name =
        call == nullptr ? std::nullopt : parse_backend_zero_arg_direct_global_typed_call(*call);
    const auto return_imm =
        caller_ret->value_str.has_value() ? parse_backend_i64_literal(*caller_ret->value_str)
                                          : std::nullopt;
    if (call == nullptr || !callee_name.has_value() || *callee_name != helper.name ||
        call->return_type != "void" || !call->result.empty() || !return_imm.has_value() ||
        *return_imm < std::numeric_limits<std::int32_t>::min() ||
        *return_imm > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::string, std::int64_t>{
        helper.name,
        caller.name,
        *return_imm,
    };
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  const auto& [helper_name, caller_name, return_imm] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_name;
  helper.return_type = bir::TypeKind::Void;

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = caller_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(make_direct_call_inst(
      helper_name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::Void,
      "void",
      std::nullopt,
      {}));
  main_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(return_imm));
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_declared_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.empty() || module.functions.size() > 2) {
    return std::nullopt;
  }

  const LirFunction* main_function = nullptr;
  const LirFunction* declared_callee = nullptr;
  for (const auto& function : module.functions) {
    if (!function.is_declaration &&
        backend_lir_signature_matches(function.signature_text, "define", "i32", function.name, {})) {
      if (main_function != nullptr) {
        return std::nullopt;
      }
      main_function = &function;
    } else {
      if (declared_callee != nullptr || !function.is_declaration) {
        return std::nullopt;
      }
      declared_callee = &function;
    }
  }

  if (main_function == nullptr || main_function->entry.value != 0 ||
      main_function->blocks.size() != 1 || !main_function->alloca_insts.empty() ||
      !main_function->stack_objects.empty()) {
    return std::nullopt;
  }

  const auto& main_lir_block = main_function->blocks.front();
  const auto* main_ret = std::get_if<LirRet>(&main_lir_block.terminator);
  if (main_lir_block.label != "entry" || main_lir_block.insts.size() != 1 || main_ret == nullptr ||
      !main_ret->value_str.has_value() ||
      lir_to_bir::legalize_function_return_type(*main_function, *main_ret) != bir::TypeKind::I32) {
    return std::nullopt;
  }

  const auto* call = std::get_if<LirCallOp>(&main_lir_block.insts.front());
  const auto symbol_name =
      call == nullptr ? std::nullopt : parse_lir_direct_global_callee(call->callee);
  const auto parsed_direct_global_call =
      call == nullptr ? std::nullopt : parse_backend_direct_global_typed_call(*call);
  const auto typed_call =
      call == nullptr ? std::nullopt : parse_lir_typed_call_or_infer_params(*call);
  if (call == nullptr || !symbol_name.has_value() ||
      lir_to_bir::legalize_call_arg_type(call->return_type) != bir::TypeKind::I32 ||
      call->result.str().empty() || symbol_name->empty()) {
    return std::nullopt;
  }

  ParsedBackendTypedCallView backend_typed_call;
  if (typed_call.has_value()) {
    backend_typed_call = make_backend_typed_call_view(*typed_call);
  } else if (parsed_direct_global_call.has_value()) {
    backend_typed_call.args = parsed_direct_global_call->typed_call.args;
    backend_typed_call.owned_param_types.reserve(parsed_direct_global_call->typed_call.param_types.size());
    backend_typed_call.param_types.reserve(parsed_direct_global_call->typed_call.param_types.size());
    for (const auto param_type : parsed_direct_global_call->typed_call.param_types) {
      backend_typed_call.owned_param_types.push_back(std::string(param_type));
      backend_typed_call.param_types.push_back(backend_typed_call.owned_param_types.back());
    }
  } else {
    return std::nullopt;
  }

  if (backend_typed_call.args.size() > 6) {
    return std::nullopt;
  }

  const LirExternDecl* extern_callee = nullptr;
  for (const auto& decl : module.extern_decls) {
    if (decl.name != *symbol_name) {
      continue;
    }
    if (extern_callee != nullptr) {
      return std::nullopt;
    }
    extern_callee = &decl;
  }

  if (extern_callee != nullptr) {
    if (lir_to_bir::legalize_extern_decl_return_type(*extern_callee) != bir::TypeKind::I32) {
      return std::nullopt;
    }
  } else {
    if (declared_callee == nullptr || declared_callee->name != *symbol_name ||
        lir_to_bir::legalize_function_decl_return_type(*declared_callee) != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto declared_call_surface =
        parsed_direct_global_call.value_or(
            ParsedBackendDirectGlobalTypedCallView{*symbol_name, backend_typed_call});
    const auto& declared_param_types = declared_call_surface.typed_call.param_types;

    const auto declared_params = lower_function_params(*declared_callee);
    if (declared_params.has_value()) {
      if (declared_param_types.size() != declared_params->size()) {
        return std::nullopt;
      }
      for (std::size_t index = 0; index < declared_params->size(); ++index) {
        const auto lowered_call_type = lir_to_bir::legalize_call_arg_type(declared_param_types[index]);
        if (!lowered_call_type.has_value() || *lowered_call_type != (*declared_params)[index].type) {
          return std::nullopt;
        }
      }
    } else {
      const auto params = parse_backend_function_signature_params(declared_callee->signature_text);
      if (!params.has_value()) {
        return std::nullopt;
      }

      std::size_t fixed_param_count = 0;
      bool saw_varargs = false;
      for (const auto& param : *params) {
        if (param.is_varargs) {
          saw_varargs = true;
          break;
        }
        ++fixed_param_count;
      }

      if (declared_param_types.size() < fixed_param_count ||
          (!saw_varargs && declared_param_types.size() != fixed_param_count)) {
        return std::nullopt;
      }
      for (std::size_t index = 0; index < fixed_param_count; ++index) {
        if (trim_lir_arg_text((*params)[index].type) !=
            trim_lir_arg_text(declared_param_types[index])) {
          return std::nullopt;
        }
      }
      if (saw_varargs) {
        backend_typed_call.owned_param_types.clear();
        backend_typed_call.param_types.clear();
        backend_typed_call.owned_param_types.reserve(declared_call_surface.typed_call.param_types.size());
        backend_typed_call.param_types.reserve(declared_call_surface.typed_call.param_types.size());
        for (const auto param_type : declared_call_surface.typed_call.param_types) {
          backend_typed_call.owned_param_types.push_back(std::string(param_type));
          backend_typed_call.param_types.push_back(backend_typed_call.owned_param_types.back());
        }
      }
    }
  }

  auto args = parse_backend_extern_call_args(backend_typed_call);
  if (!args.has_value()) {
    return std::nullopt;
  }

  const bool return_call_result = *main_ret->value_str == call->result.str();
  std::int64_t return_imm = 0;
  if (!return_call_result) {
    const auto parsed_return = parse_backend_i64_literal(*main_ret->value_str);
    if (!parsed_return.has_value() ||
        *parsed_return < std::numeric_limits<std::int32_t>::min() ||
        *parsed_return > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }
    return_imm = *parsed_return;
  }

  if (!return_call_result && call->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  lowered.globals.reserve(module.globals.size());
  for (const auto& global : module.globals) {
    if (global.name.empty()) {
      return std::nullopt;
    }
    lowered.globals.push_back(bir::Global{
        .name = global.name,
        .type = bir::TypeKind::I64,
        .is_extern = global.is_extern_decl,
        .initializer = std::nullopt,
    });
  }

  lowered.string_constants.reserve(module.string_pool.size());
  for (const auto& string_constant : module.string_pool) {
    if (string_constant.pool_name.empty() || string_constant.byte_length < 0) {
      return std::nullopt;
    }

    std::string name = string_constant.pool_name;
    if (!name.empty() && name.front() == '@') {
      name.erase(name.begin());
    }

    lowered.string_constants.push_back(bir::StringConstant{
        .name = std::move(name),
        .bytes = decode_call_llvm_byte_string(string_constant.raw_bytes),
    });
  }

  bir::Function lowered_callee;
  lowered_callee.name = *symbol_name;
  lowered_callee.return_type = bir::TypeKind::I32;
  lowered_callee.calling_convention = default_calling_convention_for_target(module.target_triple);
  lowered_callee.is_variadic =
      declared_callee != nullptr && function_signature_is_variadic(declared_callee->signature_text);
  lowered_callee.is_declaration = true;
  {
    std::vector<std::string_view> param_types;
    param_types.reserve(backend_typed_call.param_types.size());
    for (const auto param_type : backend_typed_call.param_types) {
      param_types.push_back(param_type);
    }
    const auto lowered_params = lower_call_params_from_type_texts(param_types);
    if (!lowered_params.has_value()) {
      return std::nullopt;
    }
    lowered_callee.params = *lowered_params;
  }
  lowered.functions.push_back(std::move(lowered_callee));

  bir::Function lowered_main_function;
  lowered_main_function.name = main_function->name;
  lowered_main_function.return_type = bir::TypeKind::I32;
  lowered_main_function.calling_convention =
      default_calling_convention_for_target(module.target_triple);

  bir::Block lowered_entry_block;
  lowered_entry_block.label = "entry";

  auto lowered_args = lower_direct_call_args(*args);
  if (!lowered_args.has_value()) {
    return std::nullopt;
  }
  lowered_entry_block.insts.push_back(make_direct_call_inst(
      std::string(*symbol_name),
      default_calling_convention_for_target(module.target_triple),
      lowered_callee.is_variadic,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, call->result.str()),
      std::move(*lowered_args)));

  if (return_call_result) {
    lowered_entry_block.terminator.value =
        bir::Value::named(bir::TypeKind::I32, call->result.str());
  } else {
    lowered_entry_block.terminator.value =
        bir::Value::immediate_i32(static_cast<std::int32_t>(return_imm));
  }

  lowered_main_function.blocks.push_back(std::move(lowered_entry_block));
  lowered.functions.push_back(std::move(lowered_main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& main_function,
         const LirFunction& helper)
      -> std::optional<std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*,
                                  std::int64_t, std::int64_t>> {
    if (main_function.is_declaration || helper.is_declaration ||
        !parse_backend_two_param_add_function(helper, std::nullopt).has_value() ||
        !lir_function_matches_minimal_no_param_integer_return(main_function, 32) ||
        helper.entry.value != 0 || main_function.entry.value != 0 || helper.blocks.size() != 1 ||
        main_function.blocks.size() != 1 || !helper.alloca_insts.empty() ||
        !helper.stack_objects.empty() || !main_function.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto& main_block = main_function.blocks.front();
    const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
    if (main_block.label != "entry" || main_ret == nullptr || !main_ret->value_str.has_value() ||
        lower_function_return_type(main_function, *main_ret) != bir::TypeKind::I32 ||
        main_block.insts.empty()) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&main_block.insts.back());
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (call == nullptr || call->result.empty() || call->result != *main_ret->value_str ||
        !call_operands.has_value()) {
      return std::nullopt;
    }

    if (main_function.alloca_insts.empty()) {
      if (main_block.insts.size() != 1) {
        return std::nullopt;
      }

      const auto lhs_call_arg_imm = parse_backend_i64_literal(call_operands->first);
      const auto rhs_call_arg_imm = parse_backend_i64_literal(call_operands->second);
      if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
        return std::nullopt;
      }
      return std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*, std::int64_t,
                        std::int64_t>{&helper, &main_function, call, *lhs_call_arg_imm,
                                      *rhs_call_arg_imm};
    }

    const std::vector<LirInst> slot_prefix(main_block.insts.begin(), main_block.insts.end() - 1);
    if (main_function.alloca_insts.size() == 1) {
      const auto* alloca = std::get_if<LirAllocaOp>(&main_function.alloca_insts.front());
      if (alloca == nullptr || !backend_lir_type_is_i32(alloca->type_str) ||
          !alloca->count.empty()) {
        return std::nullopt;
      }

      const auto matched_local = parse_backend_single_local_i32_slot_call_operand_imm(
          slot_prefix, alloca->result, call_operands->first, call_operands->second);
      if (!matched_local.has_value()) {
        return std::nullopt;
      }

      if (matched_local->second) {
        const auto rhs_call_arg_imm = parse_backend_i64_literal(call_operands->second);
        if (!rhs_call_arg_imm.has_value()) {
          return std::nullopt;
        }
        return std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*, std::int64_t,
                          std::int64_t>{&helper, &main_function, call, matched_local->first,
                                        *rhs_call_arg_imm};
      }

      const auto lhs_call_arg_imm = parse_backend_i64_literal(call_operands->first);
      if (!lhs_call_arg_imm.has_value()) {
        return std::nullopt;
      }
      return std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*, std::int64_t,
                        std::int64_t>{&helper, &main_function, call, *lhs_call_arg_imm,
                                      matched_local->first};
    }

    if (main_function.alloca_insts.size() != 2) {
      return std::nullopt;
    }

    const auto* lhs_alloca = std::get_if<LirAllocaOp>(&main_function.alloca_insts[0]);
    const auto* rhs_alloca = std::get_if<LirAllocaOp>(&main_function.alloca_insts[1]);
    if (lhs_alloca == nullptr || rhs_alloca == nullptr ||
        !backend_lir_type_is_i32(lhs_alloca->type_str) ||
        !backend_lir_type_is_i32(rhs_alloca->type_str) || !lhs_alloca->count.empty() ||
        !rhs_alloca->count.empty()) {
      return std::nullopt;
    }

    const auto matched_slots = parse_backend_two_local_i32_rewrite_call_operands_imm(
        slot_prefix, lhs_alloca->result, rhs_alloca->result, call_operands->first,
        call_operands->second);
    if (!matched_slots.has_value()) {
      return std::nullopt;
    }

    return std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*, std::int64_t,
                      std::int64_t>{&helper, &main_function, call, matched_slots->first,
                                    matched_slots->second};
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [helper_function, caller_function, call, lhs_call_arg_imm, rhs_call_arg_imm] =
      *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_function->name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%lhs"},
      bir::Param{.type = bir::TypeKind::I32, .name = "%rhs"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .lhs = bir::Value::named(bir::TypeKind::I32, helper.params[0].name),
      .rhs = bir::Value::named(bir::TypeKind::I32, helper.params[1].name),
  });
  helper_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = caller_function->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(make_direct_call_inst(
      helper_function->name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, call->result.str()),
      {
          bir::Value::immediate_i32(static_cast<std::int32_t>(lhs_call_arg_imm)),
          bir::Value::immediate_i32(static_cast<std::int32_t>(rhs_call_arg_imm)),
      }));
  main_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, call->result.str());
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_direct_call_add_imm_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& caller,
         const LirFunction& helper)
      -> std::optional<std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*,
                                  std::int64_t, std::int64_t>> {
    if (caller.is_declaration || helper.is_declaration ||
        !lir_function_matches_minimal_no_param_integer_return(caller, 32) ||
        caller.entry.value != 0 || helper.entry.value != 0 || caller.blocks.size() != 1 ||
        helper.blocks.size() != 1 || !caller.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto helper_shape = parse_backend_single_add_imm_function(helper, std::nullopt);
    if (!helper_shape.has_value()) {
      return std::nullopt;
    }

    const auto add_imm = parse_backend_i64_literal(helper_shape->add->rhs);
    if (!add_imm.has_value()) {
      return std::nullopt;
    }

    const auto& caller_block = caller.blocks.front();
    const auto* caller_ret = std::get_if<LirRet>(&caller_block.terminator);
    if (caller_block.label != "entry" || caller_ret == nullptr ||
        !caller_ret->value_str.has_value() ||
        lower_function_return_type(caller, *caller_ret) != bir::TypeKind::I32 ||
        caller_block.insts.empty()) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&caller_block.insts.back());
    if (call == nullptr || call->result.empty() || call->result != *caller_ret->value_str) {
      return std::nullopt;
    }

    std::optional<std::int64_t> call_arg_imm;
    if (caller.alloca_insts.empty()) {
      if (caller_block.insts.size() != 1) {
        return std::nullopt;
      }
      const auto operand =
          parse_backend_direct_global_single_typed_call_operand(*call, helper.name, "i32");
      if (!operand.has_value()) {
        return std::nullopt;
      }
      call_arg_imm = parse_backend_i64_literal(*operand);
    } else if (caller.alloca_insts.size() == 1) {
      const auto* alloca = std::get_if<LirAllocaOp>(&caller.alloca_insts.front());
      if (alloca == nullptr || !backend_lir_type_is_i32(alloca->type_str) ||
          !alloca->count.empty()) {
        return std::nullopt;
      }

      const auto parsed_local_call =
          parse_backend_single_local_typed_call(caller_block.insts, alloca->result);
      if (!parsed_local_call.has_value() || parsed_local_call->arg_load == nullptr ||
          parsed_local_call->call == nullptr) {
        return std::nullopt;
      }

      const auto operand = parse_backend_direct_global_single_typed_call_operand(
          *parsed_local_call->call, helper.name, "i32");
      if (!operand.has_value() || *operand != parsed_local_call->arg_load->result) {
        return std::nullopt;
      }

      const auto* store = std::get_if<LirStoreOp>(&caller_block.insts.front());
      if (store == nullptr || !backend_lir_type_is_i32(store->type_str) ||
          store->ptr != alloca->result) {
        return std::nullopt;
      }
      call_arg_imm = parse_backend_i64_literal(store->val);
    } else {
      return std::nullopt;
    }

    if (!call_arg_imm.has_value()) {
      return std::nullopt;
    }

    return std::tuple<const LirFunction*, const LirFunction*, const LirCallOp*, std::int64_t,
                      std::int64_t>{&helper, &caller, call, *call_arg_imm, *add_imm};
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [helper_function, main_function_lir, call, call_arg_imm, add_imm] = *parsed;
  if (call->result.str().empty()) {
    return std::nullopt;
  }

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_function->name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .lhs = bir::Value::named(bir::TypeKind::I32, helper.params.front().name),
      .rhs = bir::Value::immediate_i32(static_cast<std::int32_t>(add_imm)),
  });
  helper_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = main_function_lir->name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(make_direct_call_inst(
      helper_function->name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, call->result.str()),
      {
          bir::Value::immediate_i32(static_cast<std::int32_t>(call_arg_imm)),
      }));
  main_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, call->result.str());
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_direct_call_identity_arg_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& main_function,
         const LirFunction& helper)
      -> std::optional<std::tuple<std::string, std::string, std::string, std::int64_t>> {
    if (main_function.is_declaration || helper.is_declaration ||
        !lir_function_matches_minimal_no_param_integer_return(main_function, 32) ||
        main_function.entry.value != 0 || helper.entry.value != 0 ||
        main_function.blocks.size() != 1 || helper.blocks.size() != 1 ||
        !main_function.alloca_insts.empty() || !main_function.stack_objects.empty()) {
      return std::nullopt;
    }

    if (!parse_backend_single_identity_function(helper, std::nullopt).has_value()) {
      return std::nullopt;
    }

    const auto& main_block = main_function.blocks.front();
    const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
    if (main_block.label != "entry" || main_block.insts.size() != 1 || main_ret == nullptr ||
        !main_ret->value_str.has_value() ||
        lower_function_return_type(main_function, *main_ret) != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&main_block.insts.front());
    if (call == nullptr || call->result.empty() || call->result != *main_ret->value_str) {
      return std::nullopt;
    }

    const auto operand =
        parse_backend_direct_global_single_typed_call_operand(*call, helper.name, "i32");
    const auto call_arg_imm =
        operand.has_value() ? parse_backend_i64_literal(*operand) : std::nullopt;
    if (!call_arg_imm.has_value()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::string, std::string, std::int64_t>{
        helper.name,
        main_function.name,
        call->result.str(),
        *call_arg_imm,
    };
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [helper_name, main_name, call_result, call_arg_imm] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.terminator.value =
      bir::Value::named(bir::TypeKind::I32, helper.params.front().name);
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = main_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.insts.push_back(make_direct_call_inst(
      helper_name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, call_result),
      {
          bir::Value::immediate_i32(static_cast<std::int32_t>(call_arg_imm)),
      }));
  main_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, call_result);
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_folded_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() || !module.string_pool.empty() ||
      !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& caller,
         const LirFunction& helper) -> std::optional<std::tuple<std::string, std::int64_t>> {
    if (caller.is_declaration || helper.is_declaration ||
        !matches_minimal_i32_function_signature(caller, {}) ||
        !matches_minimal_i32_function_signature(helper, {"i32", "i32"}) ||
        caller.entry.value != 0 || helper.entry.value != 0 || caller.blocks.size() != 1 ||
        helper.blocks.size() != 1 || !caller.alloca_insts.empty() ||
        !helper.alloca_insts.empty() || !caller.stack_objects.empty() ||
        !helper.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto helper_params = lower_function_params(helper);
    if (!helper_params.has_value() || helper_params->size() != 2 ||
        (*helper_params)[0].type != bir::TypeKind::I32 ||
        (*helper_params)[1].type != bir::TypeKind::I32 ||
        (*helper_params)[0].name.empty() || (*helper_params)[1].name.empty()) {
      return std::nullopt;
    }

    const auto& helper_block = helper.blocks.front();
    const auto* helper_ret = std::get_if<LirRet>(&helper_block.terminator);
    if (helper_block.label != "entry" || helper_block.insts.size() != 2 || helper_ret == nullptr ||
        !helper_ret->value_str.has_value() || helper_ret->type_str != "i32") {
      return std::nullopt;
    }

    const auto* add = std::get_if<LirBinOp>(&helper_block.insts[0]);
    const auto* sub = std::get_if<LirBinOp>(&helper_block.insts[1]);
    const auto add_opcode = add == nullptr ? std::nullopt : add->opcode.typed();
    const auto sub_opcode = sub == nullptr ? std::nullopt : sub->opcode.typed();
    const auto add_type = add == nullptr ? std::nullopt : lower_scalar_type(add->type_str);
    const auto sub_type = sub == nullptr ? std::nullopt : lower_scalar_type(sub->type_str);
    if (add == nullptr || sub == nullptr || add_opcode != LirBinaryOpcode::Add ||
        sub_opcode != LirBinaryOpcode::Sub || add_type != bir::TypeKind::I32 ||
        sub_type != bir::TypeKind::I32 ||
        *helper_ret->value_str != sub->result || sub->lhs != add->result ||
        sub->rhs != (*helper_params)[1].name) {
      return std::nullopt;
    }

    const auto add_lhs_imm = parse_backend_i64_literal(add->lhs);
    const auto add_rhs_imm = parse_backend_i64_literal(add->rhs);
    const std::string_view lhs_param = (*helper_params)[0].name;
    std::int64_t base_imm = 0;
    if (add->rhs == lhs_param && add_lhs_imm.has_value()) {
      base_imm = *add_lhs_imm;
    } else if (add->lhs == lhs_param && add_rhs_imm.has_value()) {
      base_imm = *add_rhs_imm;
    } else {
      return std::nullopt;
    }

    const auto& caller_block = caller.blocks.front();
    const auto* caller_ret = std::get_if<LirRet>(&caller_block.terminator);
    if (caller_block.label != "entry" || caller_block.insts.size() != 1 || caller_ret == nullptr ||
        !caller_ret->value_str.has_value() || caller_ret->type_str != "i32") {
      return std::nullopt;
    }

    const auto* call = std::get_if<LirCallOp>(&caller_block.insts.front());
    const auto call_operands =
        call == nullptr
            ? std::nullopt
            : parse_backend_direct_global_two_typed_call_operands(
                  *call, helper.name, "i32", "i32");
    if (call == nullptr || call->result.empty() || call->result != *caller_ret->value_str ||
        !call_operands.has_value()) {
      return std::nullopt;
    }

    const auto lhs_call_arg_imm = parse_backend_i64_literal(call_operands->first);
    const auto rhs_call_arg_imm = parse_backend_i64_literal(call_operands->second);
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::int64_t>{
        caller.name,
        base_imm + *lhs_call_arg_imm - *rhs_call_arg_imm,
    };
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value() || std::get<1>(*parsed) < std::numeric_limits<std::int32_t>::min() ||
      std::get<1>(*parsed) > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }
  const auto& [caller_name, return_imm] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function main_function;
  main_function.name = caller_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block main_entry;
  main_entry.label = "entry";
  main_entry.terminator.value =
      bir::Value::immediate_i32(static_cast<std::int32_t>(return_imm));
  main_function.blocks.push_back(std::move(main_entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_dual_identity_direct_call_sub_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 3 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& main_function,
         const LirFunction& lhs_helper,
         const LirFunction& rhs_helper)
      -> std::optional<std::tuple<std::string, std::string, std::string, std::string, std::string,
                                  std::string, std::string, std::int64_t, std::int64_t>> {
    if (main_function.is_declaration || lhs_helper.is_declaration || rhs_helper.is_declaration ||
        lhs_helper.name == rhs_helper.name ||
        !lir_function_matches_minimal_no_param_integer_return(main_function, 32) ||
        main_function.entry.value != 0 || lhs_helper.entry.value != 0 ||
        rhs_helper.entry.value != 0 || main_function.blocks.size() != 1 ||
        lhs_helper.blocks.size() != 1 || rhs_helper.blocks.size() != 1 ||
        !main_function.alloca_insts.empty() || !main_function.stack_objects.empty()) {
      return std::nullopt;
    }

    if (!parse_backend_single_identity_function(lhs_helper, std::nullopt).has_value() ||
        !parse_backend_single_identity_function(rhs_helper, std::nullopt).has_value()) {
      return std::nullopt;
    }

    const auto& main_block = main_function.blocks.front();
    const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
    if (main_block.label != "entry" || main_block.insts.size() != 3 || main_ret == nullptr ||
        !main_ret->value_str.has_value() ||
        lower_function_return_type(main_function, *main_ret) != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* lhs_call = std::get_if<LirCallOp>(&main_block.insts[0]);
    const auto* rhs_call = std::get_if<LirCallOp>(&main_block.insts[1]);
    const auto* sub = std::get_if<LirBinOp>(&main_block.insts[2]);
    const auto sub_opcode = sub == nullptr ? std::nullopt : sub->opcode.typed();
    if (lhs_call == nullptr || rhs_call == nullptr || sub == nullptr ||
        sub_opcode != LirBinaryOpcode::Sub || !backend_lir_type_is_i32(sub->type_str) ||
        lhs_call->result.empty() || rhs_call->result.empty() || sub->result.empty() ||
        sub->lhs != lhs_call->result || sub->rhs != rhs_call->result ||
        *main_ret->value_str != sub->result) {
      return std::nullopt;
    }

    const auto lhs_operand =
        parse_backend_direct_global_single_typed_call_operand(*lhs_call, lhs_helper.name, "i32");
    const auto rhs_operand =
        parse_backend_direct_global_single_typed_call_operand(*rhs_call, rhs_helper.name, "i32");
    const auto lhs_call_arg_imm =
        lhs_operand.has_value() ? parse_backend_i64_literal(*lhs_operand) : std::nullopt;
    const auto rhs_call_arg_imm =
        rhs_operand.has_value() ? parse_backend_i64_literal(*rhs_operand) : std::nullopt;
    if (!lhs_call_arg_imm.has_value() || !rhs_call_arg_imm.has_value()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::string, std::string, std::string, std::string,
                      std::string, std::string, std::int64_t, std::int64_t>{
        lhs_helper.name,
        rhs_helper.name,
        main_function.name,
        lhs_call->result.str(),
        rhs_call->result.str(),
        sub->result.str(),
        main_block.label,
        *lhs_call_arg_imm,
        *rhs_call_arg_imm,
    };
  };

  std::optional<std::tuple<std::string, std::string, std::string, std::string, std::string,
                           std::string, std::string, std::int64_t, std::int64_t>>
      parsed;
  for (std::size_t main_index = 0; main_index < module.functions.size() && !parsed.has_value();
       ++main_index) {
    for (std::size_t lhs_index = 0; lhs_index < module.functions.size() && !parsed.has_value();
         ++lhs_index) {
      if (lhs_index == main_index) {
        continue;
      }
      for (std::size_t rhs_index = 0; rhs_index < module.functions.size(); ++rhs_index) {
        if (rhs_index == main_index || rhs_index == lhs_index) {
          continue;
        }
        parsed = try_match(
            module.functions[main_index], module.functions[lhs_index], module.functions[rhs_index]);
        if (parsed.has_value()) {
          break;
        }
      }
    }
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [lhs_helper_name, rhs_helper_name, main_name, lhs_call_result, rhs_call_result,
               sub_result, entry_label, lhs_call_arg_imm, rhs_call_arg_imm] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  auto make_identity_helper = [](std::string_view name) {
    bir::Function helper;
    helper.name = std::string(name);
    helper.return_type = bir::TypeKind::I32;
    helper.params = {
        bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
    };

    bir::Block entry;
    entry.label = "entry";
    entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%arg0");
    helper.blocks.push_back(std::move(entry));
    return helper;
  };

  lowered.functions.push_back(make_identity_helper(lhs_helper_name));
  lowered.functions.push_back(make_identity_helper(rhs_helper_name));

  bir::Function main_function;
  main_function.name = main_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = entry_label;
  entry.insts.push_back(make_direct_call_inst(
      lhs_helper_name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, lhs_call_result),
      {bir::Value::immediate_i32(static_cast<std::int32_t>(lhs_call_arg_imm))}));
  entry.insts.push_back(make_direct_call_inst(
      rhs_helper_name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, rhs_call_result),
      {bir::Value::immediate_i32(static_cast<std::int32_t>(rhs_call_arg_imm))}));
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, sub_result),
      .lhs = bir::Value::named(bir::TypeKind::I32, lhs_call_result),
      .rhs = bir::Value::named(bir::TypeKind::I32, rhs_call_result),
  });
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, sub_result);
  main_function.blocks.push_back(std::move(entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

std::optional<bir::Module> try_lower_minimal_call_crossing_direct_call_module(
    const c4c::codegen::lir::LirModule& module) {
  using namespace c4c::codegen::lir;

  if (module.functions.size() != 2 || !module.globals.empty() ||
      !module.string_pool.empty() || !module.extern_decls.empty()) {
    return std::nullopt;
  }

  const auto try_match =
      [](const LirFunction& main_function,
         const LirFunction& helper)
      -> std::optional<std::tuple<std::string, std::string, std::string, std::string, std::string,
                                  std::int64_t, std::int64_t, std::string>> {
    if (main_function.is_declaration || helper.is_declaration ||
        !lir_function_matches_minimal_no_param_integer_return(main_function, 32) ||
        main_function.entry.value != 0 || helper.entry.value != 0 ||
        main_function.blocks.size() != 1 || helper.blocks.size() != 1 ||
        !main_function.alloca_insts.empty() || !main_function.stack_objects.empty()) {
      return std::nullopt;
    }

    const auto helper_shape = parse_backend_single_add_imm_function(helper, std::nullopt);
    if (!helper_shape.has_value()) {
      return std::nullopt;
    }

    const auto& main_block = main_function.blocks.front();
    const auto* main_ret = std::get_if<LirRet>(&main_block.terminator);
    if (main_block.label.empty() || main_block.insts.size() != 3 || main_ret == nullptr ||
        !main_ret->value_str.has_value() ||
        lower_function_return_type(main_function, *main_ret) != bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto* source_add = std::get_if<LirBinOp>(&main_block.insts[0]);
    const auto* call = std::get_if<LirCallOp>(&main_block.insts[1]);
    const auto* final_add = std::get_if<LirBinOp>(&main_block.insts[2]);
    const auto source_add_opcode =
        source_add == nullptr ? std::nullopt : source_add->opcode.typed();
    const auto final_add_opcode =
        final_add == nullptr ? std::nullopt : final_add->opcode.typed();
    if (source_add == nullptr || call == nullptr || final_add == nullptr ||
        source_add_opcode != LirBinaryOpcode::Add || final_add_opcode != LirBinaryOpcode::Add ||
        !backend_lir_type_is_i32(source_add->type_str) ||
        !backend_lir_type_is_i32(final_add->type_str) ||
        source_add->result.empty() || call->result.empty() || final_add->result.empty() ||
        final_add->lhs != source_add->result || final_add->rhs != call->result ||
        *main_ret->value_str != final_add->result) {
      return std::nullopt;
    }

    const auto operand =
        parse_backend_direct_global_single_typed_call_operand(*call, helper.name, "i32");
    if (!operand.has_value() || *operand != source_add->result) {
      return std::nullopt;
    }

    const auto lhs_imm = parse_backend_i64_literal(source_add->lhs);
    const auto rhs_imm = parse_backend_i64_literal(source_add->rhs);
    const auto add_imm = parse_backend_i64_literal(helper_shape->add->rhs);
    if (!lhs_imm.has_value() || !rhs_imm.has_value() || !add_imm.has_value()) {
      return std::nullopt;
    }

    return std::tuple<std::string, std::string, std::string, std::string, std::string,
                      std::int64_t, std::int64_t, std::string>{
        helper.name,
        main_function.name,
        source_add->result.str(),
        call->result.str(),
        final_add->result.str(),
        *lhs_imm + *rhs_imm,
        *add_imm,
        main_block.label,
    };
  };

  auto parsed = try_match(module.functions[0], module.functions[1]);
  if (!parsed.has_value()) {
    parsed = try_match(module.functions[1], module.functions[0]);
  }
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  const auto& [helper_name, main_name, source_add_result, call_result, final_add_result,
               source_imm, helper_add_imm, entry_label] = *parsed;

  bir::Module lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  bir::Function helper;
  helper.name = helper_name;
  helper.return_type = bir::TypeKind::I32;
  helper.params = {
      bir::Param{.type = bir::TypeKind::I32, .name = "%arg0"},
  };

  bir::Block helper_entry;
  helper_entry.label = "entry";
  helper_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .lhs = bir::Value::named(bir::TypeKind::I32, helper.params.front().name),
      .rhs = bir::Value::immediate_i32(static_cast<std::int32_t>(helper_add_imm)),
  });
  helper_entry.terminator.value = bir::Value::named(bir::TypeKind::I32, "%sum");
  helper.blocks.push_back(std::move(helper_entry));
  lowered.functions.push_back(std::move(helper));

  bir::Function main_function;
  main_function.name = main_name;
  main_function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = entry_label;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, source_add_result),
      .lhs = bir::Value::immediate_i32(static_cast<std::int32_t>(source_imm)),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.insts.push_back(make_direct_call_inst(
      helper_name,
      default_calling_convention_for_target(module.target_triple),
      false,
      bir::TypeKind::I32,
      "i32",
      bir::Value::named(bir::TypeKind::I32, call_result),
      {
          bir::Value::named(bir::TypeKind::I32, source_add_result),
      }));
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, final_add_result),
      .lhs = bir::Value::named(bir::TypeKind::I32, source_add_result),
      .rhs = bir::Value::named(bir::TypeKind::I32, call_result),
  });
  entry.terminator.value = bir::Value::named(bir::TypeKind::I32, final_add_result);
  main_function.blocks.push_back(std::move(entry));
  lowered.functions.push_back(std::move(main_function));
  return lowered;
}

void record_call_lowering_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                         std::vector<BirLoweringNote>* notes) {
  (void)module;
  if (notes == nullptr) {
    return;
  }
  notes->push_back(BirLoweringNote{
      .phase = "call-lowering",
      .message = "call ABI and call lowering scaffold lives in lir_to_bir/calls.cpp",
  });
}

}  // namespace c4c::backend
