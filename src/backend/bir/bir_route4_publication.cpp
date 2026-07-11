#include "bir.hpp"

#include <algorithm>
#include <string_view>

namespace c4c::backend::bir {

[[nodiscard]] bool route4_record_matches_block(
    const Function& function,
    const Block* record_block,
    std::string_view record_label,
    BlockLabelId record_label_id,
    const Block& block);

Route4PublicationSourceKind route4_publication_source_kind(
    Route1ProducerKind kind) {
  switch (kind) {
    case Route1ProducerKind::Immediate:
      return Route4PublicationSourceKind::Immediate;
    case Route1ProducerKind::LoadLocal:
      return Route4PublicationSourceKind::LoadLocal;
    case Route1ProducerKind::LoadGlobal:
      return Route4PublicationSourceKind::LoadGlobal;
    case Route1ProducerKind::Cast:
      return Route4PublicationSourceKind::Cast;
    case Route1ProducerKind::Binary:
      return Route4PublicationSourceKind::Binary;
    case Route1ProducerKind::SelectMaterialization:
      return Route4PublicationSourceKind::SelectMaterialization;
    case Route1ProducerKind::Unknown:
      return Route4PublicationSourceKind::Unknown;
  }
  return Route4PublicationSourceKind::Unknown;
}

Route4CurrentBlockPublicationRecord route4_current_block_publication_record(
    Route1SameBlockProducerQuery query,
    const Value& value,
    ValueNameId value_name_id) {
  Route4CurrentBlockPublicationRecord record{
      .status = Route4PublicationAvailabilityStatus::Unavailable,
      .value = route1_source_value_identity(value, value_name_id),
      .value_name = value.kind == Value::Kind::Named ? std::string_view{value.name}
                                                     : std::string_view{},
      .value_name_id = value_name_id,
      .value_type = value.type,
      .before_instruction_index = query.before_instruction_index,
  };
  if (!query || query.index->block == nullptr) {
    record.status = Route4PublicationAvailabilityStatus::MissingBlock;
    return record;
  }
  const auto& block = *query.index->block;
  record.block = &block;
  record.block_label = block.label;
  record.block_label_id = block.label_id;
  if (value.kind != Value::Kind::Named || value.name.empty()) {
    record.status = Route4PublicationAvailabilityStatus::MissingValue;
    return record;
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->record == nullptr ||
      producer->instruction == nullptr ||
      producer->produced_value == nullptr) {
    record.status = Route4PublicationAvailabilityStatus::NoMatch;
    return record;
  }
  record.available = true;
  record.status = Route4PublicationAvailabilityStatus::Available;
  record.source_producer_kind =
      route4_publication_source_kind(producer->record->kind);
  record.source_producer_instruction = producer->instruction;
  record.source_producer_instruction_index = producer->instruction_index;
  record.source_producer_block_label_id =
      producer->record->producer_instruction.block_label_id;
  record.produced_value =
      route1_source_value_identity(*producer->produced_value, value_name_id);
  return record;
}

Route4BlockEntryPublicationRecord route4_block_entry_publication_record(
    const Block* successor_block,
    const Value& destination_value,
    ValueNameId destination_value_name_id) {
  Route4BlockEntryPublicationRecord record{
      .status = Route4PublicationAvailabilityStatus::Unavailable,
      .destination_value = route1_source_value_identity(
          destination_value, destination_value_name_id),
      .destination_value_name =
          destination_value.kind == Value::Kind::Named
              ? std::string_view{destination_value.name}
              : std::string_view{},
      .destination_value_name_id = destination_value_name_id,
      .destination_value_type = destination_value.type,
  };
  if (successor_block == nullptr) {
    record.status = Route4PublicationAvailabilityStatus::MissingBlock;
    return record;
  }
  record.successor_block = successor_block;
  record.successor_label = successor_block->label;
  record.successor_label_id = successor_block->label_id;
  if (destination_value.kind != Value::Kind::Named ||
      destination_value.name.empty()) {
    record.status = Route4PublicationAvailabilityStatus::MissingValue;
    return record;
  }
  for (std::size_t instruction_index = 0;
       instruction_index < successor_block->insts.size();
       ++instruction_index) {
    const auto& inst = successor_block->insts[instruction_index];
    const auto* phi = std::get_if<PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    if (phi->result.kind != Value::Kind::Named ||
        phi->result.name != destination_value.name) {
      continue;
    }
    if (phi->result.type != destination_value.type) {
      record.status = Route4PublicationAvailabilityStatus::NoMatch;
      return record;
    }
    record.available = true;
    record.status = Route4PublicationAvailabilityStatus::Available;
    record.destination_instruction = &inst;
    record.phi = phi;
    record.destination_instruction_index = instruction_index;
    record.destination_value =
        route1_source_value_identity(phi->result, destination_value_name_id);
    record.destination_value_name = phi->result.name;
    record.destination_value_type = phi->result.type;
    if (phi->incomings.size() == 1U) {
      record.source_value = route1_source_value_identity(phi->incomings.front().value);
    }
    return record;
  }
  record.status = Route4PublicationAvailabilityStatus::MissingPublication;
  return record;
}

Route4PublicationValueRecord route4_current_block_publication_value_record(
    Route1SameBlockProducerQuery query,
    const Value& value,
    ValueNameId value_name_id) {
  const auto publication =
      route4_current_block_publication_record(query, value, value_name_id);
  return Route4PublicationValueRecord{
      .available = publication.available,
      .scope = Route4PublicationScope::CurrentBlock,
      .status = publication.status,
      .value_role = Route4PublicationValueRole::Produced,
      .value = publication.available ? publication.produced_value
                                     : publication.value,
      .block_label = publication.block_label,
      .block_label_id = publication.block_label_id,
      .instruction_index = publication.source_producer_instruction_index,
      .current_block = publication,
  };
}

Route4PublicationValueRecord route4_block_entry_publication_value_record(
    const Block* successor_block,
    const Value& destination_value,
    ValueNameId destination_value_name_id) {
  const auto publication =
      route4_block_entry_publication_record(successor_block,
                                            destination_value,
                                            destination_value_name_id);
  return Route4PublicationValueRecord{
      .available = publication.available,
      .scope = Route4PublicationScope::BlockEntry,
      .status = publication.status,
      .value_role = Route4PublicationValueRole::Consumed,
      .value = publication.destination_value,
      .block_label = publication.successor_label,
      .block_label_id = publication.successor_label_id,
      .instruction_index = publication.destination_instruction_index,
      .block_entry = publication,
  };
}

namespace {

[[nodiscard]] bool route4_index_contains_block(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block) {
  if (index.function == nullptr) {
    return false;
  }
  return std::any_of(index.function->blocks.begin(),
                     index.function->blocks.end(),
                     [&](const Block& indexed_block) {
                       return &indexed_block == &block;
                     });
}

[[nodiscard]] RouteIndexValidationStatus route4_reference_status(
    Route4PublicationAvailabilityStatus status) {
  switch (status) {
    case Route4PublicationAvailabilityStatus::Available:
      return RouteIndexValidationStatus::Valid;
    case Route4PublicationAvailabilityStatus::MissingBlock:
    case Route4PublicationAvailabilityStatus::MissingValue:
    case Route4PublicationAvailabilityStatus::MissingPublication:
      return RouteIndexValidationStatus::MissingRecord;
    case Route4PublicationAvailabilityStatus::AlternateSource:
      return RouteIndexValidationStatus::WrongRelationship;
    case Route4PublicationAvailabilityStatus::NoMatch:
      return RouteIndexValidationStatus::NoMatch;
    case Route4PublicationAvailabilityStatus::Unavailable:
      return RouteIndexValidationStatus::Unavailable;
  }
  return RouteIndexValidationStatus::Unavailable;
}

[[nodiscard]] RouteIndexRecordReference route4_reference_key(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    RouteIndexRecordCategory category,
    RouteIndexRelationshipKind relationship,
    std::size_t instruction_index,
    std::size_t before_instruction_index,
    const Route1SourceValueIdentity& value,
    std::size_t record_index) {
  return RouteIndexRecordReference{
      .route = RouteIndexRoute::Route4PublicationAvailability,
      .owner_scope = index.function != nullptr ? RouteIndexOwnerScope::Function
                                               : RouteIndexOwnerScope::None,
      .record_category = category,
      .relationship = relationship,
      .function = index.function,
      .block = &block,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .instruction_index = instruction_index,
      .before_instruction_index = before_instruction_index,
      .value = value,
      .record_index = record_index,
  };
}

[[nodiscard]] Route4IndexReferenceValidation route4_missing_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    RouteIndexRecordCategory category,
    RouteIndexRelationshipKind relationship,
    std::size_t instruction_index,
    std::size_t before_instruction_index,
    const Route1SourceValueIdentity& value,
    RouteIndexValidationStatus status,
    Route4PublicationAvailabilityStatus route_status) {
  return Route4IndexReferenceValidation{
      .valid = false,
      .status = status,
      .route_status = route_status,
      .reference = route4_reference_key(index,
                                        block,
                                        category,
                                        relationship,
                                        instruction_index,
                                        before_instruction_index,
                                        value,
                                        0),
  };
}

[[nodiscard]] bool route4_value_record_matches(
    const Route4PublicationValueRecord& record,
    const Block& block,
    const Value& value) {
  const bool block_matches =
      (record.block_label_id != kInvalidBlockLabel ||
       block.label_id != kInvalidBlockLabel)
          ? record.block_label_id == block.label_id
          : (!record.block_label.empty() && record.block_label == block.label);
  return block_matches && value.kind == Value::Kind::Named &&
         record.value.name == value.name &&
         record.value.type == value.type;
}

}  // namespace

