#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "calls.hpp"
#include "comparison.hpp"
#include "memory.hpp"
#include "returns.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string unsupported_terminator_message(
    c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "AArch64 block dispatch visited unsupported prepared return terminator";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "AArch64 block dispatch visited prepared branch terminator; semantic lowering is not implemented";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "AArch64 block dispatch visited prepared conditional branch terminator; semantic lowering is not implemented";
  }
  return "AArch64 block dispatch visited unsupported prepared terminator";
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] module::InstructionLoweringFamily classify_instruction(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> module::InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return module::InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return module::InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return module::InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return module::InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return module::InstructionLoweringFamily::Memory;
        }
        return module::InstructionLoweringFamily::Unknown;
      },
      inst);
}

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = classify_instruction(inst),
      .message =
          "AArch64 block dispatch visited unsupported prepared BIR instruction family",
  });
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

void append_address_materialization_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Scalar,
      .message = std::move(message),
  });
}

void append_memory_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Memory,
      .message = std::move(message),
  });
}

void append_i128_transport_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Memory,
      .message = std::move(message),
  });
}

void append_i128_pair_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Scalar,
      .message = std::move(message),
  });
}

[[nodiscard]] std::optional<prepare::PreparedVariadicEntryHelperKind>
variadic_entry_helper_kind(std::string_view callee) {
  if (callee == "llvm.va_start.p0") {
    return prepare::PreparedVariadicEntryHelperKind::VaStart;
  }
  if (callee == "llvm.va_copy.p0.p0") {
    return prepare::PreparedVariadicEntryHelperKind::VaCopy;
  }
  constexpr std::string_view va_arg_prefix = "llvm.va_arg.";
  if (callee.substr(0, va_arg_prefix.size()) == va_arg_prefix) {
    if (callee == "llvm.va_arg.aggregate") {
      return prepare::PreparedVariadicEntryHelperKind::VaArgAggregate;
    }
    return prepare::PreparedVariadicEntryHelperKind::VaArg;
  }
  return std::nullopt;
}

void append_missing_variadic_entry_fact(std::vector<std::string>& missing,
                                        std::string_view fact) {
  missing.push_back(std::string{fact});
}

[[nodiscard]] std::vector<std::string> missing_variadic_entry_facts(
    const prepare::PreparedVariadicEntryPlanFunction& plan) {
  std::vector<std::string> missing = plan.missing_required_facts;
  if (!plan.named_register_counts.gp.has_value()) {
    append_missing_variadic_entry_fact(missing, "named_register_counts.gp");
  }
  if (!plan.named_register_counts.fp.has_value()) {
    append_missing_variadic_entry_fact(missing, "named_register_counts.fp");
  }
  if (plan.register_save_area.required) {
    if (!plan.register_save_area.size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.size_bytes");
    }
    if (!plan.register_save_area.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.align_bytes");
    }
    if (!plan.register_save_area.slot_id.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.slot_id");
    }
    if (!plan.register_save_area.stack_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.stack_offset_bytes");
    }
    if (!plan.register_save_area.gp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.gp_offset_bytes");
    }
    if (!plan.register_save_area.fp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.fp_offset_bytes");
    }
    if (!plan.register_save_area.gp_slot_size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.gp_slot_size_bytes");
    }
    if (!plan.register_save_area.fp_slot_size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "register_save_area.fp_slot_size_bytes");
    }
    if (!plan.register_save_area.saved_gp_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.saved_gp_register_count");
    }
    if (!plan.register_save_area.saved_fp_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.saved_fp_register_count");
    }
    if (!plan.register_save_area.initial_gp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.initial_gp_offset_bytes");
    }
    if (!plan.register_save_area.initial_fp_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "register_save_area.initial_fp_offset_bytes");
    }
  }
  if (plan.overflow_area.required) {
    if (!plan.overflow_area.base_slot_id.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.base_slot_id");
    }
    if (!plan.overflow_area.base_stack_offset_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.base_stack_offset_bytes");
    }
    if (!plan.overflow_area.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "overflow_area.align_bytes");
    }
  }
  if (plan.va_list_layout.required) {
    if (!plan.va_list_layout.size_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.size_bytes");
    }
    if (!plan.va_list_layout.align_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.align_bytes");
    }
    if (plan.va_list_layout.fields.empty()) {
      append_missing_variadic_entry_fact(missing, "va_list_layout.fields");
    }
  }
  if (!plan.helper_resources.required_helpers.empty()) {
    if (!plan.helper_resources.scratch_register_count.has_value()) {
      append_missing_variadic_entry_fact(missing,
                                         "helper_resources.scratch_register_count");
    }
    if (!plan.helper_resources.scratch_stack_bytes.has_value()) {
      append_missing_variadic_entry_fact(missing, "helper_resources.scratch_stack_bytes");
    }
    if (plan.helper_operand_homes.empty()) {
      append_missing_variadic_entry_fact(missing, "helper_operand_homes");
    }
  }
  return missing;
}

[[nodiscard]] bool variadic_helper_operand_homes_complete(
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  const auto scalar_access_plan_complete = [&homes]() {
    if (!homes.scalar_access_plan.has_value()) {
      return false;
    }
    const auto& plan = *homes.scalar_access_plan;
    return plan.source_class !=
               prepare::PreparedVariadicScalarVaArgSourceClass::Unknown &&
           plan.value_type != bir::TypeKind::Void &&
           plan.value_size_bytes != 0 &&
           plan.value_align_bytes != 0 &&
           plan.result_home.has_value() &&
           plan.source_field.has_value() &&
           plan.source_field_offset_bytes.has_value() &&
           plan.source_slot_size_bytes.has_value() &&
           plan.progression_field.has_value() &&
           plan.progression_field_offset_bytes.has_value() &&
           plan.progression_stride_bytes.has_value() &&
           plan.overflow_source_field.has_value() &&
           plan.overflow_source_field_offset_bytes.has_value() &&
           plan.overflow_stride_bytes.has_value();
  };
  switch (homes.helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      return homes.destination_va_list.has_value();
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      return homes.scalar_result.has_value() && homes.source_va_list.has_value() &&
             scalar_access_plan_complete();
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      return prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(
          homes);
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      return homes.destination_va_list.has_value() && homes.source_va_list.has_value();
  }
  return false;
}

[[nodiscard]] std::string variadic_helper_missing_consumption_fact_message(
    prepare::PreparedVariadicEntryHelperKind helper) {
  switch (helper) {
    case prepare::PreparedVariadicEntryHelperKind::VaStart:
      return {};
    case prepare::PreparedVariadicEntryHelperKind::VaArg:
      return "AArch64 scalar va_arg lowering requires prepared fact "
             "helper_operand_homes.va_arg.scalar_access_plan";
    case prepare::PreparedVariadicEntryHelperKind::VaArgAggregate:
      return "AArch64 aggregate va_arg lowering requires prepared fact "
             "helper_operand_homes.va_arg_aggregate.aggregate_access_plan";
    case prepare::PreparedVariadicEntryHelperKind::VaCopy:
      return "AArch64 va_copy lowering requires prepared source and destination "
             "va_list homes";
  }
  return {};
}

