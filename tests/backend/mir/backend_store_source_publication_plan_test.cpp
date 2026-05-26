#include "src/backend/prealloc/module.hpp"

#include <iostream>
#include <optional>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

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

prepare::PreparedValueHome source_home(
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

int records_local_store_source_identity() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%stored");
  auto access = frame_slot_store_access(101, 7, 16);
  auto home = source_home(prepare::PreparedValueHomeKind::StackSlot, 1, 101);
  home.slot_id = 7;
  home.offset_bytes = 48;
  home.size_bytes = 8;
  home.align_bytes = 8;
  const prepare::PreparedFrameSlot slot{
      .slot_id = 7,
      .object_id = 3,
      .function_name = 17,
      .offset_bytes = 48,
      .size_bytes = 8,
      .align_bytes = 8,
  };
  const prepare::PreparedStackObject object{
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
      .destination_frame_slot = &slot,
      .destination_stack_object = &object,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
  });

  if (!prepare::prepared_store_source_publication_available(plan) ||
      plan.intent !=
          prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication ||
      plan.source_value.name != "%stored" ||
      plan.source_value_id != 1 ||
      plan.source_value_name != 101 ||
      plan.destination_base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      plan.destination_frame_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{7} ||
      plan.destination_byte_offset != 16 ||
      !plan.destination_can_use_base_plus_offset ||
      plan.destination_object_id != std::optional<prepare::PreparedObjectId>{3} ||
      plan.destination_stack_offset_bytes != std::optional<std::size_t>{48} ||
      plan.source_home_kind != prepare::PreparedValueHomeKind::StackSlot ||
      plan.source_storage_encoding !=
          prepare::PreparedStorageEncodingKind::FrameSlot ||
      plan.source_slot_id != std::optional<prepare::PreparedFrameSlotId>{7}) {
    return fail("expected local store-source publication identity");
  }
  return 0;
}

int records_recovered_source_without_target_policy() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%wide_load");
  const auto recovered = bir::Value::named(bir::TypeKind::I8, "%narrow_store");
  auto access = frame_slot_store_access(102, 9, 0);
  auto home = source_home(prepare::PreparedValueHomeKind::Register, 2, 102);
  home.register_name = "target-register-token";

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &access,
      .source_home = &home,
      .recovered_source_value = &recovered,
      .recovered_source_instruction_index = std::size_t{4},
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
  });

  if (!prepare::prepared_store_source_publication_available(plan) ||
      !plan.recovered_source_value.has_value() ||
      plan.recovered_source_value->name != "%narrow_store" ||
      plan.recovered_source_instruction_index != std::optional<std::size_t>{4} ||
      plan.source_storage_encoding !=
          prepare::PreparedStorageEncodingKind::Register) {
    return fail("expected recovered source facts without target policy");
  }
  return 0;
}

prepare::PreparedBirModule cast_store_source_fixture() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("store_cast_source");
  const auto block_label = prepared.names.block_labels.intern("entry");
  prepared.names.value_names.intern("%input");
  prepared.names.value_names.intern("%casted");

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = block_label;
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I64, "%casted"),
      .operand = bir::Value::named(bir::TypeKind::I32, "%input"),
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "out",
      .value = bir::Value::named(bir::TypeKind::I64, "%casted"),
      .byte_offset = 0,
      .align_bytes = 8,
  });

  bir::Function function;
  function.name = "store_cast_source";
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = block_label,
      }},
  });

  return prepared;
}

int records_cast_source_producer_from_prepared_lookup() {
  auto prepared = cast_store_source_fixture();
  const auto casted_name = prepared.names.value_names.find("%casted");
  const auto block_label = prepared.names.block_labels.find("entry");
  const auto source = bir::Value::named(bir::TypeKind::I64, "%casted");
  auto access = frame_slot_store_access(casted_name, 15, 0);
  auto home = source_home(prepare::PreparedValueHomeKind::Register, 8, casted_name);
  home.register_name = "prepared-cast-result-register";

  const auto producer_lookups =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          prepared, prepared.control_flow.functions.front());
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          &producer_lookups, casted_name);
  if (producer == nullptr ||
      producer->kind != prepare::PreparedEdgePublicationSourceProducerKind::Cast ||
      producer->block_label != block_label ||
      producer->instruction_index != 0 ||
      producer->cast == nullptr ||
      producer->cast->opcode != bir::CastOpcode::SExt) {
    return fail("expected prepared cast source producer fact before target emission");
  }

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &access,
      .source_home = &home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      .source_producer = producer,
  });

  if (!prepare::prepared_store_source_publication_available(plan) ||
      plan.source_value_name != casted_name ||
      plan.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Cast ||
      plan.source_producer_block_label !=
          std::optional<c4c::BlockLabelId>{block_label} ||
      plan.source_producer_instruction_index != std::optional<std::size_t>{0} ||
      plan.source_cast != producer->cast ||
      plan.source_cast->opcode != bir::CastOpcode::SExt) {
    return fail("expected store-source plan to publish prepared cast producer");
  }
  return 0;
}

