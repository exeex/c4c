#include "publication_plans.hpp"

namespace c4c::backend::prepare {
namespace {

[[nodiscard]] PreparedScalarPublicationPlan missing_destination_home(
    const bir::Value* source_value,
    const PreparedBlockEntryPublication* publication) {
  PreparedScalarPublicationPlan plan{
      .status = PreparedScalarPublicationStatus::MissingDestinationHome,
      .current_block_entry_publication = publication,
      .current_block_entry_publication_available =
          publication != nullptr && prepared_block_entry_publication_available(*publication),
  };
  if (source_value != nullptr) {
    plan.source_value = *source_value;
  }
  return plan;
}

[[nodiscard]] PreparedScalarPublicationHookKind hook_kind_from_home(
    PreparedValueHomeKind home_kind) {
  switch (home_kind) {
    case PreparedValueHomeKind::Register:
      return PreparedScalarPublicationHookKind::RegisterHome;
    case PreparedValueHomeKind::StackSlot:
      return PreparedScalarPublicationHookKind::StackSlotHome;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedScalarPublicationHookKind::RematerializableImmediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedScalarPublicationHookKind::PointerBasePlusOffset;
    case PreparedValueHomeKind::None:
      return PreparedScalarPublicationHookKind::None;
  }
  return PreparedScalarPublicationHookKind::None;
}

}  // namespace

bool prepared_scalar_publication_available(
    const PreparedScalarPublicationPlan& plan) {
  return plan.status == PreparedScalarPublicationStatus::Available;
}

PreparedStorageEncodingKind prepared_publication_storage_encoding_from_home(
    PreparedValueHomeKind home_kind) {
  switch (home_kind) {
    case PreparedValueHomeKind::Register:
      return PreparedStorageEncodingKind::Register;
    case PreparedValueHomeKind::StackSlot:
      return PreparedStorageEncodingKind::FrameSlot;
    case PreparedValueHomeKind::RematerializableImmediate:
      return PreparedStorageEncodingKind::Immediate;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      return PreparedStorageEncodingKind::ComputedAddress;
    case PreparedValueHomeKind::None:
      return PreparedStorageEncodingKind::None;
  }
  return PreparedStorageEncodingKind::None;
}

PreparedScalarPublicationPlan plan_prepared_scalar_publication(
    const PreparedScalarPublicationInputs& inputs) {
  if (inputs.destination_home == nullptr) {
    return missing_destination_home(inputs.source_value,
                                    inputs.current_block_entry_publication);
  }

  const PreparedValueHome& home = *inputs.destination_home;
  PreparedScalarPublicationPlan plan{
      .status = PreparedScalarPublicationStatus::Available,
      .hook_kind = hook_kind_from_home(home.kind),
      .source_value_id = home.value_id,
      .source_value_name = home.value_name,
      .destination_home = &home,
      .destination_home_kind = home.kind,
      .storage_encoding = prepared_publication_storage_encoding_from_home(home.kind),
      .current_block_entry_publication = inputs.current_block_entry_publication,
      .current_block_entry_publication_available =
          inputs.current_block_entry_publication != nullptr &&
          prepared_block_entry_publication_available(*inputs.current_block_entry_publication),
      .slot_id = home.slot_id,
      .stack_offset_bytes = home.offset_bytes,
      .size_bytes = home.size_bytes,
      .align_bytes = home.align_bytes,
      .immediate_i32 = home.immediate_i32,
      .immediate_f128 = home.immediate_f128,
      .pointer_base_value_name = home.pointer_base_value_name,
      .pointer_byte_delta = home.pointer_byte_delta,
  };

  if (inputs.source_value == nullptr) {
    plan.status = PreparedScalarPublicationStatus::MissingSourceValue;
    return plan;
  }
  plan.source_value = *inputs.source_value;

  switch (home.kind) {
    case PreparedValueHomeKind::Register:
      return plan;
    case PreparedValueHomeKind::StackSlot:
      if (!home.slot_id.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingStackSlot;
      }
      return plan;
    case PreparedValueHomeKind::RematerializableImmediate:
      if (!home.immediate_i32.has_value() && !home.immediate_f128.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingImmediatePayload;
      }
      return plan;
    case PreparedValueHomeKind::PointerBasePlusOffset:
      if (!home.pointer_base_value_name.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingPointerBase;
      } else if (!home.pointer_byte_delta.has_value()) {
        plan.status = PreparedScalarPublicationStatus::MissingPointerDelta;
      }
      return plan;
    case PreparedValueHomeKind::None:
      plan.status = PreparedScalarPublicationStatus::UnsupportedHomeKind;
      return plan;
  }
  plan.status = PreparedScalarPublicationStatus::UnsupportedHomeKind;
  return plan;
}

}  // namespace c4c::backend::prepare
