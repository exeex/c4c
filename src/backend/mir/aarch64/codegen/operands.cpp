#include "operands.hpp"

#include <cstdint>
#include <string>

namespace c4c::backend::aarch64::codegen {
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
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto converted =
      abi::convert_prepared_register(placement, prepared_class, std::nullopt);
  if (!converted.reg.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::RegisterConversionFailed,
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

[[nodiscard]] OperandAuthority register_authority(
    prepare::PreparedDecodedHomeStorageSource source) {
  switch (source) {
    case prepare::PreparedDecodedHomeStorageSource::RegallocAssignment:
      return OperandAuthority::RegallocAssignment;
    case prepare::PreparedDecodedHomeStorageSource::StoragePlan:
      return OperandAuthority::StoragePlan;
    case prepare::PreparedDecodedHomeStorageSource::ValueHome:
    case prepare::PreparedDecodedHomeStorageSource::None:
      return OperandAuthority::PreparedValueHome;
  }
  return OperandAuthority::None;
}

[[nodiscard]] OperandAuthority frame_slot_authority(
    const prepare::PreparedDecodedHomeStorage& decoded) {
  switch (decoded.source) {
    case prepare::PreparedDecodedHomeStorageSource::StoragePlan:
      return decoded.spill_slot_placement.has_value() ? OperandAuthority::SpillSlot
                                                      : OperandAuthority::FrameSlot;
    case prepare::PreparedDecodedHomeStorageSource::ValueHome:
      return OperandAuthority::PreparedValueHome;
    case prepare::PreparedDecodedHomeStorageSource::RegallocAssignment:
    case prepare::PreparedDecodedHomeStorageSource::None:
      return OperandAuthority::FrameSlot;
  }
  return OperandAuthority::None;
}

[[nodiscard]] OperandAuthority immediate_authority(
    prepare::PreparedDecodedHomeStorageSource source) {
  return source == prepare::PreparedDecodedHomeStorageSource::StoragePlan
             ? OperandAuthority::Immediate
             : OperandAuthority::PreparedValueHome;
}

[[nodiscard]] std::optional<ResolvedOperand> make_decoded_operand(
    const prepare::PreparedDecodedHomeStorage& decoded,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (!prepare::prepared_decoded_home_storage_available(decoded)) {
    return std::nullopt;
  }

  switch (decoded.kind) {
    case prepare::PreparedDecodedHomeStorageKind::Register:
      if (!decoded.register_placement.has_value()) {
        return std::nullopt;
      }
      return make_register_operand(*decoded.register_placement,
                                   decoded.register_class,
                                   register_authority(decoded.source),
                                   decoded.value_id,
                                   decoded.value_name,
                                   decoded.function_name,
                                   diagnostics);
    case prepare::PreparedDecodedHomeStorageKind::FrameSlot:
      if (!decoded.slot_id.has_value()) {
        return std::nullopt;
      }
      return ResolvedOperand{
          .operand = mir::Operand{.payload = mir::Memory{
                                      .base = std::nullopt,
                                      .index = std::nullopt,
                                      .displacement = decoded.stack_offset_bytes.has_value()
                                                          ? static_cast<std::int64_t>(
                                                                *decoded.stack_offset_bytes)
                                                          : 0,
                                      .scale = 1,
                                  }},
          .authority = frame_slot_authority(decoded),
          .frame_slot_id = decoded.slot_id,
          .value_id = decoded.value_id,
          .value_name = decoded.value_name,
      };
    case prepare::PreparedDecodedHomeStorageKind::ImmediateI32:
      if (!decoded.immediate_i32.has_value()) {
        return std::nullopt;
      }
      return ResolvedOperand{
          .operand = mir::Operand{
              .payload = mir::Immediate{
                  .kind = mir::ImmediateKind::Signed,
                  .signed_value = *decoded.immediate_i32,
                  .unsigned_value = static_cast<std::uint64_t>(*decoded.immediate_i32),
              }},
          .authority = immediate_authority(decoded.source),
          .value_id = decoded.value_id,
          .value_name = decoded.value_name,
      };
    case prepare::PreparedDecodedHomeStorageKind::SymbolAddress:
      if (!decoded.symbol_name.has_value()) {
        return std::nullopt;
      }
      return ResolvedOperand{
          .operand = mir::Operand{.payload = mir::Symbol{.name = *decoded.symbol_name}},
          .authority = OperandAuthority::Symbol,
          .value_id = decoded.value_id,
          .value_name = decoded.value_name,
      };
    case prepare::PreparedDecodedHomeStorageKind::ComputedAddress:
    case prepare::PreparedDecodedHomeStorageKind::PointerBasePlusOffset:
    case prepare::PreparedDecodedHomeStorageKind::None:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] module::ModuleLoweringDiagnosticKind diagnostic_kind_for_decoded_failure(
    const prepare::PreparedDecodedHomeStorage& decoded) {
  if (decoded.status == prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement) {
    return module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority;
  }
  if (decoded.source == prepare::PreparedDecodedHomeStorageSource::StoragePlan) {
    return module::ModuleLoweringDiagnosticKind::UnsupportedStoragePlan;
  }
  if (decoded.source == prepare::PreparedDecodedHomeStorageSource::ValueHome) {
    return module::ModuleLoweringDiagnosticKind::UnsupportedValueHome;
  }
  return module::ModuleLoweringDiagnosticKind::MissingValueAuthority;
}

[[nodiscard]] std::string diagnostic_message_for_decoded_failure(
    const prepare::PreparedDecodedHomeStorage& decoded) {
  if (decoded.status == prepare::PreparedDecodedHomeStorageStatus::MissingRegisterPlacement) {
    if (decoded.source == prepare::PreparedDecodedHomeStorageSource::StoragePlan) {
      return "storage-plan register value is missing typed register placement";
    }
    if (decoded.source == prepare::PreparedDecodedHomeStorageSource::ValueHome) {
      return "value-home register spelling is diagnostic-only until typed placement exists";
    }
    return "regalloc register assignment is missing typed register placement";
  }
  if (decoded.source == prepare::PreparedDecodedHomeStorageSource::StoragePlan) {
    return "storage-plan value does not have a supported typed operand form";
  }
  if (decoded.source == prepare::PreparedDecodedHomeStorageSource::ValueHome) {
    return "prepared value home does not have a supported typed operand form";
  }
  return "no typed prepared authority exists for value operand";
}

void diagnose_decoded_failure(const prepare::PreparedDecodedHomeStorage& decoded,
                              module::ModuleLoweringDiagnostics& diagnostics) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = diagnostic_kind_for_decoded_failure(decoded),
      .function_name = decoded.function_name,
      .value_id = decoded.value_id,
      .value_name = decoded.value_name,
      .message = diagnostic_message_for_decoded_failure(decoded),
  });
}

}  // namespace

std::optional<ResolvedOperand> resolve_value_operand(
    prepare::PreparedValueId value_id,
    const module::FunctionLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.control_flow == nullptr) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingFunctionContext,
        .value_id = value_id,
        .message = "operand resolution requires a prepared function context",
    });
    return std::nullopt;
  }

  const auto decoded = prepare::decode_prepared_home_storage(
      prepare::PreparedHomeStorageDecodeInputs{
          .regalloc = context.regalloc,
          .storage_plan = context.storage_plan,
          .value_locations = context.value_locations,
          .value_home_lookups = context.value_home_lookups,
      },
      value_id);
  if (auto resolved = make_decoded_operand(decoded, diagnostics)) {
    return resolved;
  }
  if (decoded.source != prepare::PreparedDecodedHomeStorageSource::None) {
    diagnose_decoded_failure(decoded, diagnostics);
    return std::nullopt;
  }

  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
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

}  // namespace c4c::backend::aarch64::codegen
