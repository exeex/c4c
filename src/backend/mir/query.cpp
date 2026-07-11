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

[[nodiscard]] BirMemoryAccessNodeKind named_memory_kind_to_mir(
    bir::BirMemoryAccessKind kind) {
  switch (kind) {
    case bir::BirMemoryAccessKind::LoadLocal:
      return BirMemoryAccessNodeKind::LoadLocal;
    case bir::BirMemoryAccessKind::LoadGlobal:
      return BirMemoryAccessNodeKind::LoadGlobal;
    case bir::BirMemoryAccessKind::StoreLocal:
      return BirMemoryAccessNodeKind::StoreLocal;
    case bir::BirMemoryAccessKind::StoreGlobal:
      return BirMemoryAccessNodeKind::StoreGlobal;
    case bir::BirMemoryAccessKind::Unknown:
      return BirMemoryAccessNodeKind::Unknown;
  }
  return BirMemoryAccessNodeKind::Unknown;
}

[[nodiscard]] BirMemoryAccessBaseKind named_memory_base_to_mir(
    bir::BirMemoryBaseKind kind) {
  switch (kind) {
    case bir::BirMemoryBaseKind::LocalSlot:
      return BirMemoryAccessBaseKind::LocalSlot;
    case bir::BirMemoryBaseKind::GlobalSymbol:
      return BirMemoryAccessBaseKind::GlobalSymbol;
    case bir::BirMemoryBaseKind::PointerValue:
      return BirMemoryAccessBaseKind::PointerValue;
    case bir::BirMemoryBaseKind::StringConstant:
      return BirMemoryAccessBaseKind::StringConstant;
    case bir::BirMemoryBaseKind::None:
      return BirMemoryAccessBaseKind::None;
  }
  return BirMemoryAccessBaseKind::None;
}

[[nodiscard]] BirMemoryAccessIdentity named_memory_access_to_mir(
    const bir::BirMemoryAccessResult& result) {
  if (!result) {
    return BirMemoryAccessIdentity{.status = result.status};
  }
  const auto node_kind = named_memory_kind_to_mir(result.kind);
  const auto base_kind = named_memory_base_to_mir(result.base_kind);
  if (result.instruction == nullptr || node_kind == BirMemoryAccessNodeKind::Unknown ||
      base_kind == BirMemoryAccessBaseKind::None) {
    return BirMemoryAccessIdentity{.status = bir::BirViewStatus::Incomplete};
  }
  return BirMemoryAccessIdentity{
      .status = result.status,
      .inst = result.instruction,
      .block_label = result.block_label,
      .instruction_index = result.instruction_index,
      .node_kind = node_kind,
      .result_value_name = result.result_value_name,
      .stored_value_name = result.stored_value_name,
      .address_space = result.address_space,
      .is_volatile = result.is_volatile,
      .base_kind = base_kind,
      .local_slot_name = result.local_slot_name,
      .local_slot_id = result.local_slot_id,
      .global_name = result.global_name,
      .global_name_id = result.global_name_id,
      .pointer_base = result.pointer_base,
      .pointer_value_name = result.pointer_base_name,
      .string_constant_name = result.string_constant_name,
      .string_constant_name_id = result.string_constant_name_id,
      .result_value = result.result_value,
      .stored_value = result.stored_value,
      .byte_offset = result.byte_offset,
      .size_bytes = result.size_bytes,
      .align_bytes = result.align_bytes,
  };
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
      .status = bir::BirViewStatus::Available,
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
      .byte_offset = record.byte_offset,
      .size_bytes = record.size_bytes,
      .align_bytes = record.align_bytes,
  };
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

[[nodiscard]] SameBlockProducerKind producer_view_kind_to_same_block_kind(
    bir::BirProducerKind kind) {
  switch (kind) {
    case bir::BirProducerKind::Binary:
      return SameBlockProducerKind::Binary;
    case bir::BirProducerKind::Cast:
      return SameBlockProducerKind::Cast;
    case bir::BirProducerKind::SelectMaterialization:
      return SameBlockProducerKind::Select;
    case bir::BirProducerKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case bir::BirProducerKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case bir::BirProducerKind::Unknown:
      return SameBlockProducerKind::Unknown;
  }
  return SameBlockProducerKind::Unknown;
}