Route4PublicationAvailabilityIndex route4_build_publication_availability_index(
    const Function& function) {
  Route4PublicationAvailabilityIndex index{
      .function = &function,
  };
  for (const auto& block : function.blocks) {
    const auto route1_index = route1_build_producer_index(block);
    for (const auto& producer : route1_index.records) {
      if (!producer ||
          !producer.source_value ||
          producer.source_value.value == nullptr ||
          producer.source_value.value_kind != Value::Kind::Named ||
          producer.source_value.name.empty()) {
        continue;
      }
      const auto before = producer.producer_instruction.instruction_index + 1U;
      const auto query = Route1SameBlockProducerQuery{
          .index = &route1_index,
          .before_instruction_index = before,
      };
      const auto current =
          route4_current_block_publication_record(
              query, *producer.source_value.value, producer.source_value.name_id);
      index.current_block_records.push_back(current);
      index.value_records.push_back(
          route4_current_block_publication_value_record(
              query, *producer.source_value.value, producer.source_value.name_id));
    }

    for (const auto& inst : block.insts) {
      const auto* phi = std::get_if<PhiInst>(&inst);
      if (phi == nullptr) {
        break;
      }
      if (phi->result.kind != Value::Kind::Named || phi->result.name.empty()) {
        continue;
      }
      const auto block_entry =
          route4_block_entry_publication_record(&block, phi->result);
      index.block_entry_records.push_back(block_entry);
      index.value_records.push_back(
          route4_block_entry_publication_value_record(&block, phi->result));
    }
  }
  return index;
}