[[nodiscard]] std::string variadic_entry_missing_fact_message(
    const std::vector<std::string>& missing) {
  std::string message =
      "AArch64 variadic entry helper lowering requires complete prepared variadic entry facts";
  if (!missing.empty()) {
    message += "; missing fact=";
    message += missing.front();
  }
  return message;
}

[[nodiscard]] const prepare::PreparedVariadicEntryPlanFunction*
require_prepared_variadic_entry_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto function_name = context.function.control_flow != nullptr
                                 ? context.function.control_flow->function_name
                                 : c4c::kInvalidFunctionName;
  const auto* entry_plan =
      context.function.prepared != nullptr
          ? prepare::find_prepared_variadic_entry_plan(*context.function.prepared,
                                                       function_name)
          : nullptr;
  if (entry_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 variadic entry helper lowering requires a PreparedVariadicEntryPlanFunction");
    return nullptr;
  }
  const auto missing = missing_variadic_entry_facts(*entry_plan);
  if (!missing.empty()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        variadic_entry_missing_fact_message(missing));
    return nullptr;
  }
  return entry_plan;
}

[[nodiscard]] const prepare::PreparedIntrinsicCarrier* find_prepared_intrinsic_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr) {
    return nullptr;
  }
  const auto* function_carriers = prepare::find_prepared_intrinsic_carriers(
      *context.function.prepared, context.function.control_flow->function_name);
  if (function_carriers == nullptr) {
    return nullptr;
  }
  for (const auto& carrier : function_carriers->carriers) {
    if (carrier.block_index == context.block_index &&
        carrier.inst_index == instruction_index) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedInlineAsmCarrier* find_prepared_inline_asm_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr) {
    return nullptr;
  }
  const auto* function_carriers = prepare::find_prepared_inline_asm_carriers(
      *context.function.prepared, context.function.control_flow->function_name);
  if (function_carriers == nullptr) {
    return nullptr;
  }
  for (const auto& carrier : function_carriers->carriers) {
    if (carrier.block_index == context.block_index &&
        carrier.inst_index == instruction_index) {
      return &carrier;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::string> inline_asm_operand_name(
    const bir::CallInst& call_inst,
    std::size_t constraint_index) {
  if (!call_inst.inline_asm.has_value()) {
    return std::nullopt;
  }
  for (const auto& operand : call_inst.inline_asm->operands) {
    if (operand.constraint_index == constraint_index && operand.name.has_value()) {
      return operand.name;
    }
  }
  return std::nullopt;
}

[[nodiscard]] MachineEffectResource inline_asm_effect_from_operand(
    const OperandRecord& operand) {
  MachineEffectResource effect;
  effect.operand = operand;
  if (const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      operand.kind == OperandKind::Register && reg != nullptr) {
    effect.kind = MachineEffectResourceKind::Register;
    effect.value_id = reg->value_id;
    effect.value_name = reg->value_name;
    effect.reg = reg->reg;
    return effect;
  }
  if (const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      operand.kind == OperandKind::Immediate && immediate != nullptr) {
    effect.kind = MachineEffectResourceKind::PreparedValue;
    effect.value_id = immediate->source_value_id;
    effect.value_name = immediate->source_value_name;
    return effect;
  }
  return effect;
}

[[nodiscard]] bool inline_asm_clobber_has_malformed_spelling(std::string_view clobber) {
  if (clobber.empty()) {
    return true;
  }
  for (const char ch : clobber) {
    if (ch == '~' || ch == '{' || ch == '}' || ch == ',' || ch == ' ' ||
        ch == '\t' || ch == '\n' || ch == '\r') {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<unsigned> inline_asm_gpr_clobber_index(
    std::string_view clobber) {
  if (clobber.size() < 2 || (clobber.front() != 'x' && clobber.front() != 'w')) {
    return std::nullopt;
  }
  unsigned value = 0;
  for (std::size_t index = 1; index < clobber.size(); ++index) {
    const char ch = clobber[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10 + static_cast<unsigned>(ch - '0');
    if (value > 30) {
      return std::nullopt;
    }
  }
  return value;
}

[[nodiscard]] std::optional<MachineEffectResource> inline_asm_clobber_effect(
    std::string_view clobber,
    std::string* diagnostic_fact) {
  if (inline_asm_clobber_has_malformed_spelling(clobber)) {
    *diagnostic_fact = "malformed_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }
  if (clobber == "memory") {
    return MachineEffectResource{.kind = MachineEffectResourceKind::Memory};
  }
  if (clobber == "cc") {
    return MachineEffectResource{.kind = MachineEffectResourceKind::Flags};
  }
  if (clobber == "sp" || clobber == "wsp" || clobber == "xzr" ||
      clobber == "wzr") {
    *diagnostic_fact = "target_invalid_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }
  const auto index = inline_asm_gpr_clobber_index(clobber);
  if (!index.has_value()) {
    *diagnostic_fact = "unknown_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }

  const std::string canonical_name = "x" + std::to_string(*index);
  const auto converted = abi::convert_prepared_register(
      canonical_name,
      prepare::PreparedRegisterBank::Gpr,
      prepare::PreparedRegisterClass::General,
      abi::RegisterView::X);
  if (!converted.reg.has_value()) {
    *diagnostic_fact = "target_invalid_inline_asm_clobber:";
    diagnostic_fact->append(clobber);
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {canonical_name},
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .reg = *converted.reg,
  };
}

[[nodiscard]] std::optional<std::vector<MachineEffectResource>>
inline_asm_clobber_effects(
    const std::vector<std::string>& clobbers,
    std::string* diagnostic_fact) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(clobbers.size());
  for (const auto& clobber : clobbers) {
    auto effect = inline_asm_clobber_effect(clobber, diagnostic_fact);
    if (!effect.has_value()) {
      return std::nullopt;
    }
    effects.push_back(std::move(*effect));
  }
  return effects;
}

[[nodiscard]] std::optional<RegisterOperand> make_inline_asm_register_operand(
    const prepare::PreparedValueHome& home,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }

  const auto converted =
      abi::convert_prepared_register(*home.register_name,
                                     prepare::PreparedRegisterBank::Gpr,
                                     prepare::PreparedRegisterClass::General,
                                     std::nullopt);
  if (!converted.reg.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        context,
        instruction_index,
        converted.error.has_value()
            ? converted.error->message
            : "AArch64 inline-asm register home could not be converted");
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::ValueHome,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = {abi::register_name(*converted.reg)},
  };
}

[[nodiscard]] std::string inline_asm_carrier_error_message(
    std::string_view fact,
    const prepare::PreparedInlineAsmCarrier* carrier = nullptr);

[[nodiscard]] std::optional<OperandRecord> make_inline_asm_memory_address_operand(
    const prepare::PreparedInlineAsmOperand& operand,
    const bir::MemoryAddress& address,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view unsupported_fact) {
  if (!operand.value.has_value() ||
      address.base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      address.base_value != *operand.value || !operand.home.has_value() ||
      operand.home->kind != prepare::PreparedValueHomeKind::Register ||
      !operand.home->register_name.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message(unsupported_fact));
    return std::nullopt;
  }

  const auto reg = make_inline_asm_register_operand(
      *operand.home, diagnostics, context, instruction_index);
  if (!reg.has_value()) {
    return std::nullopt;
  }

  return make_memory_operand(MemoryOperand{
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .base_kind = MemoryBaseKind::PointerValue,
      .base_register = *reg,
      .pointer_value_name = operand.value_name,
      .pointer_value_id = operand.home->value_id,
      .byte_offset = address.byte_offset,
      .size_bytes = address.size_bytes,
      .align_bytes = address.align_bytes,
      .address_space = address.address_space,
      .is_volatile = address.is_volatile,
      .can_use_base_plus_offset = true,
  });
}

[[nodiscard]] std::string inline_asm_carrier_error_message(
    std::string_view fact,
    const prepare::PreparedInlineAsmCarrier* carrier) {
  std::string message =
      "AArch64 inline-asm lowering requires a complete prepared inline-asm carrier";
  if (!fact.empty()) {
    message += "; missing fact=";
    message += fact;
  } else if (carrier != nullptr && !carrier->missing_required_facts.empty()) {
    message += "; missing fact=";
    message += carrier->missing_required_facts.front();
  }
  return message;
}

[[nodiscard]] const prepare::PreparedInlineAsmOperand*
find_inline_asm_prepared_output_operand(
    const prepare::PreparedInlineAsmCarrier& carrier,
    std::size_t output_index) {
  for (const auto& operand : carrier.operands) {
    if (operand.kind == bir::InlineAsmOperandKind::RegisterOutput &&
        operand.output_index.has_value() && *operand.output_index == output_index) {
      return &operand;
    }
  }
  return nullptr;
}

[[nodiscard]] bool inline_asm_prepared_home_is_concrete_register(
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::Register &&
         home.register_name.has_value();
}

[[nodiscard]] bool inline_asm_identity_matches_register_constraint(
    const prepare::PreparedTargetRegisterIdentity& identity) {
  return identity.bank == prepare::PreparedRegisterBank::Gpr &&
         identity.register_class == prepare::PreparedRegisterClass::General;
}

[[nodiscard]] bool require_inline_asm_tied_home_agreement(
    const prepare::PreparedInlineAsmCarrier& carrier,
    const prepare::PreparedInlineAsmOperand& operand,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  const auto append_tied_diagnostic = [&](std::string_view fact) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message(fact));
  };

  if (!operand.tied_output_index.has_value()) {
    append_tied_diagnostic("missing_tied_output_index");
    return false;
  }
  if (find_inline_asm_prepared_output_operand(carrier, *operand.tied_output_index) ==
      nullptr) {
    append_tied_diagnostic("missing_tied_output_operand");
    return false;
  }
  if (!operand.home.has_value()) {
    append_tied_diagnostic("missing_tied_input_register_home");
    return false;
  }
  if (!carrier.result_home.has_value()) {
    append_tied_diagnostic("missing_tied_output_register_home");
    return false;
  }
  if (!inline_asm_prepared_home_is_concrete_register(*operand.home) ||
      !inline_asm_prepared_home_is_concrete_register(*carrier.result_home)) {
    append_tied_diagnostic("tied_input_output_home_requires_concrete_registers");
    return false;
  }
  const auto tied_identity = operand.home->target_register_identity;
  if (!tied_identity.has_value()) {
    append_tied_diagnostic("target_invalid_tied_input_register_home");
    return false;
  }
  const auto output_identity = carrier.result_home->target_register_identity;
  if (!output_identity.has_value()) {
    append_tied_diagnostic("target_invalid_tied_output_register_home");
    return false;
  }
  if (!inline_asm_identity_matches_register_constraint(*tied_identity) ||
      !inline_asm_identity_matches_register_constraint(*output_identity)) {
    append_tied_diagnostic("tied_input_output_home_incompatible_register_class");
    return false;
  }
  if (*tied_identity != *output_identity) {
    append_tied_diagnostic("tied_input_output_home_mismatch");
    return false;
  }
  if (!operand.tied_home_authority.has_value()) {
    append_tied_diagnostic("missing_tied_home_coallocation_authority");
    return false;
  }
  if (operand.tied_home_authority->tied_output_index != *operand.tied_output_index) {
    append_tied_diagnostic("tied_home_coallocation_authority_output_mismatch");
    return false;
  }
  if (operand.tied_home_authority->shared_register != *tied_identity) {
    append_tied_diagnostic("tied_home_coallocation_authority_home_mismatch");
    return false;
  }
  return true;
}

[[nodiscard]] std::string address_materialization_error_message(
    PreparedAddressMaterializationRecordError error) {
  std::string message =
      "AArch64 address materialization lowering requires prepared address facts";
  message += "; error=";
  message += prepared_address_materialization_record_error_name(error);
  return message;
}

[[nodiscard]] std::string intrinsic_error_message(
    PreparedScalarFpUnaryIntrinsicRecordError error) {
  std::string message =
      "AArch64 intrinsic lowering requires a complete prepared scalar FP unary carrier";
  message += "; error=";
  message += prepared_scalar_fp_unary_intrinsic_record_error_name(error);
  return message;
}

[[nodiscard]] std::string prepared_intrinsic_carrier_error_message(
    std::string_view error) {
  std::string message =
      "AArch64 intrinsic lowering requires a complete prepared intrinsic carrier";
  message += "; error=";
  message += error;
  return message;
}

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

[[nodiscard]] std::optional<abi::RegisterView> intrinsic_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::I128:
      return abi::RegisterView::Q;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I64:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<RegisterOperand> make_intrinsic_register_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register ||
      storage.value_name != home.value_name) {
    return std::nullopt;
  }
  const auto expected_view = intrinsic_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(storage.bank);
  abi::PreparedRegisterConversionResult converted;
  if (storage.register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *storage.register_placement, prepared_class, expected_view);
  } else {
    if (!home.register_name.has_value() || !storage.register_name.has_value() ||
        *home.register_name != *storage.register_name) {
      return std::nullopt;
    }
    converted = abi::convert_prepared_register(
        *storage.register_name, storage.bank, prepared_class, expected_view);
  }
  if (!converted.has_value()) {
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = {*converted.reg},
  };
}

