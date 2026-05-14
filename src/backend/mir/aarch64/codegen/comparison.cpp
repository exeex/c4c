#include "comparison.hpp"

#include <optional>
#include <string>
#include <utility>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

void append_branch_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                              const module::BlockLoweringContext& context,
                              std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_family = module::InstructionLoweringFamily::BranchControl,
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

[[nodiscard]] RegisterOperandRole register_role_from_authority(
    module::OperandAuthority authority) {
  switch (authority) {
    case module::OperandAuthority::RegallocAssignment:
      return RegisterOperandRole::AllocationResult;
    case module::OperandAuthority::StoragePlan:
      return RegisterOperandRole::StoragePlan;
    case module::OperandAuthority::PreparedValueHome:
      return RegisterOperandRole::ValueHome;
    default:
      return RegisterOperandRole::Physical;
  }
}

[[nodiscard]] std::optional<OperandRecord> make_condition_register_operand(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId condition_value_id,
    c4c::ValueNameId condition_value_name,
    bir::TypeKind condition_type,
    module::ModuleLoweringDiagnostics& diagnostics) {
  auto resolved = module::resolve_value_operand(condition_value_id, context.function, diagnostics);
  if (!resolved.has_value() || !resolved->register_reference.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_id = condition_value_id,
        .value_name = condition_value_name,
        .message =
            "AArch64 materialized-bool branch condition requires typed register authority",
    });
    return std::nullopt;
  }

  return make_register_operand(RegisterOperand{
      .reg = *resolved->register_reference,
      .role = register_role_from_authority(resolved->authority),
      .value_id = condition_value_id,
      .value_name = condition_value_name,
      .expected_view = scalar_register_view(condition_type),
  });
}

[[nodiscard]] module::MachineInstruction make_branch_instruction(
    const module::BlockLoweringContext& context,
    BranchInstructionRecord record) {
  InstructionRecord target = make_branch_instruction(std::move(record));
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;

  return module::MachineInstruction{
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

std::optional<module::MachineInstruction> lower_prepared_branch_terminator(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
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

  const auto prepared_record = make_prepared_unconditional_branch_record(
      context.function.control_flow->function_name,
      *context.control_flow_block,
      context.bir_block->terminator);
  if (!prepared_record.record.has_value()) {
    append_branch_diagnostic(
        diagnostics,
        context,
        std::string{"AArch64 unconditional branch lowering rejected prepared branch facts: "} +
            std::string{prepared_branch_record_error_name(prepared_record.error)});
    return std::nullopt;
  }

  auto instruction = make_branch_instruction(context, std::move(*prepared_record.record));
  if (instruction.target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 unconditional branch lowering did not produce a selected machine node");
    return std::nullopt;
  }
  return instruction;
}

std::optional<module::MachineInstruction> lower_prepared_conditional_branch_terminator(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
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
  if (branch_condition->kind == prepare::PreparedBranchConditionKind::FusedCompare &&
      !branch_condition->can_fuse_with_branch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering keeps non-fusable compare branches fail-closed");
    return std::nullopt;
  }

  auto prepared_record = make_prepared_conditional_branch_record(
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
            std::string{prepared_branch_record_error_name(prepared_record.error)});
    return std::nullopt;
  }

  auto& record = *prepared_record.record;
  if (branch_condition->kind == prepare::PreparedBranchConditionKind::MaterializedBool) {
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
  }

  auto instruction = make_branch_instruction(context, std::move(record));
  const auto expected_opcode =
      branch_condition->kind == prepare::PreparedBranchConditionKind::FusedCompare
          ? MachineOpcode::CompareBranch
          : MachineOpcode::ConditionalBranch;
  if (instruction.target.selection.status != MachineNodeSelectionStatus::Selected ||
      instruction.target.opcode != expected_opcode) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering did not produce a selected branch-control node");
    return std::nullopt;
  }
  return instruction;
}

mir::MachineBlockSuccessor make_unconditional_branch_successor(
    const module::BlockLoweringContext& context) {
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
    const module::BlockLoweringContext& context) {
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

}  // namespace c4c::backend::aarch64::codegen
