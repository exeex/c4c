#include "calls.hpp"
#include "f128.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

constexpr std::size_t kStackPointerAlignmentBytes = 16;

[[nodiscard]] MachineEffectResource effect_from_operand(const OperandRecord& operand);
[[nodiscard]] bool same_gp_register_index(abi::RegisterReference lhs,
                                          abi::RegisterReference rhs);
[[nodiscard]] bool call_boundary_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t load_width_bytes);
std::vector<std::string> materialize_call_boundary_frame_slot_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& memory);
[[nodiscard]] module::MachineInstruction make_call_boundary_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target);
[[nodiscard]] std::optional<module::MachineInstruction>
make_immediate_cast_call_argument_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t instruction_index);

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const auto remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] std::size_t incoming_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi) {
  return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t incoming_stack_argument_alignment_bytes(
    const bir::CallArgAbiInfo& abi) {
  const std::size_t abi_alignment = abi.align_bytes == 0 ? abi.size_bytes : abi.align_bytes;
  return std::min<std::size_t>(std::max<std::size_t>(abi_alignment, 8), 16);
}

[[nodiscard]] std::size_t outgoing_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi) {
  return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t outgoing_stack_argument_bytes(
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan) {
  std::size_t bytes = 0;
  for (const auto& argument : call_plan.arguments) {
    if (!argument.destination_stack_offset_bytes.has_value() ||
        argument.arg_index >= call.arg_abi.size()) {
      continue;
    }
    bytes = std::max(bytes,
                     *argument.destination_stack_offset_bytes +
                         outgoing_stack_argument_size_bytes(call.arg_abi[argument.arg_index]));
  }
  return align_to(bytes, kStackPointerAlignmentBytes);
}

[[nodiscard]] abi::RegisterReference outgoing_stack_argument_base_register() {
  return abi::x_register(16);
}

[[nodiscard]] bool entry_param_uses_incoming_stack(const bir::Param& param) {
  return param.abi.has_value() && param.abi->passed_on_stack;
}

[[nodiscard]] std::size_t named_incoming_stack_bytes(const bir::Function& function,
                                                     std::size_t named_parameter_count) {
  std::size_t next_offset = 0;
  const std::size_t limit = std::min(named_parameter_count, function.params.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto& param = function.params[index];
    if (!entry_param_uses_incoming_stack(param)) {
      continue;
    }
    next_offset =
        align_to(next_offset, incoming_stack_argument_alignment_bytes(*param.abi));
    next_offset += incoming_stack_argument_size_bytes(*param.abi);
  }
  return next_offset;
}

[[nodiscard]] bool function_has_call(const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::CallInst>(inst)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> fixed_frame_adjustment_bytes(
    const module::FunctionLoweringContext& context) {
  const auto* frame = context.frame_plan;
  const auto* function = context.bir_function;
  if (frame == nullptr || function == nullptr ||
      frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr) {
    return std::nullopt;
  }

  std::size_t frame_alignment =
      std::max<std::size_t>(frame->frame_alignment_bytes, kStackPointerAlignmentBytes);
  std::size_t prepared_frame_size = frame->frame_size_bytes;
  for (const auto& saved : frame->saved_callee_registers) {
    if (!saved.slot_placement.has_value() ||
        !saved.slot_placement->stack_offset_bytes.has_value() ||
        !saved.slot_placement->size_bytes.has_value() ||
        !saved.slot_placement->align_bytes.has_value()) {
      return std::nullopt;
    }
    prepared_frame_size = std::max(
        prepared_frame_size,
        *saved.slot_placement->stack_offset_bytes + *saved.slot_placement->size_bytes);
    frame_alignment = std::max(frame_alignment, *saved.slot_placement->align_bytes);
  }

  std::size_t frame_size = align_to(prepared_frame_size, frame_alignment);
  if (function_has_call(*function)) {
    frame_size = align_to(prepared_frame_size + 16, frame_alignment);
  }
  return frame_size == 0 ? std::optional<std::size_t>{}
                         : std::optional<std::size_t>{frame_size};
}

[[nodiscard]] std::optional<std::size_t> va_start_overflow_area_stack_offset(
    const module::BlockLoweringContext& context,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper) {
  if (variadic_helper !=
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaStart} ||
      variadic_entry_plan == nullptr || context.function.bir_function == nullptr) {
    return std::nullopt;
  }
  const auto frame_size = fixed_frame_adjustment_bytes(context.function);
  if (!frame_size.has_value()) {
    return std::nullopt;
  }
  return *frame_size + named_incoming_stack_bytes(*context.function.bir_function,
                                                 variadic_entry_plan->named_parameter_count);
}

prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return prepare::PreparedRegisterClass::General;
    case prepare::PreparedRegisterBank::Fpr:
      return prepare::PreparedRegisterClass::Float;
    case prepare::PreparedRegisterBank::Vreg:
      return prepare::PreparedRegisterClass::Vector;
    case prepare::PreparedRegisterBank::AggregateAddress:
      return prepare::PreparedRegisterClass::AggregateAddress;
    case prepare::PreparedRegisterBank::None:
      return prepare::PreparedRegisterClass::None;
  }
  return prepare::PreparedRegisterClass::None;
}

std::string_view register_display_name(
    abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view v_names[] = {
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  static constexpr std::string_view q_names[] = {
      "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
      "q8",  "q9",  "q10", "q11", "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::V:
      return v_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::Q:
      return q_names[reg.index];
    case abi::RegisterView::Sp:
      return "sp";
  }
  return {};
}

std::vector<std::string_view> occupied_register_views(
    abi::RegisterReference reg) {
  const auto display_name = register_display_name(reg);
  if (display_name.empty()) {
    return {};
  }
  return {display_name};
}

std::vector<std::string_view> occupied_register_views(
    const std::vector<abi::RegisterReference>& regs) {
  std::vector<std::string_view> views;
  views.reserve(regs.size());
  for (const auto reg : regs) {
    const auto display_name = register_display_name(reg);
    if (display_name.empty()) {
      return {};
    }
    views.push_back(display_name);
  }
  return views;
}

std::optional<abi::RegisterView> prepared_clobber_expected_view(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return abi::RegisterView::X;
    case prepare::PreparedRegisterBank::Fpr:
    case prepare::PreparedRegisterBank::Vreg:
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  return std::nullopt;
}

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
}