[[nodiscard]] std::optional<RegisterOperand> make_intrinsic_register_operand(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::optional<prepare::PreparedValueHome>& home,
    bir::TypeKind type) {
  if (!home.has_value()) {
    return std::nullopt;
  }
  const auto* storage = find_storage_plan_value(storage_plan, home->value_id);
  if (storage == nullptr) {
    return std::nullopt;
  }
  return make_intrinsic_register_operand(*home, *storage, type);
}

[[nodiscard]] std::optional<RegisterOperand> make_intrinsic_register_operand(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const std::vector<std::optional<prepare::PreparedValueHome>>& homes,
    std::size_t index,
    bir::TypeKind type) {
  if (index >= homes.size()) {
    return std::nullopt;
  }
  return make_intrinsic_register_operand(storage_plan, homes[index], type);
}

[[nodiscard]] InstructionRecord make_crc32w_intrinsic_record_from_carrier(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  const auto accumulator =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 0, bir::TypeKind::I32);
  const auto data =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 1, bir::TypeKind::I32);
  const auto result =
      make_intrinsic_register_operand(storage_plan, carrier.result_home, bir::TypeKind::I32);

  return make_crc32w_intrinsic_instruction(Crc32WIntrinsicRecord{
      .source_carrier = &carrier,
      .family = carrier.family,
      .operation = carrier.operation,
      .required_feature = carrier.required_feature,
      .operand_type = carrier.operand_type,
      .result_type = carrier.result_type,
      .operand_roles = carrier.operand_roles,
      .signedness = carrier.signedness,
      .accumulator_value_id = accumulator ? accumulator->value_id : std::nullopt,
      .accumulator_value_name =
          accumulator ? accumulator->value_name : c4c::kInvalidValueName,
      .data_value_id = data ? data->value_id : std::nullopt,
      .data_value_name = data ? data->value_name : c4c::kInvalidValueName,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name = result ? result->value_name : c4c::kInvalidValueName,
      .accumulator = accumulator ? make_register_operand(*accumulator) : OperandRecord{},
      .data = data ? make_register_operand(*data) : OperandRecord{},
      .result_register = result,
      .requires_feature = carrier.requires_feature,
      .source_callee_name = carrier.source_callee_name,
      .has_prepared_call_plan = carrier.has_prepared_call_plan,
  });
}

