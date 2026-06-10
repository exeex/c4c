#pragma once

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::mir {

namespace bir = c4c::backend::bir;

enum class SameBlockProducerKind {
  Unknown,
  Binary,
  Cast,
  Select,
  LoadLocal,
  LoadGlobal,
};

enum class BirMemoryAccessNodeKind {
  Unknown,
  LoadLocal,
  LoadGlobal,
  StoreLocal,
  StoreGlobal,
};

enum class BirMemoryAccessBaseKind {
  None,
  LocalSlot,
  GlobalSymbol,
  PointerValue,
  StringConstant,
};

struct BirMemoryAccessIdentityRequest {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  std::size_t instruction_index = 0;
  BirMemoryAccessNodeKind node_kind = BirMemoryAccessNodeKind::Unknown;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr && node_kind != BirMemoryAccessNodeKind::Unknown;
  }
};

struct BirMemoryAccessIdentity {
  const bir::Inst* inst = nullptr;
  std::string_view block_label;
  std::size_t instruction_index = 0;
  BirMemoryAccessNodeKind node_kind = BirMemoryAccessNodeKind::Unknown;
  std::string_view result_value_name;
  std::string_view stored_value_name;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  bool is_volatile = false;
  BirMemoryAccessBaseKind base_kind = BirMemoryAccessBaseKind::None;
  std::string_view local_slot_name;
  c4c::SlotNameId local_slot_id = c4c::kInvalidSlotName;
  std::string_view global_name;
  c4c::LinkNameId global_name_id = c4c::kInvalidLinkName;
  std::string_view pointer_value_name;
  std::string_view string_constant_name;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;

  [[nodiscard]] explicit operator bool() const { return inst != nullptr; }
};

struct SameBlockProducerIdentityRequest {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  std::string_view value_name;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr && !value_name.empty();
  }
};

struct SameBlockValueIdentity {
  const bir::Value* value = nullptr;
  std::string_view name;
  bir::TypeKind type = bir::TypeKind::Void;
  std::optional<std::int64_t> immediate_constant;

  [[nodiscard]] explicit operator bool() const {
    return value != nullptr ||
           !name.empty() ||
           immediate_constant.has_value();
  }
};

struct SameBlockProducerRecord {
  const bir::Inst* inst = nullptr;
  std::size_t instruction_index = 0;
  SameBlockProducerKind kind = SameBlockProducerKind::Unknown;
  std::string_view block_label;
  std::size_t before_instruction_index = 0;
  SameBlockValueIdentity produced_value;
  bool materialization_available = false;

  [[nodiscard]] explicit operator bool() const { return inst != nullptr; }
};

using SameBlockProducerIdentity = SameBlockProducerRecord;

struct BirCurrentBlockPublicationIdentityRequest {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  const bir::Value* root_value = nullptr;
  std::string_view root_value_name;
  bir::TypeKind root_value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr &&
           (root_value != nullptr || !root_value_name.empty());
  }
};

struct BirCurrentBlockPublicationIdentity {
  bool available = false;
  SameBlockProducerIdentity source_producer;
  const bir::Inst* instruction = nullptr;
  const bir::Value* produced_value = nullptr;
  SameBlockValueIdentity produced_value_identity;
  std::string_view produced_value_name;
  bir::TypeKind produced_value_type = bir::TypeKind::Void;
  std::size_t instruction_index = 0;
  std::string_view value_name;
  SameBlockProducerKind source_producer_kind = SameBlockProducerKind::Unknown;

  [[nodiscard]] explicit operator bool() const { return available; }
};

enum class BirBlockEntryPublicationStatus {
  Available,
  MissingSuccessorLabel,
  MissingDestinationValue,
  MissingPublication,
};

enum class BirCfgEdgePublicationSourceStatus {
  Available,
  MissingPredecessorLabel,
  MissingSuccessorLabel,
  MissingDestinationValue,
  MissingPublication,
  MissingSourceValue,
  MissingSourceProducer,
};

struct BirBlockEntryPublicationIdentityRequest {
  const bir::Block* successor_block = nullptr;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;
  const bir::Value* destination_value = nullptr;
  std::size_t destination_value_id = 0;
  std::string_view destination_value_name;
  c4c::ValueNameId destination_value_name_id = c4c::kInvalidValueName;
  bir::TypeKind destination_value_type = bir::TypeKind::Void;

