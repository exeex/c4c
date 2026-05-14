#include "module.hpp"

#include <cstdint>
#include <utility>

namespace c4c::backend::aarch64::module {
namespace {

namespace mir = c4c::backend::mir;

[[nodiscard]] mir::PhysicalRegister to_mir_register(abi::RegisterReference reg) {
  return mir::PhysicalRegister{
      .bank = static_cast<mir::RegisterBankId>(reg.bank),
      .view = static_cast<mir::RegisterViewId>(reg.view),
      .index = static_cast<mir::RegisterIndex>(reg.index),
  };
}

[[nodiscard]] std::optional<ResolvedOperand> make_register_operand(
    const prepare::PreparedRegisterPlacement& placement,
    std::optional<prepare::PreparedRegisterClass> prepared_class,
    OperandAuthority authority,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    c4c::FunctionNameId function_name,
    ModuleLoweringDiagnostics& diagnostics) {
  const auto converted =
      abi::convert_prepared_register(placement, prepared_class, std::nullopt);
  if (!converted.reg.has_value()) {
    diagnostics.entries.push_back(ModuleLoweringDiagnostic{
        .kind = ModuleLoweringDiagnosticKind::RegisterConversionFailed,
        .function_name = function_name,
        .value_id = value_id,
        .value_name = value_name,
        .message = converted.error.has_value()
                       ? converted.error->message
                       : "prepared register placement could not be converted",
    });
    return std::nullopt;
  }

  return ResolvedOperand{
      .operand = mir::Operand{.payload = to_mir_register(*converted.reg)},
      .authority = authority,
      .register_reference = converted.reg,
      .frame_slot_id = std::nullopt,
      .value_id = value_id,
      .value_name = value_name,
  };
}

[[nodiscard]] std::optional<ResolvedOperand> resolve_from_regalloc(
    prepare::PreparedValueId value_id,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.regalloc == nullptr) {
    return std::nullopt;
  }

