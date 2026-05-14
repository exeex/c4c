#include "module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::module {

namespace {

namespace bir = c4c::backend::bir;

struct BlockScalarLoweringState {
  std::unordered_map<c4c::ValueNameId, codegen::RegisterOperand> emitted_registers;
};

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
      .expected_view = scalar_register_view(value.type),
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
    const BlockScalarLoweringState& scalar_state,
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
        home == nullptr ? scalar_state.emitted_registers.end()
                        : scalar_state.emitted_registers.find(home->value_name);
    if (emitted != scalar_state.emitted_registers.end()) {
      record.value = codegen::make_register_operand(emitted->second);
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

[[nodiscard]] std::optional<codegen::RegisterOperand> find_return_abi_register(
    const BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(type);
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BeforeReturn ||
        bundle.block_index != context.block_index) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.from_value_id != value_id ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      abi::PreparedRegisterConversionResult converted;
      if (move.destination_register_placement.has_value()) {
        converted = abi::convert_prepared_register(*move.destination_register_placement,
                                                   std::nullopt,
                                                   expected_view);
      } else if (move.destination_register_name.has_value()) {
        converted = abi::convert_prepared_register(*move.destination_register_name,
                                                   std::nullopt,
                                                   std::nullopt,
                                                   expected_view);
      } else {
        continue;
      }
      if (!converted.reg.has_value()) {
        continue;
      }
      return codegen::RegisterOperand{
          .reg = *converted.reg,
          .role = codegen::RegisterOperandRole::CallAbi,
          .value_id = value_id,
          .value_name = value_name,
          .expected_view = expected_view,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<codegen::RegisterOperand> retarget_register_operand(
    codegen::RegisterOperand reg,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    bir::TypeKind type) {
  reg.value_id = value_id;
  reg.value_name = value_name;
  reg.expected_view = scalar_register_view(type);
  return reg;
}

[[nodiscard]] bool value_matches_name(const bir::Value& value,
                                      std::string_view name) {
  return value.kind == bir::Value::Kind::Named && value.name == name;
}

[[nodiscard]] std::optional<codegen::RegisterOperand> find_return_chain_register(
    const BlockLoweringContext& context,
    std::size_t instruction_index,
    const prepare::PreparedValueHome& result_home,
    bir::TypeKind result_type) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr) {
    return std::nullopt;
  }

  std::string current_name =
      std::string(prepare::prepared_value_name(context.function.prepared->names,
                                               result_home.value_name));
  if (current_name.empty()) {
    return std::nullopt;
  }

  for (std::size_t next_index = instruction_index + 1;
       next_index < context.bir_block->insts.size();
       ++next_index) {
    const auto* next_binary =
        std::get_if<bir::BinaryInst>(&context.bir_block->insts[next_index]);
    if (next_binary == nullptr ||
        !codegen::is_scalar_alu_integer_opcode(next_binary->opcode)) {
      return std::nullopt;
    }
    const bool consumes_current = value_matches_name(next_binary->lhs, current_name) ||
                                  value_matches_name(next_binary->rhs, current_name);
    if (!consumes_current || next_binary->result.kind != bir::Value::Kind::Named) {
      return std::nullopt;
    }
    current_name = next_binary->result.name;
  }

  if (!context.bir_block->terminator.value.has_value() ||
      !value_matches_name(*context.bir_block->terminator.value, current_name)) {
    return std::nullopt;
  }

  const auto* terminal_home =
      find_named_value_home(*context.bir_block->terminator.value, context.function);
  if (terminal_home == nullptr) {
    return std::nullopt;
  }
  auto terminal_register =
      find_return_abi_register(context,
                               terminal_home->value_id,
                               terminal_home->value_name,
                               context.bir_block->terminator.value->type);
  if (!terminal_register.has_value()) {
    return std::nullopt;
  }
  return retarget_register_operand(*terminal_register,
                                   result_home.value_id,
                                   result_home.value_name,
                                   result_type);
}

[[nodiscard]] std::optional<codegen::OperandRecord> make_scalar_fallback_operand(
    const bir::Value& value,
    const BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    ModuleLoweringDiagnostics& diagnostics) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return make_immediate_return_operand(value);
  }
  const auto* home = find_named_value_home(value, context.function);
  const auto emitted =
      home == nullptr ? scalar_state.emitted_registers.end()
                      : scalar_state.emitted_registers.find(home->value_name);
  if (emitted != scalar_state.emitted_registers.end()) {
    return codegen::make_register_operand(emitted->second);
  }
  return make_named_return_operand(value, context, diagnostics);
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

[[nodiscard]] std::optional<MachineInstruction> lower_scalar_instruction(
    const BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr) {
    return std::nullopt;
  }

  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr) {
    return std::nullopt;
  }

  const auto prepared =
      codegen::make_prepared_scalar_alu_instruction_record(
          context.function.prepared->names,
          *context.function.value_locations,
          *context.function.storage_plan,
          *binary);
  std::optional<codegen::ScalarInstructionRecord> scalar_record = prepared.record;
  if (!scalar_record.has_value()) {
    const auto* result_home = find_named_value_home(binary->result, context.function);
    auto result_register =
        result_home == nullptr
            ? std::optional<codegen::RegisterOperand>{}
            : find_return_abi_register(context,
                                       result_home->value_id,
                                       result_home->value_name,
                                       binary->result.type);
    if (!result_register.has_value() && result_home != nullptr) {
      result_register =
          find_return_chain_register(context, instruction_index, *result_home, binary->result.type);
    }
    const auto lhs =
        make_scalar_fallback_operand(binary->lhs, context, scalar_state, diagnostics);
    const auto rhs =
        make_scalar_fallback_operand(binary->rhs, context, scalar_state, diagnostics);
    if (result_home == nullptr || !result_register.has_value() || !lhs.has_value() ||
        !rhs.has_value() || !codegen::is_scalar_alu_integer_opcode(binary->opcode)) {
      return std::nullopt;
    }
    scalar_record = codegen::make_scalar_alu_instruction_record(codegen::ScalarAluRecord{
        .surface = codegen::RecordSurfaceKind::MachineInstructionNode,
        .operation = codegen::scalar_alu_operation_from_binary_opcode(binary->opcode),
        .source_binary_opcode = binary->opcode,
        .operand_type = binary->operand_type,
        .result_value_id = result_home->value_id,
        .result_value_name = result_home->value_name,
        .result_type = binary->result.type,
        .result_register = *result_register,
        .lhs = *lhs,
        .rhs = *rhs,
        .supported_integer_operation = true,
    });
  }

  codegen::InstructionRecord target = codegen::make_scalar_instruction(*scalar_record);
  if (target.selection.status != codegen::MachineNodeSelectionStatus::Selected) {
    return std::nullopt;
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (scalar_record->result_register.has_value() &&
      scalar_record->result_value_name != c4c::kInvalidValueName) {
    scalar_state.emitted_registers[scalar_record->result_value_name] =
        *scalar_record->result_register;
  }

  return MachineInstruction{
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

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  if (context.bir_block != nullptr) {
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      if (auto lowered = lower_scalar_instruction(
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
