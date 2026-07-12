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
prepared_publication_source_kind_to_same_block_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return SameBlockProducerKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return SameBlockProducerKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return SameBlockProducerKind::Select;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return SameBlockProducerKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return SameBlockProducerKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return SameBlockProducerKind::Unknown;
  }
  return SameBlockProducerKind::Unknown;
}

[[nodiscard]] bool prepared_publication_source_matches_instruction(
    const prepare::PreparedEdgePublicationSourceProducer& producer,
    const bir::Inst& instruction) {
  switch (producer.kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return producer.binary == std::get_if<bir::BinaryInst>(&instruction);
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return producer.cast == std::get_if<bir::CastInst>(&instruction);
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return producer.select == std::get_if<bir::SelectInst>(&instruction);
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return producer.load_local == std::get_if<bir::LoadLocalInst>(&instruction);
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return producer.load_global == std::get_if<bir::LoadGlobalInst>(&instruction);
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return false;
  }
  return false;
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
    const BirCfgEdgePublicationSourceRequest& request) {
  if (request.destination_value_type != bir::TypeKind::Void) {
    return request.destination_value_type;
  }
  return request.destination_value != nullptr ? request.destination_value->type
                                             : bir::TypeKind::Void;
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
    const prepare::PreparedCurrentBlockPublicationConsumption& prepared) {
  const auto kind = prepared_publication_source_kind_to_same_block_kind(
      prepared.source_producer_kind);
  if (!prepared.available || prepared.source_producer == nullptr ||
      prepared.instruction == nullptr || prepared.produced_value == nullptr ||
      prepared.value_name == kInvalidValueName ||
      prepared.source_producer->kind != prepared.source_producer_kind ||
      prepared.source_producer->instruction_index != prepared.instruction_index ||
      !prepared_publication_source_matches_instruction(
          *prepared.source_producer, *prepared.instruction) ||
      produced_value_for_same_block_identity(*prepared.instruction) !=
          prepared.produced_value ||
      prepared.produced_value->kind != bir::Value::Kind::Named ||
      prepared.produced_value->name.empty() || kind == SameBlockProducerKind::Unknown) {
    return {};
  }
  const auto produced_value = same_block_value_identity(*prepared.produced_value);
  const auto producer = SameBlockProducerIdentity{
      .inst = prepared.instruction,
      .instruction_index = prepared.instruction_index,
      .kind = kind,
      .produced_value = produced_value,
      .materialization_available =
          same_block_producer_kind_has_materialization(kind),
  };
  return BirCurrentBlockPublicationIdentity{
      .available = true,
      .source_producer = producer,
      .instruction = producer.inst,
      .produced_value = prepared.produced_value,
      .produced_value_identity = produced_value,
      .produced_value_name = produced_value.name,
      .produced_value_type = produced_value.type,
      .instruction_index = producer.instruction_index,
      .value_name = produced_value.name,
      .source_producer_kind = producer.kind,
  };
}

[[nodiscard]] BirBlockEntryPublicationIdentity
find_bir_block_entry_publication_identity(
    const prepare::PreparedCurrentBlockEntryPublication& prepared) {
  BirBlockEntryPublicationIdentity result{
      .status = prepared.status,
      .instruction_index = prepared.publication_bundle_instruction_index,
      .destination_value_id = prepared.destination_value_id,
      .destination_value_name = prepared.destination_value_name_text,
      .destination_value_name_id = prepared.destination_value_name,
      .destination_value_type = prepared.destination_value_type,
      .successor_label = prepared.successor_label_text,
      .successor_label_id = prepared.successor_label_id,
  };
  if (prepared.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      !prepared.block_entry_publication_proof_attributed ||
      !prepare::prepared_block_entry_publication_available(prepared.publication) ||
      prepared.successor_label_text.empty() ||
      prepared.successor_label_id == c4c::kInvalidBlockLabel ||
      prepared.destination_value_name_text.empty() ||
      prepared.destination_value_name == c4c::kInvalidValueName ||
      prepared.destination_value_type == bir::TypeKind::Void) {
    return result;
  }
  result.available = true;
  return result;
}

