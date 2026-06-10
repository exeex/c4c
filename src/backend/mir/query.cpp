#include "query.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace c4c::backend::mir {

namespace {

[[nodiscard]] std::string_view root_value_name(
    const BirSelectChainIdentityRequest& request);

[[nodiscard]] bir::TypeKind root_value_type(
    const BirSelectChainIdentityRequest& request);

}  // namespace

[[nodiscard]] BirMemoryAccessNodeKind bir_memory_access_node_kind(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          return BirMemoryAccessNodeKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          return BirMemoryAccessNodeKind::LoadGlobal;
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst>) {
          return BirMemoryAccessNodeKind::StoreLocal;
        } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst>) {
          return BirMemoryAccessNodeKind::StoreGlobal;
        } else {
          return BirMemoryAccessNodeKind::Unknown;
        }
      },
      inst);
}

[[nodiscard]] SameBlockProducerKind same_block_producer_kind(const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          return SameBlockProducerKind::Binary;
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          return SameBlockProducerKind::Cast;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return SameBlockProducerKind::Select;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          return SameBlockProducerKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          return SameBlockProducerKind::LoadGlobal;
        } else {
          return SameBlockProducerKind::Unknown;
        }
      },
      inst);
}

[[nodiscard]] bool same_block_producer_kind_has_materialization(
    SameBlockProducerKind kind) {
  switch (kind) {
    case SameBlockProducerKind::Binary:
    case SameBlockProducerKind::Cast:
    case SameBlockProducerKind::Select:
    case SameBlockProducerKind::LoadLocal:
    case SameBlockProducerKind::LoadGlobal:
      return true;
    case SameBlockProducerKind::Unknown:
      return false;
  }
  return false;
}

[[nodiscard]] SameBlockValueIdentity same_block_value_identity(
    const bir::Value& value) {
  SameBlockValueIdentity identity{
      .value = &value,
      .name = value.name,
      .type = value.type,
  };
  if (value.kind == bir::Value::Kind::Immediate) {
    identity.immediate_constant = value.immediate;
  }
  return identity;
}

namespace {

[[nodiscard]] std::string_view named_value_name(const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named) {
    return {};
  }
  return value.name;
}

[[nodiscard]] BirMemoryAccessNodeKind route3_node_kind_to_mir(
    bir::Route3MemoryAccessNodeKind kind) {
  switch (kind) {
    case bir::Route3MemoryAccessNodeKind::LoadLocal:
      return BirMemoryAccessNodeKind::LoadLocal;
    case bir::Route3MemoryAccessNodeKind::LoadGlobal:
      return BirMemoryAccessNodeKind::LoadGlobal;
    case bir::Route3MemoryAccessNodeKind::StoreLocal:
      return BirMemoryAccessNodeKind::StoreLocal;
    case bir::Route3MemoryAccessNodeKind::StoreGlobal:
      return BirMemoryAccessNodeKind::StoreGlobal;
    case bir::Route3MemoryAccessNodeKind::Unknown:
      return BirMemoryAccessNodeKind::Unknown;
  }
  return BirMemoryAccessNodeKind::Unknown;
}

[[nodiscard]] bir::Route3MemoryAccessNodeKind route3_node_kind_from_mir(
    BirMemoryAccessNodeKind kind) {
  switch (kind) {
    case BirMemoryAccessNodeKind::LoadLocal:
      return bir::Route3MemoryAccessNodeKind::LoadLocal;
    case BirMemoryAccessNodeKind::LoadGlobal:
      return bir::Route3MemoryAccessNodeKind::LoadGlobal;
    case BirMemoryAccessNodeKind::StoreLocal:
      return bir::Route3MemoryAccessNodeKind::StoreLocal;
    case BirMemoryAccessNodeKind::StoreGlobal:
      return bir::Route3MemoryAccessNodeKind::StoreGlobal;
    case BirMemoryAccessNodeKind::Unknown:
      return bir::Route3MemoryAccessNodeKind::Unknown;
  }
  return bir::Route3MemoryAccessNodeKind::Unknown;
}

[[nodiscard]] BirMemoryAccessBaseKind route3_base_kind_to_mir(
    bir::Route3MemoryAccessBaseKind kind) {
  switch (kind) {
    case bir::Route3MemoryAccessBaseKind::LocalSlot:
      return BirMemoryAccessBaseKind::LocalSlot;
    case bir::Route3MemoryAccessBaseKind::GlobalSymbol:
      return BirMemoryAccessBaseKind::GlobalSymbol;
    case bir::Route3MemoryAccessBaseKind::PointerValue:
      return BirMemoryAccessBaseKind::PointerValue;
    case bir::Route3MemoryAccessBaseKind::StringConstant:
      return BirMemoryAccessBaseKind::StringConstant;
    case bir::Route3MemoryAccessBaseKind::None:
      return BirMemoryAccessBaseKind::None;
  }
  return BirMemoryAccessBaseKind::None;
}

[[nodiscard]] SameBlockValueIdentity route1_value_identity_to_same_block(
    const bir::Route1SourceValueIdentity& value) {
  return SameBlockValueIdentity{
      .value = value.value,
      .name = value.name,
      .type = value.type,
      .immediate_constant = value.integer_constant,
  };
}

[[nodiscard]] BirMemoryAccessIdentity route3_memory_access_to_mir(
    const bir::Route3MemoryAccessRecord& record) {
  if (!record) {
    return {};
  }
  return BirMemoryAccessIdentity{
      .inst = record.instruction,
      .block_label = record.block_label,
      .instruction_index = record.instruction_index,
      .node_kind = route3_node_kind_to_mir(record.node_kind),
      .result_value_name = record.result_value.name,
      .stored_value_name = record.stored_value.name,
      .address_space = record.address_space,
      .is_volatile = record.is_volatile,
      .base_kind = route3_base_kind_to_mir(record.base_kind),
      .local_slot_name = record.local_slot_name,
      .local_slot_id = record.local_slot_id,
      .global_name = record.global_name,
      .global_name_id = record.global_name_id,
      .pointer_value_name = record.pointer_value.name,
      .string_constant_name = record.string_constant_name,
  };
}

[[nodiscard]] SameBlockProducerIdentity route3_load_access_to_producer(
    const bir::Route3MemoryAccessRecord& record,
    std::size_t before_instruction_index) {
  const auto node_kind = route3_node_kind_to_mir(record.node_kind);
  SameBlockProducerKind producer_kind = SameBlockProducerKind::Unknown;
  if (node_kind == BirMemoryAccessNodeKind::LoadLocal) {
    producer_kind = SameBlockProducerKind::LoadLocal;
  } else if (node_kind == BirMemoryAccessNodeKind::LoadGlobal) {
    producer_kind = SameBlockProducerKind::LoadGlobal;
  }
  return SameBlockProducerIdentity{
      .inst = record.instruction,
      .instruction_index = record.instruction_index,
      .kind = producer_kind,
      .block_label = record.block_label,
      .before_instruction_index = before_instruction_index,
      .produced_value = route1_value_identity_to_same_block(record.result_value),
      .materialization_available =
          same_block_producer_kind_has_materialization(producer_kind),
  };
}

