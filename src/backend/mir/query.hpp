#pragma once

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

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
