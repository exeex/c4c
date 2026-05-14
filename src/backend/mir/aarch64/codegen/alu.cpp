#include "alu.hpp"
#include "operands.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace mir = c4c::backend::mir;

[[nodiscard]] std::optional<OperandRecord> make_immediate_scalar_operand(
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

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
      return make_immediate_operand(ImmediateOperand{
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

[[nodiscard]] std::optional<OperandRecord> make_resolved_scalar_operand(
    const ResolvedOperand& resolved,
    const bir::Value& value) {
  if (const auto* immediate =
          std::get_if<mir::Immediate>(&resolved.operand.payload)) {
    return make_immediate_operand(ImmediateOperand{
        .kind = immediate->kind == mir::ImmediateKind::Boolean
                    ? ImmediateKind::Boolean
                    : ImmediateKind::SignedInteger,
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

  return make_register_operand(RegisterOperand{
      .reg = *resolved.register_reference,
      .role = register_role_from_authority(resolved.authority),
      .value_id = resolved.value_id,
      .value_name = resolved.value_name,
      .expected_view = scalar_register_view(value.type),
  });
}

[[nodiscard]] const prepare::PreparedValueHome* find_named_value_home(
    const bir::Value& value,
    const module::FunctionLoweringContext& context) {
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

[[nodiscard]] std::optional<OperandRecord> make_named_scalar_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* home = find_named_value_home(value, context.function);
  if (home == nullptr) {
    return std::nullopt;
  }
  auto resolved =
      resolve_value_operand(home->value_id, context.function, diagnostics);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  return make_resolved_scalar_operand(*resolved, value);
}

[[nodiscard]] std::optional<RegisterOperand> find_return_abi_register(
    const module::BlockLoweringContext& context,
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
      return RegisterOperand{
          .reg = *converted.reg,
          .role = RegisterOperandRole::CallAbi,
          .value_id = value_id,
          .value_name = value_name,
          .expected_view = expected_view,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<RegisterOperand> retarget_register_operand(
    RegisterOperand reg,
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

[[nodiscard]] std::optional<RegisterOperand> find_return_chain_register(
    const module::BlockLoweringContext& context,
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
        !is_scalar_alu_integer_opcode(next_binary->opcode)) {
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

[[nodiscard]] std::optional<OperandRecord> make_scalar_fallback_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return make_immediate_scalar_operand(value);
  }
  const auto* home = find_named_value_home(value, context.function);
  const auto emitted =
      home == nullptr ? std::optional<RegisterOperand>{}
                      : find_emitted_scalar_register(scalar_state, home->value_name);
  if (emitted.has_value()) {
    return make_register_operand(*emitted);
  }
  return make_named_scalar_operand(value, context, diagnostics);
}

}  // namespace

std::optional<abi::RegisterView> scalar_register_view(bir::TypeKind type) {
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

std::optional<RegisterOperand> find_emitted_scalar_register(
    const BlockScalarLoweringState& state,
    c4c::ValueNameId value_name) {
  const auto emitted = state.emitted_registers.find(value_name);
  if (emitted == state.emitted_registers.end()) {
    return std::nullopt;
  }
  return emitted->second;
}

void record_emitted_scalar_register(BlockScalarLoweringState& state,
                                    c4c::ValueNameId value_name,
                                    RegisterOperand reg) {
  if (value_name == c4c::kInvalidValueName) {
    return;
  }
  state.emitted_registers[value_name] = std::move(reg);
}

std::optional<module::MachineInstruction> lower_scalar_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
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
      make_prepared_scalar_alu_instruction_record(
          context.function.prepared->names,
          *context.function.value_locations,
          *context.function.storage_plan,
          *binary);
  std::optional<ScalarInstructionRecord> scalar_record = prepared.record;
  if (!scalar_record.has_value()) {
    const auto* result_home = find_named_value_home(binary->result, context.function);
    auto result_register =
        result_home == nullptr
            ? std::optional<RegisterOperand>{}
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
        !rhs.has_value() || !is_scalar_alu_integer_opcode(binary->opcode)) {
      return std::nullopt;
    }
    scalar_record = make_scalar_alu_instruction_record(ScalarAluRecord{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .operation = scalar_alu_operation_from_binary_opcode(binary->opcode),
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

  InstructionRecord target = make_scalar_instruction(*scalar_record);
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    return std::nullopt;
  }
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (scalar_record->result_register.has_value()) {
    record_emitted_scalar_register(scalar_state,
                                   scalar_record->result_value_name,
                                   *scalar_record->result_register);
  }

  return module::MachineInstruction{
      .opcode = static_cast<mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
              .instruction_index = instruction_index,
          },
  };
}

}  // namespace c4c::backend::aarch64::codegen