BirPublicationView make_bir_publication_view(const Function& function) {
  return BirPublicationView{route4_build_publication_availability_index(function)};
}

Route4IndexReferenceValidation validate_current_block_publication_reference(
    const BirPublicationView& view,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  return route4_validate_current_block_publication_reference(
      view.route4_index_, block, value, before_instruction_index);
}

Route4IndexReferenceValidation validate_block_entry_publication_reference(
    const BirPublicationView& view,
    const Block& successor_block,
    const Value& destination_value) {
  return route4_validate_block_entry_publication_reference(
      view.route4_index_, successor_block, destination_value);
}

Route4CurrentBlockPublicationRecord route4_find_current_block_publication(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  Route4CurrentBlockPublicationRecord result{
      .status = Route4PublicationAvailabilityStatus::Unavailable,
      .value = route1_source_value_identity(value),
      .value_name = value.kind == Value::Kind::Named ? std::string_view{value.name}
                                                     : std::string_view{},
      .value_type = value.type,
      .before_instruction_index = before_instruction_index,
  };
  if (!index) {
    result.status = Route4PublicationAvailabilityStatus::MissingBlock;
    return result;
  }
  result.block = &block;
  result.block_label = block.label;
  result.block_label_id = block.label_id;
  if (value.kind != Value::Kind::Named || value.name.empty()) {
    result.status = Route4PublicationAvailabilityStatus::MissingValue;
    return result;
  }
  const Route4CurrentBlockPublicationRecord* type_mismatch = nullptr;
  for (auto it = index.current_block_records.rbegin();
       it != index.current_block_records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        !route4_record_matches_block(*index.function,
                                     candidate.block,
                                     candidate.block_label,
                                     candidate.block_label_id,
                                     block) ||
        candidate.value_name != value.name ||
        candidate.source_producer_instruction_index >= before_instruction_index) {
      continue;
    }
    if (candidate.value_type != value.type) {
      type_mismatch = &candidate;
      continue;
    }
    return candidate;
  }
  result.status = type_mismatch != nullptr
                      ? Route4PublicationAvailabilityStatus::NoMatch
                      : Route4PublicationAvailabilityStatus::MissingPublication;
  return result;
}

