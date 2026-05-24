#pragma once

#include "calls.hpp"
#include "names.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace c4c::backend::prepare {

enum class PreparedScalarPublicationStatus {
  Available,
  MissingDestinationHome,
  MissingSourceValue,
  UnsupportedHomeKind,
  MissingStackSlot,
  MissingImmediatePayload,
  MissingPointerBase,
  MissingPointerDelta,
};

[[nodiscard]] constexpr std::string_view prepared_scalar_publication_status_name(
    PreparedScalarPublicationStatus status) {
  switch (status) {
    case PreparedScalarPublicationStatus::Available:
      return "available";
    case PreparedScalarPublicationStatus::MissingDestinationHome:
      return "missing_destination_home";
    case PreparedScalarPublicationStatus::MissingSourceValue:
      return "missing_source_value";
    case PreparedScalarPublicationStatus::UnsupportedHomeKind:
      return "unsupported_home_kind";
    case PreparedScalarPublicationStatus::MissingStackSlot:
      return "missing_stack_slot";
    case PreparedScalarPublicationStatus::MissingImmediatePayload:
      return "missing_immediate_payload";
    case PreparedScalarPublicationStatus::MissingPointerBase:
      return "missing_pointer_base";
    case PreparedScalarPublicationStatus::MissingPointerDelta:
      return "missing_pointer_delta";
  }
  return "unknown";
}

enum class PreparedScalarPublicationHookKind {
  None,
  RegisterHome,
  StackSlotHome,
  RematerializableImmediate,
  PointerBasePlusOffset,
};

[[nodiscard]] constexpr std::string_view prepared_scalar_publication_hook_kind_name(
    PreparedScalarPublicationHookKind kind) {
  switch (kind) {
    case PreparedScalarPublicationHookKind::None:
      return "none";
    case PreparedScalarPublicationHookKind::RegisterHome:
      return "register_home";
    case PreparedScalarPublicationHookKind::StackSlotHome:
      return "stack_slot_home";
    case PreparedScalarPublicationHookKind::RematerializableImmediate:
      return "rematerializable_immediate";
    case PreparedScalarPublicationHookKind::PointerBasePlusOffset:
      return "pointer_base_plus_offset";
  }
  return "unknown";
}

struct PreparedScalarPublicationPlan {
  PreparedScalarPublicationStatus status =
      PreparedScalarPublicationStatus::MissingDestinationHome;
  PreparedScalarPublicationHookKind hook_kind =
      PreparedScalarPublicationHookKind::None;
  bir::Value source_value;
  PreparedValueId source_value_id = 0;
  ValueNameId source_value_name = kInvalidValueName;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueHomeKind destination_home_kind = PreparedValueHomeKind::None;
  PreparedStorageEncodingKind storage_encoding = PreparedStorageEncodingKind::None;
  const PreparedBlockEntryPublication* current_block_entry_publication = nullptr;
  bool current_block_entry_publication_available = false;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::size_t> size_bytes;
  std::optional<std::size_t> align_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<bir::Value::F128Payload> immediate_f128;
  std::optional<ValueNameId> pointer_base_value_name;
  std::optional<std::int64_t> pointer_byte_delta;
};

struct PreparedScalarPublicationInputs {
  const bir::Value* source_value = nullptr;
  const PreparedValueHome* destination_home = nullptr;
  const PreparedBlockEntryPublication* current_block_entry_publication = nullptr;
};

[[nodiscard]] bool prepared_scalar_publication_available(
    const PreparedScalarPublicationPlan& plan);

[[nodiscard]] PreparedStorageEncodingKind prepared_publication_storage_encoding_from_home(
    PreparedValueHomeKind home_kind);

[[nodiscard]] PreparedScalarPublicationPlan plan_prepared_scalar_publication(
    const PreparedScalarPublicationInputs& inputs);

}  // namespace c4c::backend::prepare
