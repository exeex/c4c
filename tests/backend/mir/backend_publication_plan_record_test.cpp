#include "src/backend/prealloc/publication_plans.hpp"

#include <iostream>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

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
      .function_name = 11,
      .value_name = value_name,
      .kind = kind,
  };
}

int records_register_home_intent() {
  auto home = home_with_kind(prepare::PreparedValueHomeKind::Register, 1, 101);
  home.register_name = "target-register-token";
  const auto source = bir::Value::named(bir::TypeKind::I64, "%reg_source");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind != prepare::PreparedScalarPublicationHookKind::RegisterHome ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::Register ||
      plan.destination_home_kind != prepare::PreparedValueHomeKind::Register ||
      plan.source_value_id != 1 || plan.source_value_name != 101 ||
      plan.source_value.name != "%reg_source") {
    return fail("expected register-home publication intent");
  }
  return 0;
}

int records_stack_slot_home_intent() {
  auto home = home_with_kind(prepare::PreparedValueHomeKind::StackSlot, 2, 102);
  home.slot_id = 7;
  home.offset_bytes = 32;
  home.size_bytes = 8;
  home.align_bytes = 8;
  const auto source = bir::Value::named(bir::TypeKind::I64, "%stack_source");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind != prepare::PreparedScalarPublicationHookKind::StackSlotHome ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
      plan.slot_id != std::optional<prepare::PreparedFrameSlotId>{7} ||
      plan.stack_offset_bytes != std::optional<std::size_t>{32} ||
      plan.size_bytes != std::optional<std::size_t>{8} ||
      plan.align_bytes != std::optional<std::size_t>{8}) {
    return fail("expected stack-slot publication intent");
  }
  return 0;
}

int records_rematerializable_immediate_intent() {
  auto home = home_with_kind(
      prepare::PreparedValueHomeKind::RematerializableImmediate, 3, 103);
  home.immediate_i32 = -42;
  const auto source = bir::Value::named(bir::TypeKind::I32, "%imm_source");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind !=
          prepare::PreparedScalarPublicationHookKind::RematerializableImmediate ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::Immediate ||
      plan.immediate_i32 != std::optional<std::int64_t>{-42}) {
    return fail("expected rematerializable-immediate publication intent");
  }
  return 0;
}

int records_pointer_base_plus_offset_intent() {
  auto home = home_with_kind(
      prepare::PreparedValueHomeKind::PointerBasePlusOffset, 4, 104);
  home.pointer_base_value_name = 204;
  home.pointer_byte_delta = 24;
  const auto source = bir::Value::named(bir::TypeKind::Ptr, "%derived_pointer");

  const auto plan = prepare::plan_prepared_scalar_publication({
      .source_value = &source,
      .destination_home = &home,
  });

  if (!prepare::prepared_scalar_publication_available(plan) ||
      plan.hook_kind !=
          prepare::PreparedScalarPublicationHookKind::PointerBasePlusOffset ||
      plan.storage_encoding != prepare::PreparedStorageEncodingKind::ComputedAddress ||
      plan.pointer_base_value_name != std::optional<c4c::ValueNameId>{204} ||
      plan.pointer_byte_delta != std::optional<std::int64_t>{24}) {
    return fail("expected pointer-base-plus-offset publication intent");
  }
  return 0;
}

int records_shape_without_aarch64_policy() {
  if (prepare::prepared_scalar_publication_hook_kind_name(
          prepare::PreparedScalarPublicationHookKind::RegisterHome) !=
      "register_home") {
    return fail("expected target-neutral hook spelling");
  }
  if (prepare::prepared_scalar_publication_status_name(
          prepare::PreparedScalarPublicationStatus::MissingPointerDelta) !=
      "missing_pointer_delta") {
    return fail("expected target-neutral status spelling");
  }
  if (prepare::prepared_publication_storage_encoding_from_home(
          prepare::PreparedValueHomeKind::PointerBasePlusOffset) !=
      prepare::PreparedStorageEncodingKind::ComputedAddress) {
    return fail("expected pointer homes to map to computed-address storage");
  }
  return 0;
}

}  // namespace

int main() {
  if (int rc = records_register_home_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_stack_slot_home_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_rematerializable_immediate_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_pointer_base_plus_offset_intent(); rc != 0) {
    return rc;
  }
  return records_shape_without_aarch64_policy();
}