[[nodiscard]] bool route3_same_local_slot(
    const bir::Route3MemoryAccessRecord& lhs,
    const bir::Route3MemoryAccessRecord& rhs) {
  if (!lhs ||
      !rhs ||
      lhs.base_kind != bir::Route3MemoryAccessBaseKind::LocalSlot ||
      rhs.base_kind != bir::Route3MemoryAccessBaseKind::LocalSlot) {
    return false;
  }
  if (lhs.local_slot_id != c4c::kInvalidSlotName &&
      rhs.local_slot_id != c4c::kInvalidSlotName) {
    return lhs.local_slot_id == rhs.local_slot_id;
  }
  return !lhs.local_slot_name.empty() &&
         lhs.local_slot_name == rhs.local_slot_name;
}

[[nodiscard]] bir::Route3SameBlockGlobalLoadAccessRecord
find_route3_global_load_access(
    const bir::Route3MemoryAccessIndex& index,
    std::size_t before_instruction_index,
    std::string_view value_name,
    bir::TypeKind value_type) {
  if (value_name.empty()) {
    return {};
  }
  if (value_type != bir::TypeKind::Void) {
    return bir::route3_find_same_block_global_load_access(
        bir::Route3MemoryAccessQuery{
            .index = &index,
            .before_instruction_index = before_instruction_index,
        },
        bir::Value::named(value_type, std::string(value_name)));
  }
  bir::Route3SameBlockGlobalLoadAccessRecord result{
      .available = true,
      .root_value =
          bir::Route1SourceValueIdentity{
              .value_kind = bir::Value::Kind::Named,
              .name = value_name,
          },
  };
  for (auto it = index.records.rbegin(); it != index.records.rend(); ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != bir::Route3MemoryAccessNodeKind::LoadGlobal ||
        candidate.base_kind != bir::Route3MemoryAccessBaseKind::GlobalSymbol ||
        candidate.instruction_index >= before_instruction_index ||
        candidate.result_value.value_kind != bir::Value::Kind::Named ||
        candidate.result_value.name != value_name) {
      continue;
    }
    result.load_instruction_index = candidate.instruction_index;
    result.load_access = candidate;
    result.access_available = true;
    return result;
  }
  return result;
}

[[nodiscard]] bir::Route3SameBlockLoadLocalSourceRecord
find_route3_load_local_source(
    const bir::Route3MemoryAccessIndex& index,
    std::size_t before_instruction_index,
    std::string_view value_name,
    bir::TypeKind value_type) {
  if (value_name.empty()) {
    return {};
  }
  if (value_type != bir::TypeKind::Void) {
    return bir::route3_find_same_block_load_local_source(
        bir::Route3MemoryAccessQuery{
            .index = &index,
            .before_instruction_index = before_instruction_index,
        },
        bir::Value::named(value_type, std::string(value_name)));
  }
  bir::Route3SameBlockLoadLocalSourceRecord result{
      .available = true,
      .root_value =
          bir::Route1SourceValueIdentity{
              .value_kind = bir::Value::Kind::Named,
              .name = value_name,
          },
  };
  const bir::Route3MemoryAccessRecord* load_access = nullptr;
  for (auto it = index.records.rbegin(); it != index.records.rend(); ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != bir::Route3MemoryAccessNodeKind::LoadLocal ||
        candidate.base_kind != bir::Route3MemoryAccessBaseKind::LocalSlot ||
        candidate.instruction_index >= before_instruction_index ||
        candidate.result_value.value_kind != bir::Value::Kind::Named ||
        candidate.result_value.name != value_name) {
      continue;
    }
    load_access = &candidate;
    break;
  }
  if (load_access == nullptr) {
    return result;
  }
  result.load_instruction_index = load_access->instruction_index;
  result.load_access = *load_access;
  for (const auto& candidate : index.records) {
    if (!candidate ||
        candidate.node_kind != bir::Route3MemoryAccessNodeKind::StoreLocal ||
        candidate.instruction_index <= load_access->instruction_index ||
        candidate.instruction_index >= before_instruction_index ||
        !route3_same_local_slot(*load_access, candidate)) {
      continue;
    }
    result.invalidating_store_instruction_index = candidate.instruction_index;
    result.invalidating_store_access = candidate;
    return result;
  }
  result.source_available = true;
  return result;
}

