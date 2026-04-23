#pragma once

#include <algorithm>

#include "../assembler/mod.hpp"
#include "../../../bir/lir_to_bir.hpp"
#include "../../../prealloc/prealloc.hpp"
#include "../../../prealloc/target_register_profile.hpp"
#include "prepared/prepared_query_context.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {
namespace bir {
struct Module;
enum class BinaryOpcode : unsigned char;
}
namespace prepare {
struct PreparedBirModule;
struct PreparedBranchCondition;
struct PreparedControlFlowFunction;
struct PreparedParamZeroBranchReturnContext;
struct PreparedResolvedMaterializedCompareJoinRenderContract;
struct PreparedResolvedMaterializedCompareJoinReturnArm;
struct PreparedSupportedImmediateBinary;
}

struct ParsedBackendExternCallArg;
struct LivenessInput;
struct LivenessResult;
struct PhysReg {
  std::uint32_t index = 0;

  constexpr PhysReg() = default;
  constexpr explicit PhysReg(std::uint32_t v) : index(v) {}
};
struct RegAllocIntegrationResult;
}  // namespace c4c::backend

namespace c4c::backend::x86 {

// Transitional x86 codegen surface.
// The intent is that sibling translation units in this directory depend on this
// header instead of on emit.cpp-local declarations.

struct Value {
  std::uint32_t raw = 0;
};

struct Operand {
  std::uint32_t raw = 0;
  std::optional<std::int64_t> immediate = std::nullopt;

  static Operand immediate_i32(std::int32_t value) {
    return Operand{
        .raw = 0,
        .immediate = static_cast<std::int64_t>(value),
    };
  }

  static Operand immediate_i64(std::int64_t value) {
    return Operand{
        .raw = 0,
        .immediate = value,
    };
  }

  static Operand zero() { return Operand::immediate_i32(0); }
};

enum class AddressSpace : unsigned {
  Default,
  SegFs,
  SegGs,
};
enum class IrType : unsigned {
  Void,
  I8,
  I16,
  I32,
  I64,
  I128,
  U16,
  U32,
  U64,
  Ptr,
  F32,
  F64,
  F128,
};
enum class IrCmpOp : unsigned {
  Eq,
  Ne,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
};
enum class IrUnaryOp : unsigned {
  Neg,
  Not,
  Clz,
  Ctz,
  Bswap,
  Popcount,
};
enum class AtomicOrdering : unsigned;
enum class AtomicRmwOp : unsigned;
enum class FloatOp : unsigned {
  Add,
  Sub,
  Mul,
  Div,
};
enum class AsmOperandKind : unsigned;
enum class EightbyteClass : unsigned {
  None,
  Integer,
  Sse,
  X87,
  Memory,
};
enum class RiscvFloatClass : unsigned {
  None,
  Float,
  Double,
};

enum class PreparedParamZeroCompareShape : unsigned {
  SelfTest,
};

struct PreparedI32CallResultAbiSelection {
  const c4c::backend::prepare::PreparedMoveResolution* move = nullptr;
  std::string abi_register;
};

struct PreparedCallResultAbiSelection {
  const c4c::backend::prepare::PreparedMoveResolution* move = nullptr;
  std::string abi_register;
};

struct DirectBranchTargets {
  const c4c::backend::bir::Block* true_block = nullptr;
  const c4c::backend::bir::Block* false_block = nullptr;
};

inline constexpr std::string_view kPreparedCallBundleHandoffRequired =
    "x86 backend emitter requires the authoritative prepared call-bundle handoff through the canonical prepared-module handoff";

inline std::optional<std::string> narrow_i32_register(std::string_view wide_register) {
  if (wide_register == "rax") return std::string("eax");
  if (wide_register == "rbx") return std::string("ebx");
  if (wide_register == "rcx") return std::string("ecx");
  if (wide_register == "rdx") return std::string("edx");
  if (wide_register == "rdi") return std::string("edi");
  if (wide_register == "rsi") return std::string("esi");
  if (wide_register == "rbp") return std::string("ebp");
  if (wide_register == "rsp") return std::string("esp");
  if (wide_register == "r8") return std::string("r8d");
  if (wide_register == "r9") return std::string("r9d");
  if (wide_register == "r10") return std::string("r10d");
  if (wide_register == "r11") return std::string("r11d");
  if (wide_register == "r12") return std::string("r12d");
  if (wide_register == "r13") return std::string("r13d");
  if (wide_register == "r14") return std::string("r14d");
  if (wide_register == "r15") return std::string("r15d");
  return std::string(wide_register);
}

inline std::optional<std::string> narrow_i8_register(std::string_view wide_register) {
  if (wide_register == "rax") return std::string("al");
  if (wide_register == "rbx") return std::string("bl");
  if (wide_register == "rcx") return std::string("cl");
  if (wide_register == "rdx") return std::string("dl");
  if (wide_register == "rdi") return std::string("dil");
  if (wide_register == "rsi") return std::string("sil");
  if (wide_register == "rbp") return std::string("bpl");
  if (wide_register == "rsp") return std::string("spl");
  if (wide_register == "r8") return std::string("r8b");
  if (wide_register == "r9") return std::string("r9b");
  if (wide_register == "r10") return std::string("r10b");
  if (wide_register == "r11") return std::string("r11b");
  if (wide_register == "r12") return std::string("r12b");
  if (wide_register == "r13") return std::string("r13b");
  if (wide_register == "r14") return std::string("r14b");
  if (wide_register == "r15") return std::string("r15b");
  return std::string(wide_register);
}

// Compatibility holdout until the reviewed prepared call-bundle render seam
// lands in step 3. Keep these selectors live here rather than reviving
// dormant `shared_call_support.cpp` or `mod.cpp` ownership.
std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::string> select_prepared_i32_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::string> select_prepared_i32_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<PreparedCallResultAbiSelection>
select_prepared_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

std::optional<PreparedCallResultAbiSelection>
select_prepared_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

std::optional<PreparedI32CallResultAbiSelection>
select_prepared_i32_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

std::optional<PreparedI32CallResultAbiSelection>
select_prepared_i32_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

template <typename RenderMoveToEaxFn, typename RenderI32OperandFn>
inline std::optional<std::string> render_prepared_i32_binary_in_eax_if_supported(
    const c4c::backend::bir::BinaryInst& binary,
    const std::optional<std::string_view>& current_i32_name,
    const RenderMoveToEaxFn& render_value_to_eax,
    const RenderI32OperandFn& render_i32_operand) {
  if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  const auto render_rhs =
      [&](const c4c::backend::bir::Value& rhs) -> std::optional<std::string> {
    return render_i32_operand(rhs, current_i32_name);
  };
  const auto render_with_lhs_in_eax =
      [&](const c4c::backend::bir::Value& lhs,
          const c4c::backend::bir::Value& rhs) -> std::optional<std::string> {
    const auto setup = render_value_to_eax(lhs, current_i32_name);
    const auto rhs_operand = render_rhs(rhs);
    if (!setup.has_value() || !rhs_operand.has_value()) {
      return std::nullopt;
    }

    std::string rendered = *setup;
    switch (binary.opcode) {
      case c4c::backend::bir::BinaryOpcode::Add:
        rendered += "    add eax, " + *rhs_operand + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::Sub:
        rendered += "    sub eax, " + *rhs_operand + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::Mul:
        rendered += "    imul eax, " + *rhs_operand + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::And:
        rendered += "    and eax, " + *rhs_operand + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::Or:
        rendered += "    or eax, " + *rhs_operand + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::Xor:
        rendered += "    xor eax, " + *rhs_operand + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::Shl:
        if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
            rhs.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        rendered += "    shl eax, " +
                    std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::LShr:
        if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
            rhs.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        rendered += "    shr eax, " +
                    std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
        return rendered;
      case c4c::backend::bir::BinaryOpcode::AShr:
        if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
            rhs.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        rendered += "    sar eax, " +
                    std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
        return rendered;
      default:
        return std::nullopt;
    }
  };

  if (const auto rendered = render_with_lhs_in_eax(binary.lhs, binary.rhs);
      rendered.has_value()) {
    return rendered;
  }
  switch (binary.opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
      return render_with_lhs_in_eax(binary.rhs, binary.lhs);
    default:
      return std::nullopt;
  }
}

template <typename RenderI32OperandFn>
inline std::optional<std::string> render_prepared_i32_store_to_memory_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    std::string_view memory_operand,
    const RenderI32OperandFn& render_i32_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  const auto operand = render_i32_operand(value, current_i32_name);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  return "    mov " + std::string(memory_operand) + ", " + *operand + "\n";
}

template <typename RenderI8OperandFn>
inline std::optional<std::string> render_prepared_i8_store_to_memory_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i8_name,
    std::string_view memory_operand,
    const RenderI8OperandFn& render_i8_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I8) {
    return std::nullopt;
  }
  const auto operand = render_i8_operand(value, current_i8_name);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  return "    mov " + std::string(memory_operand) + ", " + *operand + "\n";
}

