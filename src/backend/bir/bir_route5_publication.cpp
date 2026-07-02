#include "bir.hpp"

#include <string_view>

namespace c4c::backend::bir {

[[nodiscard]] bool route4_record_matches_block(
    const Function& function,
    const Block* record_block,
    std::string_view record_label,
    BlockLabelId record_label_id,
    const Block& block);

Route5PublicationSourceKind route5_publication_source_kind(
    Route1ProducerKind kind) {
  switch (kind) {
    case Route1ProducerKind::Immediate:
      return Route5PublicationSourceKind::Immediate;
    case Route1ProducerKind::LoadLocal:
      return Route5PublicationSourceKind::LoadLocal;
    case Route1ProducerKind::LoadGlobal:
      return Route5PublicationSourceKind::LoadGlobal;
    case Route1ProducerKind::Cast:
      return Route5PublicationSourceKind::Cast;
    case Route1ProducerKind::Binary:
      return Route5PublicationSourceKind::Binary;
    case Route1ProducerKind::SelectMaterialization:
      return Route5PublicationSourceKind::SelectMaterialization;
    case Route1ProducerKind::Unknown:
      return Route5PublicationSourceKind::Unknown;
  }
  return Route5PublicationSourceKind::Unknown;
}

Route5CfgEdgePublicationRecord route5_cfg_edge_publication_record(
    const Block* predecessor_block,
    const Block* successor_block,
    const Value& destination_value,
    ValueNameId destination_value_name_id,
    ValueNameId source_value_name_id) {
  Route5CfgEdgePublicationRecord record{
      .status = Route5PublicationStatus::Unavailable,
      .predecessor_block = predecessor_block,
      .successor_block = successor_block,
      .destination_value =
          route1_source_value_identity(destination_value, destination_value_name_id),
      .destination_value_name =
          destination_value.kind == Value::Kind::Named
              ? std::string_view{destination_value.name}
              : std::string_view{},
      .destination_value_name_id = destination_value_name_id,
      .destination_value_type = destination_value.type,
  };
  if (predecessor_block == nullptr) {
    record.status = Route5PublicationStatus::MissingPredecessor;
    return record;
  }
  record.predecessor_label = predecessor_block->label;
  record.predecessor_label_id = predecessor_block->label_id;
  if (successor_block == nullptr) {
    record.status = Route5PublicationStatus::MissingSuccessor;
    return record;
  }
  record.successor_label = successor_block->label;
  record.successor_label_id = successor_block->label_id;
  if (destination_value.kind != Value::Kind::Named ||
      destination_value.name.empty()) {
    record.status = Route5PublicationStatus::MissingDestination;
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
      record.status = Route5PublicationStatus::NoMatch;
      return record;
    }
    record.destination_instruction = &inst;
    record.destination_phi = phi;
    record.destination_instruction_index = instruction_index;
    record.destination_value =
        route1_source_value_identity(phi->result, destination_value_name_id);
    record.destination_value_name = phi->result.name;
    record.destination_value_type = phi->result.type;

    for (const auto& incoming : phi->incomings) {
      const bool id_matches =
          incoming.label_id != kInvalidBlockLabel &&
          predecessor_block->label_id != kInvalidBlockLabel &&
          incoming.label_id == predecessor_block->label_id;
      const bool label_matches =
          !incoming.label.empty() && incoming.label == predecessor_block->label;
      if (!id_matches && !label_matches) {
        continue;
      }
      record.source_value =
          route1_source_value_identity(incoming.value, source_value_name_id);
      record.source_value_kind = incoming.value.kind;
      record.source_value_type = incoming.value.type;
      if (incoming.value.kind == Value::Kind::Named) {
        record.source_value_name = incoming.value.name;
        record.source_value_name_id = source_value_name_id;
        const auto route1_index =
            route1_build_producer_index(*predecessor_block);
        const auto producer = route1_find_same_block_scalar_producer(
            Route1SameBlockProducerQuery{
                .index = &route1_index,
                .before_instruction_index = predecessor_block->insts.size(),
            },
            incoming.value);
        if (!producer.has_value() ||
            producer->record == nullptr ||
            producer->instruction == nullptr) {
          record.status = Route5PublicationStatus::MissingSourceProducer;
          return record;
        }
        record.source_producer_kind =
            route5_publication_source_kind(producer->record->kind);
        record.source_producer_instruction = producer->instruction;
        record.source_producer_block_label_id =
            producer->record->producer_instruction.block_label_id;
        record.source_producer_instruction_index = producer->instruction_index;
        if (record.source_producer_kind == Route5PublicationSourceKind::LoadLocal) {
          const auto route3_index =
              route3_build_memory_access_index(*predecessor_block);
          const auto* memory_access = route3_find_memory_access_record(
              route3_index,
              producer->instruction_index,
              Route3MemoryAccessNodeKind::LoadLocal);
          if (memory_access == nullptr) {
            record.status = Route5PublicationStatus::MissingSourceMemoryAccess;
            return record;
          }
          record.source_memory_identity_available = true;
          record.source_memory_access = *memory_access;
          record.status = Route5PublicationStatus::MemorySource;
        } else {
          record.status = Route5PublicationStatus::Available;
        }
      } else {
        record.source_producer_kind = Route5PublicationSourceKind::Immediate;
        record.status = Route5PublicationStatus::Available;
      }
      record.available = true;
      return record;
    }
    record.explicit_no_source = true;
    record.status = Route5PublicationStatus::NoSource;
    return record;
  }
  record.status = Route5PublicationStatus::MissingPublication;
  return record;
}

