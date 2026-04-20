#pragma once

#include <algorithm>

#include "../assembler/mod.hpp"
#include "../../../bir/lir_to_bir.hpp"
#include "../../../prealloc/prealloc.hpp"
#include "../../../prealloc/target_register_profile.hpp"

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
struct PhysReg;
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
enum class AtomicOrdering : unsigned {
  Relaxed,
  Acquire,
  Release,
  AcqRel,
  SeqCst,
};
enum class AtomicRmwOp : unsigned {
  Add,
  Sub,
  And,
  Or,
  Xor,
  Nand,
  Xchg,
  TestAndSet,
};
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

struct MaterializedI32Compare {
  std::string_view i1_name;
  std::optional<std::string_view> i32_name;
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  std::string compare_setup;
};

struct ShortCircuitEntryCompareContext {
  const c4c::backend::prepare::PreparedBranchCondition* branch_condition = nullptr;
  std::string compare_setup;
  std::string false_branch_opcode;
};

struct ShortCircuitTarget {
  const c4c::backend::bir::Block* block = nullptr;
  std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels> continuation;
};

struct ShortCircuitPlan {
  ShortCircuitTarget on_compare_true;
  ShortCircuitTarget on_compare_false;
};

struct CompareDrivenBranchRenderPlan {
  ShortCircuitPlan branch_plan;
  std::string compare_setup;
  std::string false_branch_opcode;
};

struct DirectBranchTargets {
  const c4c::backend::bir::Block* true_block = nullptr;
  const c4c::backend::bir::Block* false_block = nullptr;
};

// Active intrinsic inventory carried by the translated x86 intrinsics owner.
enum class IntrinsicOp : std::uint16_t {
  Lfence,
  Mfence,
  Sfence,
  Pause,
  Clflush,
  Movnti,
  Movnti64,
  Movntdq,
  Movntpd,
  Loaddqu,
  Storedqu,
  Pcmpeqb128,
  Pcmpeqd128,
  Psubusb128,
  Psubsb128,
  Por128,
  Pand128,
  Pxor128,
  Pmovmskb128,
  SetEpi8,
  SetEpi32,
  Crc32_8,
  Crc32_16,
  Crc32_32,
  Crc32_64,
  FrameAddress,
  ReturnAddress,
  ThreadPointer,
  SqrtF64,
  SqrtF32,
  FabsF64,
  FabsF32,
  Aesenc128,
  Aesenclast128,
  Aesdec128,
  Aesdeclast128,
  Aesimc128,
  Aeskeygenassist128,
  Pclmulqdq128,
  Pslldqi128,
  Psrldqi128,
  Psllqi128,
  Psrlqi128,
  Pshufd128,
  Loadldi128,
  Paddw128,
  Psubw128,
  Pmulhw128,
  Pmaddwd128,
  Pcmpgtw128,
  Pcmpgtb128,
  Paddd128,
  Psubd128,
  Packssdw128,
  Packsswb128,
  Packuswb128,
  Punpcklbw128,
  Punpckhbw128,
  Punpcklwd128,
  Punpckhwd128,
  Psllwi128,
  Psrlwi128,
  Psrawi128,
  Psradi128,
  Pslldi128,
  Psrldi128,
  SetEpi16,
  Pinsrw128,
  Pextrw128,
  Pinsrd128,
  Pextrd128,
  Pinsrb128,
  Pextrb128,
  Pinsrq128,
  Pextrq128,
  Storeldi128,
  Cvtsi128Si32,
  Cvtsi32Si128,
  Cvtsi128Si64,
  Pshuflw128,
  Pshufhw128,
};
struct AsmOperand;
struct BlockId {
  std::uint32_t raw = 0;
};

struct StackSlot {
  std::int64_t raw = 0;
};

struct PreparedModuleLocalSlotLayout {
  std::unordered_map<std::string_view, std::size_t> offsets;
  std::unordered_map<c4c::backend::prepare::PreparedFrameSlotId, std::size_t>
      frame_slot_offsets;
  std::size_t frame_size = 0;
  std::size_t frame_alignment = 0;
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
  std::unordered_set<std::string_view> helper_global_names;
};