template <typename RenderNamedPtrAddressFn>
inline std::optional<std::string> render_prepared_ptr_store_to_memory_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_ptr_name,
    std::string_view memory_operand,
    const RenderNamedPtrAddressFn& render_named_ptr_address) {
  if (value.kind != c4c::backend::bir::Value::Kind::Named ||
      value.type != c4c::backend::bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  if (current_ptr_name.has_value() && *current_ptr_name == value.name) {
    return "    mov " + std::string(memory_operand) + ", rax\n";
  }
  const auto pointee_address = render_named_ptr_address(value.name);
  if (!pointee_address.has_value()) {
    return std::nullopt;
  }
  return "    lea rax, " + *pointee_address + "\n    mov " + std::string(memory_operand) +
         ", rax\n";
}

inline std::optional<std::string> render_prepared_scalar_load_from_memory_if_supported(
    c4c::backend::bir::TypeKind result_type,
    std::string_view memory_operand) {
  switch (result_type) {
    case c4c::backend::bir::TypeKind::Ptr:
      return "    mov rax, " + std::string(memory_operand) + "\n";
    case c4c::backend::bir::TypeKind::I32:
      return "    mov eax, " + std::string(memory_operand) + "\n";
    case c4c::backend::bir::TypeKind::I8:
      return "    movsx eax, " + std::string(memory_operand) + "\n";
    default:
      return std::nullopt;
  }
}

enum class IntrinsicOp : std::uint16_t;
struct AsmOperand;
struct BlockId {
  std::uint32_t raw = 0;
};

struct StackSlot {
  std::int64_t raw = 0;
};

struct PreparedBoundedMultiDefinedCallLaneRender {
  std::string body;
  std::vector<std::string> used_string_names;
  std::vector<std::string> used_same_module_global_names;
};

struct PreparedBoundedMultiDefinedCallLaneModuleRender {
  std::string rendered_functions;
  std::vector<std::string> used_string_names;
  std::vector<std::string> used_same_module_global_names;
};

struct PreparedBoundedSameModuleHelperPrefixRender {
  std::string helper_prefix;
  std::unordered_set<std::string_view> helper_names;
  std::unordered_set<std::string_view> helper_string_names;
  std::unordered_set<std::string_view> helper_global_names;
};

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch);

inline std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    c4c::TargetArch prepared_arch) {
  return build_prepared_module_local_slot_layout(
      function, stack_layout, nullptr, nullptr, prepared_arch);
}

inline std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    c4c::TargetArch prepared_arch) {
  return build_prepared_module_local_slot_layout(function, nullptr, nullptr, nullptr, prepared_arch);
}

inline std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch) {
  return build_prepared_module_local_slot_layout(
      function, nullptr, function_addressing, prepared_names,
                                                 prepared_arch);
}

// Compatibility holdout until the reviewed prepared fast-path operand seam lands.
std::string render_prepared_stack_memory_operand(std::size_t byte_offset,
                                                 std::string_view size_name);

std::string render_prepared_stack_address_expr(std::size_t byte_offset);

std::optional<std::string> render_prepared_local_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view slot_name,
    std::size_t stack_byte_bias,
    std::string_view size_name);

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register);

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_param_derived_i32_value_if_supported(
    std::string_view return_register,
    const c4c::backend::bir::Value& value,
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
        named_binaries,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register);

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register);

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix);

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_local_i16_arithmetic_guard_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block);

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix);

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data,
    const std::function<std::string(std::string)>& prepend_bounded_same_module_helpers);

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const PreparedX86FunctionDispatchContext& context);

struct PreparedBoundedMultiDefinedCurrentI32Carrier {
  std::string_view value_name;
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
};

struct PreparedBoundedMultiDefinedNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
};

inline const c4c::backend::prepare::PreparedValueHome*
find_prepared_bounded_multi_defined_named_value_home(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    std::string_view value_name) {
  return c4c::backend::prepare::find_prepared_value_home(module.names, function_locations, value_name);
}

inline std::optional<std::size_t> find_prepared_fixed_permanent_named_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  if (value_name.empty() || function_name == c4c::kInvalidFunctionName) {
    return std::nullopt;
  }

  const auto requested_slice = c4c::backend::prepare::parse_prepared_slot_slice_name(value_name);
  std::optional<std::size_t> best_slice_offset;
  std::optional<std::size_t> best_frame_offset;
  for (const auto& object : stack_layout.objects) {
    if (object.function_name != function_name || !object.permanent_home_slot ||
        !object.address_exposed || object.source_kind != "local_slot") {
      continue;
    }
    const auto* frame_slot =
        c4c::backend::prepare::find_prepared_frame_slot(stack_layout, object.object_id);
    if (frame_slot == nullptr || !frame_slot->fixed_location) {
      continue;
    }

    const auto object_name = c4c::backend::prepare::prepared_stack_object_name(prepared_names, object);
    if (object_name == value_name) {
      return frame_slot->offset_bytes;
    }

    if (!object.slot_name.has_value()) {
      continue;
    }
    const auto slot_name = c4c::backend::prepare::prepared_slot_name(prepared_names, *object.slot_name);
    const auto candidate_slice = c4c::backend::prepare::parse_prepared_slot_slice_name(slot_name);
    if (!candidate_slice.has_value()) {
      continue;
    }

    std::optional<std::size_t> resolved_offset;
    if (requested_slice.has_value()) {
      if (candidate_slice->first != requested_slice->first ||
          requested_slice->second < candidate_slice->second ||
          requested_slice->second >= candidate_slice->second + object.size_bytes) {
        continue;
      }
      resolved_offset =
          frame_slot->offset_bytes + (requested_slice->second - candidate_slice->second);
    } else {
      if (candidate_slice->first != value_name || candidate_slice->second != 0) {
        continue;
      }
      resolved_offset = frame_slot->offset_bytes;
    }

    if (!resolved_offset.has_value() ||
        (best_slice_offset.has_value() && candidate_slice->second >= *best_slice_offset)) {
      continue;
    }
    best_slice_offset = candidate_slice->second;
    best_frame_offset = *resolved_offset;
  }

  return best_frame_offset;
}

inline std::optional<std::size_t> find_prepared_authoritative_named_stack_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name,
    std::unordered_set<std::string_view>* visited_names = nullptr) {
  if (prepared_names == nullptr || value_name.empty()) {
    return std::nullopt;
  }

  std::unordered_set<std::string_view> local_visited_names;
  if (visited_names == nullptr) {
    visited_names = &local_visited_names;
  }
  if (!visited_names->insert(value_name).second) {
    return std::nullopt;
  }

  if (stack_layout != nullptr && function_name != c4c::kInvalidFunctionName) {
    if (const auto permanent_home_offset =
            find_prepared_fixed_permanent_named_stack_offset_if_supported(
                *stack_layout, *prepared_names, function_name, value_name);
        permanent_home_offset.has_value()) {
      return permanent_home_offset;
    }
  }

  if (const auto local_slot_it = local_layout.offsets.find(value_name);
      local_slot_it != local_layout.offsets.end()) {
    return local_slot_it->second;
  }
  if (!c4c::backend::prepare::parse_prepared_slot_slice_name(value_name).has_value()) {
    std::optional<std::size_t> best_frame_offset;
    std::optional<std::size_t> best_slice_offset;
    for (const auto& [slot_name, slot_offset] : local_layout.offsets) {
      const auto candidate_slice =
          c4c::backend::prepare::parse_prepared_slot_slice_name(slot_name);
      if (!candidate_slice.has_value() || candidate_slice->first != value_name ||
          candidate_slice->second != 0) {
        continue;
      }
      if (best_slice_offset.has_value() && candidate_slice->second >= *best_slice_offset) {
        continue;
      }
      best_slice_offset = candidate_slice->second;
      best_frame_offset = slot_offset;
    }
    if (best_frame_offset.has_value()) {
      return best_frame_offset;
    }
  }

  if (stack_layout != nullptr && function_name != c4c::kInvalidFunctionName) {
    if (const auto stack_offset =
            c4c::backend::prepare::find_prepared_stack_frame_offset_by_name(
                *prepared_names, *stack_layout, function_name, value_name);
        stack_offset.has_value()) {
      return stack_offset;
    }
  }

  if (const auto* home =
          function_locations == nullptr
              ? nullptr
              : c4c::backend::prepare::find_prepared_value_home(
                    *prepared_names, *function_locations, value_name);
      home != nullptr &&
      home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
    if (home->slot_id.has_value()) {
      const auto frame_slot_it = local_layout.frame_slot_offsets.find(*home->slot_id);
      if (frame_slot_it != local_layout.frame_slot_offsets.end()) {
        return frame_slot_it->second;
      }
    }
    if (home->offset_bytes.has_value()) {
      return *home->offset_bytes;
    }
  }

  if (function_addressing == nullptr) {
    return std::nullopt;
  }
  const auto* prepared_access = c4c::backend::prepare::find_prepared_memory_access_by_result_name(
      *prepared_names, *function_addressing, value_name);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }

  switch (prepared_access->address.base_kind) {
    case c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot: {
      if (!prepared_access->address.frame_slot_id.has_value()) {
        return std::nullopt;
      }
      const auto frame_slot_it =
          local_layout.frame_slot_offsets.find(*prepared_access->address.frame_slot_id);
      if (frame_slot_it == local_layout.frame_slot_offsets.end()) {
        return std::nullopt;
      }
      const auto signed_offset =
          static_cast<std::int64_t>(frame_slot_it->second) + prepared_access->address.byte_offset;
      if (signed_offset < 0) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(signed_offset);
    }
    case c4c::backend::prepare::PreparedAddressBaseKind::PointerValue: {
      if (!prepared_access->address.pointer_value_name.has_value() ||
          !prepared_access->address.can_use_base_plus_offset) {
        return std::nullopt;
      }
      const auto pointer_name = c4c::backend::prepare::prepared_value_name(
          *prepared_names, *prepared_access->address.pointer_value_name);
      if (pointer_name.empty()) {
        return std::nullopt;
      }
      const auto base_offset = find_prepared_authoritative_named_stack_offset_if_supported(
          local_layout,
          stack_layout,
          function_addressing,
          prepared_names,
          function_locations,
          function_name,
          pointer_name,
          visited_names);
      if (!base_offset.has_value()) {
        return std::nullopt;
      }
      const auto signed_offset =
          static_cast<std::int64_t>(*base_offset) + prepared_access->address.byte_offset;
      if (signed_offset < 0) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(signed_offset);
    }
    default:
      return std::nullopt;
  }
}