[[nodiscard]] const prepare::PreparedCallArgumentPlan* find_prepared_argument_plan(
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveResolution& move) {
  if (!move.destination_abi_index.has_value()) {
    return nullptr;
  }
  for (const auto& argument : call_plan.arguments) {
    if (argument.arg_index == *move.destination_abi_index &&
        ((!argument.source_value_id.has_value() &&
          argument.source_encoding == prepare::PreparedStorageEncodingKind::Immediate) ||
         argument.source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id})) {
      return &argument;
    }
  }
  if (move.reason == "call_arg_byval_aggregate_register_lanes") {
    for (const auto& argument : call_plan.arguments) {
      if (argument.arg_index == *move.destination_abi_index) {
        return &argument;
      }
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedAbiBinding* find_prepared_argument_binding(
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move) {
  for (const auto& binding : bundle.abi_bindings) {
    if (binding.destination_kind == move.destination_kind &&
        binding.destination_storage_kind == move.destination_storage_kind &&
        binding.destination_abi_index == move.destination_abi_index &&
        binding.destination_register_name == move.destination_register_name &&
        binding.destination_register_placement == move.destination_register_placement) {
      return &binding;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedAbiBinding* find_prepared_result_binding(
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move) {
  for (const auto& binding : bundle.abi_bindings) {
    if (binding.destination_kind == move.destination_kind &&
        binding.destination_storage_kind == move.destination_storage_kind &&
        binding.destination_abi_index == move.destination_abi_index &&
        binding.destination_register_name == move.destination_register_name &&
        binding.destination_register_placement == move.destination_register_placement) {
      return &binding;
    }
  }
  return nullptr;
}

[[nodiscard]] bool complete_full_width_f128_carrier(
    const prepare::PreparedF128Carrier* carrier) {
  return carrier != nullptr &&
         carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
         carrier->missing_required_facts.empty() &&
         carrier->total_size_bytes == 16 && carrier->total_align_bytes == 16 &&
         carrier->register_bank == prepare::PreparedRegisterBank::Vreg &&
         carrier->register_class == prepare::PreparedRegisterClass::Vector &&
         carrier->contiguous_width == 1 && carrier->register_name.has_value();
}

[[nodiscard]] bool complete_f128_constant_carrier(
    const prepare::PreparedF128Carrier* carrier) {
  return carrier != nullptr &&
         carrier->source_type == bir::TypeKind::F128 &&
         carrier->kind == prepare::PreparedF128CarrierKind::Missing &&
         carrier->missing_required_facts.empty() &&
         carrier->total_size_bytes == 16 &&
         carrier->total_align_bytes == 16 &&
         carrier->constant_payload.has_value();
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_view_from_register_name(
    const std::optional<std::string>& register_name) {
  if (!register_name.has_value() || register_name->empty()) {
    return std::nullopt;
  }
  switch (register_name->front()) {
    case 's':
      return abi::RegisterView::S;
    case 'd':
      return abi::RegisterView::D;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_from_register_name(
    const std::optional<std::string>& register_name) {
  if (!register_name.has_value() || register_name->empty()) {
    return std::nullopt;
  }
  switch (register_name->front()) {
    case 'w':
      return abi::RegisterView::W;
    case 'x':
      return abi::RegisterView::X;
    case 's':
      return abi::RegisterView::S;
    case 'd':
      return abi::RegisterView::D;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::size_t scalar_size_from_register_view(
    std::optional<abi::RegisterView> view) {
  switch (view.value_or(abi::RegisterView::W)) {
    case abi::RegisterView::D:
    case abi::RegisterView::X:
      return 8U;
    case abi::RegisterView::S:
    case abi::RegisterView::W:
    default:
      return 4U;
  }
}

[[nodiscard]] std::optional<std::string> register_name_with_expected_view(
    const std::optional<std::string>& register_name,
    std::optional<abi::RegisterView> expected_view) {
  if (!register_name.has_value() || !expected_view.has_value()) {
    return register_name;
  }
  const auto parsed = abi::parse_aarch64_register_name(*register_name);
  if (!parsed.has_value() || parsed->view == *expected_view) {
    return register_name;
  }
  if (abi::is_gp_register(*parsed)) {
    if (const auto viewed = abi::gp_register(parsed->index, *expected_view);
        viewed.has_value()) {
      return std::string{abi::register_name(*viewed)};
    }
  }
  if (abi::is_fp_simd_register(*parsed)) {
    if (const auto viewed = abi::fp_simd_register(parsed->index, *expected_view);
        viewed.has_value()) {
      return std::string{abi::register_name(*viewed)};
    }
  }
  return register_name;
}

[[nodiscard]] std::optional<RegisterOperand> make_register_operand_from_prepared_authority(
    const std::optional<std::string>& register_name,
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    const std::optional<prepare::PreparedRegisterBank>& bank,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_registers,
    std::optional<abi::RegisterView> expected_view,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  const auto prepared_class =
      bank.has_value() ? std::optional<prepare::PreparedRegisterClass>{
                             register_class_from_bank(*bank)}
                       : std::nullopt;
  abi::PreparedRegisterConversionResult converted;
  if (register_name.has_value()) {
    converted = abi::convert_prepared_register(
        *register_name, bank, prepared_class, expected_view);
  } else if (placement.has_value()) {
    converted = abi::convert_prepared_register(*placement, prepared_class, expected_view);
  } else {
    return std::nullopt;
  }
  if (!converted.reg.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        converted.error.has_value()
            ? converted.error->message
            : "prepared call-boundary move register could not be converted");
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = role,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = prepared_class.value_or(prepare::PreparedRegisterClass::None),
      .prepared_bank = bank.value_or(prepare::PreparedRegisterBank::None),
      .expected_view = expected_view,
      .contiguous_width = contiguous_width,
      .occupied_register_references = {*converted.reg},
      .occupied_registers =
          occupied_registers.empty()
              ? std::vector<std::string_view>{abi::register_name(*converted.reg)}
              : std::vector<std::string_view>(occupied_registers.begin(),
                                               occupied_registers.end()),
  };
}

[[nodiscard]] std::optional<RegisterOperand>
make_f128_q_register_operand_from_carrier(
    const prepare::PreparedF128Carrier& carrier,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (!carrier.register_name.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*carrier.register_name);
  if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        "AArch64 binary128 call-boundary source carrier is not an FP/SIMD register");
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(parsed->index, abi::RegisterView::Q);
  if (!viewed.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        "AArch64 binary128 call-boundary source carrier could not be re-viewed as q-register");
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *viewed,
      .role = role,
      .value_id = value_id,
      .value_name = value_name,
      .prepared_class = carrier.register_class,
      .prepared_bank = carrier.register_bank,
      .expected_view = abi::RegisterView::Q,
      .contiguous_width = carrier.contiguous_width,
      .occupied_register_references = {*viewed},
      .occupied_registers =
          carrier.occupied_register_names.empty()
              ? std::vector<std::string_view>{abi::register_name(*viewed)}
              : std::vector<std::string_view>(
                    carrier.occupied_register_names.begin(),
                    carrier.occupied_register_names.end()),
  };
}

[[nodiscard]] std::optional<ImmediateOperand> make_scalar_call_argument_immediate(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id) {
  ImmediateKind immediate_kind = ImmediateKind::SignedInteger;
  if (value.type == bir::TypeKind::I1) {
    immediate_kind = ImmediateKind::Boolean;
  }

  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return ImmediateOperand{
          .kind = immediate_kind,
          .type = value.type,
          .signed_value = value.immediate,
          .unsigned_value = value.immediate_bits != 0U
                                ? value.immediate_bits
                                : static_cast<std::uint64_t>(value.immediate),
          .source_value_id = source_value_id,
      };
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view_from_size(
    std::size_t size_bytes) {
  if (size_bytes > 0 && size_bytes <= 4) {
    return abi::RegisterView::W;
  }
  if (size_bytes == 8) {
    return abi::RegisterView::X;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<bir::TypeKind> scalar_integer_type_from_size(
    std::size_t size_bytes) {
  switch (size_bytes) {
    case 1:
      return bir::TypeKind::I8;
    case 4:
      return bir::TypeKind::I32;
    case 8:
      return bir::TypeKind::I64;
  }
  return std::nullopt;
}

[[nodiscard]] std::string stack_copy_address(std::string_view base,
                                             std::int64_t offset) {
  std::ostringstream out;
  out << "[" << base;
  if (offset != 0) {
    out << ", #" << offset;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::string_view aggregate_stack_copy_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

[[nodiscard]] std::string_view aggregate_stack_copy_store_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "strb";
    case 4:
    case 8:
      return "str";
  }
  return {};
}

[[nodiscard]] std::optional<abi::RegisterReference> aggregate_stack_copy_scratch(
    std::size_t width_bytes) {
  const auto scratch = abi::reserved_mir_scratch_gp_registers().front();
  if (width_bytes == 1 || width_bytes == 4) {
    return abi::w_register(scratch.index);
  }
  if (width_bytes == 8) {
    return abi::x_register(scratch.index);
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> aggregate_stack_copy_chunks(
    std::size_t size_bytes) {
  std::vector<std::size_t> chunks;
  std::size_t remaining = size_bytes;
  while (remaining >= 8) {
    chunks.push_back(8);
    remaining -= 8;
  }
  if (remaining >= 4) {
    chunks.push_back(4);
    remaining -= 4;
  }
  while (remaining > 0) {
    chunks.push_back(1);
    --remaining;
  }
  return chunks;
}

[[nodiscard]] std::string_view aggregate_register_lane_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return "ldrb";
    case 2:
      return "ldrh";
    case 4:
    case 8:
      return "ldr";
  }
  return {};
}

[[nodiscard]] abi::RegisterReference aggregate_register_lane_load_register(
    abi::RegisterReference reg,
    std::size_t width_bytes) {
  return width_bytes == 8 ? abi::x_register(reg.index) : abi::w_register(reg.index);
}

[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_scratch(
    const RegisterOperand& destination) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (same_gp_register_index(scratch, destination.reg)) {
      continue;
    }
    bool aliases_occupied = false;
    for (const auto occupied : destination.occupied_register_references) {
      if (same_gp_register_index(scratch, occupied)) {
        aliases_occupied = true;
        break;
      }
    }
    if (!aliases_occupied) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] MemoryOperand aggregate_register_lane_memory(
    MemoryOperand memory,
    std::size_t byte_offset,
    std::size_t width_bytes) {
  memory.byte_offset += static_cast<std::int64_t>(byte_offset);
  memory.size_bytes = width_bytes;
  memory.align_bytes = std::min<std::size_t>(memory.align_bytes, width_bytes);
  return memory;
}

[[nodiscard]] bool aggregate_register_lane_memory_is_printable(
    const MemoryOperand& memory,
    std::size_t width_bytes) {
  if (memory.base_kind == MemoryBaseKind::PointerValue && memory.base_register.has_value()) {
    return !memory_address(memory).empty();
  }
  if (memory.base_kind == MemoryBaseKind::Register && memory.base_register.has_value()) {
    return !memory_address(memory).empty();
  }
  return memory.base_kind == MemoryBaseKind::FrameSlot &&
         call_boundary_frame_slot_direct_offset_is_encodable(memory, width_bytes) &&
         !memory_address(memory).empty();
}

[[nodiscard]] std::optional<std::size_t> aggregate_register_lane_printable_chunk(
    const MemoryOperand& memory,
    std::size_t source_offset,
    std::size_t remaining) {
  static constexpr std::size_t kCandidateChunks[] = {8, 4, 2, 1};
  for (const std::size_t chunk_width : kCandidateChunks) {
    if (chunk_width > remaining) {
      continue;
    }
    const auto chunk_memory =
        aggregate_register_lane_memory(memory, source_offset, chunk_width);
    if (aggregate_register_lane_memory_is_printable(chunk_memory, chunk_width) &&
        !aggregate_register_lane_load_mnemonic(chunk_width).empty()) {
      return chunk_width;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_destination(
    const RegisterOperand& destination,
    std::size_t lane_index) {
  if (lane_index < destination.occupied_register_references.size()) {
    return abi::x_register(destination.occupied_register_references[lane_index].index);
  }
  if (destination.reg.index + lane_index > 30) {
    return std::nullopt;
  }
  return abi::x_register(static_cast<std::uint8_t>(destination.reg.index + lane_index));
}

[[nodiscard]] bool is_aggregate_register_lane_publication(
    const CallBoundaryMoveInstructionRecord& move) {
  return move.phase == prepare::PreparedMovePhase::BeforeCall &&
           move.move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
           move.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
           move.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.move.reason == "call_arg_byval_aggregate_register_lanes" &&
           move.source_memory.has_value() &&
         !move.source_memory_materializes_address &&
         move.source_memory->support == MemoryOperandSupportKind::Prepared &&
         move.source_memory->size_bytes > 0 &&
           move.source_memory->size_bytes <= 16 &&
           move.destination_register.has_value() &&
           move.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
         move.destination_register->expected_view == abi::RegisterView::X &&
           abi::is_gp_register(move.destination_register->reg);
}

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot_by_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      source_home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }

  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedMemoryAccess* source_access = nullptr;
  for (const auto& access : addressing->accesses) {
    if (access.result_value_name == std::optional<c4c::ValueNameId>{source_home.value_name}) {
      if (source_access != nullptr) {
        return std::nullopt;
      }
      source_access = &access;
    }
  }
  if (source_access == nullptr) {
    const auto source_offset = source_home.offset_bytes.has_value()
                                   ? source_home.offset_bytes
                                   : argument.source_stack_offset_bytes;
    if (!source_offset.has_value()) {
      return std::nullopt;
    }
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow->function_name,
        .block_label = context.control_flow_block->block_label,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = source_home.value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = source_home.slot_id.has_value() ? source_home.slot_id
                                                         : argument.source_slot_id,
        .byte_offset = static_cast<std::int64_t>(*source_offset),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_home.size_bytes.value_or(4),
        .align_bytes = source_home.align_bytes.value_or(4),
        .can_use_base_plus_offset = true,
    };
  }
  if (source_access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !source_access->address.frame_slot_id.has_value()) {
    const auto source_offset = source_home.offset_bytes.has_value()
                                   ? source_home.offset_bytes
                                   : argument.source_stack_offset_bytes;
    if (source_offset.has_value()) {
      return MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = argument.source_value_id,
          .result_value_name = source_home.value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home.slot_id.has_value() ? source_home.slot_id
                                                           : argument.source_slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_offset),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home.size_bytes.value_or(4),
          .align_bytes = source_home.align_bytes.value_or(4),
          .can_use_base_plus_offset = true,
      };
    }
    return std::nullopt;
  }

  const auto* slot = find_frame_slot_by_id(context.function.prepared->stack_layout,
                                           *source_access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }

  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = source_access->function_name,
      .block_label = source_access->block_label,
      .instruction_index = source_access->inst_index,
      .result_value_id = argument.source_value_id,
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = source_access->address.frame_slot_id,
      .byte_offset = static_cast<std::int64_t>(slot->offset_bytes) +
                     source_access->address.byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = source_access->address.size_bytes,
      .align_bytes = source_access->address.align_bytes,
      .address_space = source_access->address_space,
      .is_volatile = source_access->is_volatile,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] bool is_aarch64_byval_register_lane_move(
    const prepare::PreparedMoveResolution& move) {
  return move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
         (move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register ||
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) &&
         move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.reason == "call_arg_byval_aggregate_register_lanes";
}

[[nodiscard]] std::optional<std::size_t> byval_register_lane_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index) {
  if (!is_aarch64_byval_register_lane_move(move) ||
      context.bir_block == nullptr ||
      !move.destination_abi_index.has_value() ||
      instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto* call = std::get_if<bir::CallInst>(
      &context.bir_block->insts[instruction_index]);
  if (call == nullptr || *move.destination_abi_index >= call->arg_abi.size()) {
    return std::nullopt;
  }
  const auto& arg_abi = call->arg_abi[*move.destination_abi_index];
  if (arg_abi.type != bir::TypeKind::Ptr || !arg_abi.byval_copy ||
      arg_abi.sret_pointer ||
      (!arg_abi.passed_in_register && !arg_abi.passed_on_stack) ||
      arg_abi.primary_class != bir::AbiValueClass::Integer ||
      arg_abi.size_bytes == 0 || arg_abi.size_bytes > 16) {
    return std::nullopt;
  }
  return arg_abi.size_bytes;
}

[[nodiscard]] std::optional<std::size_t> aggregate_slot_source_byte_offset(
    std::string_view slot_name,
    std::string_view source_name) {
  if (source_name.empty() || slot_name.size() <= source_name.size() + 1 ||
      slot_name.compare(0, source_name.size(), source_name) != 0 ||
      slot_name[source_name.size()] != '.') {
    return std::nullopt;
  }
  const auto suffix = slot_name.substr(source_name.size() + 1);
  std::size_t byte_offset = 0;
  const auto* begin = suffix.data();
  const auto* end = begin + suffix.size();
  const auto [ptr, ec] = std::from_chars(begin, end, byte_offset);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }
  return byte_offset;
}

struct AggregateRegisterLaneStore {
  std::size_t source_offset = 0;
  std::int64_t stack_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 1;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
};

struct AggregateRegisterLaneLoadSource {
  std::string value_name;
  std::string slot_name;
};

struct AggregateRegisterLanePartialStore {
  std::string slot_name;
  std::size_t byte_offset = 0;
  AggregateRegisterLaneStore store;
};

[[nodiscard]] std::optional<std::string_view> find_aggregate_lane_load_source_slot(
    const std::vector<AggregateRegisterLaneLoadSource>& load_sources,
    std::string_view value_name) {
  for (auto it = load_sources.rbegin(); it != load_sources.rend(); ++it) {
    if (it->value_name == value_name) {
      return std::string_view{it->slot_name};
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool append_aggregate_lane_partial_source_stores(
    std::string_view slot_name,
    std::size_t source_offset,
    std::size_t source_size_bytes,
    std::optional<std::size_t> source_home_offset,
    const std::vector<AggregateRegisterLanePartialStore>& partial_stores,
    std::vector<AggregateRegisterLaneStore>* stores) {
  bool appended = false;
  for (const auto& partial : partial_stores) {
    if (partial.slot_name != slot_name || partial.byte_offset >= source_size_bytes) {
      continue;
    }
    auto adjusted = partial.store;
    adjusted.source_offset = source_offset + partial.byte_offset;
    adjusted.size_bytes = std::min(adjusted.size_bytes,
                                   source_size_bytes - partial.byte_offset);
    if (!source_home_offset.has_value() || adjusted.stack_offset < 0 ||
        static_cast<std::size_t>(adjusted.stack_offset) < *source_home_offset) {
      continue;
    }
    stores->push_back(std::move(adjusted));
    appended = true;
  }
  return appended;
}

[[nodiscard]] std::vector<AggregateRegisterLaneStore>
collect_byval_register_lane_stores(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  std::vector<AggregateRegisterLaneStore> stores;
  std::vector<AggregateRegisterLaneLoadSource> load_sources;
  std::vector<AggregateRegisterLanePartialStore> partial_stores;
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      source_home.value_name == c4c::kInvalidValueName) {
    return stores;
  }
  const auto source_name =
      prepare::prepared_value_name(context.function.prepared->names,
                                   source_home.value_name);
  if (source_name.empty()) {
    return stores;
  }
  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return stores;
  }

  for (std::size_t index = 0;
       index < instruction_index && index < context.bir_block->insts.size();
       ++index) {
    if (const auto* load = std::get_if<bir::LoadLocalInst>(
            &context.bir_block->insts[index]);
        load != nullptr && load->result.kind == bir::Value::Kind::Named &&
        !load->slot_name.empty()) {
      load_sources.push_back(AggregateRegisterLaneLoadSource{
          .value_name = load->result.name,
          .slot_name = load->slot_name,
      });
      continue;
    }

    const auto* store = std::get_if<bir::StoreLocalInst>(
        &context.bir_block->insts[index]);
    if (store == nullptr) {
      continue;
    }
    const auto* access =
        prepare::find_prepared_memory_access(*addressing,
                                             context.control_flow_block->block_label,
                                             index);
    if (access == nullptr ||
        access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
        !access->address.frame_slot_id.has_value() ||
        access->address.size_bytes == 0) {
      continue;
    }
    if (store->value.kind == bir::Value::Kind::Named) {
      const auto stored_value_name =
          context.function.prepared->names.value_names.find(store->value.name);
      if (stored_value_name != c4c::kInvalidValueName &&
          access->stored_value_name != std::optional<c4c::ValueNameId>{stored_value_name}) {
        continue;
      }
    }
    const auto* slot =
        find_frame_slot_by_id(context.function.prepared->stack_layout,
                              *access->address.frame_slot_id);
    if (slot == nullptr) {
      continue;
    }
    AggregateRegisterLaneStore lane_store{
        .stack_offset =
            static_cast<std::int64_t>(slot->offset_bytes) +
            access->address.byte_offset,
        .size_bytes = access->address.size_bytes,
        .align_bytes = access->address.align_bytes == 0
                           ? std::size_t{1}
                           : access->address.align_bytes,
        .frame_slot_id = access->address.frame_slot_id,
    };

    if (store->address.has_value() &&
        store->address->base_kind == bir::MemoryAddress::BaseKind::LocalSlot &&
        store->address->byte_offset >= 0 && !store->address->base_name.empty()) {
      auto partial_store = lane_store;
      if (context.function.value_locations != nullptr &&
          source_home.offset_bytes.has_value() &&
          store->value.kind == bir::Value::Kind::Named) {
        const auto stored_value_name =
            context.function.prepared->names.value_names.find(store->value.name);
        const auto* stored_home =
            stored_value_name == c4c::kInvalidValueName
                ? nullptr
                : prepare::find_prepared_value_home(*context.function.value_locations,
                                                    stored_value_name);
        if (stored_home != nullptr &&
            stored_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
            stored_home->offset_bytes.has_value() && stored_home->slot_id.has_value() &&
            *stored_home->offset_bytes >= *source_home.offset_bytes) {
          partial_store.stack_offset = static_cast<std::int64_t>(*stored_home->offset_bytes);
          partial_store.size_bytes = stored_home->size_bytes.value_or(access->address.size_bytes);
          partial_store.align_bytes =
              stored_home->align_bytes.value_or(access->address.align_bytes == 0
                                                    ? std::size_t{1}
                                                    : access->address.align_bytes);
          partial_store.frame_slot_id = stored_home->slot_id;
        }
      }
      partial_stores.push_back(AggregateRegisterLanePartialStore{
          .slot_name = store->address->base_name,
          .byte_offset = static_cast<std::size_t>(store->address->byte_offset),
          .store = partial_store,
      });
    }

    const auto source_offset =
        aggregate_slot_source_byte_offset(store->slot_name, source_name);
    if (!source_offset.has_value()) {
      continue;
    }
    lane_store.source_offset = *source_offset;

    if (store->value.kind == bir::Value::Kind::Named) {
      if (const auto load_source_slot =
              find_aggregate_lane_load_source_slot(load_sources, store->value.name);
          load_source_slot.has_value() &&
          append_aggregate_lane_partial_source_stores(*load_source_slot,
                                                      *source_offset,
                                                      access->address.size_bytes,
                                                      source_home.offset_bytes,
                                                      partial_stores,
                                                      &stores)) {
        continue;
      }
    }
    stores.push_back(std::move(lane_store));
  }
  std::sort(stores.begin(), stores.end(),
            [](const AggregateRegisterLaneStore& lhs,
               const AggregateRegisterLaneStore& rhs) {
              return lhs.source_offset < rhs.source_offset;
            });
  return stores;
}

[[nodiscard]] std::optional<MemoryOperand>
make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index) {
  if (size_bytes == 0) {
    return std::nullopt;
  }
  const auto stores =
      collect_byval_register_lane_stores(context, source_home, instruction_index);
  if (stores.empty()) {
    return std::nullopt;
  }

  if (stores.front().source_offset != 0 || stores.front().stack_offset < 0) {
    return std::nullopt;
  }
  const auto first_offset = stores.front().stack_offset;
  std::size_t covered_bytes = 0;
  std::size_t source_align = stores.front().align_bytes;
  for (const auto& store : stores) {
    if (store.stack_offset < 0 ||
        store.source_offset > covered_bytes ||
        static_cast<std::int64_t>(store.source_offset) !=
            store.stack_offset - first_offset) {
      return std::nullopt;
    }
    covered_bytes =
        std::max(covered_bytes, store.source_offset + store.size_bytes);
    source_align = std::min(source_align, store.align_bytes);
    if (covered_bytes >= size_bytes) {
      break;
    }
  }
  if (covered_bytes < size_bytes || !stores.front().frame_slot_id.has_value()) {
    return std::nullopt;
  }

  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{source_home.value_id},
      .result_value_name = std::nullopt,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = stores.front().frame_slot_id,
      .byte_offset = first_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = source_align,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> aggregate_lane_store_memory(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const AggregateRegisterLaneStore& store,
    std::size_t byte_delta,
    std::size_t width_bytes,
    std::size_t instruction_index) {
  if (!store.frame_slot_id.has_value() ||
      store.stack_offset < 0 ||
      width_bytes == 0 ||
      byte_delta >
          static_cast<std::size_t>(
              std::numeric_limits<std::int64_t>::max() - store.stack_offset)) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{
                                   source_home.value_id},
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = store.frame_slot_id,
      .byte_offset = store.stack_offset + static_cast<std::int64_t>(byte_delta),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = width_bytes,
      .align_bytes = std::min<std::size_t>(store.align_bytes, width_bytes),
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<std::size_t> aarch64_indirect_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto* call =
      std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
  if (call == nullptr || argument.arg_index >= call->arg_abi.size()) {
    return std::nullopt;
  }
  const auto& abi = call->arg_abi[argument.arg_index];
  if (abi.type != bir::TypeKind::Ptr ||
      !abi.byval_copy ||
      abi.sret_pointer ||
      !abi.passed_in_register ||
      abi.passed_on_stack ||
      abi.primary_class != bir::AbiValueClass::Integer ||
      abi.size_bytes <= 16) {
    return std::nullopt;
  }
  return abi.size_bytes;
}

[[nodiscard]] std::optional<std::size_t> aarch64_stack_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto* call =
      std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
  if (call == nullptr || argument.arg_index >= call->arg_abi.size()) {
    return std::nullopt;
  }
  const auto& abi = call->arg_abi[argument.arg_index];
  if (abi.type != bir::TypeKind::Ptr ||
      !abi.byval_copy ||
      abi.sret_pointer ||
      !abi.passed_on_stack ||
      abi.size_bytes == 0) {
    return std::nullopt;
  }
  return abi.size_bytes;
}

[[nodiscard]] std::optional<std::size_t> aarch64_register_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  const auto* call =
      std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
  if (call == nullptr || argument.arg_index >= call->arg_abi.size()) {
    return std::nullopt;
  }
  const auto& abi = call->arg_abi[argument.arg_index];
  if (abi.type != bir::TypeKind::Ptr ||
      !abi.byval_copy ||
      abi.sret_pointer ||
      !abi.passed_in_register ||
      abi.passed_on_stack ||
      abi.primary_class != bir::AbiValueClass::Integer ||
      abi.size_bytes == 0 ||
      abi.size_bytes > 16) {
    return std::nullopt;
  }
  return abi.size_bytes;
}

[[nodiscard]] std::optional<MemoryOperand> make_sret_memory_return_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (!call_plan.memory_return.has_value() ||
      !call_plan.memory_return->sret_arg_index.has_value() ||
      *call_plan.memory_return->sret_arg_index != argument.arg_index ||
      call_plan.memory_return->encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !call_plan.memory_return->slot_id.has_value() ||
      !call_plan.memory_return->stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id,
      .result_value_name = std::nullopt,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = call_plan.memory_return->slot_id,
      .byte_offset = static_cast<std::int64_t>(*call_plan.memory_return->stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = call_plan.memory_return->size_bytes,
      .align_bytes = call_plan.memory_return->align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_call_argument_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      source_home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto* addressing = prepare::find_prepared_addressing(
      *context.function.prepared, context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }
  const prepare::PreparedAddressMaterialization* selected = nullptr;
  for (const auto& materialization : addressing->address_materializations) {
    if (materialization.block_label == context.control_flow_block->block_label &&
        materialization.inst_index <= instruction_index &&
        materialization.kind == prepare::PreparedAddressMaterializationKind::FrameSlot &&
        materialization.result_value_name == source_home.value_name &&
        materialization.frame_slot_id.has_value()) {
      if (selected != nullptr && selected->inst_index == materialization.inst_index) {
        return std::nullopt;
      }
      if (selected != nullptr && selected->inst_index > materialization.inst_index) {
        continue;
      }
      selected = &materialization;
    }
  }
  if (selected == nullptr) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = selected->function_name,
      .block_label = selected->block_label,
      .instruction_index = selected->inst_index,
      .result_value_id = argument.source_value_id,
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = selected->frame_slot_id,
      .byte_offset = selected->byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = source_home.size_bytes.value_or(8),
      .align_bytes = source_home.align_bytes.value_or(8),
      .address_space = selected->address_space,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] bool call_argument_is_pointer(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size()) {
    return false;
  }
  const auto* call =
      std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
  if (call == nullptr || argument.arg_index >= call->args.size()) {
    return false;
  }
  if (argument.arg_index < call->arg_types.size() &&
      call->arg_types[argument.arg_index] == bir::TypeKind::Ptr) {
    return true;
  }
  return call->args[argument.arg_index].type == bir::TypeKind::Ptr;
}

[[nodiscard]] bool local_frame_address_name_matches(std::string_view source_name,
                                                    std::string_view candidate_name) {
  return candidate_name == source_name ||
         (candidate_name.size() == source_name.size() + 2 &&
          candidate_name.compare(0, source_name.size(), source_name) == 0 &&
          candidate_name.substr(source_name.size()) == ".0");
}

[[nodiscard]] std::optional<MemoryOperand> make_local_frame_address_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source_home.value_name == c4c::kInvalidValueName ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::Register ||
      !call_argument_is_pointer(context, argument, instruction_index)) {
    return std::nullopt;
  }

  const auto source_name =
      prepare::prepared_value_name(context.function.prepared->names,
                                   source_home.value_name);
  if (source_name.empty()) {
    return std::nullopt;
  }

  const prepare::PreparedAddressMaterialization* selected = nullptr;
  if (const auto* addressing = prepare::find_prepared_addressing(
          *context.function.prepared, context.function.control_flow->function_name);
      addressing != nullptr) {
    for (const auto& materialization : addressing->address_materializations) {
      if (materialization.block_label != context.control_flow_block->block_label ||
          materialization.inst_index > instruction_index ||
          materialization.kind != prepare::PreparedAddressMaterializationKind::FrameSlot ||
          !materialization.frame_slot_id.has_value() ||
          !materialization.result_value_name.has_value()) {
        continue;
      }
      const auto materialized_name =
          prepare::prepared_value_name(context.function.prepared->names,
                                       *materialization.result_value_name);
      if (!local_frame_address_name_matches(source_name, materialized_name)) {
        continue;
      }
      if (selected != nullptr && selected->inst_index == materialization.inst_index) {
        return std::nullopt;
      }
      if (selected == nullptr || selected->inst_index < materialization.inst_index) {
        selected = &materialization;
      }
    }
  }
  if (selected != nullptr) {
    return MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = selected->function_name,
        .block_label = selected->block_label,
        .instruction_index = selected->inst_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = source_home.value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = selected->frame_slot_id,
        .byte_offset = selected->byte_offset,
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_home.size_bytes.value_or(8),
        .align_bytes = source_home.align_bytes.value_or(8),
        .address_space = selected->address_space,
        .can_use_base_plus_offset = true,
    };
  }

  const prepare::PreparedStackObject* selected_object = nullptr;
  const prepare::PreparedFrameSlot* selected_slot = nullptr;
  for (const auto& object : context.function.prepared->stack_layout.objects) {
    if (object.function_name != context.function.control_flow->function_name ||
        object.source_kind != "local_slot") {
      continue;
    }
    const auto object_name =
        prepare::prepared_stack_object_name(context.function.prepared->names, object);
    if (!local_frame_address_name_matches(source_name, object_name)) {
      continue;
    }
    const prepare::PreparedFrameSlot* object_slot = nullptr;
    for (const auto& slot : context.function.prepared->stack_layout.frame_slots) {
      if (slot.object_id == object.object_id) {
        object_slot = &slot;
        break;
      }
    }
    if (object_slot == nullptr) {
      continue;
    }
    if (selected_slot == nullptr ||
        object_name == source_name ||
        object_slot->offset_bytes < selected_slot->offset_bytes) {
      selected_object = &object;
      selected_slot = object_slot;
      if (object_name == source_name) {
        break;
      }
    }
  }
  if (selected_object == nullptr || selected_slot == nullptr) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id,
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = selected_slot->slot_id,
      .byte_offset = static_cast<std::int64_t>(selected_slot->offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = selected_object->size_bytes == 0 ? std::size_t{8}
                                                     : selected_object->size_bytes,
      .align_bytes = selected_object->align_bytes == 0 ? std::size_t{8}
                                                       : selected_object->align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_stack_call_argument_destination(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedAbiBinding* binding,
    const MemoryOperand& source,
    std::size_t instruction_index) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source.size_bytes == 0 ||
      source_home.value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto destination_stack_offset =
      binding != nullptr && binding->destination_stack_offset_bytes.has_value()
          ? binding->destination_stack_offset_bytes
          : (move.destination_stack_offset_bytes.has_value()
                 ? move.destination_stack_offset_bytes
                 : argument.destination_stack_offset_bytes);
  if (!destination_stack_offset.has_value()) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .stored_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{move.from_value_id},
      .stored_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::Register,
      .base_register = RegisterOperand{
          .reg = outgoing_stack_argument_base_register(),
          .role = RegisterOperandRole::Physical,
          .expected_view = abi::RegisterView::X,
      },
      .byte_offset = static_cast<std::int64_t>(*destination_stack_offset),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = source.size_bytes,
      .align_bytes = source.align_bytes,
      .can_use_base_plus_offset = true,
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_aggregate_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& source_register,
    std::size_t size_bytes,
    std::int64_t byte_offset,
    std::size_t instruction_index) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      source_home.value_name == c4c::kInvalidValueName || size_bytes == 0) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow->function_name,
      .block_label = context.control_flow_block->block_label,
      .instruction_index = instruction_index,
      .result_value_id = argument.source_value_id.has_value()
                             ? argument.source_value_id
                             : std::optional<prepare::PreparedValueId>{source_home.value_id},
      .result_value_name = source_home.value_name,
      .base_kind = MemoryBaseKind::PointerValue,
      .base_register = source_register,
      .pointer_value_name = source_home.value_name,
      .pointer_value_id = source_home.value_id,
      .byte_offset = byte_offset,
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = size_bytes,
      .align_bytes = source_home.align_bytes.value_or(1),
      .can_use_base_plus_offset = true,
  };
}

struct PreservedCallArgumentSource {
  const prepare::PreparedCallPreservedValue* preserved = nullptr;
  std::optional<RegisterOperand> source_register;
  std::optional<MemoryOperand> source_memory;
};

[[nodiscard]] std::optional<std::size_t> prepared_block_index_by_label(
    const prepare::PreparedControlFlowFunction& function,
    c4c::BlockLabelId label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].block_label == label) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> prepared_block_successors(
    const prepare::PreparedControlFlowFunction& function,
    const prepare::PreparedControlFlowBlock& block) {
  std::vector<std::size_t> successors;
  auto append_label = [&](c4c::BlockLabelId label) {
    if (label == c4c::kInvalidBlockLabel) {
      return;
    }
    const auto index = prepared_block_index_by_label(function, label);
    if (index.has_value() &&
        std::find(successors.begin(), successors.end(), *index) ==
            successors.end()) {
      successors.push_back(*index);
    }
  };

  if (block.terminator_kind == bir::TerminatorKind::Branch) {
    append_label(block.branch_target_label);
  } else if (block.terminator_kind == bir::TerminatorKind::CondBranch) {
    append_label(block.true_label);
    append_label(block.false_label);
  }
  return successors;
}