[[nodiscard]] const bir::Value* produced_value_for_same_block_identity(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> const bir::Value* {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return &typed_inst.result;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] std::string_view normalized_block_label(
    const bir::Block& block,
    std::string_view requested_label) {
  if (!requested_label.empty()) {
    return requested_label;
  }
  return block.label;
}

[[nodiscard]] SameBlockProducerKind route1_producer_kind_to_same_block_kind(
    bir::Route1ProducerKind kind) {
  switch (kind) {
    case bir::Route1ProducerKind::Binary:
      return SameBlockProducerKind::Binary;
    case bir::Route1ProducerKind::Cast:
      return SameBlockProducerKind::Cast;
    case bir::Route1ProducerKind::SelectMaterialization:
      return SameBlockProducerKind::Select;
    case bir::Route1ProducerKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case bir::Route1ProducerKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case bir::Route1ProducerKind::Unknown:
    case bir::Route1ProducerKind::Immediate:
      return SameBlockProducerKind::Unknown;
  }
  return SameBlockProducerKind::Unknown;
}

[[nodiscard]] SameBlockProducerKind route2_select_chain_producer_kind_to_same_block_kind(
    bir::Route2SelectChainProducerKind kind) {
  switch (kind) {
    case bir::Route2SelectChainProducerKind::Binary:
      return SameBlockProducerKind::Binary;
    case bir::Route2SelectChainProducerKind::Cast:
      return SameBlockProducerKind::Cast;
    case bir::Route2SelectChainProducerKind::Select:
      return SameBlockProducerKind::Select;
    case bir::Route2SelectChainProducerKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case bir::Route2SelectChainProducerKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case bir::Route2SelectChainProducerKind::Unknown:
      return SameBlockProducerKind::Unknown;
  }
  return SameBlockProducerKind::Unknown;
}

[[nodiscard]] SameBlockProducerKind
route4_publication_source_kind_to_same_block_kind(
    bir::Route4PublicationSourceKind kind) {
  switch (kind) {
    case bir::Route4PublicationSourceKind::Binary:
      return SameBlockProducerKind::Binary;
    case bir::Route4PublicationSourceKind::Cast:
      return SameBlockProducerKind::Cast;
    case bir::Route4PublicationSourceKind::SelectMaterialization:
      return SameBlockProducerKind::Select;
    case bir::Route4PublicationSourceKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case bir::Route4PublicationSourceKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case bir::Route4PublicationSourceKind::Unknown:
      return SameBlockProducerKind::Unknown;
  }
  return SameBlockProducerKind::Unknown;
}

[[nodiscard]] SameBlockProducerKind
route5_publication_source_kind_to_same_block_kind(
    bir::Route5PublicationSourceKind kind) {
  switch (kind) {
    case bir::Route5PublicationSourceKind::Binary:
      return SameBlockProducerKind::Binary;
    case bir::Route5PublicationSourceKind::Cast:
      return SameBlockProducerKind::Cast;
    case bir::Route5PublicationSourceKind::SelectMaterialization:
      return SameBlockProducerKind::Select;
    case bir::Route5PublicationSourceKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case bir::Route5PublicationSourceKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case bir::Route5PublicationSourceKind::Unknown:
    case bir::Route5PublicationSourceKind::Immediate:
      return SameBlockProducerKind::Unknown;
  }
  return SameBlockProducerKind::Unknown;
}

[[nodiscard]] SameBlockValueIdentity route1_source_value_identity_to_same_block(
    const bir::Route1SourceValueIdentity& source) {
  return SameBlockValueIdentity{
      .value = source.value,
      .name = source.name,
      .type = source.type,
      .immediate_constant = source.integer_constant,
  };
}

[[nodiscard]] SameBlockValueIdentity route2_source_value_identity_to_same_block(
    const bir::Route1SourceValueIdentity& source) {
  return SameBlockValueIdentity{
      .value = source.value,
      .name = source.name,
      .type = source.type,
      .immediate_constant = source.integer_constant,
  };
}

[[nodiscard]] SameBlockProducerIdentity route1_producer_record_to_same_block(
    const bir::Route1ProducerRecord& record,
    const bir::Block& block,
    std::string_view block_label,
    std::size_t before_instruction_index) {
  if (!record || !record.producer_instruction || !record.source_value) {
    return {};
  }
  const auto kind = route1_producer_kind_to_same_block_kind(record.kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  return SameBlockProducerIdentity{
      .inst = record.producer_instruction.instruction,
      .instruction_index = record.producer_instruction.instruction_index,
      .kind = kind,
      .block_label = normalized_block_label(block, block_label),
      .before_instruction_index = before_instruction_index,
      .produced_value = route1_source_value_identity_to_same_block(
          record.source_value),
      .materialization_available =
          record.materialization.scalar_materialization_available,
  };
}

[[nodiscard]] SameBlockProducerIdentity
route2_select_chain_producer_record_to_same_block(
    const bir::Route2SelectChainValueRecord& record,
    const bir::Block& block,
    std::string_view block_label,
    std::size_t before_instruction_index) {
  if (!record ||
      !record.root_producer ||
      record.root_producer.instruction == nullptr ||
      record.root_value.value == nullptr) {
    return {};
  }
  const auto kind = route2_select_chain_producer_kind_to_same_block_kind(
      record.root_producer.kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  return SameBlockProducerIdentity{
      .inst = record.root_producer.instruction,
      .instruction_index = record.root_producer.instruction_index,
      .kind = kind,
      .block_label = normalized_block_label(block, block_label),
      .before_instruction_index = before_instruction_index,
      .produced_value = route2_source_value_identity_to_same_block(
          record.root_value),
      .materialization_available = record.scalar_materialization_available,
  };
}

[[nodiscard]] SameBlockProducerIdentity route4_current_block_record_to_same_block(
    const bir::Route4CurrentBlockPublicationRecord& record,
    const bir::Block& block,
    std::string_view block_label) {
  if (!record ||
      record.source_producer_instruction_index >= block.insts.size()) {
    return {};
  }
  const auto kind = route4_publication_source_kind_to_same_block_kind(
      record.source_producer_kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  const auto& inst = block.insts[record.source_producer_instruction_index];
  const auto* produced_value = produced_value_for_same_block_identity(inst);
  if (produced_value == nullptr ||
      produced_value->kind != bir::Value::Kind::Named ||
      produced_value->name != record.value_name ||
      produced_value->type != record.value_type) {
    return {};
  }
  return SameBlockProducerIdentity{
      .inst = &inst,
      .instruction_index = record.source_producer_instruction_index,
      .kind = kind,
      .block_label = normalized_block_label(block, block_label),
      .before_instruction_index = record.before_instruction_index,
      .produced_value = same_block_value_identity(*produced_value),
      .materialization_available = same_block_producer_kind_has_materialization(
          kind),
  };
}

[[nodiscard]] BirCfgEdgePublicationSourceStatus route5_edge_status_to_mir(
    bir::Route5PublicationStatus status) {
  switch (status) {
    case bir::Route5PublicationStatus::Available:
    case bir::Route5PublicationStatus::MemorySource:
      return BirCfgEdgePublicationSourceStatus::Available;
    case bir::Route5PublicationStatus::MissingPredecessor:
      return BirCfgEdgePublicationSourceStatus::MissingPredecessorLabel;
    case bir::Route5PublicationStatus::MissingSuccessor:
      return BirCfgEdgePublicationSourceStatus::MissingSuccessorLabel;
    case bir::Route5PublicationStatus::MissingDestination:
    case bir::Route5PublicationStatus::NoMatch:
      return BirCfgEdgePublicationSourceStatus::MissingDestinationValue;
    case bir::Route5PublicationStatus::NoSource:
      return BirCfgEdgePublicationSourceStatus::MissingSourceValue;
    case bir::Route5PublicationStatus::MissingSourceValue:
      return BirCfgEdgePublicationSourceStatus::MissingSourceValue;
    case bir::Route5PublicationStatus::MissingSourceProducer:
    case bir::Route5PublicationStatus::MissingSourceMemoryAccess:
    case bir::Route5PublicationStatus::IncompleteSourceMemoryAccess:
      return BirCfgEdgePublicationSourceStatus::MissingSourceProducer;
    case bir::Route5PublicationStatus::Unavailable:
    case bir::Route5PublicationStatus::MissingPublication:
      return BirCfgEdgePublicationSourceStatus::MissingPublication;
  }
  return BirCfgEdgePublicationSourceStatus::MissingPublication;
}

[[nodiscard]] const bir::Value* route5_original_source_value(
    const bir::Block& predecessor_block,
    const bir::Block& successor_block,
    const bir::Route5CfgEdgePublicationRecord& record) {
  if (record.destination_instruction_index >= successor_block.insts.size()) {
    return nullptr;
  }
  const auto* phi =
      std::get_if<bir::PhiInst>(&successor_block.insts[record.destination_instruction_index]);
  if (phi == nullptr) {
    return nullptr;
  }
  for (const auto& incoming : phi->incomings) {
    const bool id_matches =
        incoming.label_id != c4c::kInvalidBlockLabel &&
        predecessor_block.label_id != c4c::kInvalidBlockLabel &&
        incoming.label_id == predecessor_block.label_id;
    const bool label_matches =
        !incoming.label.empty() && incoming.label == predecessor_block.label;
    if (id_matches || label_matches) {
      return &incoming.value;
    }
  }
  return nullptr;
}

[[nodiscard]] SameBlockProducerIdentity route5_edge_source_producer_to_mir(
    const bir::Route5CfgEdgePublicationRecord& record,
    const bir::Block& predecessor_block,
    std::string_view predecessor_label) {
  if (!record.source_producer_instruction_index.has_value() ||
      *record.source_producer_instruction_index >= predecessor_block.insts.size()) {
    return {};
  }
  const auto kind = route5_publication_source_kind_to_same_block_kind(
      record.source_producer_kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  const auto& inst =
      predecessor_block.insts[*record.source_producer_instruction_index];
  const auto* produced_value = produced_value_for_same_block_identity(inst);
  if (produced_value == nullptr ||
      produced_value->kind != bir::Value::Kind::Named ||
      produced_value->name != record.source_value_name ||
      produced_value->type != record.source_value_type) {
    return {};
  }
  return SameBlockProducerIdentity{
      .inst = &inst,
      .instruction_index = *record.source_producer_instruction_index,
      .kind = kind,
      .block_label = normalized_block_label(predecessor_block, predecessor_label),
      .before_instruction_index = predecessor_block.insts.size(),
      .produced_value = same_block_value_identity(*produced_value),
      .materialization_available =
          same_block_producer_kind_has_materialization(kind),
  };
}

[[nodiscard]] BirCfgEdgePublicationSourceIdentity
route5_edge_record_to_mir(
    const bir::Route5CfgEdgePublicationRecord& record,
    const BirCfgEdgePublicationSourceRequest& request,
    std::string_view predecessor_label,
    std::string_view successor_label) {
  BirCfgEdgePublicationSourceIdentity result{
      .available = record.available,
      .status = route5_edge_status_to_mir(record.status),
      .predecessor_label = predecessor_label,
      .predecessor_label_id = request.predecessor_block != nullptr &&
                                      request.predecessor_block->label_id !=
                                          c4c::kInvalidBlockLabel
                                  ? request.predecessor_block->label_id
                                  : request.predecessor_label_id,
      .successor_label = successor_label,
      .successor_label_id = request.successor_block != nullptr &&
                                    request.successor_block->label_id !=
                                        c4c::kInvalidBlockLabel
                                ? request.successor_block->label_id
                                : request.successor_label_id,
      .destination_value_id = request.destination_value_id,
      .destination_value_name = record.destination_value_name,
      .destination_value_name_id = request.destination_value_name_id,
      .destination_value_type = record.destination_value_type,
      .source_value_name = record.source_value_name,
      .source_value_name_id = record.source_value_name_id,
      .source_value_kind = record.source_value_kind,
      .source_value_type = record.source_value_type,
      .source_producer_kind =
          route5_publication_source_kind_to_same_block_kind(
              record.source_producer_kind),
      .source_producer_block_label = predecessor_label,
      .source_producer_block_label_id =
          record.source_producer_block_label_id,
      .source_producer_instruction_index =
          record.source_producer_instruction_index,
  };
  if (request.successor_block != nullptr &&
      record.destination_instruction_index < request.successor_block->insts.size()) {
    const auto& inst =
        request.successor_block->insts[record.destination_instruction_index];
    if (const auto* phi = std::get_if<bir::PhiInst>(&inst)) {
      result.destination_instruction = &inst;
      result.destination_phi = phi;
      result.destination_instruction_index =
          record.destination_instruction_index;
      result.destination_value = &phi->result;
      result.destination_value_identity = same_block_value_identity(phi->result);
      result.destination_value_name = phi->result.name;
      result.destination_value_type = phi->result.type;
    }
  }
  if (request.predecessor_block != nullptr &&
      request.successor_block != nullptr) {
    result.source_value = route5_original_source_value(
        *request.predecessor_block, *request.successor_block, record);
    if (result.source_value != nullptr) {
      result.source_value_identity = same_block_value_identity(*result.source_value);
      result.source_value_kind = result.source_value->kind;
      result.source_value_type = result.source_value->type;
      if (result.source_value->kind == bir::Value::Kind::Named) {
        result.source_value_name = result.source_value->name;
      }
    } else {
      result.source_value_identity =
          route1_source_value_identity_to_same_block(record.source_value);
    }
    result.source_producer = route5_edge_source_producer_to_mir(
        record, *request.predecessor_block, predecessor_label);
    if (record.source_memory_identity_available) {
      result.source_memory_access =
          route3_memory_access_to_mir(record.source_memory_access);
      if (record.source_memory_access.instruction_index <
          request.predecessor_block->insts.size()) {
        result.source_memory_access.inst =
            &request.predecessor_block
                 ->insts[record.source_memory_access.instruction_index];
      }
      result.source_memory_access.block_label = predecessor_label;
    }
  }
  return result;
}

[[nodiscard]] BirSelectChainDirectGlobalDependency
route2_select_chain_direct_global_dependency_to_mir(
    const bir::Route2SelectChainDirectGlobalDependencyRecord& record) {
  if (!record ||
      !record.contains_direct_global_load ||
      record.load_global == nullptr) {
    return {};
  }
  return BirSelectChainDirectGlobalDependency{
      .contains_direct_global_load = true,
      .load_global = record.load_global,
      .instruction_index = record.direct_load_instruction_index,
  };
}

[[nodiscard]] std::optional<bir::Route2SelectChainValueRecord>
find_route2_select_chain_value_record(BirSelectChainIdentityRequest request) {
  if (!request) {
    return std::nullopt;
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return std::nullopt;
  }
  const auto index = bir::route2_build_select_chain_value_index(*request.block);
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  const auto value_type = root_value_type(request);
  if (value_type != bir::TypeKind::Void) {
    const auto lookup_value =
        bir::Value::named(value_type, std::string(value_name));
    const auto* record = bir::route2_find_select_chain_value_record(
        bir::Route2SelectChainValueQuery{
            .index = &index,
            .before_instruction_index = before,
        },
        lookup_value);
    if (record == nullptr) {
      return std::nullopt;
    }
    return *record;
  }
  for (auto it = index.records.rbegin(); it != index.records.rend(); ++it) {
    const auto& record = *it;
    if (!record ||
        record.root_value.value_kind != bir::Value::Kind::Named ||
        record.root_value.name != value_name ||
        !record.root_instruction_index.has_value() ||
        *record.root_instruction_index >= before) {
      continue;
    }
    return record;
  }
  return std::nullopt;
}

[[nodiscard]] SameBlockProducerIdentity find_route1_producer_identity(
    SameBlockProducerIdentityRequest request) {
  if (!request) {
    return {};
  }
  const auto index = bir::route1_build_producer_index(*request.block);
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  for (auto it = index.records.rbegin(); it != index.records.rend(); ++it) {
    const auto& record = *it;
    if (!record ||
        !record.producer_instruction ||
        record.producer_instruction.instruction_index >= before ||
        !record.source_value ||
        record.source_value.value_kind != bir::Value::Kind::Named ||
        record.source_value.name != request.value_name) {
      continue;
    }
    if (request.value_type != bir::TypeKind::Void &&
        record.source_value.type != request.value_type) {
      return {};
    }
    return route1_producer_record_to_same_block(
        record, *request.block, request.block_label,
        request.before_instruction_index);
  }
  return {};
}

[[nodiscard]] std::optional<SameBlockScalarProducer>
find_route1_same_block_scalar_producer(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
  if (!query ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto index = bir::route1_build_producer_index(*query.block);
  const auto route1_producer = bir::route1_find_same_block_scalar_producer(
      bir::Route1SameBlockProducerQuery{
          .index = &index,
          .before_instruction_index =
              std::min(query.before_instruction_index, query.block->insts.size()),
      },
      value);
  if (!route1_producer.has_value() ||
      route1_producer->record == nullptr ||
      route1_producer->produced_value == nullptr) {
    return std::nullopt;
  }
  const auto producer = route1_producer_record_to_same_block(
      *route1_producer->record, *query.block, query.block_label,
      query.before_instruction_index);
  if (!producer) {
    return std::nullopt;
  }
  return SameBlockScalarProducer{
      .producer = producer,
      .instruction = route1_producer->instruction,
      .produced_value = route1_producer->produced_value,
      .instruction_index = route1_producer->instruction_index,
  };
}

[[nodiscard]] std::optional<SameBlockIntegerConstant>
route1_integer_constant_to_same_block(
    const std::optional<bir::Route1ImmediateIntegerConstant>& constant) {
  if (!constant.has_value()) {
    return std::nullopt;
  }
  return SameBlockIntegerConstant{
      .value = constant->value,
      .depth = constant->depth,
  };
}

[[nodiscard]] std::string_view root_value_name(
    const BirSelectChainIdentityRequest& request) {
  if (!request.root_value_name.empty()) {
    return request.root_value_name;
  }
  if (request.root_value != nullptr &&
      request.root_value->kind == bir::Value::Kind::Named) {
    return request.root_value->name;
  }
  return {};
}

[[nodiscard]] bir::TypeKind root_value_type(
    const BirSelectChainIdentityRequest& request) {
  if (request.root_value_type != bir::TypeKind::Void) {
    return request.root_value_type;
  }
  return request.root_value != nullptr ? request.root_value->type
                                      : bir::TypeKind::Void;
}

[[nodiscard]] std::string_view root_value_name(
    const BirSameBlockGlobalLoadAccessRequest& request) {
  if (!request.root_value_name.empty()) {
    return request.root_value_name;
  }
  if (request.root_value != nullptr &&
      request.root_value->kind == bir::Value::Kind::Named) {
    return request.root_value->name;
  }
  return {};
}

[[nodiscard]] bir::TypeKind root_value_type(
    const BirSameBlockGlobalLoadAccessRequest& request) {
  if (request.root_value_type != bir::TypeKind::Void) {
    return request.root_value_type;
  }
  return request.root_value != nullptr ? request.root_value->type
                                      : bir::TypeKind::Void;
}

[[nodiscard]] std::string_view root_value_name(
    const BirSameBlockLoadLocalSourceRequest& request) {
  if (!request.root_value_name.empty()) {
    return request.root_value_name;
  }
  if (request.root_value != nullptr &&
      request.root_value->kind == bir::Value::Kind::Named) {
    return request.root_value->name;
  }
  return {};
}

[[nodiscard]] bir::TypeKind root_value_type(
    const BirSameBlockLoadLocalSourceRequest& request) {
  if (request.root_value_type != bir::TypeKind::Void) {
    return request.root_value_type;
  }
  return request.root_value != nullptr ? request.root_value->type
                                      : bir::TypeKind::Void;
}

[[nodiscard]] std::string_view root_value_name(
    const BirCurrentBlockPublicationIdentityRequest& request) {
  if (!request.root_value_name.empty()) {
    return request.root_value_name;
  }
  if (request.root_value != nullptr &&
      request.root_value->kind == bir::Value::Kind::Named) {
    return request.root_value->name;
  }
  return {};
}

[[nodiscard]] bir::TypeKind root_value_type(
    const BirCurrentBlockPublicationIdentityRequest& request) {
  if (request.root_value_type != bir::TypeKind::Void) {
    return request.root_value_type;
  }
  return request.root_value != nullptr ? request.root_value->type
                                      : bir::TypeKind::Void;
}

[[nodiscard]] std::string_view destination_value_name(
    const BirBlockEntryPublicationIdentityRequest& request) {
  if (!request.destination_value_name.empty()) {
    return request.destination_value_name;
  }
  if (request.destination_value != nullptr &&
      request.destination_value->kind == bir::Value::Kind::Named) {
    return request.destination_value->name;
  }
  return {};
}

[[nodiscard]] std::string_view destination_value_name(
    const BirCfgEdgePublicationSourceRequest& request) {
  if (!request.destination_value_name.empty()) {
    return request.destination_value_name;
  }
  if (request.destination_value != nullptr &&
      request.destination_value->kind == bir::Value::Kind::Named) {
    return request.destination_value->name;
  }
  return {};
}

[[nodiscard]] bir::TypeKind destination_value_type(
    const BirBlockEntryPublicationIdentityRequest& request) {
  if (request.destination_value_type != bir::TypeKind::Void) {
    return request.destination_value_type;
  }
  return request.destination_value != nullptr ? request.destination_value->type
                                             : bir::TypeKind::Void;
}

[[nodiscard]] bir::TypeKind destination_value_type(
    const BirCfgEdgePublicationSourceRequest& request) {
  if (request.destination_value_type != bir::TypeKind::Void) {
    return request.destination_value_type;
  }
  return request.destination_value != nullptr ? request.destination_value->type
                                             : bir::TypeKind::Void;
}

[[nodiscard]] bool successor_block_label_matches(
    const BirBlockEntryPublicationIdentityRequest& request) {
  if (request.successor_block == nullptr) {
    return false;
  }
  if (request.successor_label_id != c4c::kInvalidBlockLabel &&
      request.successor_block->label_id != c4c::kInvalidBlockLabel &&
      request.successor_label_id != request.successor_block->label_id) {
    return false;
  }
  return request.successor_label.empty() ||
         request.successor_label == request.successor_block->label;
}

[[nodiscard]] bool predecessor_block_label_matches(
    const BirCfgEdgePublicationSourceRequest& request) {
  if (request.predecessor_block == nullptr) {
    return false;
  }
  if (request.predecessor_label_id != c4c::kInvalidBlockLabel &&
      request.predecessor_block->label_id != c4c::kInvalidBlockLabel &&
      request.predecessor_label_id != request.predecessor_block->label_id) {
    return false;
  }
  return request.predecessor_label.empty() ||
         request.predecessor_label == request.predecessor_block->label;
}

[[nodiscard]] bool successor_block_label_matches(
    const BirCfgEdgePublicationSourceRequest& request) {
  if (request.successor_block == nullptr) {
    return false;
  }
  if (request.successor_label_id != c4c::kInvalidBlockLabel &&
      request.successor_block->label_id != c4c::kInvalidBlockLabel &&
      request.successor_label_id != request.successor_block->label_id) {
    return false;
  }
  return request.successor_label.empty() ||
         request.successor_label == request.successor_block->label;
}

[[nodiscard]] bool successor_block_label_matches(
    const BirCurrentBlockJoinSourceRequest& request) {
  if (request.successor_block == nullptr) {
    return false;
  }
  if (request.successor_label_id != c4c::kInvalidBlockLabel &&
      request.successor_block->label_id != c4c::kInvalidBlockLabel &&
      request.successor_label_id != request.successor_block->label_id) {
    return false;
  }
  return request.successor_label.empty() ||
         request.successor_label == request.successor_block->label;
}

[[nodiscard]] bool phi_incoming_label_matches(
    const bir::PhiIncoming& incoming,
    const BirCfgEdgePublicationSourceRequest& request) {
  if (request.predecessor_label_id != c4c::kInvalidBlockLabel &&
      incoming.label_id != c4c::kInvalidBlockLabel) {
    return incoming.label_id == request.predecessor_label_id;
  }
  if (request.predecessor_block != nullptr &&
      request.predecessor_block->label_id != c4c::kInvalidBlockLabel &&
      incoming.label_id != c4c::kInvalidBlockLabel) {
    return incoming.label_id == request.predecessor_block->label_id;
  }
  const auto label = !request.predecessor_label.empty()
                         ? request.predecessor_label
                         : request.predecessor_block != nullptr
                               ? std::string_view{request.predecessor_block->label}
                               : std::string_view{};
  return !label.empty() && incoming.label == label;
}

void append_unique_value_identity(
    std::vector<SameBlockValueIdentity>& values,
    SameBlockValueIdentity value) {
  if (!value) {
    return;
  }
  const auto matches = [&](const SameBlockValueIdentity& existing) {
    if (!value.name.empty() || !existing.name.empty()) {
      return value.name == existing.name && value.type == existing.type;
    }
    return value.immediate_constant == existing.immediate_constant &&
           value.type == existing.type;
  };
  if (std::find_if(values.begin(), values.end(), matches) == values.end()) {
    values.push_back(value);
  }
}

void append_bir_expression_operands(
    std::vector<SameBlockValueIdentity>& pending,
    const bir::Inst& producer) {
  auto append_operand = [&](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Named) {
      append_unique_value_identity(pending, same_block_value_identity(value));
    }
  };
  std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          append_operand(typed_inst.lhs);
          append_operand(typed_inst.rhs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          append_operand(typed_inst.operand);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          append_operand(typed_inst.lhs);
          append_operand(typed_inst.rhs);
          append_operand(typed_inst.true_value);
          append_operand(typed_inst.false_value);
        }
      },
      producer);
}

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value,
    unsigned depth);

}  // namespace

[[nodiscard]] BirMemoryAccessIdentity find_bir_memory_access_identity(
    BirMemoryAccessIdentityRequest request) {
  if (!request || request.instruction_index >= request.block->insts.size()) {
    return {};
  }
  if (!request.block_label.empty() && request.block_label != request.block->label) {
    return {};
  }
  const auto index = bir::route3_build_memory_access_index(*request.block);
  const auto* record = bir::route3_find_memory_access_record(
      index,
      request.instruction_index,
      route3_node_kind_from_mir(request.node_kind));
  if (record == nullptr) {
    return {};
  }
  auto identity = route3_memory_access_to_mir(*record);
  identity.block_label = normalized_block_label(*request.block, request.block_label);
  return identity;
}

[[nodiscard]] BirCurrentBlockPublicationIdentity
find_bir_current_block_publication_identity(
    BirCurrentBlockPublicationIdentityRequest request) {
  if (!request ||
      (!request.block_label.empty() && request.block_label != request.block->label)) {
    return {};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {};
  }
  const auto value_type = root_value_type(request);
  bir::Function function;
  function.blocks.push_back(*request.block);
  const auto& indexed_block = function.blocks.front();
  const auto index = bir::route4_build_publication_availability_index(function);
  const auto publication = bir::route4_find_current_block_publication(
      index,
      indexed_block,
      bir::Value::named(value_type, std::string{value_name}),
      request.before_instruction_index);
  const auto producer = route4_current_block_record_to_same_block(
      publication, *request.block, request.block_label);
  if (!producer || !producer.produced_value ||
      producer.produced_value.name != value_name) {
    return {};
  }
  return BirCurrentBlockPublicationIdentity{
      .available = true,
      .source_producer = producer,
      .instruction = producer.inst,
      .produced_value = producer.produced_value.value,
      .produced_value_identity = producer.produced_value,
      .produced_value_name = producer.produced_value.name,
      .produced_value_type = producer.produced_value.type,
      .instruction_index = producer.instruction_index,
      .value_name = value_name,
      .source_producer_kind = producer.kind,
  };
}

[[nodiscard]] BirBlockEntryPublicationIdentity
find_bir_block_entry_publication_identity(
    BirBlockEntryPublicationIdentityRequest request) {
  BirBlockEntryPublicationIdentity result{
      .destination_value_id = request.destination_value_id,
      .destination_value_name_id = request.destination_value_name_id,
  };
  if (request.successor_block != nullptr) {
    result.successor_label =
        normalized_block_label(*request.successor_block, request.successor_label);
    result.successor_label_id = request.successor_block->label_id != c4c::kInvalidBlockLabel
                                    ? request.successor_block->label_id
                                    : request.successor_label_id;
  }
  if (request.successor_block == nullptr || !successor_block_label_matches(request)) {
    result.status = BirBlockEntryPublicationStatus::MissingSuccessorLabel;
    return result;
  }

  const auto value_name = destination_value_name(request);
  const auto value_type = destination_value_type(request);
  if (value_name.empty()) {
    result.status = BirBlockEntryPublicationStatus::MissingDestinationValue;
    result.destination_value_type = value_type;
    return result;
  }
  result.destination_value_name = value_name;
  result.destination_value_type = value_type;

  for (std::size_t index = 0; index < request.successor_block->insts.size();
       ++index) {
    const auto& inst = request.successor_block->insts[index];
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    if (phi->result.kind != bir::Value::Kind::Named ||
        phi->result.name != value_name) {
      continue;
    }
    if (value_type != bir::TypeKind::Void && phi->result.type != value_type) {
      result.status = BirBlockEntryPublicationStatus::MissingDestinationValue;
      return result;
    }
    result.available = true;
    result.status = BirBlockEntryPublicationStatus::Available;
    result.instruction = &inst;
    result.phi = phi;
    result.instruction_index = index;
    result.destination_value = &phi->result;
    result.destination_value_identity = same_block_value_identity(phi->result);
    result.destination_value_name = phi->result.name;
    result.destination_value_type = phi->result.type;
    return result;
  }

  result.status = BirBlockEntryPublicationStatus::MissingPublication;
  return result;
}

[[nodiscard]] BirCfgEdgePublicationSourceIdentity
find_bir_cfg_edge_publication_source_identity(
    BirCfgEdgePublicationSourceRequest request) {
  BirCfgEdgePublicationSourceIdentity result{
      .destination_value_id = request.destination_value_id,
      .destination_value_name_id = request.destination_value_name_id,
  };
  if (request.predecessor_block != nullptr) {
    result.predecessor_label =
        normalized_block_label(*request.predecessor_block,
                               request.predecessor_label);
    result.predecessor_label_id =
        request.predecessor_block->label_id != c4c::kInvalidBlockLabel
            ? request.predecessor_block->label_id
            : request.predecessor_label_id;
  }
  if (request.successor_block != nullptr) {
    result.successor_label =
        normalized_block_label(*request.successor_block, request.successor_label);
    result.successor_label_id =
        request.successor_block->label_id != c4c::kInvalidBlockLabel
            ? request.successor_block->label_id
            : request.successor_label_id;
  }
  if (request.predecessor_block == nullptr ||
      !predecessor_block_label_matches(request)) {
    result.status = BirCfgEdgePublicationSourceStatus::MissingPredecessorLabel;
    return result;
  }
  if (request.successor_block == nullptr ||
      !successor_block_label_matches(request)) {
    result.status = BirCfgEdgePublicationSourceStatus::MissingSuccessorLabel;
    return result;
  }

  const auto value_name = destination_value_name(request);
  const auto value_type = destination_value_type(request);
  if (value_name.empty()) {
    result.status = BirCfgEdgePublicationSourceStatus::MissingDestinationValue;
    result.destination_value_type = value_type;
    return result;
  }
  result.destination_value_name = value_name;
  result.destination_value_type = value_type;

  const bir::Value* destination_value = request.destination_value;
  for (const auto& inst : request.successor_block->insts) {
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    if (phi->result.kind == bir::Value::Kind::Named &&
        phi->result.name == value_name &&
        (value_type == bir::TypeKind::Void || phi->result.type == value_type)) {
      destination_value = &phi->result;
      break;
    }
  }
  const auto synthesized_destination =
      bir::Value::named(value_type, std::string{value_name});
  if (destination_value == nullptr) {
    destination_value = &synthesized_destination;
  }

  bir::Function route5_function;
  route5_function.blocks.push_back(*request.predecessor_block);
  for (const auto& inst : request.successor_block->insts) {
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    for (const auto& incoming : phi->incomings) {
      const bool matches_request =
          (incoming.label_id != c4c::kInvalidBlockLabel &&
           request.predecessor_block->label_id != c4c::kInvalidBlockLabel &&
           incoming.label_id == request.predecessor_block->label_id) ||
          (!incoming.label.empty() &&
           incoming.label == request.predecessor_block->label);
      if (matches_request) {
        continue;
      }
      const auto already_added =
          std::find_if(route5_function.blocks.begin(),
                       route5_function.blocks.end(),
                       [&](const bir::Block& block) {
                         if (incoming.label_id != c4c::kInvalidBlockLabel &&
                             block.label_id != c4c::kInvalidBlockLabel) {
                           return block.label_id == incoming.label_id;
                         }
                         return !incoming.label.empty() &&
                                block.label == incoming.label;
                       }) != route5_function.blocks.end();
      if (already_added) {
        continue;
      }
      route5_function.blocks.push_back(bir::Block{
          .label = incoming.label,
          .insts = {},
          .terminator = {},
          .label_id = incoming.label_id,
      });
    }
  }
  route5_function.blocks.push_back(*request.successor_block);
  const auto& route5_predecessor = route5_function.blocks.front();
  const auto& route5_successor = route5_function.blocks.back();
  const auto route5_index =
      bir::route5_build_edge_join_source_index(route5_function);
  const auto route5_edge = bir::route5_find_cfg_edge_publication(
      route5_index, route5_predecessor, route5_successor, *destination_value);
  return route5_edge_record_to_mir(
      route5_edge, request, result.predecessor_label, result.successor_label);
}

[[nodiscard]] BirCurrentBlockJoinSourceIdentity
find_bir_current_block_join_source_identity(
    BirCurrentBlockJoinSourceRequest request) {
  BirCurrentBlockJoinSourceIdentity result;
  if (request.successor_block != nullptr) {
    result.successor_label =
        normalized_block_label(*request.successor_block, request.successor_label);
    result.successor_label_id =
        request.successor_block->label_id != c4c::kInvalidBlockLabel
            ? request.successor_block->label_id
            : request.successor_label_id;
  }
  if (request.successor_block == nullptr) {
    result.status = BirCurrentBlockJoinSourceStatus::MissingBlock;
    return result;
  }
  if (!successor_block_label_matches(request)) {
    result.status = BirCurrentBlockJoinSourceStatus::MissingSuccessorLabel;
    return result;
  }

  bool saw_phi = false;
  for (std::size_t index = 0; index < request.successor_block->insts.size();
       ++index) {
    const auto& inst = request.successor_block->insts[index];
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    saw_phi = true;
    for (const auto& incoming : phi->incomings) {
      BirCurrentBlockJoinSourceFact fact{
          .predecessor_label = incoming.label,
          .predecessor_label_id = incoming.label_id,
          .successor_label = result.successor_label,
          .successor_label_id = result.successor_label_id,
          .destination_instruction = &inst,
          .destination_phi = phi,
          .destination_instruction_index = index,
          .destination_value = &phi->result,
          .destination_value_identity = same_block_value_identity(phi->result),
          .destination_value_name = named_value_name(phi->result),
          .destination_value_type = phi->result.type,
          .source_value = &incoming.value,
          .source_value_identity = same_block_value_identity(incoming.value),
          .source_value_name = named_value_name(incoming.value),
          .source_value_kind = incoming.value.kind,
          .source_value_type = incoming.value.type,
      };
      append_unique_value_identity(result.source_values,
                                   fact.destination_value_identity);
      if (incoming.value.kind == bir::Value::Kind::Named) {
        append_unique_value_identity(result.source_values,
                                     fact.source_value_identity);
        append_unique_value_identity(result.incoming_expression_values,
                                     fact.source_value_identity);
        const auto producer = find_same_block_producer_identity(
            SameBlockProducerIdentityRequest{
                .block = request.successor_block,
                .block_label = result.successor_label,
                .value_name = incoming.value.name,
                .value_type = incoming.value.type,
                .before_instruction_index =
                    request.successor_block->insts.size(),
            });
        if (!producer) {
          fact.status = BirCurrentBlockJoinSourceStatus::MissingSourceProducer;
          result.facts.push_back(fact);
          continue;
        }
        fact.source_producer = producer;
        fact.source_producer_kind = producer.kind;
        fact.source_producer_instruction_index = producer.instruction_index;
      }
      fact.status = BirCurrentBlockJoinSourceStatus::Available;
      result.facts.push_back(fact);
    }
  }

  if (!saw_phi || result.facts.empty()) {
    result.status = BirCurrentBlockJoinSourceStatus::MissingPublication;
    return result;
  }

  std::vector<SameBlockValueIdentity> pending =
      result.incoming_expression_values;
  std::vector<SameBlockValueIdentity> processed;
  while (!pending.empty()) {
    const auto value = pending.back();
    pending.pop_back();
    if (value.name.empty()) {
      continue;
    }
    const auto already_processed =
        std::find_if(processed.begin(), processed.end(),
                     [&](const SameBlockValueIdentity& existing) {
                       return existing.name == value.name &&
                              existing.type == value.type;
                     }) != processed.end();
    if (already_processed) {
      continue;
    }
    append_unique_value_identity(processed, value);
    append_unique_value_identity(result.incoming_expression_values, value);
    const auto producer = find_same_block_producer_identity(
        SameBlockProducerIdentityRequest{
            .block = request.successor_block,
            .block_label = result.successor_label,
            .value_name = value.name,
            .value_type = value.type,
            .before_instruction_index = request.successor_block->insts.size(),
        });
    if (producer.inst != nullptr) {
      append_bir_expression_operands(pending, *producer.inst);
    }
  }

  result.available =
      std::all_of(result.facts.begin(), result.facts.end(), [](const auto& fact) {
        return fact.status == BirCurrentBlockJoinSourceStatus::Available;
      });
  result.status = result.available
                      ? BirCurrentBlockJoinSourceStatus::Available
                      : BirCurrentBlockJoinSourceStatus::MissingSourceProducer;
  return result;
}

[[nodiscard]] BirSameBlockGlobalLoadAccessIdentity
find_bir_same_block_global_load_access_identity(
    BirSameBlockGlobalLoadAccessRequest request) {
  if (!request ||
      (!request.block_label.empty() && request.block_label != request.block->label)) {
    return {};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {};
  }
  const auto value_type = root_value_type(request);
  const auto index = bir::route3_build_memory_access_index(*request.block);
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  const auto record =
      find_route3_global_load_access(index, before, value_name, value_type);
  if (!record) {
    return {};
  }
  const auto* load_global =
      record.load_access.instruction != nullptr
          ? std::get_if<bir::LoadGlobalInst>(record.load_access.instruction)
          : nullptr;
  if (load_global == nullptr) {
    return {};
  }
  const auto memory_access = route3_memory_access_to_mir(record.load_access);
  const auto producer =
      route3_load_access_to_producer(record.load_access,
                                     request.before_instruction_index);
  if (!memory_access ||
      memory_access.base_kind != BirMemoryAccessBaseKind::GlobalSymbol ||
      memory_access.result_value_name != value_name) {
    return {};
  }
  return BirSameBlockGlobalLoadAccessIdentity{
      .producer = producer,
      .memory_access = memory_access,
      .load_global = load_global,
      .result_value = producer.produced_value,
      .root_value_name = value_name,
      .root_value_type = value_type,
      .before_instruction_index = request.before_instruction_index,
  };
}

[[nodiscard]] BirSameBlockLoadLocalSourceIdentity
find_bir_same_block_load_local_source_identity(
    BirSameBlockLoadLocalSourceRequest request) {
  if (!request ||
      (!request.block_label.empty() && request.block_label != request.block->label)) {
    return {};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {};
  }
  const auto value_type = root_value_type(request);
  const auto index = bir::route3_build_memory_access_index(*request.block);
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  const auto record =
      find_route3_load_local_source(index, before, value_name, value_type);
  if (!record) {
    return {};
  }
  const auto* load_local =
      record.load_access.instruction != nullptr
          ? std::get_if<bir::LoadLocalInst>(record.load_access.instruction)
          : nullptr;
  if (load_local == nullptr) {
    return {};
  }
  const auto memory_access = route3_memory_access_to_mir(record.load_access);
  const auto producer =
      route3_load_access_to_producer(record.load_access,
                                     request.before_instruction_index);
  if (!memory_access ||
      memory_access.base_kind != BirMemoryAccessBaseKind::LocalSlot ||
      memory_access.result_value_name != value_name) {
    return {};
  }
  return BirSameBlockLoadLocalSourceIdentity{
      .producer = producer,
      .memory_access = memory_access,
      .load_local = load_local,
      .result_value = producer.produced_value,
      .root_value_name = value_name,
      .root_value_type = value_type,
      .before_instruction_index = request.before_instruction_index,
  };
}

[[nodiscard]] SameBlockBinaryProducer find_same_block_binary_producer(
    const bir::Block* block,
    const bir::Value& value) {
  if (block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }
  for (std::size_t index = 0; index < block->insts.size(); ++index) {
    const auto* binary = std::get_if<bir::BinaryInst>(&block->insts[index]);
    if (binary != nullptr &&
        binary->result.kind == bir::Value::Kind::Named &&
        binary->result.name == value.name) {
      return SameBlockBinaryProducer{.binary = binary, .instruction_index = index};
    }
  }
  return {};
}

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }
  const auto before = std::min(before_instruction_index, block->insts.size());
  for (std::size_t index = before; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto* select = std::get_if<bir::SelectInst>(&block->insts[candidate_index]);
    if (select != nullptr &&
        select->result.kind == bir::Value::Kind::Named &&
        select->result.name == value.name) {
      return SameBlockSelectProducer{.select = select,
                                     .instruction_index = candidate_index};
    }
  }
  return {};
}