struct PreparedModuleMultiDefinedDispatchState {
  std::string helper_prefix;
  std::unordered_set<std::string_view> helper_names;
  std::unordered_set<std::string_view> helper_global_names;
  std::optional<std::string> rendered_module;
  bool has_bounded_same_module_helpers = false;
};

struct PreparedX86FunctionDispatchContext;

struct PreparedX86BlockDispatchContext {
  const PreparedX86FunctionDispatchContext* function_context = nullptr;
  const c4c::backend::bir::Block* block = nullptr;
  const PreparedModuleLocalSlotLayout* local_layout = nullptr;
  c4c::BlockLabelId block_label_id = c4c::kInvalidBlockLabel;
};

struct PreparedX86FunctionDispatchContext {
  const c4c::backend::bir::Module* module = nullptr;
  const c4c::backend::bir::Function* function = nullptr;
  const c4c::backend::bir::Block* entry = nullptr;
  const c4c::backend::prepare::PreparedStackLayout* stack_layout = nullptr;
  const c4c::backend::prepare::PreparedAddressingFunction* function_addressing = nullptr;
  const c4c::backend::prepare::PreparedNameTables* prepared_names = nullptr;
  const c4c::backend::prepare::PreparedValueLocationFunction* function_locations = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow = nullptr;
  c4c::TargetArch prepared_arch = c4c::TargetArch::Unknown;
  std::string_view asm_prefix;
  std::string_view return_register;
  const std::unordered_set<std::string_view>* bounded_same_module_helper_names = nullptr;
  const std::unordered_set<std::string_view>* bounded_same_module_helper_global_names = nullptr;
  std::function<const c4c::backend::bir::Block*(std::string_view)> find_block;
  std::function<const c4c::backend::bir::StringConstant*(std::string_view)> find_string_constant;
  std::function<const c4c::backend::bir::Global*(std::string_view)> find_same_module_global;
  std::function<bool(const c4c::backend::bir::Global&,
                     c4c::backend::bir::TypeKind,
                     std::size_t)>
      same_module_global_supports_scalar_load;
  std::function<std::string(std::string_view)> render_private_data_label;
  std::function<std::string(std::string_view)> render_asm_symbol_name;
  std::function<std::string(const c4c::backend::bir::StringConstant&)> emit_string_constant_data;
  std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>
      emit_same_module_global_data;
  std::function<std::string(std::string)> prepend_bounded_same_module_helpers;
  std::function<std::optional<std::string>(const c4c::backend::bir::Param&)> minimal_param_register;