inline std::optional<std::size_t>
find_prepared_bounded_multi_defined_named_frame_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  return find_prepared_authoritative_named_stack_offset_if_supported(
      local_layout,
      stack_layout,
      function_addressing,
      &module.names,
      &function_locations,
      function_name,
      value_name);
}

inline bool finalize_prepared_bounded_multi_defined_call_result_if_supported(
    const c4c::backend::bir::CallInst& call,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    std::string* body,
    std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier>* current_i32) {
  if (!call.result.has_value() || call.result->type != c4c::backend::bir::TypeKind::I32 ||
      call.result->kind != c4c::backend::bir::Value::Kind::Named) {
    *current_i32 = std::nullopt;
    return true;
  }

  const auto* result_home = find_prepared_bounded_multi_defined_named_value_home(
      module, function_locations, call.result->name);
  const auto call_result_selection = select_prepared_i32_call_result_abi_if_supported(
      &function_locations, instruction_index, result_home);
  if (!call_result_selection.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  if (call_result_selection->move != nullptr) {
    if (call_result_selection->move->destination_storage_kind !=
        c4c::backend::prepare::PreparedMoveStorageKind::Register) {
      return false;
    }
    if (result_home == nullptr) {
      return false;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        result_home->register_name.has_value()) {
      const auto home_register = narrow_i32_register(*result_home->register_name);
      if (!home_register.has_value()) {
        return false;
      }
      if (*home_register != call_result_selection->abi_register) {
        *body += "    mov " + *home_register + ", " + call_result_selection->abi_register + "\n";
      }
      *current_i32 = PreparedBoundedMultiDefinedCurrentI32Carrier{
          .value_name = call.result->name,
          .register_name = *home_register,
          .stack_operand = std::nullopt,
      };
      return true;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        result_home->offset_bytes.has_value()) {
      const auto stack_operand =
          render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD");
      *body += "    mov " + stack_operand + ", " + call_result_selection->abi_register + "\n";
      *current_i32 = PreparedBoundedMultiDefinedCurrentI32Carrier{
          .value_name = call.result->name,
          .register_name = std::nullopt,
          .stack_operand = stack_operand,
      };
      return true;
    }
    return false;
  }

  *current_i32 = PreparedBoundedMultiDefinedCurrentI32Carrier{
      .value_name = call.result->name,
      .register_name = call_result_selection->abi_register,
      .stack_operand = std::nullopt,
  };
  return true;
}

inline bool finalize_prepared_bounded_multi_defined_return_if_supported(
    const c4c::backend::bir::Value& returned,
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view return_register,
    std::string* body) {
  if (returned.kind != c4c::backend::bir::Value::Kind::Immediate ||
      returned.type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  *body += "    mov " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(returned.immediate)) + "\n";
  *body += "    add rsp, " + std::to_string(local_layout.frame_size + 8) + "\n";
  *body += "    ret\n";
  return true;
}

inline void note_prepared_bounded_multi_defined_name_once(std::vector<std::string>* names,
                                                          std::string_view name) {
  const auto it = std::find(names->begin(), names->end(), std::string(name));
  if (it == names->end()) {
    names->push_back(std::string(name));
  }
}

inline std::optional<PreparedBoundedMultiDefinedNamedI32Source>
select_prepared_bounded_multi_defined_named_i32_source_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier>& current_i32,
    std::string_view value_name) {
  if (current_i32.has_value() && current_i32->value_name == value_name) {
    return PreparedBoundedMultiDefinedNamedI32Source{
        .register_name = current_i32->register_name,
        .stack_operand = current_i32->stack_operand,
        .immediate_i32 = std::nullopt,
    };
  }

  const auto* home = find_prepared_bounded_multi_defined_named_value_home(
      module, function_locations, value_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    return PreparedBoundedMultiDefinedNamedI32Source{
        .register_name = narrow_i32_register(*home->register_name),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    return PreparedBoundedMultiDefinedNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand =
            render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD"),
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    return PreparedBoundedMultiDefinedNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = *home->immediate_i32,
    };
  }
  return std::nullopt;
}

inline bool append_prepared_bounded_multi_defined_i32_move_into_register_if_supported(
    std::string* body,
    std::string_view destination_register,
    const PreparedBoundedMultiDefinedNamedI32Source& source) {
  if (source.immediate_i32.has_value()) {
    *body += "    mov " + std::string(destination_register) + ", " +
             std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
    return true;
  }
  if (source.register_name.has_value()) {
    if (*source.register_name != destination_register) {
      *body += "    mov " + std::string(destination_register) + ", " +
               *source.register_name + "\n";
    }
    return true;
  }
  if (source.stack_operand.has_value()) {
    *body += "    mov " + std::string(destination_register) + ", " +
             *source.stack_operand + "\n";
    return true;
  }
  return false;
}

inline bool append_prepared_bounded_multi_defined_i32_move_into_memory_if_supported(
    std::string* body,
    std::string_view destination_memory,
    const PreparedBoundedMultiDefinedNamedI32Source& source) {
  if (source.immediate_i32.has_value()) {
    *body += "    mov " + std::string(destination_memory) + ", " +
             std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
    return true;
  }
  if (source.register_name.has_value()) {
    *body += "    mov " + std::string(destination_memory) + ", " +
             *source.register_name + "\n";
    return true;
  }
  if (source.stack_operand.has_value()) {
    *body += "    mov eax, " + *source.stack_operand + "\n";
    *body += "    mov " + std::string(destination_memory) + ", eax\n";
    return true;
  }
  return false;
}

inline bool append_prepared_bounded_multi_defined_call_argument_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::size_t arg_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::FunctionNameId function_name,
    const PreparedModuleLocalSlotLayout& local_layout,
    const std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier>& current_i32,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::vector<std::string>* used_string_names,
    std::vector<std::string>* used_same_module_global_names,
    std::string* body) {
  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
  static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

  if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
    if (arg.kind != c4c::backend::bir::Value::Kind::Named || arg.name.empty()) {
      return false;
    }
    if (arg.name.front() == '@') {
      const std::string_view symbol_name(arg.name.data() + 1, arg.name.size() - 1);
      if (has_string_constant(symbol_name)) {
        note_prepared_bounded_multi_defined_name_once(used_string_names, symbol_name);
        *body += "    lea ";
        *body += kArgRegs64[arg_index];
        *body += ", [rip + ";
        *body += render_private_data_label(symbol_name);
        *body += "]\n";
        return true;
      }
      if (!has_same_module_global(symbol_name)) {
        return false;
      }
      note_prepared_bounded_multi_defined_name_once(used_same_module_global_names,
                                                    symbol_name);
      *body += "    lea ";
      *body += kArgRegs64[arg_index];
      *body += ", [rip + ";
      *body += render_asm_symbol_name(symbol_name);
      *body += "]\n";
      return true;
    }

    const auto frame_offset = find_prepared_bounded_multi_defined_named_frame_offset_if_supported(
        local_layout, &module.stack_layout, function_addressing, module, function_locations,
        function_name, arg.name);
    if (frame_offset.has_value()) {
      *body += "    lea ";
      *body += kArgRegs64[arg_index];
      *body += ", ";
      *body += render_prepared_stack_address_expr(*frame_offset + 8);
      *body += "\n";
      return true;
    }

    const auto* home = find_prepared_bounded_multi_defined_named_value_home(
        module, function_locations, arg.name);
    if (home != nullptr &&
        home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home->register_name.has_value()) {
      if (*home->register_name != kArgRegs64[arg_index]) {
        *body += "    mov ";
        *body += kArgRegs64[arg_index];
        *body += ", ";
        *body += *home->register_name;
        *body += "\n";
      }
      return true;
    }
    if (home != nullptr &&
        home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      *body += "    lea ";
      *body += kArgRegs64[arg_index];
      *body += ", ";
      *body += render_prepared_stack_address_expr(*home->offset_bytes);
      *body += "\n";
      return true;
    }
    return false;
  }

  if (arg_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }
  if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
    *body += "    mov ";
    *body += kArgRegs32[arg_index];
    *body += ", ";
    *body += std::to_string(static_cast<std::int32_t>(arg.immediate));
    *body += "\n";
    return true;
  }
  if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
    return false;
  }
  const auto source = select_prepared_bounded_multi_defined_named_i32_source_if_supported(
      module, function_locations, current_i32, arg.name);
  if (!source.has_value()) {
    return false;
  }
  const auto destination_register = select_prepared_i32_call_argument_abi_register_if_supported(
      &function_locations, instruction_index, arg_index);
  if (!destination_register.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  return append_prepared_bounded_multi_defined_i32_move_into_register_if_supported(
      body, *destination_register, *source);
}

