#include "prepared_edge_publication_emit.hpp"

#include "prepared_frame_emit.hpp"

#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/prepared_contract_verifier.hpp"

#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

namespace {

void clear_stack_source_intent(EdgePublicationMoveIntent& intent) {
  intent.source_stack_slot_id.reset();
  intent.source_stack_offset_bytes.reset();
  intent.source_stack_size_bytes.reset();
  intent.source_stack_align_bytes.reset();
  intent.source_stack_extension_policy =
      c4c::backend::prepare::PreparedTypedStackSourceExtensionPolicy::None;
  intent.source_memory_base_value_id.reset();
  intent.source_memory_base_register.clear();
  intent.source_memory_byte_offset.reset();
  intent.source_memory_size_bytes.reset();
  intent.source_memory_align_bytes.reset();
}

void copy_same_width_i32_stack_source_publication(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedTypedStackSourcePublication& typed) {
  intent.source_value_id = typed.source_value_id;
  intent.destination_value_id = typed.destination_value_id;
  intent.source_type = typed.source_type;
  intent.destination_type = typed.destination_type;
  intent.source_stack_slot_id = typed.source_slot_id;
  intent.source_stack_offset_bytes = typed.source_stack_offset_bytes;
  intent.source_stack_size_bytes = typed.source_stack_size_bytes;
  intent.source_stack_align_bytes = typed.source_stack_align_bytes;
  intent.source_stack_extension_policy = typed.extension_policy;
  intent.destination_register_bank = typed.destination_register_bank;
  intent.destination_register_placement = typed.destination_register_placement;
}

std::optional<std::string> riscv_gpr_register_name_from_placement(
    const c4c::backend::prepare::PreparedRegisterPlacement& placement) {
  namespace prepare = c4c::backend::prepare;

  if (placement.bank != prepare::PreparedRegisterBank::Gpr ||
      placement.contiguous_width != 1) {
    return std::nullopt;
  }

  switch (placement.pool) {
    case prepare::PreparedRegisterSlotPool::CallerSaved:
      if (placement.slot_index == 0) {
        return std::string{"t0"};
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::CalleeSaved:
      if (placement.slot_index < 2) {
        return std::string{"s"} + std::to_string(placement.slot_index + 1);
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::CallArgument:
      if (placement.slot_index < 8) {
        return std::string{"a"} + std::to_string(placement.slot_index);
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::CallResult:
      if (placement.slot_index == 0) {
        return std::string{"a0"};
      }
      return std::nullopt;
    case prepare::PreparedRegisterSlotPool::ReservedScratch:
    case prepare::PreparedRegisterSlotPool::None:
      return std::nullopt;
  }
  return std::nullopt;
}


std::optional<std::string> render_edge_publication_source_operand(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const c4c::backend::prepare::PreparedValueHome& source_home) {
  namespace prepare = c4c::backend::prepare;

  if (source_home.kind == prepare::PreparedValueHomeKind::Register &&
      source_home.register_name.has_value()) {
    intent.source_register = *source_home.register_name;
    return intent.source_register;
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::RematerializableImmediate &&
      source_home.immediate_i32.has_value()) {
    intent.source_immediate_i32 = *source_home.immediate_i32;
    return std::to_string(*source_home.immediate_i32);
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      source_home.offset_bytes.has_value() &&
      (source_home.size_bytes == std::optional<std::size_t>{4} ||
       source_home.size_bytes == std::optional<std::size_t>{8})) {
    intent.source_stack_slot_id = source_home.slot_id;
    intent.source_stack_offset_bytes = *source_home.offset_bytes;
    intent.source_stack_size_bytes = *source_home.size_bytes;
    return std::to_string(*source_home.offset_bytes) + "(sp)";
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      !source_home.offset_bytes.has_value() &&
      source_home.size_bytes == std::optional<std::size_t>{4} &&
      publication.source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      publication.source_memory_access_status ==
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available &&
      publication.source_memory_access != nullptr &&
      publication.source_value.type == c4c::backend::bir::TypeKind::I32 &&
      publication.destination_value.type == c4c::backend::bir::TypeKind::I32 &&
      publication.source_memory_base_kind ==
          prepare::PreparedAddressBaseKind::PointerValue &&
      publication.source_memory_pointer_value_name.has_value() &&
      publication.source_memory_size_bytes == 4 &&
      publication.source_memory_align_bytes >= 4 &&
      publication.source_memory_address_space ==
          c4c::backend::bir::AddressSpace::Default &&
      !publication.source_memory_is_volatile &&
      publication.source_memory_can_use_base_plus_offset &&
      !publication.source_memory_requires_address_materialization &&
      fits_signed_12_bit_immediate(publication.source_memory_byte_offset) &&
      publication.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      publication.move != nullptr &&
      publication.source_value_id.has_value() &&
      publication.move->authority_kind ==
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
      publication.move->destination_kind ==
          prepare::PreparedMoveDestinationKind::Value &&
      publication.move->op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
      publication.move->from_value_id == *publication.source_value_id &&
      publication.move->to_value_id == publication.destination_value_id &&
      publication.move->destination_register_placement.has_value()) {
    const auto* indexed_access =
        prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
            &lookups->memory_accesses, *publication.source_value_id);
    if (indexed_access == nullptr ||
        indexed_access != publication.source_memory_access) {
      return std::nullopt;
    }
    const auto base_id_it =
        lookups->value_homes.value_ids.find(
            *publication.source_memory_pointer_value_name);
    if (base_id_it == lookups->value_homes.value_ids.end()) {
      return std::nullopt;
    }
    const auto base_home_it =
        lookups->value_homes.homes_by_id.find(base_id_it->second);
    if (base_home_it == lookups->value_homes.homes_by_id.end() ||
        base_home_it->second == nullptr ||
        base_home_it->second->kind != prepare::PreparedValueHomeKind::Register ||
        !base_home_it->second->register_name.has_value()) {
      return std::nullopt;
    }
    intent.source_stack_slot_id = source_home.slot_id;
    intent.source_stack_size_bytes = source_home.size_bytes;
    intent.source_stack_align_bytes = source_home.align_bytes;
    intent.source_stack_extension_policy =
        prepare::PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension;
    intent.source_memory_base_value_id = base_id_it->second;
    intent.source_memory_base_register = *base_home_it->second->register_name;
    intent.source_memory_byte_offset = publication.source_memory_byte_offset;
    intent.source_memory_size_bytes = publication.source_memory_size_bytes;
    intent.source_memory_align_bytes = publication.source_memory_align_bytes;
    intent.destination_register_placement =
        publication.move->destination_register_placement;
    intent.destination_register_bank =
        intent.destination_register_placement->bank;
    return std::to_string(publication.source_memory_byte_offset) + "(" +
           intent.source_memory_base_register + ")";
  }
  if (source_home.kind == prepare::PreparedValueHomeKind::PointerBasePlusOffset) {
    const auto pointer_report =
        prepare::verify_prepared_pointer_base_plus_offset_contract(&source_home);
    const auto pointer_fact =
        prepare::as_pointer_base_plus_offset_fact(source_home);
    if (pointer_report.fail_closed || !pointer_fact.has_value()) {
      return std::nullopt;
    }
    const auto base_id_it =
        lookups->value_homes.value_ids.find(pointer_fact->base_value_name);
    if (base_id_it == lookups->value_homes.value_ids.end()) {
      return std::nullopt;
    }
    const auto base_home_it = lookups->value_homes.homes_by_id.find(base_id_it->second);
    if (base_home_it == lookups->value_homes.homes_by_id.end() ||
        base_home_it->second == nullptr ||
        base_home_it->second->kind != prepare::PreparedValueHomeKind::Register ||
        !base_home_it->second->register_name.has_value()) {
      return std::nullopt;
    }
    intent.source_pointer_base_value_id = base_id_it->second;
    intent.source_pointer_base_register = *base_home_it->second->register_name;
    intent.source_pointer_byte_delta = pointer_fact->byte_delta;
    return std::to_string(pointer_fact->byte_delta);
  }
  return std::nullopt;
}

bool has_direct_register_source_for_stack_destination(
    const EdgePublicationMoveIntent& intent) {
  return !intent.source_register.empty() &&
         !intent.source_immediate_i32.has_value() &&
         !intent.source_stack_offset_bytes.has_value() &&
         !intent.source_pointer_byte_delta.has_value();
}

bool has_rematerializable_i32_source_for_stack_destination(
    const EdgePublicationMoveIntent& intent) {
  return intent.source_register.empty() &&
         intent.source_immediate_i32.has_value() &&
         !intent.source_stack_offset_bytes.has_value() &&
         !intent.source_pointer_byte_delta.has_value();
}

bool stack_ranges_overlap(std::size_t lhs_offset,
                          std::size_t lhs_size,
                          std::size_t rhs_offset,
                          std::size_t rhs_size) {
  return lhs_offset < rhs_offset + rhs_size && rhs_offset < lhs_offset + lhs_size;
}

bool has_non_aliasing_i32_stack_source_for_stack_destination(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedValueHome& destination_home) {
  if (!intent.source_register.empty() ||
      intent.source_immediate_i32.has_value() ||
      !intent.source_stack_offset_bytes.has_value() ||
      intent.source_stack_size_bytes != std::optional<std::size_t>{4} ||
      intent.source_pointer_byte_delta.has_value() ||
      !fits_signed_12_bit_load_offset(*intent.source_stack_offset_bytes) ||
      !destination_home.offset_bytes.has_value() ||
      destination_home.size_bytes != std::optional<std::size_t>{4}) {
    return false;
  }
  if (intent.source_stack_slot_id.has_value() &&
      destination_home.slot_id.has_value() &&
      intent.source_stack_slot_id == destination_home.slot_id) {
    return false;
  }
  return !stack_ranges_overlap(*intent.source_stack_offset_bytes,
                               *intent.source_stack_size_bytes,
                               *destination_home.offset_bytes,
                               *destination_home.size_bytes);
}

bool has_existing_concrete_i64_stack_source_register_policy(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) {
  namespace bir = c4c::backend::bir;

  return publication.source_value.type == bir::TypeKind::I64 &&
         publication.destination_value.type == bir::TypeKind::I64 &&
         intent.source_stack_offset_bytes.has_value() &&
         intent.source_stack_size_bytes == std::optional<std::size_t>{8};
}

bool has_select_publication_pointer_stack_source_register_policy(
    const EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) {
  namespace bir = c4c::backend::bir;
  namespace prepare = c4c::backend::prepare;

  return publication.carrier_kind ==
             prepare::PreparedJoinTransferCarrierKind::SelectMaterialization &&
         publication.source_value.type == bir::TypeKind::Ptr &&
         publication.destination_value.type == bir::TypeKind::Ptr &&
         publication.destination_storage_kind ==
             prepare::PreparedMoveStorageKind::Register &&
         publication.move != nullptr &&
         publication.move->authority_kind ==
             prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
         publication.move->destination_kind ==
             prepare::PreparedMoveDestinationKind::Value &&
         publication.move->op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         publication.source_value_id.has_value() &&
         publication.move->from_value_id == *publication.source_value_id &&
         publication.move->to_value_id == publication.destination_value_id &&
         intent.source_stack_offset_bytes.has_value() &&
         intent.source_stack_size_bytes == std::optional<std::size_t>{8};
}

bool is_supported_direct_register_to_stack_publication_size(
    std::optional<std::size_t> size_bytes) {
  return size_bytes == std::optional<std::size_t>{1} ||
         size_bytes == std::optional<std::size_t>{2} ||
         size_bytes == std::optional<std::size_t>{4};
}

std::optional<std::string> rv64_store_mnemonic_for_stack_publication_size(
    std::optional<std::size_t> size_bytes) {
  if (size_bytes == std::optional<std::size_t>{1}) {
    return std::string{"sb"};
  }
  if (size_bytes == std::optional<std::size_t>{2}) {
    return std::string{"sh"};
  }
  if (size_bytes == std::optional<std::size_t>{4}) {
    return std::string{"sw"};
  }
  return std::nullopt;
}

bir::Route5PublicationSourceKind route5_source_kind_from_prepared(
    c4c::backend::prepare::PreparedEdgePublicationSourceProducerKind kind) {
  namespace prepare = c4c::backend::prepare;

  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return bir::Route5PublicationSourceKind::Immediate;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::Route5PublicationSourceKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::Route5PublicationSourceKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::Route5PublicationSourceKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::Route5PublicationSourceKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::Route5PublicationSourceKind::SelectMaterialization;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return bir::Route5PublicationSourceKind::Unknown;
  }
  return bir::Route5PublicationSourceKind::Unknown;
}

bool route3_base_kind_agrees_with_prepared_source_memory(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const bir::Route3MemoryAccessRecord& route3_access) {
  namespace prepare = c4c::backend::prepare;

  switch (publication.source_memory_base_kind) {
    case prepare::PreparedAddressBaseKind::FrameSlot:
      return route3_access.base_kind ==
                 bir::Route3MemoryAccessBaseKind::LocalSlot &&
             publication.source_memory_frame_slot_id.has_value() &&
             route3_access.local_slot_id != c4c::kInvalidSlotName;
    case prepare::PreparedAddressBaseKind::GlobalSymbol:
      return route3_access.base_kind ==
                 bir::Route3MemoryAccessBaseKind::GlobalSymbol &&
             publication.source_memory_symbol_name.has_value() &&
             route3_access.global_name_id == *publication.source_memory_symbol_name;
    case prepare::PreparedAddressBaseKind::PointerValue:
      if (route3_access.base_kind !=
              bir::Route3MemoryAccessBaseKind::PointerValue ||
          !publication.source_memory_pointer_value_name.has_value() ||
          !route3_access.pointer_value) {
        return false;
      }
      return route3_access.pointer_value.name_id == c4c::kInvalidValueName ||
             route3_access.pointer_value.name_id ==
                 *publication.source_memory_pointer_value_name;
    case prepare::PreparedAddressBaseKind::StringConstant:
      return route3_access.base_kind ==
                 bir::Route3MemoryAccessBaseKind::StringConstant &&
             publication.source_memory_symbol_name.has_value() &&
             route3_access.string_constant_name_id ==
                 *publication.source_memory_symbol_name;
    case prepare::PreparedAddressBaseKind::None:
      return route3_access.base_kind == bir::Route3MemoryAccessBaseKind::None;
  }
  return false;
}

bool route3_source_memory_agrees_with_prepared_publication(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const bir::Route3MemoryAccessRecord& route3_access) {
  namespace prepare = c4c::backend::prepare;

  if (publication.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      publication.source_memory_access_status !=
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available ||
      publication.source_memory_access == nullptr ||
      !route3_access ||
      route3_access.node_kind != bir::Route3MemoryAccessNodeKind::LoadLocal ||
      !route3_access.result_value ||
      route3_access.result_value.value_kind != publication.source_value.kind ||
      route3_access.result_value.type != publication.source_value.type ||
      route3_access.result_value.name != publication.source_value.name ||
      route3_access.address_space != publication.source_memory_address_space ||
      route3_access.is_volatile != publication.source_memory_is_volatile ||
      route3_access.byte_offset != publication.source_memory_byte_offset ||
      route3_access.size_bytes != publication.source_memory_size_bytes ||
      route3_access.align_bytes != publication.source_memory_align_bytes) {
    return false;
  }

  return route3_base_kind_agrees_with_prepared_source_memory(publication,
                                                            route3_access);
}

bool route5_edge_source_agrees_with_prepared_publication(
    const c4c::backend::prepare::PreparedEdgePublication& publication,
    const bir::Route5CfgEdgePublicationRecord& route5_edge) {
  namespace prepare = c4c::backend::prepare;

  if ((route5_edge.status != bir::Route5PublicationStatus::Available &&
       route5_edge.status != bir::Route5PublicationStatus::MemorySource) ||
      !route5_edge ||
      route5_edge.predecessor_label_id != publication.predecessor_label ||
      route5_edge.successor_label_id != publication.successor_label ||
      route5_edge.destination_value_name != publication.destination_value.name ||
      route5_edge.destination_value_type != publication.destination_value.type ||
      route5_edge.source_value_kind != publication.source_value_kind ||
      route5_edge.source_value_type != publication.source_value.type) {
    return false;
  }

  const bool comparable_prepared_producer =
      publication.source_producer_kind !=
      prepare::PreparedEdgePublicationSourceProducerKind::Unknown;
  if (comparable_prepared_producer &&
      route5_edge.source_producer_kind !=
          route5_source_kind_from_prepared(publication.source_producer_kind)) {
    return false;
  }

  if (publication.source_value_kind == bir::Value::Kind::Immediate) {
    return route5_edge.source_value.value_kind == bir::Value::Kind::Immediate &&
           route5_edge.source_value.integer_constant.has_value() &&
           route5_edge.source_value.integer_constant ==
               publication.source_value.immediate;
  }

  if (publication.source_value_kind != bir::Value::Kind::Named ||
      route5_edge.source_value_name != publication.source_value.name ||
      route5_edge.source_producer_instruction == nullptr ||
      !route5_edge.source_producer_instruction_index.has_value()) {
    return false;
  }

  if (publication.source_producer_kind ==
      prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) {
    return route5_edge.status == bir::Route5PublicationStatus::MemorySource &&
           route5_edge.source_memory_identity_available &&
           route3_source_memory_agrees_with_prepared_publication(
               publication, route5_edge.source_memory_access);
  }

  return !comparable_prepared_producer ||
         route5_edge.status == bir::Route5PublicationStatus::Available;
}

struct RiscvEdgePublicationMoveAdapter {
  const c4c::backend::prepare::PreparedFunctionLookups* lookups = nullptr;
  c4c::BlockLabelId predecessor_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId successor_label = c4c::kInvalidBlockLabel;
  c4c::backend::prepare::PreparedValueId destination_value_id = 0;
  const bir::Route5CfgEdgePublicationRecord* route5_edge = nullptr;

  [[nodiscard]] EdgePublicationMoveIntent consume_prepared_backed_move_intent() const;

 private:
  [[nodiscard]] const c4c::backend::prepare::PreparedEdgePublication*
  find_prepared_publication() const;

  [[nodiscard]] std::optional<std::string> render_prepared_source_operand(
      EdgePublicationMoveIntent& intent,
      const c4c::backend::prepare::PreparedEdgePublication& publication) const;

  void attach_route5_edge_agreement(
      EdgePublicationMoveIntent& intent,
      const c4c::backend::prepare::PreparedEdgePublication& publication) const;
};

const c4c::backend::prepare::PreparedEdgePublication*
RiscvEdgePublicationMoveAdapter::find_prepared_publication() const {
  return c4c::backend::prepare::find_unique_indexed_prepared_edge_publication(
      &lookups->edge_publications, predecessor_label, successor_label,
      destination_value_id);
}

std::optional<std::string>
RiscvEdgePublicationMoveAdapter::render_prepared_source_operand(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) const {
  return render_edge_publication_source_operand(intent,
                                                lookups,
                                                publication,
                                                *publication.source_home);
}

void RiscvEdgePublicationMoveAdapter::attach_route5_edge_agreement(
    EdgePublicationMoveIntent& intent,
    const c4c::backend::prepare::PreparedEdgePublication& publication) const {
  if (route5_edge == nullptr) {
    return;
  }
  intent.route5_edge_status = route5_edge->status;
  intent.route5_edge_source_agrees =
      route5_edge_source_agrees_with_prepared_publication(publication, *route5_edge);
  if (route5_edge->source_memory_identity_available) {
    intent.route3_source_memory_agrees =
        route3_source_memory_agrees_with_prepared_publication(
            publication, route5_edge->source_memory_access);
  }
}

EdgePublicationMoveIntent
RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent() const {
  namespace prepare = c4c::backend::prepare;

  if (lookups == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingSharedLookups,
    };
  }

  const auto* publication = find_prepared_publication();
  if (publication == nullptr) {
    return EdgePublicationMoveIntent{
        .status = EdgePublicationMoveIntentStatus::MissingPublication,
    };
  }

  EdgePublicationMoveIntent intent{
      .status = EdgePublicationMoveIntentStatus::UnsupportedPublication,
      .publication = publication,
      .destination_value_id = publication->destination_value_id,
      .source_type = publication->source_value.type,
      .destination_type = publication->destination_value.type,
  };
  if (publication->source_value_id.has_value()) {
    intent.source_value_id = *publication->source_value_id;
  }
  attach_route5_edge_agreement(intent, *publication);

  if (publication->status != prepare::PreparedEdgePublicationLookupStatus::Available ||
      publication->move == nullptr ||
      publication->move->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return intent;
  }
  if (route5_edge != nullptr &&
      publication->source_producer_kind ==
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal &&
      publication->source_memory_access_status ==
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available &&
      publication->source_memory_access != nullptr &&
      !intent.route5_edge_source_agrees) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
    return intent;
  }
  std::optional<std::string> source_operand;
  if (publication->source_value_kind == bir::Value::Kind::Immediate &&
      publication->move->source_immediate_i32.has_value()) {
    if (publication->source_value.immediate !=
        *publication->move->source_immediate_i32) {
      intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
      return intent;
    }
    intent.source_immediate_i32 = *publication->move->source_immediate_i32;
  } else {
    if (publication->source_home == nullptr) {
      intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
      return intent;
    }
    source_operand = render_prepared_source_operand(intent, *publication);
    if (!source_operand.has_value()) {
      intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
      return intent;
    }
  }
  if (publication->destination_home == nullptr) {
    intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
    return intent;
  }