  PreparedX86BlockDispatchContext make_block_context(
      const c4c::backend::bir::Block& block,
      const PreparedModuleLocalSlotLayout& local_layout) const {
    const c4c::BlockLabelId block_label_id =
        prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                  : c4c::backend::prepare::resolve_prepared_block_label_id(
                                        *prepared_names, block.label)
                                        .value_or(c4c::kInvalidBlockLabel);
    return PreparedX86BlockDispatchContext{
        .function_context = this,
        .block = &block,
        .local_layout = &local_layout,
        .block_label_id = block_label_id,
    };
  }
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

std::optional<std::string> render_prepared_local_slot_guard_chain_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_local_i32_arithmetic_guard_if_supported(
    const PreparedX86FunctionDispatchContext& context);

std::optional<std::string> render_prepared_local_i16_arithmetic_guard_if_supported(
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

std::optional<std::string> render_prepared_trivial_defined_function_if_supported(
    const c4c::backend::bir::Function& candidate,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix);

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

std::optional<std::string> render_prepared_single_block_return_dispatch_if_supported(
    const PreparedX86FunctionDispatchContext& context);

inline std::optional<PreparedBoundedMultiDefinedCallLaneRender>
render_prepared_bounded_multi_defined_call_lane_body_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const c4c::backend::prepare::PreparedValueLocationFunction& function_locations,
    const c4c::backend::bir::Function& candidate,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view return_register,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
  static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

  PreparedBoundedMultiDefinedCallLaneRender rendered{
      .body = "    sub rsp, " + std::to_string(local_layout.frame_size + 8) + "\n",
  };
  struct CurrentI32Carrier {
    std::string_view value_name;
    std::optional<std::string> register_name;
    std::optional<std::string> stack_operand;
  };
  struct NamedI32Source {
    std::optional<std::string> register_name;
    std::optional<std::string> stack_operand;
    std::optional<std::int64_t> immediate_i32;
  };
  std::optional<CurrentI32Carrier> current_i32;
  const auto note_once = [](std::vector<std::string>* names, std::string_view name) {
    const auto it = std::find(names->begin(), names->end(), std::string(name));
    if (it == names->end()) {
      names->push_back(std::string(name));
    }
  };
  const auto narrow_i32_register = [](std::string_view wide_register) -> std::optional<std::string> {
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
  };
  const auto find_named_value_home =
      [&](std::string_view value_name) -> const c4c::backend::prepare::PreparedValueHome* {
    return c4c::backend::prepare::find_prepared_value_home(module.names, function_locations, value_name);
  };
  const auto source_for_named_i32 = [&](std::string_view value_name) -> std::optional<NamedI32Source> {
    if (current_i32.has_value() && current_i32->value_name == value_name) {
      return NamedI32Source{
          .register_name = current_i32->register_name,
          .stack_operand = current_i32->stack_operand,
          .immediate_i32 = std::nullopt,
      };
    }
    const auto* home = find_named_value_home(value_name);
    if (home == nullptr) {
      return std::nullopt;
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home->register_name.has_value()) {
      return NamedI32Source{
          .register_name = narrow_i32_register(*home->register_name),
          .stack_operand = std::nullopt,
          .immediate_i32 = std::nullopt,
      };
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      return NamedI32Source{
          .register_name = std::nullopt,
          .stack_operand =
              render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD"),
          .immediate_i32 = std::nullopt,
      };
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
        home->immediate_i32.has_value()) {
      return NamedI32Source{
          .register_name = std::nullopt,
          .stack_operand = std::nullopt,
          .immediate_i32 = *home->immediate_i32,
      };
    }
    return std::nullopt;
  };
  const auto append_move_into_register =
      [&](std::string* body,
          std::string_view destination_register,
          const NamedI32Source& source) -> bool {
    if (source.immediate_i32.has_value()) {
      *body += "    mov " + std::string(destination_register) + ", " +
               std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
      return true;
    }
    if (source.register_name.has_value()) {
      if (*source.register_name != destination_register) {
        *body += "    mov " + std::string(destination_register) + ", " + *source.register_name + "\n";
      }
      return true;
    }
    if (source.stack_operand.has_value()) {
      *body += "    mov " + std::string(destination_register) + ", " + *source.stack_operand + "\n";
      return true;
    }
    return false;
  };
  const auto append_move_into_memory =
      [&](std::string* body, std::string_view destination_memory, const NamedI32Source& source) -> bool {
    if (source.immediate_i32.has_value()) {
      *body += "    mov " + std::string(destination_memory) + ", " +
               std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
      return true;
    }
    if (source.register_name.has_value()) {
      *body += "    mov " + std::string(destination_memory) + ", " + *source.register_name + "\n";
      return true;
    }
    if (source.stack_operand.has_value()) {
      *body += "    mov eax, " + *source.stack_operand + "\n";
      *body += "    mov " + std::string(destination_memory) + ", eax\n";
      return true;
    }
    return false;
  };

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
      const auto* before_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
          function_locations, c4c::backend::prepare::PreparedMovePhase::BeforeCall, 0,
          instruction_index);
      for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
        const auto& arg = call->args[arg_index];
        if (call->arg_types[arg_index] == c4c::backend::bir::TypeKind::Ptr) {
          if (arg.kind != c4c::backend::bir::Value::Kind::Named || arg.name.empty() ||
              arg.name.front() != '@') {
            return std::nullopt;
          }
          const std::string_view symbol_name(arg.name.data() + 1, arg.name.size() - 1);
          if (has_string_constant(symbol_name)) {
            note_once(&rendered.used_string_names, symbol_name);
            rendered.body += "    lea ";
            rendered.body += kArgRegs64[arg_index];
            rendered.body += ", [rip + ";
            rendered.body += render_private_data_label(symbol_name);
            rendered.body += "]\n";
            continue;
          }
          if (!has_same_module_global(symbol_name)) {
            return std::nullopt;
          }
          note_once(&rendered.used_same_module_global_names, symbol_name);
          rendered.body += "    lea ";
          rendered.body += kArgRegs64[arg_index];
          rendered.body += ", [rip + ";
          rendered.body += render_asm_symbol_name(symbol_name);
          rendered.body += "]\n";
          continue;
        }