inline std::optional<PreparedBoundedMultiDefinedCallLaneRender>
render_prepared_bounded_multi_defined_call_lane_body_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::Function& candidate,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view return_register,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  PreparedBoundedMultiDefinedCallLaneRender rendered{
      .body = "    sub rsp, " + std::to_string(local_layout.frame_size + 8) + "\n",
  };
  std::optional<PreparedBoundedMultiDefinedCurrentI32Carrier> current_i32;
  std::optional<std::string_view> current_i8;

  for (const auto& inst : candidate.blocks.front().insts) {
    if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst)) {
      if (call->args.size() != call->arg_types.size() || call->args.size() > 6 ||
          (call->result.has_value() &&
           call->result->type != c4c::backend::bir::TypeKind::I32 &&
           call->result->type != c4c::backend::bir::TypeKind::Void)) {
        return std::nullopt;
      }
      std::string callee_name;
      if (!call->is_indirect) {
        if (call->callee.empty() || call->callee_value.has_value()) {
          return std::nullopt;
        }
        callee_name = call->callee;
      } else {
        if (!call->callee.empty() || !call->callee_value.has_value() ||
            call->callee_value->kind != c4c::backend::bir::Value::Kind::Named ||
            call->callee_value->type != c4c::backend::bir::TypeKind::Ptr ||
            call->callee_value->name.empty() || call->callee_value->name.front() != '@') {
          return std::nullopt;
        }
        callee_name = std::string(call->callee_value->name.substr(1));
        bool found_same_module_function = false;
        for (const auto* available_function : defined_functions) {
          if (available_function->name == callee_name) {
            found_same_module_function = true;
            break;
          }
        }
        if (!found_same_module_function) {
          return std::nullopt;
        }
      }

      const auto instruction_index =
          static_cast<std::size_t>(&inst - candidate.blocks.front().insts.data());
      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        if (!append_prepared_bounded_multi_defined_call_argument_if_supported(
                call->args[arg_index], call->arg_types[arg_index], arg_index,
                instruction_index, module, function_locations, function_addressing, function_name,
                local_layout, current_i32, has_string_constant, has_same_module_global,
                render_private_data_label, render_asm_symbol_name, &rendered.used_string_names,
                &rendered.used_same_module_global_names, &rendered.body)) {
          return std::nullopt;
        }
      }

      rendered.body += "    xor eax, eax\n";
      rendered.body += "    call ";
      rendered.body += render_asm_symbol_name(callee_name);
      rendered.body += "\n";
      if (!finalize_prepared_bounded_multi_defined_call_result_if_supported(
              *call, instruction_index, module, function_locations, &rendered.body, &current_i32)) {
        return std::nullopt;
      }
      current_i8 = std::nullopt;
      continue;
    }

    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (store->byte_offset != 0 || store->address.has_value()) {
        return std::nullopt;
      }

      if (store->value.type == c4c::backend::bir::TypeKind::I32) {
        const auto memory = render_prepared_local_slot_memory_operand_if_supported(
            local_layout, store->slot_name, 8, "DWORD");
        if (!memory.has_value()) {
          return std::nullopt;
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          rendered.body += "    mov " + *memory + ", " +
                           std::to_string(static_cast<std::int32_t>(store->value.immediate)) +
                           "\n";
          current_i32 = std::nullopt;
          current_i8 = std::nullopt;
          continue;
        }
        if (store->value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto source =
            select_prepared_bounded_multi_defined_named_i32_source_if_supported(
                module, function_locations, current_i32, store->value.name);
        if (!source.has_value() ||
            !append_prepared_bounded_multi_defined_i32_move_into_memory_if_supported(
                &rendered.body, *memory, *source)) {
          return std::nullopt;
        }
        current_i32 = std::nullopt;
        current_i8 = std::nullopt;
        continue;
      }

      if (store->value.type == c4c::backend::bir::TypeKind::I8) {
        const auto memory = render_prepared_local_slot_memory_operand_if_supported(
            local_layout, store->slot_name, 8, "BYTE");
        if (!memory.has_value()) {
          return std::nullopt;
        }
        const auto rendered_store = render_prepared_i8_store_to_memory_if_supported(
            store->value,
            current_i8,
            *memory,
            [](const c4c::backend::bir::Value& value,
               const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
              if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
                return std::to_string(static_cast<std::int8_t>(value.immediate));
              }
              if (value.kind != c4c::backend::bir::Value::Kind::Named ||
                  !current_name.has_value() || *current_name != value.name) {
                return std::nullopt;
              }
              return "al";
            });
        if (!rendered_store.has_value()) {
          return std::nullopt;
        }
        rendered.body += *rendered_store;
        current_i32 = std::nullopt;
        current_i8 = store->value.kind == c4c::backend::bir::Value::Kind::Named
                         ? std::optional<std::string_view>(store->value.name)
                         : std::nullopt;
        continue;
      }

      return std::nullopt;
    }

    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
    if (load == nullptr || load->byte_offset != 0 || load->address.has_value()) {
      return std::nullopt;
    }

    if (load->result.type == c4c::backend::bir::TypeKind::I32) {
      const auto memory = render_prepared_local_slot_memory_operand_if_supported(
          local_layout, load->slot_name, 8, "DWORD");
      if (!memory.has_value()) {
        return std::nullopt;
      }
      rendered.body += "    mov eax, " + *memory + "\n";
      current_i32 = PreparedBoundedMultiDefinedCurrentI32Carrier{
          .value_name = load->result.name,
          .register_name = std::string("eax"),
          .stack_operand = std::nullopt,
      };
      current_i8 = std::nullopt;
      continue;
    }

    if (load->result.type == c4c::backend::bir::TypeKind::I8) {
      const auto memory = render_prepared_local_slot_memory_operand_if_supported(
          local_layout, load->slot_name, 8, "BYTE");
      if (!memory.has_value()) {
        return std::nullopt;
      }
      rendered.body += "    mov al, " + *memory + "\n";
      current_i32 = std::nullopt;
      current_i8 = load->result.name;
      continue;
    }

    return std::nullopt;
  }

  const auto& returned = *candidate.blocks.front().terminator.value;
  if (!finalize_prepared_bounded_multi_defined_return_if_supported(
          returned, local_layout, return_register, &rendered.body)) {
    return std::nullopt;
  }
  return rendered;
}

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::unordered_set<std::string_view>& helper_names,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name);

std::optional<PreparedBoundedSameModuleHelperPrefixRender>
render_prepared_bounded_same_module_helper_prefix_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function& entry_function,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name);

std::optional<std::string>
render_prepared_bounded_multi_defined_call_lane_data_if_supported(
    const PreparedBoundedMultiDefinedCallLaneModuleRender& rendered_module,
    const c4c::backend::bir::Module& module,
    const std::unordered_set<std::string_view>& helper_string_names,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data);

std::optional<std::string> render_prepared_bounded_multi_defined_call_lane_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::unordered_set<std::string_view>& helper_names,
    const std::unordered_set<std::string_view>& helper_string_names,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data);

std::optional<std::string> render_prepared_param_zero_branch_function(
    std::string_view asm_prefix,
    std::string_view function_name,
    std::string_view false_label,
    const char* false_branch_opcode,
    std::string_view compare_setup,
    std::string_view true_body,
    std::string_view false_body,
    std::string_view trailing_data = {});

std::optional<std::string> find_and_render_prepared_param_zero_branch_return_context_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::bir::Param& param,
    std::string_view asm_prefix,
    std::string_view compare_setup,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Block&,
                                                   const c4c::backend::bir::Value&)>&
        render_return);

std::optional<std::string> render_prepared_minimal_compare_branch_entry_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Block&,
                                                   const c4c::backend::bir::Value&)>&
        render_return);

std::optional<std::string>
find_and_render_prepared_materialized_compare_join_function_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::bir::Param& param,
    std::string_view asm_prefix,
    std::string_view compare_setup,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_return,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data);

std::optional<std::string> render_prepared_materialized_compare_join_entry_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_return,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data);

std::optional<std::string> render_prepared_compare_driven_entry_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Block&,
                                                   const c4c::backend::bir::Value&)>&
        render_param_derived_return,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_materialized_compare_join_return);

std::optional<std::string> render_prepared_compare_driven_entry_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Block&,
                                                   const c4c::backend::bir::Value&)>&
        render_param_derived_return,
    const std::function<std::optional<std::string>(
        const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&,
        const c4c::backend::bir::Param&)>& render_materialized_compare_join_return,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data);

std::optional<c4c::backend::prepare::PreparedShortCircuitJoinContext>
find_prepared_short_circuit_join_context_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
    const c4c::backend::bir::Function& function,
    c4c::BlockLabelId source_block_label);

std::optional<ShortCircuitTarget> build_prepared_short_circuit_target(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedShortCircuitTargetLabels& prepared_target,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block);

std::optional<ShortCircuitPlan> build_prepared_short_circuit_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedShortCircuitBranchPlan& prepared_plan,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block);