[[nodiscard]] SameBlockProducerIdentity producer_view_result_to_same_block(
    const bir::BirProducerResult& result,
    const bir::Block& block,
    std::size_t before_instruction_index) {
  if (!result || result.produced_value == nullptr ||
      result.instruction_index >= block.insts.size()) {
    return {};
  }
  const auto kind = producer_view_kind_to_same_block_kind(result.kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  return SameBlockProducerIdentity{
      .inst = &block.insts[result.instruction_index],
      .instruction_index = result.instruction_index,
      .kind = kind,
      .block_label = result.block_label,
      .before_instruction_index = before_instruction_index,
      .produced_value = same_block_value_identity(*result.produced_value),
      .materialization_available = result.scalar_materialization_available,
  };
}

[[nodiscard]] std::string_view normalized_block_label(
    const bir::Block& block,
    std::string_view requested_label) {
  if (!requested_label.empty()) {
    return requested_label;
  }
  return block.label;
}

[[nodiscard]] SameBlockProducerKind producer_view_kind_to_select_chain_kind(
    bir::BirProducerKind kind) {
  switch (kind) {
    case bir::BirProducerKind::Binary:
      return SameBlockProducerKind::Binary;
    case bir::BirProducerKind::Cast:
      return SameBlockProducerKind::Cast;
    case bir::BirProducerKind::SelectMaterialization:
      return SameBlockProducerKind::Select;
    case bir::BirProducerKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case bir::BirProducerKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case bir::BirProducerKind::Unknown:
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

[[nodiscard]] SameBlockProducerIdentity
select_chain_producer_result_to_same_block(
    const bir::BirProducerResult& result,
    const bir::Block& block,
    std::string_view block_label,
    std::size_t before_instruction_index) {
  if (!result || result.produced_value == nullptr ||
      result.instruction_index >= block.insts.size()) {
    return {};
  }
  const auto kind = producer_view_kind_to_select_chain_kind(result.kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  return SameBlockProducerIdentity{
      .inst = &block.insts[result.instruction_index],
      .instruction_index = result.instruction_index,
      .kind = kind,
      .block_label = normalized_block_label(block, block_label),
      .before_instruction_index = before_instruction_index,
      .produced_value = same_block_value_identity(*result.produced_value),
      .materialization_available = result.scalar_materialization_available,
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

[[nodiscard]] BirCurrentBlockJoinSourceStatus route5_join_status_to_mir(
    bir::Route5PublicationStatus status) {
  switch (status) {
    case bir::Route5PublicationStatus::Available:
      return BirCurrentBlockJoinSourceStatus::Available;
    case bir::Route5PublicationStatus::MissingSourceProducer:
    case bir::Route5PublicationStatus::MissingSourceMemoryAccess:
    case bir::Route5PublicationStatus::IncompleteSourceMemoryAccess:
      return BirCurrentBlockJoinSourceStatus::MissingSourceProducer;
    case bir::Route5PublicationStatus::Unavailable:
    case bir::Route5PublicationStatus::NoSource:
    case bir::Route5PublicationStatus::MemorySource:
    case bir::Route5PublicationStatus::MissingPredecessor:
    case bir::Route5PublicationStatus::MissingSuccessor:
    case bir::Route5PublicationStatus::MissingDestination:
    case bir::Route5PublicationStatus::MissingPublication:
    case bir::Route5PublicationStatus::MissingSourceValue:
    case bir::Route5PublicationStatus::NoMatch:
      return BirCurrentBlockJoinSourceStatus::MissingPublication;
  }
  return BirCurrentBlockJoinSourceStatus::MissingPublication;
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

[[nodiscard]] SameBlockProducerIdentity route5_join_source_producer_to_mir(
    const bir::Route5CurrentBlockJoinSourceRecord& record,
    const bir::Block& successor_block,
    std::string_view successor_label) {
  if (!record.source_producer_instruction_index.has_value() ||
      *record.source_producer_instruction_index >= successor_block.insts.size()) {
    return {};
  }
  const auto kind = route5_publication_source_kind_to_same_block_kind(
      record.source_producer_kind);
  if (kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  const auto& inst =
      successor_block.insts[*record.source_producer_instruction_index];
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
      .block_label = normalized_block_label(successor_block, successor_label),
      .before_instruction_index = successor_block.insts.size(),
      .produced_value = same_block_value_identity(*produced_value),
      .materialization_available =
          same_block_producer_kind_has_materialization(kind),
  };
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

[[nodiscard]] bir::Route5CurrentBlockJoinSourceRecord
route5_missing_current_block_join_source_record(
    const BirCurrentBlockJoinSourceRequest& request,
    const bir::PhiInst* phi,
    const bir::PhiIncoming* incoming,
    std::size_t instruction_index,
    bir::Route5PublicationStatus status) {
  bir::Route5CurrentBlockJoinSourceRecord record{
      .status = status,
      .successor_block = request.successor_block,
      .successor_label =
          request.successor_block != nullptr ? request.successor_block->label
                                             : std::string_view{},
      .successor_label_id =
          request.successor_block != nullptr ? request.successor_block->label_id
                                             : c4c::kInvalidBlockLabel,
      .predecessor_label = incoming != nullptr ? incoming->label : std::string_view{},
      .predecessor_label_id =
          incoming != nullptr ? incoming->label_id : c4c::kInvalidBlockLabel,
      .destination_instruction =
          request.successor_block != nullptr &&
                  instruction_index < request.successor_block->insts.size()
              ? &request.successor_block->insts[instruction_index]
              : nullptr,
      .destination_phi = phi,
      .destination_instruction_index = instruction_index,
  };
  if (phi != nullptr) {
    record.destination_value = bir::route1_source_value_identity(phi->result);
    record.destination_value_name =
        phi->result.kind == bir::Value::Kind::Named
            ? std::string_view{phi->result.name}
            : std::string_view{};
    record.destination_value_type = phi->result.type;
  }
  if (incoming != nullptr) {
    record.source_value = bir::route1_source_value_identity(incoming->value);
    record.source_value_name =
        incoming->value.kind == bir::Value::Kind::Named
            ? std::string_view{incoming->value.name}
            : std::string_view{};
    record.source_value_kind = incoming->value.kind;
    record.source_value_type = incoming->value.type;
  }
  return record;
}

[[nodiscard]] bool route5_index_block_matches(
    const bir::Block* record_block,
    std::string_view record_label,
    c4c::BlockLabelId record_label_id,
    const bir::Block& expected_block) {
  if (record_block == &expected_block) {
    return true;
  }
  if (record_label_id != c4c::kInvalidBlockLabel &&
      expected_block.label_id != c4c::kInvalidBlockLabel) {
    return record_label_id == expected_block.label_id;
  }
  return !record_label.empty() && record_label == expected_block.label;
}

[[nodiscard]] bool route5_index_predecessor_matches(
    const bir::Route5CurrentBlockJoinSourceRecord& record,
    const bir::PhiIncoming& incoming) {
  if (record.predecessor_label_id != c4c::kInvalidBlockLabel &&
      incoming.label_id != c4c::kInvalidBlockLabel) {
    return record.predecessor_label_id == incoming.label_id;
  }
  return !record.predecessor_label.empty() &&
         record.predecessor_label == incoming.label;
}

[[nodiscard]] bool route5_index_value_matches(
    const bir::Route1SourceValueIdentity& record_value,
    std::string_view record_name,
    bir::TypeKind record_type,
    const bir::Value& value) {
  if (record_type != value.type) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Named) {
    return !record_name.empty() && record_name == value.name;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return record_value.integer_constant.has_value() &&
           *record_value.integer_constant == value.immediate;
  }
  return false;
}

[[nodiscard]] bool route5_index_join_record_matches(
    const bir::Route5CurrentBlockJoinSourceRecord& record,
    const bir::Block& successor_block,
    const bir::PhiInst& phi,
    const bir::PhiIncoming& incoming) {
  return route5_index_block_matches(record.successor_block,
                                    record.successor_label,
                                    record.successor_label_id,
                                    successor_block) &&
         route5_index_predecessor_matches(record, incoming) &&
         route5_index_value_matches(record.destination_value,
                                    record.destination_value_name,
                                    record.destination_value_type,
                                    phi.result) &&
         route5_index_value_matches(record.source_value,
                                    record.source_value_name,
                                    record.source_value_type,
                                    incoming.value);
}

[[nodiscard]] bir::Route5PublicationStatus
validate_route5_index_join_record(
    const bir::Route5CurrentBlockJoinSourceRecord& record,
    const bir::Block& successor_block,
    const bir::Inst& destination_instruction,
    const bir::PhiInst& destination_phi,
    std::size_t destination_instruction_index,
    const bir::PhiIncoming& incoming) {
  if (record.status != bir::Route5PublicationStatus::Available) {
    return record.status;
  }
  if (!record ||
      record.destination_instruction != &destination_instruction ||
      record.destination_phi != &destination_phi ||
      record.destination_instruction_index != destination_instruction_index) {
    return bir::Route5PublicationStatus::MissingPublication;
  }
  if (!route5_index_join_record_matches(record,
                                        successor_block,
                                        destination_phi,
                                        incoming)) {
    return bir::Route5PublicationStatus::NoMatch;
  }
  if (incoming.value.kind == bir::Value::Kind::Named &&
      (!record.source_producer_instruction_index.has_value() ||
       *record.source_producer_instruction_index >= successor_block.insts.size() ||
       record.source_producer_instruction !=
           &successor_block.insts[*record.source_producer_instruction_index])) {
    return bir::Route5PublicationStatus::MissingSourceProducer;
  }
  return bir::Route5PublicationStatus::Available;
}

[[nodiscard]] bir::Route5CurrentBlockJoinSourceRecord
find_validated_indexed_current_block_join_source_record(
    const BirCurrentBlockJoinSourceRequest& request,
    const bir::Inst& destination_instruction,
    const bir::PhiInst& phi,
    const bir::PhiIncoming& incoming,
    std::size_t instruction_index) {
  if (request.successor_block == nullptr ||
      request.route5_edge_join_sources == nullptr) {
    return route5_missing_current_block_join_source_record(
        request,
        &phi,
        &incoming,
        instruction_index,
        bir::Route5PublicationStatus::MissingSuccessor);
  }

  const bir::Route5CurrentBlockJoinSourceRecord* match = nullptr;
  std::size_t match_count = 0;
  for (const auto& candidate : request.route5_edge_join_sources->join_records) {
    if (!route5_index_join_record_matches(
            candidate, *request.successor_block, phi, incoming)) {
      continue;
    }
    match = &candidate;
    ++match_count;
  }
  if (match_count == 0) {
    auto missing = bir::route5_find_current_block_join_source(
        *request.route5_edge_join_sources,
        *request.successor_block,
        phi.result,
        incoming.value);
    if (missing.status == bir::Route5PublicationStatus::Available) {
      missing.status = bir::Route5PublicationStatus::NoMatch;
      missing.available = false;
    }
    if (missing.destination_instruction == nullptr) {
      missing.destination_instruction = &destination_instruction;
      missing.destination_phi = &phi;
      missing.destination_instruction_index = instruction_index;
    }
    if (missing.predecessor_label.empty()) {
      missing.predecessor_label = incoming.label;
      missing.predecessor_label_id = incoming.label_id;
    }
    return missing;
  }
  if (match_count != 1) {
    return route5_missing_current_block_join_source_record(
        request,
        &phi,
        &incoming,
        instruction_index,
        bir::Route5PublicationStatus::NoMatch);
  }

  auto record = *match;
  const auto validation_status = validate_route5_index_join_record(
      record,
      *request.successor_block,
      destination_instruction,
      phi,
      instruction_index,
      incoming);
  if (validation_status != bir::Route5PublicationStatus::Available) {
    record.status = validation_status;
    record.available = false;
  }
  return record;
}

[[nodiscard]] std::vector<bir::Route5CurrentBlockJoinSourceRecord>
route5_indexed_current_block_join_source_records(
    const BirCurrentBlockJoinSourceRequest& request) {
  std::vector<bir::Route5CurrentBlockJoinSourceRecord> records;
  if (request.successor_block == nullptr) {
    records.push_back(route5_missing_current_block_join_source_record(
        request,
        nullptr,
        nullptr,
        std::size_t{0},
        bir::Route5PublicationStatus::MissingSuccessor));
    return records;
  }

  bool saw_phi = false;
  for (std::size_t instruction_index = 0;
       instruction_index < request.successor_block->insts.size();
       ++instruction_index) {
    const auto& inst = request.successor_block->insts[instruction_index];
    const auto* phi = std::get_if<bir::PhiInst>(&inst);
    if (phi == nullptr) {
      break;
    }
    saw_phi = true;
    for (const auto& incoming : phi->incomings) {
      auto record = find_validated_indexed_current_block_join_source_record(
          request, inst, *phi, incoming, instruction_index);
      records.push_back(record);
    }
  }

  if (!saw_phi || records.empty()) {
    records.push_back(bir::Route5CurrentBlockJoinSourceRecord{
        .status = bir::Route5PublicationStatus::MissingPublication,
        .successor_block = request.successor_block,
        .successor_label = request.successor_block->label,
        .successor_label_id = request.successor_block->label_id,
    });
  }
  return records;
}

struct SelectChainViewResult {
  bir::BirSelectDependencyResult dependency;
  bir::BirProducerResult root;
};

[[nodiscard]] std::optional<SelectChainViewResult>
find_select_chain_view_result(BirSelectChainIdentityRequest request) {
  if (!request) {
    return std::nullopt;
  }
  if (!request.block_label.empty() &&
      request.block_label != request.block->label) {
    return std::nullopt;
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return std::nullopt;
  }
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  const auto value_type = root_value_type(request);
  if (request.root_value != nullptr &&
      (request.root_value->kind != bir::Value::Kind::Named ||
       request.root_value->name != value_name ||
       request.root_value->type != value_type)) {
    return std::nullopt;
  }
  std::optional<bir::Value> lookup_value;
  if (value_type != bir::TypeKind::Void) {
    lookup_value = bir::Value::named(value_type, std::string(value_name));
  }
  const auto dependency = bir::find_bir_select_dependency(
      bir::BirSelectDependencyRequest{
          .block = request.block,
          .root_value = lookup_value.has_value() ? &*lookup_value : nullptr,
          .root_value_name = value_name,
          .block_label = request.block_label,
          .before_instruction_index = before,
      });
  if (!dependency.complete() || !dependency.root_producer ||
      dependency.root_value == nullptr ||
      dependency.root_producer.produced_value != dependency.root_value ||
      dependency.root_value->kind != bir::Value::Kind::Named ||
      dependency.root_value->name != value_name ||
      (value_type != bir::TypeKind::Void &&
       dependency.root_value->type != value_type)) {
    return std::nullopt;
  }
  return SelectChainViewResult{.dependency = dependency,
                               .root = dependency.root_producer};
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
    unsigned depth) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return SameBlockIntegerConstant{.value = value.immediate, .depth = depth};
  }
  if (!query || depth > 4U || value.kind != bir::Value::Kind::Named ||
      value.name.empty() ||
      (!query.block_label.empty() && query.block_label != query.block->label)) {
    return std::nullopt;
  }
  const auto before =
      std::min(query.before_instruction_index, query.block->insts.size());
  const auto result = bir::find_same_block_producer(
      bir::make_bir_producer_view(*query.block), value, before);
  if (!result || !result.scalar_materialization_available ||
      result.instruction_index >= query.block->insts.size()) {
    return std::nullopt;
  }
  if (result.immediate_integer_constant.has_value()) {
    return SameBlockIntegerConstant{
        .value = *result.immediate_integer_constant,
        .depth = depth,
    };
  }
  const auto* binary =
      std::get_if<bir::BinaryInst>(&query.block->insts[result.instruction_index]);
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto nested_query = SameBlockValueMaterializationQuery{
      .block = query.block,
      .block_label = query.block_label,
      .before_instruction_index = result.instruction_index,
  };
  const auto lhs = evaluate_same_block_integer_constant(
      nested_query, binary->lhs, depth + 1U);
  const auto rhs = evaluate_same_block_integer_constant(
      nested_query, binary->rhs, depth + 1U);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  const auto lhs_value = lhs->value;
  const auto rhs_value = rhs->value;
  const auto unsigned_lhs = static_cast<std::uint64_t>(lhs_value);
  const auto unsigned_rhs = static_cast<std::uint64_t>(rhs_value);
  std::optional<std::int64_t> evaluated;
  switch (binary->opcode) {
    case bir::BinaryOpcode::Add:
      evaluated = static_cast<std::int64_t>(unsigned_lhs + unsigned_rhs);
      break;
    case bir::BinaryOpcode::Sub:
      evaluated = static_cast<std::int64_t>(unsigned_lhs - unsigned_rhs);
      break;
    case bir::BinaryOpcode::Mul:
      evaluated = static_cast<std::int64_t>(unsigned_lhs * unsigned_rhs);
      break;
    case bir::BinaryOpcode::And:
      evaluated = static_cast<std::int64_t>(unsigned_lhs & unsigned_rhs);
      break;
    case bir::BinaryOpcode::Or:
      evaluated = static_cast<std::int64_t>(unsigned_lhs | unsigned_rhs);
      break;
    case bir::BinaryOpcode::Xor:
      evaluated = static_cast<std::int64_t>(unsigned_lhs ^ unsigned_rhs);
      break;
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      if (binary->opcode == bir::BinaryOpcode::Shl) {
        evaluated = static_cast<std::int64_t>(
            unsigned_lhs << static_cast<unsigned>(rhs_value));
      } else if (binary->opcode == bir::BinaryOpcode::LShr) {
        evaluated = static_cast<std::int64_t>(
            unsigned_lhs >> static_cast<unsigned>(rhs_value));
      } else {
        evaluated = lhs_value >> static_cast<unsigned>(rhs_value);
      }
      break;
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      if (binary->opcode == bir::BinaryOpcode::SDiv) {
        evaluated = lhs_value / rhs_value;
      } else if (binary->opcode == bir::BinaryOpcode::UDiv) {
        evaluated = static_cast<std::int64_t>(unsigned_lhs / unsigned_rhs);
      } else if (binary->opcode == bir::BinaryOpcode::SRem) {
        evaluated = lhs_value % rhs_value;
      } else {
        evaluated = static_cast<std::int64_t>(unsigned_lhs % unsigned_rhs);
      }
      break;
    case bir::BinaryOpcode::Eq:
      evaluated = lhs_value == rhs_value ? 1 : 0;
      break;
    case bir::BinaryOpcode::Ne:
      evaluated = lhs_value != rhs_value ? 1 : 0;
      break;
    case bir::BinaryOpcode::Slt:
      evaluated = lhs_value < rhs_value ? 1 : 0;
      break;
    case bir::BinaryOpcode::Sle:
      evaluated = lhs_value <= rhs_value ? 1 : 0;
      break;
    case bir::BinaryOpcode::Sgt:
      evaluated = lhs_value > rhs_value ? 1 : 0;
      break;
    case bir::BinaryOpcode::Sge:
      evaluated = lhs_value >= rhs_value ? 1 : 0;
      break;
    case bir::BinaryOpcode::Ult:
      evaluated = unsigned_lhs < unsigned_rhs ? 1 : 0;
      break;
    case bir::BinaryOpcode::Ule:
      evaluated = unsigned_lhs <= unsigned_rhs ? 1 : 0;
      break;
    case bir::BinaryOpcode::Ugt:
      evaluated = unsigned_lhs > unsigned_rhs ? 1 : 0;
      break;
    case bir::BinaryOpcode::Uge:
      evaluated = unsigned_lhs >= unsigned_rhs ? 1 : 0;
      break;
  }
  if (!evaluated.has_value()) {
    return std::nullopt;
  }
  return SameBlockIntegerConstant{
      .value = *evaluated,
      .depth = depth,
  };
}

}  // namespace

[[nodiscard]] BirMemoryAccessIdentity find_bir_memory_access_identity(
    BirMemoryAccessIdentityRequest request) {
  if (!request || request.instruction_index >= request.block->insts.size()) {
    return {};
  }
  if (!request.block_label.empty() && request.block_label != request.block->label) {
    return {};
  }
  const auto view = bir::make_bir_memory_access_view(*request.block);
  const auto result = bir::find_memory_access(view, request.instruction_index);
  const auto identity = named_memory_access_to_mir(result);
  if (!identity) {
    return identity;
  }
  if (identity.node_kind != request.node_kind ||
      identity.block_label != request.block->label ||
      identity.instruction_index != request.instruction_index ||
      identity.inst != &request.block->insts[request.instruction_index]) {
    return {};
  }
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

  bir::Function function;
  function.blocks.push_back(*request.successor_block);
  const auto& indexed_successor = function.blocks.front();
  const auto route4_publications =
      bir::route4_build_publication_availability_index(function);
  const auto route_index_facade =
      bir::route_index_reference_facade(route4_publications);
  const auto publication_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route_index_facade,
          indexed_successor,
          bir::Value::named(value_type, std::string{value_name}));
  const auto* publication_record = publication_ref.block_entry_record;

  if (!publication_ref) {
    switch (publication_ref.route_status) {
      case bir::Route4PublicationAvailabilityStatus::MissingValue:
      case bir::Route4PublicationAvailabilityStatus::NoMatch:
        result.status = BirBlockEntryPublicationStatus::MissingDestinationValue;
        return result;
      case bir::Route4PublicationAvailabilityStatus::Unavailable:
      case bir::Route4PublicationAvailabilityStatus::MissingBlock:
      case bir::Route4PublicationAvailabilityStatus::MissingPublication:
      case bir::Route4PublicationAvailabilityStatus::AlternateSource:
      case bir::Route4PublicationAvailabilityStatus::Available:
        result.status = BirBlockEntryPublicationStatus::MissingPublication;
        return result;
    }
  }
  if (publication_record == nullptr) {
    result.status = BirBlockEntryPublicationStatus::MissingPublication;
    return result;
  }
  if (!*publication_record) {
    switch (publication_record->status) {
      case bir::Route4PublicationAvailabilityStatus::MissingValue:
      case bir::Route4PublicationAvailabilityStatus::NoMatch:
        result.status = BirBlockEntryPublicationStatus::MissingDestinationValue;
        return result;
      case bir::Route4PublicationAvailabilityStatus::Unavailable:
      case bir::Route4PublicationAvailabilityStatus::MissingBlock:
      case bir::Route4PublicationAvailabilityStatus::MissingPublication:
      case bir::Route4PublicationAvailabilityStatus::AlternateSource:
      case bir::Route4PublicationAvailabilityStatus::Available:
        result.status = BirBlockEntryPublicationStatus::MissingPublication;
        return result;
    }
  }

  if (publication_record->destination_instruction_index >=
      request.successor_block->insts.size()) {
    result.status = BirBlockEntryPublicationStatus::MissingPublication;
    return result;
  }
  const auto& inst =
      request.successor_block
          ->insts[publication_record->destination_instruction_index];
  const auto* phi = std::get_if<bir::PhiInst>(&inst);
  if (phi == nullptr ||
      phi->result.kind != bir::Value::Kind::Named ||
      phi->result.name != publication_record->destination_value_name) {
    result.status = BirBlockEntryPublicationStatus::MissingPublication;
    return result;
  }
  if (value_type != bir::TypeKind::Void && phi->result.type != value_type) {
    result.status = BirBlockEntryPublicationStatus::MissingDestinationValue;
    return result;
  }
  result.available = true;
  result.status = BirBlockEntryPublicationStatus::Available;
  result.instruction = &inst;
  result.phi = phi;
  result.instruction_index = publication_record->destination_instruction_index;
  result.destination_value = &phi->result;
  result.destination_value_identity = same_block_value_identity(phi->result);
  result.destination_value_name = phi->result.name;
  result.destination_value_type = phi->result.type;
  return result;
}

[[nodiscard]] BirCfgEdgePublicationSourceIdentity
find_bir_cfg_edge_publication_source_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgeCopySourceFacts& prepared) {
  BirCfgEdgePublicationSourceIdentity result{
      .predecessor_label_id = prepared.predecessor_label,
      .successor_label_id = prepared.successor_label,
      .destination_value_id = prepared.destination_value_id,
      .destination_value_name_id = prepared.destination_value_name,
      .destination_value_type = prepared.destination_value.type,
      .source_value_id = prepared.source_value_id,
      .source_value_name_id = prepared.source_value_name,
      .source_value_kind = prepared.source_value_kind,
      .source_value_type = prepared.source_value.type,
  };
  auto fail = [&](BirCfgEdgePublicationSourceStatus status) {
    result.status = status;
    return result;
  };
  if (prepared.status != prepare::PreparedEdgeCopySourceFactsStatus::Available) {
    switch (prepared.status) {
      case prepare::PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel:
        return fail(BirCfgEdgePublicationSourceStatus::MissingPredecessorLabel);
      case prepare::PreparedEdgeCopySourceFactsStatus::MissingSuccessorLabel:
        return fail(BirCfgEdgePublicationSourceStatus::MissingSuccessorLabel);
      case prepare::PreparedEdgeCopySourceFactsStatus::MissingDestinationValue:
        return fail(BirCfgEdgePublicationSourceStatus::MissingDestinationValue);
      case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceValue:
        return fail(BirCfgEdgePublicationSourceStatus::MissingSourceValue);
      case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceProducer:
      case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceMemoryAccess:
      case prepare::PreparedEdgeCopySourceFactsStatus::IncompleteSourceMemoryAccess:
        return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
      default:
        return fail(BirCfgEdgePublicationSourceStatus::MissingPublication);
    }
  }
  if (prepared.publication == nullptr || prepared.move == nullptr ||
      !prepared.source_producer_block_label.has_value() ||
      !prepared.source_producer_instruction_index.has_value()) {
    return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
  }
  result.destination_value_name =
      prepare::prepared_value_name(names, prepared.destination_value_name);
  result.destination_value_identity = SameBlockValueIdentity{
      .name = result.destination_value_name,
      .type = prepared.destination_value.type,
  };
  result.source_value_name =
      prepare::prepared_value_name(names, prepared.source_value_name);
  result.source_value_identity = SameBlockValueIdentity{
      .name = result.source_value_name,
      .type = prepared.source_value.type,
      .immediate_constant =
          prepared.source_value_kind == bir::Value::Kind::Immediate
              ? std::optional<std::int64_t>{prepared.source_value.immediate}
              : std::nullopt,
  };
  result.source_producer_block_label_id = *prepared.source_producer_block_label;
  result.source_producer_instruction_index =
      prepared.source_producer_instruction_index;
  switch (prepared.source_producer_kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      result.source_producer_kind = SameBlockProducerKind::Binary;
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      result.source_producer_kind = SameBlockProducerKind::Cast;
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      result.source_producer_kind = SameBlockProducerKind::Select;
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      result.source_producer_kind = SameBlockProducerKind::LoadLocal;
      break;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      result.source_producer_kind = SameBlockProducerKind::LoadGlobal;
      break;
    default:
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
  }
  result.source_producer.kind = result.source_producer_kind;
  result.source_producer.instruction_index =
      *prepared.source_producer_instruction_index;
  result.source_producer.produced_value = result.source_value_identity;
  result.source_producer.materialization_available = true;
  if (prepared.source_memory_access_status ==
      prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available) {
    if (prepared.source_memory_access == nullptr) {
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
    }
    result.source_memory_access.status = bir::BirViewStatus::Available;
    result.source_memory_access.instruction_index =
        prepared.source_memory_access->inst_index;
    result.source_memory_access.result_value_name = result.source_value_name;
    result.source_memory_access.address_space = prepared.source_memory_address_space;
    result.source_memory_access.is_volatile = prepared.source_memory_is_volatile;
    result.source_memory_access.byte_offset = prepared.source_memory_byte_offset;
    result.source_memory_access.size_bytes = prepared.source_memory_size_bytes;
    result.source_memory_access.align_bytes = prepared.source_memory_align_bytes;
    if (result.source_producer_kind == SameBlockProducerKind::LoadLocal &&
        prepared.source_memory_frame_slot_id.has_value()) {
      result.source_memory_access.node_kind = BirMemoryAccessNodeKind::LoadLocal;
      result.source_memory_access.base_kind = BirMemoryAccessBaseKind::LocalSlot;
      result.source_memory_access.local_slot_id =
          static_cast<c4c::SlotNameId>(*prepared.source_memory_frame_slot_id);
    } else if (result.source_producer_kind == SameBlockProducerKind::LoadGlobal &&
               prepared.source_memory_symbol_name.has_value()) {
      result.source_memory_access.node_kind = BirMemoryAccessNodeKind::LoadGlobal;
      result.source_memory_access.base_kind = BirMemoryAccessBaseKind::GlobalSymbol;
      result.source_memory_access.global_name_id = *prepared.source_memory_symbol_name;
    } else {
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
    }
  }
  result.available = true;
  result.status = BirCfgEdgePublicationSourceStatus::Available;
  return result;
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

  const bool use_route5_index = request.route5_edge_join_sources != nullptr;
  const auto join_records =
      use_route5_index
          ? route5_indexed_current_block_join_source_records(request)
          : bir::route5_current_block_join_source_records(request.successor_block);
  bool saw_missing_publication = false;
  for (const auto& record : join_records) {
    if (record.status == bir::Route5PublicationStatus::MissingPublication) {
      if (!use_route5_index) {
        continue;
      }
      saw_missing_publication = true;
    }
    BirCurrentBlockJoinSourceFact fact{
        .status = route5_join_status_to_mir(record.status),
        .predecessor_label = record.predecessor_label,
        .predecessor_label_id = record.predecessor_label_id,
        .successor_label = result.successor_label,
        .successor_label_id = result.successor_label_id,
        .destination_instruction = record.destination_instruction,
        .destination_phi = record.destination_phi,
        .destination_instruction_index = record.destination_instruction_index,
        .destination_value = record.destination_value.value,
        .destination_value_identity =
            route1_source_value_identity_to_same_block(record.destination_value),
        .destination_value_name = record.destination_value_name,
        .destination_value_type = record.destination_value_type,
        .source_value = record.source_value.value,
        .source_value_identity =
            route1_source_value_identity_to_same_block(record.source_value),
        .source_value_name = record.source_value_name,
        .source_value_kind = record.source_value_kind,
        .source_value_type = record.source_value_type,
        .source_producer_kind =
            route5_publication_source_kind_to_same_block_kind(
                record.source_producer_kind),
        .source_producer_instruction_index =
            record.source_producer_instruction_index,
    };
    if (use_route5_index &&
        fact.status == BirCurrentBlockJoinSourceStatus::MissingPublication) {
      saw_missing_publication = true;
    }
    append_unique_value_identity(result.source_values,
                                 fact.destination_value_identity);
    if (record.source_value_kind == bir::Value::Kind::Named) {
      append_unique_value_identity(result.source_values,
                                   fact.source_value_identity);
      append_unique_value_identity(result.incoming_expression_values,
                                   fact.source_value_identity);
    }
    if (record.status == bir::Route5PublicationStatus::Available &&
        record.source_value_kind == bir::Value::Kind::Named) {
      fact.source_producer = route5_join_source_producer_to_mir(
          record, *request.successor_block, result.successor_label);
      if (!fact.source_producer) {
        fact.status = BirCurrentBlockJoinSourceStatus::MissingSourceProducer;
        fact.source_producer_kind = SameBlockProducerKind::Unknown;
        fact.source_producer_instruction_index.reset();
      } else {
        fact.source_producer_kind = fact.source_producer.kind;
        fact.source_producer_instruction_index =
            fact.source_producer.instruction_index;
      }
    }
    result.facts.push_back(fact);
  }

  if (result.facts.empty()) {
    result.status = BirCurrentBlockJoinSourceStatus::MissingPublication;
    return result;
  }

  const auto route1_index =
      bir::route1_build_producer_index(*request.successor_block);
  const auto route1_query = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = request.successor_block->insts.size(),
  };
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
    const auto producer = bir::route1_find_same_block_scalar_producer(
        route1_query, bir::Value::named(value.type, std::string{value.name}));
    if (producer.has_value() && producer->instruction != nullptr) {
      append_bir_expression_operands(pending, *producer->instruction);
    }
  }

  result.available =
      std::all_of(result.facts.begin(), result.facts.end(), [](const auto& fact) {
        return fact.status == BirCurrentBlockJoinSourceStatus::Available;
      });
  if (result.available) {
    result.status = BirCurrentBlockJoinSourceStatus::Available;
  } else if (saw_missing_publication) {
    result.status = BirCurrentBlockJoinSourceStatus::MissingPublication;
  } else {
    result.status = BirCurrentBlockJoinSourceStatus::MissingSourceProducer;
  }
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
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  if (value_type == bir::TypeKind::Void) {
    return {};
  }
  const auto result = bir::find_same_block_global_load(
      bir::BirSameBlockGlobalLoadRequest{
          .block = request.block,
          .value_name = value_name,
          .value_type = value_type,
          .before_instruction_index = before,
      });
  if (!result) {
    return {};
  }
  const auto memory_access = BirMemoryAccessIdentity{
      .status = result.status,
      .inst = result.access.instruction,
      .block_label = result.access.block_label,
      .instruction_index = result.access.instruction_index,
      .node_kind = named_memory_kind_to_mir(result.access.kind),
      .result_value_name = result.access.result_value_name,
      .address_space = result.access.address_space,
      .is_volatile = result.access.is_volatile,
      .base_kind = named_memory_base_to_mir(result.access.base_kind),
      .global_name = result.access.global_name,
      .global_name_id = result.access.global_name_id,
      .result_value = result.result_value,
      .byte_offset = result.access.byte_offset,
      .size_bytes = result.access.size_bytes,
      .align_bytes = result.access.align_bytes,
  };
  const auto producer = SameBlockProducerIdentity{
      .inst = result.access.instruction,
      .instruction_index = result.access.instruction_index,
      .kind = SameBlockProducerKind::LoadGlobal,
      .block_label = result.access.block_label,
      .before_instruction_index = request.before_instruction_index,
      .produced_value = same_block_value_identity(*result.result_value),
      .materialization_available = true,
  };
  if (!memory_access ||
      memory_access.base_kind != BirMemoryAccessBaseKind::GlobalSymbol ||
      memory_access.result_value_name != value_name) {
    return {};
  }
  return BirSameBlockGlobalLoadAccessIdentity{
      .producer = producer,
      .memory_access = memory_access,
      .load_global = result.load,
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
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  const auto result = bir::find_same_block_load_local_source(
      bir::BirSameBlockLoadLocalRequest{
          .block = request.block,
          .value_name = value_name,
          .value_type = value_type,
          .before_instruction_index = before,
      });
  if (!result) {
    return {};
  }
  const auto memory_access = BirMemoryAccessIdentity{
      .status = result.status,
      .inst = result.access.instruction,
      .block_label = result.access.block_label,
      .instruction_index = result.access.instruction_index,
      .node_kind = named_memory_kind_to_mir(result.access.kind),
      .result_value_name = result.access.result_value_name,
      .address_space = result.access.address_space,
      .is_volatile = result.access.is_volatile,
      .base_kind = named_memory_base_to_mir(result.access.base_kind),
      .local_slot_name = result.access.local_slot_name,
      .local_slot_id = result.access.local_slot_id,
      .result_value = result.result_value,
      .byte_offset = result.access.byte_offset,
      .size_bytes = result.access.size_bytes,
      .align_bytes = result.access.align_bytes,
  };
  const auto producer = SameBlockProducerIdentity{
      .inst = result.access.instruction,
      .instruction_index = result.access.instruction_index,
      .kind = SameBlockProducerKind::LoadLocal,
      .block_label = result.access.block_label,
      .before_instruction_index = request.before_instruction_index,
      .produced_value = same_block_value_identity(*result.result_value),
      .materialization_available = true,
  };
  if (!memory_access ||
      memory_access.base_kind != BirMemoryAccessBaseKind::LocalSlot ||
      memory_access.result_value_name != value_name) {
    return {};
  }
  return BirSameBlockLoadLocalSourceIdentity{
      .producer = producer,
      .memory_access = memory_access,
      .load_local = result.load,
      .result_value = producer.produced_value,
      .root_value_name = value_name,
      .root_value_type = value_type,
      .before_instruction_index = request.before_instruction_index,
  };
}

[[nodiscard]] BirSameBlockLoadLocalStoredValueSourceIdentity
find_bir_same_block_load_local_stored_value_source_identity(
    BirSameBlockLoadLocalSourceRequest request) {
  if (!request ||
      (!request.block_label.empty() && request.block_label != request.block->label)) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  const auto value_type = root_value_type(request);
  if (value_type == bir::TypeKind::Void) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  const auto result = bir::find_same_block_store_local_source(
      bir::BirSameBlockLoadLocalRequest{
          .block = request.block,
          .value = request.root_value,
          .value_name = value_name,
          .value_type = value_type,
          .before_instruction_index = request.before_instruction_index,
      });
  if (!result) {
    return {.status = result.status};
  }
  const auto load_memory_access = named_memory_access_to_mir(result.load_access);
  const auto store_memory_access = named_memory_access_to_mir(result.store_access);
  if (!load_memory_access ||
      !store_memory_access ||
      load_memory_access.base_kind != BirMemoryAccessBaseKind::LocalSlot ||
      store_memory_access.base_kind != BirMemoryAccessBaseKind::LocalSlot ||
      load_memory_access.result_value_name != value_name ||
      load_memory_access.local_slot_id != store_memory_access.local_slot_id) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  return BirSameBlockLoadLocalStoredValueSourceIdentity{
      .status = result.status,
      .load_memory_access = load_memory_access,
      .store_memory_access = store_memory_access,
      .load_local = result.load,
      .store_local = result.store,
      .loaded_value = same_block_value_identity(*result.loaded_value),
      .stored_value = same_block_value_identity(*result.stored_value),
      .root_value_name = value_name,
      .root_value_type = value_type,
      .before_instruction_index = request.before_instruction_index,
  };
}

[[nodiscard]] SameBlockBinaryProducer find_same_block_binary_producer(
    const bir::Block* block,
    const bir::Value& value) {
  if (block == nullptr) {
    return {.status = bir::BirViewStatus::Unavailable};
  }
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  const auto result = bir::find_same_block_producer(
      bir::make_bir_producer_view(*block), value, block->insts.size());
  if (!result) {
    return {.status = result.status};
  }
  if (result.kind != bir::BirProducerKind::Binary ||
      result.produced_value == nullptr || result.block_label != block->label ||
      result.instruction_index >= block->insts.size()) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  const auto* binary =
      std::get_if<bir::BinaryInst>(&block->insts[result.instruction_index]);
  return binary == nullptr
             ? SameBlockBinaryProducer{.status = bir::BirViewStatus::Incomplete}
             : SameBlockBinaryProducer{.status = result.status,
                                       .binary = binary,
                                       .produced_value = result.produced_value,
                                       .block_label = result.block_label,
                                       .instruction_index = result.instruction_index};
}

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {.status = block == nullptr ? bir::BirViewStatus::Unavailable
                                      : bir::BirViewStatus::Incomplete};
  }
  const auto before = std::min(before_instruction_index, block->insts.size());
  const auto result = bir::find_same_block_producer(
      bir::make_bir_producer_view(*block), value, before);
  if (!result) {
    return {.status = result.status};
  }
  if (result.kind != bir::BirProducerKind::SelectMaterialization ||
      result.produced_value == nullptr || result.block_label != block->label ||
      result.instruction_index >= before ||
      result.instruction_index >= block->insts.size()) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  const auto* select =
      std::get_if<bir::SelectInst>(&block->insts[result.instruction_index]);
  if (select == nullptr || result.produced_value != &select->result ||
      select->result.kind != bir::Value::Kind::Named ||
      select->result.name != value.name || select->result.type != value.type) {
    return {.status = bir::BirViewStatus::Incomplete};
  }
  return SameBlockSelectProducer{.status = result.status,
                                 .select = select,
                                 .produced_value = result.produced_value,
                                 .block_label = result.block_label,
                                 .instruction_index = result.instruction_index};
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
  if (!request ||
      (!request.block_label.empty() && request.block_label != request.block->label)) {
    return {};
  }
  const auto before =
      std::min(request.before_instruction_index, request.block->insts.size());
  const auto view = bir::make_bir_producer_view(*request.block);

  if (request.value_type != bir::TypeKind::Void) {
    const auto result = bir::find_same_block_producer(
        view,
        bir::Value::named(request.value_type, std::string{request.value_name}),
        before);
    return producer_view_result_to_same_block(
        result, *request.block, request.before_instruction_index);
  }

  SameBlockProducerIdentity match;
  for (std::size_t index = before; index > 0; --index) {
    const auto* value =
        produced_value_for_same_block_identity(request.block->insts[index - 1]);
    if (value == nullptr || value->kind != bir::Value::Kind::Named ||
        value->name != request.value_name) {
      continue;
    }
    const auto result = bir::find_same_block_producer(view, *value, before);
    const auto candidate = producer_view_result_to_same_block(
        result, *request.block, request.before_instruction_index);
    if (!candidate) {
      return {};
    }
    if (match && match.instruction_index != candidate.instruction_index) {
      return {};
    }
    match = candidate;
  }
  return match;
}