[[nodiscard]] BirBlockEntryPublicationIdentity
find_bir_block_entry_publication_identity(
    const prepare::PreparedCurrentBlockEntryPublication& prepared,
    const bir::Route4BlockEntryPublicationClassification& classification) {
  auto result = find_bir_block_entry_publication_identity(prepared);
  if (prepared.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available) {
    return result;
  }
  if (classification.status ==
      bir::Route4BlockEntryPublicationClassificationStatus::Ambiguous) {
    result.available = false;
    result.status =
        prepare::PreparedCurrentBlockEntryPublicationStatus::ProofAmbiguous;
    return result;
  }
  if (!classification || !classification.selected_claim_index.has_value() ||
      *classification.selected_claim_index >= classification.claims.size()) {
    result.available = false;
    result.status =
        classification.status ==
                bir::Route4BlockEntryPublicationClassificationStatus::Missing ||
            classification.status ==
                bir::Route4BlockEntryPublicationClassificationStatus::Unavailable
        ? prepare::PreparedCurrentBlockEntryPublicationStatus::ProofUnavailable
        : prepare::PreparedCurrentBlockEntryPublicationStatus::ProofMismatch;
    return result;
  }
  const auto& destination = classification.destination;
  const auto& claim =
      classification.claims[*classification.selected_claim_index];
  const auto* phi = claim.instruction == nullptr
                        ? nullptr
                        : std::get_if<bir::PhiInst>(claim.instruction);
  if (!prepared.block_entry_publication_proof_attributed ||
      prepared.block_entry_publication_proof_attribution_id == 0 ||
      !claim.attributed ||
      claim.attribution_id !=
          prepared.block_entry_publication_proof_attribution_id ||
      prepared.block_entry_publication_proof_successor_block !=
          destination.successor_owner ||
      prepared.block_entry_publication_proof_destination_value !=
          destination.destination_value ||
      prepared.block_entry_publication_proof_instruction != claim.instruction ||
      claim.claimed_destination.successor_owner != destination.successor_owner ||
      claim.claimed_destination.destination_value != destination.destination_value ||
      claim.instruction_owner != destination.successor_owner ||
      claim.instruction_owner_label_id != prepared.successor_label_id ||
      destination.successor_label_id != prepared.successor_label_id ||
      destination.destination_value_name_id != prepared.destination_value_name ||
      destination.destination_value_name != prepared.destination_value_name_text ||
      destination.destination_value_type != prepared.destination_value_type ||
      phi == nullptr || &phi->result != destination.destination_value ||
      claim.instruction_index !=
          prepared.block_entry_publication_proof_instruction_index ||
      claim.instruction_index !=
          prepared.publication_bundle_instruction_index) {
    result.available = false;
    result.status =
        prepare::PreparedCurrentBlockEntryPublicationStatus::ProofMismatch;
    return result;
  }

  result.available = true;
  result.instruction_index = claim.instruction_index;
  result.destination_value_name = destination.destination_value_name;
  result.destination_value_name_id = destination.destination_value_name_id;
  result.destination_value_type = destination.destination_value_type;
  result.successor_block = destination.successor_owner;
  result.destination_instruction = claim.instruction;
  result.destination_phi = phi;
  result.destination_value = destination.destination_value;
  result.successor_label = prepared.successor_label_text;
  result.successor_label_id = destination.successor_label_id;
  return result;
}

[[nodiscard]] BirBlockEntryPublicationIdentity
find_bir_block_entry_publication_identity(
    const prepare::PreparedCurrentBlockEntryPublication& prepared,
    const bir::Block* proof_successor_block,
    const bir::Value* proof_destination_value) {
  auto result = find_bir_block_entry_publication_identity(prepared);
  if (proof_successor_block == nullptr || proof_destination_value == nullptr) {
    result.available = false;
    result.status = prepare::PreparedCurrentBlockEntryPublicationStatus::MissingProof;
    return result;
  }

  std::size_t matching_phi_count = 0;
  for (const auto& instruction : proof_successor_block->insts) {
    const auto* phi = std::get_if<bir::PhiInst>(&instruction);
    if (phi == nullptr) break;
    if (phi->result.kind == bir::Value::Kind::Named &&
        phi->result.name == proof_destination_value->name &&
        phi->result.type == proof_destination_value->type) {
      ++matching_phi_count;
    }
  }
  result.available = false;
  result.status = matching_phi_count == 0
                      ? prepare::PreparedCurrentBlockEntryPublicationStatus::ProofUnavailable
                  : matching_phi_count > 1
                      ? prepare::PreparedCurrentBlockEntryPublicationStatus::ProofAmbiguous
                      : prepare::PreparedCurrentBlockEntryPublicationStatus::ProofMismatch;
  return result;
}