std::optional<CompareDrivenBranchRenderPlan> build_prepared_short_circuit_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& source_block,
    const c4c::backend::prepare::PreparedShortCircuitJoinContext& join_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<ShortCircuitPlan>(
        const c4c::backend::prepare::PreparedShortCircuitBranchPlan&)>&
        build_short_circuit_plan);

std::optional<CompareDrivenBranchRenderPlan> build_prepared_plain_cond_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::bir::Block& source_block,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block);

std::optional<CompareDrivenBranchRenderPlan> build_prepared_compare_join_entry_render_plan(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& source_block,
    const c4c::backend::prepare::PreparedShortCircuitContinuationLabels& continuation_plan,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<std::optional<ShortCircuitPlan>(
        const c4c::backend::prepare::PreparedShortCircuitBranchPlan&)>&
        build_short_circuit_plan);

std::optional<std::string> render_compare_driven_branch_plan(
    std::string_view function_name,
    std::string_view rendered_body,
    const CompareDrivenBranchRenderPlan& render_plan,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(const ShortCircuitTarget&, bool)>&
        render_short_circuit_target);

std::optional<std::string> render_compare_driven_branch_plan_with_block_renderer(
    std::string_view function_name,
    std::string_view rendered_body,
    const CompareDrivenBranchRenderPlan& render_plan,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block_target);

std::optional<std::string> render_prepared_supported_immediate_binary(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedSupportedImmediateBinary& binary);

std::optional<std::string> render_prepared_materialized_compare_join_return_if_supported(
    std::string_view return_register,
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
        prepared_return_arm,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load);

std::string render_prepared_return_body(std::string_view value_render,
                                        std::string_view trailing_render = {});

std::optional<std::string> render_prepared_loop_join_countdown_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction& function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix);

std::optional<std::string> render_prepared_local_i32_countdown_loop_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const PreparedModuleLocalSlotLayout& layout);

struct SlotAddr {
  enum class Kind : unsigned char {
    Direct,
    Indirect,
    OverAligned,
  };

  Kind kind = Kind::Direct;
  StackSlot slot{};
  std::uint32_t value_id = 0;

  static SlotAddr Direct(StackSlot direct_slot) {
    return SlotAddr{
        .kind = Kind::Direct,
        .slot = direct_slot,
    };
  }

  static SlotAddr Indirect(StackSlot indirect_slot) {
    return SlotAddr{
        .kind = Kind::Indirect,
        .slot = indirect_slot,
    };
  }

  static SlotAddr OverAligned(StackSlot over_aligned_slot, std::uint32_t over_aligned_value_id) {
    return SlotAddr{
        .kind = Kind::OverAligned,
        .slot = over_aligned_slot,
        .value_id = over_aligned_value_id,
    };
  }
};

struct CallAbiConfig {
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

struct CallArgClass {
  enum class Kind : unsigned char {
    Register,
    Stack,
  };

  Kind kind = Kind::Register;

  bool is_stack() const { return kind == Kind::Stack; }
  bool is_register() const { return kind == Kind::Register; }
};

struct IrInstruction {
  enum class Kind : unsigned char {
    Other,
    CallIndirect,
    BinOp,
    UnaryOp,
    Cast,
    Cmp,
    Store,
    AtomicRmw,
  };

  Kind kind = Kind::Other;
  IrType ty = IrType::Void;
  IrType from_ty = IrType::Void;
  IrType to_ty = IrType::Void;

  bool is_call_indirect() const { return kind == Kind::CallIndirect; }
};

struct IrBlock {
  std::vector<IrInstruction> instructions;
};

struct IrParam {
  std::string name;
  IrType type = IrType::Void;
};

struct IrFunction {
  bool is_variadic = false;
  IrType return_type = IrType::Void;
  std::vector<EightbyteClass> ret_eightbyte_classes;
  std::vector<IrParam> params;
  std::vector<IrBlock> blocks;
};

namespace ParamClass {

struct IntReg {
  std::size_t reg_idx = 0;
};

struct FloatReg {
  std::size_t reg_idx = 0;
};

struct StructByValReg {
  std::size_t base_reg_idx = 0;
  std::size_t size = 0;
};

struct StructSseReg {
  std::size_t lo_fp_idx = 0;
  std::optional<std::size_t> hi_fp_idx = std::nullopt;
};

struct StructMixedIntSseReg {
  std::size_t int_reg_idx = 0;
  std::size_t fp_reg_idx = 0;
};

struct StructMixedSseIntReg {
  std::size_t fp_reg_idx = 0;
  std::size_t int_reg_idx = 0;
};

struct StructStack {
  std::int64_t offset = 0;
  std::size_t size = 0;
};

struct LargeStructStack {
  std::int64_t offset = 0;
  std::size_t size = 0;
};

struct StackScalar {
  std::int64_t offset = 0;
};

}  // namespace ParamClass

struct ParamClassInfo {
  using Data = std::variant<ParamClass::IntReg,
                            ParamClass::FloatReg,
                            ParamClass::StructByValReg,
                            ParamClass::StructSseReg,
                            ParamClass::StructMixedIntSseReg,
                            ParamClass::StructMixedSseIntReg,
                            ParamClass::StructStack,
                            ParamClass::LargeStructStack,
                            ParamClass::StackScalar>;

  Data data = ParamClass::StackScalar{};

  std::size_t gp_reg_count() const {
    return std::visit(
        [](const auto& value) -> std::size_t {
          using T = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<T, ParamClass::IntReg>) {
            return 1;
          } else if constexpr (std::is_same_v<T, ParamClass::StructByValReg>) {
            return value.size > 8 ? 2 : 1;
          } else if constexpr (std::is_same_v<T, ParamClass::StructMixedIntSseReg> ||
                               std::is_same_v<T, ParamClass::StructMixedSseIntReg>) {
            return 1;
          } else {
            return 0;
          }
        },
        data);
  }

  bool is_float_reg() const {
    return std::holds_alternative<ParamClass::FloatReg>(data);
  }
};

struct ParamClassification {
  std::vector<ParamClassInfo> classes;
};

struct X86CodegenState;

struct X86CodegenOutput {
  X86CodegenState* owner = nullptr;

  X86CodegenOutput() = default;
  explicit X86CodegenOutput(X86CodegenState* owner_state) : owner(owner_state) {}

  void bind(X86CodegenState* owner_state) { owner = owner_state; }
  void emit_instr_imm_reg(const char* mnemonic, std::int64_t imm, const char* reg);
  void emit_instr_reg_rbp(const char* mnemonic, const char* reg, std::int64_t offset);
  void emit_instr_rbp_reg(const char* mnemonic, std::int64_t offset, const char* reg);
  void emit_instr_rbp(const char* mnemonic, std::int64_t offset);
  void emit_instr_mem_reg(const char* mnemonic,
                          std::int64_t offset,
                          const char* base_reg,
                          const char* dest_reg);
};

struct X86CodegenRegCache {
  std::optional<std::uint32_t> acc_value_id;
  bool acc_known_zero_extended = false;
  bool acc_valid = false;

  void invalidate_all();
  void invalidate_acc();
  void set_acc(std::uint32_t value_id, bool known_zero_extended);
};

struct X86CodegenState {
  X86CodegenOutput out;
  X86CodegenRegCache reg_cache;
  std::unordered_set<std::uint32_t> f128_direct_slots;
  std::vector<std::string> asm_lines;
  std::unordered_set<std::string> got_addr_symbols;
  std::unordered_map<std::uint32_t, StackSlot> slots;
  std::unordered_map<std::uint32_t, std::uint8_t> reg_assignment_indices;
  std::unordered_set<std::uint32_t> allocas;
  std::unordered_map<std::uint32_t, std::size_t> over_aligned_allocas;
  std::unordered_map<std::uint32_t, std::uint32_t> f128_load_sources;
  std::unordered_map<std::uint32_t, std::array<std::uint64_t, 2>> f128_constant_words;
  std::vector<ParamClassInfo> param_classes;
  std::unordered_set<std::size_t> param_pre_stored;
  bool pic_mode = false;
  bool cf_protection_branch = false;
  bool function_return_thunk = false;

  X86CodegenState();
  X86CodegenState(const X86CodegenState& other);
  X86CodegenState& operator=(const X86CodegenState& other);
  X86CodegenState(X86CodegenState&& other) noexcept;
  X86CodegenState& operator=(X86CodegenState&& other) noexcept;
  void rebind_output();

  void emit(const std::string& asm_line);
  std::optional<StackSlot> get_slot(std::uint32_t value_id) const;
  std::optional<std::uint8_t> assigned_reg_index(std::uint32_t value_id) const;
  bool is_alloca(std::uint32_t value_id) const;
  std::optional<SlotAddr> resolve_slot_addr(std::uint32_t value_id) const;
  void track_f128_load(std::uint32_t dest_id, std::uint32_t ptr_id, std::int64_t offset);
  std::optional<std::uint32_t> get_f128_source(std::uint32_t value_id) const;
  std::optional<std::size_t> alloca_over_align(std::uint32_t value_id) const;
  void set_f128_constant_words(std::uint32_t operand_id, std::uint64_t lo, std::uint64_t hi);
  std::optional<std::array<std::uint64_t, 2>> get_f128_constant_words(std::uint32_t operand_id) const;
  void mark_needs_got_for_addr(std::string name);
  bool needs_got_for_addr(std::string_view name) const;
};