[[nodiscard]] SameBlockProducerIdentity find_bir_select_chain_source_producer(
    BirSelectChainIdentityRequest request) {
  const auto result = find_select_chain_view_result(request);
  if (!result.has_value() || request.block == nullptr) {
    return {};
  }
  return select_chain_producer_result_to_same_block(
      result->root, *request.block, request.block_label,
      request.before_instruction_index);
}

[[nodiscard]] BirSelectChainDirectGlobalDependency
find_bir_select_chain_direct_global_dependency(
    BirSelectChainIdentityRequest request) {
  const auto result = find_select_chain_view_result(request);
  if (!result.has_value()) {
    return {};
  }
  if (result->dependency.status !=
          bir::BirSelectDependencyStatus::CompleteDirectGlobal ||
      result->dependency.dependency_load == nullptr) {
    return {};
  }
  return BirSelectChainDirectGlobalDependency{
      .contains_direct_global_load = true,
      .load_global = result->dependency.dependency_load,
      .instruction_index = result->dependency.dependency_instruction_index,
  };
}

[[nodiscard]] bool find_bir_select_chain_scalar_materialization_eligibility(
    BirSelectChainIdentityRequest request) {
  const auto result = find_select_chain_view_result(request);
  return result.has_value() && result->root.scalar_materialization_available;
}

