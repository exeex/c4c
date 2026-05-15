#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "comparison.hpp"
#include "returns.hpp"

#include <cstddef>
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

[[nodiscard]] const prepare::PreparedCallPlan* find_prepared_call_plan(
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

[[nodiscard]] std::string address_materialization_error_message(
    PreparedAddressMaterializationRecordError error) {
  std::string message =
      "AArch64 address materialization lowering requires prepared address facts";
  message += "; error=";
  message += prepared_address_materialization_record_error_name(error);
  return message;
}

[[nodiscard]] std::string memory_error_message(
    PreparedMemoryOperandRecordError error) {
  std::string message =
      "AArch64 memory lowering requires prepared memory and destination facts";
  message += "; error=";
  message += prepared_memory_operand_record_error_name(error);
  return message;
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
  const bool supported_f128_comparison =
      binary != nullptr &&
      (binary->opcode == bir::BinaryOpcode::Eq ||
       binary->opcode == bir::BinaryOpcode::Ne ||
       binary->opcode == bir::BinaryOpcode::Slt ||
       binary->opcode == bir::BinaryOpcode::Sle ||
       binary->opcode == bir::BinaryOpcode::Sgt ||
       binary->opcode == bir::BinaryOpcode::Sge) &&
      binary->operand_type == bir::TypeKind::F128 &&
      binary->result.type == bir::TypeKind::I1;
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
  if ((binary == nullptr ||
       (!supported_f128_comparison &&
        ((binary->opcode != bir::BinaryOpcode::Add &&
          binary->opcode != bir::BinaryOpcode::Sub &&
          binary->opcode != bir::BinaryOpcode::Mul &&
          binary->opcode != bir::BinaryOpcode::SDiv) ||
         binary->operand_type != bir::TypeKind::F128 ||
         binary->result.type != bir::TypeKind::F128))) &&
      !supported_f128_cast) {
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

[[nodiscard]] LowerMemoryInstructionResult lower_memory_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
  module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  const auto* local_store = std::get_if<bir::StoreLocalInst>(&inst);
  const auto* global_store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (load == nullptr && local_store == nullptr && global_store == nullptr) {
    return LowerMemoryInstructionResult{.handled = false};
  }

  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return LowerMemoryInstructionResult{.handled = true};
  }

  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  if (addressing == nullptr) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess));
    return LowerMemoryInstructionResult{.handled = true};
  }

  PreparedMemoryInstructionRecordResult prepared;
  if (load != nullptr) {
    prepared = make_prepared_frame_slot_load_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *load);
  } else if (local_store != nullptr) {
    prepared = make_prepared_store_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *local_store);
  } else {
    prepared = make_prepared_store_memory_instruction_record(
        context.function.prepared->names,
        *context.function.value_locations,
        *context.function.storage_plan,
        *addressing,
        context.control_flow_block->block_label,
        instruction_index,
        *global_store);
  }
  if (!prepared.record.has_value()) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        memory_error_message(prepared.error));
    return LowerMemoryInstructionResult{.handled = true};
  }

  InstructionRecord target = make_memory_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_memory_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        std::string{target.selection.diagnostic});
    return LowerMemoryInstructionResult{.handled = true};
  }

  return LowerMemoryInstructionResult{
      .handled = true,
      .instruction =
          module::MachineInstruction{
              .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
              .operands = {},
              .target = std::move(target),
              .origin =
                  c4c::backend::mir::MachineOrigin{
                      .reason =
                          c4c::backend::mir::MachineOriginReason::BirInstruction,
                      .function_name = context.function.control_flow->function_name,
                      .block_label = context.control_flow_block->block_label,
                      .instruction_index = instruction_index,
                  },
          },
  };
}