std::vector<Route5CurrentBlockJoinSourceRecord>
route5_current_block_join_source_records(const Block* successor_block) {
  std::vector<Route5CurrentBlockJoinSourceRecord> records;
  if (successor_block == nullptr) {
    records.push_back(Route5CurrentBlockJoinSourceRecord{
        .status = Route5PublicationStatus::MissingSuccessor,
    });
    return records;
  }
  const auto route1_index = route1_build_producer_index(*successor_block);
  bool saw_phi = false;
  for (std::size_t instruction_index = 0;
       instruction_index < successor_block->insts.size();
       ++instruction_index) {
    const auto& inst = successor_block->insts[instruction_index];
    const auto* phi = std::get_if<PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    saw_phi = true;
    for (const auto& incoming : phi->incomings) {
      Route5CurrentBlockJoinSourceRecord record{
          .status = Route5PublicationStatus::Unavailable,
          .successor_block = successor_block,
          .successor_label = successor_block->label,
          .successor_label_id = successor_block->label_id,
          .predecessor_label = incoming.label,
          .predecessor_label_id = incoming.label_id,
          .destination_instruction = &inst,
          .destination_phi = phi,
          .destination_instruction_index = instruction_index,
          .destination_value = route1_source_value_identity(phi->result),
          .destination_value_name =
              phi->result.kind == Value::Kind::Named
                  ? std::string_view{phi->result.name}
                  : std::string_view{},
          .destination_value_type = phi->result.type,
          .source_value = route1_source_value_identity(incoming.value),
          .source_value_name =
              incoming.value.kind == Value::Kind::Named
                  ? std::string_view{incoming.value.name}
                  : std::string_view{},
          .source_value_kind = incoming.value.kind,
          .source_value_type = incoming.value.type,
      };
      if (incoming.value.kind == Value::Kind::Named) {
        const auto producer = route1_find_same_block_scalar_producer(
            Route1SameBlockProducerQuery{
                .index = &route1_index,
                .before_instruction_index = successor_block->insts.size(),
            },
            incoming.value);
        if (!producer.has_value() ||
            producer->record == nullptr ||
            producer->instruction == nullptr) {
          record.status = Route5PublicationStatus::MissingSourceProducer;
          records.push_back(record);
          continue;
        }
        record.source_producer_kind =
            route5_publication_source_kind(producer->record->kind);
        record.source_producer_instruction = producer->instruction;
        record.source_producer_instruction_index = producer->instruction_index;
      } else {
        record.source_producer_kind = Route5PublicationSourceKind::Immediate;
      }
      record.available = true;
      record.status = Route5PublicationStatus::Available;
      records.push_back(record);
    }
  }
  if (!saw_phi || records.empty()) {
    records.push_back(Route5CurrentBlockJoinSourceRecord{
        .status = Route5PublicationStatus::MissingPublication,
        .successor_block = successor_block,
        .successor_label = successor_block->label,
        .successor_label_id = successor_block->label_id,
    });
  }
  return records;
}

Route5PublicationValueRecord route5_edge_destination_value_record(
    const Route5CfgEdgePublicationRecord& edge) {
  return Route5PublicationValueRecord{
      .available = edge.available,
      .scope = Route5PublicationScope::CfgEdge,
      .status = edge.status,
      .value_role = Route5PublicationValueRole::Destination,
      .value = edge.destination_value,
      .block_label = edge.successor_label,
      .block_label_id = edge.successor_label_id,
      .predecessor_label = edge.predecessor_label,
      .predecessor_label_id = edge.predecessor_label_id,
      .instruction_index = edge.destination_instruction_index,
      .edge = edge,
  };
}

