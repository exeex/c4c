#include "src/backend/prealloc/publication_plans.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

enum class X86PublicationAction {
  Reject,
  UseRegisterHome,
  ReloadFrameSlot,
  MaterializeImmediate,
  ComputeAddress,
};

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedValueHome home_with_kind(
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

X86PublicationAction choose_x86_publication_action(
    const prepare::PreparedScalarPublicationPlan& plan) {
  if (!prepare::prepared_scalar_publication_available(plan)) {
    return X86PublicationAction::Reject;
  }

  switch (plan.hook_kind) {
    case prepare::PreparedScalarPublicationHookKind::RegisterHome:
      return plan.storage_encoding == prepare::PreparedStorageEncodingKind::Register
                 ? X86PublicationAction::UseRegisterHome
                 : X86PublicationAction::Reject;
    case prepare::PreparedScalarPublicationHookKind::StackSlotHome:
      return plan.storage_encoding == prepare::PreparedStorageEncodingKind::FrameSlot &&
                     plan.slot_id.has_value() &&
                     plan.stack_offset_bytes.has_value()
                 ? X86PublicationAction::ReloadFrameSlot
                 : X86PublicationAction::Reject;
    case prepare::PreparedScalarPublicationHookKind::RematerializableImmediate:
      return plan.storage_encoding == prepare::PreparedStorageEncodingKind::Immediate &&
                     (plan.immediate_i32.has_value() ||
                      plan.immediate_f128.has_value())
                 ? X86PublicationAction::MaterializeImmediate
                 : X86PublicationAction::Reject;
    case prepare::PreparedScalarPublicationHookKind::PointerBasePlusOffset:
      return plan.storage_encoding ==
                         prepare::PreparedStorageEncodingKind::ComputedAddress &&
                     plan.pointer_base_value_name.has_value() &&
                     plan.pointer_byte_delta.has_value()
                 ? X86PublicationAction::ComputeAddress
                 : X86PublicationAction::Reject;
    case prepare::PreparedScalarPublicationHookKind::None:
      return X86PublicationAction::Reject;
  }
  return X86PublicationAction::Reject;
}

int x86_consumer_reuses_register_publication_plan() {
  auto home = home_with_kind(prepare::PreparedValueHomeKind::Register, 1, 101);
  home.register_name = "rax";
  const auto source = bir::Value::named(bir::TypeKind::I64, "%published");
  const prepare::PreparedBlockEntryPublication entry_publication{
      .status = prepare::PreparedBlockEntryPublicationStatus::Available,
      .home = &home,
      .destination_value_id = 1,
      .destination_value_name = 101,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = std::string{"rax"},
  };

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
      .current_block_entry_publication = &entry_publication,
  });

  if (choose_x86_publication_action(plan) !=
          X86PublicationAction::UseRegisterHome ||
      plan.source_value.name != "%published" ||
      plan.destination_home == nullptr ||
      plan.destination_home->register_name != std::optional<std::string>{"rax"} ||
      !plan.current_block_entry_publication_available) {
    return fail("expected x86 consumer to use register publication plan");
  }
  return 0;
}

int x86_consumer_reuses_stack_slot_publication_plan() {
  auto home = home_with_kind(prepare::PreparedValueHomeKind::StackSlot, 2, 102);
  home.slot_id = 8;
  home.offset_bytes = 40;
  home.size_bytes = 8;
  home.align_bytes = 8;
  const auto source = bir::Value::named(bir::TypeKind::I64, "%from_stack");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (choose_x86_publication_action(plan) !=
          X86PublicationAction::ReloadFrameSlot ||
      plan.slot_id != std::optional<prepare::PreparedFrameSlotId>{8} ||
      plan.stack_offset_bytes != std::optional<std::size_t>{40} ||
      plan.size_bytes != std::optional<std::size_t>{8}) {
    return fail("expected x86 consumer to use stack-slot publication plan");
  }
  return 0;
}