        if (call->arg_types[arg_index] != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
          rendered.body += "    mov ";
          rendered.body += kArgRegs32[arg_index];
          rendered.body += ", ";
          rendered.body += std::to_string(static_cast<std::int32_t>(arg.immediate));
          rendered.body += "\n";
          continue;
        }
        if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto source = source_for_named_i32(arg.name);
        if (!source.has_value()) {
          return std::nullopt;
        }
        std::optional<std::string> destination_register;
        if (before_call_bundle != nullptr) {
          for (const auto& move : before_call_bundle->moves) {
            if (move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
                move.destination_storage_kind != c4c::backend::prepare::PreparedMoveStorageKind::Register ||
                move.destination_abi_index != std::optional<std::size_t>{arg_index} ||
                !move.destination_register_name.has_value()) {
              continue;
            }
            destination_register = narrow_i32_register(*move.destination_register_name);
            break;
          }
        }
        if (!destination_register.has_value()) {
          throw std::invalid_argument(
              "x86 backend emitter requires the authoritative prepared call-bundle handoff through the canonical prepared-module handoff");
        }
        if (!append_move_into_register(&rendered.body, *destination_register, *source)) {
          return std::nullopt;
        }
      }

      rendered.body += "    xor eax, eax\n";
      rendered.body += "    call ";
      rendered.body += render_asm_symbol_name(callee_name);
      rendered.body += "\n";
      if (call->result.has_value() && call->result->type == c4c::backend::bir::TypeKind::I32 &&
          call->result->kind == c4c::backend::bir::Value::Kind::Named) {
        const auto* after_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
            function_locations, c4c::backend::prepare::PreparedMovePhase::AfterCall, 0,
            instruction_index);
        const auto* result_home = find_named_value_home(call->result->name);
        const auto* after_call_move = [&]() -> const c4c::backend::prepare::PreparedMoveResolution* {
          if (after_call_bundle == nullptr) {
            return nullptr;
          }
          if (result_home != nullptr) {
            for (const auto& move : after_call_bundle->moves) {
              if (move.destination_kind ==
                      c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi &&
                  move.to_value_id == result_home->value_id) {
                return &move;
              }
            }
          }
          for (const auto& move : after_call_bundle->moves) {
            if (move.destination_kind ==
                c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi) {
              return &move;
            }
          }
          return nullptr;
        }();
        if (after_call_move == nullptr || !after_call_move->destination_register_name.has_value()) {
          throw std::invalid_argument(
              "x86 backend emitter requires the authoritative prepared call-bundle handoff through the canonical prepared-module handoff");
        }
        const auto abi_result_register =
            narrow_i32_register(*after_call_move->destination_register_name);
        if (!abi_result_register.has_value()) {
          return std::nullopt;
        }
        if (after_call_move != nullptr) {
          if (after_call_move->destination_storage_kind !=
              c4c::backend::prepare::PreparedMoveStorageKind::Register) {
            return std::nullopt;
          }
          if (result_home == nullptr) {
            return std::nullopt;
          }
          if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
              result_home->register_name.has_value()) {
            const auto home_register = narrow_i32_register(*result_home->register_name);
            if (!home_register.has_value()) {
              return std::nullopt;
            }
            if (*home_register != *abi_result_register) {
              rendered.body += "    mov " + *home_register + ", " + *abi_result_register + "\n";
            }
            current_i32 = CurrentI32Carrier{
                .value_name = call->result->name,
                .register_name = *home_register,
                .stack_operand = std::nullopt,
            };
          } else if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                     result_home->offset_bytes.has_value()) {
            const auto stack_operand =
                render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD");
            rendered.body += "    mov " + stack_operand + ", " + *abi_result_register + "\n";
            current_i32 = CurrentI32Carrier{
                .value_name = call->result->name,
                .register_name = std::nullopt,
                .stack_operand = stack_operand,
            };
          } else {
            return std::nullopt;
          }
        } else {
          current_i32 = CurrentI32Carrier{
              .value_name = call->result->name,
              .register_name = *abi_result_register,
              .stack_operand = std::nullopt,
          };
        }
      } else {
        current_i32 = std::nullopt;
      }
      continue;
    }

    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (store->byte_offset != 0 || store->address.has_value() ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
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
        continue;
      }
      if (store->value.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      const auto source = source_for_named_i32(store->value.name);
      if (!source.has_value() || !append_move_into_memory(&rendered.body, *memory, *source)) {
        return std::nullopt;
      }
      current_i32 = std::nullopt;
      continue;
    }

    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
    if (load == nullptr || load->byte_offset != 0 || load->address.has_value() ||
        load->result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    const auto memory =
        render_prepared_local_slot_memory_operand_if_supported(local_layout, load->slot_name, 8,
                                                               "DWORD");
    if (!memory.has_value()) {
      return std::nullopt;
    }
    rendered.body += "    mov eax, " + *memory + "\n";
    current_i32 = CurrentI32Carrier{
        .value_name = load->result.name,
        .register_name = std::string("eax"),
        .stack_operand = std::nullopt,
    };
  }

  const auto& returned = *candidate.blocks.front().terminator.value;
  if (returned.kind != c4c::backend::bir::Value::Kind::Immediate ||
      returned.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  rendered.body += "    mov " + std::string(return_register) + ", " +
                   std::to_string(static_cast<std::int32_t>(returned.immediate)) +
                   "\n    add rsp, " + std::to_string(local_layout.frame_size + 8) + "\n    ret\n";
  return rendered;
}

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
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
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name);

