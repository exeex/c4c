#include "returns.hpp"

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::optional<OperandRecord> make_immediate_return_operand(
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

[[nodiscard]] std::optional<OperandRecord> make_resolved_return_operand(
    const module::ResolvedOperand& resolved,
    const bir::Value& value) {
  if (const auto* immediate = std::get_if<mir::Immediate>(&resolved.operand.payload)) {
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

[[nodiscard]] std::optional<OperandRecord> make_named_return_operand(
    const bir::Value& value,
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* home = find_named_value_home(value, context.function);
  if (home == nullptr) {
    return std::nullopt;
  }
  auto resolved =
      module::resolve_value_operand(home->value_id, context.function, diagnostics);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  return make_resolved_return_operand(*resolved, value);
}

[[nodiscard]] std::optional<ReturnInstructionRecord> make_return_record(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  ReturnInstructionRecord record;
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
        home == nullptr ? std::optional<RegisterOperand>{}
                        : find_emitted_scalar_register(scalar_state,
                                                       home->value_name);
    if (emitted.has_value()) {
      record.value = make_register_operand(*emitted);
    } else {
      record.value = make_named_return_operand(value, context, diagnostics);
    }
  }
  if (!record.value.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
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

[[nodiscard]] module::MachineInstruction make_return_machine_instruction(
    const module::BlockLoweringContext& context,
    ReturnInstructionRecord record) {
  InstructionRecord target = make_return_instruction(std::move(record));
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;

  return module::MachineInstruction{
      .opcode = static_cast<mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirTerminator,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
          },
  };
}

}  // namespace

std::optional<module::MachineInstruction> lower_prepared_return_terminator(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto record = make_return_record(context, scalar_state, diagnostics)) {
    return make_return_machine_instruction(context, std::move(*record));
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::codegen