[[nodiscard]] SameBlockProducerRecord find_same_block_named_producer_record(
    const bir::Block* block,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  return find_same_block_producer_identity(SameBlockProducerIdentityRequest{
      .block = block,
      .value_name = value_name,
      .before_instruction_index = before_instruction_index,
  });
}

[[nodiscard]] SameBlockProducerIdentity find_same_block_producer_identity(
    SameBlockProducerIdentityRequest request) {
  return find_route1_producer_identity(request);
}

[[nodiscard]] SameBlockProducerIdentity find_bir_select_chain_source_producer(
    BirSelectChainIdentityRequest request) {
  const auto record = find_route2_select_chain_value_record(request);
  if (!record.has_value() || request.block == nullptr) {
    return {};
  }
  return route2_select_chain_producer_record_to_same_block(
      *record, *request.block, request.block_label,
      request.before_instruction_index);
}

[[nodiscard]] BirSelectChainDirectGlobalDependency
find_bir_select_chain_direct_global_dependency(
    BirSelectChainIdentityRequest request) {
  const auto record = find_route2_select_chain_value_record(request);
  if (!record.has_value()) {
    return {};
  }
  return route2_select_chain_direct_global_dependency_to_mir(
      record->direct_global_dependency);
}

[[nodiscard]] bool find_bir_select_chain_scalar_materialization_eligibility(
    BirSelectChainIdentityRequest request) {
  const auto record = find_route2_select_chain_value_record(request);
  return record.has_value() && record->scalar_materialization_available;
}