Route4IndexReferenceValidation
route4_validate_current_block_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  const auto value_identity = route1_source_value_identity(value);
  constexpr auto category =
      RouteIndexRecordCategory::Route4CurrentBlockPublication;
  constexpr auto relationship =
      RouteIndexRelationshipKind::Route4CurrentBlockPublication;
  if (!index) {
    return route4_missing_reference(
        index,
        block,
        category,
        relationship,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        RouteIndexValidationStatus::MissingRecord,
        Route4PublicationAvailabilityStatus::MissingBlock);
  }
  if (!route4_index_contains_block(index, block)) {
    return route4_missing_reference(
        index,
        block,
        category,
        relationship,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        RouteIndexValidationStatus::StaleOwner,
        Route4PublicationAvailabilityStatus::MissingBlock);
  }
  if (value.kind != Value::Kind::Named || value.name.empty()) {
    return route4_missing_reference(
        index,
        block,
        category,
        relationship,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        RouteIndexValidationStatus::MissingRecord,
        Route4PublicationAvailabilityStatus::MissingValue);
  }

  const Route4CurrentBlockPublicationRecord* match = nullptr;
  std::size_t match_index = 0;
  bool wrong_key = false;
  for (std::size_t record_index = 0;
       record_index < index.current_block_records.size();
       ++record_index) {
    const auto& candidate = index.current_block_records[record_index];
    if (!candidate ||
        !route4_record_matches_block(*index.function,
                                     candidate.block,
                                     candidate.block_label,
                                     candidate.block_label_id,
                                     block) ||
        candidate.value_name != value.name) {
      continue;
    }
    if (candidate.value_type != value.type ||
        candidate.source_producer_instruction_index >= before_instruction_index) {
      wrong_key = true;
      continue;
    }
    if (match != nullptr) {
      return route4_missing_reference(
          index,
          block,
          category,
          relationship,
          candidate.source_producer_instruction_index,
          before_instruction_index,
          value_identity,
          RouteIndexValidationStatus::DuplicateReference,
          Route4PublicationAvailabilityStatus::NoMatch);
    }
    match = &candidate;
    match_index = record_index;
  }

  if (match == nullptr) {
    bool wrong_relationship = false;
    for (const auto& value_record : index.value_records) {
      if (value_record.scope != Route4PublicationScope::CurrentBlock &&
          route4_value_record_matches(value_record, block, value)) {
        wrong_relationship = true;
        break;
      }
    }
    const auto status =
        wrong_relationship ? RouteIndexValidationStatus::WrongRelationship
                           : (wrong_key ? RouteIndexValidationStatus::WrongKey
                                        : RouteIndexValidationStatus::MissingRecord);
    return route4_missing_reference(
        index,
        block,
        category,
        relationship,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        status,
        wrong_key ? Route4PublicationAvailabilityStatus::NoMatch
                  : Route4PublicationAvailabilityStatus::MissingPublication);
  }

  auto status = route4_reference_status(match->status);
  bool valid = status == RouteIndexValidationStatus::Valid;
  if (valid &&
      (match->source_producer_instruction == nullptr ||
       match->source_producer_instruction_index >= block.insts.size() ||
       match->source_producer_instruction !=
           &block.insts[match->source_producer_instruction_index])) {
    status = RouteIndexValidationStatus::Diverged;
    valid = false;
  }
  return Route4IndexReferenceValidation{
      .valid = valid,
      .status = status,
      .route_status = match->status,
      .reference = route4_reference_key(index,
                                        block,
                                        category,
                                        relationship,
                                        match->source_producer_instruction_index,
                                        before_instruction_index,
                                        match->value,
                                        match_index),
      .current_block_record = match,
  };
}

