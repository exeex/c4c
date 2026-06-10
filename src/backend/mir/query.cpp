#include "query.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace c4c::backend::mir {

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

[[nodiscard]] BirMemoryAccessBaseKind bir_memory_access_base_kind(
    bir::MemoryAddress::BaseKind kind) {
  switch (kind) {
    case bir::MemoryAddress::BaseKind::LocalSlot:
      return BirMemoryAccessBaseKind::LocalSlot;
    case bir::MemoryAddress::BaseKind::GlobalSymbol:
      return BirMemoryAccessBaseKind::GlobalSymbol;
    case bir::MemoryAddress::BaseKind::PointerValue:
      return BirMemoryAccessBaseKind::PointerValue;
    case bir::MemoryAddress::BaseKind::StringConstant:
      return BirMemoryAccessBaseKind::StringConstant;
    case bir::MemoryAddress::BaseKind::None:
    case bir::MemoryAddress::BaseKind::Label:
      return BirMemoryAccessBaseKind::None;
  }
  return BirMemoryAccessBaseKind::None;
}

[[nodiscard]] std::string_view named_value_name(const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named) {
    return {};
  }
  return value.name;
}

void populate_bir_memory_address_identity(
    BirMemoryAccessIdentity& identity,
    const bir::MemoryAddress* address) {
  if (address == nullptr) {
    return;
  }
  identity.address_space = address->address_space;
  identity.is_volatile = address->is_volatile;
  identity.base_kind = bir_memory_access_base_kind(address->base_kind);
  switch (address->base_kind) {
    case bir::MemoryAddress::BaseKind::LocalSlot:
      if (!address->base_name.empty()) {
        identity.local_slot_name = address->base_name;
      }
      if (address->base_slot_id != c4c::kInvalidSlotName) {
        identity.local_slot_id = address->base_slot_id;
      }
      break;
    case bir::MemoryAddress::BaseKind::GlobalSymbol:
      identity.global_name = address->base_name;
      identity.global_name_id = address->base_link_name_id;
      break;
    case bir::MemoryAddress::BaseKind::PointerValue:
      identity.pointer_value_name = named_value_name(address->base_value);
      break;
    case bir::MemoryAddress::BaseKind::StringConstant:
      identity.string_constant_name = address->base_name;
      break;
    case bir::MemoryAddress::BaseKind::None:
    case bir::MemoryAddress::BaseKind::Label:
      break;
  }
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

[[nodiscard]] const bir::Value* produced_value_for_same_block_producer(
    const SameBlockProducerIdentity& producer) {
  if (!producer) {
    return nullptr;
  }
  return producer.produced_value.value;
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

[[nodiscard]] bool same_local_slot_identity(
    const BirMemoryAccessIdentity& lhs,
    const BirMemoryAccessIdentity& rhs) {
  if (!lhs ||
      !rhs ||
      lhs.base_kind != BirMemoryAccessBaseKind::LocalSlot ||
      rhs.base_kind != BirMemoryAccessBaseKind::LocalSlot) {
    return false;
  }
  if (lhs.local_slot_id != c4c::kInvalidSlotName &&
      rhs.local_slot_id != c4c::kInvalidSlotName) {
    return lhs.local_slot_id == rhs.local_slot_id;
  }
  return !lhs.local_slot_name.empty() &&
         lhs.local_slot_name == rhs.local_slot_name;
}

[[nodiscard]] bool has_intervening_same_slot_store(
    const bir::Block& block,
    std::string_view block_label,
    const BirMemoryAccessIdentity& load_access,
    std::size_t after_instruction_index,
    std::size_t before_instruction_index) {
  const auto limit = std::min(before_instruction_index, block.insts.size());
  for (std::size_t index = after_instruction_index; index < limit; ++index) {
    const auto store_access = find_bir_memory_access_identity(
        BirMemoryAccessIdentityRequest{
            .block = &block,
            .block_label = block_label,
            .instruction_index = index,
            .node_kind = BirMemoryAccessNodeKind::StoreLocal,
        });
    if (same_local_slot_identity(load_access, store_access)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value,
    unsigned depth);

[[nodiscard]] std::optional<BirSelectChainDirectGlobalDependency>
find_bir_select_chain_direct_global_dependency(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth) {
  if (block == nullptr ||
      depth > 64U ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer = find_same_block_producer_identity(
      SameBlockProducerIdentityRequest{
          .block = block,
          .value_name = value.name,
          .value_type = value.type,
          .before_instruction_index = before_instruction_index,
      });
  if (!producer || producer.inst == nullptr) {
    return std::nullopt;
  }
  const auto nested_before = producer.instruction_index;
  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(producer.inst);
      load_global != nullptr) {
    return BirSelectChainDirectGlobalDependency{
        .contains_direct_global_load = true,
        .load_global = load_global,
        .instruction_index = producer.instruction_index,
    };
  }
  if (std::get_if<bir::LoadLocalInst>(producer.inst) != nullptr) {
    return BirSelectChainDirectGlobalDependency{};
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer.inst);
      select != nullptr) {
    const auto true_value = find_bir_select_chain_direct_global_dependency(
        block, select->true_value, nested_before, depth + 1U);
    if (!true_value.has_value() || *true_value) {
      return true_value;
    }
    return find_bir_select_chain_direct_global_dependency(
        block, select->false_value, nested_before, depth + 1U);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(producer.inst);
      cast != nullptr) {
    return find_bir_select_chain_direct_global_dependency(
        block, cast->operand, nested_before, depth + 1U);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer.inst);
      binary != nullptr) {
    const auto lhs = find_bir_select_chain_direct_global_dependency(
        block, binary->lhs, nested_before, depth + 1U);
    if (!lhs.has_value() || *lhs) {
      return lhs;
    }
    return find_bir_select_chain_direct_global_dependency(
        block, binary->rhs, nested_before, depth + 1U);
  }
  return std::nullopt;
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
  const auto& inst = request.block->insts[request.instruction_index];
  const auto node_kind = bir_memory_access_node_kind(inst);
  if (node_kind != request.node_kind) {
    return {};
  }

  BirMemoryAccessIdentity identity{
      .inst = &inst,
      .block_label = normalized_block_label(*request.block, request.block_label),
      .instruction_index = request.instruction_index,
      .node_kind = node_kind,
  };
  std::visit(
      [&identity](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          identity.result_value_name = named_value_name(typed_inst.result);
          identity.local_slot_name = typed_inst.slot_name;
          identity.local_slot_id = typed_inst.slot_id;
          populate_bir_memory_address_identity(
              identity,
              typed_inst.address.has_value() ? &*typed_inst.address : nullptr);
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          identity.result_value_name = named_value_name(typed_inst.result);
          identity.global_name = typed_inst.global_name;
          identity.global_name_id = typed_inst.global_name_id;
          populate_bir_memory_address_identity(
              identity,
              typed_inst.address.has_value() ? &*typed_inst.address : nullptr);
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst>) {
          identity.stored_value_name = named_value_name(typed_inst.value);
          identity.local_slot_name = typed_inst.slot_name;
          identity.local_slot_id = typed_inst.slot_id;
          populate_bir_memory_address_identity(
              identity,
              typed_inst.address.has_value() ? &*typed_inst.address : nullptr);
        } else if constexpr (std::is_same_v<T, bir::StoreGlobalInst>) {
          identity.stored_value_name = named_value_name(typed_inst.value);
          identity.global_name = typed_inst.global_name;
          identity.global_name_id = typed_inst.global_name_id;
          populate_bir_memory_address_identity(
              identity,
              typed_inst.address.has_value() ? &*typed_inst.address : nullptr);
        }
      },
      inst);
  return identity;
}

[[nodiscard]] BirSameBlockGlobalLoadAccessIdentity
find_bir_same_block_global_load_access_identity(
    BirSameBlockGlobalLoadAccessRequest request) {
  if (!request) {
    return {};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {};
  }
  const auto value_type = root_value_type(request);
  const auto producer = find_same_block_producer_identity(
      SameBlockProducerIdentityRequest{
          .block = request.block,
          .block_label = request.block_label,
          .value_name = value_name,
          .value_type = value_type,
          .before_instruction_index = request.before_instruction_index,
      });
  if (!producer || producer.kind != SameBlockProducerKind::LoadGlobal) {
    return {};
  }
  const auto* load_global =
      producer.inst != nullptr ? std::get_if<bir::LoadGlobalInst>(producer.inst)
                               : nullptr;
  if (load_global == nullptr) {
    return {};
  }
  const auto memory_access = find_bir_memory_access_identity(
      BirMemoryAccessIdentityRequest{
          .block = request.block,
          .block_label = request.block_label,
          .instruction_index = producer.instruction_index,
          .node_kind = BirMemoryAccessNodeKind::LoadGlobal,
      });
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
  if (!request) {
    return {};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {};
  }
  const auto value_type = root_value_type(request);
  const auto producer = find_same_block_producer_identity(
      SameBlockProducerIdentityRequest{
          .block = request.block,
          .block_label = request.block_label,
          .value_name = value_name,
          .value_type = value_type,
          .before_instruction_index = request.before_instruction_index,
      });
  if (!producer || producer.kind != SameBlockProducerKind::LoadLocal) {
    return {};
  }
  const auto* load_local =
      producer.inst != nullptr ? std::get_if<bir::LoadLocalInst>(producer.inst)
                               : nullptr;
  if (load_local == nullptr) {
    return {};
  }
  const auto memory_access = find_bir_memory_access_identity(
      BirMemoryAccessIdentityRequest{
          .block = request.block,
          .block_label = request.block_label,
          .instruction_index = producer.instruction_index,
          .node_kind = BirMemoryAccessNodeKind::LoadLocal,
      });
  if (!memory_access ||
      memory_access.base_kind != BirMemoryAccessBaseKind::LocalSlot ||
      memory_access.result_value_name != value_name) {
    return {};
  }
  if (has_intervening_same_slot_store(*request.block,
                                      request.block_label,
                                      memory_access,
                                      producer.instruction_index + 1U,
                                      request.before_instruction_index)) {
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
  if (!request) {
    return {};
  }
  const auto before = std::min(request.before_instruction_index,
                               request.block->insts.size());
  for (std::size_t index = before; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto& candidate = request.block->insts[candidate_index];
    const auto* result = produced_value_for_same_block_identity(candidate);
    if (result == nullptr ||
        result->kind != bir::Value::Kind::Named ||
        result->name != request.value_name) {
      continue;
    }
    if (request.value_type != bir::TypeKind::Void &&
        result->type != request.value_type) {
      return {};
    }
    const auto kind = same_block_producer_kind(candidate);
    return SameBlockProducerIdentity{
        .inst = &candidate,
        .instruction_index = candidate_index,
        .kind = kind,
        .block_label = normalized_block_label(*request.block, request.block_label),
        .before_instruction_index = request.before_instruction_index,
        .produced_value = same_block_value_identity(*result),
        .materialization_available =
            same_block_producer_kind_has_materialization(kind),
    };
  }
  return {};
}

[[nodiscard]] SameBlockProducerIdentity find_bir_select_chain_source_producer(
    BirSelectChainIdentityRequest request) {
  if (!request) {
    return {};
  }
  const auto value_name = root_value_name(request);
  if (value_name.empty()) {
    return {};
  }
  return find_same_block_producer_identity(SameBlockProducerIdentityRequest{
      .block = request.block,
      .block_label = request.block_label,
      .value_name = value_name,
      .value_type = root_value_type(request),
      .before_instruction_index = request.before_instruction_index,
  });
}

[[nodiscard]] BirSelectChainDirectGlobalDependency
find_bir_select_chain_direct_global_dependency(
    BirSelectChainIdentityRequest request) {
  const auto root = find_bir_select_chain_source_producer(request);
  const auto* root_value = produced_value_for_same_block_producer(root);
  if (!root || root_value == nullptr) {
    return {};
  }
  const auto dependency = find_bir_select_chain_direct_global_dependency(
      request.block, *root_value, request.before_instruction_index, 0U);
  if (!dependency.has_value()) {
    return {};
  }
  return *dependency;
}

[[nodiscard]] bool find_bir_select_chain_scalar_materialization_eligibility(
    BirSelectChainIdentityRequest request) {
  const auto root = find_bir_select_chain_source_producer(request);
  const auto* root_value = produced_value_for_same_block_producer(root);
  if (!root || root_value == nullptr) {
    return false;
  }
  return find_same_block_scalar_producer(
             SameBlockValueMaterializationQuery{
                 .block = request.block,
                 .block_label = request.block_label,
                 .before_instruction_index = request.before_instruction_index,
             },
             *root_value)
      .has_value();
}

[[nodiscard]] BirSelectChainIdentity find_bir_select_chain_identity(
    BirSelectChainIdentityRequest request) {
  const auto root = find_bir_select_chain_source_producer(request);
  const auto* root_value = produced_value_for_same_block_producer(root);
  if (!root || root_value == nullptr) {
    return BirSelectChainIdentity{
        .root_value_name = root_value_name(request),
    };
  }
  return BirSelectChainIdentity{
      .root_producer = root,
      .root_value = same_block_value_identity(*root_value),
      .root_value_name = root_value->name,
      .root_is_select = root.kind == SameBlockProducerKind::Select,
      .root_instruction_index = root.instruction_index,
      .direct_global_dependency =
          find_bir_select_chain_direct_global_dependency(request),
      .scalar_materialization_available =
          find_bir_select_chain_scalar_materialization_eligibility(request),
  };
}

[[nodiscard]] std::optional<SameBlockScalarProducer>
find_same_block_scalar_producer(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
  if (!query ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer = find_same_block_producer_identity(
      SameBlockProducerIdentityRequest{
          .block = query.block,
          .block_label = query.block_label,
          .value_name = value.name,
          .value_type = value.type,
          .before_instruction_index = query.before_instruction_index,
      });
  const auto* produced_value = produced_value_for_same_block_producer(producer);
  if (!producer ||
      produced_value == nullptr ||
      produced_value->kind != bir::Value::Kind::Named ||
      produced_value->name != value.name ||
      produced_value->type != value.type) {
    return std::nullopt;
  }
  return SameBlockScalarProducer{
      .producer = producer,
      .instruction = producer.inst,
      .produced_value = produced_value,
      .instruction_index = producer.instruction_index,
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
  return evaluate_same_block_integer_constant(
      SameBlockValueMaterializationQuery{
          .block = block,
          .before_instruction_index = block != nullptr ? block->insts.size() : 0U,
      },
      value,
      depth);
}

namespace {

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value,
    unsigned depth) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return SameBlockIntegerConstant{.value = value.immediate, .depth = depth};
  }
  if (!query ||
      depth > 4U ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer = find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->producer.kind != SameBlockProducerKind::Binary ||
      producer->producer.inst == nullptr) {
    return std::nullopt;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(producer->producer.inst);
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto nested_query = SameBlockValueMaterializationQuery{
      .block = query.block,
      .block_label = query.block_label,
      .before_instruction_index = producer->instruction_index,
  };
  const auto lhs =
      evaluate_same_block_integer_constant(nested_query, binary->lhs, depth + 1);
  const auto rhs =
      evaluate_same_block_integer_constant(nested_query, binary->rhs, depth + 1);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  const auto lhs_value = lhs->value;
  const auto rhs_value = rhs->value;
  switch (binary->opcode) {
    case bir::BinaryOpcode::Add:
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) +
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::Sub:
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) -
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::Mul:
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) *
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::And:
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) &
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::Or:
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) |
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::Xor:
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) ^
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::Shl:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value)
                                            << static_cast<unsigned>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::LShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value)
                                            >> static_cast<unsigned>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::AShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{.value = lhs_value >> static_cast<unsigned>(rhs_value),
                                      .depth = depth};
    case bir::BinaryOpcode::SDiv:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{.value = lhs_value / rhs_value, .depth = depth};
    case bir::BinaryOpcode::UDiv:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) /
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::SRem:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{.value = lhs_value % rhs_value, .depth = depth};
    case bir::BinaryOpcode::URem:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return SameBlockIntegerConstant{
          .value = static_cast<std::int64_t>(static_cast<std::uint64_t>(lhs_value) %
                                            static_cast<std::uint64_t>(rhs_value)),
          .depth = depth};
    case bir::BinaryOpcode::Eq:
      return SameBlockIntegerConstant{.value = lhs_value == rhs_value ? 1 : 0,
                                      .depth = depth};
    case bir::BinaryOpcode::Ne:
      return SameBlockIntegerConstant{.value = lhs_value != rhs_value ? 1 : 0,
                                      .depth = depth};
    case bir::BinaryOpcode::Slt:
      return SameBlockIntegerConstant{.value = lhs_value < rhs_value ? 1 : 0,
                                      .depth = depth};
    case bir::BinaryOpcode::Sle:
      return SameBlockIntegerConstant{.value = lhs_value <= rhs_value ? 1 : 0,
                                      .depth = depth};
    case bir::BinaryOpcode::Sgt:
      return SameBlockIntegerConstant{.value = lhs_value > rhs_value ? 1 : 0,
                                      .depth = depth};
    case bir::BinaryOpcode::Sge:
      return SameBlockIntegerConstant{.value = lhs_value >= rhs_value ? 1 : 0,
                                      .depth = depth};
    case bir::BinaryOpcode::Ult:
      return SameBlockIntegerConstant{
          .value = static_cast<std::uint64_t>(lhs_value) <
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .depth = depth};
    case bir::BinaryOpcode::Ule:
      return SameBlockIntegerConstant{
          .value = static_cast<std::uint64_t>(lhs_value) <=
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .depth = depth};
    case bir::BinaryOpcode::Ugt:
      return SameBlockIntegerConstant{
          .value = static_cast<std::uint64_t>(lhs_value) >
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .depth = depth};
    case bir::BinaryOpcode::Uge:
      return SameBlockIntegerConstant{
          .value = static_cast<std::uint64_t>(lhs_value) >=
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .depth = depth};
  }
  return std::nullopt;
}

}  // namespace

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    SameBlockValueMaterializationQuery query,
    const bir::Value& value) {
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
