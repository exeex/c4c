#pragma once

#include "prepared_lookups.hpp"
#include "regalloc.hpp"
#include "storage.hpp"
#include "value_locations.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedDecodedHomeStorageSource {
  None,
  RegallocAssignment,
  StoragePlan,
  ValueHome,
};

[[nodiscard]] constexpr std::string_view prepared_decoded_home_storage_source_name(
    PreparedDecodedHomeStorageSource source) {
  switch (source) {
    case PreparedDecodedHomeStorageSource::None:
      return "none";
    case PreparedDecodedHomeStorageSource::RegallocAssignment:
      return "regalloc_assignment";
    case PreparedDecodedHomeStorageSource::StoragePlan:
      return "storage_plan";
    case PreparedDecodedHomeStorageSource::ValueHome:
      return "value_home";
  }
  return "unknown";
}

enum class PreparedDecodedHomeStorageKind {
  None,
  Register,
  FrameSlot,
  ImmediateI32,
  SymbolAddress,
  ComputedAddress,
  PointerBasePlusOffset,
};

[[nodiscard]] constexpr std::string_view prepared_decoded_home_storage_kind_name(
    PreparedDecodedHomeStorageKind kind) {
  switch (kind) {
    case PreparedDecodedHomeStorageKind::None:
      return "none";
    case PreparedDecodedHomeStorageKind::Register:
      return "register";
    case PreparedDecodedHomeStorageKind::FrameSlot:
      return "frame_slot";
    case PreparedDecodedHomeStorageKind::ImmediateI32:
      return "immediate_i32";
    case PreparedDecodedHomeStorageKind::SymbolAddress:
      return "symbol_address";
    case PreparedDecodedHomeStorageKind::ComputedAddress:
      return "computed_address";
    case PreparedDecodedHomeStorageKind::PointerBasePlusOffset:
      return "pointer_base_plus_offset";
  }
  return "unknown";
}

enum class PreparedDecodedHomeStorageStatus {
  Available,
  MissingAuthority,
  MissingRegisterPlacement,
  MissingFrameSlot,
  MissingImmediatePayload,
  MissingSymbolName,
  UnsupportedStorageEncoding,
  UnsupportedValueHomeKind,
};

[[nodiscard]] constexpr std::string_view prepared_decoded_home_storage_status_name(
    PreparedDecodedHomeStorageStatus status) {
  switch (status) {
    case PreparedDecodedHomeStorageStatus::Available:
      return "available";
    case PreparedDecodedHomeStorageStatus::MissingAuthority:
      return "missing_authority";
    case PreparedDecodedHomeStorageStatus::MissingRegisterPlacement:
      return "missing_register_placement";
    case PreparedDecodedHomeStorageStatus::MissingFrameSlot:
      return "missing_frame_slot";
    case PreparedDecodedHomeStorageStatus::MissingImmediatePayload:
      return "missing_immediate_payload";
    case PreparedDecodedHomeStorageStatus::MissingSymbolName:
      return "missing_symbol_name";
    case PreparedDecodedHomeStorageStatus::UnsupportedStorageEncoding:
      return "unsupported_storage_encoding";
    case PreparedDecodedHomeStorageStatus::UnsupportedValueHomeKind:
      return "unsupported_value_home_kind";
  }
  return "unknown";
}

struct PreparedDecodedHomeStorage {
  PreparedDecodedHomeStorageSource source = PreparedDecodedHomeStorageSource::None;
  PreparedDecodedHomeStorageKind kind = PreparedDecodedHomeStorageKind::None;
  PreparedDecodedHomeStorageStatus status = PreparedDecodedHomeStorageStatus::MissingAuthority;
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  PreparedStorageEncodingKind storage_encoding = PreparedStorageEncodingKind::None;
  PreparedValueHomeKind value_home_kind = PreparedValueHomeKind::None;
  std::optional<PreparedRegisterClass> register_class;
  PreparedRegisterBank register_bank = PreparedRegisterBank::None;
  std::size_t contiguous_width = 1;
  std::optional<std::string> register_name;
  std::vector<std::string> occupied_register_names;
  std::optional<PreparedRegisterPlacement> register_placement;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::optional<PreparedSpillSlotPlacement> spill_slot_placement;
  std::optional<std::int64_t> immediate_i32;
  std::optional<LinkNameId> symbol_name;
  std::optional<ValueNameId> pointer_base_value_name;
  std::optional<std::int64_t> pointer_byte_delta;
};

enum class PreparedDecodedHomeStorageDiagnosticCategory {
  MissingValueAuthority,
  MissingTypedRegisterAuthority,
  UnsupportedStoragePlanAuthority,
  UnsupportedValueHomeAuthority,
};

[[nodiscard]] constexpr std::string_view
prepared_decoded_home_storage_diagnostic_category_name(
    PreparedDecodedHomeStorageDiagnosticCategory category) {
  switch (category) {
    case PreparedDecodedHomeStorageDiagnosticCategory::MissingValueAuthority:
      return "missing_value_authority";
    case PreparedDecodedHomeStorageDiagnosticCategory::MissingTypedRegisterAuthority:
      return "missing_typed_register_authority";
    case PreparedDecodedHomeStorageDiagnosticCategory::UnsupportedStoragePlanAuthority:
      return "unsupported_storage_plan_authority";
    case PreparedDecodedHomeStorageDiagnosticCategory::UnsupportedValueHomeAuthority:
      return "unsupported_value_home_authority";
  }
  return "unknown";
}

struct PreparedDecodedHomeStorageDiagnostic {
  PreparedDecodedHomeStorageDiagnosticCategory category =
      PreparedDecodedHomeStorageDiagnosticCategory::MissingValueAuthority;
  PreparedDecodedHomeStorageSource source = PreparedDecodedHomeStorageSource::None;
  PreparedDecodedHomeStorageKind kind = PreparedDecodedHomeStorageKind::None;
  PreparedDecodedHomeStorageStatus status =
      PreparedDecodedHomeStorageStatus::MissingAuthority;
  FunctionNameId function_name = kInvalidFunctionName;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  std::string message;
};

struct PreparedHomeStorageDecodeInputs {
  const PreparedRegallocFunction* regalloc = nullptr;
  const PreparedStoragePlanFunction* storage_plan = nullptr;
  const PreparedValueLocationFunction* value_locations = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
};

[[nodiscard]] bool prepared_decoded_home_storage_available(
    const PreparedDecodedHomeStorage& decoded);

[[nodiscard]] PreparedDecodedHomeStorage decode_prepared_regalloc_assignment(
    const PreparedRegallocFunction* regalloc,
    PreparedValueId value_id);

[[nodiscard]] PreparedDecodedHomeStorage decode_prepared_storage_plan_value(
    const PreparedStoragePlanFunction* storage_plan,
    PreparedValueId value_id);

[[nodiscard]] PreparedDecodedHomeStorage decode_prepared_value_home(
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups,
    PreparedValueId value_id);

[[nodiscard]] PreparedDecodedHomeStorage decode_prepared_home_storage(
    const PreparedHomeStorageDecodeInputs& inputs,
    PreparedValueId value_id);

[[nodiscard]] PreparedDecodedHomeStorageDiagnostic
build_prepared_decoded_home_storage_diagnostic(
    const PreparedDecodedHomeStorage& decoded);

}  // namespace c4c::backend::prepare
