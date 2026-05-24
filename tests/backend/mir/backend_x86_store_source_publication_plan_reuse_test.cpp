#include "src/backend/prealloc/publication_plans.hpp"

#include <iostream>
#include <optional>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

enum class X86StoreSourceAction {
  Reject,
  StoreStackSourceToFrameDestination,
  StoreRecoveredRegisterSource,
  WritebackPointerThroughFrameHome,
};

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedMemoryAccess frame_slot_store_access(
    c4c::ValueNameId stored_value_name,
    prepare::PreparedFrameSlotId slot_id,
    std::int64_t byte_offset) {
  return prepare::PreparedMemoryAccess{
      .function_name = 17,
      .block_label = 23,
      .inst_index = 5,
      .stored_value_name = stored_value_name,
      .address =
          prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = slot_id,
              .byte_offset = byte_offset,
              .size_bytes = 8,
              .align_bytes = 8,
              .can_use_base_plus_offset = true,
          },
  };
}

prepare::PreparedMemoryAccess pointer_value_store_access(
    c4c::ValueNameId stored_value_name,
    c4c::ValueNameId pointer_value_name,
    std::int64_t byte_offset) {
  return prepare::PreparedMemoryAccess{
      .function_name = 17,
      .block_label = 23,
      .inst_index = 9,
      .stored_value_name = stored_value_name,
      .address =
          prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
              .pointer_value_name = pointer_value_name,
              .byte_offset = byte_offset,
              .size_bytes = 8,
              .align_bytes = 8,
              .can_use_base_plus_offset = true,
          },
  };
}

prepare::PreparedValueHome value_home(
    prepare::PreparedValueHomeKind kind,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = 17,
      .value_name = value_name,
      .kind = kind,
  };
}

X86StoreSourceAction choose_x86_store_source_action(
    const prepare::PreparedStoreSourcePublicationPlan& plan) {
  if (!prepare::prepared_store_source_publication_available(plan)) {
    return X86StoreSourceAction::Reject;
  }

  if (plan.intent ==
          prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback &&
      plan.pointer_store_writeback &&
      plan.destination_base_kind == prepare::PreparedAddressBaseKind::PointerValue &&
      plan.destination_pointer_value_name.has_value() &&
      plan.source_home_kind == prepare::PreparedValueHomeKind::Register &&
      plan.pointer_base_home_kind == prepare::PreparedValueHomeKind::StackSlot &&
      plan.pointer_base_stack_offset_bytes.has_value()) {
    return X86StoreSourceAction::WritebackPointerThroughFrameHome;
  }

  if (plan.intent ==
          prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication &&
      plan.destination_base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
      plan.destination_stack_offset_bytes.has_value() &&
      plan.destination_can_use_base_plus_offset &&
      plan.source_home_kind == prepare::PreparedValueHomeKind::StackSlot &&
      plan.source_stack_offset_bytes.has_value()) {
    return X86StoreSourceAction::StoreStackSourceToFrameDestination;
  }

  if (plan.intent ==
          prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication &&
      plan.destination_base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
      plan.destination_stack_offset_bytes.has_value() &&
      plan.recovered_source_value.has_value() &&
      plan.recovered_source_instruction_index.has_value() &&
      plan.source_storage_encoding ==
          prepare::PreparedStorageEncodingKind::Register) {
    return X86StoreSourceAction::StoreRecoveredRegisterSource;
  }

  return X86StoreSourceAction::Reject;
}