[[nodiscard]] BirSelectChainIdentity find_bir_select_chain_identity(
    BirSelectChainIdentityRequest request) {
  const auto result = find_select_chain_view_result(request);
  if (!result.has_value() || request.block == nullptr) {
    return BirSelectChainIdentity{
        .root_value_name = root_value_name(request),
    };
  }
  const auto root = select_chain_producer_result_to_same_block(
      result->root, *request.block, request.block_label,
      request.before_instruction_index);
  if (!root) {
    return BirSelectChainIdentity{
        .root_value_name = root_value_name(request),
    };
  }
  return BirSelectChainIdentity{
      .root_producer = root,
      .root_value = same_block_value_identity(*result->root.produced_value),
      .root_value_name = result->root.produced_value->name,
      .root_is_select = result->root.kind ==
                        bir::BirProducerKind::SelectMaterialization,
      .root_instruction_index = result->root.instruction_index,
      .direct_global_dependency =
          result->dependency.status ==
                      bir::BirSelectDependencyStatus::CompleteDirectGlobal &&
                  result->dependency.dependency_load != nullptr
              ? BirSelectChainDirectGlobalDependency{
                    .contains_direct_global_load = true,
                    .load_global = result->dependency.dependency_load,
                    .instruction_index =
                        result->dependency.dependency_instruction_index,
                }
              : BirSelectChainDirectGlobalDependency{},
      .scalar_materialization_available =
          result->root.scalar_materialization_available,
  };
}

[[nodiscard]] std::optional<SameBlockScalarProducer>
find_same_block_scalar_producer(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
  if (!query || value.kind != bir::Value::Kind::Named || value.name.empty() ||
      (!query.block_label.empty() && query.block_label != query.block->label)) {
    return std::nullopt;
  }
  const auto before =
      std::min(query.before_instruction_index, query.block->insts.size());
  const auto result = bir::find_same_block_producer(
      bir::make_bir_producer_view(*query.block), value, before);
  const auto producer = producer_view_result_to_same_block(
      result, *query.block, query.before_instruction_index);
  if (!producer || !result.scalar_materialization_available) {
    return std::nullopt;
  }
  return SameBlockScalarProducer{
      .producer = producer,
      .instruction = producer.inst,
      .produced_value = result.produced_value,
      .instruction_index = result.instruction_index,
  };
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
  return evaluate_same_block_integer_constant(
      SameBlockValueMaterializationQuery{
          .block = block,
          .block_label = block->label,
          .before_instruction_index = block->insts.size(),
      },
      value,
      depth);
}

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return SameBlockIntegerConstant{.value = value.immediate, .depth = 0U};
  }
  return evaluate_same_block_integer_constant(query, value, 0U);
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