  [[nodiscard]] explicit operator bool() const {
    return successor_block != nullptr &&
           (destination_value != nullptr ||
            !destination_value_name.empty() ||
            destination_value_name_id != c4c::kInvalidValueName);
  }
};

struct BirBlockEntryPublicationIdentity {
  bool available = false;
  BirBlockEntryPublicationStatus status =
      BirBlockEntryPublicationStatus::MissingPublication;
  const bir::Inst* instruction = nullptr;
  const bir::PhiInst* phi = nullptr;
  std::size_t instruction_index = 0;
  const bir::Value* destination_value = nullptr;
  std::size_t destination_value_id = 0;
  SameBlockValueIdentity destination_value_identity;
  std::string_view destination_value_name;
  c4c::ValueNameId destination_value_name_id = c4c::kInvalidValueName;
  bir::TypeKind destination_value_type = bir::TypeKind::Void;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct BirCfgEdgePublicationSourceRequest {
  const bir::Block* predecessor_block = nullptr;
  std::string_view predecessor_label;
  c4c::BlockLabelId predecessor_label_id = c4c::kInvalidBlockLabel;
  const bir::Block* successor_block = nullptr;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;
  const bir::Value* destination_value = nullptr;
  std::size_t destination_value_id = 0;
  std::string_view destination_value_name;
  c4c::ValueNameId destination_value_name_id = c4c::kInvalidValueName;
  bir::TypeKind destination_value_type = bir::TypeKind::Void;

  [[nodiscard]] explicit operator bool() const {
    return predecessor_block != nullptr &&
           successor_block != nullptr &&
           (destination_value != nullptr ||
            !destination_value_name.empty() ||
            destination_value_name_id != c4c::kInvalidValueName);
  }
};

struct BirCfgEdgePublicationSourceIdentity {
  bool available = false;
  BirCfgEdgePublicationSourceStatus status =
      BirCfgEdgePublicationSourceStatus::MissingPublication;
  std::string_view predecessor_label;
  c4c::BlockLabelId predecessor_label_id = c4c::kInvalidBlockLabel;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;
  const bir::Inst* destination_instruction = nullptr;
  const bir::PhiInst* destination_phi = nullptr;
  std::size_t destination_instruction_index = 0;
  const bir::Value* destination_value = nullptr;
  std::size_t destination_value_id = 0;
  SameBlockValueIdentity destination_value_identity;
  std::string_view destination_value_name;
  c4c::ValueNameId destination_value_name_id = c4c::kInvalidValueName;
  bir::TypeKind destination_value_type = bir::TypeKind::Void;
  const bir::Value* source_value = nullptr;
  std::optional<std::size_t> source_value_id;
  SameBlockValueIdentity source_value_identity;
  std::string_view source_value_name;
  c4c::ValueNameId source_value_name_id = c4c::kInvalidValueName;
  bir::Value::Kind source_value_kind = bir::Value::Kind::Immediate;
  bir::TypeKind source_value_type = bir::TypeKind::Void;
  SameBlockProducerIdentity source_producer;
  SameBlockProducerKind source_producer_kind = SameBlockProducerKind::Unknown;
  std::string_view source_producer_block_label;
  c4c::BlockLabelId source_producer_block_label_id = c4c::kInvalidBlockLabel;
  std::optional<std::size_t> source_producer_instruction_index;
  BirMemoryAccessIdentity source_memory_access;

  [[nodiscard]] explicit operator bool() const { return available; }
};

enum class BirCurrentBlockJoinSourceStatus {
  Available,
  MissingBlock,
  MissingSuccessorLabel,
  MissingPublication,
  MissingSourceProducer,
};

struct BirCurrentBlockJoinSourceRequest {
  const bir::Block* successor_block = nullptr;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;