int x86_consumer_reuses_frame_destination_and_source_home() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%stored");
  auto access = frame_slot_store_access(101, 7, 8);
  auto home = value_home(prepare::PreparedValueHomeKind::StackSlot, 1, 101);
  home.slot_id = 11;
  home.offset_bytes = 40;
  home.size_bytes = 8;
  home.align_bytes = 8;
  const prepare::PreparedFrameSlot destination_slot{
      .slot_id = 7,
      .object_id = 3,
      .function_name = 17,
      .offset_bytes = 80,
      .size_bytes = 8,
      .align_bytes = 8,
  };
  const prepare::PreparedStackObject destination_object{
      .object_id = 3,
      .function_name = 17,
      .slot_name = c4c::SlotNameId{33},
      .source_kind = "local",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  };

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &access,
      .source_home = &home,
      .destination_frame_slot = &destination_slot,
      .destination_stack_object = &destination_object,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
  });

  if (choose_x86_store_source_action(plan) !=
          X86StoreSourceAction::StoreStackSourceToFrameDestination ||
      plan.source_value_name != 101 ||
      plan.source_slot_id != std::optional<prepare::PreparedFrameSlotId>{11} ||
      plan.source_stack_offset_bytes != std::optional<std::size_t>{40} ||
      plan.destination_frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{7} ||
      plan.destination_object_id != std::optional<prepare::PreparedObjectId>{3} ||
      plan.destination_stack_offset_bytes != std::optional<std::size_t>{80} ||
      plan.destination_byte_offset != 8) {
    return fail("expected x86 consumer to reuse neutral frame/source-home facts");
  }
  return 0;
}

int x86_consumer_reuses_recovered_source_facts() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%wide");
  const auto recovered = bir::Value::named(bir::TypeKind::I8, "%narrow");
  auto access = frame_slot_store_access(102, 9, 0);
  auto home = value_home(prepare::PreparedValueHomeKind::Register, 2, 102);
  home.register_name = "x86-register-token";
  const prepare::PreparedFrameSlot destination_slot{
      .slot_id = 9,
      .object_id = 4,
      .function_name = 17,
      .offset_bytes = 112,
      .size_bytes = 8,
      .align_bytes = 8,
  };

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &access,
      .source_home = &home,
      .destination_frame_slot = &destination_slot,
      .recovered_source_value = &recovered,
      .recovered_source_instruction_index = std::size_t{6},
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
  });

  if (choose_x86_store_source_action(plan) !=
          X86StoreSourceAction::StoreRecoveredRegisterSource ||
      plan.recovered_source_value->name != "%narrow" ||
      plan.recovered_source_instruction_index != std::optional<std::size_t>{6} ||
      plan.source_storage_encoding !=
          prepare::PreparedStorageEncodingKind::Register ||
      plan.destination_stack_offset_bytes != std::optional<std::size_t>{112}) {
    return fail("expected x86 consumer to reuse recovered source facts");
  }
  return 0;
}

int x86_consumer_reuses_pointer_writeback_facts() {
  const auto source = bir::Value::named(bir::TypeKind::Ptr, "%pointer_value");
  auto access = pointer_value_store_access(201, 301, 16);
  auto source_home = value_home(prepare::PreparedValueHomeKind::Register, 4, 201);
  source_home.register_name = "x86-pointer-token";
  auto pointer_home =
      value_home(prepare::PreparedValueHomeKind::StackSlot, 5, 301);
  pointer_home.slot_id = 12;
  pointer_home.offset_bytes = 128;

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &access,
      .source_home = &source_home,
      .pointer_base_home = &pointer_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback,
      .pointer_store_writeback = true,
  });

  if (choose_x86_store_source_action(plan) !=
          X86StoreSourceAction::WritebackPointerThroughFrameHome ||
      plan.destination_pointer_value_name !=
          std::optional<c4c::ValueNameId>{301} ||
      plan.destination_byte_offset != 16 ||
      !plan.pointer_store_writeback ||
      plan.pointer_base_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{12} ||
      plan.pointer_base_stack_offset_bytes !=
          std::optional<std::size_t>{128}) {
    return fail("expected x86 consumer to reuse pointer writeback facts");
  }
  return 0;
}

int x86_consumer_rejects_incomplete_store_source_plan() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%missing_access");
  const auto missing_access = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
  });

  if (choose_x86_store_source_action(missing_access) !=
      X86StoreSourceAction::Reject) {
    return fail("expected x86 consumer to reject incomplete store-source plan");
  }
  return 0;
}

}  // namespace

int main() {
  if (int rc = x86_consumer_reuses_frame_destination_and_source_home(); rc != 0) {
    return rc;
  }
  if (int rc = x86_consumer_reuses_recovered_source_facts(); rc != 0) {
    return rc;
  }
  if (int rc = x86_consumer_reuses_pointer_writeback_facts(); rc != 0) {
    return rc;
  }
  return x86_consumer_rejects_incomplete_store_source_plan();
}