[[nodiscard]] bool prepared_block_dominates(
    const prepare::PreparedControlFlowFunction& function,
    std::size_t dominator_index,
    std::size_t block_index) {
  const std::size_t count = function.blocks.size();
  if (dominator_index >= count || block_index >= count) {
    return false;
  }
  if (dominator_index == block_index) {
    return true;
  }
  std::vector<std::vector<std::size_t>> predecessors(count);
  for (std::size_t index = 0; index < count; ++index) {
    for (const auto successor :
         prepared_block_successors(function, function.blocks[index])) {
      if (successor < count) {
        predecessors[successor].push_back(index);
      }
    }
  }

  std::vector<std::vector<bool>> dominates(
      count, std::vector<bool>(count, true));
  if (count != 0) {
    std::fill(dominates.front().begin(), dominates.front().end(), false);
    dominates.front().front() = true;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t index = 1; index < count; ++index) {
      std::vector<bool> next(count, !predecessors[index].empty());
      if (predecessors[index].empty()) {
        std::fill(next.begin(), next.end(), false);
      } else {
        for (const auto predecessor : predecessors[index]) {
          for (std::size_t candidate = 0; candidate < count; ++candidate) {
            next[candidate] = next[candidate] && dominates[predecessor][candidate];
          }
        }
      }
      next[index] = true;
      if (next != dominates[index]) {
        dominates[index] = std::move(next);
        changed = true;
      }
    }
  }
  return dominates[block_index][dominator_index];
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_call_argument(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move) {
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return nullptr;
  }

  const auto value_id = argument.source_value_id.value_or(move.from_value_id);

  const prepare::PreparedCallPreservedValue* selected = nullptr;
  for (const auto& call : call_plans->calls) {
    if (call.block_index > current_call_plan.block_index ||
        (call.block_index == current_call_plan.block_index &&
         call.instruction_index >= current_call_plan.instruction_index)) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.value_id == value_id &&
          preserved.route != prepare::PreparedCallPreservationRoute::Unknown) {
        selected = &preserved;
      }
    }
  }
  return selected;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id) {
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return nullptr;
  }

  const prepare::PreparedCallPreservedValue* selected = nullptr;
  for (const auto& call : call_plans->calls) {
    if (call.block_index == current_call_plan.block_index) {
      if (call.instruction_index >= current_call_plan.instruction_index) {
        continue;
      }
    } else if (context.function.control_flow == nullptr ||
               !prepared_block_dominates(*context.function.control_flow,
                                         call.block_index,
                                         current_call_plan.block_index)) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.value_id == value_id &&
          preserved.route != prepare::PreparedCallPreservationRoute::Unknown) {
        selected = &preserved;
      }
    }
  }
  return selected;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_stack_preserved_value_before_instruction(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    std::size_t instruction_index) {
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return nullptr;
  }

  const prepare::PreparedCallPreservedValue* selected = nullptr;
  for (const auto& call : call_plans->calls) {
    if (call.block_index != context.block_index ||
        call.instruction_index >= instruction_index) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.value_id == value_id &&
          preserved.route == prepare::PreparedCallPreservationRoute::StackSlot &&
          preserved.slot_id.has_value() &&
          preserved.stack_offset_bytes.has_value() &&
          preserved.stack_size_bytes.has_value() &&
          *preserved.stack_size_bytes != 0) {
        selected = &preserved;
      }
    }
  }
  return selected;
}

[[nodiscard]] bool value_spelling_matches(const bir::Value& value,
                                          std::string_view spelling) {
  return value.kind == bir::Value::Kind::Named && value.name == spelling;
}

[[nodiscard]] bool non_call_instruction_uses_value(const bir::Inst& inst,
                                                   std::string_view spelling) {
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return value_spelling_matches(binary->lhs, spelling) ||
           value_spelling_matches(binary->rhs, spelling);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
    return value_spelling_matches(select->lhs, spelling) ||
           value_spelling_matches(select->rhs, spelling) ||
           value_spelling_matches(select->true_value, spelling) ||
           value_spelling_matches(select->false_value, spelling);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    return value_spelling_matches(cast->operand, spelling);
  }
  if (const auto* store_global = std::get_if<bir::StoreGlobalInst>(&inst)) {
    return value_spelling_matches(store_global->value, spelling);
  }
  if (const auto* store_local = std::get_if<bir::StoreLocalInst>(&inst)) {
    return value_spelling_matches(store_local->value, spelling);
  }
  return false;
}

[[nodiscard]] bool terminator_uses_value(const bir::Terminator& terminator,
                                         std::string_view spelling) {
  if (terminator.value.has_value() &&
      value_spelling_matches(*terminator.value, spelling)) {
    return true;
  }
  for (const auto& lane : terminator.return_lanes) {
    if (value_spelling_matches(lane, spelling)) {
      return true;
    }
  }
  return terminator.kind == bir::TerminatorKind::CondBranch &&
         value_spelling_matches(terminator.condition, spelling);
}

[[nodiscard]] bool branch_condition_uses_value(
    const module::BlockLoweringContext& context,
    std::string_view spelling) {
  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    return false;
  }
  const auto* condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (condition == nullptr) {
    return false;
  }
  return value_spelling_matches(condition->condition_value, spelling) ||
         (condition->lhs.has_value() &&
          value_spelling_matches(*condition->lhs, spelling)) ||
         (condition->rhs.has_value() &&
          value_spelling_matches(*condition->rhs, spelling));
}

[[nodiscard]] bool preserved_value_has_later_non_call_use(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallPreservedValue& preserved) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      preserved.value_name == c4c::kInvalidValueName ||
      call_plan.instruction_index >= context.bir_block->insts.size()) {
    return false;
  }
  const auto spelling =
      prepare::prepared_value_name(context.function.prepared->names, preserved.value_name);
  if (spelling.empty()) {
    return false;
  }

  for (std::size_t index = call_plan.instruction_index + 1;
       index < context.bir_block->insts.size();
       ++index) {
    if (std::holds_alternative<bir::CallInst>(context.bir_block->insts[index])) {
      return false;
    }
    if (non_call_instruction_uses_value(context.bir_block->insts[index], spelling)) {
      return true;
    }
  }
  return terminator_uses_value(context.bir_block->terminator, spelling) ||
         branch_condition_uses_value(context, spelling);
}

[[nodiscard]] bool preserved_value_has_block_entry_non_call_use(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPreservedValue& preserved) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      preserved.value_name == c4c::kInvalidValueName) {
    return false;
  }
  const auto spelling =
      prepare::prepared_value_name(context.function.prepared->names, preserved.value_name);
  if (spelling.empty()) {
    return false;
  }

  for (const auto& inst : context.bir_block->insts) {
    if (std::holds_alternative<bir::CallInst>(inst)) {
      return false;
    }
    if (non_call_instruction_uses_value(inst, spelling)) {
      return true;
    }
  }
  return terminator_uses_value(context.bir_block->terminator, spelling) ||
         branch_condition_uses_value(context, spelling);
}

[[nodiscard]] std::optional<PreservedCallArgumentSource>
make_prior_preserved_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* preserved = find_prior_preserved_value_for_call_argument(
      context, current_call_plan, argument, move);
  if (preserved == nullptr) {
    return std::nullopt;
  }

  const auto source_view =
      source_home != nullptr && source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved->register_name);
  PreservedCallArgumentSource result{.preserved = preserved};
  if (preserved->route == prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    auto source = make_register_operand_from_prepared_authority(
        preserved->register_name,
        preserved->register_placement,
        preserved->register_bank,
        RegisterOperandRole::CallAbi,
        preserved->value_id,
        preserved->value_name,
        preserved->contiguous_width,
        preserved->occupied_register_names,
        source_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    source->value_name = c4c::kInvalidValueName;
    result.source_register = *source;
    return result;
  }

  if (preserved->route == prepare::PreparedCallPreservationRoute::StackSlot &&
      preserved->slot_id.has_value() && preserved->stack_offset_bytes.has_value() &&
      preserved->stack_size_bytes.has_value() && *preserved->stack_size_bytes != 0 &&
      preserved->stack_align_bytes.has_value() && *preserved->stack_align_bytes != 0) {
    result.source_memory = MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = std::nullopt,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = preserved->slot_id,
        .byte_offset = static_cast<std::int64_t>(*preserved->stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *preserved->stack_size_bytes,
        .align_bytes = *preserved->stack_align_bytes,
        .can_use_base_plus_offset = true,
    };
    return result;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index,
    std::string reason,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.value_name == c4c::kInvalidValueName ||
      !preserved.register_name.has_value() ||
      !preserved.register_bank.has_value()) {
    return std::nullopt;
  }

  const auto* source_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        preserved.value_id);
  const auto expected_view =
      source_home != nullptr && source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto source = make_register_operand_from_prepared_authority(
      preserved.register_name,
      preserved.register_placement,
      preserved.register_bank,
      RegisterOperandRole::StoragePlan,
      preserved.value_id,
      preserved.value_name,
      preserved.contiguous_width,
      preserved.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  auto destination = make_register_operand_from_prepared_authority(
      preserved.register_name,
      preserved.register_placement,
      preserved.register_bank,
      RegisterOperandRole::StoragePlan,
      preserved.value_id,
      preserved.value_name,
      preserved.contiguous_width,
      preserved.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!source.has_value() || !destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = preserved.value_id,
      .to_value_id = preserved.value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = preserved.register_name,
      .destination_contiguous_width = preserved.contiguous_width,
      .destination_occupied_register_names = preserved.occupied_register_names,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = std::move(reason),
      .destination_register_placement = preserved.register_placement,
  };
  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow->function_name,
      .phase = phase,
      .authority_kind = bundle.authority_kind,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .move = synthetic_move,
      .source_register = *source,
      .destination_register = *destination,
      .source_bundle = &bundle,
      .source_move = &synthetic_move,
  };
  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (!preserved_value_has_later_non_call_use(context, call_plan, preserved)) {
    return std::nullopt;
  }
  return make_callee_saved_preservation_home_republication_instruction(
      context,
      bundle,
      preserved,
      prepare::PreparedMovePhase::BeforeInstruction,
      call_plan.block_index,
      call_plan.instruction_index,
      "callee_saved_preservation_home_republication",
      diagnostics);
}

[[nodiscard]] module::MachineInstruction make_call_boundary_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
      },
  };
}