[[nodiscard]] InstructionRecord make_vector_load_intrinsic_record_from_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  const auto pointer =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 0, bir::TypeKind::Ptr);
  const auto result =
      make_intrinsic_register_operand(storage_plan, carrier.result_home, bir::TypeKind::I128);
  const auto pointer_value_name =
      pointer ? pointer->value_name : c4c::kInvalidValueName;
  const auto pointer_value_id = pointer ? pointer->value_id : std::nullopt;
  MemoryOperand memory{
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name =
          result ? std::optional<c4c::ValueNameId>{result->value_name} : std::nullopt,
      .base_kind = MemoryBaseKind::PointerValue,
      .base_register = pointer,
      .pointer_value_name = pointer_value_name,
      .pointer_value_id = pointer_value_id,
      .byte_offset = carrier.memory_operand ? carrier.memory_operand->byte_offset : 0,
      .size_bytes = carrier.vector_total_width_bytes,
      .align_bytes =
          carrier.memory_operand ? carrier.memory_operand->align_bytes
                                 : carrier.vector_total_width_bytes,
      .address_space = carrier.memory_operand ? carrier.memory_operand->address_space
                                              : bir::AddressSpace::Default,
  };

  return make_vector_load_intrinsic_instruction(VectorLoadIntrinsicRecord{
      .source_carrier = &carrier,
      .family = carrier.family,
      .operation = carrier.operation,
      .required_feature = carrier.required_feature,
      .operand_type = carrier.operand_type,
      .result_type = carrier.result_type,
      .operand_roles = carrier.operand_roles,
      .vector_element_type = carrier.vector_element_type,
      .vector_element_width_bytes = carrier.vector_element_width_bytes,
      .vector_lane_count = carrier.vector_lane_count,
      .vector_total_width_bytes = carrier.vector_total_width_bytes,
      .signedness = carrier.signedness,
      .memory_access = carrier.memory_access,
      .pointer_value_id = pointer_value_id,
      .pointer_value_name = pointer_value_name,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name = result ? result->value_name : c4c::kInvalidValueName,
      .pointer = pointer ? make_register_operand(*pointer) : OperandRecord{},
      .memory = memory,
      .result_register = result,
      .requires_feature = carrier.requires_feature,
      .source_callee_name = carrier.source_callee_name,
      .has_prepared_call_plan = carrier.has_prepared_call_plan,
  });
}

[[nodiscard]] InstructionRecord make_vector_add_intrinsic_record_from_carrier(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier) {
  const auto lhs =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 0, bir::TypeKind::I128);
  const auto rhs =
      make_intrinsic_register_operand(storage_plan, carrier.operand_homes, 1, bir::TypeKind::I128);
  const auto result =
      make_intrinsic_register_operand(storage_plan, carrier.result_home, bir::TypeKind::I128);

  return make_vector_add_intrinsic_instruction(VectorAddIntrinsicRecord{
      .source_carrier = &carrier,
      .family = carrier.family,
      .operation = carrier.operation,
      .required_feature = carrier.required_feature,
      .operand_type = carrier.operand_type,
      .result_type = carrier.result_type,
      .operand_roles = carrier.operand_roles,
      .vector_element_type = carrier.vector_element_type,
      .vector_element_width_bytes = carrier.vector_element_width_bytes,
      .vector_lane_count = carrier.vector_lane_count,
      .vector_total_width_bytes = carrier.vector_total_width_bytes,
      .signedness = carrier.signedness,
      .memory_access = carrier.memory_access,
      .lhs_value_id = lhs ? lhs->value_id : std::nullopt,
      .lhs_value_name = lhs ? lhs->value_name : c4c::kInvalidValueName,
      .rhs_value_id = rhs ? rhs->value_id : std::nullopt,
      .rhs_value_name = rhs ? rhs->value_name : c4c::kInvalidValueName,
      .result_value_id = result ? result->value_id : std::nullopt,
      .result_value_name = result ? result->value_name : c4c::kInvalidValueName,
      .lhs = lhs ? make_register_operand(*lhs) : OperandRecord{},
      .rhs = rhs ? make_register_operand(*rhs) : OperandRecord{},
      .result_register = result,
      .requires_feature = carrier.requires_feature,
      .source_callee_name = carrier.source_callee_name,
      .has_prepared_call_plan = carrier.has_prepared_call_plan,
  });
}

[[nodiscard]] std::string i128_transport_error_message(
    PreparedI128TransportRecordError error) {
  std::string message =
      "AArch64 i128 transport lowering requires prepared i128 carrier facts";
  message += "; error=";
  message += prepared_i128_transport_record_error_name(error);
  return message;
}