int x86_consumer_reuses_immediate_and_address_publication_plans() {
  auto immediate_home = home_with_kind(
      prepare::PreparedValueHomeKind::RematerializableImmediate, 3, 103);
  immediate_home.immediate_i32 = 99;
  const auto immediate_source = bir::Value::immediate_i32(99);
  const auto immediate_plan = prepare::plan_prepared_scalar_publication({
      .source_value = &immediate_source,
      .destination_home = &immediate_home,
  });

  if (choose_x86_publication_action(immediate_plan) !=
          X86PublicationAction::MaterializeImmediate ||
      immediate_plan.immediate_i32 != std::optional<std::int64_t>{99}) {
    return fail("expected x86 consumer to use immediate publication plan");
  }

  auto address_home = home_with_kind(
      prepare::PreparedValueHomeKind::PointerBasePlusOffset, 4, 104);
  address_home.pointer_base_value_name = 501;
  address_home.pointer_byte_delta = -16;
  const auto address_source = bir::Value::named(bir::TypeKind::Ptr, "%addr");
  const auto address_plan = prepare::plan_prepared_scalar_publication({
      .source_value = &address_source,
      .destination_home = &address_home,
  });

  if (choose_x86_publication_action(address_plan) !=
          X86PublicationAction::ComputeAddress ||
      address_plan.pointer_base_value_name !=
          std::optional<c4c::ValueNameId>{501} ||
      address_plan.pointer_byte_delta != std::optional<std::int64_t>{-16}) {
    return fail("expected x86 consumer to use address publication plan");
  }
  return 0;
}

int x86_consumer_rejects_incomplete_publication_plans() {
  auto register_home =
      home_with_kind(prepare::PreparedValueHomeKind::Register, 5, 105);
  const auto missing_source_plan = prepare::plan_prepared_scalar_publication({
      .destination_home = &register_home,
  });
  if (missing_source_plan.status !=
          prepare::PreparedScalarPublicationStatus::MissingSourceValue ||
      choose_x86_publication_action(missing_source_plan) !=
          X86PublicationAction::Reject) {
    return fail("expected x86 consumer to reject missing-source plan");
  }

  auto stack_home =
      home_with_kind(prepare::PreparedValueHomeKind::StackSlot, 6, 106);
  stack_home.offset_bytes = 24;
  const auto stack_source = bir::Value::named(bir::TypeKind::I64, "%bad_stack");
  const auto missing_slot_plan = prepare::plan_prepared_scalar_publication({
      .source_value = &stack_source,
      .destination_home = &stack_home,
  });
  if (missing_slot_plan.status !=
          prepare::PreparedScalarPublicationStatus::MissingStackSlot ||
      choose_x86_publication_action(missing_slot_plan) !=
          X86PublicationAction::Reject) {
    return fail("expected x86 consumer to reject missing-slot plan");
  }

  auto address_home = home_with_kind(
      prepare::PreparedValueHomeKind::PointerBasePlusOffset, 7, 107);
  address_home.pointer_base_value_name = 777;
  const auto address_source = bir::Value::named(bir::TypeKind::Ptr, "%bad_addr");
  const auto missing_delta_plan = prepare::plan_prepared_scalar_publication({
      .source_value = &address_source,
      .destination_home = &address_home,
  });
  if (missing_delta_plan.status !=
          prepare::PreparedScalarPublicationStatus::MissingPointerDelta ||
      choose_x86_publication_action(missing_delta_plan) !=
          X86PublicationAction::Reject) {
    return fail("expected x86 consumer to reject missing-delta plan");
  }

  return 0;
}

}  // namespace

int main() {
  if (int rc = x86_consumer_reuses_register_publication_plan(); rc != 0) {
    return rc;
  }
  if (int rc = x86_consumer_reuses_stack_slot_publication_plan(); rc != 0) {
    return rc;
  }
  if (int rc = x86_consumer_reuses_immediate_and_address_publication_plans();
      rc != 0) {
    return rc;
  }
  return x86_consumer_rejects_incomplete_publication_plans();
}
