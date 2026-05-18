#include "comparison.hpp"
#include "operands.hpp"

#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <variant>

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

void append_i128_compare_diagnostic(
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

[[nodiscard]] std::string i128_compare_error_message(
    PreparedI128PairRecordError error) {
  std::string message =
      "AArch64 i128 comparison lowering requires prepared i128 carrier facts";
  message += "; error=";
  message += prepared_i128_pair_record_error_name(error);
  return message;
}

[[nodiscard]] RegisterOperandRole register_role_from_authority(
    OperandAuthority authority) {
  switch (authority) {
    case OperandAuthority::RegallocAssignment:
      return RegisterOperandRole::AllocationResult;
    case OperandAuthority::StoragePlan:
      return RegisterOperandRole::StoragePlan;
    case OperandAuthority::PreparedValueHome:
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
  auto resolved = resolve_value_operand(condition_value_id, context.function, diagnostics);
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

[[nodiscard]] std::optional<OperandRecord> make_fused_compare_print_operand(
    const module::BlockLoweringContext& context,
    const CompareValueRecord& value,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (value.source_value.kind == bir::Value::Kind::Immediate) {
    return make_immediate_operand(ImmediateOperand{
        .kind = value.source_value.immediate < 0 ? ImmediateKind::SignedInteger
                                                 : ImmediateKind::UnsignedInteger,
        .type = value.source_value.type,
        .signed_value = value.source_value.immediate,
        .unsigned_value = value.source_value.immediate_bits,
        .source_value_id = value.value_id,
        .source_value_name = value.value_name,
    });
  }

  if (!value.value_id.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_name = value.value_name,
        .message =
            "AArch64 fused compare branch requires prepared compare operand identity",
    });
    return std::nullopt;
  }

  const auto expected_view = scalar_register_view(value.type);
  if (!expected_view.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_id = *value.value_id,
        .value_name = value.value_name,
        .message =
            "AArch64 fused compare branch only supports scalar integer or pointer compare operands",
    });
    return std::nullopt;
  }

  auto resolved = resolve_value_operand(*value.value_id, context.function, diagnostics);
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
        .value_id = *value.value_id,
        .value_name = value.value_name,
        .message =
            "AArch64 fused compare branch requires register compare operands",
    });
    return std::nullopt;
  }

  auto reg = *resolved->register_reference;
  reg.view = *expected_view;

  return make_register_operand(RegisterOperand{
      .reg = reg,
      .role = register_role_from_authority(resolved->authority),
      .value_id = *value.value_id,
      .value_name = value.value_name,
      .expected_view = expected_view,
  });
}

[[nodiscard]] bool install_fused_compare_print_operands(
    const module::BlockLoweringContext& context,
    const BranchConditionRecord& condition,
    InstructionRecord& instruction,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (condition.form != BranchConditionForm::FusedCompare) {
    return true;
  }
  if (!condition.compare_operands.has_value() || instruction.operands.size() < 5U) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .message =
            "AArch64 fused compare branch requires printable compare operands",
    });
    return false;
  }

  auto lhs = make_fused_compare_print_operand(
      context, condition.compare_operands->lhs, diagnostics);
  auto rhs = make_fused_compare_print_operand(
      context, condition.compare_operands->rhs, diagnostics);
  if (!lhs.has_value() || !rhs.has_value()) {
    return false;
  }

  instruction.operands[3] = std::move(*lhs);
  instruction.operands[4] = std::move(*rhs);
  return true;
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

[[nodiscard]] module::MachineInstruction make_bir_compare_machine_instruction(
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

[[nodiscard]] PreparedBranchInstructionRecordResult branch_record_error(
    PreparedBranchRecordError error) {
  return PreparedBranchInstructionRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] bool bir_terminator_target_matches_prepared_label(
    const prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label,
    c4c::BlockLabelId bir_label_id,
    std::string_view bir_label) {
  if (prepared_label == c4c::kInvalidBlockLabel ||
      bir_label_id == c4c::kInvalidBlockLabel) {
    return false;
  }
  if (prepared_label == bir_label_id) {
    return true;
  }
  return names.block_labels.find(bir_label) == prepared_label;
}

[[nodiscard]] bir::Terminator canonicalize_branch_terminator_target(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator) {
  bir::Terminator canonical = terminator;
  if (block.terminator_kind != bir::TerminatorKind::Branch ||
      canonical.kind != bir::TerminatorKind::Branch) {
    return canonical;
  }
  if (bir_terminator_target_matches_prepared_label(names,
                                                   block.branch_target_label,
                                                   canonical.target_label_id,
                                                   canonical.target_label)) {
    canonical.target_label_id = block.branch_target_label;
  }
  return canonical;
}

[[nodiscard]] bir::Terminator canonicalize_conditional_branch_terminator_targets(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator) {
  bir::Terminator canonical = terminator;
  if (block.terminator_kind != bir::TerminatorKind::CondBranch ||
      canonical.kind != bir::TerminatorKind::CondBranch) {
    return canonical;
  }
  if (bir_terminator_target_matches_prepared_label(names,
                                                   block.true_label,
                                                   canonical.true_label_id,
                                                   canonical.true_label)) {
    canonical.true_label_id = block.true_label;
  }
  if (bir_terminator_target_matches_prepared_label(names,
                                                   block.false_label,
                                                   canonical.false_label_id,
                                                   canonical.false_label)) {
    canonical.false_label_id = block.false_label;
  }
  return canonical;
}

}  // namespace