Route4IndexReferenceValidation
route4_validate_block_entry_publication_reference(
    const Route4PublicationAvailabilityIndex& index,
    const Block& successor_block,
    const Value& destination_value) {
  const auto value_identity = route1_source_value_identity(destination_value);
  constexpr auto category = RouteIndexRecordCategory::Route4BlockEntryPublication;
  constexpr auto relationship =
      RouteIndexRelationshipKind::Route4BlockEntryPublication;
  if (!index) {
    return route4_missing_reference(
        index,
        successor_block,
        category,
        relationship,
        0,
        0,
        value_identity,
        RouteIndexValidationStatus::MissingRecord,
        Route4PublicationAvailabilityStatus::MissingBlock);
  }
  if (!route4_index_contains_block(index, successor_block)) {
    return route4_missing_reference(
        index,
        successor_block,
        category,
        relationship,
        0,
        0,
        value_identity,
        RouteIndexValidationStatus::StaleOwner,
        Route4PublicationAvailabilityStatus::MissingBlock);
  }
  if (destination_value.kind != Value::Kind::Named ||
      destination_value.name.empty()) {
    return route4_missing_reference(
        index,
        successor_block,
        category,
        relationship,
        0,
        0,
        value_identity,
        RouteIndexValidationStatus::MissingRecord,
        Route4PublicationAvailabilityStatus::MissingValue);
  }

  const Route4BlockEntryPublicationRecord* match = nullptr;
  std::size_t match_index = 0;
  bool wrong_key = false;
  for (std::size_t record_index = 0;
       record_index < index.block_entry_records.size();
       ++record_index) {
    const auto& candidate = index.block_entry_records[record_index];
    if (!candidate ||
        !route4_record_matches_block(*index.function,
                                     candidate.successor_block,
                                     candidate.successor_label,
                                     candidate.successor_label_id,
                                     successor_block) ||
        candidate.destination_value_name != destination_value.name) {
      continue;
    }
    if (candidate.destination_value_type != destination_value.type) {
      wrong_key = true;
      continue;
    }
    if (match != nullptr) {
      return route4_missing_reference(
          index,
          successor_block,
          category,
          relationship,
          candidate.destination_instruction_index,
          candidate.destination_instruction_index,
          value_identity,
          RouteIndexValidationStatus::DuplicateReference,
          Route4PublicationAvailabilityStatus::NoMatch);
    }
    match = &candidate;
    match_index = record_index;
  }

  if (match == nullptr) {
    bool wrong_relationship = false;
    for (const auto& value_record : index.value_records) {
      if (value_record.scope != Route4PublicationScope::BlockEntry &&
          route4_value_record_matches(
              value_record, successor_block, destination_value)) {
        wrong_relationship = true;
        break;
      }
    }
    const auto status =
        wrong_relationship ? RouteIndexValidationStatus::WrongRelationship
                           : (wrong_key ? RouteIndexValidationStatus::WrongKey
                                        : RouteIndexValidationStatus::MissingRecord);
    return route4_missing_reference(
        index,
        successor_block,
        category,
        relationship,
        0,
        0,
        value_identity,
        status,
        wrong_key ? Route4PublicationAvailabilityStatus::NoMatch
                  : Route4PublicationAvailabilityStatus::MissingPublication);
  }

  auto status = route4_reference_status(match->status);
  bool valid = status == RouteIndexValidationStatus::Valid;
  if (valid &&
      (match->destination_instruction == nullptr ||
       match->destination_instruction_index >= successor_block.insts.size() ||
       match->destination_instruction !=
           &successor_block.insts[match->destination_instruction_index])) {
    status = RouteIndexValidationStatus::Diverged;
    valid = false;
  }
  return Route4IndexReferenceValidation{
      .valid = valid,
      .status = status,
      .route_status = match->status,
      .reference = route4_reference_key(
          index,
          successor_block,
          category,
          relationship,
          match->destination_instruction_index,
          match->destination_instruction_index,
          match->destination_value,
          match_index),
      .block_entry_record = match,
  };
}

}  // namespace c4c::backend::bir