struct X86Codegen {
  X86CodegenState state;
  IrType current_return_type = IrType::Void;
  std::vector<EightbyteClass> func_ret_classes;
  std::vector<EightbyteClass> call_ret_classes;
  std::unordered_map<std::string, std::uint8_t> reg_assignments;
  std::vector<std::uint8_t> used_callee_saved;
  bool is_variadic = false;
  bool no_sse = false;
  std::size_t num_named_int_params = 0;
  std::size_t num_named_fp_params = 0;
  std::int64_t num_named_stack_bytes = 0;
  std::int64_t reg_save_area_offset = 0;

  void operand_to_reg(const Operand& op, const char* reg);
  void operand_to_rax(const Operand& op);
  void operand_to_rcx(const Operand& op);
  void operand_to_rax_rdx(const Operand& op);
  void prep_i128_binop(const Operand& lhs, const Operand& rhs);
  std::optional<std::int64_t> const_as_imm32(const Operand& op) const;
  void store_rax_to(const Value& dest);
  void store_rax_rdx_to(const Value& dest);
  const char* reg_for_type(const char* reg, IrType ty) const;
  const char* mov_load_for_type(IrType ty) const;
  const char* mov_store_for_type(IrType ty) const;
  const char* type_suffix(IrType ty) const;
  void emit_x86_atomic_op_loop(IrType ty, const char* op);

  std::int64_t calculate_stack_space_impl(const IrFunction& func);
  std::int64_t aligned_frame_size_impl(std::int64_t raw_space) const;
  void emit_prologue_impl(const IrFunction& func, std::int64_t frame_size);
  void emit_epilogue_impl(std::int64_t frame_size);
  void emit_store_params_impl(const IrFunction& func);
  void emit_param_ref_impl(const Value& dest, std::size_t param_idx, IrType ty);
  void emit_epilogue_and_ret_impl(std::int64_t frame_size);
  const char* store_instr_for_type_impl(IrType ty) const;
  const char* load_instr_for_type_impl(IrType ty) const;

  CallAbiConfig call_abi_config_impl() const;
  std::size_t emit_call_compute_stack_space_impl(const std::vector<CallArgClass>& arg_classes,
                                                 const std::vector<IrType>& arg_types) const;
  std::int64_t emit_call_stack_args_impl(const std::vector<Operand>& args,
                                         const std::vector<CallArgClass>& arg_classes,
                                         const std::vector<IrType>& arg_types,
                                         std::size_t stack_arg_space,
                                         std::size_t fptr_spill,
                                         std::size_t f128_temp_space);
  void emit_call_reg_args_impl(
      const std::vector<Operand>& args,
      const std::vector<CallArgClass>& arg_classes,
      const std::vector<IrType>& arg_types,
      std::int64_t total_sp_adjust,
      std::size_t f128_temp_space,
      std::size_t stack_arg_space,
      const std::vector<std::optional<RiscvFloatClass>>& classes);
  void emit_call_instruction_impl(const std::optional<std::string>& direct_name,
                                  const std::optional<Operand>& func_ptr,
                                  bool indirect,
                                  std::size_t stack_arg_space);
  void emit_call_cleanup_impl(std::size_t stack_arg_space,
                              std::size_t f128_temp_space,
                              bool indirect);
  void set_call_ret_eightbyte_classes_impl(const std::vector<EightbyteClass>& classes);
  void emit_call_store_result_impl(const Value& dest, IrType return_type);
  void emit_call_store_i128_result_impl(const Value& dest);
  void emit_call_move_f32_to_acc_impl();
  void emit_call_move_f64_to_acc_impl();

