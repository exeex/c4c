#include "src/backend/prealloc/module.hpp"
#include "src/backend/mir/query.hpp"

#include <iostream>
#include <optional>
#include <variant>

namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
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

prepare::PreparedMemoryAccess frame_slot_load_access(
    c4c::ValueNameId result_value_name,
    prepare::PreparedFrameSlotId slot_id,
    std::size_t inst_index,
    std::int64_t byte_offset) {
  return prepare::PreparedMemoryAccess{
      .function_name = 17,
      .block_label = 23,
      .inst_index = inst_index,
      .result_value_name = result_value_name,
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

bir::CallArgAbiInfo register_abi(bir::TypeKind type) {
  return bir::CallArgAbiInfo{
      .type = type,
      .size_bytes = 4,
      .align_bytes = 4,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_in_register = true,
  };
}

bool prepared_and_bir_load_local_source_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedMemoryAccessLookups& memory_accesses,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto prepared =
      prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &memory_accesses,
          &source_producers,
          block_label,
          &block,
          value,
          before_instruction_index);
  const auto mir_identity = mir::find_bir_same_block_load_local_source_identity(
      mir::BirSameBlockLoadLocalSourceRequest{
          .block = &block,
          .block_label = block.label,
          .root_value = &value,
          .root_value_name = value.kind == bir::Value::Kind::Named
                                 ? std::string_view(value.name)
                                 : std::string_view{},
          .root_value_type = value.type,
          .before_instruction_index = before_instruction_index,
      });
  const auto route1_index = bir::route1_build_producer_index(block);
  const auto route3 =
      bir::route3_same_block_load_local_source_record(
          bir::Route1SameBlockProducerQuery{
              .index = &route1_index,
              .before_instruction_index = before_instruction_index,
          },
          value);
  const auto route3_index = bir::route3_build_memory_access_index(block);
  const auto indexed_route3 =
      bir::route3_find_same_block_load_local_source(
          bir::Route3MemoryAccessQuery{
              .index = &route3_index,
              .before_instruction_index = before_instruction_index,
          },
          value);
  if (prepared.has_value() != static_cast<bool>(mir_identity)) {
    return false;
  }
  if (!prepared.has_value()) {
    return !route3 && !indexed_route3;
  }
  const auto prepared_slot =
      prepared->source_access != nullptr
          ? prepared->source_access->address.frame_slot_id
          : std::optional<prepare::PreparedFrameSlotId>{};
  return prepared->producer != nullptr &&
         prepared->producer->load_local == mir_identity.load_local &&
         prepared->source_access != nullptr &&
         route3 &&
         indexed_route3 &&
         route3.load_access.instruction == mir_identity.producer.inst &&
         indexed_route3.load_access.instruction == route3.load_access.instruction &&
         indexed_route3.load_access.instruction_index ==
             route3.load_access.instruction_index &&
         route3.load_access.node_kind == bir::Route3MemoryAccessNodeKind::LoadLocal &&
         route3.load_access.base_kind == bir::Route3MemoryAccessBaseKind::LocalSlot &&
         route3.load_access.result_value.name == value.name &&
         mir_identity.producer.kind == mir::SameBlockProducerKind::LoadLocal &&
         mir_identity.producer.instruction_index ==
             prepared->producer->instruction_index &&
         mir_identity.memory_access.node_kind ==
             mir::BirMemoryAccessNodeKind::LoadLocal &&
         mir_identity.memory_access.base_kind ==
             mir::BirMemoryAccessBaseKind::LocalSlot &&
         mir_identity.memory_access.result_value_name == value.name &&
         (!prepared_slot.has_value() ||
          mir_identity.memory_access.local_slot_id ==
              static_cast<c4c::SlotNameId>(*prepared_slot)) &&
         mir_identity.result_value.name == value.name &&
         mir_identity.result_value.type == value.type;
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

int records_fixed_formal_store_source_from_prepared_publication_plan() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("fixed_formal_store");
  const auto formal_name = names.value_names.intern("%formal");
  bir::Function function;
  function.name = "fixed_formal_store";
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "%formal",
      .abi = register_abi(bir::TypeKind::I32),
  });
  auto access = frame_slot_store_access(formal_name, 11, 0);
  auto home = source_home(prepare::PreparedValueHomeKind::Register, 6, formal_name);
  home.register_name = "w0";
  prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {home},
  };
  const auto lookups = prepare::make_prepared_value_home_lookups(&locations);
  const auto source = bir::Value::named(bir::TypeKind::I32, "%formal");
  const auto planned =
      prepare::plan_prepared_fixed_formal_store_source_publication(
          prepare::PreparedFormalPublicationInputs{
              .names = &names,
              .function = &function,
              .value_locations = &locations,
              .value_home_lookups = &lookups,
          },
          prepare::PreparedStoreSourcePublicationInputs{
              .source_value = &source,
              .destination_access = &access,
              .source_home = &locations.value_homes.front(),
              .intent =
                  prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
          });

  if (!planned.fixed_formal_source ||
      !prepare::prepared_store_source_publication_available(planned.store_source) ||
      planned.store_source.source_value_name != formal_name ||
      planned.store_source.source_home != &locations.value_homes.front() ||
      planned.formal_publication.value_name != formal_name ||
      planned.formal_publication.value_id !=
          std::optional<prepare::PreparedValueId>{6}) {
    return fail("expected fixed-formal store-source publication from prepared planning");
  }

  function.params.front().is_byval = true;
  const auto byval_rejected =
      prepare::plan_prepared_fixed_formal_store_source_publication(
          prepare::PreparedFormalPublicationInputs{
              .names = &names,
              .function = &function,
              .value_locations = &locations,
              .value_home_lookups = &lookups,
          },
          prepare::PreparedStoreSourcePublicationInputs{
              .source_value = &source,
              .destination_access = &access,
              .source_home = &locations.value_homes.front(),
              .intent =
                  prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
          });
  if (byval_rejected.fixed_formal_source ||
      !prepare::prepared_store_source_publication_available(
          byval_rejected.store_source)) {
    return fail("expected fixed-formal planning to reject byval formals without losing store facts");
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

int recovered_narrow_store_requires_prepared_memory_access() {
  const auto block_label = c4c::BlockLabelId{23};
  const auto slot_id = prepare::PreparedFrameSlotId{9};

  bir::NameTables bir_names;
  prepare::PreparedNameTables names;
  const auto narrow_name = names.value_names.intern("%narrow_store");
  const auto wide_name = names.value_names.intern("%wide_load");
  prepare::PreparedStackLayout stack_layout;
  stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = 4,
      .function_name = 17,
      .offset_bytes = 64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block block;
  block.insts.push_back(bir::StoreLocalInst{
      .slot_name = "spelling_only_store",
      .value = bir::Value::named(bir::TypeKind::I8, "%narrow_store"),
      .byte_offset = 0,
      .align_bytes = 1,
  });
  block.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%wide_load"),
      .slot_name = "different_load_spelling",
      .byte_offset = 0,
      .align_bytes = 4,
  });

  prepare::PreparedAddressingFunction addressing;
  auto store_access = frame_slot_store_access(narrow_name, slot_id, 0);
  store_access.inst_index = 0;
  auto load_access = frame_slot_load_access(wide_name, slot_id, 1, 0);
  addressing.accesses = {store_access, load_access};

  const auto recovered =
      prepare::find_prepared_recovered_narrow_store_source_for_wide_local_load(
          names,
          bir_names,
          stack_layout,
          &addressing,
          block_label,
          &block,
          std::get<bir::LoadLocalInst>(block.insts[1]),
          1);
  if (!recovered.has_value() ||
      recovered->instruction_index != std::size_t{0} ||
      recovered->stored_value.name != "%narrow_store") {
    return fail("expected recovered source through prepared frame-slot access");
  }

  auto missing_store_access = addressing;
  missing_store_access.accesses = {load_access};
  if (prepare::find_prepared_recovered_narrow_store_source_for_wide_local_load(
          names,
          bir_names,
          stack_layout,
          &missing_store_access,
          block_label,
          &block,
          std::get<bir::LoadLocalInst>(block.insts[1]),
          1)
          .has_value()) {
    return fail("recovered source should not use local spelling without store access");
  }

  auto mismatched_store_access = store_access;
  mismatched_store_access.address.byte_offset = 1;
  auto mismatched_addressing = addressing;
  mismatched_addressing.accesses = {mismatched_store_access, load_access};
  if (prepare::find_prepared_recovered_narrow_store_source_for_wide_local_load(
          names,
          bir_names,
          stack_layout,
          &mismatched_addressing,
          block_label,
          &block,
          std::get<bir::LoadLocalInst>(block.insts[1]),
          1)
          .has_value()) {
    return fail("recovered source should reject prepared access offset drift");
  }

  return 0;
}