[[nodiscard]] const prepare::PreparedCallArgumentPlan* find_prepared_argument_plan(
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveResolution& move) {
  if (!move.destination_abi_index.has_value()) {
    return nullptr;
  }
  for (const auto& argument : call_plan.arguments) {
    if (argument.arg_index == *move.destination_abi_index &&
        argument.source_value_id == move.from_value_id) {
      return &argument;
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
  if (placement.has_value()) {
    converted = abi::convert_prepared_register(*placement, prepared_class, expected_view);
  } else if (register_name.has_value()) {
    converted = abi::convert_prepared_register(
        *register_name, bank, prepared_class, expected_view);
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

  const bool selected_gpr_argument_move =
      argument != nullptr &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Gpr &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Gpr;
  const bool selected_f128_argument_move =
      argument != nullptr &&
      argument->source_register_bank == prepare::PreparedRegisterBank::Vreg &&
      argument->destination_register_bank == prepare::PreparedRegisterBank::Vreg &&
      complete_full_width_f128_carrier(source_f128_carrier);

  if (bundle.phase == prepare::PreparedMovePhase::BeforeCall &&
      move.destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value() && argument != nullptr &&
      argument->source_encoding == prepare::PreparedStorageEncodingKind::Register &&
      (selected_gpr_argument_move || selected_f128_argument_move) &&
      binding != nullptr &&
      binding->destination_storage_kind == prepare::PreparedMoveStorageKind::Register) {
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
                                    : std::nullopt;
    auto source = make_register_operand_from_prepared_authority(
        source_home->register_name,
        argument->source_register_placement,
        argument->source_register_bank,
        RegisterOperandRole::CallAbi,
        source_home->value_id,
        source_home->value_name,
        selected_f128_argument_move ? source_f128_carrier->contiguous_width : 1,
        selected_f128_argument_move ? source_f128_carrier->occupied_register_names
                                    : std::vector<std::string>{},
        expected_view,
        diagnostics,
        context,
        instruction_index);
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
    if (!source.has_value() || !destination.has_value()) {
      return std::nullopt;
    }
    move_record.source_register = *source;
    move_record.destination_register = *destination;
    move_record.source_f128_carrier =
        selected_f128_argument_move ? source_f128_carrier : nullptr;
  }

  InstructionRecord target = make_call_boundary_move_instruction(std::move(move_record));
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
      (selected_gpr_result_move || selected_f128_result_move) &&
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
        destination_home->register_name,
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

  InstructionRecord target = make_call_boundary_move_instruction(std::move(move_record));
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

[[nodiscard]] std::vector<module::MachineInstruction> lower_after_call_moves(
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
  if (bundle == nullptr) {
    return lowered;
  }
  for (const auto& move : bundle->moves) {
    if (auto instruction =
            lower_after_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
}

[[nodiscard]] std::vector<module::MachineInstruction> lower_before_call_moves(
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
  if (bundle == nullptr) {
    return lowered;
  }
  for (const auto& move : bundle->moves) {
    if (auto instruction =
            lower_before_call_move(context, call_plan, *bundle, move, instruction_index, diagnostics)) {
      lowered.push_back(std::move(*instruction));
    }
  }
  return lowered;
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

  const auto* call_plan = find_prepared_call_plan(context, instruction_index);
  if (call_plan == nullptr) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan,
        context,
        instruction_index,
        "AArch64 call lowering requires an authoritative PreparedCallPlan");
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

  CallInstructionRecord call_record{
      .wrapper_kind = call_plan->wrapper_kind,
      .variadic_fpr_arg_register_count = call_plan->variadic_fpr_arg_register_count,
      .memory_return = call_plan->memory_return,
      .memory_return_storage =
          call_plan->memory_return.has_value()
              ? make_memory_return_storage(context,
                                           *call_plan->memory_return,
                                           instruction_index)
              : std::nullopt,
      .prepared_indirect_callee = call_plan->indirect_callee,
      .prepared_arguments = call_plan->arguments,
      .prepared_result = call_plan->result,
      .preserved_values = call_plan->preserved_values,
      .clobbered_registers = call_plan->clobbered_registers,
      .source_call = call_plan,
      .source_variadic_entry = variadic_entry_plan,
      .source_variadic_helper_operand_homes = variadic_helper_operand_homes,
      .variadic_entry_helper = variadic_helper,
      .calling_convention = call_inst.calling_convention,
      .is_indirect = call_plan->is_indirect,
      .is_variadic =
          call_plan->wrapper_kind == prepare::PreparedCallWrapperKind::DirectExternVariadic ||
          call_inst.is_variadic,
      .is_noreturn = call_inst.is_noreturn,
  };

  if (call_plan->is_indirect) {
    if (!call_inst.is_indirect || !call_plan->indirect_callee.has_value()) {
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          "AArch64 indirect call lowering requires matching retained BIR and prepared indirect callee facts");
      return std::nullopt;
    }
    auto callee = make_indirect_callee_register(
        context, *call_plan->indirect_callee, instruction_index, diagnostics);
    if (!callee.has_value()) {
      return std::nullopt;
    }
    call_record.indirect_callee = make_register_operand(*callee);
  } else {
    if (call_inst.is_indirect || !call_plan->direct_callee_name.has_value()) {
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
        .is_extern = call_plan->wrapper_kind != prepare::PreparedCallWrapperKind::SameModule,
    };
    call_record.direct_callee_label = *call_plan->direct_callee_name;
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
        if (!lowered_memory.handled) {
          lowered_memory =
              lower_memory_instruction(context, inst, instruction_index, diagnostics);
        }
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
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      }
      ++result.visited_operations;
    }
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