std::optional<std::string>
render_prepared_bounded_multi_defined_call_lane_data_if_supported(
    const PreparedBoundedMultiDefinedCallLaneModuleRender& rendered_module,
    const c4c::backend::bir::Module& module,
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

PreparedModuleMultiDefinedDispatchState build_prepared_module_multi_defined_dispatch_state(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function* entry_function,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
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

std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name);

std::optional<std::pair<std::string, std::string>>
render_prepared_guard_false_branch_compare_from_condition(
    const c4c::backend::prepare::PreparedBranchCondition& condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name);

std::optional<ShortCircuitEntryCompareContext> build_prepared_guard_compare_context(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name);

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

std::optional<std::string> render_prepared_countdown_entry_routes_if_supported(
    const PreparedX86FunctionDispatchContext& context);

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

  void float_operand_to_xmm0(const Operand& op, bool is_f32);
  void emit_nontemporal_store(const IntrinsicOp& op,
                              const Operand& ptr,
                              const Operand& val,
                              std::optional<Value> dest);
  void emit_sse_binary_128(const Value& dest_ptr,
                           const Operand& lhs,
                           const Operand& rhs,
                           const char* mnemonic);
  void emit_sse_unary_imm_128(const Value& dest_ptr,
                              const Operand& src,
                              std::uint8_t imm,
                              const char* mnemonic);
  void emit_sse_shuffle_imm_128(const Value& dest_ptr,
                                const Operand& lhs,
                                const Operand& rhs,
                                std::uint8_t imm,
                                const char* mnemonic);
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
std::string emit_module(const c4c::backend::bir::Module& module);
std::string emit_module(const c4c::codegen::lir::LirModule& module);
assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path);

}  // namespace c4c::backend::x86
