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
  identity.byte_offset = address->byte_offset;
  identity.size_bytes = address->size_bytes;
  identity.align_bytes = address->align_bytes;
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

[[nodiscard]] SameBlockValueIdentity route1_source_value_identity_to_same_block(
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
  const auto producer = find_same_block_producer_identity(
      SameBlockProducerIdentityRequest{
          .block = request.block,
          .block_label = request.block_label,
          .value_name = value_name,
          .value_type = root_value_type(request),
          .before_instruction_index = request.before_instruction_index,
      });
  if (!producer ||
      producer.inst == nullptr ||
      !producer.produced_value ||
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
      result.status = BirCfgEdgePublicationSourceStatus::MissingDestinationValue;
      return result;
    }

    result.destination_instruction = &inst;
    result.destination_phi = phi;
    result.destination_instruction_index = index;
    result.destination_value = &phi->result;
    result.destination_value_identity = same_block_value_identity(phi->result);
    result.destination_value_name = phi->result.name;
    result.destination_value_type = phi->result.type;

    for (const auto& incoming : phi->incomings) {
      if (!phi_incoming_label_matches(incoming, request)) {
        continue;
      }
      result.source_value = &incoming.value;
      result.source_value_identity = same_block_value_identity(incoming.value);
      result.source_value_kind = incoming.value.kind;
      result.source_value_type = incoming.value.type;
      if (incoming.value.kind == bir::Value::Kind::Named) {
        result.source_value_name = incoming.value.name;
        const auto producer = find_same_block_producer_identity(
            SameBlockProducerIdentityRequest{
                .block = request.predecessor_block,
                .block_label = result.predecessor_label,
                .value_name = incoming.value.name,
                .value_type = incoming.value.type,
                .before_instruction_index = request.predecessor_block->insts.size(),
            });
        if (!producer) {
          result.status =
              BirCfgEdgePublicationSourceStatus::MissingSourceProducer;
          return result;
        }
        result.source_producer = producer;
        result.source_producer_kind = producer.kind;
        result.source_producer_block_label = producer.block_label;
        result.source_producer_block_label_id = result.predecessor_label_id;
        result.source_producer_instruction_index = producer.instruction_index;
        if (producer.kind == SameBlockProducerKind::LoadLocal) {
          const auto load_local = find_bir_same_block_load_local_source_identity(
              BirSameBlockLoadLocalSourceRequest{
                  .block = request.predecessor_block,
                  .block_label = result.predecessor_label,
                  .root_value = &incoming.value,
                  .root_value_name = incoming.value.name,
                  .root_value_type = incoming.value.type,
                  .before_instruction_index =
                      request.predecessor_block->insts.size(),
              });
          if (load_local) {
            result.source_memory_access = load_local.memory_access;
          }
        }
      }
      result.available = true;
      result.status = BirCfgEdgePublicationSourceStatus::Available;
      return result;
    }

    result.status = BirCfgEdgePublicationSourceStatus::MissingSourceValue;
    return result;
  }

  result.status = BirCfgEdgePublicationSourceStatus::MissingPublication;
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
  return find_route1_producer_identity(request);
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