[[nodiscard]] BirSelectChainIdentity find_bir_select_chain_identity(
    BirSelectChainIdentityRequest request) {
  const auto record = find_route2_select_chain_value_record(request);
  if (!record.has_value() || request.block == nullptr) {
    return BirSelectChainIdentity{
        .root_value_name = root_value_name(request),
    };
  }
  const auto root = route2_select_chain_producer_record_to_same_block(
      *record, *request.block, request.block_label,
      request.before_instruction_index);
  if (!root) {
    return BirSelectChainIdentity{
        .root_value_name = root_value_name(request),
    };
  }
  return BirSelectChainIdentity{
      .root_producer = root,
      .root_value = route2_source_value_identity_to_same_block(
          record->root_value),
      .root_value_name = record->root_value_name,
      .root_is_select = record->root_is_select,
      .root_instruction_index = record->root_instruction_index,
      .direct_global_dependency =
          route2_select_chain_direct_global_dependency_to_mir(
              record->direct_global_dependency),
      .scalar_materialization_available =
          record->scalar_materialization_available,
  };
}

[[nodiscard]] std::optional<SameBlockScalarProducer>
find_same_block_scalar_producer(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
  return find_route1_same_block_scalar_producer(query, value);
}

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const bir::Block* block,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  return find_same_block_named_producer_record(block, value_name, before_instruction_index)
      .inst;
}