  void emit_store_impl(const Operand& val, const Value& ptr, IrType ty);
  void emit_load_impl(const Value& dest, const Value& ptr, IrType ty);
  void emit_store_with_const_offset_impl(const Operand& val,
                                         const Value& base,
                                         std::int64_t offset,
                                         IrType ty);
  void emit_load_with_const_offset_impl(const Value& dest,
                                        const Value& base,
                                        std::int64_t offset,
                                        IrType ty);
  void emit_typed_store_to_slot_impl(const char* instr, IrType ty, StackSlot slot);
  void emit_typed_load_from_slot_impl(const char* instr, StackSlot slot);
  void emit_save_acc_impl();
  void emit_load_ptr_from_slot_impl(StackSlot slot, std::uint32_t val_id);
  void emit_typed_store_indirect_impl(const char* instr, IrType ty);
  void emit_typed_load_indirect_impl(const char* instr);
  void emit_add_offset_to_addr_reg_impl(std::int64_t offset);
  void emit_alloca_addr_to(const char* reg, std::uint32_t val_id, std::int64_t offset);
  void emit_slot_addr_to_secondary_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_add_secondary_to_acc_impl();
  void emit_gep_direct_const_impl(StackSlot slot, std::int64_t offset);
  void emit_gep_indirect_const_impl(StackSlot slot, std::int64_t offset, std::uint32_t val_id);
  void emit_gep_add_const_to_acc_impl(std::int64_t offset);
  void emit_add_imm_to_acc_impl(std::int64_t imm);
  void emit_round_up_acc_to_16_impl();
  void emit_sub_sp_by_acc_impl();
  void emit_mov_sp_to_acc_impl();
  void emit_mov_acc_to_sp_impl();
  void emit_align_acc_impl(std::size_t align);
  void emit_memcpy_load_dest_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_memcpy_load_src_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id);
  void emit_alloca_aligned_addr_impl(StackSlot slot, std::uint32_t val_id);
  void emit_alloca_aligned_addr_to_acc_impl(StackSlot slot, std::uint32_t val_id);
  void emit_acc_to_secondary_impl();
  void emit_memcpy_store_dest_from_acc_impl();
  void emit_memcpy_store_src_from_acc_impl();
  void emit_memcpy_impl_impl(std::size_t size);
  void emit_seg_load_impl(const Value& dest, const Value& ptr, IrType ty, AddressSpace seg);
  void emit_seg_load_symbol_impl(const Value& dest,
                                 const std::string& sym,
                                 IrType ty,
                                 AddressSpace seg);
  void emit_seg_store_impl(const Operand& val, const Value& ptr, IrType ty, AddressSpace seg);
  void emit_seg_store_symbol_impl(const Operand& val,
                                  const std::string& sym,
                                  IrType ty,
                                  AddressSpace seg);

  void emit_global_addr_impl(const Value& dest, const std::string& name);
  void emit_tls_global_addr_impl(const Value& dest, const std::string& name);
  void emit_global_addr_absolute_impl(const Value& dest, const std::string& name);
  void emit_global_load_rip_rel_impl(const Value& dest, const std::string& sym, IrType ty);
  void emit_global_store_rip_rel_impl(const Operand& val, const std::string& sym, IrType ty);
  void emit_label_addr_impl(const Value& dest, const std::string& label);
  void emit_store_result_impl(const Value& dest);
  void emit_load_operand_impl(const Operand& op);

  void emit_float_cmp_impl(const Value& dest,
                           IrCmpOp op,
                           const Operand& lhs,
                           const Operand& rhs,
                           IrType ty);
  void emit_f128_cmp_impl(const Value& dest,
                          IrCmpOp op,
                          const Operand& lhs,
                          const Operand& rhs);
  void emit_int_cmp_impl(const Value& dest,
                         IrCmpOp op,
                         const Operand& lhs,
                         const Operand& rhs,
                         IrType ty);
  void emit_fused_cmp_branch_impl(IrCmpOp op,
                                  const Operand& lhs,
                                  const Operand& rhs,
                                  IrType ty,
                                  const std::string& true_label,
                                  const std::string& false_label);
  void emit_fused_cmp_branch_blocks_impl(IrCmpOp op,
                                         const Operand& lhs,
                                         const Operand& rhs,
                                         IrType ty,
                                         BlockId true_block,
                                         BlockId false_block);
  void emit_cond_branch_blocks_impl(const Operand& cond, BlockId true_block, BlockId false_block);
  void emit_select_impl(const Value& dest,
                        const Operand& cond,
                        const Operand& true_val,
                        const Operand& false_val,
                        IrType ty);

  void emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size);
  IrType current_return_type_impl() const;
  void emit_return_f32_to_reg_impl();
  void emit_return_f64_to_reg_impl();
  void emit_return_i128_to_regs_impl();
  void emit_return_f128_to_reg_impl();
  void emit_return_int_to_reg_impl();
  void emit_get_return_f64_second_impl(const Value& dest);
  void emit_set_return_f64_second_impl(const Operand& src);
  void emit_get_return_f32_second_impl(const Value& dest);
  void emit_set_return_f32_second_impl(const Operand& src);
  void emit_get_return_f128_second_impl(const Value& dest);
  void emit_set_return_f128_second_impl(const Operand& src);

  void emit_cast_instrs_impl(IrType from_ty, IrType to_ty);
  void emit_cast_impl(const Value& dest,
                      const Operand& src,
                      IrType from_ty,
                      IrType to_ty);
  void emit_float_neg_impl(IrType ty);
  void emit_int_neg_impl(IrType ty);
  void emit_int_not_impl(IrType ty);
  void emit_int_clz_impl(IrType ty);
  void emit_int_ctz_impl(IrType ty);
  void emit_int_bswap_impl(IrType ty);
  void emit_int_popcount_impl(IrType ty);
  void emit_int_binop_impl(const Value& dest,
                           unsigned op,
                           const Operand& lhs,
                           const Operand& rhs,
                           IrType ty);
  void emit_copy_i128_impl(const Value& dest, const Operand& src);
  void emit_float_binop_impl(const Value& dest,
                             FloatOp op,
                             const Operand& lhs,
                             const Operand& rhs,
                             IrType ty);
  void emit_float_binop_impl_impl(const std::string& mnemonic, IrType ty);
  const char* emit_float_binop_mnemonic_impl(FloatOp op) const;
  void emit_unaryop_impl(const Value& dest, IrUnaryOp op, const Operand& src, IrType ty);

  void emit_i128_prep_binop_impl(const Operand& lhs, const Operand& rhs);
  void emit_i128_add_impl();
  void emit_i128_sub_impl();
  void emit_i128_mul_impl();
  void emit_i128_and_impl();
  void emit_i128_or_impl();
  void emit_i128_xor_impl();
  void emit_i128_shl_impl();
  void emit_i128_lshr_impl();
  void emit_i128_ashr_impl();
  void emit_i128_prep_shift_lhs_impl(const Operand& lhs);
  void emit_i128_shl_const_impl(std::uint32_t amount);
  void emit_i128_lshr_const_impl(std::uint32_t amount);
  void emit_i128_ashr_const_impl(std::uint32_t amount);
  void emit_i128_divrem_call_impl(const std::string& func_name,
                                  const Operand& lhs,
                                  const Operand& rhs);
  void emit_i128_store_result_impl(const Value& dest);
  void emit_i128_to_float_call_impl(const Operand& src, bool from_signed, IrType to_ty);
  void emit_float_to_i128_call_impl(const Operand& src, bool to_signed, IrType from_ty);
  void emit_i128_cmp_eq_impl(bool is_ne);
  void emit_i128_cmp_ordered_impl(IrCmpOp op);
  void emit_i128_cmp_store_result_impl(const Value& dest);
  void emit_load_acc_pair_impl(const Operand& op);
  void emit_store_acc_pair_impl(const Value& dest);
  void emit_sign_extend_acc_high_impl();
  void emit_zero_acc_high_impl();
  void emit_store_pair_to_slot_impl(StackSlot slot);
  void emit_load_pair_from_slot_impl(StackSlot slot);
  void emit_save_acc_pair_impl();
  void emit_store_pair_indirect_impl();
  void emit_load_pair_indirect_impl();
  void emit_i128_neg_impl();
  void emit_i128_not_impl();

  std::optional<std::int64_t> emit_f128_resolve_addr(const SlotAddr& addr,
                                                     std::uint32_t ptr_id,
                                                     std::int64_t offset);
  void emit_f128_fldt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
  void emit_f128_fstpt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset);
  void emit_f128_store_raw_bytes(const SlotAddr& addr,
                                 std::uint32_t ptr_id,
                                 std::int64_t offset,
                                 std::uint64_t lo,
                                 std::uint64_t hi);
  void emit_f128_store_f64_via_x87(const SlotAddr& addr,
                                   std::uint32_t ptr_id,
                                   std::int64_t offset);
  void emit_f128_load_finish(const Value& dest);
  void emit_f128_to_int_from_memory(const SlotAddr& addr, IrType to_ty);
  void emit_f128_st0_to_int(IrType to_ty);
  void emit_cast_instrs_x86(IrType from_ty, IrType to_ty);
  void emit_int_to_f128_cast(IrType from_ty);
  void emit_f128_to_int_cast(IrType to_ty);
  void emit_f128_to_u64_cast();
  void emit_f128_to_f32_cast();
  void emit_fild_to_f64_via_stack();
  void emit_fisttp_from_f64_via_stack();
  void emit_extend_to_rax(IrType ty);
  void emit_sign_extend_to_rax(IrType ty);
  void emit_zero_extend_to_rax(IrType ty);
  void emit_generic_cast(IrType from_ty, IrType to_ty);
  void emit_float_to_unsigned(bool from_f64, bool to_u64, IrType to_ty);
  void emit_int_to_float_conv(bool to_f64);
  void emit_u64_to_float(bool to_f64);
  void emit_f128_load_to_x87(const Operand& operand);

  void emit_atomic_rmw_impl(const Value& dest,
                            AtomicRmwOp op,
                            const Operand& ptr,
                            const Operand& val,
                            IrType ty,
                            AtomicOrdering ordering);
  void emit_atomic_cmpxchg_impl(const Value& dest,
                                const Operand& ptr,
                                const Operand& expected,
                                const Operand& desired,
                                IrType ty,
                                AtomicOrdering success_ordering,
                                AtomicOrdering failure_ordering,
                                bool returns_bool);
  void emit_atomic_load_impl(const Value& dest,
                             const Operand& ptr,
                             IrType ty,
                             AtomicOrdering ordering);
  void emit_atomic_store_impl(const Operand& ptr,
                              const Operand& val,
                              IrType ty,
                              AtomicOrdering ordering);
  void emit_fence_impl(AtomicOrdering ordering);

  void emit_va_arg_impl(const Value& dest, const Value& va_list_ptr, IrType result_ty);
  void emit_va_arg_struct_impl(const Value& dest_ptr, const Value& va_list_ptr, std::size_t size);
  void emit_va_arg_struct_ex_impl(const Value& dest_ptr,
                                  const Value& va_list_ptr,
                                  std::size_t size,
                                  bool from_register_area);
  void emit_va_start_impl(const Value& va_list_ptr);
  void emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr);
  void emit_partial_copy(std::int64_t offset, std::size_t remaining);

  AsmOperandKind classify_constraint(const std::string& constraint);
  void setup_operand_metadata(AsmOperand& op, const Operand& val, bool is_output);
  bool resolve_memory_operand(AsmOperand& op,
                              const Operand& val,
                              const std::vector<std::string>& excluded);
  std::string assign_scratch_reg(const AsmOperandKind& kind,
                                 const std::vector<std::string>& excluded);
  void load_input_to_reg(const AsmOperand& op,
                         const Operand& val,
                         const std::string& constraint);
  void preload_readwrite_output(const AsmOperand& op, const Value& ptr);
  std::string substitute_template_line(
      const std::string& line,
      const std::vector<AsmOperand>& operands,
      const std::vector<std::size_t>& gcc_to_internal,
      const std::vector<IrType>& operand_types,
      const std::vector<std::pair<std::string, BlockId>>& goto_labels);
  void store_output_from_reg(const AsmOperand& op,
                             const Value& ptr,
                             const std::string& constraint,
                             const std::vector<const char*>& all_output_regs);
  void reset_scratch_state();
  static std::string substitute_x86_asm_operands(
      const std::string& line,
      const std::vector<std::string>& op_regs,
      const std::vector<std::optional<std::string>>& op_names,
      const std::vector<bool>& op_is_memory,
      const std::vector<std::string>& op_mem_addrs,
      const std::vector<IrType>& op_types,
      const std::vector<std::size_t>& gcc_to_internal,
      const std::vector<std::pair<std::string, BlockId>>& goto_labels,
      const std::vector<std::optional<std::int64_t>>& op_imm_values,
      const std::vector<std::optional<std::string>>& op_imm_symbols);
  static void emit_operand_with_modifier(
      std::string& result,
      std::size_t idx,
      std::optional<char> modifier,
      const std::vector<std::string>& op_regs,
      const std::vector<bool>& op_is_memory,
      const std::vector<std::string>& op_mem_addrs,
      const std::vector<IrType>& op_types,
      const std::vector<std::optional<std::int64_t>>& op_imm_values,
      const std::vector<std::optional<std::string>>& op_imm_symbols);
  static std::optional<char> default_modifier_for_type(std::optional<IrType> ty);
  static std::string format_x86_reg(const std::string& reg, std::optional<char> modifier);
  static std::string reg_to_32(const std::string& reg);
  static std::string reg_to_64(const std::string& reg);
  static std::string reg_to_16(const std::string& reg);
  static std::string reg_to_8l(const std::string& reg);
  static const char* gcc_cc_to_x86(const std::string& cond);

  void emit_intrinsic_impl(const std::optional<Value>& dest,
                           const IntrinsicOp& intrinsic,
                           const std::vector<Operand>& args);
};

void emit_unaryop_default(X86Codegen& codegen,
                          const Value& dest,
                          IrUnaryOp op,
                          const Operand& src,
                          IrType ty);

struct MinimalGlobalStoreReturnAndEntryReturnSlice {
  std::string global_name;
  std::string helper_name;
  std::string entry_name;
  std::int64_t init_imm = 0;
  std::int64_t store_imm = 0;
  std::int64_t helper_imm = 0;
  std::int64_t entry_imm = 0;
  std::size_t align_bytes = 4;
  bool zero_initializer = false;
};