[[nodiscard]] module::MachineInstruction make_outgoing_stack_base_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::size_t outgoing_bytes) {
  const auto scratch = outgoing_stack_argument_base_register();
  std::ostringstream asm_text;
  asm_text << "sub " << abi::register_name(scratch) << ", sp, #" << outgoing_bytes;
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::Register,
          .reg = scratch,
      }},
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = asm_text.str(),
      },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_population(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.value_name == c4c::kInvalidValueName ||
      !preserved.register_name.has_value() ||
      !preserved.register_bank.has_value() ||
      find_prior_preserved_value_for_value(context, call_plan, preserved.value_id) != nullptr) {
    return std::nullopt;
  }
  const auto* source_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        preserved.value_id);
  if (source_home == nullptr || source_home->value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto expected_view =
      source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      preserved.register_name,
      preserved.register_placement,
      preserved.register_bank,
      RegisterOperandRole::CallAbi,
      preserved.value_id,
      preserved.value_name,
      preserved.contiguous_width,
      preserved.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = preserved.value_id,
      .to_value_id = preserved.value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = preserved.register_name,
      .destination_contiguous_width = preserved.contiguous_width,
      .destination_occupied_register_names = preserved.occupied_register_names,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "callee_saved_preservation_home_population",
      .destination_register_placement = preserved.register_placement,
  };
  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow->function_name,
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .authority_kind = bundle.authority_kind,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .move = synthetic_move,
      .destination_register = *destination,
      .source_bundle = &bundle,
      .source_move = &synthetic_move,
  };

  if (source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value()) {
    const auto source_register_name =
        register_name_with_expected_view(source_home->register_name, expected_view);
    const auto source_parsed =
        source_register_name.has_value()
            ? abi::parse_aarch64_register_name(*source_register_name)
            : std::optional<abi::RegisterReference>{};
    std::optional<prepare::PreparedRegisterBank> source_bank;
    if (source_parsed.has_value()) {
      if (source_parsed->bank == abi::RegisterBank::GeneralPurpose) {
        source_bank = prepare::PreparedRegisterBank::Gpr;
      } else if (source_parsed->bank == abi::RegisterBank::FpSimd) {
        source_bank = source_parsed->view == abi::RegisterView::Q
                          ? prepare::PreparedRegisterBank::Vreg
                          : prepare::PreparedRegisterBank::Fpr;
      }
    }
    auto source = make_register_operand_from_prepared_authority(
        source_register_name,
        std::nullopt,
        source_bank,
        RegisterOperandRole::StoragePlan,
        source_home->value_id,
        source_home->value_name,
        1,
        {},
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() ||
        (source->reg.bank == destination->reg.bank &&
         source->reg.index == destination->reg.index)) {
      return std::nullopt;
    }
    move_record.source_register = *source;
  } else if (source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             source_home->offset_bytes.has_value()) {
    move_record.source_memory = MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow->function_name,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .result_value_id = source_home->value_id,
        .result_value_name = source_home->value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = source_home->slot_id,
        .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_home->size_bytes.value_or(
            scalar_size_from_register_view(expected_view)),
        .align_bytes = source_home->align_bytes.value_or(
            scalar_size_from_register_view(expected_view)),
        .can_use_base_plus_offset = true,
    };
  } else {
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<std::vector<std::string>>
fragmented_aggregate_register_lane_publication_lines(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t size_bytes,
    std::size_t source_instruction_index,
    std::size_t instruction_index,
    std::vector<MemoryOperand>& source_operands) {
  const auto stores =
      collect_byval_register_lane_stores(context, source_home, source_instruction_index);
  if (stores.empty() || stores.front().source_offset != 0) {
    return std::nullopt;
  }
  const auto scratch = aggregate_register_lane_scratch(destination);
  if (!scratch.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::size_t covered_bytes = 0;
  std::size_t lane_index = 0;
  std::size_t lane_offset = 0;
  const std::string scratch_x = abi::register_name(*scratch);
  for (const auto& store : stores) {
    if (store.source_offset > covered_bytes) {
      return std::nullopt;
    }
    std::size_t store_offset = covered_bytes - store.source_offset;
    while (covered_bytes < size_bytes && store_offset < store.size_bytes) {
      const std::size_t lane_remaining = 8 - lane_offset;
      const std::size_t store_remaining = store.size_bytes - store_offset;
      const std::size_t total_remaining = size_bytes - covered_bytes;
      const std::size_t max_chunk =
          std::min({std::size_t{8}, lane_remaining, store_remaining, total_remaining});
      const auto lane_register =
          aggregate_register_lane_destination(destination, lane_index);
      if (!lane_register.has_value()) {
        return std::nullopt;
      }
      std::optional<MemoryOperand> source_memory;
      bool source_memory_needs_materialized_address = false;
      std::size_t chunk_width = 0;
      for (const std::size_t candidate : {std::size_t{8},
                                          std::size_t{4},
                                          std::size_t{2},
                                          std::size_t{1}}) {
        if (candidate > max_chunk) {
          continue;
        }
        auto candidate_memory =
            aggregate_lane_store_memory(context,
                                        argument,
                                        source_home,
                                        store,
                                        store_offset,
                                        candidate,
                                        instruction_index);
        if (candidate_memory.has_value() &&
            !aggregate_register_lane_load_mnemonic(candidate).empty()) {
          const bool printable =
              aggregate_register_lane_memory_is_printable(*candidate_memory, candidate);
          const bool materializable =
              !printable &&
              candidate_memory->base_kind == MemoryBaseKind::FrameSlot &&
              !materialize_call_boundary_frame_slot_address_lines(
                   *scratch, *candidate_memory).empty();
          if (!printable && !materializable) {
            continue;
          }
          source_memory = std::move(candidate_memory);
          source_memory_needs_materialized_address = materializable;
          chunk_width = candidate;
          break;
        }
      }
      if (!source_memory.has_value() || chunk_width == 0) {
        return std::nullopt;
      }
      const auto mnemonic = aggregate_register_lane_load_mnemonic(chunk_width);
      const bool first_chunk = lane_offset == 0;
      const auto load_register =
          first_chunk
              ? aggregate_register_lane_load_register(*lane_register, chunk_width)
              : aggregate_register_lane_load_register(*scratch, chunk_width);
      if (source_memory_needs_materialized_address) {
        auto address_lines =
            materialize_call_boundary_frame_slot_address_lines(*scratch, *source_memory);
        lines.insert(lines.end(), address_lines.begin(), address_lines.end());
      }
      lines.push_back(std::string{mnemonic} + " " +
                      std::string{abi::register_name(load_register)} + ", " +
                      (source_memory_needs_materialized_address
                           ? ("[" + scratch_x + "]")
                           : memory_address(*source_memory)));
      source_operands.push_back(*source_memory);
      if (!first_chunk) {
        lines.push_back("orr " + std::string{abi::register_name(*lane_register)} +
                        ", " + std::string{abi::register_name(*lane_register)} +
                        ", " + scratch_x + ", lsl #" +
                        std::to_string(lane_offset * 8));
      }
      covered_bytes += chunk_width;
      store_offset += chunk_width;
      lane_offset += chunk_width;
      if (lane_offset == 8) {
        lane_offset = 0;
        ++lane_index;
      }
    }
    if (covered_bytes >= size_bytes) {
      break;
    }
  }
  if (covered_bytes == 0) {
    return std::nullopt;
  }
  return lines;
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_fragmented_aggregate_register_lane_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t size_bytes,
    std::size_t source_instruction_index,
    std::size_t instruction_index) {
  std::vector<MemoryOperand> source_operands;
  const auto lines = fragmented_aggregate_register_lane_publication_lines(
      context,
      argument,
      source_home,
      destination,
      size_bytes,
      source_instruction_index,
      instruction_index,
      source_operands);
  if (!lines.has_value() || lines->empty()) {
    return std::nullopt;
  }

  std::string asm_text;
  for (std::size_t index = 0; index < lines->size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += (*lines)[index];
  }

  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> uses;
  const auto destination_operand = make_register_operand(destination);
  operands.push_back(destination_operand);
  for (const auto& source : source_operands) {
    const auto source_operand = make_memory_operand(source);
    operands.push_back(source_operand);
    uses.push_back(effect_from_operand(source_operand));
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .operands = operands,
      .defs = {effect_from_operand(destination_operand)},
      .uses = uses,
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = std::move(operands),
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  (void)move;
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<std::string_view> value_move_load_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> value_move_store_mnemonic(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"strb"};
    case 2:
      return std::string_view{"strh"};
    case 4:
    case 8:
      return std::string_view{"str"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> value_move_scratch_register(
    std::size_t width_bytes) {
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  if (width_bytes == 1 || width_bytes == 2 || width_bytes == 4) {
    return abi::w_register(scratches.front().index);
  }
  if (width_bytes == 8) {
    return abi::x_register(scratches.front().index);
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> value_move_frame_slot_address(
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return std::nullopt;
  }
  std::ostringstream out;
  out << "[sp";
  if (*home.offset_bytes != 0) {
    out << ", #" << *home.offset_bytes;
  }
  out << "]";
  return out.str();
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_value_stack_move_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedValueHome& destination_home,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index) {
  const auto width_bytes =
      destination_home.size_bytes.value_or(source_home != nullptr
                                               ? source_home->size_bytes.value_or(4)
                                               : 4);
  const auto store_mnemonic = value_move_store_mnemonic(width_bytes);
  const auto destination = value_move_frame_slot_address(destination_home);
  if (!store_mnemonic.has_value() || !destination.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::string value_register;
  if (move.source_immediate_i32.has_value()) {
    const auto scratch = value_move_scratch_register(width_bytes);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*scratch)};
    auto materialized = materialize_integer_constant_lines(
        *scratch,
        static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(*move.source_immediate_i32)),
        width_bytes == 8 ? 64U : 32U);
    if (materialized.empty()) {
      return std::nullopt;
    }
    lines.insert(lines.end(), materialized.begin(), materialized.end());
  } else if (source_home != nullptr &&
             source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             source_home->offset_bytes.has_value()) {
    const auto scratch = value_move_scratch_register(width_bytes);
    const auto load_mnemonic = value_move_load_mnemonic(width_bytes);
    const auto source = value_move_frame_slot_address(*source_home);
    if (!scratch.has_value() || !load_mnemonic.has_value() || !source.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*scratch)};
    lines.push_back(std::string{*load_mnemonic} + " " + value_register + ", " +
                    *source);
  } else if (source_home != nullptr &&
             source_home->kind == prepare::PreparedValueHomeKind::Register &&
             source_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*source_home->register_name);
    if (!parsed.has_value() ||
        (parsed->bank != abi::RegisterBank::GeneralPurpose &&
         parsed->bank != abi::RegisterBank::FpSimd)) {
      return std::nullopt;
    }
    std::optional<abi::RegisterReference> source_reg;
    if (parsed->bank == abi::RegisterBank::FpSimd) {
      source_reg = abi::fp_simd_register(
          parsed->index,
          width_bytes == 8 ? abi::RegisterView::D : abi::RegisterView::S);
    } else {
      source_reg = width_bytes == 8 ? abi::x_register(parsed->index)
                                    : abi::w_register(parsed->index);
    }
    if (!source_reg.has_value()) {
      return std::nullopt;
    }
    value_register = std::string{abi::register_name(*source_reg)};
  } else {
    return std::nullopt;
  }
  lines.push_back(std::string{*store_mnemonic} + " " + value_register + ", " +
                  *destination);

  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .defs = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = destination_home.value_id,
          .value_name = destination_home.value_name,
      }},
      .uses = source_home != nullptr
                  ? std::vector<MachineEffectResource>{MachineEffectResource{
                        .kind = MachineEffectResourceKind::PreparedValue,
                        .value_id = source_home->value_id,
                        .value_name = source_home->value_name,
                    }}
                  : std::vector<MachineEffectResource>{},
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] module::MachineInstruction make_aggregate_stack_copy_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  const auto source_base = abi::register_name(source.base_register->reg);
  const std::string destination_base =
      destination.base_kind == MemoryBaseKind::Register && destination.base_register.has_value()
          ? std::string{abi::register_name(destination.base_register->reg)}
          : std::string{"sp"};
  std::string asm_text;
  std::size_t offset = 0;
  for (const auto width : aggregate_stack_copy_chunks(source.size_bytes)) {
    const auto scratch = aggregate_stack_copy_scratch(width);
    const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(width);
    const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(width);
    if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
      continue;
    }
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += std::string{load_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += stack_copy_address(
        source_base, source.byte_offset + static_cast<std::int64_t>(offset));
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += stack_copy_address(
        destination_base, destination.byte_offset + static_cast<std::int64_t>(offset));
    offset += width;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {make_memory_operand(source), make_memory_operand(destination)},
      .defs = {effect_from_operand(make_memory_operand(destination))},
      .uses = {effect_from_operand(make_memory_operand(source))},
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {make_memory_operand(source), make_memory_operand(destination)},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_byval_register_lane_stack_publication_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const MemoryOperand& source,
    const MemoryOperand& destination) {
  if (source.size_bytes == 0 || source.size_bytes != destination.size_bytes) {
    return std::nullopt;
  }

  auto chunks = aggregate_stack_copy_chunks(source.size_bytes);
  bool printable_chunks = true;
  std::size_t printable_offset = 0;
  for (const auto width : chunks) {
    auto source_chunk =
        aggregate_register_lane_memory(source, printable_offset, width);
    auto destination_chunk =
        aggregate_register_lane_memory(destination, printable_offset, width);
    if (!aggregate_register_lane_memory_is_printable(source_chunk, width) ||
        !aggregate_register_lane_memory_is_printable(destination_chunk, width)) {
      printable_chunks = false;
      break;
    }
    printable_offset += width;
  }
  if (!printable_chunks) {
    chunks.assign(source.size_bytes, std::size_t{1});
  }

  std::string asm_text;
  std::size_t offset = 0;
  for (const auto width : chunks) {
    auto source_chunk = aggregate_register_lane_memory(source, offset, width);
    auto destination_chunk =
        aggregate_register_lane_memory(destination, offset, width);
    if (!aggregate_register_lane_memory_is_printable(source_chunk, width) ||
        !aggregate_register_lane_memory_is_printable(destination_chunk, width)) {
      return std::nullopt;
    }
    const auto scratch = aggregate_stack_copy_scratch(width);
    const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(width);
    const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(width);
    if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
      return std::nullopt;
    }
    if (!asm_text.empty()) {
      asm_text += '\n';
    }
    asm_text += std::string{load_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += memory_address(source_chunk);
    asm_text += '\n';
    asm_text += std::string{store_mnemonic};
    asm_text += " ";
    asm_text += std::string{abi::register_name(*scratch)};
    asm_text += ", ";
    asm_text += memory_address(destination_chunk);
    offset += width;
  }
  if (asm_text.empty()) {
    return std::nullopt;
  }

  const auto source_operand = make_memory_operand(source);
  const auto destination_operand = make_memory_operand(destination);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {source_operand, destination_operand},
      .defs = {effect_from_operand(destination_operand)},
      .uses = {effect_from_operand(source_operand)},
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {source_operand, destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index,
                                                std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_fragmented_byval_register_lane_stack_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t source_instruction_index,
    std::size_t instruction_index,
    const MemoryOperand& destination) {
  const auto stores =
      collect_byval_register_lane_stores(context, source_home, source_instruction_index);
  if (stores.empty() || stores.front().source_offset != 0) {
    return std::nullopt;
  }

  std::string asm_text;
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> uses;
  std::size_t covered_bytes = 0;
  for (const auto& store : stores) {
    if (store.source_offset > covered_bytes) {
      return std::nullopt;
    }
    std::size_t store_offset = covered_bytes - store.source_offset;
    while (covered_bytes < size_bytes && store_offset < store.size_bytes) {
      const std::size_t store_remaining = store.size_bytes - store_offset;
      const std::size_t total_remaining = size_bytes - covered_bytes;
      const std::size_t max_chunk = std::min<std::size_t>(store_remaining, total_remaining);
      std::optional<MemoryOperand> source_memory;
      bool source_memory_needs_materialized_address = false;
      std::size_t chunk_width = 0;
      for (const std::size_t candidate : {std::size_t{8},
                                          std::size_t{4},
                                          std::size_t{2},
                                          std::size_t{1}}) {
        if (candidate > max_chunk) {
          continue;
        }
        auto candidate_source = aggregate_lane_store_memory(context,
                                                           argument,
                                                           source_home,
                                                           store,
                                                           store_offset,
                                                           candidate,
                                                           instruction_index);
        auto candidate_destination =
            aggregate_register_lane_memory(destination, covered_bytes, candidate);
        if (!candidate_source.has_value() ||
            !aggregate_register_lane_memory_is_printable(candidate_destination, candidate)) {
          continue;
        }
        const bool printable =
            aggregate_register_lane_memory_is_printable(*candidate_source, candidate);
        const auto scratch = aggregate_stack_copy_scratch(candidate);
        const bool materializable =
            !printable && scratch.has_value() &&
            candidate_source->base_kind == MemoryBaseKind::FrameSlot &&
            !materialize_call_boundary_frame_slot_address_lines(
                 abi::x_register(scratch->index), *candidate_source).empty();
        if ((!printable && !materializable) || !scratch.has_value() ||
            aggregate_stack_copy_load_mnemonic(candidate).empty() ||
            aggregate_stack_copy_store_mnemonic(candidate).empty()) {
          continue;
        }
        source_memory = std::move(candidate_source);
        source_memory_needs_materialized_address = materializable;
        chunk_width = candidate;
        break;
      }
      if (!source_memory.has_value() || chunk_width == 0) {
        return std::nullopt;
      }

      const auto scratch = aggregate_stack_copy_scratch(chunk_width);
      const auto load_mnemonic = aggregate_stack_copy_load_mnemonic(chunk_width);
      const auto store_mnemonic = aggregate_stack_copy_store_mnemonic(chunk_width);
      if (!scratch.has_value() || load_mnemonic.empty() || store_mnemonic.empty()) {
        return std::nullopt;
      }
      const auto destination_chunk =
          aggregate_register_lane_memory(destination, covered_bytes, chunk_width);
      if (!asm_text.empty()) {
        asm_text += '\n';
      }
      if (source_memory_needs_materialized_address) {
        auto address_lines = materialize_call_boundary_frame_slot_address_lines(
            abi::x_register(scratch->index), *source_memory);
        for (const auto& line : address_lines) {
          asm_text += line;
          asm_text += '\n';
        }
      }
      asm_text += std::string{load_mnemonic} + " " +
                  std::string{abi::register_name(*scratch)} + ", " +
                  (source_memory_needs_materialized_address
                       ? ("[" + std::string{abi::register_name(abi::x_register(scratch->index))} +
                          "]")
                       : memory_address(*source_memory));
      asm_text += '\n';
      asm_text += std::string{store_mnemonic} + " " +
                  std::string{abi::register_name(*scratch)} + ", " +
                  memory_address(destination_chunk);
      const auto source_operand = make_memory_operand(*source_memory);
      operands.push_back(source_operand);
      uses.push_back(effect_from_operand(source_operand));
      covered_bytes += chunk_width;
      store_offset += chunk_width;
    }
  }
  if (covered_bytes == 0 || asm_text.empty()) {
    return std::nullopt;
  }

  const auto destination_operand = make_memory_operand(destination);
  operands.push_back(destination_operand);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = operands,
      .defs = {effect_from_operand(destination_operand)},
      .uses = uses,
      .side_effects = {MachineSideEffectKind::MemoryRead,
                       MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = std::move(operands),
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* source_home =
      context.function.value_locations == nullptr
          ? nullptr
          : prepare::find_prepared_value_home(*context.function.value_locations,
                                              move.from_value_id);
  const auto* argument = find_prepared_argument_plan(call_plan, move);
  const auto* binding = find_prepared_argument_binding(bundle, move);
  const auto* f128_carriers =
      context.function.prepared == nullptr
          ? nullptr
          : prepare::find_prepared_f128_carriers(
                *context.function.prepared,
                context.function.control_flow != nullptr
                    ? context.function.control_flow->function_name
                    : c4c::kInvalidFunctionName);
  const auto* source_f128_carrier =
      f128_carriers != nullptr && source_home != nullptr
          ? prepare::find_prepared_f128_carrier(*f128_carriers, source_home->value_name)
          : nullptr;
  if (source_f128_carrier == nullptr && f128_carriers != nullptr && argument != nullptr &&
      argument->source_value_id.has_value()) {
    source_f128_carrier =
        prepare::find_prepared_f128_carrier(*f128_carriers, *argument->source_value_id);
  }

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = move,
      .source_bundle = &bundle,
      .source_move = &move,
  };
  const auto aggregate_lane_size =
      byval_register_lane_size_bytes(context, move, call_plan.instruction_index);

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr) {
    const bool frame_slot_address_argument =
        source_home != nullptr &&
        argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
        (make_sret_memory_return_address_source(
             context, call_plan, *argument, instruction_index)
             .has_value() ||
         make_frame_slot_call_argument_address_source(
             context, *argument, *source_home, instruction_index)
             .has_value());
    if (!frame_slot_address_argument) {
      auto preserved_source = make_prior_preserved_call_argument_source(
          context,
          call_plan,
          *argument,
          move,
          source_home,
          instruction_index,
          diagnostics);
      if (preserved_source.has_value()) {
        auto destination = make_register_operand_from_prepared_authority(
            binding != nullptr && binding->destination_register_name.has_value()
                ? binding->destination_register_name
                : move.destination_register_name,
            binding != nullptr && binding->destination_register_placement.has_value()
                ? binding->destination_register_placement
                : move.destination_register_placement,
            argument->destination_register_bank,
            RegisterOperandRole::CallAbi,
            move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                                  : argument->source_value_id,
            preserved_source->preserved != nullptr
                ? preserved_source->preserved->value_name
                : c4c::kInvalidValueName,
            binding != nullptr ? binding->destination_contiguous_width
                               : move.destination_contiguous_width,
            binding != nullptr ? binding->destination_occupied_register_names
                               : move.destination_occupied_register_names,
            preserved_source->source_memory.has_value()
                ? scalar_integer_register_view_from_size(
                      preserved_source->source_memory->size_bytes)
                : (source_home != nullptr && source_home->size_bytes.has_value()
                       ? scalar_integer_register_view_from_size(*source_home->size_bytes)
                       : scalar_view_from_register_name(move.destination_register_name)),
            diagnostics,
            context,
            instruction_index);
        if (destination.has_value()) {
          move_record.source_register = preserved_source->source_register;
          move_record.source_memory = preserved_source->source_memory;
          move_record.destination_register = *destination;
          return make_call_boundary_machine_instruction(
              context,
              instruction_index,
              make_call_boundary_move_instruction(std::move(move_record)));
        }
      }
    }
  }

  const bool selected_gpr_argument_move =
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_register_name.has_value() &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_f128_argument_move =
      argument != nullptr &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Vreg ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Fpr) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(source_f128_carrier);
  const bool selected_scalar_fpr_argument_move =
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      scalar_fp_view_from_register_name(binding->destination_register_name).has_value();
  const bool selected_f128_constant_argument_move =
      argument != nullptr &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->source_literal.has_value() &&
      argument->source_literal->type == bir::TypeKind::F128 &&
      argument->source_literal->f128_payload.has_value() &&
      argument->source_value_id.has_value() &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_f128_constant_carrier(source_f128_carrier) &&
      source_f128_carrier->constant_payload->low_bits ==
          argument->source_literal->f128_payload->low_bits &&
      source_f128_carrier->constant_payload->high_bits ==
          argument->source_literal->f128_payload->high_bits &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register;

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      !selected_f128_constant_argument_move) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        "AArch64 binary128 constant argument move requires a complete structured full-width constant carrier");
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() && argument != nullptr &&
      (selected_gpr_argument_move || selected_scalar_fpr_argument_move ||
       selected_f128_argument_move) &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      !move_record.destination_register.has_value()) {
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-boundary move source register disagrees with prepared value home");
      return std::nullopt;
    }
    if (selected_f128_argument_move &&
        source_f128_carrier->register_name != source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 binary128 call-boundary move source register disagrees with prepared f128 carrier");
      return std::nullopt;
    }
    const auto expected_view =
        selected_f128_argument_move ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : selected_scalar_fpr_argument_move
            ? scalar_fp_view_from_register_name(binding->destination_register_name)
            : std::nullopt;
    const auto source_register_name =
        selected_scalar_fpr_argument_move &&
                argument->source_register_placement.has_value()
            ? std::optional<std::string>{}
            : source_home->register_name;
    auto destination = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : std::nullopt,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    move_record.destination_register = *destination;
    if (selected_f128_argument_move) {
      auto source = make_f128_q_register_operand_from_carrier(
          *source_f128_carrier,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        return std::nullopt;
      }
      move_record.source_register = *source;
      move_record.source_f128_carrier = source_f128_carrier;
    } else {
      auto source = make_register_operand_from_prepared_authority(
          source_register_name,
          argument->source_register_placement,
          argument->source_register_bank,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          std::vector<std::string>{},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        return std::nullopt;
      }
      move_record.source_register = *source;
    }
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      argument != nullptr &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto register_byval_size = aarch64_register_byval_argument_size_bytes(
        context, *argument, call_plan.instruction_index);
    std::optional<MemoryOperand> source;
    std::optional<MemoryOperand> address_source;
    if (register_byval_size.has_value()) {
      source = make_byval_register_lane_prepared_source(
          context, *argument, *source_home, *register_byval_size, call_plan.instruction_index);
      if (!source.has_value() && source_home->register_name.has_value()) {
        auto source_register = make_register_operand_from_prepared_authority(
            source_home->register_name,
            argument->source_register_placement,
            argument->source_register_bank.has_value()
                ? argument->source_register_bank
                : std::optional<prepare::PreparedRegisterBank>{
                      prepare::PreparedRegisterBank::Gpr},
            RegisterOperandRole::CallAbi,
            source_home->value_id,
            source_home->value_name,
            1,
            {},
            abi::RegisterView::X,
            diagnostics,
            context,
            instruction_index);
        if (source_register.has_value()) {
          source = make_aggregate_call_argument_source(
              context,
              *argument,
              *source_home,
              *source_register,
              *register_byval_size,
              source_home->pointer_byte_delta.value_or(0),
              instruction_index);
        }
      }
    } else {
      address_source =
          make_local_frame_address_call_argument_source(
              context, *argument, *source_home, instruction_index);
      source = address_source;
    }
    if (source.has_value()) {
      const auto destination_register_placement =
          binding != nullptr && binding->destination_register_placement.has_value()
              ? binding->destination_register_placement
              : (move.destination_register_placement.has_value()
                     ? move.destination_register_placement
                     : argument->destination_register_placement);
      const auto destination_register_name =
          destination_register_placement.has_value()
              ? std::optional<std::string>{}
              : (binding != nullptr && binding->destination_register_name.has_value()
                     ? binding->destination_register_name
                     : move.destination_register_name);
      auto destination = make_register_operand_from_prepared_authority(
          destination_register_name,
          destination_register_placement,
          argument->destination_register_bank,
          RegisterOperandRole::CallAbi,
          move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                                : argument->source_value_id,
          source_home->value_name,
          binding != nullptr ? binding->destination_contiguous_width
                             : move.destination_contiguous_width,
          binding != nullptr ? binding->destination_occupied_register_names
                             : move.destination_occupied_register_names,
          abi::RegisterView::X,
          diagnostics,
          context,
          instruction_index);
      if (!destination.has_value()) {
        return std::nullopt;
      }
      move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.source_memory_materializes_address = true;
      move_record.destination_register = *destination;
      if (register_byval_size.has_value()) {
        move_record.source_memory_materializes_address = false;
        move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
      }
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      move_record.destination_register.has_value()) {
    if (auto cast_publication =
            make_immediate_cast_call_argument_publication_instruction(
                context, *source_home, *move_record.destination_register, instruction_index)) {
      return cast_publication;
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      is_aarch64_byval_register_lane_move(move) &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto lane_size =
        aggregate_lane_size.has_value() ? aggregate_lane_size : source_home->size_bytes;
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument source register disagrees with prepared value home");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && source_home->size_bytes.has_value()) {
      auto source_register = make_register_operand_from_prepared_authority(
          source_home->register_name,
          argument->source_register_placement,
          argument->source_register_bank.has_value()
              ? argument->source_register_bank
              : std::optional<prepare::PreparedRegisterBank>{
                    prepare::PreparedRegisterBank::Gpr},
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          abi::RegisterView::X,
          diagnostics,
          context,
          instruction_index);
      if (source_register.has_value()) {
        source = make_aggregate_call_argument_source(
            context,
            *argument,
            *source_home,
            *source_register,
            *lane_size,
            source_home->pointer_byte_delta.value_or(0),
            instruction_index);
      }
    }
    const auto destination_register_placement =
        move.destination_register_placement.has_value()
            ? move.destination_register_placement
            : argument->destination_register_placement;
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : move.destination_register_name;
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        std::max(move.destination_contiguous_width,
                 argument->destination_contiguous_width),
        !move.destination_occupied_register_names.empty()
            ? move.destination_occupied_register_names
            : argument->destination_occupied_register_names,
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires prepared source bytes and destination register");
      return std::nullopt;
      }
    move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.destination_register = *destination;
    move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      is_aarch64_byval_register_lane_move(move) &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto lane_size =
        aggregate_lane_size.has_value() ? aggregate_lane_size : source_home->size_bytes;
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    const auto destination_register_placement =
        move.destination_register_placement.has_value()
            ? move.destination_register_placement
            : argument->destination_register_placement;
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : move.destination_register_name;
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        std::max(move.destination_contiguous_width,
                 argument->destination_contiguous_width),
        !move.destination_occupied_register_names.empty()
            ? move.destination_occupied_register_names
            : argument->destination_occupied_register_names,
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
      }
    if (!source.has_value()) {
      if (auto fragmented =
              make_fragmented_aggregate_register_lane_publication_instruction(
                  context,
                  bundle,
                  move,
                  *argument,
                  *source_home,
                  *destination,
                  *lane_size,
                  call_plan.instruction_index,
                  instruction_index)) {
        return fragmented;
      }
    }
    if (!source.has_value() && source_home->size_bytes.has_value()) {
      source =
          make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
      if (source.has_value()) {
        source->size_bytes = *lane_size;
      }
    }
    if (!source.has_value() || source->size_bytes == 0 || source->size_bytes > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate register-lane call-argument publication requires a small prepared frame-slot source");
      return std::nullopt;
    }
    move_record.source_register.reset();
      move_record.source_memory = *source;
      move_record.destination_register = *destination;
    move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
    }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      scalar_fp_view_from_register_name(binding->destination_register_name).has_value()) {
    auto source =
        make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
    const auto destination_register_placement =
        binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        scalar_fp_view_from_register_name(binding->destination_register_name),
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 scalar FPR frame-slot call-argument move requires a prepared load access for the source value");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->size_bytes == std::optional<std::size_t>{16} &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) {
    auto source =
        make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
    const auto destination_register_placement =
        binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        abi::RegisterView::Q,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 frame-slot call-argument move requires a prepared frame-slot source and q-register destination");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !is_aarch64_byval_register_lane_move(move) &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register)) {
    const auto register_byval_size = aarch64_register_byval_argument_size_bytes(
        context, *argument, call_plan.instruction_index);
    std::optional<MemoryOperand> address_source;
    std::optional<MemoryOperand> source;
    if (register_byval_size.has_value()) {
      source = make_byval_register_lane_prepared_source(
          context, *argument, *source_home, *register_byval_size, call_plan.instruction_index);
      if (!source.has_value()) {
        source = make_frame_slot_call_argument_source(
            context, *argument, *source_home, instruction_index);
        if (source.has_value()) {
          source->size_bytes = *register_byval_size;
        }
      }
    } else {
      address_source = make_sret_memory_return_address_source(
          context, call_plan, *argument, instruction_index);
      if (!address_source.has_value()) {
        address_source = make_frame_slot_call_argument_address_source(
            context, *argument, *source_home, instruction_index);
      }
      if (!address_source.has_value()) {
        if (const auto byval_size = aarch64_indirect_byval_argument_size_bytes(
                context, *argument, call_plan.instruction_index);
            byval_size.has_value()) {
          address_source = make_byval_register_lane_prepared_source(
              context, *argument, *source_home, *byval_size, call_plan.instruction_index);
        }
      }
      source =
          address_source.has_value()
              ? address_source
              : make_frame_slot_call_argument_source(
                    context, *argument, *source_home, instruction_index);
    }
    const auto destination_register_placement =
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument->destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    auto destination = make_register_operand_from_prepared_authority(
        destination_register_name,
        destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_home->value_name,
        binding != nullptr ? binding->destination_contiguous_width
                           : move.destination_contiguous_width,
        binding != nullptr ? binding->destination_occupied_register_names
                           : move.destination_occupied_register_names,
        register_byval_size.has_value() || address_source.has_value()
            ? std::optional<abi::RegisterView>{abi::RegisterView::X}
            : source.has_value()
            ? scalar_integer_register_view_from_size(source->size_bytes)
            : std::nullopt,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 frame-slot call-argument move requires a prepared load access for the source value");
      return std::nullopt;
    }
    move_record.source_memory = *source;
    move_record.source_memory_materializes_address = address_source.has_value();
    move_record.destination_register = *destination;
    if (register_byval_size.has_value()) {
      move_record.move.reason = "call_arg_byval_aggregate_register_lanes";
    }
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (argument->value_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      is_aarch64_byval_register_lane_move(move) &&
      (!argument->source_register_bank.has_value() ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto lane_size =
        aggregate_lane_size.has_value() ? aggregate_lane_size : source_home->size_bytes;
    if (!lane_size.has_value() || *lane_size == 0 || *lane_size > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a small ABI byval size");
      return std::nullopt;
    }
    auto source = make_byval_register_lane_prepared_source(
        context, *argument, *source_home, *lane_size, call_plan.instruction_index);
    if (!source.has_value() && source_home->size_bytes.has_value()) {
      source =
          make_frame_slot_call_argument_source(context, *argument, *source_home, instruction_index);
      if (source.has_value()) {
        source->size_bytes = *lane_size;
      }
    }
    if (!source.has_value() || source->size_bytes == 0 || source->size_bytes > 16) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires prepared source bytes");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires a prepared destination stack offset");
      return std::nullopt;
    }
    if (auto fragmented =
            make_fragmented_byval_register_lane_stack_publication_instruction(
                context,
                *argument,
                *source_home,
                *lane_size,
                call_plan.instruction_index,
                instruction_index,
                *destination)) {
      return fragmented;
    }
    auto lowered = make_byval_register_lane_stack_publication_instruction(
        context, instruction_index, *source, *destination);
    if (!lowered.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 aggregate stack-lane call-argument publication requires printable prepared source and destination stack slots");
      return std::nullopt;
    }
    return lowered;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      argument != nullptr &&
      (argument->source_encoding == prepare::PreparedStorageEncodingKind::Register ||
       argument->source_encoding == prepare::PreparedStorageEncodingKind::ComputedAddress) &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->value_bank == prepare::PreparedRegisterBank::AggregateAddress &&
      (!argument->source_register_bank.has_value() ||
       argument->source_register_bank == prepare::PreparedRegisterBank::Gpr ||
       argument->source_register_bank == prepare::PreparedRegisterBank::AggregateAddress) &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto* source_register_home = source_home;
    if (source_home->kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
        argument->source_base_value_id.has_value() &&
        context.function.value_locations != nullptr) {
      source_register_home = prepare::find_prepared_value_home(
          *context.function.value_locations, *argument->source_base_value_id);
    }
    if (source_register_home == nullptr ||
        source_register_home->kind != prepare::PreparedValueHomeKind::Register ||
        !source_register_home->register_name.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate address register");
      return std::nullopt;
    }
    if (argument->source_register_name.has_value() &&
        *argument->source_register_name != *source_register_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument source register disagrees with prepared value home");
      return std::nullopt;
    }
    const auto size_bytes =
        source_home->size_bytes.has_value()
            ? source_home->size_bytes
            : aarch64_stack_byval_argument_size_bytes(
                  context, *argument, call_plan.instruction_index);
    if (!size_bytes.has_value() || *size_bytes == 0) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate size");
      return std::nullopt;
    }
    auto source_register = make_register_operand_from_prepared_authority(
        source_register_home->register_name,
        argument->source_register_placement,
        argument->source_register_bank.has_value()
            ? argument->source_register_bank
            : std::optional<prepare::PreparedRegisterBank>{
                  prepare::PreparedRegisterBank::Gpr},
        RegisterOperandRole::CallAbi,
        source_register_home->value_id,
        source_register_home->value_name,
        1,
        {},
        abi::RegisterView::X,
        diagnostics,
        context,
        instruction_index);
    if (!source_register.has_value()) {
      return std::nullopt;
    }
    const auto source_byte_offset = source_home->pointer_byte_delta.value_or(0);
    const auto source = make_aggregate_call_argument_source(
        context,
        *argument,
        *source_home,
        *source_register,
        *size_bytes,
        source_byte_offset,
        instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared aggregate address source");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 aggregate stack call-argument copy requires a prepared destination stack offset");
      return std::nullopt;
    }
    return make_aggregate_stack_copy_instruction(
        context, instruction_index, *source, *destination);
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home->size_bytes == std::optional<std::size_t>{16} &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      argument->value_bank == prepare::PreparedRegisterBank::Vreg &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto source = make_frame_slot_call_argument_source(
        context, *argument, *source_home, instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a prepared frame-slot source");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a prepared destination stack offset");
      return std::nullopt;
    }
    if (f128_carriers == nullptr) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires prepared f128 carrier facts");
      return std::nullopt;
    }
    auto prepared = make_prepared_f128_carrier_transport_record(
        *f128_carriers,
        source_home->value_name,
        F128TransportKind::StoreToMemory,
        *destination);
    if (!prepared.record.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 binary128 stack call-argument move requires a complete source carrier");
      return std::nullopt;
    }
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_f128_transport_instruction(std::move(*prepared.record)));
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
      argument->source_value_id == std::optional<prepare::PreparedValueId>{move.from_value_id} &&
      (binding == nullptr ||
       binding->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot)) {
    const auto source = make_frame_slot_call_argument_source(
        context, *argument, *source_home, instruction_index);
    if (!source.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 stack call-argument move requires a prepared frame-slot source");
      return std::nullopt;
    }
    const auto value_type = scalar_integer_type_from_size(source->size_bytes);
    if (!value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 stack call-argument move lowering requires a 1, 4, or 8 byte prepared stack slot");
      return std::nullopt;
    }
    const auto destination = make_stack_call_argument_destination(
        context, *argument, *source_home, move, binding, *source, instruction_index);
    if (!destination.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          "AArch64 stack call-argument move requires a prepared destination stack offset");
      return std::nullopt;
    }
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = *destination,
            .value = make_memory_operand(*source),
            .value_type = *value_type,
        }));
  }

  if (selected_f128_constant_argument_move) {
    auto destination = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        argument->destination_register_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : argument->source_value_id,
        source_f128_carrier->value_name,
        binding->destination_contiguous_width,
        binding->destination_occupied_register_names,
        abi::RegisterView::Q,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      return std::nullopt;
    }
    move_record.destination_register = *destination;
    move_record.source_f128_carrier = source_f128_carrier;
    move_record.source_f128_constant_payload = source_f128_carrier->constant_payload;
  }

  const bool symbol_address_argument_materialized_at_call_site =
      bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::SymbolAddress &&
      source_home != nullptr &&
      source_home->kind != prepare::PreparedValueHomeKind::Register &&
      context.function.prepared != nullptr &&
      context.function.control_flow != nullptr &&
      context.control_flow_block != nullptr &&
      [&]() {
        const auto* addressing = prepare::find_prepared_addressing(
            *context.function.prepared, context.function.control_flow->function_name);
        if (addressing == nullptr) {
          return false;
        }
        for (const auto& materialization : addressing->address_materializations) {
          if (materialization.block_label == context.control_flow_block->block_label &&
              materialization.inst_index <= instruction_index &&
              materialization.result_value_name == source_home->value_name) {
            return true;
          }
        }
        return false;
      }();
  if (symbol_address_argument_materialized_at_call_site) {
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      !selected_f128_constant_argument_move &&
      ((!move_record.source_register.has_value() &&
        !move_record.source_memory.has_value()) ||
       !move_record.destination_register.has_value())) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 call-argument move lowering requires selected prepared register source and destination");
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_before_call_immediate_binding(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedAbiBinding& binding,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (bundle.phase != prepare::PreparedMovePhase::BeforeCall ||
      binding.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      !binding.destination_abi_index.has_value()) {
    return std::nullopt;
  }

  const auto* argument = [&]() -> const prepare::PreparedCallArgumentPlan* {
    for (const auto& candidate : call_plan.arguments) {
      if (candidate.arg_index == *binding.destination_abi_index &&
          candidate.source_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
          candidate.source_literal.has_value() &&
          (binding.destination_storage_kind ==
               prepare::PreparedMoveStorageKind::StackSlot ||
           candidate.destination_register_bank == prepare::PreparedRegisterBank::Gpr)) {
        return &candidate;
      }
    }
    return nullptr;
  }();
  if (argument == nullptr) {
    return std::nullopt;
  }

  const auto source_immediate =
      make_scalar_call_argument_immediate(*argument->source_literal,
                                          argument->source_value_id);
  const auto expected_view =
      scalar_integer_register_view(argument->source_literal->type);
  if (!source_immediate.has_value() || !expected_view.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 immediate call-argument move requires a scalar integer literal");
    return std::nullopt;
  }

  if (binding.destination_storage_kind ==
      prepare::PreparedMoveStorageKind::StackSlot) {
    if (!binding.destination_stack_offset_bytes.has_value()) {
      return std::nullopt;
    }
    const auto value_type = scalar_integer_type_from_size(
        scalar_size_from_register_view(expected_view));
    if (!value_type.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 immediate stack call-argument move requires a 4 or 8 byte scalar integer literal");
      return std::nullopt;
    }
    MemoryOperand destination{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .stored_value_id = argument->source_value_id,
        .stored_value_name = c4c::kInvalidValueName,
        .base_kind = MemoryBaseKind::Register,
        .base_register = RegisterOperand{
            .reg = outgoing_stack_argument_base_register(),
            .role = RegisterOperandRole::Physical,
            .expected_view = abi::RegisterView::X,
        },
        .byte_offset =
            static_cast<std::int64_t>(*binding.destination_stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = scalar_size_from_register_view(expected_view),
        .align_bytes = scalar_size_from_register_view(expected_view),
        .can_use_base_plus_offset = true,
    };
    return make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_memory_instruction(MemoryInstructionRecord{
            .memory_kind = MemoryInstructionKind::Store,
            .address = std::move(destination),
            .value = make_immediate_operand(*source_immediate),
            .value_type = *value_type,
        }));
  }

  if (binding.destination_storage_kind !=
          prepare::PreparedMoveStorageKind::Register ||
      !binding.destination_register_name.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      binding.destination_register_placement.has_value()
          ? std::optional<std::string>{}
          : binding.destination_register_name,
      binding.destination_register_placement.has_value()
          ? binding.destination_register_placement
          : argument->destination_register_placement,
      argument->destination_register_bank,
      RegisterOperandRole::CallAbi,
      argument->source_value_id,
      c4c::kInvalidValueName,
      binding.destination_contiguous_width,
      binding.destination_occupied_register_names.empty()
          ? argument->destination_occupied_register_names
          : binding.destination_occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = argument->source_value_id.value_or(prepare::PreparedValueId{0}),
      .to_value_id = argument->source_value_id.value_or(prepare::PreparedValueId{0}),
      .destination_kind = binding.destination_kind,
      .destination_storage_kind = binding.destination_storage_kind,
      .destination_abi_index = binding.destination_abi_index,
      .destination_register_name = binding.destination_register_name,
      .destination_contiguous_width = binding.destination_contiguous_width,
      .destination_occupied_register_names = binding.destination_occupied_register_names,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "call_arg_immediate_to_register",
      .destination_register_placement = binding.destination_register_placement,
  };

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = std::move(synthetic_move),
      .source_immediate = source_immediate,
      .destination_register = *destination,
      .source_bundle = &bundle,
  };
  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_after_call_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* destination_home =
      context.function.value_locations == nullptr
          ? nullptr
          : prepare::find_prepared_value_home(*context.function.value_locations,
                                              move.to_value_id);
  const auto* result_plan = call_plan.result.has_value() ? &*call_plan.result : nullptr;
  const auto* binding = find_prepared_result_binding(bundle, move);
  const auto* f128_carriers =
      context.function.prepared == nullptr
          ? nullptr
          : prepare::find_prepared_f128_carriers(
                *context.function.prepared,
                context.function.control_flow != nullptr
                    ? context.function.control_flow->function_name
                    : c4c::kInvalidFunctionName);
  const auto* destination_f128_carrier =
      f128_carriers != nullptr && destination_home != nullptr
          ? prepare::find_prepared_f128_carrier(*f128_carriers, destination_home->value_name)
          : nullptr;

  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = bundle.phase,
      .authority_kind = bundle.authority_kind,
      .block_index = bundle.block_index,
      .instruction_index = bundle.instruction_index,
      .source_parallel_copy_predecessor_label =
          bundle.source_parallel_copy_predecessor_label,
      .source_parallel_copy_successor_label =
          bundle.source_parallel_copy_successor_label,
      .move = move,
      .source_bundle = &bundle,
      .source_move = &move,
  };

  const bool selected_gpr_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_scalar_fpr_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Fpr &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Fpr &&
      result_plan->source_contiguous_width == 1 &&
      result_plan->destination_contiguous_width == 1;
  const bool selected_f128_result_move =
      result_plan != nullptr &&
      result_plan->source_register_bank == prepare::PreparedRegisterBank::Vreg &&
      result_plan->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(destination_f128_carrier);

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      !move.destination_abi_index.has_value() &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      destination_home != nullptr &&
      destination_home->kind == prepare::PreparedValueHomeKind::Register &&
      destination_home->register_name.has_value() &&
      result_plan != nullptr &&
      result_plan->instruction_index == instruction_index &&
      result_plan->destination_value_id == std::optional<prepare::PreparedValueId>{move.to_value_id} &&
      result_plan->source_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      result_plan->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      result_plan->source_register_name.has_value() &&
      result_plan->destination_register_name.has_value() &&
      (selected_gpr_result_move || selected_scalar_fpr_result_move ||
       selected_f128_result_move) &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      binding->destination_register_name.has_value()) {
    if (*result_plan->source_register_name != *binding->destination_register_name ||
        *result_plan->source_register_name != *move.destination_register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-result move source register disagrees with prepared ABI binding");
      return std::nullopt;
    }
    if (*result_plan->destination_register_name != *destination_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 call-result move destination register disagrees with prepared value home");
      return std::nullopt;
    }
    if (selected_f128_result_move &&
        destination_f128_carrier->register_name != destination_home->register_name) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          context,
          instruction_index,
          "AArch64 binary128 call-result move destination register disagrees with prepared f128 carrier");
      return std::nullopt;
    }
    const auto expected_view =
        selected_f128_result_move ? std::optional<abi::RegisterView>{abi::RegisterView::Q}
        : selected_scalar_fpr_result_move
            ? scalar_fp_view_from_register_name(binding->destination_register_name)
        : std::nullopt;
    auto source = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        result_plan->source_register_placement.has_value()
            ? result_plan->source_register_placement
            : binding->destination_register_placement,
        result_plan->source_register_bank,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        result_plan->source_contiguous_width,
        result_plan->source_occupied_register_names.empty()
            ? binding->destination_occupied_register_names
            : result_plan->source_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    auto destination = make_register_operand_from_prepared_authority(
        selected_scalar_fpr_result_move &&
                result_plan->destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : destination_home->register_name,
        result_plan->destination_register_placement,
        result_plan->destination_register_bank,
        RegisterOperandRole::CallAbi,
        destination_home->value_id,
        destination_home->value_name,
        result_plan->destination_contiguous_width,
        result_plan->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
    move_record.destination_f128_carrier =
        selected_f128_result_move ? destination_f128_carrier : nullptr;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      result_plan != nullptr &&
      result_plan->destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot) {
    return std::nullopt;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      destination_home != nullptr &&
      destination_home->kind == prepare::PreparedValueHomeKind::Register &&
      destination_home->register_name.has_value() &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      binding->destination_register_name.has_value() &&
      binding->destination_register_placement.has_value() &&
      binding->destination_register_placement->bank == prepare::PreparedRegisterBank::Fpr &&
      (!move_record.source_register.has_value() ||
       !move_record.destination_register.has_value())) {
    const auto expected_view =
        scalar_fp_view_from_register_name(binding->destination_register_name);
    auto source = make_register_operand_from_prepared_authority(
        binding->destination_register_name,
        binding->destination_register_placement,
        prepare::PreparedRegisterBank::Fpr,
        RegisterOperandRole::CallAbi,
        move.from_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.from_value_id}
                                : std::nullopt,
        destination_home->value_name,
        1,
        binding->destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    auto destination = make_register_operand_from_prepared_authority(
        destination_home->register_name,
        std::nullopt,
        prepare::PreparedRegisterBank::Fpr,
        RegisterOperandRole::CallAbi,
        destination_home->value_id,
        destination_home->value_name,
        1,
        {},
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
  }

  if (bundle.phase == prepare::PreparedMovePhase::AfterCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallResultAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      (!move_record.source_register.has_value() ||
       !move_record.destination_register.has_value())) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 call-result move lowering requires selected prepared register source and destination");
    return std::nullopt;
  }

  return make_call_boundary_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<RegisterOperand> make_indirect_callee_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedIndirectCalleePlan& callee,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      !callee.register_name.has_value() || callee.bank != prepare::PreparedRegisterBank::Gpr ||
      callee.slot_id.has_value() || callee.stack_offset_bytes.has_value() ||
      callee.immediate_i32.has_value() || callee.pointer_base_value_name.has_value() ||
      callee.pointer_byte_delta.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 indirect call lowering requires an explicit prepared GPR callee register");
    return std::nullopt;
  }

  const auto converted = abi::convert_prepared_register(
      *callee.register_name,
      callee.bank,
      prepare::PreparedRegisterClass::General,
      abi::RegisterView::X);
  if (!converted.reg.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        converted.error.has_value()
            ? converted.error->message
            : "prepared indirect callee register could not be converted");
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = callee.value_id,
      .value_name = callee.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = callee.bank,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_registers = {*callee.register_name},
  };
}