Route5PublicationValueRecord route5_edge_source_value_record(
    const Route5CfgEdgePublicationRecord& edge) {
  return Route5PublicationValueRecord{
      .available = edge.available,
      .scope = Route5PublicationScope::CfgEdge,
      .status = edge.status,
      .value_role = Route5PublicationValueRole::Source,
      .value = edge.source_value,
      .block_label = edge.successor_label,
      .block_label_id = edge.successor_label_id,
      .predecessor_label = edge.predecessor_label,
      .predecessor_label_id = edge.predecessor_label_id,
      .instruction_index = edge.source_producer_instruction_index.value_or(0),
      .edge = edge,
  };
}

Route5PublicationValueRecord route5_join_destination_value_record(
    const Route5CurrentBlockJoinSourceRecord& join) {
  return Route5PublicationValueRecord{
      .available = join.available,
      .scope = Route5PublicationScope::CurrentBlockJoin,
      .status = join.status,
      .value_role = Route5PublicationValueRole::Destination,
      .value = join.destination_value,
      .block_label = join.successor_label,
      .block_label_id = join.successor_label_id,
      .predecessor_label = join.predecessor_label,
      .predecessor_label_id = join.predecessor_label_id,
      .instruction_index = join.destination_instruction_index,
      .join = join,
  };
}

Route5PublicationValueRecord route5_join_source_value_record(
    const Route5CurrentBlockJoinSourceRecord& join) {
  return Route5PublicationValueRecord{
      .available = join.available,
      .scope = Route5PublicationScope::CurrentBlockJoin,
      .status = join.status,
      .value_role = Route5PublicationValueRole::Source,
      .value = join.source_value,
      .block_label = join.successor_label,
      .block_label_id = join.successor_label_id,
      .predecessor_label = join.predecessor_label,
      .predecessor_label_id = join.predecessor_label_id,
      .instruction_index = join.source_producer_instruction_index.value_or(0),
      .join = join,
  };
}