const char* reg_name_to_32(std::string_view name);
std::string x86_reg_name_to_64(std::string_view reg);
std::string x86_reg_name_to_16(std::string_view reg);
std::string x86_reg_name_to_8l(std::string_view reg);
std::string x86_reg_name_to_8h(std::string_view reg);
std::string x86_format_reg(std::string_view reg, std::optional<char> modifier);
const char* x86_gcc_cc_to_x86(std::string_view cond);
const char* phys_reg_name(c4c::backend::PhysReg reg);
const char* phys_reg_name_32(c4c::backend::PhysReg reg);
const char* x86_alu_mnemonic(c4c::backend::bir::BinaryOpcode op);
std::pair<const char*, const char*> x86_shift_mnemonic(c4c::backend::bir::BinaryOpcode op);
std::vector<c4c::backend::PhysReg> x86_callee_saved_regs();
std::vector<c4c::backend::PhysReg> x86_caller_saved_regs();
std::vector<c4c::backend::PhysReg> x86_prune_caller_saved_regs(bool has_indirect_call,
                                                               bool has_i128_ops,
                                                               bool has_atomic_rmw);
ParamClassification classify_params_full(const IrFunction& func, const CallAbiConfig& config);
std::int64_t named_params_stack_bytes(const std::vector<ParamClassInfo>& classes);
void collect_inline_asm_callee_saved_x86(const IrFunction& func,
                                         std::vector<c4c::backend::PhysReg>& asm_clobbered_regs);
std::vector<c4c::backend::PhysReg> filter_available_regs(
    const std::vector<c4c::backend::PhysReg>& callee_saved,
    const std::vector<c4c::backend::PhysReg>& asm_clobbered);
std::pair<std::unordered_map<std::string, std::uint8_t>,
          std::optional<c4c::backend::LivenessResult>>
run_regalloc_and_merge_clobbers(
    const IrFunction& func,
    const std::vector<c4c::backend::PhysReg>& available_regs,
    const std::vector<c4c::backend::PhysReg>& caller_saved_regs,
    const std::vector<c4c::backend::PhysReg>& asm_clobbered_regs,
    std::unordered_map<std::string, std::uint8_t>& reg_assignments,
    std::vector<std::uint8_t>& used_callee_saved,
    bool allow_inline_asm_regalloc);
std::int64_t calculate_stack_space_common(
    X86CodegenState& state,
    const IrFunction& func,
    std::int64_t initial_space,
    const std::function<std::pair<std::int64_t, std::int64_t>(
        std::int64_t, std::int64_t, std::int64_t)>& alloc_fn,
    const std::unordered_map<std::string, std::uint8_t>& reg_assigned,
    const std::vector<c4c::backend::PhysReg>& callee_saved,
    const std::optional<c4c::backend::LivenessResult>& cached_liveness,
    bool track_param_allocas);
std::optional<c4c::backend::PhysReg> x86_constraint_to_callee_saved(std::string_view constraint);
std::optional<c4c::backend::PhysReg> x86_clobber_name_to_callee_saved(std::string_view name);
const char* x86_arg_reg_name(std::size_t reg_index);
const char* x86_float_arg_reg_name(std::size_t reg_index);
std::int64_t x86_param_stack_base_offset();
std::int64_t x86_param_stack_offset(std::int64_t class_stack_offset);
std::string x86_param_slot_name(std::string_view param_name);
bool x86_param_slot_matches(std::string_view slot_name, std::string_view param_name);
bool x86_allow_struct_split_reg_stack();
const char* x86_param_ref_scalar_load_instr(std::string_view scalar_type);
const char* x86_param_ref_scalar_dest_reg(std::string_view scalar_type);
const char* x86_param_ref_scalar_arg_reg(std::size_t reg_index, std::string_view scalar_type);
const char* x86_param_ref_float_reg_move_instr(std::string_view scalar_type);
const char* x86_param_ref_float_arg_reg(std::size_t reg_index, std::string_view scalar_type);
std::string x86_param_ref_scalar_stack_operand(std::int64_t class_stack_offset,
                                               std::string_view scalar_type);
bool x86_phys_reg_is_callee_saved(c4c::backend::PhysReg reg);
bool x86_param_can_prestore_direct_to_reg(bool has_stack_slot,
                                          std::optional<c4c::backend::PhysReg> assigned_reg,
                                          std::size_t assigned_param_count);
const char* x86_param_prestore_move_instr();
const char* x86_param_prestore_arg_reg(std::size_t reg_index);
const char* x86_param_prestore_dest_reg(c4c::backend::PhysReg reg);
const char* x86_param_prestore_float_move_instr(std::string_view scalar_type);
const char* x86_param_prestore_float_arg_reg(std::size_t reg_index, std::string_view scalar_type);
const char* x86_param_prestore_dest_reg(c4c::backend::PhysReg reg, std::string_view scalar_type);
std::size_t x86_param_struct_reg_qword_count(std::size_t size_bytes);
const char* x86_param_struct_reg_arg_reg(std::size_t base_reg_index, std::size_t qword_index);
std::int64_t x86_param_struct_reg_dest_offset(std::int64_t slot_offset, std::size_t qword_index);
const char* x86_param_struct_sse_arg_reg(std::size_t fp_reg_index);
std::int64_t x86_param_struct_sse_dest_offset(std::int64_t slot_offset, std::size_t qword_index);
const char* x86_param_struct_mixed_int_sse_int_arg_reg(std::size_t int_reg_index);
const char* x86_param_struct_mixed_int_sse_fp_arg_reg(std::size_t fp_reg_index);
std::int64_t x86_param_struct_mixed_int_sse_int_dest_offset(std::int64_t slot_offset);
std::int64_t x86_param_struct_mixed_int_sse_fp_dest_offset(std::int64_t slot_offset);
const char* x86_param_struct_mixed_sse_int_fp_arg_reg(std::size_t fp_reg_index);
const char* x86_param_struct_mixed_sse_int_int_arg_reg(std::size_t int_reg_index);
std::int64_t x86_param_struct_mixed_sse_int_fp_dest_offset(std::int64_t slot_offset);
std::int64_t x86_param_struct_mixed_sse_int_int_dest_offset(std::int64_t slot_offset);
std::size_t x86_param_aggregate_copy_qword_count(std::size_t size_bytes);
std::int64_t x86_param_aggregate_copy_src_offset(std::int64_t class_stack_offset,
                                                 std::size_t qword_index);
std::int64_t x86_param_aggregate_copy_dest_offset(std::int64_t slot_offset,
                                                  std::size_t qword_index);
const char* x86_param_split_reg_stack_arg_reg(std::size_t reg_index);
std::size_t x86_param_split_reg_stack_qword_count(std::size_t size_bytes);
std::int64_t x86_param_split_reg_stack_src_offset(std::int64_t stack_offset,
                                                  std::size_t qword_index);
std::int64_t x86_param_split_reg_stack_dest_offset(std::int64_t slot_offset,
                                                   std::size_t qword_index);
void x86_mark_param_prestored(std::unordered_set<std::size_t>& pre_stored_params,
                              std::size_t param_index);
bool x86_param_is_prestored(const std::unordered_set<std::size_t>& pre_stored_params,
                            std::size_t param_index);
std::int64_t x86_variadic_reg_save_area_size(bool no_sse);
std::int64_t x86_aligned_frame_size(std::int64_t raw_space);
std::int64_t x86_stack_probe_page_size();
bool x86_needs_stack_probe(std::int64_t frame_size);
std::int64_t x86_callee_saved_slot_offset(std::int64_t frame_size, std::size_t save_index);
std::int64_t x86_variadic_gp_save_offset(std::int64_t reg_save_area_base, std::size_t reg_index);
std::int64_t x86_variadic_sse_save_offset(std::int64_t reg_save_area_base, std::size_t reg_index);
std::string decode_llvm_byte_string(std::string_view text);
std::string escape_asm_string(std::string_view raw_bytes);
std::string asm_symbol_name(std::string_view target_triple, std::string_view logical_name);
std::string asm_private_data_label(std::string_view target_triple, std::string_view pool_name);
std::string emit_function_prelude(std::string_view target_triple, std::string_view symbol_name);
std::string emit_global_symbol_prelude(std::string_view target_triple,
                                       std::string_view symbol_name,
                                       std::size_t align_bytes,
                                       bool is_zero_init);
std::string emit_minimal_scalar_global_load_slice_asm(std::string_view target_triple,
                                                      std::string_view function_name,
                                                      std::string_view global_name,
                                                      std::int64_t init_imm,
                                                      std::size_t align_bytes,
                                                      bool zero_initializer);
std::string emit_minimal_extern_scalar_global_load_slice_asm(std::string_view target_triple,
                                                             std::string_view function_name,
                                                             std::string_view global_name);
std::string emit_minimal_extern_global_array_load_slice_asm(std::string_view target_triple,
                                                            std::string_view function_name,
                                                            std::string_view global_name,
                                                            std::int64_t byte_offset);
std::string emit_minimal_scalar_global_store_reload_slice_asm(std::string_view target_triple,
                                                              std::string_view function_name,
                                                              std::string_view global_name,
                                                              std::int64_t init_imm,
                                                              std::int64_t store_imm,
                                                              std::size_t align_bytes);
std::string emit_minimal_global_store_return_and_entry_return_asm(
    std::string_view target_triple,
    const MinimalGlobalStoreReturnAndEntryReturnSlice& slice);
std::string emit_minimal_local_temp_asm(std::string_view target_triple,
                                        std::string_view function_name,
                                        std::int64_t stored_imm);
std::string emit_minimal_constant_branch_return_asm(std::string_view target_triple,
                                                    std::string_view function_name,
                                                    std::int64_t returned_imm);
c4c::backend::RegAllocIntegrationResult run_shared_x86_regalloc(
    const c4c::backend::LivenessInput& liveness_input);

std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::x86
