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

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
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