[[nodiscard]] std::optional<MemoryOperand> make_memory_return_storage(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryReturnPlan& memory_return,
    std::size_t instruction_index) {
  if (memory_return.encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !memory_return.slot_id.has_value() ||
      !memory_return.stack_offset_bytes.has_value()) {
    return std::nullopt;
  }
  return MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .base_kind = MemoryBaseKind::FrameSlot,
      .frame_slot_id = memory_return.slot_id,
      .byte_offset = static_cast<std::int64_t>(*memory_return.stack_offset_bytes),
      .byte_offset_is_prepared_snapshot = true,
      .size_bytes = memory_return.size_bytes,
      .align_bytes = memory_return.align_bytes,
      .can_use_base_plus_offset = true,
  };
}

}  // namespace

const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.call_plans == nullptr) {
    return nullptr;
  }
  for (const auto& call : context.function.call_plans->calls) {
    if (call.block_index == context.block_index &&
        call.instruction_index == instruction_index) {
      return &call;
    }
  }
  return nullptr;
}

const prepare::PreparedCallPlan* require_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* call_plan = find_prepared_call_plan(context, instruction_index);
  if (call_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 call lowering requires an authoritative PreparedCallPlan");
  }
  return call_plan;
}

[[nodiscard]] bool prepared_argument_is_small_byval_stack_lane(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  if (context.bir_block == nullptr ||
      instruction_index >= context.bir_block->insts.size() ||
      argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      !argument.source_value_id.has_value() ||
      !argument.destination_stack_offset_bytes.has_value()) {
    return false;
  }
  const auto* call =
      std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
  if (call == nullptr || argument.arg_index >= call->arg_abi.size()) {
    return false;
  }
  const auto& abi = call->arg_abi[argument.arg_index];
  return abi.type == bir::TypeKind::Ptr &&
         abi.byval_copy &&
         !abi.sret_pointer &&
         abi.primary_class == bir::AbiValueClass::Integer &&
         abi.size_bytes > 0 &&
         abi.size_bytes <= 16;
}

