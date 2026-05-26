#include "calls.hpp"
#include "dispatch_diagnostics.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

constexpr std::size_t kStackPointerAlignmentBytes = 16;

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

}  // namespace

[[nodiscard]] std::size_t outgoing_stack_argument_bytes(
    const prepare::PreparedCallPlan& call_plan) {
  std::size_t bytes = 0;
  for (const auto& argument : call_plan.arguments) {
    if (!argument.destination_stack_offset_bytes.has_value() ||
        !argument.destination_stack_size_bytes.has_value()) {
      continue;
    }
    bytes = std::max(bytes,
                     *argument.destination_stack_offset_bytes +
                         *argument.destination_stack_size_bytes);
  }
  return align_to(bytes, kStackPointerAlignmentBytes);
}

[[nodiscard]] abi::RegisterReference outgoing_stack_argument_base_register() {
  return abi::x_register(16);
}

namespace {

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

}  // namespace

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

namespace {

[[nodiscard]] prepare::PreparedRegisterClass register_class_from_bank(
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

}  // namespace

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

[[nodiscard]] const prepare::PreparedF128Carrier*
find_prepared_f128_carrier_in_module(const prepare::PreparedBirModule& prepared,
                                     prepare::PreparedValueId value_id) {
  for (const auto& function_carriers : prepared.f128_carriers.functions) {
    if (const auto* carrier =
            prepare::find_prepared_f128_carrier(function_carriers, value_id)) {
      return carrier;
    }
  }
  return nullptr;
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
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
      return ImmediateOperand{
          .kind = ImmediateKind::UnsignedInteger,
          .type = value.type,
          .signed_value = value.immediate,
          .unsigned_value = value.immediate_bits,
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
    case 2:
      return bir::TypeKind::I16;
    case 4:
      return bir::TypeKind::I32;
    case 8:
      return bir::TypeKind::I64;
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::codegen