[[nodiscard]] std::string f128_transport_error_message(
    PreparedF128TransportRecordError error) {
  std::string message =
      "AArch64 binary128 memory transport lowering requires prepared f128 carrier facts";
  message += "; error=";
  message += prepared_f128_transport_record_error_name(error);
  return message;
}

[[nodiscard]] std::string f128_runtime_helper_error_message(
    PreparedF128RuntimeHelperRecordError error) {
  std::string message =
      "AArch64 binary128 runtime helper-boundary lowering requires prepared f128 helper facts";
  message += "; error=";
  message += prepared_f128_runtime_helper_record_error_name(error);
  return message;
}

[[nodiscard]] std::string i128_pair_error_message(
    PreparedI128PairRecordError error) {
  std::string message =
      "AArch64 i128 pair operation lowering requires prepared i128 carrier facts";
  message += "; error=";
  message += prepared_i128_pair_record_error_name(error);
  return message;
}

[[nodiscard]] std::string i128_runtime_helper_error_message(
    PreparedI128RuntimeHelperRecordError error) {
  std::string message =
      "AArch64 i128 runtime helper-boundary lowering requires prepared i128 helper facts";
  message += "; error=";
  message += prepared_i128_runtime_helper_record_error_name(error);
  return message;
}

[[nodiscard]] bool is_i128_div_rem_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::SDiv || opcode == bir::BinaryOpcode::UDiv ||
         opcode == bir::BinaryOpcode::SRem || opcode == bir::BinaryOpcode::URem;
}

