#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "frame.hpp"
#include "names.hpp"
#include "prepared_lookups.hpp"
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

[[nodiscard]] PreparedStorageEncodingKind prepared_storage_encoding_from_value_home_kind(
    PreparedValueHomeKind home_kind);

[[nodiscard]] PreparedStorageEncodingKind prepared_publication_storage_encoding_from_home(
    PreparedValueHomeKind home_kind);

[[nodiscard]] PreparedScalarPublicationPlan plan_prepared_scalar_publication(
    const PreparedScalarPublicationInputs& inputs);

enum class PreparedStoreSourcePublicationStatus {
  Available,
  MissingSourceValue,
  MissingDestinationAccess,
};

[[nodiscard]] constexpr std::string_view prepared_store_source_publication_status_name(
    PreparedStoreSourcePublicationStatus status) {
  switch (status) {
    case PreparedStoreSourcePublicationStatus::Available:
      return "available";
    case PreparedStoreSourcePublicationStatus::MissingSourceValue:
      return "missing_source_value";
    case PreparedStoreSourcePublicationStatus::MissingDestinationAccess:
      return "missing_destination_access";
  }
  return "unknown";
}

enum class PreparedStoreSourcePublicationIntent {
  None,
  StoreLocalPublication,
  StoreGlobalPublication,
  PointerStoreWriteback,
};

[[nodiscard]] constexpr std::string_view prepared_store_source_publication_intent_name(
    PreparedStoreSourcePublicationIntent intent) {
  switch (intent) {
    case PreparedStoreSourcePublicationIntent::None:
      return "none";
    case PreparedStoreSourcePublicationIntent::StoreLocalPublication:
      return "store_local_publication";
    case PreparedStoreSourcePublicationIntent::StoreGlobalPublication:
      return "store_global_publication";
    case PreparedStoreSourcePublicationIntent::PointerStoreWriteback:
      return "pointer_store_writeback";
  }
  return "unknown";
}

struct PreparedStoreSourcePublicationPlan {
  PreparedStoreSourcePublicationStatus status =
      PreparedStoreSourcePublicationStatus::MissingSourceValue;
  PreparedStoreSourcePublicationIntent intent =
      PreparedStoreSourcePublicationIntent::None;

  bir::Value source_value;
  PreparedValueId source_value_id = 0;
  ValueNameId source_value_name = kInvalidValueName;

  const PreparedMemoryAccess* destination_access = nullptr;
  PreparedAddressBaseKind destination_base_kind = PreparedAddressBaseKind::None;
  std::optional<PreparedFrameSlotId> destination_frame_slot_id;
  std::optional<ValueNameId> destination_pointer_value_name;
  std::int64_t destination_byte_offset = 0;
  std::size_t destination_size_bytes = 0;
  std::size_t destination_align_bytes = 0;
  bool destination_can_use_base_plus_offset = false;
  bool destination_is_volatile = false;

  const PreparedFrameSlot* destination_frame_slot = nullptr;
  std::optional<PreparedObjectId> destination_object_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::optional<std::size_t> destination_stack_size_bytes;
  std::optional<std::size_t> destination_stack_align_bytes;
  const PreparedStackObject* destination_stack_object = nullptr;

  const PreparedValueHome* source_home = nullptr;
  PreparedValueHomeKind source_home_kind = PreparedValueHomeKind::None;
  PreparedStorageEncodingKind source_storage_encoding = PreparedStorageEncodingKind::None;
  std::optional<PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<std::size_t> source_size_bytes;
  std::optional<std::size_t> source_align_bytes;
  std::optional<ValueNameId> source_pointer_base_value_name;
  std::optional<std::int64_t> source_pointer_byte_delta;

  PreparedEdgePublicationSourceProducerKind source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  std::optional<BlockLabelId> source_producer_block_label;
  std::optional<std::size_t> source_producer_instruction_index;
  const bir::LoadLocalInst* source_load_local = nullptr;
  const bir::CastInst* source_cast = nullptr;
  const bir::BinaryInst* source_binary = nullptr;
  const bir::SelectInst* source_select = nullptr;

  std::optional<bir::Value> recovered_source_value;
  std::optional<std::size_t> recovered_source_instruction_index;
  bool byval_load_local_source = false;

  bool pending_publication = false;
  bool stack_homes_only = false;
  bool pointer_store_writeback = false;
  bool duplicate_publication = false;

  const PreparedValueHome* pointer_base_home = nullptr;
  PreparedValueHomeKind pointer_base_home_kind = PreparedValueHomeKind::None;
  std::optional<PreparedFrameSlotId> pointer_base_slot_id;
  std::optional<std::size_t> pointer_base_stack_offset_bytes;
};

struct PreparedRecoveredStoreSourcePublication {
  bir::Value stored_value;
  std::size_t instruction_index = 0;
};

struct PreparedStoreSourcePublicationInputs {
  const bir::Value* source_value = nullptr;
  const PreparedMemoryAccess* destination_access = nullptr;
  const PreparedValueHome* source_home = nullptr;
  const PreparedFrameSlot* destination_frame_slot = nullptr;
  const PreparedStackObject* destination_stack_object = nullptr;
  const bir::Value* recovered_source_value = nullptr;
  std::optional<std::size_t> recovered_source_instruction_index;
  bool byval_load_local_source = false;
  const PreparedValueHome* pointer_base_home = nullptr;
  PreparedStoreSourcePublicationIntent intent =
      PreparedStoreSourcePublicationIntent::None;
  bool pending_publication = false;
  bool stack_homes_only = false;
  bool pointer_store_writeback = false;
  bool duplicate_publication = false;
  const PreparedEdgePublicationSourceProducer* source_producer = nullptr;
};

[[nodiscard]] bool prepared_store_source_publication_available(
    const PreparedStoreSourcePublicationPlan& plan);

[[nodiscard]] PreparedStoreSourcePublicationPlan
plan_prepared_store_source_publication(
    const PreparedStoreSourcePublicationInputs& inputs);

[[nodiscard]] std::optional<PreparedRecoveredStoreSourcePublication>
find_prepared_recovered_narrow_store_source_for_wide_local_load(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index);

[[nodiscard]] bool
prepared_store_source_load_local_is_byval_formal_pointer_source(
    const PreparedNameTables& names,
    const bir::Function* bir_function,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducer* source_producer);

}  // namespace c4c::backend::prepare