int classifies_byval_load_local_source_from_prepared_authority() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("byval_source");
  const auto block_label = names.block_labels.intern("entry");
  const auto byval_name = names.value_names.intern("%byval");

  bir::Function function;
  function.name = "byval_source";
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%byval",
      .is_byval = true,
  });

  const bir::LoadLocalInst load{
      .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .slot_name = "field",
  };
  const prepare::PreparedEdgePublicationSourceProducer producer{
      .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
      .block_label = block_label,
      .instruction_index = 3,
      .load_local = &load,
  };
  const prepare::PreparedAddressingFunction addressing{
      .function_name = function_name,
      .accesses = {prepare::PreparedMemoryAccess{
          .function_name = function_name,
          .block_label = block_label,
          .inst_index = 3,
          .result_value_name = names.value_names.intern("%loaded"),
          .address =
              prepare::PreparedAddress{
                  .base_kind = prepare::PreparedAddressBaseKind::PointerValue,
                  .pointer_value_name = byval_name,
                  .byte_offset = 8,
                  .size_bytes = 4,
                  .align_bytes = 4,
                  .can_use_base_plus_offset = true,
              },
      }},
  };

  const bool classified =
      prepare::prepared_store_source_load_local_is_byval_formal_pointer_source(
          names, &function, &addressing, &producer);
  const auto source = bir::Value::named(bir::TypeKind::I32, "%loaded");
  auto destination_access = frame_slot_store_access(102, 9, 0);
  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &destination_access,
      .byval_load_local_source = classified,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
      .source_producer = &producer,
  });

  if (!classified || !prepare::prepared_store_source_publication_available(plan) ||
      !plan.byval_load_local_source ||
      plan.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      plan.source_load_local != &load) {
    return fail("expected byval load-local source from prepared authority");
  }
  return 0;
}

