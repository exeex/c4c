#include "module.hpp"

#include <optional>
#include <string>
#include <utility>

namespace c4c::backend::aarch64::module {
namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;

void append_branch_diagnostic(ModuleLoweringDiagnostics& diagnostics,
                              const BlockLoweringContext& context,
                              std::string message) {
  diagnostics.entries.push_back(ModuleLoweringDiagnostic{
      .kind = ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_family = InstructionLoweringFamily::BranchControl,
      .message = std::move(message),
  });
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_register_view(bir::TypeKind type) {
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

[[nodiscard]] codegen::RegisterOperandRole register_role_from_authority(
    OperandAuthority authority) {
  switch (authority) {
    case OperandAuthority::RegallocAssignment:
      return codegen::RegisterOperandRole::AllocationResult;
    case OperandAuthority::StoragePlan:
      return codegen::RegisterOperandRole::StoragePlan;
    case OperandAuthority::PreparedValueHome:
      return codegen::RegisterOperandRole::ValueHome;
    default:
      return codegen::RegisterOperandRole::Physical;
  }
}

[[nodiscard]] std::optional<codegen::OperandRecord> make_condition_register_operand(
    const BlockLoweringContext& context,
    prepare::PreparedValueId condition_value_id,
    c4c::ValueNameId condition_value_name,
    bir::TypeKind condition_type,
    ModuleLoweringDiagnostics& diagnostics) {
  auto resolved = resolve_value_operand(condition_value_id, context.function, diagnostics);
  if (!resolved.has_value() || !resolved->register_reference.has_value()) {
    diagnostics.entries.push_back(ModuleLoweringDiagnostic{
        .kind = ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = InstructionLoweringFamily::BranchControl,
        .value_id = condition_value_id,
        .value_name = condition_value_name,
        .message =
            "AArch64 materialized-bool branch condition requires typed register authority",
    });
    return std::nullopt;
  }

  return codegen::make_register_operand(codegen::RegisterOperand{
      .reg = *resolved->register_reference,
      .role = register_role_from_authority(resolved->authority),
      .value_id = condition_value_id,
      .value_name = condition_value_name,
      .expected_view = scalar_register_view(condition_type),
  });
}

[[nodiscard]] MachineInstruction make_branch_instruction(
    const BlockLoweringContext& context,
    codegen::BranchInstructionRecord record) {
  codegen::InstructionRecord target =
      codegen::make_branch_instruction(std::move(record));
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;

  return MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirTerminator,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
          },
  };
}

}  // namespace

std::optional<MachineInstruction> lower_prepared_branch_terminator(
    const BlockLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 unconditional branch lowering requires prepared and retained BIR block context");
    return std::nullopt;
  }
  if (context.control_flow_block->terminator_kind != bir::TerminatorKind::Branch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::Branch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 branch-control lowering only supports prepared unconditional branches");
    return std::nullopt;
  }

  const auto prepared_record = codegen::make_prepared_unconditional_branch_record(
      context.function.control_flow->function_name,
      *context.control_flow_block,
      context.bir_block->terminator);
  if (!prepared_record.record.has_value()) {
    append_branch_diagnostic(
        diagnostics,
        context,
        std::string{"AArch64 unconditional branch lowering rejected prepared branch facts: "} +
            std::string{codegen::prepared_branch_record_error_name(prepared_record.error)});
    return std::nullopt;
  }

  auto instruction = make_branch_instruction(context, std::move(*prepared_record.record));
  if (instruction.target.selection.status != codegen::MachineNodeSelectionStatus::Selected) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 unconditional branch lowering did not produce a selected machine node");
    return std::nullopt;
  }
  return instruction;
}

std::optional<MachineInstruction> lower_prepared_conditional_branch_terminator(
    const BlockLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr ||
      context.function.value_locations == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering requires prepared and retained BIR block context");
    return std::nullopt;
  }
  if (context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 branch-control lowering only supports prepared conditional branches");
    return std::nullopt;
  }

  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering requires prepared branch condition authority");
    return std::nullopt;
  }
  if (branch_condition->kind != prepare::PreparedBranchConditionKind::MaterializedBool) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering keeps fused compare branches fail-closed");
    return std::nullopt;
  }

  auto prepared_record = codegen::make_prepared_conditional_branch_record(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.control_flow_block,
      *branch_condition,
      context.bir_block->terminator);
  if (!prepared_record.record.has_value()) {
    append_branch_diagnostic(
        diagnostics,
        context,
        std::string{"AArch64 conditional branch lowering rejected prepared branch facts: "} +
            std::string{codegen::prepared_branch_record_error_name(prepared_record.error)});
    return std::nullopt;
  }

  auto& record = *prepared_record.record;
  if (!record.condition_record.has_value() ||
      !record.condition_record->condition_value_id.has_value()) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering requires prepared condition value identity");
    return std::nullopt;
  }
  auto condition_operand = make_condition_register_operand(
      context,
      *record.condition_record->condition_value_id,
      record.condition_record->condition_value_name,
      record.condition_record->condition_type,
      diagnostics);
  if (!condition_operand.has_value()) {
    return std::nullopt;
  }
  record.condition = std::move(*condition_operand);

  auto instruction = make_branch_instruction(context, std::move(record));
  if (instruction.target.selection.status != codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.opcode != codegen::MachineOpcode::ConditionalBranch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 materialized-bool branch lowering did not produce a selected conditional node");
    return std::nullopt;
  }
  return instruction;
}

mir::MachineBlockSuccessor make_unconditional_branch_successor(
    const BlockLoweringContext& context) {
  return mir::MachineBlockSuccessor{
      .target_label = context.control_flow_block != nullptr
                          ? context.control_flow_block->branch_target_label
                          : c4c::kInvalidBlockLabel,
      .kind = mir::MachineBlockSuccessorKind::Unconditional,
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirTerminator,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
          },
  };
}

std::vector<mir::MachineBlockSuccessor> make_conditional_branch_successors(
    const BlockLoweringContext& context) {
  const mir::MachineOrigin origin{
      .reason = mir::MachineOriginReason::BirTerminator,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
  };
  return {
      mir::MachineBlockSuccessor{
          .target_label = context.control_flow_block != nullptr
                              ? context.control_flow_block->true_label
                              : c4c::kInvalidBlockLabel,
          .kind = mir::MachineBlockSuccessorKind::ConditionalTrue,
          .origin = origin,
      },
      mir::MachineBlockSuccessor{
          .target_label = context.control_flow_block != nullptr
                              ? context.control_flow_block->false_label
                              : c4c::kInvalidBlockLabel,
          .kind = mir::MachineBlockSuccessorKind::ConditionalFalse,
          .origin = origin,
      },
  };
}

}  // namespace c4c::backend::aarch64::module
