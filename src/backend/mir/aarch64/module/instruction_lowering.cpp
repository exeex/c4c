#include "module.hpp"

#include "../codegen/alu.hpp"
#include "../codegen/comparison.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::module {

namespace {

namespace bir = c4c::backend::bir;

void append_block_diagnostic(ModuleLoweringDiagnostics& diagnostics,
                             ModuleLoweringDiagnosticKind kind,
                             const BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(ModuleLoweringDiagnostic{
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

[[nodiscard]] std::optional<codegen::OperandRecord> make_immediate_return_operand(
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  codegen::ImmediateKind immediate_kind = codegen::ImmediateKind::SignedInteger;
  if (value.type == bir::TypeKind::I1) {
    immediate_kind = codegen::ImmediateKind::Boolean;
  }

  switch (value.type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      return codegen::make_immediate_operand(codegen::ImmediateOperand{
          .kind = immediate_kind,
          .type = value.type,
          .signed_value = value.immediate,
          .unsigned_value = value.immediate_bits != 0U
                                ? value.immediate_bits
                                : static_cast<std::uint64_t>(value.immediate),
      });
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<codegen::OperandRecord> make_resolved_return_operand(
    const ResolvedOperand& resolved,
    const bir::Value& value) {
  if (const auto* immediate =
          std::get_if<c4c::backend::mir::Immediate>(&resolved.operand.payload)) {
    return codegen::make_immediate_operand(codegen::ImmediateOperand{
        .kind = immediate->kind == c4c::backend::mir::ImmediateKind::Boolean
                    ? codegen::ImmediateKind::Boolean
                    : codegen::ImmediateKind::SignedInteger,
        .type = value.type,
        .signed_value = immediate->signed_value,
        .unsigned_value = immediate->unsigned_value,
        .source_value_id = resolved.value_id,
        .source_value_name = resolved.value_name,
    });
  }

  if (!resolved.register_reference.has_value()) {
    return std::nullopt;
  }

  codegen::RegisterOperandRole role = codegen::RegisterOperandRole::ValueHome;
  switch (resolved.authority) {
    case OperandAuthority::RegallocAssignment:
      role = codegen::RegisterOperandRole::AllocationResult;
      break;
    case OperandAuthority::StoragePlan:
      role = codegen::RegisterOperandRole::StoragePlan;
      break;
    case OperandAuthority::PreparedValueHome:
      role = codegen::RegisterOperandRole::ValueHome;
      break;
    default:
      role = codegen::RegisterOperandRole::Physical;
      break;
  }

  return codegen::make_register_operand(codegen::RegisterOperand{
      .reg = *resolved.register_reference,
      .role = role,
      .value_id = resolved.value_id,
      .value_name = resolved.value_name,
      .expected_view = codegen::scalar_register_view(value.type),
  });
}

[[nodiscard]] const prepare::PreparedValueHome* find_named_value_home(
    const bir::Value& value,
    const FunctionLoweringContext& context) {
  if (context.prepared == nullptr || context.value_locations == nullptr ||
      value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  const auto value_name =
      prepare::resolve_prepared_value_name_id(context.prepared->names, value.name);
  if (!value_name.has_value()) {
    return nullptr;
  }
  return prepare::find_prepared_value_home(*context.value_locations, *value_name);
}

[[nodiscard]] std::optional<codegen::OperandRecord> make_named_return_operand(
    const bir::Value& value,
    const BlockLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  const auto* home = find_named_value_home(value, context.function);
  if (home == nullptr) {
    return std::nullopt;
  }
  auto resolved = resolve_value_operand(home->value_id, context.function, diagnostics);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  return make_resolved_return_operand(*resolved, value);
}

[[nodiscard]] std::optional<codegen::ReturnInstructionRecord> make_return_record(
    const BlockLoweringContext& context,
    const codegen::BlockScalarLoweringState& scalar_state,
    ModuleLoweringDiagnostics& diagnostics) {
  codegen::ReturnInstructionRecord record;
  if (context.bir_block == nullptr || !context.bir_block->terminator.value.has_value()) {
    return record;
  }

  const auto& value = *context.bir_block->terminator.value;
  record.value_type = value.type;
  if (value.kind == bir::Value::Kind::Immediate) {
    record.value = make_immediate_return_operand(value);
  } else {
    const auto* home = find_named_value_home(value, context.function);
    const auto emitted =
        home == nullptr ? std::optional<codegen::RegisterOperand>{}
                        : codegen::find_emitted_scalar_register(scalar_state,
                                                                home->value_name);
    if (emitted.has_value()) {
      record.value = codegen::make_register_operand(*emitted);
    } else {
      record.value = make_named_return_operand(value, context, diagnostics);
    }
  }
  if (!record.value.has_value()) {
    diagnostics.entries.push_back(ModuleLoweringDiagnostic{
        .kind = ModuleLoweringDiagnosticKind::MissingValueAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .message = "AArch64 return value does not have supported typed operand authority",
    });
    return std::nullopt;
  }
  return record;
}

[[nodiscard]] MachineInstruction make_return_instruction(
    const BlockLoweringContext& context,
    codegen::ReturnInstructionRecord record) {
  codegen::InstructionRecord target =
      codegen::make_return_instruction(std::move(record));
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

[[nodiscard]] const bir::Block* find_bir_block(
    const FunctionLoweringContext& function,
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

[[nodiscard]] InstructionLoweringFamily classify_instruction(const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return InstructionLoweringFamily::Memory;
        }
        return InstructionLoweringFamily::Unknown;
      },
      inst);
}

void append_unsupported_instruction_diagnostic(ModuleLoweringDiagnostics& diagnostics,
                                               const BlockLoweringContext& context,
                                               const bir::Inst& inst,
                                               std::size_t instruction_index) {
  diagnostics.entries.push_back(ModuleLoweringDiagnostic{
      .kind = ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
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

}  // namespace

BlockLoweringContext make_block_lowering_context(
    FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const BlockLoweringContext& context,
    MachineBlock& block,
    ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            ModuleLoweringDiagnosticKind::MissingBlockContext,
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
        ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  codegen::BlockScalarLoweringState scalar_state;
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (auto lowered = codegen::lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        append_unsupported_instruction_diagnostic(
            diagnostics, context, inst, instruction_index);
      }
      ++result.visited_operations;
    }
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    if (auto record = make_return_record(context, scalar_state, diagnostics)) {
      block.instructions.push_back(make_return_instruction(context, std::move(*record)));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    if (auto lowered = codegen::lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(codegen::make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (auto lowered =
            codegen::lower_prepared_conditional_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = codegen::make_conditional_branch_successors(context);
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::module