int records_direct_global_select_chain_dependency_from_prepared_authority() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("entry");
  bir::Block block;
  block.label = "entry";
  block.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%global_ptr"),
      .global_name = "g",
  });
  block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::Ptr, "%selected"),
      .compare_type = bir::TypeKind::I1,
      .lhs = bir::Value::named(bir::TypeKind::I1, "%cond"),
      .rhs = bir::Value::immediate_i1(false),
      .true_value = bir::Value::named(bir::TypeKind::Ptr, "%global_ptr"),
      .false_value = bir::Value::named(bir::TypeKind::Ptr, "%fallback"),
  });
  const auto* load_global = std::get_if<bir::LoadGlobalInst>(&block.insts[0]);
  const auto* select = std::get_if<bir::SelectInst>(&block.insts[1]);
  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern("%global_ptr"),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_global = load_global,
      });
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern("%selected"),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind =
              prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization,
          .block_label = block_label,
          .instruction_index = 1,
          .select = select,
      });

  const auto dependency =
      prepare::find_prepared_store_source_direct_global_select_chain_dependency(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::Ptr, "%selected"),
          2);
  const auto source = bir::Value::named(bir::TypeKind::Ptr, "%selected");
  auto destination_access = frame_slot_store_access(103, 10, 0);
  const auto plan = prepare::plan_prepared_store_source_publication({
      .source_value = &source,
      .destination_access = &destination_access,
      .direct_global_select_chain_source =
          dependency.contains_direct_global_load,
      .direct_global_select_chain_root_is_select = dependency.root_is_select,
      .direct_global_select_chain_root_instruction_index =
          dependency.root_instruction_index,
      .intent = prepare::PreparedStoreSourcePublicationIntent::StoreLocalPublication,
  });

  if (!dependency.contains_direct_global_load ||
      !dependency.root_is_select ||
      dependency.root_instruction_index != std::optional<std::size_t>{1} ||
      !prepare::prepared_store_source_publication_available(plan) ||
      !plan.direct_global_select_chain_source ||
      !plan.direct_global_select_chain_root_is_select ||
      plan.direct_global_select_chain_root_instruction_index !=
          std::optional<std::size_t>{1}) {
    return fail("expected direct-global select-chain dependency facts");
  }
  return 0;
}