std::string_view comparison_unconditional_branch_mnemonic(
    const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::optional<ComparisonBranchPrintSpelling> comparison_materialized_bool_branch_spelling(
    const InstructionRecord& instruction) {
  const auto condition_mnemonic = machine_instruction_primary_printer_mnemonic(instruction);
  const auto branch_mnemonic =
      machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Branch);
  if (condition_mnemonic.empty() || branch_mnemonic.empty()) {
    return std::nullopt;
  }
  return ComparisonBranchPrintSpelling{
      .condition_mnemonic = condition_mnemonic,
      .branch_mnemonic = branch_mnemonic,
  };
}

std::optional<std::string_view> f128_compare_result_condition(
    prepare::PreparedF128CmpResultZeroTest zero_test) {
  switch (zero_test) {
    case prepare::PreparedF128CmpResultZeroTest::EqualZero:
      return std::string_view{"eq"};
    case prepare::PreparedF128CmpResultZeroTest::NotEqualZero:
      return std::string_view{"ne"};
    case prepare::PreparedF128CmpResultZeroTest::LessThanZero:
      return std::string_view{"lt"};
    case prepare::PreparedF128CmpResultZeroTest::LessOrEqualZero:
      return std::string_view{"le"};
    case prepare::PreparedF128CmpResultZeroTest::GreaterThanZero:
      return std::string_view{"gt"};
    case prepare::PreparedF128CmpResultZeroTest::GreaterOrEqualZero:
      return std::string_view{"ge"};
    case prepare::PreparedF128CmpResultZeroTest::Missing:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string_view> i128_equality_compare_condition(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Eq:
      return std::string_view{"eq"};
    case bir::BinaryOpcode::Ne:
      return std::string_view{"ne"};
    default:
      return std::nullopt;
  }
}

bool is_i128_relational_compare_predicate(bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

std::optional<I128RelationalComparePrintSpelling> i128_relational_compare_spelling(
    bir::BinaryOpcode predicate) {
  if (!is_i128_relational_compare_predicate(predicate)) {
    return std::nullopt;
  }
  const auto signed_high =
      predicate == bir::BinaryOpcode::Slt ||
      predicate == bir::BinaryOpcode::Sle ||
      predicate == bir::BinaryOpcode::Sgt ||
      predicate == bir::BinaryOpcode::Sge;
  const auto greater_predicate =
      predicate == bir::BinaryOpcode::Sgt ||
      predicate == bir::BinaryOpcode::Sge ||
      predicate == bir::BinaryOpcode::Ugt ||
      predicate == bir::BinaryOpcode::Uge;
  const auto inclusive_predicate =
      predicate == bir::BinaryOpcode::Sle ||
      predicate == bir::BinaryOpcode::Sge ||
      predicate == bir::BinaryOpcode::Ule ||
      predicate == bir::BinaryOpcode::Uge;
  return I128RelationalComparePrintSpelling{
      .high_true_condition =
          signed_high ? (greater_predicate ? std::string_view{"gt"} : std::string_view{"lt"})
                      : (greater_predicate ? std::string_view{"hi"} : std::string_view{"lo"}),
      .high_false_condition =
          signed_high ? (greater_predicate ? std::string_view{"lt"} : std::string_view{"gt"})
                      : (greater_predicate ? std::string_view{"lo"} : std::string_view{"hi"}),
      .low_true_condition =
          greater_predicate
              ? (inclusive_predicate ? std::string_view{"hs"} : std::string_view{"hi"})
              : (inclusive_predicate ? std::string_view{"ls"} : std::string_view{"lo"}),
  };
}

PreparedBranchInstructionRecordResult make_prepared_unconditional_branch_record(
    c4c::FunctionNameId function_name,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator) {
  if (function_name == c4c::kInvalidFunctionName) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != bir::TerminatorKind::Branch ||
      terminator.kind != bir::TerminatorKind::Branch) {
    return branch_record_error(PreparedBranchRecordError::TerminatorKindMismatch);
  }
  if (block.branch_target_label == c4c::kInvalidBlockLabel ||
      terminator.target_label_id == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchTarget);
  }
  if (block.branch_target_label != terminator.target_label_id) {
    return branch_record_error(PreparedBranchRecordError::TerminatorTargetMismatch);
  }

  return PreparedBranchInstructionRecordResult{
      .record =
          BranchInstructionRecord{
              .target =
                  BranchTargetOperand{
                      .surface = RecordSurfaceKind::RecordOnly,
                      .block_label = block.branch_target_label,
                      .function_name = function_name,
                  },
              .condition_record =
                  BranchConditionRecord{
                      .surface = RecordSurfaceKind::RecordOnly,
                      .form = BranchConditionForm::Unconditional,
                  },
              .conditional = false,
          },
      .error = PreparedBranchRecordError::None,
  };
}

PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition,
    const bir::Terminator& terminator) {
  if (branch_condition.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != branch_condition.function_name) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel ||
      branch_condition.block_label != block.block_label) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != bir::TerminatorKind::CondBranch ||
      terminator.kind != bir::TerminatorKind::CondBranch) {
    return branch_record_error(PreparedBranchRecordError::TerminatorKindMismatch);
  }
  if (block.true_label == c4c::kInvalidBlockLabel ||
      block.false_label == c4c::kInvalidBlockLabel ||
      branch_condition.true_label == c4c::kInvalidBlockLabel ||
      branch_condition.false_label == c4c::kInvalidBlockLabel ||
      terminator.true_label_id == c4c::kInvalidBlockLabel ||
      terminator.false_label_id == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchTarget);
  }
  if (block.true_label != branch_condition.true_label ||
      block.false_label != branch_condition.false_label ||
      terminator.true_label_id != branch_condition.true_label ||
      terminator.false_label_id != branch_condition.false_label) {
    return branch_record_error(PreparedBranchRecordError::TerminatorTargetMismatch);
  }
  const bool is_fused_compare =
      branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare;
  if (!is_fused_compare &&
      (branch_condition.condition_value.type != bir::TypeKind::I1 ||
       terminator.condition.type != bir::TypeKind::I1)) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchCondition);
  }
  if (is_fused_compare &&
      (branch_condition.condition_value.type == bir::TypeKind::Void ||
       terminator.condition.type == bir::TypeKind::Void)) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchCondition);
  }
  if (branch_condition.condition_value != terminator.condition) {
    return branch_record_error(PreparedBranchRecordError::ConditionValueMismatch);
  }

  const auto* condition_home =
      branch_condition.condition_value.kind == bir::Value::Kind::Named
          ? prepare::find_prepared_value_home(
                names, value_locations, branch_condition.condition_value.name)
          : nullptr;
  if (condition_home == nullptr || condition_home->value_name == c4c::kInvalidValueName) {
    return branch_record_error(PreparedBranchRecordError::MissingConditionValueHome);
  }

  BranchConditionRecord condition{
      .surface = RecordSurfaceKind::RecordOnly,
      .form = branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare
                  ? BranchConditionForm::FusedCompare
                  : BranchConditionForm::MaterializedBool,
      .condition_value_id = condition_home->value_id,
      .condition_value_name = condition_home->value_name,
      .condition_type = branch_condition.condition_value.type,
      .can_fuse_with_branch = branch_condition.can_fuse_with_branch,
  };

  if (branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare) {
    if (!branch_condition.predicate.has_value() || !branch_condition.compare_type.has_value() ||
        !branch_condition.lhs.has_value() || !branch_condition.rhs.has_value()) {
      return branch_record_error(PreparedBranchRecordError::MissingCompareFacts);
    }
    if (!is_compare_predicate(*branch_condition.predicate)) {
      return branch_record_error(PreparedBranchRecordError::UnsupportedComparePredicate);
    }

    const auto make_record = [&names, &value_locations](
                                 const bir::Value& value)
        -> std::optional<CompareValueRecord> {
      CompareValueRecord record{
          .surface = RecordSurfaceKind::RecordOnly,
          .value_id = std::nullopt,
          .value_name = c4c::kInvalidValueName,
          .type = value.type,
          .source_value = value,
      };
      if (value.kind == bir::Value::Kind::Immediate) {
        return record;
      }
      if (value.kind != bir::Value::Kind::Named) {
        return std::nullopt;
      }
      const auto* home =
          prepare::find_prepared_value_home(names, value_locations, value.name);
      if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
        return std::nullopt;
      }
      record.value_id = home->value_id;
      record.value_name = home->value_name;
      return record;
    };

    const auto lhs = make_record(*branch_condition.lhs);
    const auto rhs = make_record(*branch_condition.rhs);
    if (!lhs.has_value() || !rhs.has_value()) {
      return branch_record_error(PreparedBranchRecordError::MissingCompareValueHome);
    }

    condition.predicate = ComparePredicateRecord{
        .surface = RecordSurfaceKind::RecordOnly,
        .source_predicate = *branch_condition.predicate,
        .compare_type = *branch_condition.compare_type,
    };
    condition.compare_operands = CompareOperandPairRecord{
        .surface = RecordSurfaceKind::RecordOnly,
        .lhs = *lhs,
        .rhs = *rhs,
        .compare_type = *branch_condition.compare_type,
    };
  }

  const auto condition_value_id = condition.condition_value_id;
  const BranchTargetPairRecord target_pair{
      .surface = RecordSurfaceKind::RecordOnly,
      .true_target =
          BranchTargetOperand{
              .surface = RecordSurfaceKind::RecordOnly,
              .block_label = branch_condition.true_label,
              .function_name = branch_condition.function_name,
              .condition_value_id = condition_value_id,
          },
      .false_target =
          BranchTargetOperand{
              .surface = RecordSurfaceKind::RecordOnly,
              .block_label = branch_condition.false_label,
              .function_name = branch_condition.function_name,
              .condition_value_id = condition_value_id,
          },
  };
  condition.compare_branch_candidate = BranchCompareCandidateRecord{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind =
          branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare
              ? (branch_condition.can_fuse_with_branch
                     ? BranchCompareCandidateKind::FusedCompareAndBranch
                     : BranchCompareCandidateKind::NonFusableCompare)
              : BranchCompareCandidateKind::MaterializedBoolCondition,
      .condition_value_id = condition.condition_value_id,
      .condition_value_name = condition.condition_value_name,
      .condition_type = condition.condition_type,
      .predicate = condition.predicate,
      .compare_operands = condition.compare_operands,
      .target_pair = target_pair,
      .can_fuse_with_branch = condition.can_fuse_with_branch,
  };

  return PreparedBranchInstructionRecordResult{
      .record =
          BranchInstructionRecord{
              .target = target_pair.true_target,
              .target_pair = target_pair,
              .condition_record = condition,
              .conditional = true,
          },
      .error = PreparedBranchRecordError::None,
  };
}

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

  const auto canonical_terminator =
      context.function.prepared != nullptr
          ? canonicalize_branch_terminator_target(context.function.prepared->names,
                                                  *context.control_flow_block,
                                                  context.bir_block->terminator)
          : context.bir_block->terminator;
  const auto prepared_record = make_prepared_unconditional_branch_record(
      context.function.control_flow->function_name,
      *context.control_flow_block,
      canonical_terminator);
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

  const auto canonical_terminator = canonicalize_conditional_branch_terminator_targets(
      context.function.prepared->names,
      *context.control_flow_block,
      context.bir_block->terminator);
  auto prepared_record = make_prepared_conditional_branch_record(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.control_flow_block,
      *branch_condition,
      canonical_terminator);
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
  const auto* branch =
      std::get_if<BranchInstructionRecord>(&instruction.target.payload);
  if (branch != nullptr && branch->condition_record.has_value() &&
      !install_fused_compare_print_operands(
          context, *branch->condition_record, instruction.target, diagnostics)) {
    return std::nullopt;
  }
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

std::optional<module::MachineInstruction> lower_prepared_i128_compare_instruction(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I1 ||
      !is_compare_predicate(binary.opcode)) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 i128 comparison lowering only supports prepared i128 compare instructions");
    return std::nullopt;
  }
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(
            PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return std::nullopt;
  }

  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(
            PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return std::nullopt;
  }

  if (context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(
            PreparedI128PairRecordError::MissingScalarResultStorage));
    return std::nullopt;
  }

  auto prepared = make_prepared_i128_compare_record(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.function.storage_plan,
      *i128_carriers,
      binary);
  if (!prepared.record.has_value()) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(prepared.error));
    return std::nullopt;
  }

  auto target = make_i128_compare_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (auto* record = std::get_if<I128CompareRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  }
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_compare_diagnostic(diagnostics,
                                   module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                   context,
                                   instruction_index,
                                   std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return make_bir_compare_machine_instruction(
      context, instruction_index, std::move(target));
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