  [[nodiscard]] explicit operator bool() const {
    return successor_block != nullptr;
  }
};

struct BirCurrentBlockJoinSourceFact {
  BirCurrentBlockJoinSourceStatus status =
      BirCurrentBlockJoinSourceStatus::MissingPublication;
  std::string_view predecessor_label;
  c4c::BlockLabelId predecessor_label_id = c4c::kInvalidBlockLabel;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;
  const bir::Inst* destination_instruction = nullptr;
  const bir::PhiInst* destination_phi = nullptr;
  std::size_t destination_instruction_index = 0;
  const bir::Value* destination_value = nullptr;
  SameBlockValueIdentity destination_value_identity;
  std::string_view destination_value_name;
  bir::TypeKind destination_value_type = bir::TypeKind::Void;
  const bir::Value* source_value = nullptr;
  SameBlockValueIdentity source_value_identity;
  std::string_view source_value_name;
  bir::Value::Kind source_value_kind = bir::Value::Kind::Immediate;
  bir::TypeKind source_value_type = bir::TypeKind::Void;
  SameBlockProducerIdentity source_producer;
  SameBlockProducerKind source_producer_kind = SameBlockProducerKind::Unknown;
  std::optional<std::size_t> source_producer_instruction_index;

  [[nodiscard]] explicit operator bool() const {
    return status == BirCurrentBlockJoinSourceStatus::Available;
  }
};

struct BirCurrentBlockJoinSourceIdentity {
  bool available = false;
  BirCurrentBlockJoinSourceStatus status =
      BirCurrentBlockJoinSourceStatus::MissingPublication;
  std::string_view successor_label;
  c4c::BlockLabelId successor_label_id = c4c::kInvalidBlockLabel;
  std::vector<BirCurrentBlockJoinSourceFact> facts;
  std::vector<SameBlockValueIdentity> incoming_expression_values;
  std::vector<SameBlockValueIdentity> source_values;

  [[nodiscard]] explicit operator bool() const { return available; }
};

struct BirSameBlockGlobalLoadAccessRequest {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  const bir::Value* root_value = nullptr;
  std::string_view root_value_name;
  bir::TypeKind root_value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr &&
           (root_value != nullptr || !root_value_name.empty());
  }
};

struct BirSameBlockGlobalLoadAccessIdentity {
  SameBlockProducerIdentity producer;
  BirMemoryAccessIdentity memory_access;
  const bir::LoadGlobalInst* load_global = nullptr;
  SameBlockValueIdentity result_value;
  std::string_view root_value_name;
  bir::TypeKind root_value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return load_global != nullptr && memory_access && producer;
  }
};

struct BirSameBlockLoadLocalSourceRequest {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  const bir::Value* root_value = nullptr;
  std::string_view root_value_name;
  bir::TypeKind root_value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr &&
           (root_value != nullptr || !root_value_name.empty());
  }
};

struct BirSameBlockLoadLocalSourceIdentity {
  SameBlockProducerIdentity producer;
  BirMemoryAccessIdentity memory_access;
  const bir::LoadLocalInst* load_local = nullptr;
  SameBlockValueIdentity result_value;
  std::string_view root_value_name;
  bir::TypeKind root_value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return load_local != nullptr && memory_access && producer;
  }
};

struct SameBlockProducerIndex {
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return producer != nullptr; }
};

struct SameBlockBinaryProducer {
  const bir::BinaryInst* binary = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return binary != nullptr; }
};

struct SameBlockSelectProducer {
  const bir::SelectInst* select = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return select != nullptr; }
};

struct SameBlockIntegerConstant {
  std::int64_t value = 0;
  unsigned depth = 0;
};

struct SameBlockValueMaterializationQuery {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return block != nullptr; }
};

struct SameBlockScalarProducer {
  SameBlockProducerIdentity producer;
  const bir::Inst* instruction = nullptr;
  const bir::Value* produced_value = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return producer && instruction != nullptr; }
};

struct BirSelectChainIdentityRequest {
  const bir::Block* block = nullptr;
  std::string_view block_label;
  const bir::Value* root_value = nullptr;
  std::string_view root_value_name;
  bir::TypeKind root_value_type = bir::TypeKind::Void;
  std::size_t before_instruction_index = 0;

  [[nodiscard]] explicit operator bool() const {
    return block != nullptr &&
           (root_value != nullptr || !root_value_name.empty());
  }
};

struct BirSelectChainDirectGlobalDependency {
  bool contains_direct_global_load = false;
  const bir::LoadGlobalInst* load_global = nullptr;
  std::optional<std::size_t> instruction_index;