[[nodiscard]] BirCfgEdgePublicationSourceIdentity
find_bir_cfg_edge_publication_source_identity(
    BirCfgEdgePublicationSourceRequest request) {
  BirCfgEdgePublicationSourceIdentity result;
  auto fail = [&](BirCfgEdgePublicationSourceStatus status) {
    result.status = status;
    return result;
  };
  if (request.predecessor_block == nullptr) {
    return fail(BirCfgEdgePublicationSourceStatus::MissingPredecessorLabel);
  }
  if (!predecessor_block_label_matches(request)) {
    return fail(BirCfgEdgePublicationSourceStatus::MismatchedRequest);
  }
  if (request.successor_block == nullptr) {
    return fail(BirCfgEdgePublicationSourceStatus::MissingSuccessorLabel);
  }
  if (!successor_block_label_matches(request)) {
    return fail(BirCfgEdgePublicationSourceStatus::MismatchedRequest);
  }
  const auto destination_name = destination_value_name(request);
  const auto destination_type = destination_value_type(request);
  if (request.destination_value == nullptr || destination_name.empty() ||
      destination_type == bir::TypeKind::Void) {
    return fail(BirCfgEdgePublicationSourceStatus::MissingDestinationValue);
  }
  if (request.destination_value->kind != bir::Value::Kind::Named ||
      request.destination_value->name != destination_name ||
      request.destination_value->type != destination_type) {
    return fail(BirCfgEdgePublicationSourceStatus::MismatchedRequest);
  }

  const auto edge = bir::route5_cfg_edge_publication_record(
      request.predecessor_block, request.successor_block,
      *request.destination_value);
  result.predecessor_label = edge.predecessor_label;
  result.predecessor_label_id = edge.predecessor_label_id;
  result.successor_label = edge.successor_label;
  result.successor_label_id = edge.successor_label_id;
  result.destination_instruction = edge.destination_instruction;
  result.destination_phi = edge.destination_phi;
  result.destination_instruction_index = edge.destination_instruction_index;
  result.destination_value = edge.destination_value_ptr;
  result.destination_value_name = edge.destination_value_name;
  result.destination_value_type = edge.destination_value_type;
  result.destination_value_identity = edge.destination_value_ptr != nullptr
      ? same_block_value_identity(*edge.destination_value_ptr)
      : SameBlockValueIdentity{};
  result.source_value = edge.source_value_ptr;
  result.source_value_name = edge.source_value_name;
  result.source_value_name_id = edge.source_value_name_id;
  result.source_value_kind = edge.source_value_kind;
  result.source_value_type = edge.source_value_type;
  result.source_value_identity = edge.source_value_ptr != nullptr
      ? same_block_value_identity(*edge.source_value_ptr)
      : SameBlockValueIdentity{};
  result.source_producer_block_label = request.predecessor_block->label;
  result.source_producer_block_label_id = edge.source_producer_block_label_id;
  result.source_producer_instruction_index = edge.source_producer_instruction_index;

  switch (edge.status) {
    case bir::Route5PublicationStatus::MissingPredecessor:
      return fail(BirCfgEdgePublicationSourceStatus::MissingPredecessorLabel);
    case bir::Route5PublicationStatus::MissingSuccessor:
      return fail(BirCfgEdgePublicationSourceStatus::MissingSuccessorLabel);
    case bir::Route5PublicationStatus::MissingDestination:
      return fail(BirCfgEdgePublicationSourceStatus::MissingDestinationValue);
    case bir::Route5PublicationStatus::MissingSourceValue:
    case bir::Route5PublicationStatus::NoSource:
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceValue);
    case bir::Route5PublicationStatus::MissingSourceProducer:
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
    case bir::Route5PublicationStatus::MissingSourceMemoryAccess:
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceMemoryAccess);
    case bir::Route5PublicationStatus::IncompleteSourceMemoryAccess:
      return fail(BirCfgEdgePublicationSourceStatus::IncompleteSourceMemoryAccess);
    case bir::Route5PublicationStatus::NoMatch:
      return fail(BirCfgEdgePublicationSourceStatus::MismatchedRequest);
    case bir::Route5PublicationStatus::AmbiguousPublication:
      return fail(BirCfgEdgePublicationSourceStatus::AmbiguousPublication);
    case bir::Route5PublicationStatus::Unavailable:
      return fail(BirCfgEdgePublicationSourceStatus::AmbiguousPublication);
    case bir::Route5PublicationStatus::MissingPublication:
      return fail(BirCfgEdgePublicationSourceStatus::MissingPublication);
    case bir::Route5PublicationStatus::Available:
    case bir::Route5PublicationStatus::MemorySource:
      break;
  }
  switch (edge.source_producer_kind) {
    case bir::Route5PublicationSourceKind::Binary:
      result.source_producer_kind = SameBlockProducerKind::Binary;
      break;
    case bir::Route5PublicationSourceKind::Cast:
      result.source_producer_kind = SameBlockProducerKind::Cast;
      break;
    case bir::Route5PublicationSourceKind::SelectMaterialization:
      result.source_producer_kind = SameBlockProducerKind::Select;
      break;
    case bir::Route5PublicationSourceKind::LoadLocal:
      result.source_producer_kind = SameBlockProducerKind::LoadLocal;
      break;
    case bir::Route5PublicationSourceKind::LoadGlobal:
      result.source_producer_kind = SameBlockProducerKind::LoadGlobal;
      break;
    case bir::Route5PublicationSourceKind::Immediate:
      result.source_producer_kind = SameBlockProducerKind::Unknown;
      break;
    case bir::Route5PublicationSourceKind::Unknown:
      return fail(BirCfgEdgePublicationSourceStatus::MissingSourceProducer);
  }
  result.source_producer.inst = edge.source_producer_instruction;
  result.source_producer.kind = result.source_producer_kind;
  result.source_producer.instruction_index =
      edge.source_producer_instruction_index.value_or(0);
  result.source_producer.block_label = request.predecessor_block->label;
  result.source_producer.before_instruction_index =
      request.predecessor_block->insts.size();
  result.source_producer.produced_value = result.source_value_identity;
  result.source_producer.materialization_available =
      edge.source_producer_kind != bir::Route5PublicationSourceKind::Immediate;
  if (edge.source_memory_identity_available) {
    const auto& memory = edge.source_memory_access;
    result.source_memory_access.status = bir::BirViewStatus::Available;
    result.source_memory_access.inst = memory.instruction;
    result.source_memory_access.instruction_index = memory.instruction_index;
    result.source_memory_access.block_label = memory.block_label;
    result.source_memory_access.result_value_name = result.source_value_name;
    result.source_memory_access.address_space = memory.address_space;
    result.source_memory_access.is_volatile = memory.is_volatile;
    result.source_memory_access.byte_offset = memory.byte_offset;
    result.source_memory_access.size_bytes = memory.size_bytes;
    result.source_memory_access.align_bytes = memory.align_bytes;
    if (memory.node_kind == bir::Route3MemoryAccessNodeKind::LoadLocal) {
      result.source_memory_access.node_kind = BirMemoryAccessNodeKind::LoadLocal;
      result.source_memory_access.base_kind = BirMemoryAccessBaseKind::LocalSlot;
      result.source_memory_access.local_slot_name = memory.local_slot_name;
      result.source_memory_access.local_slot_id = memory.local_slot_id;
    } else if (memory.node_kind == bir::Route3MemoryAccessNodeKind::LoadGlobal) {
      result.source_memory_access.node_kind = BirMemoryAccessNodeKind::LoadGlobal;
      result.source_memory_access.base_kind = BirMemoryAccessBaseKind::GlobalSymbol;
      result.source_memory_access.global_name = memory.global_name;
      result.source_memory_access.global_name_id = memory.global_name_id;
    } else {
      return fail(BirCfgEdgePublicationSourceStatus::IncompleteSourceMemoryAccess);
    }
  }
  result.available = true;
  result.status = BirCfgEdgePublicationSourceStatus::Available;
  return result;
}