int finds_unpublished_load_local_source_from_indexed_authority() {
  prepare::PreparedNameTables names;
  const auto block_label = c4c::BlockLabelId{23};
  const auto slot_id = prepare::PreparedFrameSlotId{9};
  const auto other_slot_id = prepare::PreparedFrameSlotId{10};
  const auto loaded_name = names.value_names.intern("%loaded");
  const auto other_name = names.value_names.intern("%other");
  const auto stored_name = names.value_names.intern("%stored");

  prepare::PreparedStackLayout stack_layout;
  stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = slot_id,
      .object_id = 4,
      .function_name = 17,
      .offset_bytes = 64,
      .size_bytes = 32,
      .align_bytes = 8,
  });
  stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = other_slot_id,
      .object_id = 5,
      .function_name = 17,
      .offset_bytes = 96,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block block;
  block.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%loaded"),
      .slot_name = "slot",
      .slot_id = static_cast<c4c::SlotNameId>(slot_id),
      .byte_offset = 0,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "slot",
              .byte_offset = 0,
              .size_bytes = 8,
              .align_bytes = 8,
              .base_slot_id = static_cast<c4c::SlotNameId>(slot_id),
          },
  });
  const auto* load = std::get_if<bir::LoadLocalInst>(&block.insts[0]);
  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_local = load,
      });

  prepare::PreparedAddressingFunction addressing;
  addressing.accesses = {frame_slot_load_access(loaded_name, slot_id, 0, 0)};
  const auto memory_accesses = prepare::make_prepared_memory_access_lookups(&addressing);

  const auto source =
      prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &memory_accesses,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          1);
  if (!source.has_value() ||
      source->producer == nullptr ||
      source->producer->load_local != load ||
      source->source_access != &addressing.accesses.front()) {
    return fail("expected indexed unpublished load-local source authority");
  }
  block.label = "entry";
  if (!prepared_and_bir_load_local_source_match(
          names,
          stack_layout,
          memory_accesses,
          source_producers,
          block_label,
          block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          1)) {
    return fail("expected BIR load-local source identity to match prepared oracle");
  }
  if (!prepared_and_bir_load_local_source_match(
          names,
          stack_layout,
          memory_accesses,
          source_producers,
          block_label,
          block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          0)) {
    return fail("expected BIR/prepared load-local source to fail closed before producer");
  }
  if (mir::find_bir_same_block_load_local_source_identity(
          mir::BirSameBlockLoadLocalSourceRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%loaded",
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = 1,
          })) {
    return fail("expected BIR load-local source to reject root type mismatch");
  }
  const auto route3_index = bir::route3_build_memory_access_index(block);
  if (bir::route3_find_same_block_load_local_source(
          bir::Route3MemoryAccessQuery{
              .index = &route3_index,
              .before_instruction_index = 1,
          },
          bir::Value::named(bir::TypeKind::I32, "%loaded"))) {
    return fail("expected Route 3 load-local lookup to reject root type mismatch");
  }

  bir::Block mismatched_position_block;
  mismatched_position_block.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%other"),
      .slot_name = "other",
      .byte_offset = 0,
      .align_bytes = 8,
  });
  mismatched_position_block.insts.push_back(block.insts[0]);
  const auto* mismatched_position_load =
      std::get_if<bir::LoadLocalInst>(&mismatched_position_block.insts[1]);
  prepare::PreparedEdgePublicationSourceProducerLookups mismatched_position_producers;
  mismatched_position_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 1,
          .load_local = mismatched_position_load,
      });
  auto shifted_loaded_access = frame_slot_load_access(loaded_name, slot_id, 0, 0);
  shifted_loaded_access.inst_index = 0;
  auto wrong_position_access =
      frame_slot_load_access(other_name, other_slot_id, 0, 1);
  wrong_position_access.inst_index = 1;
  prepare::PreparedAddressingFunction mismatched_position_addressing;
  mismatched_position_addressing.accesses = {
      shifted_loaded_access,
      wrong_position_access,
  };
  const auto mismatched_position_memory_accesses =
      prepare::make_prepared_memory_access_lookups(&mismatched_position_addressing);
  const auto source_from_result_index =
      prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &mismatched_position_memory_accesses,
          &mismatched_position_producers,
          block_label,
          &mismatched_position_block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          2);
  if (!source_from_result_index.has_value() ||
      source_from_result_index->producer == nullptr ||
      source_from_result_index->source_access !=
          &mismatched_position_addressing.accesses.front()) {
    return fail(
        "source-producer authority should recover mismatched position accesses from result-indexed memory access");
  }

  const auto source_from_memory_access =
      prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &memory_accesses,
          nullptr,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          1);
  if (!source_from_memory_access.has_value() ||
      source_from_memory_access->producer != nullptr ||
      source_from_memory_access->source_access != &addressing.accesses.front()) {
    return fail("load-local source authority should recover from indexed memory access");
  }

  prepare::PreparedAddressingFunction empty_addressing;
  const auto empty_memory_accesses =
      prepare::make_prepared_memory_access_lookups(&empty_addressing);
  if (prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &empty_memory_accesses,
          nullptr,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          1)
          .has_value()) {
    return fail("load-local source authority should require indexed memory access");
  }

  bir::Block non_overlapping_store_block;
  non_overlapping_store_block.insts.push_back(block.insts[0]);
  non_overlapping_store_block.insts.push_back(bir::StoreLocalInst{
      .slot_name = "slot",
      .slot_id = static_cast<c4c::SlotNameId>(slot_id),
      .value = bir::Value::named(bir::TypeKind::I64, "%stored"),
      .byte_offset = 16,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "slot",
              .byte_offset = 16,
              .size_bytes = 8,
              .align_bytes = 8,
              .base_slot_id = static_cast<c4c::SlotNameId>(slot_id),
          },
  });
  non_overlapping_store_block.label = "entry";
  const auto* non_overlapping_load =
      std::get_if<bir::LoadLocalInst>(&non_overlapping_store_block.insts[0]);
  prepare::PreparedEdgePublicationSourceProducerLookups non_overlapping_producers;
  non_overlapping_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_local = non_overlapping_load,
      });
  auto non_overlapping_store = frame_slot_store_access(stored_name, slot_id, 16);
  non_overlapping_store.inst_index = 1;
  prepare::PreparedAddressingFunction non_overlapping_addressing;
  non_overlapping_addressing.accesses = {
      frame_slot_load_access(loaded_name, slot_id, 0, 0),
      non_overlapping_store,
  };
  const auto non_overlapping_accesses =
      prepare::make_prepared_memory_access_lookups(&non_overlapping_addressing);
  if (!prepare::find_prepared_same_block_load_local_source_producer(
           names,
           stack_layout,
           &non_overlapping_accesses,
           &non_overlapping_producers,
           block_label,
           &non_overlapping_store_block,
           bir::Value::named(bir::TypeKind::I64, "%loaded"),
           2)
           .has_value()) {
    return fail("non-overlapping intervening store should preserve load-local source");
  }
  if (mir::find_bir_same_block_load_local_source_identity(
          mir::BirSameBlockLoadLocalSourceRequest{
              .block = &non_overlapping_store_block,
              .block_label = non_overlapping_store_block.label,
              .root_value_name = "%loaded",
              .root_value_type = bir::TypeKind::I64,
              .before_instruction_index = 2,
          })) {
    return fail("BIR load-local source should fail closed for same-slot intervening store without layout overlap authority");
  }
  const auto non_overlapping_route3_index =
      bir::route3_build_memory_access_index(non_overlapping_store_block);
  if (bir::route3_find_same_block_load_local_source(
          bir::Route3MemoryAccessQuery{
              .index = &non_overlapping_route3_index,
              .before_instruction_index = 2,
          },
          bir::Value::named(bir::TypeKind::I64, "%loaded"))) {
    return fail("Route 3 load-local lookup should fail closed for same-slot store without layout authority");
  }

  bir::Block overlapping_store_block;
  overlapping_store_block.insts.push_back(block.insts[0]);
  overlapping_store_block.insts.push_back(bir::StoreLocalInst{
      .slot_name = "slot",
      .slot_id = static_cast<c4c::SlotNameId>(slot_id),
      .value = bir::Value::named(bir::TypeKind::I64, "%stored"),
      .byte_offset = 0,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "slot",
              .byte_offset = 0,
              .size_bytes = 8,
              .align_bytes = 8,
              .base_slot_id = static_cast<c4c::SlotNameId>(slot_id),
          },
  });
  overlapping_store_block.label = "entry";
  const auto* overlapping_load =
      std::get_if<bir::LoadLocalInst>(&overlapping_store_block.insts[0]);
  prepare::PreparedEdgePublicationSourceProducerLookups overlapping_producers;
  overlapping_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_local = overlapping_load,
      });
  auto overlapping_store = frame_slot_store_access(stored_name, slot_id, 0);
  overlapping_store.inst_index = 1;
  prepare::PreparedAddressingFunction overlapping_addressing;
  overlapping_addressing.accesses = {
      frame_slot_load_access(loaded_name, slot_id, 0, 0),
      overlapping_store,
  };
  const auto overlapping_accesses =
      prepare::make_prepared_memory_access_lookups(&overlapping_addressing);
  if (prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &overlapping_accesses,
          &overlapping_producers,
          block_label,
          &overlapping_store_block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          2)
          .has_value()) {
    return fail("overlapping intervening store should block load-local source");
  }
  if (!prepared_and_bir_load_local_source_match(
          names,
          stack_layout,
          overlapping_accesses,
          overlapping_producers,
          block_label,
          overlapping_store_block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          2)) {
    return fail("expected BIR/prepared load-local source to fail closed for same-slot intervening store");
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

int plans_pending_global_publication_candidates_from_prepared_state() {
  const auto selected_name = c4c::ValueNameId{701};
  const auto tail_name = c4c::ValueNameId{702};
  const auto global_name = c4c::LinkNameId{17};
  const bir::Block block{
      .insts =
          {bir::SelectInst{
               .result = bir::Value::named(bir::TypeKind::I32, "%selected"),
               .compare_type = bir::TypeKind::I32,
               .lhs = bir::Value::immediate_i32(1),
               .rhs = bir::Value::immediate_i32(1),
               .true_value = bir::Value::immediate_i32(2),
               .false_value = bir::Value::immediate_i32(3),
           },
           bir::StoreGlobalInst{
               .global_name_id = global_name,
               .value = bir::Value::named(bir::TypeKind::I32, "%selected"),
               .address =
                   bir::MemoryAddress{
                       .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .base_link_name_id = global_name,
                   },
           },
           bir::BinaryInst{
               .opcode = bir::BinaryOpcode::Add,
               .result = bir::Value::named(bir::TypeKind::I32, "%tail"),
               .operand_type = bir::TypeKind::I32,
               .lhs = bir::Value::immediate_i32(4),
               .rhs = bir::Value::immediate_i32(5),
           },
           bir::StoreGlobalInst{
               .global_name_id = global_name,
               .value = bir::Value::named(bir::TypeKind::I32, "%tail"),
               .byte_offset = 4,
               .address =
                   bir::MemoryAddress{
                       .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                       .byte_offset = 4,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .base_link_name_id = global_name,
                   },
           },
           bir::StoreGlobalInst{
               .global_name_id = global_name,
               .value = bir::Value::named(bir::TypeKind::I32, "%tail"),
               .byte_offset = 8,
               .address =
                   bir::MemoryAddress{
                       .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                       .byte_offset = 8,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .base_link_name_id = global_name,
                   },
           }},
  };
  const auto* selected_producer = std::get_if<bir::SelectInst>(&block.insts[0]);
  const auto* tail_producer = std::get_if<bir::BinaryInst>(&block.insts[2]);
  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      selected_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind =
              prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization,
          .block_label = c4c::BlockLabelId{23},
          .instruction_index = 0,
          .select = selected_producer,
      });
  source_producers.producers_by_value_name.emplace(
      tail_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = c4c::BlockLabelId{23},
          .instruction_index = 2,
          .binary = tail_producer,
      });
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = 17,
      .value_homes =
          {prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{21},
               .function_name = 17,
               .value_name = selected_name,
               .kind = prepare::PreparedValueHomeKind::StackSlot,
               .slot_id = prepare::PreparedFrameSlotId{51},
               .offset_bytes = std::size_t{16},
               .size_bytes = std::size_t{4},
               .align_bytes = std::size_t{4},
           },
           prepare::PreparedValueHome{
               .value_id = prepare::PreparedValueId{22},
               .function_name = 17,
               .value_name = tail_name,
               .kind = prepare::PreparedValueHomeKind::StackSlot,
               .slot_id = prepare::PreparedFrameSlotId{52},
               .offset_bytes = std::size_t{20},
               .size_bytes = std::size_t{4},
               .align_bytes = std::size_t{4},
           }},
  };
  const prepare::PreparedAddressingFunction addressing{
      .function_name = 17,
      .accesses =
          {prepare::PreparedMemoryAccess{
               .function_name = 17,
               .block_label = 99,
               .inst_index = 1,
               .stored_value_name = tail_name,
               .address =
                   prepare::PreparedAddress{
                       .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                       .symbol_name = global_name,
                       .byte_offset = 12,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .can_use_base_plus_offset = true,
                   },
           },
           prepare::PreparedMemoryAccess{
               .function_name = 17,
               .block_label = 23,
               .inst_index = 1,
               .stored_value_name = selected_name,
               .address =
                   prepare::PreparedAddress{
                       .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                       .symbol_name = global_name,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .can_use_base_plus_offset = true,
                   },
           },
           prepare::PreparedMemoryAccess{
               .function_name = 17,
               .block_label = 23,
               .inst_index = 3,
               .stored_value_name = tail_name,
               .address =
                   prepare::PreparedAddress{
                       .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                       .symbol_name = global_name,
                       .byte_offset = 4,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .can_use_base_plus_offset = true,
                   },
           },
           prepare::PreparedMemoryAccess{
               .function_name = 17,
               .block_label = 23,
               .inst_index = 4,
               .stored_value_name = tail_name,
               .address =
                   prepare::PreparedAddress{
                       .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                       .symbol_name = global_name,
                       .byte_offset = 8,
                       .size_bytes = 4,
                       .align_bytes = 4,
                       .can_use_base_plus_offset = true,
                   },
           }},
  };

  const auto pending = prepare::plan_pending_prepared_store_global_publications(
      &value_locations,
      &addressing,
      c4c::BlockLabelId{23},
      &block,
      1,
      &source_producers);
  if (pending.size() != 3 ||
      pending[0].instruction_index != 1 ||
      pending[1].instruction_index != 3 ||
      pending[2].instruction_index != 4 ||
      !pending[0].store_source.pending_publication ||
      !pending[0].store_source.stack_homes_only ||
      pending[0].store_source.duplicate_publication ||
      pending[0].store_source.source_value_name != selected_name ||
      pending[0].store_source.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization ||
      pending[0].store_source.source_producer_instruction_index !=
          std::optional<std::size_t>{0} ||
      pending[1].store_source.source_value_name != tail_name ||
      pending[1].store_source.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      pending[1].store_source.source_producer_instruction_index !=
          std::optional<std::size_t>{2} ||
      !pending[2].store_source.duplicate_publication ||
      pending[2].store_source.source_value_name != tail_name ||
      pending[2].store_source.source_producer_instruction_index !=
          std::optional<std::size_t>{2}) {
    return fail("expected prepared pending global publication candidates");
  }
  const prepare::PreparedEdgePublicationSourceProducerLookups empty_source_producers;
  const auto missing_producer =
      prepare::plan_pending_prepared_store_global_publications(
          &value_locations,
          &addressing,
          c4c::BlockLabelId{23},
          &block,
          1,
          &empty_source_producers);
  if (!missing_producer.empty()) {
    return fail("expected pending global publication candidates to require producers");
  }
  const auto no_producer_authority =
      prepare::plan_pending_prepared_store_global_publications(
          &value_locations,
          &addressing,
          c4c::BlockLabelId{23},
          &block,
          1,
          nullptr);
  if (!no_producer_authority.empty()) {
    return fail("expected pending global publication candidates to require producer lookup authority");
  }
  auto ambiguous_source_producers = source_producers;
  ambiguous_source_producers.producers_by_value_name[selected_name] =
      prepare::PreparedEdgePublicationSourceProducer{};
  const auto ambiguous_producer =
      prepare::plan_pending_prepared_store_global_publications(
          &value_locations,
          &addressing,
          c4c::BlockLabelId{23},
          &block,
          1,
          &ambiguous_source_producers);
  if (ambiguous_producer.size() != 2 ||
      ambiguous_producer.front().store_source.source_value_name != tail_name) {
    return fail("expected pending global publication candidates to reject ambiguous producers");
  }
  const auto replay = prepare::plan_pending_prepared_store_global_publications(
      &value_locations,
      &addressing,
      c4c::BlockLabelId{23},
      &block,
      3,
      &source_producers);
  if (!replay.empty()) {
    return fail("expected prepared pending global publication replay suppression");
  }

  const auto normal = prepare::plan_prepared_store_global_publication(
      &value_locations,
      &addressing,
      c4c::BlockLabelId{23},
      *std::get_if<bir::StoreGlobalInst>(&block.insts[1]),
      1,
      false,
      false);
  if (!prepare::prepared_store_source_publication_available(normal) ||
      normal.pending_publication ||
      normal.stack_homes_only ||
      !normal.duplicate_publication ||
      normal.source_home_kind != prepare::PreparedValueHomeKind::StackSlot) {
    return fail("expected prepared duplicate global stack-home publication plan");
  }
  const auto wrong_block = prepare::plan_prepared_store_global_publication(
      &value_locations,
      &addressing,
      c4c::BlockLabelId{99},
      *std::get_if<bir::StoreGlobalInst>(&block.insts[1]),
      1,
      false,
      false);
  if (!prepare::prepared_store_source_publication_available(wrong_block) ||
      wrong_block.source_value_name != tail_name ||
      wrong_block.source_home != &value_locations.value_homes[1]) {
    return fail("expected store-global publication lookup to honor block label");
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
  if (int rc = records_fixed_formal_store_source_from_prepared_publication_plan();
      rc != 0) {
    return rc;
  }
  if (int rc = records_recovered_source_without_target_policy(); rc != 0) {
    return rc;
  }
  if (int rc = recovered_narrow_store_requires_prepared_memory_access(); rc != 0) {
    return rc;
  }
  if (int rc = classifies_byval_load_local_source_from_prepared_authority(); rc != 0) {
    return rc;
  }
  if (int rc = records_direct_global_select_chain_dependency_from_prepared_authority();
      rc != 0) {
    return rc;
  }
  if (int rc = finds_unpublished_load_local_source_from_indexed_authority();
      rc != 0) {
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
  if (int rc = plans_pending_global_publication_candidates_from_prepared_state();
      rc != 0) {
    return rc;
  }
  return rejects_incomplete_inputs();
}