namespace {

[[nodiscard]] const Block* route5_find_block_by_label(
    const Function& function,
    std::string_view label,
    BlockLabelId label_id) {
  for (const auto& block : function.blocks) {
    if (label_id != kInvalidBlockLabel &&
        block.label_id != kInvalidBlockLabel &&
        block.label_id == label_id) {
      return &block;
    }
    if (!label.empty() && block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

[[nodiscard]] bool route5_value_matches_record(
    const Route1SourceValueIdentity& record_value,
    std::string_view record_name,
    TypeKind record_type,
    const Value& value) {
  if (value.kind == Value::Kind::Named) {
    return !record_name.empty() &&
           record_name == value.name &&
           record_type == value.type;
  }
  if (value.kind == Value::Kind::Immediate) {
    return record_value.value_kind == Value::Kind::Immediate &&
           record_value.integer_constant == value.immediate &&
           record_type == value.type;
  }
  return false;
}

}  // namespace

Route5EdgeJoinSourceIndex route5_build_edge_join_source_index(
    const Function& function) {
  Route5EdgeJoinSourceIndex index{
      .function = &function,
  };
  for (const auto& successor_block : function.blocks) {
    const auto join_records =
        route5_current_block_join_source_records(&successor_block);
    for (const auto& join : join_records) {
      index.join_records.push_back(join);
      index.value_records.push_back(route5_join_destination_value_record(join));
      index.value_records.push_back(route5_join_source_value_record(join));
    }

    for (const auto& inst : successor_block.insts) {
      const auto* phi = std::get_if<PhiInst>(&inst);
      if (phi == nullptr) {
        break;
      }
      if (phi->result.kind != Value::Kind::Named || phi->result.name.empty()) {
        continue;
      }
      for (const auto& incoming : phi->incomings) {
        const auto* predecessor_block =
            route5_find_block_by_label(function, incoming.label, incoming.label_id);
        if (predecessor_block == nullptr) {
          continue;
        }
        const auto edge = route5_cfg_edge_publication_record(
            predecessor_block, &successor_block, phi->result);
        index.edge_records.push_back(edge);
        index.value_records.push_back(route5_edge_destination_value_record(edge));
        index.value_records.push_back(route5_edge_source_value_record(edge));
      }
    }
  }
  return index;
}

Route5CfgEdgePublicationRecord route5_find_cfg_edge_publication(
    const Route5EdgeJoinSourceIndex& index,
    const Block& predecessor_block,
    const Block& successor_block,
    const Value& destination_value) {
  Route5CfgEdgePublicationRecord result{
      .status = Route5PublicationStatus::Unavailable,
      .predecessor_block = &predecessor_block,
      .predecessor_label = predecessor_block.label,
      .predecessor_label_id = predecessor_block.label_id,
      .successor_block = &successor_block,
      .successor_label = successor_block.label,
      .successor_label_id = successor_block.label_id,
      .destination_value = route1_source_value_identity(destination_value),
      .destination_value_name =
          destination_value.kind == Value::Kind::Named
              ? std::string_view{destination_value.name}
              : std::string_view{},
      .destination_value_type = destination_value.type,
  };
  if (!index) {
    result.status = Route5PublicationStatus::MissingPublication;
    return result;
  }
  if (destination_value.kind != Value::Kind::Named ||
      destination_value.name.empty()) {
    result.status = Route5PublicationStatus::MissingDestination;
    return result;
  }

  bool saw_destination_name = false;
  bool saw_destination_type_mismatch = false;
  bool saw_successor = false;
  const Route5CfgEdgePublicationRecord* no_source_candidate = nullptr;
  for (const auto& candidate : index.edge_records) {
    if (!route4_record_matches_block(*index.function,
                                     candidate.successor_block,
                                     candidate.successor_label,
                                     candidate.successor_label_id,
                                     successor_block)) {
      continue;
    }
    saw_successor = true;
    if (candidate.destination_value_name != destination_value.name) {
      continue;
    }
    saw_destination_name = true;
    if (candidate.destination_value_type != destination_value.type) {
      saw_destination_type_mismatch = true;
      continue;
    }
    if (!route4_record_matches_block(*index.function,
                                     candidate.predecessor_block,
                                     candidate.predecessor_label,
                                     candidate.predecessor_label_id,
                                     predecessor_block)) {
      if (no_source_candidate == nullptr) {
        no_source_candidate = &candidate;
      }
      continue;
    }
    return candidate;
  }
  if (!saw_successor) {
    result.status = Route5PublicationStatus::MissingSuccessor;
  } else if (saw_destination_type_mismatch) {
    result.status = Route5PublicationStatus::NoMatch;
  } else if (no_source_candidate != nullptr) {
    result = *no_source_candidate;
    result.available = false;
    result.status = Route5PublicationStatus::NoSource;
    result.explicit_no_source = true;
  } else if (saw_destination_name) {
    result.status = Route5PublicationStatus::NoSource;
    result.explicit_no_source = true;
  } else {
    result.status = Route5PublicationStatus::MissingPublication;
  }
  return result;
}

Route5CurrentBlockJoinSourceRecord route5_find_current_block_join_source(
    const Route5EdgeJoinSourceIndex& index,
    const Block& successor_block,
    const Value& destination_value,
    const Value& source_value) {
  Route5CurrentBlockJoinSourceRecord result{
      .status = Route5PublicationStatus::Unavailable,
      .successor_block = &successor_block,
      .successor_label = successor_block.label,
      .successor_label_id = successor_block.label_id,
      .destination_value = route1_source_value_identity(destination_value),
      .destination_value_name =
          destination_value.kind == Value::Kind::Named
              ? std::string_view{destination_value.name}
              : std::string_view{},
      .destination_value_type = destination_value.type,
      .source_value = route1_source_value_identity(source_value),
      .source_value_name =
          source_value.kind == Value::Kind::Named
              ? std::string_view{source_value.name}
              : std::string_view{},
      .source_value_kind = source_value.kind,
      .source_value_type = source_value.type,
  };
  if (!index) {
    result.status = Route5PublicationStatus::MissingPublication;
    return result;
  }
  if (destination_value.kind != Value::Kind::Named ||
      destination_value.name.empty()) {
    result.status = Route5PublicationStatus::MissingDestination;
    return result;
  }

  bool saw_successor = false;
  bool saw_destination_name = false;
  bool saw_destination_type_mismatch = false;
  bool saw_source_name = false;
  for (const auto& candidate : index.join_records) {
    if (!route4_record_matches_block(*index.function,
                                     candidate.successor_block,
                                     candidate.successor_label,
                                     candidate.successor_label_id,
                                     successor_block)) {
      continue;
    }
    saw_successor = true;
    if (candidate.destination_value_name != destination_value.name) {
      continue;
    }
    saw_destination_name = true;
    if (candidate.destination_value_type != destination_value.type) {
      saw_destination_type_mismatch = true;
      continue;
    }
    if (source_value.kind == Value::Kind::Named &&
        candidate.source_value_name == source_value.name) {
      saw_source_name = true;
    }
    if (route5_value_matches_record(candidate.source_value,
                                    candidate.source_value_name,
                                    candidate.source_value_type,
                                    source_value)) {
      return candidate;
    }
  }
  if (!saw_successor) {
    result.status = Route5PublicationStatus::MissingSuccessor;
  } else if (saw_destination_type_mismatch) {
    result.status = Route5PublicationStatus::NoMatch;
  } else if (!saw_destination_name) {
    result.status = Route5PublicationStatus::MissingPublication;
  } else if (saw_source_name) {
    result.status = Route5PublicationStatus::NoMatch;
  } else {
    result.status = Route5PublicationStatus::MissingSourceValue;
  }
  return result;
}

}  // namespace c4c::backend::bir