[[nodiscard]] BirCurrentBlockJoinSourceIdentity
find_bir_current_block_join_source_identity(
    const prepare::PreparedNameTables& names,
    const prepared::PreparedMirDirectEdgePublicationSourceQuery& prepared_query) {
  BirCurrentBlockJoinSourceIdentity result;
  if (prepared_query.status !=
      prepared::PreparedMirDirectEdgePublicationSourceQueryStatus::Available) {
    return result;
  }
  const auto source_status = [](
                                 prepared::PreparedMirDirectEdgePublicationSourceStatus status) {
    switch (status) {
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::Available:
        return BirCurrentBlockJoinSourceStatus::Available;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::MissingPublication:
        return BirCurrentBlockJoinSourceStatus::MissingPublication;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::MissingSelectedFreshness:
        return BirCurrentBlockJoinSourceStatus::MissingSelectedFreshness;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::AmbiguousSourceFreshness:
        return BirCurrentBlockJoinSourceStatus::AmbiguousSourceFreshness;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::InvalidSourceFreshness:
        return BirCurrentBlockJoinSourceStatus::InvalidSourceFreshness;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::UnsupportedSourceHome:
        return BirCurrentBlockJoinSourceStatus::UnsupportedSourceHome;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::UnsupportedDestinationHome:
        return BirCurrentBlockJoinSourceStatus::UnsupportedDestinationHome;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::UnsupportedMove:
        return BirCurrentBlockJoinSourceStatus::UnsupportedMove;
      case prepared::PreparedMirDirectEdgePublicationSourceStatus::UnsupportedSource:
        return BirCurrentBlockJoinSourceStatus::UnsupportedSource;
    }
    return BirCurrentBlockJoinSourceStatus::UnsupportedSource;
  };
  for (const auto& record : prepared_query.sources) {
    const auto duplicate = std::find_if(
        result.facts.begin(), result.facts.end(), [&](const auto& fact) {
          return fact.predecessor_label_id == record.predecessor_label &&
                 fact.successor_label_id == record.successor_label &&
                 fact.destination_prepared_value_id == record.destination_value_id &&
                 fact.prepared_destination_value == record.destination_value &&
                 fact.destination_value_type == record.destination_value.type;
        });
    if (duplicate != result.facts.end()) {
      return BirCurrentBlockJoinSourceIdentity{};
    }
    BirCurrentBlockJoinSourceFact fact{
        .status = source_status(record.status),
        .predecessor_label = prepare::prepared_block_label(names, record.predecessor_label),
        .predecessor_label_id = record.predecessor_label,
        .successor_label = prepare::prepared_block_label(names, record.successor_label),
        .successor_label_id = record.successor_label,
        .prepared_destination_value = record.destination_value,
        .destination_prepared_value_id = record.destination_value_id,
        .destination_value_name = prepare::prepared_value_name(names, record.destination_value_name),
        .destination_value_type = record.destination_value.type,
        .prepared_source_value = record.source_value,
        .source_prepared_value_id = record.source_value_id,
        .source_value_name = prepare::prepared_value_name(names, record.source_value_name),
        .source_value_kind = record.immediate_source ? bir::Value::Kind::Immediate
                                                     : bir::Value::Kind::Named,
        .source_value_type = record.source_value.type,
        .source_producer_instruction_index = record.source_producer_instruction_index,
        .source_producer_block_label = record.source_producer_block_label,
        .source_load_local = record.source_load_local,
        .source_load_global = record.source_load_global,
        .source_cast = record.source_cast,
        .source_binary = record.source_binary,
        .source_select = record.source_select,
        .publication_move_bundle_identity = record.bundle,
        .publication_move_identity = record.move,
        .publication_identity = record.publication,
        .source_home_kind = record.source_home_kind,
        .destination_home_kind = record.destination_home_kind,
        .destination_storage_kind = record.destination_storage_kind,
        .source_freshness_status = record.source_freshness_status,
        .source_freshness_candidate_count = record.source_freshness_candidate_count,
        .selected_freshness_authority = record.selected_freshness_authority,
    };
    fact.destination_value_identity.name = fact.destination_value_name;
    fact.source_value_identity.name = fact.source_value_name;
    if (record.immediate_source && record.source_immediate_i32.has_value()) {
      fact.source_value_identity.immediate_constant = *record.source_immediate_i32;
    }
    switch (record.source_producer_kind) {
      case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
        fact.source_producer_kind = SameBlockProducerKind::Unknown;
        break;
      case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
        fact.source_producer_kind = SameBlockProducerKind::LoadLocal;
        break;
      case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
        fact.source_producer_kind = SameBlockProducerKind::LoadGlobal;
        break;
      case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
        fact.source_producer_kind = SameBlockProducerKind::Cast;
        break;
      case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
        fact.source_producer_kind = SameBlockProducerKind::Binary;
        break;
      case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
        fact.source_producer_kind = SameBlockProducerKind::Select;
        break;
      case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
        fact.source_producer_kind = SameBlockProducerKind::Unknown;
        break;
    }
    fact.source_producer.kind = fact.source_producer_kind;
    fact.source_producer.instruction_index =
        fact.source_producer_instruction_index.value_or(0);
    fact.source_producer.block_label = fact.predecessor_label;
    fact.source_producer.before_instruction_index =
        fact.source_producer_instruction_index.value_or(0) + 1;
    fact.source_producer.produced_value = fact.source_value_identity;
    fact.source_producer.materialization_available = !record.immediate_source;

    const bool base_authority =
        record.bundle != nullptr && record.move != nullptr &&
        record.publication != nullptr && record.destination_value_id != 0 &&
        record.destination_value.type != bir::TypeKind::Void &&
        record.source_value.type != bir::TypeKind::Void &&
        record.destination_value.kind == bir::Value::Kind::Named &&
        fact.destination_value_name == record.destination_value.name &&
        (record.immediate_source ||
         (record.source_value_id.has_value() &&
          fact.source_value_name == record.source_value.name));
    const auto producer_pointer_count =
        static_cast<unsigned>(record.source_load_local != nullptr) +
        static_cast<unsigned>(record.source_load_global != nullptr) +
        static_cast<unsigned>(record.source_cast != nullptr) +
        static_cast<unsigned>(record.source_binary != nullptr) +
        static_cast<unsigned>(record.source_select != nullptr);
    const bool producer_authority = record.immediate_source
        ? record.source_producer_kind ==
              prepare::PreparedEdgePublicationSourceProducerKind::Immediate &&
              !record.source_producer_instruction_index.has_value() &&
              !record.source_producer_block_label.has_value() &&
              producer_pointer_count == 0
        : record.source_producer_block_label.has_value() &&
              *record.source_producer_block_label == record.predecessor_label &&
              record.source_producer_instruction_index.has_value() &&
              producer_pointer_count == 1 &&
              ((record.source_producer_kind == prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal && record.source_load_local != nullptr) ||
               (record.source_producer_kind == prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal && record.source_load_global != nullptr) ||
               (record.source_producer_kind == prepare::PreparedEdgePublicationSourceProducerKind::Cast && record.source_cast != nullptr) ||
               (record.source_producer_kind == prepare::PreparedEdgePublicationSourceProducerKind::Binary && record.source_binary != nullptr) ||
               (record.source_producer_kind == prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization && record.source_select != nullptr));
    const bool freshness_authority = record.immediate_source
        ? record.source_freshness_status == prepare::PreparedValueFreshnessQueryStatus::NoCandidate &&
              !record.selected_freshness_authority.has_value()
        : record.source_freshness_status == prepare::PreparedValueFreshnessQueryStatus::Selected &&
              record.selected_freshness_authority.has_value() &&
              record.selected_freshness_authority->value_id == *record.source_value_id &&
              record.selected_freshness_authority->value_name == record.source_value_name &&
              record.selected_freshness_authority->reference.edge_publication == record.publication &&
              record.selected_freshness_authority->reference.move == record.move;
    if (fact.status == BirCurrentBlockJoinSourceStatus::Available &&
        (!base_authority || !producer_authority || !freshness_authority)) {
      fact.status = BirCurrentBlockJoinSourceStatus::MissingPublication;
    }
    if (result.successor_label_id == c4c::kInvalidBlockLabel) {
      result.successor_label_id = record.successor_label;
      result.successor_label = fact.successor_label;
    } else if (result.successor_label_id != record.successor_label) {
      return BirCurrentBlockJoinSourceIdentity{};
    }
    result.facts.push_back(fact);
  }

  if (result.facts.empty()) {
    result.status = BirCurrentBlockJoinSourceStatus::MissingPublication;
    return result;
  }

  result.available =
      std::all_of(result.facts.begin(), result.facts.end(), [](const auto& fact) {
        return fact.status == BirCurrentBlockJoinSourceStatus::Available;
      });
  if (result.available) {
    result.status = BirCurrentBlockJoinSourceStatus::Available;
  } else {
    result.status = BirCurrentBlockJoinSourceStatus::MissingPublication;
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