  [[nodiscard]] explicit operator bool() const {
    return contains_direct_global_load && load_global != nullptr;
  }
};

struct BirSelectChainIdentity {
  SameBlockProducerIdentity root_producer;
  SameBlockValueIdentity root_value;
  std::string_view root_value_name;
  bool root_is_select = false;
  std::optional<std::size_t> root_instruction_index;
  BirSelectChainDirectGlobalDependency direct_global_dependency;
  bool scalar_materialization_available = false;

  [[nodiscard]] explicit operator bool() const {
    return root_producer && root_value;
  }
};

[[nodiscard]] SameBlockProducerIdentity find_bir_select_chain_source_producer(
    BirSelectChainIdentityRequest request);

[[nodiscard]] BirSelectChainDirectGlobalDependency
find_bir_select_chain_direct_global_dependency(
    BirSelectChainIdentityRequest request);

[[nodiscard]] bool find_bir_select_chain_scalar_materialization_eligibility(
    BirSelectChainIdentityRequest request);

[[nodiscard]] BirSelectChainIdentity find_bir_select_chain_identity(
    BirSelectChainIdentityRequest request);

[[nodiscard]] BirMemoryAccessNodeKind bir_memory_access_node_kind(
    const bir::Inst& inst);

[[nodiscard]] BirMemoryAccessIdentity find_bir_memory_access_identity(
    BirMemoryAccessIdentityRequest request);

[[nodiscard]] BirSameBlockGlobalLoadAccessIdentity
find_bir_same_block_global_load_access_identity(
    BirSameBlockGlobalLoadAccessRequest request);

[[nodiscard]] BirSameBlockLoadLocalSourceIdentity
find_bir_same_block_load_local_source_identity(
    BirSameBlockLoadLocalSourceRequest request);

[[nodiscard]] BirCurrentBlockPublicationIdentity
find_bir_current_block_publication_identity(
    BirCurrentBlockPublicationIdentityRequest request);

[[nodiscard]] BirBlockEntryPublicationIdentity
find_bir_block_entry_publication_identity(
    BirBlockEntryPublicationIdentityRequest request);

[[nodiscard]] BirCfgEdgePublicationSourceIdentity
find_bir_cfg_edge_publication_source_identity(
    BirCfgEdgePublicationSourceRequest request);

[[nodiscard]] BirCurrentBlockJoinSourceIdentity
find_bir_current_block_join_source_identity(
    BirCurrentBlockJoinSourceRequest request);

struct DependencyTraversalRecord {
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
  const bir::Value* source_value = nullptr;
  unsigned depth = 0;
  SameBlockProducerKind kind = SameBlockProducerKind::Unknown;

  [[nodiscard]] explicit operator bool() const { return producer != nullptr; }
};

using DependencyPredicate = bool (*)(const DependencyTraversalRecord& record);

[[nodiscard]] SameBlockProducerKind same_block_producer_kind(const bir::Inst& inst);

[[nodiscard]] bool same_block_producer_kind_has_materialization(
    SameBlockProducerKind kind);

[[nodiscard]] SameBlockValueIdentity same_block_value_identity(
    const bir::Value& value);

[[nodiscard]] SameBlockBinaryProducer find_same_block_binary_producer(
    const bir::Block* block,
    const bir::Value& value);

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] SameBlockProducerRecord find_same_block_named_producer_record(
    const bir::Block* block,
    std::string_view value_name,
    std::size_t before_instruction_index);

[[nodiscard]] SameBlockProducerIdentity find_same_block_producer_identity(
    SameBlockProducerIdentityRequest request);

[[nodiscard]] std::optional<SameBlockScalarProducer>
find_same_block_scalar_producer(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value);

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const bir::Block* block,
    std::string_view value_name,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<SameBlockProducerIndex> producer_instruction_index_record(
    const bir::Block* block,
    const bir::Inst* producer);

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const bir::Block* block,
    const bir::Inst* producer);

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    const bir::Block* block,
    const bir::Value& value,
    unsigned depth = 0);

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value);

[[nodiscard]] bool select_chain_contains_dependency(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    DependencyPredicate matches,
    unsigned depth = 0);

}  // namespace c4c::backend::mir