[[nodiscard]] std::optional<SameBlockProducerIndex> producer_instruction_index_record(
    const bir::Block* block,
    const bir::Inst* producer) {
  if (block == nullptr || producer == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < block->insts.size(); ++index) {
    if (&block->insts[index] == producer) {
      return SameBlockProducerIndex{.producer = producer, .instruction_index = index};
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const bir::Block* block,
    const bir::Inst* producer) {
  const auto record = producer_instruction_index_record(block, producer);
  if (!record.has_value()) {
    return std::nullopt;
  }
  return record->instruction_index;
}

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    const bir::Block* block,
    const bir::Value& value,
    unsigned depth) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return SameBlockIntegerConstant{.value = value.immediate, .depth = depth};
  }
  if (block == nullptr) {
    return std::nullopt;
  }
  const auto index = bir::route1_build_producer_index(*block);
  return route1_integer_constant_to_same_block(
      bir::route1_evaluate_same_block_integer_constant(
          bir::Route1SameBlockProducerQuery{
              .index = &index,
              .before_instruction_index = block->insts.size(),
          },
          value,
          depth));
}

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return SameBlockIntegerConstant{.value = value.immediate, .depth = 0U};
  }
  if (!query) {
    return std::nullopt;
  }
  const auto index = bir::route1_build_producer_index(*query.block);
  return route1_integer_constant_to_same_block(
      bir::route1_evaluate_same_block_integer_constant(
          bir::Route1SameBlockProducerQuery{
              .index = &index,
              .before_instruction_index =
                  std::min(query.before_instruction_index,
                           query.block->insts.size()),
          },
          value,
          0U));
}

