#include "decoded_home_storage.hpp"

#include <algorithm>

namespace c4c::backend::prepare {
namespace {

[[nodiscard]] PreparedDecodedHomeStorage missing_authority(PreparedValueId value_id) {
  return PreparedDecodedHomeStorage{
      .source = PreparedDecodedHomeStorageSource::None,
      .kind = PreparedDecodedHomeStorageKind::None,
      .status = PreparedDecodedHomeStorageStatus::MissingAuthority,
      .value_id = value_id,
  };
}

[[nodiscard]] const PreparedRegallocValue* find_regalloc_value(
    const PreparedRegallocFunction* regalloc,
    PreparedValueId value_id) {
  if (regalloc == nullptr) {
    return nullptr;
  }
  for (const auto& value : regalloc->values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] const PreparedStoragePlanValue* find_storage_plan_value(
    const PreparedStoragePlanFunction* storage_plan,
    PreparedValueId value_id) {
  if (storage_plan == nullptr) {
    return nullptr;
  }
  for (const auto& value : storage_plan->values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] PreparedDecodedHomeStorageStatus status_for_register(
    const std::optional<PreparedRegisterPlacement>& placement) {
  return placement.has_value()
             ? PreparedDecodedHomeStorageStatus::Available
             : PreparedDecodedHomeStorageStatus::MissingRegisterPlacement;
}

[[nodiscard]] PreparedDecodedHomeStorageStatus status_for_frame_slot(
    const std::optional<PreparedFrameSlotId>& slot_id) {
  return slot_id.has_value() ? PreparedDecodedHomeStorageStatus::Available
                             : PreparedDecodedHomeStorageStatus::MissingFrameSlot;
}

[[nodiscard]] PreparedDecodedHomeStorageStatus status_for_immediate(
    const std::optional<std::int64_t>& immediate_i32) {
  return immediate_i32.has_value()
             ? PreparedDecodedHomeStorageStatus::Available
             : PreparedDecodedHomeStorageStatus::MissingImmediatePayload;
}

[[nodiscard]] PreparedDecodedHomeStorageStatus status_for_symbol(
    const std::optional<LinkNameId>& symbol_name) {
  return symbol_name.has_value() ? PreparedDecodedHomeStorageStatus::Available
                                 : PreparedDecodedHomeStorageStatus::MissingSymbolName;
}

}  // namespace

bool prepared_decoded_home_storage_available(
    const PreparedDecodedHomeStorage& decoded) {
  return decoded.status == PreparedDecodedHomeStorageStatus::Available;
}

PreparedDecodedHomeStorage decode_prepared_regalloc_assignment(
    const PreparedRegallocFunction* regalloc,
    PreparedValueId value_id) {
  const auto* value = find_regalloc_value(regalloc, value_id);
  if (value == nullptr) {
    return missing_authority(value_id);
  }

  if (value->assigned_register.has_value()) {
    const auto& assigned = *value->assigned_register;
    return PreparedDecodedHomeStorage{
        .source = PreparedDecodedHomeStorageSource::RegallocAssignment,
        .kind = PreparedDecodedHomeStorageKind::Register,
        .status = status_for_register(assigned.placement),
        .function_name = value->function_name,
        .value_id = value->value_id,
        .value_name = value->value_name,
        .register_class = assigned.reg_class,
        .register_bank = assigned.placement.has_value()
                             ? assigned.placement->bank
                             : PreparedRegisterBank::None,
        .contiguous_width = std::max<std::size_t>(assigned.contiguous_width, 1),
        .register_name = assigned.register_name.empty()
                             ? std::nullopt
                             : std::optional<std::string>{assigned.register_name},
        .occupied_register_names = assigned.occupied_register_names,
        .register_placement = assigned.placement,
    };
  }

  if (value->assigned_stack_slot.has_value()) {
    const auto& assigned = *value->assigned_stack_slot;
    return PreparedDecodedHomeStorage{
        .source = PreparedDecodedHomeStorageSource::RegallocAssignment,
        .kind = PreparedDecodedHomeStorageKind::FrameSlot,
        .status = PreparedDecodedHomeStorageStatus::Available,
        .function_name = value->function_name,
        .value_id = value->value_id,
        .value_name = value->value_name,
        .slot_id = assigned.slot_id,
        .stack_offset_bytes = assigned.offset_bytes,
        .size_bytes = assigned.size_bytes,
        .align_bytes = assigned.align_bytes,
        .spill_slot_placement = assigned.placement,
    };
  }

  return PreparedDecodedHomeStorage{
      .source = PreparedDecodedHomeStorageSource::RegallocAssignment,
      .kind = PreparedDecodedHomeStorageKind::None,
      .status = PreparedDecodedHomeStorageStatus::MissingAuthority,
      .function_name = value->function_name,
      .value_id = value->value_id,
      .value_name = value->value_name,
  };
}

PreparedDecodedHomeStorage decode_prepared_storage_plan_value(
    const PreparedStoragePlanFunction* storage_plan,
    PreparedValueId value_id) {
  const auto* value = find_storage_plan_value(storage_plan, value_id);
  if (value == nullptr) {
    return missing_authority(value_id);
  }

  PreparedDecodedHomeStorage decoded{
      .source = PreparedDecodedHomeStorageSource::StoragePlan,
      .status = PreparedDecodedHomeStorageStatus::UnsupportedStorageEncoding,
      .function_name = storage_plan == nullptr ? kInvalidFunctionName
                                               : storage_plan->function_name,
      .value_id = value->value_id,
      .value_name = value->value_name,
      .storage_encoding = value->encoding,
      .register_bank = value->bank,
      .contiguous_width = std::max<std::size_t>(value->contiguous_width, 1),
      .register_name = value->register_name,
      .occupied_register_names = value->occupied_register_names,
      .register_placement = value->register_placement,
      .slot_id = value->slot_id,
      .stack_offset_bytes = value->stack_offset_bytes,
      .spill_slot_placement = value->spill_slot_placement,
      .immediate_i32 = value->immediate_i32,
      .symbol_name = value->symbol_name,
  };

  switch (value->encoding) {
    case PreparedStorageEncodingKind::Register:
      decoded.kind = PreparedDecodedHomeStorageKind::Register;
      decoded.status = status_for_register(value->register_placement);
      return decoded;
    case PreparedStorageEncodingKind::FrameSlot:
      decoded.kind = PreparedDecodedHomeStorageKind::FrameSlot;
      decoded.status = status_for_frame_slot(value->slot_id);
      return decoded;
    case PreparedStorageEncodingKind::Immediate:
      decoded.kind = PreparedDecodedHomeStorageKind::ImmediateI32;
      decoded.status = status_for_immediate(value->immediate_i32);
      return decoded;
    case PreparedStorageEncodingKind::SymbolAddress:
      decoded.kind = PreparedDecodedHomeStorageKind::SymbolAddress;
      decoded.status = status_for_symbol(value->symbol_name);
      return decoded;
    case PreparedStorageEncodingKind::ComputedAddress:
      decoded.kind = PreparedDecodedHomeStorageKind::ComputedAddress;
      decoded.status = PreparedDecodedHomeStorageStatus::UnsupportedStorageEncoding;
      return decoded;
    case PreparedStorageEncodingKind::None:
      decoded.kind = PreparedDecodedHomeStorageKind::None;
      decoded.status = PreparedDecodedHomeStorageStatus::MissingAuthority;
      return decoded;
  }
  return decoded;
}

PreparedDecodedHomeStorage decode_prepared_value_home(
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups,
    PreparedValueId value_id) {
  const auto* home =
      find_indexed_prepared_value_home(value_home_lookups, value_locations, value_id);
  if (home == nullptr) {
    return missing_authority(value_id);
  }

  PreparedDecodedHomeStorage decoded{
      .source = PreparedDecodedHomeStorageSource::ValueHome,
      .status = PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind,
      .function_name = home->function_name,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .value_home_kind = home->kind,
      .register_name = home->register_name,
      .slot_id = home->slot_id,
      .stack_offset_bytes = home->offset_bytes,
      .size_bytes = home->size_bytes,
      .align_bytes = home->align_bytes,
      .immediate_i32 = home->immediate_i32,
      .pointer_base_value_name = home->pointer_base_value_name,
      .pointer_byte_delta = home->pointer_byte_delta,
  };

  switch (home->kind) {
    case PreparedValueHomeKind::StackSlot:
      decoded.kind = PreparedDecodedHomeStorageKind::FrameSlot;
      decoded.status = status_for_frame_slot(home->slot_id);
      return decoded;
    case PreparedValueHomeKind::RematerializableImmediate:
      decoded.kind = PreparedDecodedHomeStorageKind::ImmediateI32;
      decoded.status = status_for_immediate(home->immediate_i32);
      return decoded;
    case PreparedValueHomeKind::Register:
      decoded.kind = PreparedDecodedHomeStorageKind::Register;
      decoded.status = PreparedDecodedHomeStorageStatus::MissingRegisterPlacement;
      return decoded;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      decoded.kind = PreparedDecodedHomeStorageKind::PointerBasePlusOffset;
      decoded.status = PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind;
      return decoded;
    case PreparedValueHomeKind::None:
      decoded.kind = PreparedDecodedHomeStorageKind::None;
      decoded.status = PreparedDecodedHomeStorageStatus::MissingAuthority;
      return decoded;
  }
  return decoded;
}

PreparedDecodedHomeStorage decode_prepared_home_storage(
    const PreparedHomeStorageDecodeInputs& inputs,
    PreparedValueId value_id) {
  auto decoded = decode_prepared_regalloc_assignment(inputs.regalloc, value_id);
  if (decoded.source != PreparedDecodedHomeStorageSource::None) {
    return decoded;
  }

  decoded = decode_prepared_storage_plan_value(inputs.storage_plan, value_id);
  if (decoded.source != PreparedDecodedHomeStorageSource::None) {
    return decoded;
  }

  decoded = decode_prepared_value_home(inputs.value_locations,
                                       inputs.value_home_lookups,
                                       value_id);
  if (decoded.source != PreparedDecodedHomeStorageSource::None) {
    return decoded;
  }

  return missing_authority(value_id);
}

PreparedDecodedHomeStorageDiagnostic build_prepared_decoded_home_storage_diagnostic(
    const PreparedDecodedHomeStorage& decoded) {
  PreparedDecodedHomeStorageDiagnostic diagnostic{
      .source = decoded.source,
      .kind = decoded.kind,
      .status = decoded.status,
      .function_name = decoded.function_name,
      .value_id = decoded.value_id,
      .value_name = decoded.value_name,
  };

  if (decoded.status == PreparedDecodedHomeStorageStatus::MissingRegisterPlacement) {
    diagnostic.category =
        PreparedDecodedHomeStorageDiagnosticCategory::MissingTypedRegisterAuthority;
    if (decoded.source == PreparedDecodedHomeStorageSource::StoragePlan) {
      diagnostic.message = "storage-plan register value is missing typed register placement";
    } else if (decoded.source == PreparedDecodedHomeStorageSource::ValueHome) {
      diagnostic.message =
          "value-home register spelling is diagnostic-only until typed placement exists";
    } else {
      diagnostic.message = "regalloc register assignment is missing typed register placement";
    }
    return diagnostic;
  }

  if (decoded.source == PreparedDecodedHomeStorageSource::StoragePlan) {
    diagnostic.category =
        PreparedDecodedHomeStorageDiagnosticCategory::UnsupportedStoragePlanAuthority;
    diagnostic.message = "storage-plan value does not have a supported typed operand form";
    return diagnostic;
  }

  if (decoded.source == PreparedDecodedHomeStorageSource::ValueHome) {
    diagnostic.category =
        PreparedDecodedHomeStorageDiagnosticCategory::UnsupportedValueHomeAuthority;
    diagnostic.message = "prepared value home does not have a supported typed operand form";
    return diagnostic;
  }

  diagnostic.category =
      PreparedDecodedHomeStorageDiagnosticCategory::MissingValueAuthority;
  diagnostic.message = "no typed prepared authority exists for value operand";
  return diagnostic;
}

}  // namespace c4c::backend::prepare