[[nodiscard]] const prepare::PreparedI128RuntimeHelper* find_i128_runtime_helper_for_instruction(
    const prepare::PreparedI128RuntimeHelperFunction& helpers,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& helper : helpers.helpers) {
    if (helper.block_index == block_index &&
        helper.instruction_index == instruction_index) {
      return &helper;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedF128RuntimeHelper* find_f128_runtime_helper_for_instruction(
    const prepare::PreparedF128RuntimeHelperFunction& helpers,
    std::size_t block_index,
    std::size_t instruction_index) {
  for (const auto& helper : helpers.helpers) {
    if (helper.block_index == block_index &&
        helper.instruction_index == instruction_index) {
      return &helper;
    }
  }
  return nullptr;
}

struct LowerMemoryInstructionResult {
  bool handled = false;
  std::optional<module::MachineInstruction> instruction;
};

[[nodiscard]] module::MachineInstruction make_bir_machine_instruction(
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

[[nodiscard]] std::optional<module::MachineInstruction> lower_address_materialization(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    return std::nullopt;
  }

  const auto prepared =
      make_prepared_address_materialization_instruction_record(
          context.function.prepared->names,
          *context.function.value_locations,
          *context.function.storage_plan,
          *addressing,
          context.control_flow_block->block_label,
          instruction_index);
  if (!prepared.record.has_value()) {
    if (prepared.error != PreparedAddressMaterializationRecordError::
                              MissingPreparedAddressMaterialization) {
      append_address_materialization_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          address_materialization_error_message(prepared.error));
    }
    return std::nullopt;
  }

  InstructionRecord target =
      make_address_materialization_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_address_materialization_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] LowerMemoryInstructionResult lower_f128_transport_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if ((load == nullptr || load->result.type != bir::TypeKind::F128) &&
      (store == nullptr || store->value.type != bir::TypeKind::F128)) {
    return LowerMemoryInstructionResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_transport_error_message(
            PreparedF128TransportRecordError::MissingPreparedF128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* f128_carriers =
      prepare::find_prepared_f128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (f128_carriers == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_transport_error_message(
            PreparedF128TransportRecordError::MissingPreparedF128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return LowerMemoryInstructionResult{.handled = true};
  }

  PreparedMemoryOperandRecordResult memory;
  c4c::ValueNameId carrier_value_name = c4c::kInvalidValueName;
  F128TransportKind transport_kind = F128TransportKind::CarrierSnapshot;
  if (load != nullptr) {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *load);
    carrier_value_name =
        context.function.prepared->names.value_names.find(load->result.name);
    transport_kind = F128TransportKind::LoadFromMemory;
  } else {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *store);
    carrier_value_name =
        store->value.kind == bir::Value::Kind::Named
            ? context.function.prepared->names.value_names.find(store->value.name)
            : c4c::kInvalidValueName;
    transport_kind = F128TransportKind::StoreToMemory;
  }
  if (!memory.record.has_value()) {
    append_memory_diagnostic(diagnostics,
                             module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                             context,
                             instruction_index,
                             memory_error_message(memory.error));
    return LowerMemoryInstructionResult{.handled = true};
  }
  if (carrier_value_name == c4c::kInvalidValueName) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_transport_error_message(
            PreparedF128TransportRecordError::MissingPreparedF128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }

  auto prepared = make_prepared_f128_carrier_transport_record(
      *f128_carriers,
      carrier_value_name,
      transport_kind,
      std::move(memory.record));
  if (!prepared.record.has_value()) {
    append_memory_diagnostic(diagnostics,
                             module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                             context,
                             instruction_index,
                             f128_transport_error_message(prepared.error));
    return LowerMemoryInstructionResult{.handled = true};
  }

  InstructionRecord target = make_f128_transport_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_memory_diagnostic(diagnostics,
                             module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                             context,
                             instruction_index,
                             std::string{target.selection.diagnostic});
    return LowerMemoryInstructionResult{.handled = true};
  }

  return LowerMemoryInstructionResult{
      .handled = true,
      .instruction = make_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

[[nodiscard]] LowerMemoryInstructionResult lower_f128_runtime_helper_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const bool f128_binary_helper_candidate =
      binary != nullptr &&
      binary->operand_type == bir::TypeKind::F128 &&
      (binary->result.type == bir::TypeKind::F128 ||
       binary->result.type == bir::TypeKind::I1);
  const bool supported_f128_cast =
      cast != nullptr &&
      ((cast->opcode == bir::CastOpcode::FPExt &&
        (cast->operand.type == bir::TypeKind::F32 ||
         cast->operand.type == bir::TypeKind::F64) &&
        cast->result.type == bir::TypeKind::F128) ||
       (cast->opcode == bir::CastOpcode::FPTrunc &&
        cast->operand.type == bir::TypeKind::F128 &&
        (cast->result.type == bir::TypeKind::F32 ||
         cast->result.type == bir::TypeKind::F64)));
  if (!f128_binary_helper_candidate && !supported_f128_cast) {
    return LowerMemoryInstructionResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_runtime_helper_error_message(
            PreparedF128RuntimeHelperRecordError::MissingPreparedF128RuntimeHelper));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* f128_carriers =
      prepare::find_prepared_f128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (f128_carriers == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_runtime_helper_error_message(
            PreparedF128RuntimeHelperRecordError::MissingPreparedF128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* helper_function =
      prepare::find_prepared_f128_runtime_helpers(*context.function.prepared,
                                                  context.function.control_flow->function_name);
  if (helper_function == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_runtime_helper_error_message(
            PreparedF128RuntimeHelperRecordError::MissingPreparedF128RuntimeHelper));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* helper =
      find_f128_runtime_helper_for_instruction(
          *helper_function, context.block_index, instruction_index);
  if (helper == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_runtime_helper_error_message(
            PreparedF128RuntimeHelperRecordError::MissingPreparedF128RuntimeHelper));
    return LowerMemoryInstructionResult{.handled = true};
  }

  auto prepared =
      make_prepared_f128_runtime_helper_boundary_record(*f128_carriers, *helper);
  if (!prepared.record.has_value()) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        f128_runtime_helper_error_message(prepared.error));
    return LowerMemoryInstructionResult{.handled = true};
  }

  InstructionRecord target =
      make_f128_runtime_helper_boundary_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (auto* record = std::get_if<F128RuntimeHelperBoundaryRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->block_index = context.block_index;
    record->instruction_index = instruction_index;
  }
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_pair_diagnostic(diagnostics,
                                module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                context,
                                instruction_index,
                                std::string{target.selection.diagnostic});
    return LowerMemoryInstructionResult{.handled = true};
  }

  return LowerMemoryInstructionResult{
      .handled = true,
      .instruction = make_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

[[nodiscard]] LowerMemoryInstructionResult lower_i128_transport_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if ((load == nullptr || load->result.type != bir::TypeKind::I128) &&
      (store == nullptr || store->value.type != bir::TypeKind::I128)) {
    return LowerMemoryInstructionResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_transport_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(
            PreparedI128TransportRecordError::MissingPreparedI128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_i128_transport_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(
            PreparedI128TransportRecordError::MissingPreparedI128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    append_i128_transport_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return LowerMemoryInstructionResult{.handled = true};
  }

  PreparedMemoryOperandRecordResult memory;
  c4c::ValueNameId carrier_value_name = c4c::kInvalidValueName;
  I128TransportKind transport_kind = I128TransportKind::CarrierSnapshot;
  if (load != nullptr) {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *load);
    carrier_value_name =
        context.function.prepared->names.value_names.find(load->result.name);
    transport_kind = I128TransportKind::LoadFromMemory;
  } else {
    memory = make_prepared_memory_operand_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *store);
    carrier_value_name =
        store->value.kind == bir::Value::Kind::Named
            ? context.function.prepared->names.value_names.find(store->value.name)
            : c4c::kInvalidValueName;
    transport_kind = I128TransportKind::StoreToMemory;
  }
  if (!memory.record.has_value()) {
    append_i128_transport_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(memory.error));
    return LowerMemoryInstructionResult{.handled = true};
  }
  if (carrier_value_name == c4c::kInvalidValueName) {
    append_i128_transport_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(
            PreparedI128TransportRecordError::MissingPreparedI128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }

  auto prepared = make_prepared_i128_carrier_transport_record(
      *i128_carriers,
      carrier_value_name,
      transport_kind,
      std::move(memory.record));
  if (!prepared.record.has_value()) {
    append_i128_transport_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_transport_error_message(prepared.error));
    return LowerMemoryInstructionResult{.handled = true};
  }

  InstructionRecord target = make_i128_transport_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_transport_diagnostic(diagnostics,
                                     module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                     context,
                                     instruction_index,
                                     std::string{target.selection.diagnostic});
    return LowerMemoryInstructionResult{.handled = true};
  }

  return LowerMemoryInstructionResult{
      .handled = true,
      .instruction = make_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

[[nodiscard]] LowerMemoryInstructionResult lower_i128_pair_operation_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      (binary->operand_type != bir::TypeKind::I128 &&
       binary->result.type != bir::TypeKind::I128)) {
    return LowerMemoryInstructionResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_pair_error_message(PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }
  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_i128_pair_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_pair_error_message(PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return LowerMemoryInstructionResult{.handled = true};
  }

  InstructionRecord target;
  if (binary->opcode == bir::BinaryOpcode::Shl ||
      binary->opcode == bir::BinaryOpcode::LShr ||
      binary->opcode == bir::BinaryOpcode::AShr) {
    if (context.function.value_locations == nullptr ||
        context.function.storage_plan == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(PreparedI128PairRecordError::MissingShiftCountStorage));
      return LowerMemoryInstructionResult{.handled = true};
    }
    auto prepared = make_prepared_i128_shift_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *i128_carriers,
        *binary);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(prepared.error));
      return LowerMemoryInstructionResult{.handled = true};
    }
    target = make_i128_shift_instruction(*prepared.record);
  } else if (is_compare_predicate(binary->opcode)) {
    if (context.function.value_locations == nullptr ||
        context.function.storage_plan == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(PreparedI128PairRecordError::MissingScalarResultStorage));
      return LowerMemoryInstructionResult{.handled = true};
    }
    auto prepared = make_prepared_i128_compare_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *i128_carriers,
        *binary);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(prepared.error));
      return LowerMemoryInstructionResult{.handled = true};
    }
    target = make_i128_compare_instruction(*prepared.record);
  } else if (is_i128_div_rem_opcode(binary->opcode)) {
    const auto* helper_function =
        prepare::find_prepared_i128_runtime_helpers(*context.function.prepared,
                                                    context.function.control_flow->function_name);
    if (helper_function == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_runtime_helper_error_message(
              PreparedI128RuntimeHelperRecordError::MissingPreparedI128RuntimeHelper));
      return LowerMemoryInstructionResult{.handled = true};
    }
    const auto* helper =
        find_i128_runtime_helper_for_instruction(
            *helper_function, context.block_index, instruction_index);
    if (helper == nullptr) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_runtime_helper_error_message(
              PreparedI128RuntimeHelperRecordError::MissingPreparedI128RuntimeHelper));
      return LowerMemoryInstructionResult{.handled = true};
    }
    auto prepared =
        make_prepared_i128_runtime_helper_boundary_record(*i128_carriers, *helper);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_runtime_helper_error_message(prepared.error));
      return LowerMemoryInstructionResult{.handled = true};
    }
    target = make_i128_runtime_helper_boundary_instruction(*prepared.record);
  } else {
    auto prepared = make_prepared_i128_pair_operation_record(
        context.function.prepared->names,
        *i128_carriers,
        *binary);
    if (!prepared.record.has_value()) {
      append_i128_pair_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
          context,
          instruction_index,
          i128_pair_error_message(prepared.error));
      return LowerMemoryInstructionResult{.handled = true};
    }
    target = make_i128_pair_operation_instruction(*prepared.record);
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (auto* record = std::get_if<I128PairOperationRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  } else if (auto* record = std::get_if<I128ShiftRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  } else if (auto* record = std::get_if<I128CompareRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  } else if (auto* record = std::get_if<I128RuntimeHelperBoundaryRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->block_index = context.block_index;
    record->instruction_index = instruction_index;
  }
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_pair_diagnostic(diagnostics,
                                module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                context,
                                instruction_index,
                                std::string{target.selection.diagnostic});
    return LowerMemoryInstructionResult{.handled = true};
  }

  return LowerMemoryInstructionResult{
      .handled = true,
      .instruction = make_bir_machine_instruction(context, instruction_index, std::move(target)),
  };
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_inline_asm_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (!call_inst.inline_asm.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message("missing_retained_inline_asm_metadata"));
    return std::nullopt;
  }
  if (context.function.target_profile == nullptr ||
      !abi::is_aarch64_target(*context.function.target_profile)) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message("target_profile_is_not_aarch64"));
    return std::nullopt;
  }

  const auto* carrier = find_prepared_inline_asm_carrier(context, instruction_index);
  if (carrier == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message("missing_prepared_inline_asm_carrier"));
    return std::nullopt;
  }
  if (carrier->carrier_kind != prepare::PreparedInlineAsmCarrierKind::Complete ||
      !carrier->missing_required_facts.empty()) {
    append_call_diagnostic(diagnostics,
                           module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                           context,
                           instruction_index,
                           inline_asm_carrier_error_message({}, carrier));
    return std::nullopt;
  }
  std::string clobber_diagnostic_fact;
  auto clobber_effects =
      inline_asm_clobber_effects(carrier->clobbers, &clobber_diagnostic_fact);
  if (!clobber_effects.has_value()) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        inline_asm_carrier_error_message(clobber_diagnostic_fact));
    return std::nullopt;
  }
  AssemblerInstructionRecord record{
      .has_inline_asm_payload = true,
      .side_effects = carrier->side_effects,
      .inline_asm_template = carrier->asm_text,
      .inline_asm_constraints = carrier->constraints,
      .inline_asm_has_named_operand_references =
          carrier->has_named_operand_references,
      .inline_asm_has_template_modifiers = carrier->has_template_modifiers,
      .inline_asm_clobbers = carrier->clobbers,
      .inline_asm_result = carrier->result,
      .inline_asm_result_value_name = carrier->result_value_name,
      .inline_asm_result_home = carrier->result_home,
  };

  record.inline_asm_operands.reserve(carrier->operands.size());
  record.operands.reserve(carrier->operands.size());
  for (const auto& operand : carrier->operands) {
    InlineAsmMachineOperandRecord selected{
        .kind = operand.kind,
        .constraint_index = operand.constraint_index,
        .constraint = operand.constraint,
        .arg_index = operand.arg_index,
        .output_index = operand.output_index,
        .tied_output_index = operand.tied_output_index,
        .name = inline_asm_operand_name(call_inst, operand.constraint_index),
        .value = operand.value,
        .value_name = operand.value_name,
        .home = operand.home,
        .immediate_value = operand.immediate_value,
    };

    switch (operand.kind) {
      case bir::InlineAsmOperandKind::RegisterOutput: {
        if (!carrier->result_home.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("missing_result_register_home"));
          return std::nullopt;
        }
        if (carrier->result_home->kind != prepare::PreparedValueHomeKind::Register ||
            !carrier->result_home->register_name.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("result_home_not_a_gpr"));
          return std::nullopt;
        }
        selected.value = carrier->result;
        selected.value_name = carrier->result_value_name;
        selected.home = carrier->result_home;
        const auto reg = make_inline_asm_register_operand(
            *carrier->result_home, diagnostics, context, instruction_index);
        if (!reg.has_value()) {
          return std::nullopt;
        }
        selected.selected_operand = make_register_operand(*reg);
        break;
      }
      case bir::InlineAsmOperandKind::RegisterInput: {
        if (!operand.home.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("missing_register_operand_home"));
          return std::nullopt;
        }
        if (operand.home->kind != prepare::PreparedValueHomeKind::Register ||
            !operand.home->register_name.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("register_operand_home_not_a_gpr"));
          return std::nullopt;
        }
        const auto reg = make_inline_asm_register_operand(
            *operand.home, diagnostics, context, instruction_index);
        if (!reg.has_value()) {
          return std::nullopt;
        }
        selected.selected_operand = make_register_operand(*reg);
        break;
      }
      case bir::InlineAsmOperandKind::TiedInput: {
        if (!require_inline_asm_tied_home_agreement(
                *carrier, operand, diagnostics, context, instruction_index)) {
          return std::nullopt;
        }
        const auto reg = make_inline_asm_register_operand(
            *operand.home, diagnostics, context, instruction_index);
        if (!reg.has_value()) {
          return std::nullopt;
        }
        selected.selected_operand = make_register_operand(*reg);
        break;
      }
      case bir::InlineAsmOperandKind::IntegerImmediateInput: {
        if (!operand.immediate_value.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message("missing_integer_immediate_value"));
          return std::nullopt;
        }
        selected.selected_operand = make_immediate_operand(ImmediateOperand{
            .kind = ImmediateKind::SignedInteger,
            .type = operand.value.has_value() ? operand.value->type : bir::TypeKind::Void,
            .signed_value = *operand.immediate_value,
            .unsigned_value = static_cast<std::uint64_t>(*operand.immediate_value),
            .source_value_id = operand.home.has_value()
                                   ? std::optional<prepare::PreparedValueId>{
                                         operand.home->value_id}
                                   : std::nullopt,
            .source_value_name = operand.value_name.value_or(c4c::kInvalidValueName),
        });
        break;
      }
      case bir::InlineAsmOperandKind::MemoryInput:
        if (!operand.memory_address.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message(
                  "unsupported_inline_asm_memory_address_selection"));
          return std::nullopt;
        }
        selected.selected_operand = make_inline_asm_memory_address_operand(
            operand,
            *operand.memory_address,
            diagnostics,
            context,
            instruction_index,
            "unsupported_inline_asm_memory_address_selection");
        if (!selected.selected_operand.has_value()) {
          return std::nullopt;
        }
        break;
      case bir::InlineAsmOperandKind::AddressInput:
        if (!operand.address.has_value()) {
          append_call_diagnostic(
              diagnostics,
              module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
              context,
              instruction_index,
              inline_asm_carrier_error_message(
                  "unsupported_inline_asm_address_selection"));
          return std::nullopt;
        }
        selected.selected_operand = make_inline_asm_memory_address_operand(
            operand,
            *operand.address,
            diagnostics,
            context,
            instruction_index,
            "unsupported_inline_asm_address_selection");
        if (!selected.selected_operand.has_value()) {
          return std::nullopt;
        }
        break;
      case bir::InlineAsmOperandKind::Clobber:
        append_call_diagnostic(
            diagnostics,
            module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
            context,
            instruction_index,
            inline_asm_carrier_error_message("unsupported_inline_asm_clobber_operand_kind"));
        return std::nullopt;
      case bir::InlineAsmOperandKind::Unsupported:
        append_call_diagnostic(
            diagnostics,
            module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
            context,
            instruction_index,
            inline_asm_carrier_error_message("unsupported_inline_asm_operand_kind"));
        return std::nullopt;
    }

    if (selected.selected_operand.has_value()) {
      record.operands.push_back(*selected.selected_operand);
    }
    record.inline_asm_operands.push_back(std::move(selected));
  }

  InstructionRecord target = make_assembler_instruction(record);
  target.surface = RecordSurfaceKind::MachineInstructionNode;
  target.selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  target.function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName;
  target.block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  target.operands = record.operands;
  target.defs.clear();
  target.uses.clear();
  target.clobbers = std::move(*clobber_effects);
  for (const auto& inline_operand : record.inline_asm_operands) {
    if (!inline_operand.selected_operand.has_value()) {
      continue;
    }
    const auto effect = inline_asm_effect_from_operand(*inline_operand.selected_operand);
    if (inline_operand.kind == bir::InlineAsmOperandKind::RegisterOutput) {
      target.defs.push_back(effect);
    } else {
      target.uses.push_back(effect);
    }
  }

  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_intrinsic_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* carrier = find_prepared_intrinsic_carrier(context, instruction_index);
  if (carrier == nullptr) {
    return std::nullopt;
  }
  if (context.function.prepared == nullptr || context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr || context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        intrinsic_error_message(
            PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedIntrinsicCarrier));
    return std::nullopt;
  }

  InstructionRecord target;
  if (carrier->family == bir::IntrinsicFamilyKind::ScalarFpUnary) {
    const auto prepared = make_prepared_scalar_fp_unary_intrinsic_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *carrier);
    if (!prepared.record.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          intrinsic_error_message(prepared.error));
      return std::nullopt;
    }

    target = make_scalar_fp_unary_intrinsic_instruction(*prepared.record);
  } else if (carrier->carrier_kind != prepare::PreparedIntrinsicCarrierKind::Complete) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        prepared_intrinsic_carrier_error_message("incomplete_prepared_intrinsic_carrier"));
    return std::nullopt;
  } else if (carrier->family == bir::IntrinsicFamilyKind::Crc &&
             carrier->operation == bir::IntrinsicOperationKind::Crc32W) {
    target = make_crc32w_intrinsic_record_from_carrier(*context.function.storage_plan,
                                                       *carrier);
  } else if (carrier->family == bir::IntrinsicFamilyKind::VectorMemory &&
             carrier->operation == bir::IntrinsicOperationKind::VectorLoad) {
    target = make_vector_load_intrinsic_record_from_carrier(
        context, instruction_index, *context.function.storage_plan, *carrier);
  } else if (carrier->family == bir::IntrinsicFamilyKind::VectorOperation &&
             carrier->operation == bir::IntrinsicOperationKind::VectorAdd) {
    target = make_vector_add_intrinsic_record_from_carrier(*context.function.storage_plan,
                                                           *carrier);
  } else {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        prepared_intrinsic_carrier_error_message("unsupported_intrinsic_family"));
    return std::nullopt;
  }

  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_call_diagnostic(diagnostics,
                           module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                           context,
                           instruction_index,
                           std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

}  // namespace