  for (const auto& value : context.regalloc->values) {
    if (value.value_id != value_id) {
      continue;
    }
    if (value.assigned_register.has_value() &&
        value.assigned_register->placement.has_value()) {
      return make_register_operand(*value.assigned_register->placement,
                                   value.assigned_register->reg_class,
                                   OperandAuthority::RegallocAssignment,
                                   value.value_id,
                                   value.value_name,
                                   value.function_name,
                                   diagnostics);
    }
    if (value.assigned_stack_slot.has_value()) {
      return ResolvedOperand{
          .operand = mir::Operand{.payload = mir::Memory{
                                      .base = std::nullopt,
                                      .index = std::nullopt,
                                      .displacement = static_cast<std::int64_t>(
                                          value.assigned_stack_slot->offset_bytes),
                                      .scale = 1,
                                  }},
          .authority = OperandAuthority::FrameSlot,
          .frame_slot_id = value.assigned_stack_slot->slot_id,
          .value_id = value.value_id,
          .value_name = value.value_name,
      };
    }
    return std::nullopt;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<ResolvedOperand> resolve_from_storage_plan(
    prepare::PreparedValueId value_id,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.storage_plan == nullptr) {
    return std::nullopt;
  }

  for (const auto& value : context.storage_plan->values) {
    if (value.value_id != value_id) {
      continue;
    }

    switch (value.encoding) {
      case prepare::PreparedStorageEncodingKind::Register:
        if (!value.register_placement.has_value()) {
          diagnostics.entries.push_back(ModuleLoweringDiagnostic{
              .kind = ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
              .function_name = context.storage_plan->function_name,
              .value_id = value.value_id,
              .value_name = value.value_name,
              .message =
                  "storage-plan register value is missing typed register placement",
          });
          return std::nullopt;
        }
        return make_register_operand(*value.register_placement,
                                     std::nullopt,
                                     OperandAuthority::StoragePlan,
                                     value.value_id,
                                     value.value_name,
                                     context.storage_plan->function_name,
                                     diagnostics);
      case prepare::PreparedStorageEncodingKind::FrameSlot:
        if (!value.slot_id.has_value()) {
          break;
        }
        return ResolvedOperand{
            .operand = mir::Operand{.payload = mir::Memory{
                                        .base = std::nullopt,
                                        .index = std::nullopt,
                                        .displacement = value.stack_offset_bytes.has_value()
                                                            ? static_cast<std::int64_t>(
                                                                  *value.stack_offset_bytes)
                                                            : 0,
                                        .scale = 1,
                                    }},
            .authority = value.spill_slot_placement.has_value()
                             ? OperandAuthority::SpillSlot
                             : OperandAuthority::FrameSlot,
            .frame_slot_id = value.slot_id,
            .value_id = value.value_id,
            .value_name = value.value_name,
        };
      case prepare::PreparedStorageEncodingKind::Immediate:
        if (!value.immediate_i32.has_value()) {
          break;
        }
        return ResolvedOperand{
            .operand = mir::Operand{
                .payload = mir::Immediate{
                    .kind = mir::ImmediateKind::Signed,
                    .signed_value = *value.immediate_i32,
                    .unsigned_value = static_cast<std::uint64_t>(*value.immediate_i32),
                }},
            .authority = OperandAuthority::Immediate,
            .value_id = value.value_id,
            .value_name = value.value_name,
        };
      case prepare::PreparedStorageEncodingKind::SymbolAddress:
        if (!value.symbol_name.has_value()) {
          break;
        }
        return ResolvedOperand{
            .operand = mir::Operand{.payload = mir::Symbol{.name = *value.symbol_name}},
            .authority = OperandAuthority::Symbol,
            .value_id = value.value_id,
            .value_name = value.value_name,
        };
      case prepare::PreparedStorageEncodingKind::ComputedAddress:
      case prepare::PreparedStorageEncodingKind::None:
        break;
    }

    diagnostics.entries.push_back(ModuleLoweringDiagnostic{
        .kind = ModuleLoweringDiagnosticKind::UnsupportedStoragePlan,
        .function_name = context.storage_plan->function_name,
        .value_id = value.value_id,
        .value_name = value.value_name,
        .message = "storage-plan value does not have a supported typed operand form",
    });
    return std::nullopt;
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<ResolvedOperand> resolve_from_value_home(
    prepare::PreparedValueId value_id,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.value_locations == nullptr) {
    return std::nullopt;
  }

  const auto* home = prepare::find_prepared_value_home(*context.value_locations, value_id);
  if (home == nullptr) {
    return std::nullopt;
  }

  switch (home->kind) {
    case prepare::PreparedValueHomeKind::StackSlot:
      if (!home->slot_id.has_value()) {
        break;
      }
      return ResolvedOperand{
          .operand = mir::Operand{.payload = mir::Memory{
                                      .base = std::nullopt,
                                      .index = std::nullopt,
                                      .displacement = home->offset_bytes.has_value()
                                                          ? static_cast<std::int64_t>(
                                                                *home->offset_bytes)
                                                          : 0,
                                      .scale = 1,
                                  }},
          .authority = OperandAuthority::PreparedValueHome,
          .frame_slot_id = home->slot_id,
          .value_id = home->value_id,
          .value_name = home->value_name,
      };
    case prepare::PreparedValueHomeKind::RematerializableImmediate:
      if (!home->immediate_i32.has_value()) {
        break;
      }
      return ResolvedOperand{
          .operand = mir::Operand{
              .payload = mir::Immediate{
                  .kind = mir::ImmediateKind::Signed,
                  .signed_value = *home->immediate_i32,
                  .unsigned_value = static_cast<std::uint64_t>(*home->immediate_i32),
              }},
          .authority = OperandAuthority::PreparedValueHome,
          .value_id = home->value_id,
          .value_name = home->value_name,
      };
    case prepare::PreparedValueHomeKind::Register:
      diagnostics.entries.push_back(ModuleLoweringDiagnostic{
          .kind = ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
          .function_name = home->function_name,
          .value_id = home->value_id,
          .value_name = home->value_name,
          .message =
              "value-home register spelling is diagnostic-only until typed placement exists",
      });
      return std::nullopt;
    case prepare::PreparedValueHomeKind::PointerBasePlusOffset:
    case prepare::PreparedValueHomeKind::None:
      break;
  }

  diagnostics.entries.push_back(ModuleLoweringDiagnostic{
      .kind = ModuleLoweringDiagnosticKind::UnsupportedValueHome,
      .function_name = home->function_name,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .message = "prepared value home does not have a supported typed operand form",
  });
  return std::nullopt;
}

}  // namespace

std::optional<ResolvedOperand> resolve_value_operand(
    prepare::PreparedValueId value_id,
    const FunctionLoweringContext& context,
    ModuleLoweringDiagnostics& diagnostics) {
  if (context.control_flow == nullptr) {
    diagnostics.entries.push_back(ModuleLoweringDiagnostic{
        .kind = ModuleLoweringDiagnosticKind::MissingFunctionContext,
        .value_id = value_id,
        .message = "operand resolution requires a prepared function context",
    });
    return std::nullopt;
  }

  if (auto resolved = resolve_from_regalloc(value_id, context, diagnostics)) {
    return resolved;
  }
  if (auto resolved = resolve_from_storage_plan(value_id, context, diagnostics)) {
    return resolved;
  }
  if (auto resolved = resolve_from_value_home(value_id, context, diagnostics)) {
    return resolved;
  }

  diagnostics.entries.push_back(ModuleLoweringDiagnostic{
      .kind = ModuleLoweringDiagnosticKind::MissingValueAuthority,
      .function_name = context.control_flow->function_name,
      .value_id = value_id,
      .message = "no typed prepared authority exists for value operand",
  });
  return std::nullopt;
}

ResolvedOperand resolve_immediate_operand(mir::Immediate immediate) {
  return ResolvedOperand{
      .operand = mir::Operand{.payload = immediate},
      .authority = OperandAuthority::Immediate,
  };
}

ResolvedOperand resolve_label_operand(c4c::BlockLabelId label) {
  return ResolvedOperand{
      .operand = mir::Operand{.payload = mir::Label{.label = label}},
      .authority = OperandAuthority::Label,
  };
}

ResolvedOperand resolve_symbol_operand(c4c::LinkNameId symbol, std::int64_t addend) {
  return ResolvedOperand{
      .operand = mir::Operand{.payload = mir::Symbol{.name = symbol, .addend = addend}},
      .authority = OperandAuthority::Symbol,
  };
}

}  // namespace c4c::backend::aarch64::module