  const auto& destination_home = *publication->destination_home;
  if (destination_home.kind == prepare::PreparedValueHomeKind::Register) {
    intent.status = EdgePublicationMoveIntentStatus::Available;
    if (intent.source_immediate_i32.has_value()) {
      if (!destination_home.register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.destination_register = *destination_home.register_name;
      intent.instruction_text =
          "li " + intent.destination_register + ", " +
          std::to_string(*intent.source_immediate_i32);
    } else if (intent.source_stack_offset_bytes.has_value()) {
      const auto typed =
          prepare::prepare_same_width_i32_stack_source_publication(publication);
      if (typed.status !=
          prepare::PreparedTypedStackSourcePublicationStatus::Available) {
        if (has_existing_concrete_i64_stack_source_register_policy(intent,
                                                                   *publication) &&
            destination_home.register_name.has_value()) {
          intent.destination_register = *destination_home.register_name;
          if (fits_signed_12_bit_load_offset(*intent.source_stack_offset_bytes)) {
            intent.instruction_text =
                "ld " + intent.destination_register + ", " + *source_operand;
          } else {
            const auto offset_text = std::to_string(*intent.source_stack_offset_bytes);
            intent.instruction_text =
                "li t6, " + offset_text +
                "\n    add t6, sp, t6" +
                "\n    ld " + intent.destination_register + ", 0(t6)";
          }
          return intent;
        }
        if (has_select_publication_pointer_stack_source_register_policy(
                intent,
                *publication) &&
            destination_home.register_name.has_value()) {
          intent.destination_register = *destination_home.register_name;
          intent.destination_register_bank = prepare::PreparedRegisterBank::Gpr;
          if (publication->move->destination_register_placement.has_value()) {
            intent.destination_register_placement =
                publication->move->destination_register_placement;
          }
          if (fits_signed_12_bit_load_offset(*intent.source_stack_offset_bytes)) {
            intent.instruction_text =
                "ld " + intent.destination_register + ", " + *source_operand;
          } else {
            const auto offset_text = std::to_string(*intent.source_stack_offset_bytes);
            intent.instruction_text =
                "li t6, " + offset_text +
                "\n    add t6, sp, t6" +
                "\n    ld " + intent.destination_register + ", 0(t6)";
          }
          return intent;
        }
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedSourceHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
        clear_stack_source_intent(intent);
        return intent;
      }
      copy_same_width_i32_stack_source_publication(intent, typed);
      const auto register_name =
          riscv_gpr_register_name_from_placement(*typed.destination_register_placement);
      if (!register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
        clear_stack_source_intent(intent);
        return intent;
      }
      intent.destination_register = *register_name;
      if (fits_signed_12_bit_load_offset(*typed.source_stack_offset_bytes)) {
        intent.instruction_text =
            "lw " + intent.destination_register + ", " +
            std::to_string(*typed.source_stack_offset_bytes) + "(sp)";
      } else {
        const auto offset_text = std::to_string(*typed.source_stack_offset_bytes);
        intent.instruction_text =
            "li t6, " + offset_text +
            "\n    add t6, sp, t6" +
            "\n    lw " + intent.destination_register + ", 0(t6)";
      }
    } else if (intent.source_memory_byte_offset.has_value()) {
      const auto register_name =
          riscv_gpr_register_name_from_placement(*intent.destination_register_placement);
      if (!register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
        clear_stack_source_intent(intent);
        return intent;
      }
      intent.destination_register = *register_name;
      intent.instruction_text =
          "lw " + intent.destination_register + ", " + *source_operand;
    } else if (intent.source_pointer_byte_delta.has_value()) {
      if (!destination_home.register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.destination_register = *destination_home.register_name;
      if (*intent.source_pointer_byte_delta == 0) {
        if (intent.destination_register != intent.source_pointer_base_register) {
          intent.instruction_text =
              "mv " + intent.destination_register + ", " +
              intent.source_pointer_base_register;
        }
      } else if (fits_signed_12_bit_immediate(*intent.source_pointer_byte_delta)) {
        intent.instruction_text =
            "addi " + intent.destination_register + ", " +
            intent.source_pointer_base_register + ", " + *source_operand;
      } else if (intent.destination_register != intent.source_pointer_base_register) {
        intent.instruction_text =
            "li " + intent.destination_register + ", " + *source_operand +
            "\n    add " + intent.destination_register + ", " +
            intent.source_pointer_base_register + ", " + intent.destination_register;
      } else {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        intent.destination_register.clear();
        intent.instruction_text.clear();
      }
    } else {
      if (!destination_home.register_name.has_value()) {
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.destination_register = *destination_home.register_name;
      if (intent.destination_register != intent.source_register) {
        intent.instruction_text =
            "mv " + intent.destination_register + ", " + intent.source_register;
      }
    }
    return intent;
  }

  // Prepared edge-publication stack destinations have a target-local scratch
  // contract. The direct Register -> StackSlot case reserves no scratch and
  // clobbers only the destination memory slot. The I32 immediate materializing
  // form may own `t0` as a value scratch for one publication sequence,
  // clobbering it before the final `sw`; the I32 StackSlot -> StackSlot form
  // uses the same `t0` scratch for a signed-12-bit source load before the final
  // store. The scratch value must not survive across edge publications.
  // `t1`/`t2` are not reserved by this path, and `t5`/`t6` remain available
  // only to a later explicit address/large-offset helper contract. Pointer
  // sources and large stack-source offsets to StackSlot destinations
  // intentionally remain fail-closed.
  if (destination_home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      destination_home.offset_bytes.has_value() &&
      is_supported_direct_register_to_stack_publication_size(
          destination_home.size_bytes) &&
      fits_signed_12_bit_load_offset(*destination_home.offset_bytes)) {
    intent.status = EdgePublicationMoveIntentStatus::Available;
    intent.destination_stack_slot_id = destination_home.slot_id;
    intent.destination_stack_offset_bytes = *destination_home.offset_bytes;
    intent.destination_stack_size_bytes = *destination_home.size_bytes;
    if (has_direct_register_source_for_stack_destination(intent)) {
      const auto store_mnemonic =
          rv64_store_mnemonic_for_stack_publication_size(
              destination_home.size_bytes);
      if (!store_mnemonic.has_value()) {
        intent.destination_stack_slot_id.reset();
        intent.destination_stack_offset_bytes.reset();
        intent.destination_stack_size_bytes.reset();
        intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
        return intent;
      }
      intent.instruction_text =
          *store_mnemonic + " " + intent.source_register + ", " +
          std::to_string(*destination_home.offset_bytes) + "(sp)";
      return intent;
    }
    if (has_rematerializable_i32_source_for_stack_destination(intent)) {
      intent.instruction_text =
          "li t0, " + std::to_string(*intent.source_immediate_i32) +
          "\n    sw t0, " + std::to_string(*destination_home.offset_bytes) + "(sp)";
      return intent;
    }
    if (has_non_aliasing_i32_stack_source_for_stack_destination(intent,
                                                               destination_home)) {
      intent.instruction_text =
          "lw t0, " + std::to_string(*intent.source_stack_offset_bytes) +
          "(sp)\n    sw t0, " +
          std::to_string(*destination_home.offset_bytes) + "(sp)";
      return intent;
    }
    intent.destination_stack_slot_id.reset();
    intent.destination_stack_offset_bytes.reset();
    intent.destination_stack_size_bytes.reset();
  }

  intent.status = EdgePublicationMoveIntentStatus::UnsupportedDestinationHome;
  return intent;
}


}  // namespace

EdgePublicationMoveIntent consume_edge_publication_move_intent(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  return consume_edge_publication_move_intent(lookups,
                                              predecessor_label,
                                              successor_label,
                                              destination_value_id,
                                              nullptr);
}

EdgePublicationMoveIntent consume_edge_publication_move_intent(
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id,
    const bir::Route5CfgEdgePublicationRecord* route5_edge) {
  const RiscvEdgePublicationMoveAdapter adapter{
      .lookups = lookups,
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .destination_value_id = destination_value_id,
      .route5_edge = route5_edge,
  };
  return adapter.consume_prepared_backed_move_intent();
}

EdgePublicationMoveIntent append_edge_publication_move_instruction(
    std::string& output,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    c4c::backend::prepare::PreparedValueId destination_value_id) {
  auto intent = consume_edge_publication_move_intent(
      lookups, predecessor_label, successor_label, destination_value_id);
  if (intent.status == EdgePublicationMoveIntentStatus::Available &&
      !intent.instruction_text.empty()) {
    output += "    " + intent.instruction_text + "\n";
  }
  return intent;
}

}  // namespace c4c::backend::riscv::codegen