[[nodiscard]] bool select_chain_contains_dependency(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    DependencyPredicate matches,
    unsigned depth) {
  if (matches == nullptr ||
      depth > 64U ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  const auto producer =
      find_same_block_named_producer_record(block, value.name, before_instruction_index);
  if (!producer) {
    return false;
  }
  const DependencyTraversalRecord record{
      .producer = producer.inst,
      .instruction_index = producer.instruction_index,
      .source_value = &value,
      .depth = depth,
      .kind = producer.kind,
  };
  if (matches(record)) {
    return true;
  }
  const auto nested_before = producer.instruction_index;
  if (const auto* select = std::get_if<bir::SelectInst>(producer.inst);
      select != nullptr) {
    return select_chain_contains_dependency(
               block, select->true_value, nested_before, matches, depth + 1) ||
           select_chain_contains_dependency(
               block, select->false_value, nested_before, matches, depth + 1);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(producer.inst);
      cast != nullptr) {
    return select_chain_contains_dependency(
        block, cast->operand, nested_before, matches, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer.inst);
      binary != nullptr) {
    return select_chain_contains_dependency(
               block, binary->lhs, nested_before, matches, depth + 1) ||
           select_chain_contains_dependency(
               block, binary->rhs, nested_before, matches, depth + 1);
  }
  return false;
}

}  // namespace c4c::backend::mir