int records_pointer_writeback_intent() {
  const auto source_value = bir::Value::named(bir::TypeKind::I32, "%store_value");
  prepare::PreparedMemoryAccess access{
      .function_name = 17,
      .block_label = 23,
      .inst_index = 8,
      .stored_value_name = c4c::ValueNameId{201},
      .address =
          prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
              .pointer_value_name = c4c::ValueNameId{301},
              .byte_offset = 12,
              .size_bytes = 4,
              .align_bytes = 4,
              .can_use_base_plus_offset = true,
          },
  };
  auto value_home = source_home(prepare::PreparedValueHomeKind::Register, 4, 201);
  value_home.register_name = "value-register-token";
  auto pointer_home =
      source_home(prepare::PreparedValueHomeKind::StackSlot, 5, 301);
  pointer_home.slot_id = 12;
  pointer_home.offset_bytes = 96;

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source_value,
      .destination_access = &access,
      .source_home = &value_home,
      .pointer_base_home = &pointer_home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback,
      .pointer_store_writeback = true,
  });

  if (!prepare::prepared_store_source_publication_available(plan) ||
      plan.intent !=
          prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback ||
      !plan.pointer_store_writeback ||
      plan.destination_base_kind !=
          prepare::PreparedAddressBaseKind::PointerValue ||
      plan.destination_pointer_value_name !=
          std::optional<c4c::ValueNameId>{301} ||
      plan.destination_byte_offset != 12 ||
      plan.pointer_base_home_kind != prepare::PreparedValueHomeKind::StackSlot ||
      plan.pointer_base_slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{12} ||
      plan.pointer_base_stack_offset_bytes !=
          std::optional<std::size_t>{96}) {
    return fail("expected pointer writeback planning facts");
  }
  return 0;
}

int records_global_pending_publication_flags() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%global_value");
  auto access = frame_slot_store_access(401, 14, 24);
  auto home = source_home(prepare::PreparedValueHomeKind::PointerBasePlusOffset,
                          6,
                          401);
  home.pointer_base_value_name = 501;
  home.pointer_byte_delta = -8;

  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &access,
      .source_home = &home,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication,
      .pending_publication = true,
      .stack_homes_only = true,
      .duplicate_publication = true,
  });

  if (!prepare::prepared_store_source_publication_available(plan) ||
      plan.intent !=
          prepare::PreparedStoreSourcePublicationIntent::StoreGlobalPublication ||
      !plan.pending_publication ||
      !plan.stack_homes_only ||
      !plan.duplicate_publication ||
      plan.source_storage_encoding !=
          prepare::PreparedStorageEncodingKind::ComputedAddress ||
      plan.source_pointer_base_value_name !=
          std::optional<c4c::ValueNameId>{501} ||
      plan.source_pointer_byte_delta != std::optional<std::int64_t>{-8}) {
    return fail("expected pending global publication facts");
  }
  return 0;
}

int rejects_incomplete_inputs() {
  const auto source = bir::Value::named(bir::TypeKind::I64, "%missing_access");
  const auto missing_source = prepare::plan_prepared_store_source_publication({});
  if (missing_source.status !=
          prepare::PreparedStoreSourcePublicationStatus::MissingSourceValue ||
      prepare::prepared_store_source_publication_available(missing_source)) {
    return fail("expected missing-source store-source plan");
  }

  const auto missing_access = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
  });
  if (missing_access.status !=
          prepare::PreparedStoreSourcePublicationStatus::MissingDestinationAccess ||
      prepare::prepared_store_source_publication_available(missing_access)) {
    return fail("expected missing-destination-access store-source plan");
  }

  if (prepare::prepared_store_source_publication_intent_name(
          prepare::PreparedStoreSourcePublicationIntent::PointerStoreWriteback) !=
      "pointer_store_writeback") {
    return fail("expected stable store-source intent spelling");
  }
  return 0;
}

}  // namespace

int main() {
  if (int rc = records_local_store_source_identity(); rc != 0) {
    return rc;
  }
  if (int rc = records_recovered_source_without_target_policy(); rc != 0) {
    return rc;
  }
  if (int rc = records_cast_source_producer_from_prepared_lookup(); rc != 0) {
    return rc;
  }
  if (int rc = records_pointer_writeback_intent(); rc != 0) {
    return rc;
  }
  if (int rc = records_global_pending_publication_flags(); rc != 0) {
    return rc;
  }
  return rejects_incomplete_inputs();
}