std::vector<module::MachineInstruction> lower_before_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BeforeCall,
      context.block_index,
      instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };
  if (bundle == nullptr) {
    bundle = &synthetic_bundle;
  }
  if (context.bir_block != nullptr && instruction_index < context.bir_block->insts.size()) {
    if (const auto* call =
            std::get_if<bir::CallInst>(&context.bir_block->insts[instruction_index]);
        call != nullptr) {
      const std::size_t outgoing_bytes =
          outgoing_stack_argument_bytes(*call, call_plan);
      if (outgoing_bytes > 0) {
        lowered.push_back(
            make_outgoing_stack_base_instruction(context, instruction_index, outgoing_bytes));
      }
    }
  }
  for (const auto& preserved : call_plan.preserved_values) {
    if (auto instruction = make_callee_saved_preservation_home_population(
            context, call_plan, *bundle, preserved, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  std::vector<std::size_t> lowered_stack_byval_args;
  for (const auto& move : bundle->moves) {
    if (auto instruction =
            lower_before_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      if (move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
          move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
          move.reason == "call_arg_byval_aggregate_register_lanes" &&
          move.destination_abi_index.has_value()) {
        lowered_stack_byval_args.push_back(*move.destination_abi_index);
      }
      lowered.push_back(std::move(*instruction));
    }
  }
  for (const auto& argument : call_plan.arguments) {
    if (!prepared_argument_is_small_byval_stack_lane(context, argument, instruction_index) ||
        std::find(lowered_stack_byval_args.begin(),
                  lowered_stack_byval_args.end(),
                  argument.arg_index) != lowered_stack_byval_args.end()) {
      continue;
    }
    prepare::PreparedMoveResolution move{
        .from_value_id = *argument.source_value_id,
        .to_value_id = *argument.source_value_id,
        .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
        .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
        .destination_abi_index = argument.arg_index,
        .destination_stack_offset_bytes = argument.destination_stack_offset_bytes,
        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
        .reason = "call_arg_byval_aggregate_register_lanes",
    };
    if (auto instruction =
            lower_before_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  for (const auto& binding : bundle->abi_bindings) {
    if (auto instruction =
            lower_before_call_immediate_binding(
                context, call_plan, *bundle, binding, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_after_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::AfterCall,
      context.block_index,
      instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .phase = prepare::PreparedMovePhase::BeforeInstruction,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };
  if (bundle != nullptr) {
    for (const auto& move : bundle->moves) {
      if (auto instruction =
              lower_after_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
        lowered.push_back(std::move(*instruction));
      }
    }
  }

  const auto& republication_bundle =
      bundle != nullptr ? *bundle : synthetic_bundle;
  for (const auto& preserved : call_plan.preserved_values) {
    if (auto instruction = make_callee_saved_preservation_home_republication(
            context,
            call_plan,
            republication_bundle,
            preserved,
            instruction_index,
            diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_before_return_moves(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr) {
    return lowered;
  }
  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BeforeReturn,
      context.block_index,
      instruction_index);
  if (bundle == nullptr) {
    return lowered;
  }

  for (const auto& move : bundle->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
      continue;
    }
    const auto* source_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.from_value_id);
    if (source_home == nullptr || !move.destination_register_name.has_value()) {
      continue;
    }

    const auto expected_view = scalar_view_from_register_name(move.destination_register_name);
    const auto destination_bank =
        move.destination_register_placement.has_value()
            ? std::optional<prepare::PreparedRegisterBank>{
                  move.destination_register_placement->bank}
            : std::nullopt;
    auto destination = make_register_operand_from_prepared_authority(
        move.destination_register_name,
        move.destination_register_placement,
        destination_bank,
        RegisterOperandRole::CallAbi,
        move.to_value_id != 0 ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                              : std::nullopt,
        source_home->value_name,
        move.destination_contiguous_width,
        move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      continue;
    }

    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow->function_name,
        .phase = bundle->phase,
        .authority_kind = bundle->authority_kind,
        .block_index = bundle->block_index,
        .instruction_index = bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            bundle->source_parallel_copy_successor_label,
        .move = move,
        .destination_register = *destination,
        .source_bundle = bundle,
        .source_move = &move,
    };
    if (source_home->kind == prepare::PreparedValueHomeKind::Register &&
        source_home->register_name.has_value()) {
      const auto source_register_name =
          register_name_with_expected_view(source_home->register_name, expected_view);
      auto source = make_register_operand_from_prepared_authority(
          source_register_name,
          std::nullopt,
          destination_bank,
          RegisterOperandRole::CallAbi,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_register = *source;
    } else if (source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
               source_home->offset_bytes.has_value()) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block != nullptr
                             ? context.control_flow_block->block_label
                             : c4c::kInvalidBlockLabel,
          .instruction_index = instruction_index,
          .result_value_id = source_home->value_id,
          .result_value_name = source_home->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home->slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home->size_bytes.value_or(
              scalar_size_from_register_view(expected_view)),
          .align_bytes = source_home->align_bytes.value_or(
              scalar_size_from_register_view(expected_view)),
          .can_use_base_plus_offset = true,
      };
    } else {
      continue;
    }
    lowered.push_back(make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
  }
  return lowered;
}

std::vector<module::MachineInstruction> lower_value_moves(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      (phase != prepare::PreparedMovePhase::BlockEntry &&
       phase != prepare::PreparedMovePhase::BeforeInstruction)) {
    return lowered;
  }
  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      phase,
      context.block_index,
      instruction_index);
  const prepare::PreparedMoveBundle synthetic_bundle{
      .function_name = context.function.control_flow->function_name,
      .phase = phase,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
  };

  if (bundle != nullptr) {
    for (const auto& move : bundle->moves) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
      continue;
    }
    const auto* destination_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.to_value_id);
    if (destination_home == nullptr) {
      continue;
    }
    const auto* source_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.from_value_id);
    if (move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
        destination_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      if (auto stack_move = make_value_stack_move_instruction(
              context,
              *bundle,
              move,
              *destination_home,
              source_home,
              instruction_index)) {
        lowered.push_back(std::move(*stack_move));
      }
      continue;
    }
    if (move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register) {
      continue;
    }
    const auto destination_register_view =
        scalar_view_from_register_name(destination_home->register_name);
    const auto expected_view =
        destination_register_view.has_value()
            ? destination_register_view
            : (destination_home->size_bytes.has_value()
                   ? scalar_integer_register_view_from_size(*destination_home->size_bytes)
                   : std::nullopt);
    const auto destination_bank =
        expected_view == std::optional<abi::RegisterView>{abi::RegisterView::S} ||
                expected_view == std::optional<abi::RegisterView>{abi::RegisterView::D}
            ? prepare::PreparedRegisterBank::Fpr
            : prepare::PreparedRegisterBank::Gpr;
    auto destination = make_register_operand_from_prepared_authority(
        destination_home->register_name,
        move.destination_register_placement,
        destination_bank,
        RegisterOperandRole::StoragePlan,
        destination_home->value_id,
        destination_home->value_name,
        move.destination_contiguous_width,
        move.destination_occupied_register_names,
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!destination.has_value()) {
      continue;
    }

    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow->function_name,
        .phase = phase,
        .authority_kind = bundle->authority_kind,
        .block_index = bundle->block_index,
        .instruction_index = bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            bundle->source_parallel_copy_successor_label,
        .move = move,
        .destination_register = *destination,
        .source_bundle = bundle,
        .source_move = &move,
    };
    if (move.source_immediate_i32.has_value()) {
      const auto immediate = make_scalar_call_argument_immediate(
          bir::Value::immediate_i32(static_cast<std::int32_t>(*move.source_immediate_i32)),
          move.from_value_id);
      if (!immediate.has_value()) {
        continue;
      }
      move_record.source_immediate = immediate;
    } else if (const auto* prior_stack_preserved =
                   phase == prepare::PreparedMovePhase::BeforeInstruction
                       ? find_prior_stack_preserved_value_before_instruction(
                             context, move.from_value_id, instruction_index)
                       : nullptr;
               prior_stack_preserved != nullptr) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = prior_stack_preserved->value_id,
          .result_value_name = prior_stack_preserved->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = prior_stack_preserved->slot_id,
          .byte_offset =
              static_cast<std::int64_t>(*prior_stack_preserved->stack_offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = *prior_stack_preserved->stack_size_bytes,
          .align_bytes = prior_stack_preserved->stack_align_bytes.value_or(
              *prior_stack_preserved->stack_size_bytes),
          .can_use_base_plus_offset = true,
      };
    } else if (source_home != nullptr &&
               source_home->kind == prepare::PreparedValueHomeKind::Register &&
               source_home->register_name.has_value()) {
      auto source = make_register_operand_from_prepared_authority(
          source_home->register_name,
          std::nullopt,
          destination_bank,
          RegisterOperandRole::StoragePlan,
          source_home->value_id,
          source_home->value_name,
          1,
          {},
          expected_view,
          diagnostics,
          context,
          instruction_index);
      if (!source.has_value()) {
        continue;
      }
      move_record.source_register = *source;
    } else if (source_home != nullptr &&
               source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
               source_home->offset_bytes.has_value()) {
      move_record.source_memory = MemoryOperand{
          .surface = RecordSurfaceKind::MachineInstructionNode,
          .support = MemoryOperandSupportKind::Prepared,
          .function_name = context.function.control_flow->function_name,
          .block_label = context.control_flow_block->block_label,
          .instruction_index = instruction_index,
          .result_value_id = source_home->value_id,
          .result_value_name = source_home->value_name,
          .base_kind = MemoryBaseKind::FrameSlot,
          .frame_slot_id = source_home->slot_id,
          .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
          .byte_offset_is_prepared_snapshot = true,
          .size_bytes = source_home->size_bytes.value_or(4),
          .align_bytes = source_home->align_bytes.value_or(4),
          .can_use_base_plus_offset = true,
      };
    } else {
      continue;
    }

    lowered.push_back(make_call_boundary_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
    }
  }

  if (phase != prepare::PreparedMovePhase::BlockEntry) {
    return lowered;
  }

  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return lowered;
  }

  std::vector<const prepare::PreparedCallPreservedValue*> selected_preserved;
  for (const auto& call : call_plans->calls) {
    if (call.block_index >= context.block_index) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
        continue;
      }
      auto existing = std::find_if(
          selected_preserved.begin(),
          selected_preserved.end(),
          [&](const prepare::PreparedCallPreservedValue* candidate) {
            return candidate->value_id == preserved.value_id;
          });
      if (existing == selected_preserved.end()) {
        selected_preserved.push_back(&preserved);
      } else {
        *existing = &preserved;
      }
    }
  }

  const auto& republication_bundle = bundle != nullptr ? *bundle : synthetic_bundle;
  for (const auto* preserved : selected_preserved) {
    if (preserved == nullptr ||
        !preserved_value_has_block_entry_non_call_use(context, *preserved)) {
      continue;
    }
    if (auto instruction =
            make_callee_saved_preservation_home_republication_instruction(
                context,
                republication_bundle,
                *preserved,
                prepare::PreparedMovePhase::BlockEntry,
                context.block_index,
                instruction_index,
                "callee_saved_preservation_home_block_entry_republication",
                diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

std::optional<module::MachineInstruction> lower_prepared_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper,
    module::ModuleLoweringDiagnostics& diagnostics) {
  CallInstructionRecord call_record{
      .wrapper_kind = call_plan.wrapper_kind,
      .variadic_fpr_arg_register_count = call_plan.variadic_fpr_arg_register_count,
      .memory_return = call_plan.memory_return,
      .memory_return_storage =
          call_plan.memory_return.has_value()
              ? make_memory_return_storage(context,
                                           *call_plan.memory_return,
                                           instruction_index)
              : std::nullopt,
      .prepared_indirect_callee = call_plan.indirect_callee,
      .prepared_arguments = call_plan.arguments,
      .prepared_result = call_plan.result,
      .preserved_values = call_plan.preserved_values,
      .clobbered_registers = call_plan.clobbered_registers,
      .source_call = &call_plan,
      .outgoing_stack_argument_bytes =
          outgoing_stack_argument_bytes(call_inst, call_plan),
      .source_variadic_entry = variadic_entry_plan,
      .source_variadic_helper_operand_homes = variadic_helper_operand_homes,
      .variadic_entry_helper = variadic_helper,
      .variadic_va_start_overflow_area_stack_offset_bytes =
          va_start_overflow_area_stack_offset(context, variadic_entry_plan, variadic_helper),
      .calling_convention = call_inst.calling_convention,
      .is_indirect = call_plan.is_indirect,
      .is_variadic =
          call_plan.wrapper_kind == prepare::PreparedCallWrapperKind::DirectExternVariadic ||
          call_inst.is_variadic,
      .is_noreturn = call_inst.is_noreturn,
  };

  if (call_plan.is_indirect) {
    if (!call_inst.is_indirect || !call_plan.indirect_callee.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 indirect call lowering requires matching retained BIR and prepared indirect callee facts");
      return std::nullopt;
    }
    auto callee = make_indirect_callee_register(
        context, *call_plan.indirect_callee, instruction_index, diagnostics);
    if (!callee.has_value()) {
      return std::nullopt;
    }
    call_record.indirect_callee = make_register_operand(*callee);
  } else {
    if (call_inst.is_indirect || !call_plan.direct_callee_name.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 direct call lowering requires matching retained BIR and prepared direct callee facts");
      return std::nullopt;
    }
    call_record.direct_callee = SymbolOperand{
        .link_name = call_inst.callee_link_name_id,
        .type = bir::TypeKind::Ptr,
        .is_extern = call_plan.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule,
    };
    call_record.direct_callee_label = *call_plan.direct_callee_name;
  }

  InstructionRecord target = make_call_instruction(std::move(call_record));
  target.function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName;
  target.block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

std::optional<MachineEffectResource> effect_from_prepared_call_clobber(
    const prepare::PreparedClobberedRegister& clobber) {
  if (clobber.register_name.empty() || clobber.contiguous_width == 0 ||
      clobber.bank == prepare::PreparedRegisterBank::None) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(clobber.bank);
  const auto expected_view = prepared_clobber_expected_view(clobber.bank);
  const auto converted_primary = abi::convert_prepared_register(
      clobber.register_name, clobber.bank, prepared_class, expected_view);
  if (!converted_primary.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> occupied_names = clobber.occupied_register_names;
  if (occupied_names.empty() && clobber.contiguous_width == 1) {
    occupied_names.push_back(clobber.register_name);
  }
  if (occupied_names.size() != clobber.contiguous_width) {
    return std::nullopt;
  }

  std::vector<abi::RegisterReference> occupied_refs;
  occupied_refs.reserve(occupied_names.size());
  for (const auto& occupied_name : occupied_names) {
    const auto converted_occupied = abi::convert_prepared_register(
        occupied_name, clobber.bank, prepared_class, expected_view);
    if (!converted_occupied.has_value()) {
      return std::nullopt;
    }
    occupied_refs.push_back(*converted_occupied.reg);
  }
  if (occupied_refs.empty() || occupied_refs.front() != *converted_primary.reg) {
    return std::nullopt;
  }

  const auto occupied_views = occupied_register_views(occupied_refs);
  if (occupied_views.size() != occupied_refs.size()) {
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted_primary.reg,
      .role = RegisterOperandRole::CallAbi,
      .prepared_class = prepared_class,
      .prepared_bank = clobber.bank,
      .expected_view = expected_view,
      .contiguous_width = clobber.contiguous_width,
      .occupied_register_references = occupied_refs,
      .occupied_registers = occupied_views,
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .reg = *converted_primary.reg,
  };
}

std::vector<MachineEffectResource> effects_from_prepared_call_clobbers(
    const std::vector<prepare::PreparedClobberedRegister>& clobbers) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(clobbers.size());
  for (const auto& clobber : clobbers) {
    if (auto effect = effect_from_prepared_call_clobber(clobber)) {
      effects.push_back(std::move(*effect));
    }
  }
  return effects;
}

std::optional<MachineEffectResource> effect_from_prepared_call_preserved_value(
    const prepare::PreparedCallPreservedValue& preserved) {
  if (preserved.route == prepare::PreparedCallPreservationRoute::StackSlot) {
    if (!preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0 ||
        !preserved.stack_align_bytes.has_value() ||
        *preserved.stack_align_bytes == 0) {
      return std::nullopt;
    }

    const OperandRecord operand = make_memory_operand(MemoryOperand{
        .support = MemoryOperandSupportKind::Prepared,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = preserved.slot_id,
        .byte_offset = static_cast<std::int64_t>(*preserved.stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *preserved.stack_size_bytes,
        .align_bytes = *preserved.stack_align_bytes,
        .can_use_base_plus_offset = true,
    });
    return MachineEffectResource{
        .kind = MachineEffectResourceKind::Memory,
        .operand = operand,
        .value_id = preserved.value_id,
        .value_name = preserved.value_name,
        .frame_slot_id = preserved.slot_id,
    };
  }
  if (preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !preserved.register_name.has_value() || !preserved.register_bank.has_value() ||
      preserved.register_name->empty() ||
      *preserved.register_bank == prepare::PreparedRegisterBank::None ||
      preserved.contiguous_width == 0) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(*preserved.register_bank);
  const auto expected_view = prepared_clobber_expected_view(*preserved.register_bank);
  const auto converted_primary = abi::convert_prepared_register(
      *preserved.register_name, *preserved.register_bank, prepared_class, expected_view);
  if (!converted_primary.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> occupied_names = preserved.occupied_register_names;
  if (occupied_names.empty() && preserved.contiguous_width == 1) {
    occupied_names.push_back(*preserved.register_name);
  }
  if (occupied_names.size() != preserved.contiguous_width) {
    return std::nullopt;
  }

  std::vector<abi::RegisterReference> occupied_refs;
  occupied_refs.reserve(occupied_names.size());
  for (const auto& occupied_name : occupied_names) {
    const auto converted_occupied = abi::convert_prepared_register(
        occupied_name, *preserved.register_bank, prepared_class, expected_view);
    if (!converted_occupied.has_value()) {
      return std::nullopt;
    }
    occupied_refs.push_back(*converted_occupied.reg);
  }
  if (occupied_refs.empty() || occupied_refs.front() != *converted_primary.reg) {
    return std::nullopt;
  }

  const auto occupied_views = occupied_register_views(occupied_refs);
  if (occupied_views.size() != occupied_refs.size()) {
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted_primary.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = *preserved.register_bank,
      .expected_view = expected_view,
      .contiguous_width = preserved.contiguous_width,
      .occupied_register_references = occupied_refs,
      .occupied_registers = occupied_views,
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .reg = *converted_primary.reg,
  };
}

std::vector<MachineEffectResource> effects_from_prepared_call_preserved_values(
    const std::vector<prepare::PreparedCallPreservedValue>& preserved_values) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(preserved_values.size());
  for (const auto& preserved : preserved_values) {
    if (auto effect = effect_from_prepared_call_preserved_value(preserved)) {
      effects.push_back(std::move(*effect));
    }
  }
  return effects;
}

namespace {

MachineEffectResource effect_from_operand(const OperandRecord& operand) {
  MachineEffectResource resource;
  resource.operand = operand;
  switch (operand.kind) {
    case OperandKind::Register: {
      const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Register;
      if (reg != nullptr) {
        resource.value_id = reg->value_id;
        resource.value_name = reg->value_name;
        resource.reg = reg->reg;
      }
      break;
    }
    case OperandKind::Immediate: {
      const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (immediate != nullptr) {
        resource.value_id = immediate->source_value_id;
        resource.value_name = immediate->source_value_name;
      }
      break;
    }
    case OperandKind::PreparedValue: {
      const auto* value = std::get_if<PreparedValueOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (value != nullptr) {
        resource.value_id = value->value_id;
        resource.value_name = value->value_name;
      }
      break;
    }
    case OperandKind::FrameSlot: {
      const auto* slot = std::get_if<FrameSlotOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::FrameSlot;
      if (slot != nullptr) {
        resource.frame_slot_id = slot->slot_id;
        if (slot->value_name.has_value()) {
          resource.value_name = *slot->value_name;
        }
      }
      break;
    }
    case OperandKind::Symbol: {
      const auto* symbol = std::get_if<SymbolOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Symbol;
      if (symbol != nullptr) {
        resource.symbol_name = symbol->link_name;
      }
      break;
    }
    case OperandKind::BranchTarget: {
      const auto* target = std::get_if<BranchTargetOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::BranchTarget;
      if (target != nullptr) {
        resource.value_id = target->condition_value_id;
        resource.block_label = target->block_label;
      }
      break;
    }
    case OperandKind::Memory: {
      const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Memory;
      if (memory != nullptr) {
        resource.value_id = memory->result_value_id.has_value() ? memory->result_value_id
                                                                : memory->stored_value_id;
        if (memory->result_value_name.has_value()) {
          resource.value_name = *memory->result_value_name;
        } else if (memory->stored_value_name.has_value()) {
          resource.value_name = *memory->stored_value_name;
        }
        resource.frame_slot_id = memory->frame_slot_id;
        resource.symbol_name = memory->symbol_name.has_value() ? memory->symbol_name
                                                               : memory->string_symbol_name;
      }
      break;
    }
  }
  return resource;
}

MachineEffectResource prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

std::vector<MachineEffectResource> effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(effect_from_operand(operand));
  }
  return effects;
}

MachineNodeStatusRecord call_boundary_move_selection_status(
    const CallBoundaryMoveInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr ||
      (instruction.source_move == nullptr && !instruction.source_immediate.has_value() &&
       !instruction.source_memory.has_value()) ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary move node is missing prepared move provenance"};
  }
  const bool selected_register_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_result_move =
      instruction.phase == prepare::PreparedMovePhase::AfterCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallResultAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_return_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeReturn &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_value_register_move =
      (instruction.phase == prepare::PreparedMovePhase::BlockEntry ||
       instruction.phase == prepare::PreparedMovePhase::BeforeInstruction) &&
      instruction.move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
      instruction.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_preservation_home_population =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
      instruction.move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      instruction.move.reason == "callee_saved_preservation_home_population";
  const bool stack_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::StackSlot &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  if (stack_argument_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary stack argument move requires AArch64 stack-copy lowering"};
  }
  if (!selected_register_argument_move && !selected_register_result_move &&
      !selected_register_return_move && !selected_value_register_move &&
      !selected_preservation_home_population) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node is outside the selected register call-boundary move subset"};
  }
  if ((!instruction.source_register.has_value() && !instruction.source_immediate.has_value() &&
       !instruction.source_memory.has_value()) ||
      !instruction.destination_register.has_value()) {
    const bool selected_f128_constant_argument_move =
        selected_register_argument_move &&
        !instruction.source_register.has_value() &&
        instruction.destination_register.has_value() &&
        instruction.destination_register->prepared_bank ==
            prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_f128_carrier != nullptr &&
        instruction.source_f128_carrier->source_type == bir::TypeKind::F128 &&
        instruction.source_f128_carrier->kind ==
            prepare::PreparedF128CarrierKind::Missing &&
        instruction.source_f128_carrier->missing_required_facts.empty() &&
        instruction.source_f128_carrier->total_size_bytes == 16 &&
        instruction.source_f128_carrier->total_align_bytes == 16 &&
        instruction.source_f128_carrier->constant_payload.has_value() &&
        instruction.source_f128_constant_payload.has_value() &&
        instruction.source_f128_constant_payload->low_bits ==
            instruction.source_f128_carrier->constant_payload->low_bits &&
        instruction.source_f128_constant_payload->high_bits ==
            instruction.source_f128_carrier->constant_payload->high_bits;
    if (selected_f128_constant_argument_move) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
    if (instruction.source_immediate.has_value() &&
        instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
  if (is_aggregate_register_lane_publication(instruction)) {
    if (instruction.source_memory->base_kind == MemoryBaseKind::PointerValue &&
        instruction.source_memory->base_register.has_value()) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (instruction.source_memory->base_kind == MemoryBaseKind::FrameSlot &&
        instruction.source_memory->frame_slot_id.has_value() &&
        instruction.source_memory->byte_offset_is_prepared_snapshot &&
        instruction.source_memory->can_use_base_plus_offset) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
  }
  if (instruction.source_memory.has_value() &&
        instruction.source_memory->support == MemoryOperandSupportKind::Prepared &&
      instruction.source_memory->base_kind == MemoryBaseKind::FrameSlot &&
      instruction.source_memory->frame_slot_id.has_value() &&
      instruction.source_memory->byte_offset_is_prepared_snapshot &&
      instruction.source_memory->can_use_base_plus_offset &&
      (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr ||
       instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr ||
       (instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_memory->size_bytes == 16))) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (instruction.source_memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared frame-slot source and GPR destination"};
  }
  if (!instruction.source_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
  if (instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const bool selected_scalar_fpr_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Fpr &&
      (instruction.source_register->expected_view == abi::RegisterView::S ||
       instruction.source_register->expected_view == abi::RegisterView::D) &&
      instruction.source_register->expected_view ==
          instruction.destination_register->expected_view;
  if (selected_scalar_fpr_register_move) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const auto* f128_carrier =
      instruction.source_f128_carrier != nullptr
          ? instruction.source_f128_carrier
          : instruction.destination_f128_carrier;
  const bool selected_f128_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.source_register->expected_view == abi::RegisterView::Q &&
      instruction.destination_register->expected_view == abi::RegisterView::Q &&
      f128_carrier != nullptr &&
      f128_carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
      f128_carrier->missing_required_facts.empty() &&
      f128_carrier->total_size_bytes == 16 && f128_carrier->total_align_bytes == 16;
  if (!selected_f128_register_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared GPR registers, scalar FPR registers, or structured f128 q-register authority"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_boundary_abi_binding_selection_status(
    const CallBoundaryAbiBindingInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr || instruction.source_binding == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary ABI binding node is missing prepared binding provenance"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_selection_status(const CallInstructionRecord& instruction) {
  if (auto variadic_status = variadic_call_selection_status(instruction)) {
    return *variadic_status;
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

mir::TargetInstructionPrintResult target_unsupported(std::string diagnostic) {
  return mir::target_instruction_unsupported(std::move(diagnostic));
}

mir::TargetInstructionPrintResult target_printed(std::vector<std::string> lines) {
  return mir::target_instruction_lines_printed(std::move(lines));
}

std::string bad_header(const InstructionRecord& instruction) {
  return std::string("cannot print AArch64 machine node family=") +
         std::string(instruction_family_name(instruction.family)) + " opcode=" +
         std::string(machine_opcode_name(instruction.opcode)) + ": ";
}

std::string_view required_primary_mnemonic(const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::string register_name(const RegisterOperand& operand) {
  return abi::register_name(operand.reg);
}

bool same_gp_register_index(abi::RegisterReference lhs, abi::RegisterReference rhs) {
  return lhs.index == rhs.index && abi::is_gp_register(lhs) && abi::is_gp_register(rhs);
}

std::optional<std::size_t> call_boundary_load_width_bytes(
    const RegisterOperand& destination) {
  switch (destination.reg.view) {
    case abi::RegisterView::S:
    case abi::RegisterView::W:
      return 4U;
    case abi::RegisterView::D:
    case abi::RegisterView::X:
      return 8U;
    case abi::RegisterView::Q:
      return 16U;
    default:
      return std::nullopt;
  }
}

bool call_boundary_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t load_width_bytes) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0 ||
      load_width_bytes == 0) {
    return false;
  }
  if (load_width_bytes != 1 && load_width_bytes != 2 && load_width_bytes != 4 &&
      load_width_bytes != 8 && load_width_bytes != 16) {
    return false;
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  return offset % load_width_bytes == 0 && offset / load_width_bytes <= 4095U;
}

std::optional<abi::RegisterReference> call_boundary_address_scratch_register(
    abi::RegisterReference destination) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (!same_gp_register_index(scratch, destination)) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

std::vector<std::string> materialize_call_boundary_frame_slot_address_lines(
    abi::RegisterReference scratch,
    const MemoryOperand& memory) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0) {
    return {};
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  const std::string scratch_name = abi::register_name(scratch);
  if (offset <= 4095U) {
    return {"add " + scratch_name + ", sp, #" + std::to_string(offset)};
  }
  auto lines = materialize_integer_constant_lines(scratch, offset, 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + scratch_name + ", sp, " + scratch_name);
  return lines;
}

std::optional<std::vector<std::string>> print_aggregate_register_lane_publication_lines(
    const CallBoundaryMoveInstructionRecord& move) {
  if (!is_aggregate_register_lane_publication(move)) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  const auto scratch = aggregate_register_lane_scratch(*move.destination_register);
  if (!scratch.has_value()) {
    return std::nullopt;
  }
  const std::string scratch_x = abi::register_name(*scratch);
  std::size_t remaining = move.source_memory->size_bytes;
  std::size_t lane_index = 0;
  std::size_t source_offset = 0;
  while (remaining > 0) {
    const std::size_t lane_bytes = std::min<std::size_t>(remaining, 8);
    const auto lane_register =
        aggregate_register_lane_destination(*move.destination_register, lane_index);
    if (!lane_register.has_value()) {
      return std::nullopt;
    }
    std::size_t lane_offset = 0;
    while (lane_offset < lane_bytes) {
      const auto printable_chunk =
          aggregate_register_lane_printable_chunk(*move.source_memory,
                                                  source_offset + lane_offset,
                                                  lane_bytes - lane_offset);
      if (!printable_chunk.has_value()) {
        return std::nullopt;
      }
      const std::size_t chunk_width = *printable_chunk;
      const auto chunk_memory =
          aggregate_register_lane_memory(*move.source_memory,
                                         source_offset + lane_offset,
                                         chunk_width);
      const auto mnemonic = aggregate_register_lane_load_mnemonic(chunk_width);
      if (mnemonic.empty()) {
        return std::nullopt;
      }
      const bool first_chunk = lane_offset == 0;
      const auto load_register =
          first_chunk
              ? aggregate_register_lane_load_register(*lane_register, chunk_width)
              : aggregate_register_lane_load_register(*scratch, chunk_width);
      lines.push_back(std::string{mnemonic} + " " +
                      std::string{abi::register_name(load_register)} + ", " +
                      memory_address(chunk_memory));
      if (!first_chunk) {
        lines.push_back("orr " + std::string{abi::register_name(*lane_register)} +
                        ", " + std::string{abi::register_name(*lane_register)} +
                        ", " + scratch_x + ", lsl #" +
                        std::to_string(lane_offset * 8));
      }
      lane_offset += chunk_width;
    }
    remaining -= lane_bytes;
    source_offset += lane_bytes;
    ++lane_index;
  }
  return lines;
}

std::optional<std::vector<std::string>> print_call_boundary_frame_slot_load_lines(
    const CallBoundaryMoveInstructionRecord& move) {
  if (!move.source_memory.has_value() || !move.destination_register.has_value()) {
    return std::nullopt;
  }
  std::string address;
  std::vector<std::string> lines;
  const auto load_width = call_boundary_load_width_bytes(*move.destination_register);
  if (!load_width.has_value()) {
    return std::nullopt;
  }
  if (call_boundary_frame_slot_direct_offset_is_encodable(*move.source_memory,
                                                          *load_width)) {
    address = memory_address(*move.source_memory);
  } else {
    const auto scratch =
        call_boundary_address_scratch_register(move.destination_register->reg);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    lines =
        materialize_call_boundary_frame_slot_address_lines(*scratch, *move.source_memory);
    if (lines.empty()) {
      return std::nullopt;
    }
    address = "[" + std::string{abi::register_name(*scratch)} + "]";
  }
  if (address.empty()) {
    return std::nullopt;
  }
  lines.push_back("ldr " + register_name(*move.destination_register) + ", " + address);
  return lines;
}

std::optional<std::string> f128_call_boundary_vector_register_name(
    const RegisterOperand& operand) {
  if (operand.expected_view != abi::RegisterView::Q ||
      operand.prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      operand.prepared_class != prepare::PreparedRegisterClass::Vector ||
      operand.contiguous_width != 1 ||
      !abi::is_fp_simd_register(operand.reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::fp_simd_register(operand.reg.index, abi::RegisterView::V);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return std::string{abi::register_name(*viewed)} + ".16b";
}

std::optional<unsigned> scalar_integer_width_bits(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::uint64_t scalar_integer_immediate_bits(const ImmediateOperand& immediate,
                                            unsigned width_bits) {
  if (width_bits == 32U) {
    return static_cast<std::uint32_t>(immediate.unsigned_value);
  }
  return immediate.unsigned_value;
}

bool is_single_move_wide_immediate(std::uint64_t value, unsigned width_bits) {
  const unsigned chunks = width_bits == 64U ? 4U : 2U;
  unsigned nonzero_chunks = 0;
  for (unsigned chunk = 0; chunk < chunks; ++chunk) {
    if (((value >> (chunk * 16U)) & 0xffffU) != 0U) {
      ++nonzero_chunks;
    }
  }
  return nonzero_chunks <= 1U;
}

[[nodiscard]] const bir::CastInst* find_same_block_cast_producer(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  const auto spelling = context.function.prepared->names.value_names.spelling(value_name);
  if (spelling.empty()) {
    return nullptr;
  }
  const auto limit = std::min(before_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto* cast = std::get_if<bir::CastInst>(&context.bir_block->insts[index]);
    if (cast != nullptr && cast->result.kind == bir::Value::Kind::Named &&
        cast->result.name == spelling) {
      return cast;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<unsigned> integer_width_bits_for_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits) {
  if (value.immediate_bits != 0U) {
    return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate_bits)
                             : value.immediate_bits;
  }
  return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate)
                           : static_cast<std::uint64_t>(value.immediate);
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::fp_simd_register(reg.index, abi::RegisterView::S);
    case bir::TypeKind::F64:
      return abi::fp_simd_register(reg.index, abi::RegisterView::D);
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_gp_register_view(
    abi::RegisterReference reg,
    unsigned width_bits) {
  if (width_bits == 32U) {
    return abi::gp_register(reg.index, abi::RegisterView::W);
  }
  if (width_bits == 64U) {
    return abi::gp_register(reg.index, abi::RegisterView::X);
  }
  return std::nullopt;
}

bool append_materialize_fp_immediate(std::vector<std::string>& lines,
                                     const bir::Value& value,
                                     abi::RegisterReference gp_scratch_base,
                                     abi::RegisterReference fp_scratch_base,
                                     abi::RegisterReference& fp_scratch) {
  const auto fp_view = scalar_fp_register_view(fp_scratch_base, value.type);
  if (!fp_view.has_value() || value.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  fp_scratch = *fp_view;
  if (value.type == bir::TypeKind::F32) {
    const auto gp_scratch = abi::gp_register(gp_scratch_base.index, abi::RegisterView::W);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 32);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  if (value.type == bir::TypeKind::F64) {
    const auto gp_scratch = abi::gp_register(gp_scratch_base.index, abi::RegisterView::X);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 64);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  return false;
}

[[nodiscard]] std::optional<std::vector<std::string>>
make_immediate_cast_call_argument_publication_lines(
    const bir::CastInst& cast,
    const RegisterOperand& destination) {
  if (cast.operand.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto gp_scratch_base = abi::reserved_mir_scratch_gp_registers().front();
  const auto fp_scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
  std::vector<std::string> lines;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI: {
      if (!abi::is_gp_register(destination.reg)) {
        return std::nullopt;
      }
      abi::RegisterReference fp_source{};
      if (!append_materialize_fp_immediate(
              lines, cast.operand, gp_scratch_base, fp_scratch_base, fp_source)) {
        return std::nullopt;
      }
      const auto width_bits = integer_width_bits_for_type(cast.result.type);
      const auto viewed_destination =
          width_bits.has_value()
              ? scalar_gp_register_view(destination.reg, *width_bits)
              : std::nullopt;
      if (!viewed_destination.has_value()) {
        return std::nullopt;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::FPToSI ? "fcvtzs "
                                                                         : "fcvtzu "} +
                      std::string{abi::register_name(*viewed_destination)} + ", " +
                      std::string{abi::register_name(fp_source)});
      return lines;
    }
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP: {
      if (!abi::is_fp_simd_register(destination.reg)) {
        return std::nullopt;
      }
      const auto source_width = integer_width_bits_for_type(cast.operand.type);
      const auto gp_scratch = source_width.has_value()
                                  ? scalar_gp_register_view(gp_scratch_base, *source_width)
                                  : std::nullopt;
      const auto fp_destination = scalar_fp_register_view(destination.reg, cast.result.type);
      if (!source_width.has_value() || !gp_scratch.has_value() ||
          !fp_destination.has_value()) {
        return std::nullopt;
      }
      auto materialize = materialize_integer_constant_lines(
          *gp_scratch, immediate_integer_bits(cast.operand, *source_width), *source_width);
      if (materialize.empty()) {
        return std::nullopt;
      }
      lines.insert(lines.end(), materialize.begin(), materialize.end());
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                         : "ucvtf "} +
                      std::string{abi::register_name(*fp_destination)} + ", " +
                      std::string{abi::register_name(*gp_scratch)});
      return lines;
    }
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPTrunc: {
      if (!abi::is_fp_simd_register(destination.reg)) {
        return std::nullopt;
      }
      abi::RegisterReference fp_source{};
      if (!append_materialize_fp_immediate(
              lines, cast.operand, gp_scratch_base, fp_scratch_base, fp_source)) {
        return std::nullopt;
      }
      const auto fp_destination = scalar_fp_register_view(destination.reg, cast.result.type);
      if (!fp_destination.has_value()) {
        return std::nullopt;
      }
      lines.push_back("fcvt " + std::string{abi::register_name(*fp_destination)} +
                      ", " + std::string{abi::register_name(fp_source)});
      return lines;
    }
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_immediate_cast_call_argument_publication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& destination,
    std::size_t instruction_index) {
  const auto* cast =
      find_same_block_cast_producer(context, source_home.value_name, instruction_index);
  if (cast == nullptr) {
    return std::nullopt;
  }
  const auto lines =
      make_immediate_cast_call_argument_publication_lines(*cast, destination);
  if (!lines.has_value() || lines->empty()) {
    return std::nullopt;
  }
  std::string asm_text;
  for (std::size_t index = 0; index < lines->size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += (*lines)[index];
  }
  const auto destination_operand = make_register_operand(destination);
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .operands = {destination_operand},
      .defs = {effect_from_operand(destination_operand)},
      .uses = {MachineEffectResource{
          .kind = MachineEffectResourceKind::PreparedValue,
          .value_id = source_home.value_id,
          .value_name = source_home.value_name,
      }},
      .side_effects = {MachineSideEffectKind::InlineAssembly},
      .payload =
          AssemblerInstructionRecord{
              .operands = {destination_operand},
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_call_boundary_machine_instruction(context, instruction_index, std::move(target));
}

}  // namespace

InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.destination_register.has_value()) {
    const auto destination = make_register_operand(*instruction.destination_register);
    operands.push_back(destination);
    defs.push_back(effect_from_operand(destination));
  } else if (instruction.move.to_value_id != 0) {
    defs.push_back(prepared_value_def(instruction.move.to_value_id, c4c::kInvalidValueName));
  }
  if (instruction.source_register.has_value()) {
    const auto source = make_register_operand(*instruction.source_register);
    operands.push_back(source);
    uses.push_back(effect_from_operand(source));
  } else if (instruction.source_memory.has_value()) {
    const auto source = make_memory_operand(*instruction.source_memory);
    operands.push_back(source);
    if (!instruction.source_memory_materializes_address) {
      uses.push_back(effect_from_operand(source));
    }
  } else if (instruction.source_immediate.has_value()) {
    const auto source = make_immediate_operand(*instruction.source_immediate);
    operands.push_back(source);
    uses.push_back(effect_from_operand(source));
  } else if (instruction.move.from_value_id != 0) {
    uses.push_back(prepared_value_def(instruction.move.from_value_id, c4c::kInvalidValueName));
  }
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryMove,
      .selection = call_boundary_move_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .payload = instruction,
  };
}

InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryAbiBinding,
      .selection = call_boundary_abi_binding_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .payload = instruction,
  };
}

InstructionRecord make_call_instruction(CallInstructionRecord instruction) {
  complete_variadic_call_record(instruction);

  std::vector<OperandRecord> operands = instruction.arguments;
  if (instruction.indirect_callee.has_value()) {
    operands.insert(operands.begin(), *instruction.indirect_callee);
  } else if (instruction.direct_callee.has_value()) {
    operands.insert(operands.begin(), make_symbol_operand(*instruction.direct_callee));
  }
  std::vector<MachineEffectResource> defs;
  if (instruction.result.has_value()) {
    defs.push_back(effect_from_operand(*instruction.result));
  }
  std::vector<MachineEffectResource> uses = effects_from_operands(operands);
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(effect_from_operand(make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_start.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_va_start->destination_va_list.value_id,
        instruction.variadic_va_start->destination_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_scalar_va_arg.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_scalar_va_arg->result_home.value_id,
        instruction.variadic_scalar_va_arg->result_home.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_scalar_va_arg->source_va_list.value_id,
        instruction.variadic_scalar_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_aggregate_va_arg.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_id,
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_aggregate_va_arg->source_va_list.value_id,
        instruction.variadic_aggregate_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_copy.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_va_copy->destination_va_list.value_id,
        instruction.variadic_va_copy->destination_va_list.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_va_copy->source_va_list.value_id,
        instruction.variadic_va_copy->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = instruction.variadic_va_start.has_value()
                    ? MachineOpcode::VariadicVaStart
                    : (instruction.variadic_scalar_va_arg.has_value()
                           ? MachineOpcode::VariadicVaArgScalar
                           : (instruction.variadic_aggregate_va_arg.has_value()
                                  ? MachineOpcode::VariadicVaArgAggregate
                                  : (instruction.variadic_va_copy.has_value()
                                         ? MachineOpcode::VariadicVaCopy
                                         : (instruction.is_indirect
                                                ? MachineOpcode::IndirectCall
                                                : MachineOpcode::DirectCall)))),
      .selection = call_selection_status(instruction),
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .preserves = effects_from_prepared_call_preserved_values(instruction.preserved_values),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

mir::TargetInstructionPrintResult print_call(const InstructionRecord& instruction,
                                             const CallInstructionRecord& call) {
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (auto variadic_print = print_variadic_call(call, bad_header(instruction), mnemonic)) {
    return *variadic_print;
  }
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) + "call mnemonic is not printable");
  }
  if (call.is_indirect) {
    if (!call.indirect_callee.has_value() ||
        !call.prepared_indirect_callee.has_value() || call.source_call == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call node is missing prepared callee provenance");
    }
    const auto* callee = std::get_if<RegisterOperand>(&call.indirect_callee->payload);
    if (call.indirect_callee->kind != OperandKind::Register || callee == nullptr) {
      return target_unsupported(bad_header(instruction) +
                                "indirect call callee is not a register operand");
    }
    std::ostringstream out;
    out << mnemonic << " " << register_name(*callee);
    if (call.outgoing_stack_argument_bytes == 0) {
      return target_printed({out.str()});
    }
    return target_printed({
        "sub sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
        out.str(),
        "add sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
    });
  }
  if (!call.direct_callee.has_value() || call.direct_callee_label.empty() ||
      call.source_call == nullptr || !call.wrapper_kind.has_value()) {
    return target_unsupported(bad_header(instruction) +
                              "direct call node is missing prepared callee provenance");
  }

  std::ostringstream out;
  out << mnemonic << " " << call.direct_callee_label;
  if (call.outgoing_stack_argument_bytes == 0) {
    return target_printed({out.str()});
  }
  return target_printed({
      "sub sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
      out.str(),
      "add sp, sp, #" + std::to_string(call.outgoing_stack_argument_bytes),
  });
}

mir::TargetInstructionPrintResult print_call_boundary_move(
    const InstructionRecord& instruction,
    const CallBoundaryMoveInstructionRecord& move) {
  if (move.source_bundle == nullptr ||
      (move.source_move == nullptr && !move.source_immediate.has_value() &&
       !move.source_memory.has_value())) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move node is missing prepared move provenance");
  }
    if ((!move.source_register.has_value() && !move.source_immediate.has_value() &&
         !move.source_memory.has_value()) ||
        !move.destination_register.has_value()) {
      return target_unsupported(
          bad_header(instruction) +
          "call-boundary move node requires prepared register source and destination");
    }
  if (const auto lines = print_aggregate_register_lane_publication_lines(move);
      lines.has_value()) {
    return target_printed(*lines);
  }
    if (move.source_memory.has_value()) {
      if (move.source_memory->support != MemoryOperandSupportKind::Prepared ||
          move.source_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !move.source_memory->frame_slot_id.has_value() ||
        !move.source_memory->byte_offset_is_prepared_snapshot ||
        !move.source_memory->can_use_base_plus_offset) {
      return target_unsupported(
          bad_header(instruction) +
          "call-boundary frame-slot move requires a prepared frame-slot address");
    }
    if (move.source_memory_materializes_address) {
      const auto lines = materialize_call_boundary_frame_slot_address_lines(
          move.destination_register->reg, *move.source_memory);
      if (lines.empty()) {
        return target_unsupported(bad_header(instruction) +
                                  "call-boundary frame-slot address is not printable");
      }
      return target_printed(lines);
    }
    const auto lines = print_call_boundary_frame_slot_load_lines(move);
    if (!lines.has_value()) {
      return target_unsupported(bad_header(instruction) +
                                "call-boundary frame-slot address is not printable");
    }
    return target_printed(*lines);
  }
  const auto mnemonic = required_primary_mnemonic(instruction);
  if (mnemonic.empty()) {
    return target_unsupported(bad_header(instruction) +
                              "call-boundary move mnemonic is not printable");
  }
  if (!move.source_register.has_value() && move.source_immediate.has_value()) {
    const auto width_bits = scalar_integer_width_bits(move.source_immediate->type);
    if (!width_bits.has_value() || !abi::is_gp_register(move.destination_register->reg)) {
      return target_unsupported(
          bad_header(instruction) +
          "call-boundary immediate move requires scalar integer GPR materialization");
    }
    const auto value =
        scalar_integer_immediate_bits(*move.source_immediate, *width_bits);
    if (!is_single_move_wide_immediate(value, *width_bits)) {
      return target_printed(materialize_integer_constant_lines(
          move.destination_register->reg, value, *width_bits));
    }
  }
  std::ostringstream out;
  const bool f128_q_register_move =
      move.source_register.has_value() &&
      move.source_f128_carrier != nullptr &&
      move.source_register->expected_view == abi::RegisterView::Q &&
      move.destination_register->expected_view == abi::RegisterView::Q;
  if (f128_q_register_move) {
    const auto destination =
        f128_call_boundary_vector_register_name(*move.destination_register);
    const auto source =
        f128_call_boundary_vector_register_name(*move.source_register);
    if (!destination.has_value() || !source.has_value()) {
      return target_unsupported(
          bad_header(instruction) +
          "binary128 call-boundary q-register move is not printable");
    }
    out << "mov " << *destination << ", " << *source;
    return target_printed({out.str()});
  }
  const bool scalar_fp_register_move =
      move.source_register.has_value() &&
      abi::is_fp_simd_register(move.source_register->reg) &&
      abi::is_fp_simd_register(move.destination_register->reg) &&
      (move.source_register->reg.view == abi::RegisterView::S ||
       move.source_register->reg.view == abi::RegisterView::D) &&
      move.source_register->reg.view == move.destination_register->reg.view;
  out << (scalar_fp_register_move ? "fmov" : mnemonic) << " "
      << register_name(*move.destination_register) << ", ";
  if (move.source_register.has_value()) {
    out << register_name(*move.source_register);
  } else {
    out << "#" << move.source_immediate->signed_value;
  }
  return target_printed({out.str()});
}

mir::TargetInstructionPrintResult print_call_boundary_abi_binding(
    const InstructionRecord& instruction,
    const CallBoundaryAbiBindingInstructionRecord& binding) {
  if (binding.source_bundle == nullptr || binding.source_binding == nullptr) {
    return target_unsupported(
        bad_header(instruction) +
        "call-boundary ABI binding node is missing prepared binding provenance");
  }
  return target_unsupported(
      bad_header(instruction) +
      "call-boundary ABI binding node requires later AArch64 move lowering");
}


}  // namespace c4c::backend::aarch64::codegen
