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