module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return module::BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            module::ModuleLoweringDiagnosticKind::MissingBlockContext,
                            context,
                            "AArch64 block dispatch requires prepared function and block context");
    return result;
  }

  block.block_label = context.control_flow_block->block_label;
  block.index = context.block_index;
  block.successors.clear();

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (call->inline_asm.has_value() ||
            find_prepared_inline_asm_carrier(context, instruction_index) != nullptr) {
          if (auto lowered = lower_inline_asm_instruction(
                  context, *call, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        if (find_prepared_intrinsic_carrier(context, instruction_index) != nullptr) {
          if (auto lowered = lower_intrinsic_instruction(
                  context, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        const auto* call_plan = find_prepared_call_plan(context, instruction_index);
        if (call_plan != nullptr) {
          auto before_call_moves =
              lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          for (auto& before_call_move : before_call_moves) {
            block.instructions.push_back(std::move(before_call_move));
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          if (call_plan != nullptr) {
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              block.instructions.push_back(std::move(after_call_move));
            }
          }
        }
      } else if (auto lowered = lower_address_materialization(
                     context, instruction_index, diagnostics)) {
        if (const auto* address_record =
                std::get_if<AddressMaterializationRecord>(&lowered->target.payload);
            address_record != nullptr && address_record->result_register.has_value()) {
          record_emitted_scalar_register(scalar_state,
                                         address_record->result_value_name,
                                         *address_record->result_register);
        }
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_i128_pair =
                     lower_i128_pair_operation_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_i128_pair.handled) {
        if (lowered_i128_pair.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_pair.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_f128_helper =
                     lower_f128_runtime_helper_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_helper.handled) {
        if (lowered_f128_helper.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_f128_helper.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        auto lowered_i128_transport =
            lower_i128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_i128_transport.handled) {
          if (lowered_i128_transport.instruction.has_value()) {
            block.instructions.push_back(std::move(*lowered_i128_transport.instruction));
          }
          ++result.visited_operations;
          continue;
        }
        auto lowered_memory =
            lower_f128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_memory.handled) {
          if (lowered_memory.instruction.has_value()) {
            if (const auto* memory_record =
                    std::get_if<MemoryInstructionRecord>(
                        &lowered_memory.instruction->target.payload);
                memory_record != nullptr && memory_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             memory_record->result_value_name,
                                             *memory_record->result_register);
            }
            block.instructions.push_back(std::move(*lowered_memory.instruction));
          }
        } else if (auto lowered_ordinary_memory =
                       lower_memory_instruction(context, inst, instruction_index, diagnostics);
                   lowered_ordinary_memory.handled) {
          if (lowered_ordinary_memory.instruction.has_value()) {
            if (const auto* memory_record =
                    std::get_if<MemoryInstructionRecord>(
                        &lowered_ordinary_memory.instruction->target.payload);
                memory_record != nullptr && memory_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             memory_record->result_value_name,
                                             *memory_record->result_register);
            }
            block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
          }
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      }
      ++result.visited_operations;
    }
  }

  auto lowered_atomic_operations =
      lower_atomic_memory_operations_for_block(context, diagnostics);
  result.visited_operations += lowered_atomic_operations.size();
  for (auto& atomic_instruction : lowered_atomic_operations) {
    block.instructions.push_back(std::move(atomic_instruction));
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    if (auto lowered =
            lower_prepared_return_terminator(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    if (auto lowered = lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (auto lowered =
            lower_prepared_conditional_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::codegen
