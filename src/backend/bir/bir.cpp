#include "bir.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace c4c::backend::bir {
namespace {

[[nodiscard]] const Value* produced_value_for_comparison_producer(
    const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const Value* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, BinaryInst> ||
                      std::is_same_v<T, SelectInst> ||
                      std::is_same_v<T, CastInst> ||
                      std::is_same_v<T, LoadLocalInst> ||
                      std::is_same_v<T, LoadGlobalInst>) {
          return &candidate.result;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] const Value* produced_value_for_route1_producer(
    const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const Value* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, BinaryInst> ||
                      std::is_same_v<T, SelectInst> ||
                      std::is_same_v<T, CastInst> ||
                      std::is_same_v<T, LoadLocalInst> ||
                      std::is_same_v<T, LoadGlobalInst>) {
          return &candidate.result;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] ComparisonProducerKind comparison_producer_kind_for_inst(
    const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> ComparisonProducerKind {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst>) {
          return ComparisonProducerKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
          return ComparisonProducerKind::LoadGlobal;
        } else if constexpr (std::is_same_v<T, CastInst>) {
          return ComparisonProducerKind::Cast;
        } else if constexpr (std::is_same_v<T, BinaryInst>) {
          return ComparisonProducerKind::Binary;
        } else if constexpr (std::is_same_v<T, SelectInst>) {
          return ComparisonProducerKind::Select;
        } else {
          return ComparisonProducerKind::Unknown;
        }
      },
      inst);
}

[[nodiscard]] bool is_comparison_binary_opcode(BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Eq:
    case BinaryOpcode::Ne:
    case BinaryOpcode::Slt:
    case BinaryOpcode::Sle:
    case BinaryOpcode::Sgt:
    case BinaryOpcode::Sge:
    case BinaryOpcode::Ult:
    case BinaryOpcode::Ule:
    case BinaryOpcode::Ugt:
    case BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

struct SameBlockComparisonProducer {
  const Inst* inst = nullptr;
  const Value* produced_value = nullptr;
  std::size_t instruction_index = 0;
  bool ambiguous = false;
};

[[nodiscard]] SameBlockComparisonProducer find_unique_comparison_producer(
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  if (value.kind != Value::Kind::Named || value.name.empty()) {
    return {};
  }
  const std::size_t before =
      std::min(before_instruction_index, block.insts.size());
  SameBlockComparisonProducer result;
  for (std::size_t index = before; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto& candidate = block.insts[candidate_index];
    const auto* produced = produced_value_for_comparison_producer(candidate);
    if (produced == nullptr ||
        produced->kind != Value::Kind::Named ||
        produced->name != value.name ||
        produced->type != value.type) {
      continue;
    }
    if (result.inst != nullptr) {
      result.ambiguous = true;
      return result;
    }
    result = SameBlockComparisonProducer{
        .inst = &candidate,
        .produced_value = produced,
        .instruction_index = candidate_index,
    };
  }
  return result;
}

[[nodiscard]] std::optional<std::int64_t> evaluate_comparison_integer_constant(
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  if (value.kind == Value::Kind::Immediate) {
    return value.immediate;
  }
  if (depth > 4U ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer =
      find_unique_comparison_producer(block, value, before_instruction_index);
  if (producer.inst == nullptr || producer.ambiguous) {
    return std::nullopt;
  }
  const auto* binary = std::get_if<BinaryInst>(producer.inst);
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto lhs = evaluate_comparison_integer_constant(
      block, binary->lhs, producer.instruction_index, depth + 1U);
  const auto rhs = evaluate_comparison_integer_constant(
      block, binary->rhs, producer.instruction_index, depth + 1U);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  const auto lhs_value = *lhs;
  const auto rhs_value = *rhs;
  switch (binary->opcode) {
    case BinaryOpcode::Add:
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(lhs_value) +
          static_cast<std::uint64_t>(rhs_value));
    case BinaryOpcode::Sub:
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(lhs_value) -
          static_cast<std::uint64_t>(rhs_value));
    case BinaryOpcode::Mul:
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(lhs_value) *
          static_cast<std::uint64_t>(rhs_value));
    case BinaryOpcode::And:
      return lhs_value & rhs_value;
    case BinaryOpcode::Or:
      return lhs_value | rhs_value;
    case BinaryOpcode::Xor:
      return lhs_value ^ rhs_value;
    case BinaryOpcode::UDiv:
      return rhs_value != 0
                 ? std::optional<std::int64_t>{static_cast<std::int64_t>(
                       static_cast<std::uint64_t>(lhs_value) /
                       static_cast<std::uint64_t>(rhs_value))}
                 : std::nullopt;
    default:
      return std::nullopt;
  }
}

}  // namespace

Route1SourceValueIdentity route1_source_value_identity(
    const Value& value,
    ValueNameId name_id) {
  return Route1SourceValueIdentity{
      .value = &value,
      .value_kind = value.kind,
      .type = value.type,
      .name = value.kind == Value::Kind::Named ? std::string_view(value.name)
                                               : std::string_view{},
      .name_id = name_id,
      .integer_constant = value.kind == Value::Kind::Immediate
                              ? std::optional<std::int64_t>{value.immediate}
                              : std::nullopt,
      .pointer_symbol_link_name_id = value.pointer_symbol_link_name_id,
  };
}

Route1ImmediateIntegerConstant route1_immediate_integer_constant(
    const Value& value,
    unsigned depth) {
  if (value.kind != Value::Kind::Immediate) {
    return {};
  }
  return Route1ImmediateIntegerConstant{
      .available = true,
      .value = value.immediate,
      .type = value.type,
      .depth = depth,
  };
}

Route1ProducerKind route1_producer_kind(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> Route1ProducerKind {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst>) {
          return Route1ProducerKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
          return Route1ProducerKind::LoadGlobal;
        } else if constexpr (std::is_same_v<T, CastInst>) {
          return Route1ProducerKind::Cast;
        } else if constexpr (std::is_same_v<T, BinaryInst>) {
          return Route1ProducerKind::Binary;
        } else if constexpr (std::is_same_v<T, SelectInst>) {
          return Route1ProducerKind::SelectMaterialization;
        } else {
          return Route1ProducerKind::Unknown;
        }
      },
      inst);
}

const Value* route1_produced_value(const Inst& inst) {
  return produced_value_for_route1_producer(inst);
}

Route1ProducerInstructionIdentity route1_producer_instruction_identity(
    const Block& block,
    std::size_t instruction_index) {
  if (instruction_index >= block.insts.size()) {
    return {};
  }
  const auto& inst = block.insts[instruction_index];
  const auto kind = route1_producer_kind(inst);
  if (kind == Route1ProducerKind::Unknown) {
    return {};
  }
  return Route1ProducerInstructionIdentity{
      .instruction = &inst,
      .instruction_index = instruction_index,
      .kind = kind,
      .block_label = block.label,
      .block_label_id = block.label_id,
  };
}

Route1ProducerRecord route1_producer_record(const Block& block,
                                            std::size_t instruction_index) {
  const auto instruction =
      route1_producer_instruction_identity(block, instruction_index);
  if (!instruction) {
    return {};
  }
  const auto* produced_value = route1_produced_value(*instruction.instruction);
  if (produced_value == nullptr) {
    return {};
  }
  const auto kind = instruction.kind;
  return Route1ProducerRecord{
      .available = true,
      .kind = kind,
      .source_value = route1_source_value_identity(*produced_value),
      .producer_instruction = instruction,
      .integer_constant = route1_immediate_integer_constant(*produced_value),
      .materialization =
          Route1MaterializationAvailability{
              .available = kind != Route1ProducerKind::Unknown,
              .scalar_materialization_available =
                  route1_producer_kind_has_materialization(kind),
              .producer_kind = kind,
          },
  };
}

Route1ProducerIndex route1_build_producer_index(const Block& block) {
  Route1ProducerIndex index{
      .block = &block,
  };
  index.records.reserve(block.insts.size());
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    const auto record = route1_producer_record(block, instruction_index);
    if (record) {
      index.records.push_back(record);
    }
  }
  return index;
}

std::optional<Route1SameBlockScalarProducer>
route1_find_same_block_scalar_producer(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  if (!query ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& record = *it;
    if (!record ||
        !record.producer_instruction ||
        record.producer_instruction.instruction_index >=
            query.before_instruction_index ||
        !record.source_value ||
        record.source_value.value == nullptr ||
        record.source_value.value_kind != Value::Kind::Named ||
        record.source_value.name != value.name ||
        record.source_value.type != value.type) {
      continue;
    }
    return Route1SameBlockScalarProducer{
        .record = &record,
        .instruction = record.producer_instruction.instruction,
        .produced_value = record.source_value.value,
        .instruction_index = record.producer_instruction.instruction_index,
        .materialization = record.materialization,
    };
  }
  return std::nullopt;
}

Route1MaterializationAvailability route1_find_materialization_availability(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value()) {
    return {};
  }
  return producer->materialization;
}

Route2SelectChainProducerKind route2_select_chain_producer_kind(
    const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> Route2SelectChainProducerKind {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst>) {
          return Route2SelectChainProducerKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
          return Route2SelectChainProducerKind::LoadGlobal;
        } else if constexpr (std::is_same_v<T, CastInst>) {
          return Route2SelectChainProducerKind::Cast;
        } else if constexpr (std::is_same_v<T, BinaryInst>) {
          return Route2SelectChainProducerKind::Binary;
        } else if constexpr (std::is_same_v<T, SelectInst>) {
          return Route2SelectChainProducerKind::Select;
        } else {
          return Route2SelectChainProducerKind::Unknown;
        }
      },
      inst);
}

Route2SelectChainProducerRecord route2_select_chain_producer_record(
    const Block& block,
    std::size_t instruction_index) {
  if (instruction_index >= block.insts.size()) {
    return {};
  }
  const auto& inst = block.insts[instruction_index];
  const auto kind = route2_select_chain_producer_kind(inst);
  const auto* produced_value = route1_produced_value(inst);
  if (kind == Route2SelectChainProducerKind::Unknown ||
      produced_value == nullptr) {
    return {};
  }
  Route2SelectChainProducerRecord record{
      .available = true,
      .kind = kind,
      .instruction = &inst,
      .instruction_index = instruction_index,
      .produced_value = produced_value,
      .block_label = block.label,
      .block_label_id = block.label_id,
  };
  if (const auto* load_global = std::get_if<LoadGlobalInst>(&inst);
      load_global != nullptr) {
    record.global_name = load_global->global_name;
    record.global_name_id = load_global->global_name_id;
  }
  return record;
}

namespace {

[[nodiscard]] std::optional<Route2SelectChainDirectGlobalDependencyRecord>
route2_find_direct_global_dependency(
    Route1SameBlockProducerQuery query,
    const Value& value,
    unsigned depth) {
  if (!query ||
      depth > 64U ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->instruction == nullptr ||
      producer->produced_value == nullptr) {
    return std::nullopt;
  }
  const auto nested_query = Route1SameBlockProducerQuery{
      .index = query.index,
      .before_instruction_index = producer->instruction_index,
  };
  if (const auto* load_global =
          std::get_if<LoadGlobalInst>(producer->instruction);
      load_global != nullptr) {
    return Route2SelectChainDirectGlobalDependencyRecord{
        .available = true,
        .contains_direct_global_load = true,
        .load_global = load_global,
        .direct_load_instruction_index = producer->instruction_index,
        .global_name = load_global->global_name,
        .global_name_id = load_global->global_name_id,
    };
  }
  if (std::get_if<LoadLocalInst>(producer->instruction) != nullptr) {
    return Route2SelectChainDirectGlobalDependencyRecord{.available = true};
  }
  if (const auto* select = std::get_if<SelectInst>(producer->instruction);
      select != nullptr) {
    const auto true_value = route2_find_direct_global_dependency(
        nested_query, select->true_value, depth + 1U);
    if (!true_value.has_value() || *true_value) {
      return true_value;
    }
    return route2_find_direct_global_dependency(
        nested_query, select->false_value, depth + 1U);
  }
  if (const auto* cast = std::get_if<CastInst>(producer->instruction);
      cast != nullptr) {
    return route2_find_direct_global_dependency(
        nested_query, cast->operand, depth + 1U);
  }
  if (const auto* binary = std::get_if<BinaryInst>(producer->instruction);
      binary != nullptr) {
    const auto lhs = route2_find_direct_global_dependency(
        nested_query, binary->lhs, depth + 1U);
    if (!lhs.has_value() || *lhs) {
      return lhs;
    }
    return route2_find_direct_global_dependency(
        nested_query, binary->rhs, depth + 1U);
  }
  return std::nullopt;
}

}  // namespace

Route2SelectChainValueRecord route2_select_chain_value_record(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->instruction == nullptr ||
      producer->produced_value == nullptr ||
      query.index == nullptr ||
      query.index->block == nullptr) {
    return {};
  }
  const auto root_producer = route2_select_chain_producer_record(
      *query.index->block, producer->instruction_index);
  if (!root_producer) {
    return {};
  }
  auto direct_dependency =
      route2_find_direct_global_dependency(query, *producer->produced_value, 0U)
          .value_or(Route2SelectChainDirectGlobalDependencyRecord{});
  direct_dependency.root_is_select =
      root_producer.kind == Route2SelectChainProducerKind::Select;
  direct_dependency.root_instruction_index = producer->instruction_index;
  const auto materialization =
      route1_find_materialization_availability(query, *producer->produced_value);
  return Route2SelectChainValueRecord{
      .available = true,
      .root_value = route1_source_value_identity(*producer->produced_value),
      .root_value_name = producer->produced_value->kind == Value::Kind::Named
                             ? std::string_view(producer->produced_value->name)
                             : std::string_view{},
      .root_is_select =
          root_producer.kind == Route2SelectChainProducerKind::Select,
      .root_instruction_index = producer->instruction_index,
      .scalar_materialization_available =
          materialization.scalar_materialization_available,
      .root_producer = root_producer,
      .direct_global_dependency = direct_dependency,
  };
}

Route2SelectChainValueIndex route2_build_select_chain_value_index(
    const Block& block) {
  Route2SelectChainValueIndex index{
      .block = &block,
  };
  index.records.reserve(block.insts.size());
  const auto route1_index = route1_build_producer_index(block);
  for (const auto& route1_record : route1_index.records) {
    if (!route1_record ||
        !route1_record.source_value ||
        route1_record.source_value.value == nullptr) {
      continue;
    }
    const auto query = Route1SameBlockProducerQuery{
        .index = &route1_index,
        .before_instruction_index =
            route1_record.producer_instruction.instruction_index + 1U,
    };
    const auto record =
        route2_select_chain_value_record(query, *route1_record.source_value.value);
    if (record) {
      index.records.push_back(record);
    }
  }
  return index;
}

const Route2SelectChainValueRecord* route2_find_select_chain_value_record(
    Route2SelectChainValueQuery query,
    const Value& value) {
  if (!query ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& record = *it;
    if (!record ||
        record.root_value.value == nullptr ||
        record.root_value.value_kind != Value::Kind::Named ||
        record.root_value.name != value.name ||
        record.root_value.type != value.type ||
        !record.root_instruction_index.has_value() ||
        *record.root_instruction_index >= query.before_instruction_index) {
      continue;
    }
    return &record;
  }
  return nullptr;
}

Route3MemoryAccessNodeKind route3_memory_access_node_kind(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> Route3MemoryAccessNodeKind {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst>) {
          return Route3MemoryAccessNodeKind::LoadLocal;
        } else if constexpr (std::is_same_v<T, LoadGlobalInst>) {
          return Route3MemoryAccessNodeKind::LoadGlobal;
        } else if constexpr (std::is_same_v<T, StoreLocalInst>) {
          return Route3MemoryAccessNodeKind::StoreLocal;
        } else if constexpr (std::is_same_v<T, StoreGlobalInst>) {
          return Route3MemoryAccessNodeKind::StoreGlobal;
        } else {
          return Route3MemoryAccessNodeKind::Unknown;
        }
      },
      inst);
}

Route3MemoryAccessBaseKind route3_memory_access_base_kind(
    const MemoryAddress& address) {
  switch (address.base_kind) {
    case MemoryAddress::BaseKind::None:
      return Route3MemoryAccessBaseKind::None;
    case MemoryAddress::BaseKind::LocalSlot:
      return Route3MemoryAccessBaseKind::LocalSlot;
    case MemoryAddress::BaseKind::GlobalSymbol:
      return Route3MemoryAccessBaseKind::GlobalSymbol;
    case MemoryAddress::BaseKind::PointerValue:
      return Route3MemoryAccessBaseKind::PointerValue;
    case MemoryAddress::BaseKind::StringConstant:
      return Route3MemoryAccessBaseKind::StringConstant;
    case MemoryAddress::BaseKind::Label:
      return Route3MemoryAccessBaseKind::None;
  }
  return Route3MemoryAccessBaseKind::None;
}

namespace {

[[nodiscard]] const Value* route3_result_value(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const Value* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst> ||
                      std::is_same_v<T, LoadGlobalInst>) {
          return &candidate.result;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] const Value* route3_stored_value(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const Value* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, StoreLocalInst> ||
                      std::is_same_v<T, StoreGlobalInst>) {
          return &candidate.value;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] const MemoryAddress* route3_memory_address(const Inst& inst) {
  return std::visit(
      [](const auto& candidate) -> const MemoryAddress* {
        using T = std::decay_t<decltype(candidate)>;
        if constexpr (std::is_same_v<T, LoadLocalInst> ||
                      std::is_same_v<T, LoadGlobalInst> ||
                      std::is_same_v<T, StoreLocalInst> ||
                      std::is_same_v<T, StoreGlobalInst>) {
          return candidate.address.has_value() ? &*candidate.address : nullptr;
        } else {
          return nullptr;
        }
      },
      inst);
}

[[nodiscard]] bool route3_same_local_slot(
    const Route3MemoryAccessRecord& lhs,
    const Route3MemoryAccessRecord& rhs) {
  if (!lhs ||
      !rhs ||
      lhs.base_kind != Route3MemoryAccessBaseKind::LocalSlot ||
      rhs.base_kind != Route3MemoryAccessBaseKind::LocalSlot) {
    return false;
  }
  if (lhs.local_slot_id != kInvalidSlotName &&
      rhs.local_slot_id != kInvalidSlotName) {
    return lhs.local_slot_id == rhs.local_slot_id;
  }
  return !lhs.local_slot_name.empty() &&
         lhs.local_slot_name == rhs.local_slot_name;
}

}  // namespace

Route3MemoryAccessRecord route3_memory_access_record(
    const Block& block,
    std::size_t instruction_index) {
  if (instruction_index >= block.insts.size()) {
    return {};
  }
  const auto& inst = block.insts[instruction_index];
  const auto node_kind = route3_memory_access_node_kind(inst);
  if (node_kind == Route3MemoryAccessNodeKind::Unknown) {
    return {};
  }
  const auto* address = route3_memory_address(inst);
  if (address == nullptr) {
    return {};
  }
  const auto base_kind = route3_memory_access_base_kind(*address);
  if (address->base_kind == MemoryAddress::BaseKind::Label) {
    return {};
  }
  Route3MemoryAccessRecord record{
      .available = true,
      .instruction = &inst,
      .instruction_index = instruction_index,
      .node_kind = node_kind,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .address_space = address->address_space,
      .is_volatile = address->is_volatile,
      .base_kind = base_kind,
  };
  if (const auto* result = route3_result_value(inst); result != nullptr) {
    record.result_value = route1_source_value_identity(*result);
  }
  if (const auto* stored = route3_stored_value(inst); stored != nullptr) {
    record.stored_value = route1_source_value_identity(*stored);
  }
  switch (base_kind) {
    case Route3MemoryAccessBaseKind::LocalSlot:
      record.local_slot_name = address->base_name;
      record.local_slot_id = address->base_slot_id;
      break;
    case Route3MemoryAccessBaseKind::GlobalSymbol:
      record.global_name = address->base_name;
      record.global_name_id = address->base_link_name_id;
      break;
    case Route3MemoryAccessBaseKind::PointerValue:
      record.pointer_value = route1_source_value_identity(address->base_value);
      break;
    case Route3MemoryAccessBaseKind::StringConstant:
      record.string_constant_name = address->base_name;
      record.string_constant_name_id = address->base_link_name_id;
      break;
    case Route3MemoryAccessBaseKind::None:
      break;
  }
  return record;
}

Route3MemoryAccessValueRecord route3_memory_access_result_value_record(
    const Block& block,
    std::size_t instruction_index) {
  const auto access = route3_memory_access_record(block, instruction_index);
  if (!access || !access.result_value) {
    return {};
  }
  return Route3MemoryAccessValueRecord{
      .available = true,
      .role = Route3MemoryAccessValueRole::Result,
      .value = access.result_value,
      .access_instruction_index = access.instruction_index,
      .node_kind = access.node_kind,
      .access = access,
  };
}

Route3MemoryAccessValueRecord route3_memory_access_stored_value_record(
    const Block& block,
    std::size_t instruction_index) {
  const auto access = route3_memory_access_record(block, instruction_index);
  if (!access || !access.stored_value) {
    return {};
  }
  return Route3MemoryAccessValueRecord{
      .available = true,
      .role = Route3MemoryAccessValueRole::Stored,
      .value = access.stored_value,
      .access_instruction_index = access.instruction_index,
      .node_kind = access.node_kind,
      .access = access,
  };
}

Route3MemoryAccessIndex route3_build_memory_access_index(const Block& block) {
  Route3MemoryAccessIndex index{
      .block = &block,
  };
  index.records.reserve(block.insts.size());
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    const auto record = route3_memory_access_record(block, instruction_index);
    if (record) {
      index.records.push_back(record);
    }
  }
  return index;
}

const Route3MemoryAccessRecord* route3_find_memory_access_record(
    const Route3MemoryAccessIndex& index,
    std::size_t instruction_index,
    Route3MemoryAccessNodeKind node_kind) {
  if (!index) {
    return nullptr;
  }
  for (const auto& record : index.records) {
    if (!record ||
        record.instruction_index != instruction_index ||
        record.node_kind != node_kind) {
      continue;
    }
    return &record;
  }
  return nullptr;
}

Route3SameBlockGlobalLoadAccessRecord
route3_same_block_global_load_access_record(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  Route3SameBlockGlobalLoadAccessRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available || query.index->block == nullptr) {
    return {};
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->record == nullptr ||
      producer->record->kind != Route1ProducerKind::LoadGlobal) {
    return record;
  }
  record.load_instruction_index = producer->instruction_index;
  record.load_access =
      route3_memory_access_record(*query.index->block, producer->instruction_index);
  record.access_available =
      record.load_access &&
      record.load_access.node_kind == Route3MemoryAccessNodeKind::LoadGlobal &&
      record.load_access.base_kind == Route3MemoryAccessBaseKind::GlobalSymbol;
  return record;
}

Route3SameBlockLoadLocalSourceRecord
route3_same_block_load_local_source_record(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  Route3SameBlockLoadLocalSourceRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available || query.index->block == nullptr) {
    return {};
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->record == nullptr ||
      producer->record->kind != Route1ProducerKind::LoadLocal) {
    return record;
  }
  const auto& block = *query.index->block;
  record.load_instruction_index = producer->instruction_index;
  record.load_access =
      route3_memory_access_record(block, producer->instruction_index);
  if (!record.load_access ||
      record.load_access.node_kind != Route3MemoryAccessNodeKind::LoadLocal ||
      record.load_access.base_kind != Route3MemoryAccessBaseKind::LocalSlot) {
    return record;
  }
  const auto limit = std::min(query.before_instruction_index, block.insts.size());
  for (std::size_t index = producer->instruction_index + 1U; index < limit;
       ++index) {
    const auto store_access = route3_memory_access_record(block, index);
    if (store_access.node_kind == Route3MemoryAccessNodeKind::StoreLocal &&
        route3_same_local_slot(record.load_access, store_access)) {
      record.invalidating_store_instruction_index = index;
      record.invalidating_store_access = store_access;
      return record;
    }
  }
  record.source_available = true;
  return record;
}

Route3SameBlockGlobalLoadAccessRecord
route3_find_same_block_global_load_access(
    Route3MemoryAccessQuery query,
    const Value& value) {
  Route3SameBlockGlobalLoadAccessRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available) {
    return {};
  }
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::LoadGlobal ||
        candidate.base_kind != Route3MemoryAccessBaseKind::GlobalSymbol ||
        candidate.instruction_index >= query.before_instruction_index ||
        candidate.result_value.value_kind != Value::Kind::Named ||
        candidate.result_value.name != value.name ||
        candidate.result_value.type != value.type) {
      continue;
    }
    record.load_instruction_index = candidate.instruction_index;
    record.load_access = candidate;
    record.access_available = true;
    return record;
  }
  return record;
}

Route3SameBlockLoadLocalSourceRecord
route3_find_same_block_load_local_source(
    Route3MemoryAccessQuery query,
    const Value& value) {
  Route3SameBlockLoadLocalSourceRecord record{
      .available = value.kind == Value::Kind::Named && !value.name.empty(),
      .root_value = route1_source_value_identity(value),
  };
  if (!query || !record.available) {
    return {};
  }
  const Route3MemoryAccessRecord* load_access = nullptr;
  for (auto it = query.index->records.rbegin();
       it != query.index->records.rend();
       ++it) {
    const auto& candidate = *it;
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::LoadLocal ||
        candidate.base_kind != Route3MemoryAccessBaseKind::LocalSlot ||
        candidate.instruction_index >= query.before_instruction_index ||
        candidate.result_value.value_kind != Value::Kind::Named ||
        candidate.result_value.name != value.name ||
        candidate.result_value.type != value.type) {
      continue;
    }
    load_access = &candidate;
    break;
  }
  if (load_access == nullptr) {
    return record;
  }
  record.load_instruction_index = load_access->instruction_index;
  record.load_access = *load_access;
  for (const auto& candidate : query.index->records) {
    if (!candidate ||
        candidate.node_kind != Route3MemoryAccessNodeKind::StoreLocal ||
        candidate.instruction_index <= load_access->instruction_index ||
        candidate.instruction_index >= query.before_instruction_index ||
        !route3_same_local_slot(*load_access, candidate)) {
      continue;
    }
    record.invalidating_store_instruction_index = candidate.instruction_index;
    record.invalidating_store_access = candidate;
    return record;
  }
  record.source_available = true;
  return record;
}

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

[[nodiscard]] bool route4_record_matches_block(
    const Function& function,
    const Block* record_block,
    std::string_view record_label,
    BlockLabelId record_label_id,
    const Block& block) {
  const auto block_is_indexed =
      std::any_of(function.blocks.begin(),
                  function.blocks.end(),
                  [&](const Block& indexed_block) {
                    return &indexed_block == &block;
                  });
  if (block_is_indexed) {
    return record_block == &block;
  }
  if (record_label_id != kInvalidBlockLabel &&
      block.label_id != kInvalidBlockLabel) {
    return record_label_id == block.label_id;
  }
  return !record_label.empty() && record_label == block.label;
}

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

Route4BlockEntryPublicationRecord route4_find_block_entry_publication(
    const Route4PublicationAvailabilityIndex& index,
    const Block& successor_block,
    const Value& destination_value) {
  Route4BlockEntryPublicationRecord result{
      .status = Route4PublicationAvailabilityStatus::Unavailable,
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
    result.status = Route4PublicationAvailabilityStatus::MissingBlock;
    return result;
  }
  if (destination_value.kind != Value::Kind::Named ||
      destination_value.name.empty()) {
    result.status = Route4PublicationAvailabilityStatus::MissingValue;
    return result;
  }
  const Route4BlockEntryPublicationRecord* type_mismatch = nullptr;
  for (const auto& candidate : index.block_entry_records) {
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

namespace {

[[nodiscard]] std::optional<Route1ImmediateIntegerConstant>
route1_evaluate_same_block_integer_constant_impl(
    Route1SameBlockProducerQuery query,
    const Value& value,
    unsigned depth) {
  const auto immediate = route1_immediate_integer_constant(value, depth);
  if (immediate) {
    return immediate;
  }
  if (!query ||
      depth > 4U ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto producer = route1_find_same_block_scalar_producer(query, value);
  if (!producer.has_value() ||
      producer->record == nullptr ||
      producer->record->kind != Route1ProducerKind::Binary ||
      producer->instruction == nullptr) {
    return std::nullopt;
  }
  const auto* binary = std::get_if<BinaryInst>(producer->instruction);
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto nested_query = Route1SameBlockProducerQuery{
      .index = query.index,
      .before_instruction_index = producer->instruction_index,
  };
  const auto lhs =
      route1_evaluate_same_block_integer_constant_impl(nested_query, binary->lhs, depth + 1);
  const auto rhs =
      route1_evaluate_same_block_integer_constant_impl(nested_query, binary->rhs, depth + 1);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  const auto lhs_value = lhs->value;
  const auto rhs_value = rhs->value;
  switch (binary->opcode) {
    case BinaryOpcode::Add:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) +
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Sub:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) -
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Mul:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) *
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::And:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) &
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Or:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) |
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Xor:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) ^
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Shl:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) <<
              static_cast<unsigned>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::LShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) >>
              static_cast<unsigned>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::AShr:
      if (rhs_value < 0 || rhs_value >= 64) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value >> static_cast<unsigned>(rhs_value),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::SDiv:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value / rhs_value,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::UDiv:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) /
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::SRem:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value % rhs_value,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::URem:
      if (rhs_value == 0) {
        return std::nullopt;
      }
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::int64_t>(
              static_cast<std::uint64_t>(lhs_value) %
              static_cast<std::uint64_t>(rhs_value)),
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Eq:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value == rhs_value ? 1 : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Ne:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value != rhs_value ? 1 : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Slt:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value < rhs_value ? 1 : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Sle:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value <= rhs_value ? 1 : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Sgt:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value > rhs_value ? 1 : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Sge:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = lhs_value >= rhs_value ? 1 : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Ult:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::uint64_t>(lhs_value) <
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Ule:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::uint64_t>(lhs_value) <=
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Ugt:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::uint64_t>(lhs_value) >
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .type = binary->result.type,
          .depth = depth};
    case BinaryOpcode::Uge:
      return Route1ImmediateIntegerConstant{
          .available = true,
          .value = static_cast<std::uint64_t>(lhs_value) >=
                           static_cast<std::uint64_t>(rhs_value)
                       ? 1
                       : 0,
          .type = binary->result.type,
          .depth = depth};
  }
  return std::nullopt;
}

}  // namespace

std::optional<Route1ImmediateIntegerConstant>
route1_evaluate_same_block_integer_constant(
    Route1SameBlockProducerQuery query,
    const Value& value,
    unsigned depth) {
  return route1_evaluate_same_block_integer_constant_impl(query, value, depth);
}

std::optional<Route1ImmediateIntegerConstant>
route1_evaluate_same_block_integer_constant(
    Route1SameBlockProducerQuery query,
    const Value& value) {
  return route1_evaluate_same_block_integer_constant_impl(query, value, 0U);
}

Value Value::immediate_i1(bool value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I1;
  result.immediate = value ? 1 : 0;
  result.immediate_bits = value ? 1u : 0u;
  return result;
}

Value Value::immediate_i8(std::int8_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I8;
  result.immediate = value;
  return result;
}

Value Value::immediate_i16(std::int16_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I16;
  result.immediate = value;
  return result;
}

Value Value::immediate_i32(std::int32_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I32;
  result.immediate = value;
  return result;
}

Value Value::immediate_i64(std::int64_t value) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::I64;
  result.immediate = value;
  return result;
}

Value Value::immediate_f32_bits(std::uint32_t bits) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::F32;
  result.immediate = static_cast<std::int64_t>(bits);
  result.immediate_bits = bits;
  return result;
}

Value Value::immediate_f64_bits(std::uint64_t bits) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::F64;
  result.immediate = static_cast<std::int64_t>(bits);
  result.immediate_bits = bits;
  return result;
}

Value Value::immediate_f128_bits(std::uint64_t low_bits, std::uint64_t high_bits) {
  Value result;
  result.kind = Kind::Immediate;
  result.type = TypeKind::F128;
  result.immediate = static_cast<std::int64_t>(low_bits);
  result.immediate_bits = low_bits;
  result.f128_payload = F128Payload{
      .low_bits = low_bits,
      .high_bits = high_bits,
  };
  return result;
}

Value Value::named(TypeKind type, std::string value_name) {
  Value result;
  result.kind = Kind::Named;
  result.type = type;
  result.name = std::move(value_name);
  return result;
}

Value Value::named_symbol_pointer(std::string value_name, LinkNameId link_name_id) {
  Value result = named(TypeKind::Ptr, std::move(value_name));
  result.pointer_symbol_link_name_id = link_name_id;
  return result;
}

const StructuredTypeDeclSpelling* StructuredTypeSpellingContext::find_struct_decl(
    std::string_view name) const {
  for (const auto& declaration : declarations) {
    if (declaration.name == name) {
      return &declaration;
    }
  }
  return nullptr;
}

std::string render_type(TypeKind type) {
  switch (type) {
    case TypeKind::Void:
      return "void";
    case TypeKind::I1:
      return "i1";
    case TypeKind::I8:
      return "i8";
    case TypeKind::I16:
      return "i16";
    case TypeKind::I32:
      return "i32";
    case TypeKind::I64:
      return "i64";
    case TypeKind::Ptr:
      return "ptr";
    case TypeKind::F32:
      return "float";
    case TypeKind::F64:
      return "double";
    case TypeKind::F128:
      return "f128";
  }
  return "<unknown>";
}

std::string render_binary_opcode(BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Add:
      return "add";
    case BinaryOpcode::Sub:
      return "sub";
    case BinaryOpcode::Mul:
      return "mul";
    case BinaryOpcode::And:
      return "and";
    case BinaryOpcode::Or:
      return "or";
    case BinaryOpcode::Xor:
      return "xor";
    case BinaryOpcode::Shl:
      return "shl";
    case BinaryOpcode::LShr:
      return "lshr";
    case BinaryOpcode::AShr:
      return "ashr";
    case BinaryOpcode::SDiv:
      return "sdiv";
    case BinaryOpcode::UDiv:
      return "udiv";
    case BinaryOpcode::SRem:
      return "srem";
    case BinaryOpcode::URem:
      return "urem";
    case BinaryOpcode::Eq:
      return "eq";
    case BinaryOpcode::Ne:
      return "ne";
    case BinaryOpcode::Slt:
      return "slt";
    case BinaryOpcode::Sle:
      return "sle";
    case BinaryOpcode::Sgt:
      return "sgt";
    case BinaryOpcode::Sge:
      return "sge";
    case BinaryOpcode::Ult:
      return "ult";
    case BinaryOpcode::Ule:
      return "ule";
    case BinaryOpcode::Ugt:
      return "ugt";
    case BinaryOpcode::Uge:
      return "uge";
  }
  return "<unknown>";
}

std::string render_cast_opcode(CastOpcode opcode) {
  switch (opcode) {
    case CastOpcode::SExt:
      return "sext";
    case CastOpcode::ZExt:
      return "zext";
    case CastOpcode::Trunc:
      return "trunc";
    case CastOpcode::FPTrunc:
      return "fptrunc";
    case CastOpcode::FPExt:
      return "fpext";
    case CastOpcode::FPToSI:
      return "fptosi";
    case CastOpcode::FPToUI:
      return "fptoui";
    case CastOpcode::SIToFP:
      return "sitofp";
    case CastOpcode::UIToFP:
      return "uitofp";
    case CastOpcode::PtrToInt:
      return "ptrtoint";
    case CastOpcode::IntToPtr:
      return "inttoptr";
    case CastOpcode::Bitcast:
      return "bitcast";
  }
  return "<unknown>";
}

bool call_argument_binary_source_producer_opcode_is_materializable(
    BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Add:
    case BinaryOpcode::Sub:
    case BinaryOpcode::And:
    case BinaryOpcode::Or:
    case BinaryOpcode::Xor:
    case BinaryOpcode::Mul:
    case BinaryOpcode::SDiv:
    case BinaryOpcode::SRem:
      return true;
    case BinaryOpcode::UDiv:
    case BinaryOpcode::URem:
    case BinaryOpcode::Shl:
    case BinaryOpcode::LShr:
    case BinaryOpcode::AShr:
    case BinaryOpcode::Eq:
    case BinaryOpcode::Ne:
    case BinaryOpcode::Slt:
    case BinaryOpcode::Sle:
    case BinaryOpcode::Sgt:
    case BinaryOpcode::Sge:
    case BinaryOpcode::Ult:
    case BinaryOpcode::Ule:
    case BinaryOpcode::Ugt:
    case BinaryOpcode::Uge:
      return false;
  }
  return false;
}

const CallArgumentSourceRelationship* find_call_argument_source_relationship(
    const CallInst& call,
    std::size_t arg_index) {
  if (arg_index >= call.args.size()) {
    return nullptr;
  }

  const CallArgumentSourceRelationship* result = nullptr;
  for (const auto& relationship : call.arg_sources) {
    if (relationship.arg_index != arg_index) {
      continue;
    }
    if (result != nullptr) {
      return nullptr;
    }
    result = &relationship;
  }
  return result;
}

const CallInst* indexed_call_inst(const Block& block,
                                  const CallInst& call,
                                  std::size_t call_instruction_index) {
  if (call_instruction_index >= block.insts.size()) {
    return nullptr;
  }
  const auto* block_call =
      std::get_if<CallInst>(&block.insts[call_instruction_index]);
  return block_call == &call ? block_call : nullptr;
}

namespace {

CallArgumentSourceProducerMaterialization
raw_call_argument_source_producer_materialization(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  if (call_instruction_index >= block.insts.size() ||
      arg_index >= call.args.size() ||
      find_call_argument_source_relationship(call, arg_index) == nullptr) {
    return {};
  }
  if (indexed_call_inst(block, call, call_instruction_index) == nullptr) {
    return {};
  }

  const auto& source_value = call.args[arg_index];
  if (source_value.kind != Value::Kind::Named || source_value.name.empty()) {
    return {};
  }
  for (std::size_t inst_index = call_instruction_index; inst_index-- > 0;) {
    const auto& inst = block.insts[inst_index];
    if (const auto* load_local = std::get_if<LoadLocalInst>(&inst);
        load_local != nullptr &&
        load_local->result.kind == Value::Kind::Named &&
        load_local->result.name == source_value.name &&
        load_local->result.type == source_value.type) {
      return CallArgumentSourceProducerMaterialization{
          .available = true,
          .arg_index = arg_index,
          .producer_kind = CallArgumentSourceProducerKind::LoadLocal,
          .producer_instruction = &inst,
          .producer_instruction_index = inst_index,
          .produced_value = &load_local->result,
          .materializable = true,
      };
    }
    if (const auto* binary = std::get_if<BinaryInst>(&inst);
        binary != nullptr &&
        binary->result.kind == Value::Kind::Named &&
        binary->result.name == source_value.name &&
        binary->result.type == source_value.type) {
      return CallArgumentSourceProducerMaterialization{
          .available = true,
          .arg_index = arg_index,
          .producer_kind = CallArgumentSourceProducerKind::Binary,
          .producer_instruction = &inst,
          .producer_instruction_index = inst_index,
          .produced_value = &binary->result,
          .materializable =
              call_argument_binary_source_producer_opcode_is_materializable(
                  binary->opcode),
      };
    }
  }
  return {};
}

[[nodiscard]] CallArgumentSourceProducerKind
route6_public_call_argument_source_producer_kind(Route1ProducerKind kind) {
  switch (kind) {
    case Route1ProducerKind::LoadLocal:
      return CallArgumentSourceProducerKind::LoadLocal;
    case Route1ProducerKind::Binary:
      return CallArgumentSourceProducerKind::Binary;
    case Route1ProducerKind::Unknown:
    case Route1ProducerKind::Immediate:
    case Route1ProducerKind::LoadGlobal:
    case Route1ProducerKind::Cast:
    case Route1ProducerKind::SelectMaterialization:
      return CallArgumentSourceProducerKind::Unknown;
  }
  return CallArgumentSourceProducerKind::Unknown;
}

}  // namespace

CallArgumentSourceProducerMaterialization
find_call_argument_source_producer_materialization(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  const auto record = route6_call_argument_source_producer_record(
      block, call, call_instruction_index, arg_index);
  if (!record ||
      !record.producer ||
      !record.producer.producer_instruction ||
      record.producer.source_value.value == nullptr) {
    return {};
  }
  return CallArgumentSourceProducerMaterialization{
      .available = true,
      .arg_index = record.argument_source.arg_index,
      .producer_kind =
          route6_public_call_argument_source_producer_kind(record.producer.kind),
      .producer_instruction =
          record.producer.producer_instruction.instruction,
      .producer_instruction_index =
          record.producer.producer_instruction.instruction_index,
      .produced_value = record.producer.source_value.value,
      .materializable =
          record.materialization.scalar_materialization_available,
  };
}

namespace {

[[nodiscard]] const CallInst* route6_indexed_call_inst(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index) {
  return indexed_call_inst(block, call, call_instruction_index);
}

[[nodiscard]] std::size_t route6_call_argument_relationship_count(
    const CallInst& call,
    std::size_t arg_index) {
  std::size_t count = 0;
  for (const auto& relationship : call.arg_sources) {
    if (relationship.arg_index == arg_index) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] Route6CallUseSourceKind route6_source_kind(
    CallArgumentSourceEncodingKind encoding) {
  switch (encoding) {
    case CallArgumentSourceEncodingKind::Immediate:
      return Route6CallUseSourceKind::Immediate;
    case CallArgumentSourceEncodingKind::ComputedAddress:
    case CallArgumentSourceEncodingKind::SymbolAddress:
      return Route6CallUseSourceKind::BaseValue;
    case CallArgumentSourceEncodingKind::Register:
    case CallArgumentSourceEncodingKind::FrameSlot:
      return Route6CallUseSourceKind::ArgumentValue;
    case CallArgumentSourceEncodingKind::None:
      return Route6CallUseSourceKind::Unknown;
  }
  return Route6CallUseSourceKind::Unknown;
}

[[nodiscard]] Route6CallUseSourceKind route6_source_kind(
    CallArgumentSourceProducerKind kind) {
  switch (kind) {
    case CallArgumentSourceProducerKind::LoadLocal:
      return Route6CallUseSourceKind::LoadLocal;
    case CallArgumentSourceProducerKind::Binary:
      return Route6CallUseSourceKind::Binary;
    case CallArgumentSourceProducerKind::Unknown:
      return Route6CallUseSourceKind::Unknown;
  }
  return Route6CallUseSourceKind::Unknown;
}

[[nodiscard]] bool route6_selection_is_abi_bound(
    const CallArgumentSourceSelection& selection) {
  return selection.kind == CallArgumentSourceSelectionKind::PriorPreservation ||
         selection.kind == CallArgumentSourceSelectionKind::ByvalRegisterLane ||
         selection.source_stack_offset_bytes.has_value() ||
         selection.source_size_bytes.has_value() ||
         selection.source_align_bytes.has_value() ||
         selection.address_materialization_block_label.has_value() ||
         selection.address_materialization_inst_index.has_value() ||
         selection.address_materialization_frame_slot_id.has_value() ||
         selection.address_materialization_byte_offset.has_value();
}

[[nodiscard]] bool route6_values_match(const Value& lhs, const Value& rhs) {
  return lhs.kind == rhs.kind && lhs.name == rhs.name && lhs.type == rhs.type &&
         lhs.immediate == rhs.immediate;
}

[[nodiscard]] bool route6_duplicate_lane_match(const CallInst& call,
                                               const Value& value) {
  if (!call.result.has_value()) {
    return false;
  }
  std::size_t matches = route6_values_match(*call.result, value) ? 1U : 0U;
  for (std::size_t lane_index = 0; lane_index < call.result_lanes.size();
       ++lane_index) {
    const auto& lane = call.result_lanes[lane_index];
    if (!route6_values_match(lane, value)) {
      continue;
    }
    const bool aliases_primary =
        lane_index == 0 && route6_values_match(*call.result, lane);
    if (!aliases_primary) {
      ++matches;
    }
  }
  return matches > 1U;
}

}  // namespace

Route6CallArgumentSourceRecord route6_call_argument_source_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  Route6CallArgumentSourceRecord record{
      .status = Route6CallUseStatus::Unavailable,
      .call = &call,
      .call_instruction_index = call_instruction_index,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .callee = call.callee,
      .callee_link_name_id = call.callee_link_name_id,
      .arg_index = arg_index,
  };
  if (call_instruction_index >= block.insts.size()) {
    record.status = Route6CallUseStatus::MissingCall;
    return record;
  }
  if (route6_indexed_call_inst(block, call, call_instruction_index) == nullptr) {
    record.status = Route6CallUseStatus::WrongCall;
    return record;
  }
  if (arg_index >= call.args.size()) {
    record.status = Route6CallUseStatus::MissingArgument;
    return record;
  }
  record.argument_value = &call.args[arg_index];
  record.source_value = route1_source_value_identity(call.args[arg_index]);

  const auto relationship_count =
      route6_call_argument_relationship_count(call, arg_index);
  if (relationship_count > 1U) {
    record.status = Route6CallUseStatus::DuplicateRelationship;
    return record;
  }
  const auto* relationship =
      find_call_argument_source_relationship(call, arg_index);
  if (relationship == nullptr) {
    record.status = Route6CallUseStatus::MissingSourceRelationship;
    return record;
  }

  record.available = true;
  record.status = Route6CallUseStatus::Available;
  record.source_encoding = relationship->source_encoding;
  record.source_value_id = relationship->source_value_id;
  record.source_value_name =
      relationship->source_value_name.has_value()
          ? std::optional<std::string_view>{*relationship->source_value_name}
          : std::nullopt;
  record.source_base_value_id = relationship->source_base_value_id;
  record.source_base_value_name =
      relationship->source_base_value_name.has_value()
          ? std::optional<std::string_view>{*relationship->source_base_value_name}
          : std::nullopt;
  record.source_pointer_byte_delta = relationship->source_pointer_byte_delta;
  record.source_kind = route6_source_kind(relationship->source_encoding);
  if (relationship->source_selection.has_value()) {
    if (route6_selection_is_abi_bound(*relationship->source_selection)) {
      record.available = false;
      record.status = Route6CallUseStatus::AbiBoundExcluded;
      record.source_kind = Route6CallUseSourceKind::AbiBoundExcluded;
      return record;
    }
    if (record.source_kind == Route6CallUseSourceKind::Unknown) {
      record.source_kind = Route6CallUseSourceKind::PublicationSource;
    }
  }
  if (relationship->direct_global_select_chain_dependency.has_value()) {
    record.source_kind = Route6CallUseSourceKind::DirectGlobalSelectChain;
  }
  return record;
}

Route6CallArgumentSourceProducerRecord
route6_call_argument_source_producer_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  Route6CallArgumentSourceProducerRecord record{
      .status = Route6CallUseStatus::Unavailable,
      .argument_source = route6_call_argument_source_record(
          block, call, call_instruction_index, arg_index),
  };
  if (!record.argument_source) {
    record.status = record.argument_source.status;
    return record;
  }
  const auto materialization =
      raw_call_argument_source_producer_materialization(
          block, call, call_instruction_index, arg_index);
  if (!materialization.available || materialization.producer_instruction == nullptr ||
      materialization.produced_value == nullptr) {
    record.status = Route6CallUseStatus::MissingSourceProducer;
    return record;
  }
  record.available = true;
  record.status = Route6CallUseStatus::Available;
  record.producer =
      route1_producer_record(block, materialization.producer_instruction_index);
  record.materialization = record.producer.materialization;
  if (record.producer) {
    record.materialization.scalar_materialization_available =
        materialization.materializable;
  }
  record.argument_source.source_kind =
      route6_source_kind(materialization.producer_kind);
  return record;
}

Route6CallArgumentDirectGlobalDependencyRecord
route6_call_argument_direct_global_dependency_record(
    Route1SameBlockProducerQuery query,
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  Route6CallArgumentDirectGlobalDependencyRecord record{
      .status = Route6CallUseStatus::Unavailable,
      .argument_source = route6_call_argument_source_record(
          block, call, call_instruction_index, arg_index),
  };
  if (!record.argument_source) {
    record.status = record.argument_source.status;
    return record;
  }
  const auto* relationship =
      find_call_argument_source_relationship(call, arg_index);
  if (relationship == nullptr ||
      !relationship->direct_global_select_chain_dependency.has_value()) {
    record.status = Route6CallUseStatus::MissingDirectGlobal;
    return record;
  }
  const auto& dependency =
      *relationship->direct_global_select_chain_dependency;
  record.source_value_name = dependency.source_value_name;
  if (!call_argument_direct_global_select_chain_dependency_available(
          dependency)) {
    record.status = Route6CallUseStatus::MissingDirectGlobal;
    return record;
  }

  auto route2 = route2_select_chain_value_record(
      query, call.args[arg_index]);
  record.direct_global_dependency = route2.direct_global_dependency;
  if (!record.direct_global_dependency.available) {
    record.direct_global_dependency.available = true;
    record.direct_global_dependency.contains_direct_global_load =
        dependency.contains_direct_global_load;
    record.direct_global_dependency.root_is_select = dependency.root_is_select;
    record.direct_global_dependency.root_instruction_index =
        dependency.root_instruction_index;
  }
  record.available = true;
  record.status = Route6CallUseStatus::Available;
  record.argument_source.source_kind =
      Route6CallUseSourceKind::DirectGlobalSelectChain;
  return record;
}

Route6CallArgumentPublicationSourceRecord
route6_call_argument_publication_source_record(
    Route1SameBlockProducerQuery query,
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  Route6CallArgumentPublicationSourceRecord record{
      .status = Route6CallUseStatus::Unavailable,
      .argument_source = route6_call_argument_source_record(
          block, call, call_instruction_index, arg_index),
  };
  if (!record.argument_source) {
    record.status = record.argument_source.status;
    if (record.status == Route6CallUseStatus::AbiBoundExcluded) {
      record.source_kind = Route6CallUseSourceKind::AbiBoundExcluded;
      record.abi_bound_excluded = true;
    }
    return record;
  }
  const auto routing =
      find_call_argument_publication_source_routing(call, arg_index);
  if (!routing.available) {
    record.status = Route6CallUseStatus::MissingPublicationSource;
    return record;
  }
  record.source_value_id = routing.source_value_id;
  record.source_base_value_id = routing.source_base_value_id;
  record.source_base_value_name =
      routing.source_base_value_name.has_value()
          ? std::optional<std::string_view>{*routing.source_base_value_name}
          : std::nullopt;
  record.source_pointer_byte_delta = routing.source_pointer_byte_delta;
  if (routing.source_selection != nullptr &&
      route6_selection_is_abi_bound(*routing.source_selection)) {
    record.status = Route6CallUseStatus::AbiBoundExcluded;
    record.source_kind = Route6CallUseSourceKind::AbiBoundExcluded;
    record.abi_bound_excluded = true;
    return record;
  }

  const auto producer = route6_call_argument_source_producer_record(
      block, call, call_instruction_index, arg_index);
  if (producer && producer.producer.producer_instruction.instruction != nullptr) {
    const auto& producer_identity = producer.producer.producer_instruction;
    if (producer_identity.kind == Route1ProducerKind::LoadLocal ||
        producer_identity.kind == Route1ProducerKind::LoadGlobal) {
      record.memory_source =
          route3_memory_access_record(block, producer_identity.instruction_index);
      if (record.memory_source) {
        record.available = true;
        record.status = Route6CallUseStatus::Available;
        record.source_kind = Route6CallUseSourceKind::MemorySource;
        return record;
      }
      record.status = Route6CallUseStatus::MissingMemorySource;
      return record;
    }
  }

  record.current_block_publication_source =
      route4_current_block_publication_record(
          query, call.args[arg_index]);
  if (record.current_block_publication_source) {
    record.available = true;
    record.status = Route6CallUseStatus::Available;
    record.source_kind = Route6CallUseSourceKind::PublicationSource;
    return record;
  }
  if (routing.direct_global_select_chain_dependency != nullptr ||
      routing.source_encoding != CallArgumentSourceEncodingKind::None ||
      routing.source_selection != nullptr) {
    record.available = true;
    record.status = Route6CallUseStatus::Available;
    record.source_kind =
        routing.direct_global_select_chain_dependency != nullptr
            ? Route6CallUseSourceKind::DirectGlobalSelectChain
            : route6_source_kind(routing.source_encoding);
    return record;
  }
  record.status = Route6CallUseStatus::MissingPublicationSource;
  return record;
}

Route6CallResultSourceRecord route6_call_result_source_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index) {
  Route6CallResultSourceRecord record{
      .status = Route6CallUseStatus::Unavailable,
      .call = &call,
      .call_instruction_index = call_instruction_index,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .callee = call.callee,
      .callee_link_name_id = call.callee_link_name_id,
  };
  if (call_instruction_index >= block.insts.size()) {
    record.status = Route6CallUseStatus::MissingCall;
    return record;
  }
  if (route6_indexed_call_inst(block, call, call_instruction_index) == nullptr) {
    record.status = Route6CallUseStatus::WrongCall;
    return record;
  }
  const auto identity =
      find_call_result_source_identity(block, call, call_instruction_index);
  if (!identity.available || identity.result_value == nullptr) {
    record.status = Route6CallUseStatus::MissingResult;
    return record;
  }
  record.available = true;
  record.status = Route6CallUseStatus::Available;
  record.result_value = identity.result_value;
  record.result_identity = route1_source_value_identity(*identity.result_value);
  return record;
}

Route6CallResultLaneSourceRecord route6_call_result_lane_source_record(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    const Value& value) {
  Route6CallResultLaneSourceRecord record{
      .status = Route6CallUseStatus::Unavailable,
      .result_source =
          route6_call_result_source_record(block, call, call_instruction_index),
  };
  if (!record.result_source) {
    record.status = record.result_source.status;
    return record;
  }
  const auto identity = find_call_result_lane_source_identity(
      block, call, call_instruction_index, value);
  if (!identity.available || identity.lane_value == nullptr) {
    record.status = route6_duplicate_lane_match(call, value)
                        ? Route6CallUseStatus::DuplicateResultLane
                        : Route6CallUseStatus::NoMatch;
    record.lane_identity = route1_source_value_identity(value);
    return record;
  }
  record.available = true;
  record.status = Route6CallUseStatus::Available;
  record.lane_index = identity.lane_index;
  record.lane_value = identity.lane_value;
  record.lane_identity = route1_source_value_identity(*identity.lane_value);
  record.aliases_primary_result = identity.aliases_primary_result;
  return record;
}

namespace {

[[nodiscard]] bool route6_block_matches(std::string_view record_label,
                                        BlockLabelId record_label_id,
                                        const Block& block) {
  if (record_label_id != kInvalidBlockLabel &&
      block.label_id != kInvalidBlockLabel) {
    return record_label_id == block.label_id;
  }
  return !record_label.empty() && record_label == block.label;
}

[[nodiscard]] bool route6_call_key_matches(std::string_view record_block_label,
                                           BlockLabelId record_block_label_id,
                                           std::size_t record_call_index,
                                           std::string_view record_callee,
                                           const Block& block,
                                           std::size_t call_instruction_index,
                                           std::string_view callee) {
  return route6_block_matches(record_block_label, record_block_label_id, block) &&
         record_call_index == call_instruction_index &&
         record_callee == callee;
}

[[nodiscard]] Route6CallUseStatus route6_missing_call_status(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee) {
  bool block_seen = false;
  bool instruction_seen = false;
  for (const auto& record : index.argument_source_records) {
    if (!route6_block_matches(record.block_label, record.block_label_id, block)) {
      continue;
    }
    block_seen = true;
    if (record.call_instruction_index == call_instruction_index) {
      instruction_seen = true;
      if (record.callee != callee) {
        return Route6CallUseStatus::WrongCall;
      }
    }
  }
  for (const auto& record : index.result_records) {
    if (!route6_block_matches(record.block_label, record.block_label_id, block)) {
      continue;
    }
    block_seen = true;
    if (record.call_instruction_index == call_instruction_index) {
      instruction_seen = true;
      if (record.callee != callee) {
        return Route6CallUseStatus::WrongCall;
      }
    }
  }
  return block_seen && instruction_seen ? Route6CallUseStatus::NoMatch
                                        : Route6CallUseStatus::MissingCall;
}

}  // namespace

Route6CallUseSourceIndex route6_build_call_use_source_index(
    const Function& function) {
  Route6CallUseSourceIndex index{.function = &function};
  for (const auto& block : function.blocks) {
    const auto producer_index = route1_build_producer_index(block);
    for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
         ++instruction_index) {
      const auto* call = std::get_if<CallInst>(&block.insts[instruction_index]);
      if (call == nullptr) {
        continue;
      }
      const auto query = Route1SameBlockProducerQuery{
          .index = &producer_index,
          .before_instruction_index = instruction_index,
      };
      for (std::size_t arg_index = 0; arg_index < call->args.size();
           ++arg_index) {
        index.argument_source_records.push_back(
            route6_call_argument_source_record(block, *call, instruction_index,
                                               arg_index));
        index.argument_producer_records.push_back(
            route6_call_argument_source_producer_record(
                block, *call, instruction_index, arg_index));
        index.direct_global_records.push_back(
            route6_call_argument_direct_global_dependency_record(
                query, block, *call, instruction_index, arg_index));
        index.publication_source_records.push_back(
            route6_call_argument_publication_source_record(
                query, block, *call, instruction_index, arg_index));
      }
      index.result_records.push_back(
          route6_call_result_source_record(block, *call, instruction_index));
      if (call->result.has_value()) {
        index.result_lane_records.push_back(
            route6_call_result_lane_source_record(
                block, *call, instruction_index, *call->result));
      }
      for (const auto& lane : call->result_lanes) {
        index.result_lane_records.push_back(
            route6_call_result_lane_source_record(
                block, *call, instruction_index, lane));
      }
    }
  }
  return index;
}

Route6CallArgumentSourceRecord route6_find_call_argument_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index) {
  for (const auto& record : index.argument_source_records) {
    if (route6_call_key_matches(record.block_label, record.block_label_id,
                                record.call_instruction_index, record.callee,
                                block, call_instruction_index, callee) &&
        record.arg_index == arg_index) {
      return record;
    }
  }
  return Route6CallArgumentSourceRecord{
      .status = route6_missing_call_status(
          index, block, call_instruction_index, callee),
      .call_instruction_index = call_instruction_index,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .callee = callee,
      .arg_index = arg_index,
  };
}

Route6CallArgumentSourceProducerRecord
route6_find_call_argument_source_producer(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index) {
  for (const auto& record : index.argument_producer_records) {
    const auto& source = record.argument_source;
    if (route6_call_key_matches(source.block_label, source.block_label_id,
                                source.call_instruction_index, source.callee,
                                block, call_instruction_index, callee) &&
        source.arg_index == arg_index) {
      return record;
    }
  }
  return Route6CallArgumentSourceProducerRecord{
      .status = route6_find_call_argument_source(
                    index, block, call_instruction_index, callee, arg_index)
                    .status,
  };
}

Route6CallArgumentDirectGlobalDependencyRecord
route6_find_call_argument_direct_global_dependency(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index) {
  for (const auto& record : index.direct_global_records) {
    const auto& source = record.argument_source;
    if (route6_call_key_matches(source.block_label, source.block_label_id,
                                source.call_instruction_index, source.callee,
                                block, call_instruction_index, callee) &&
        source.arg_index == arg_index) {
      return record;
    }
  }
  return Route6CallArgumentDirectGlobalDependencyRecord{
      .status = route6_find_call_argument_source(
                    index, block, call_instruction_index, callee, arg_index)
                    .status,
  };
}

Route6CallArgumentPublicationSourceRecord
route6_find_call_argument_publication_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    std::size_t arg_index) {
  for (const auto& record : index.publication_source_records) {
    const auto& source = record.argument_source;
    if (route6_call_key_matches(source.block_label, source.block_label_id,
                                source.call_instruction_index, source.callee,
                                block, call_instruction_index, callee) &&
        source.arg_index == arg_index) {
      return record;
    }
  }
  return Route6CallArgumentPublicationSourceRecord{
      .status = route6_find_call_argument_source(
                    index, block, call_instruction_index, callee, arg_index)
                    .status,
  };
}

Route6CallResultSourceRecord route6_find_call_result_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    const Value& result_value) {
  for (const auto& record : index.result_records) {
    if (route6_call_key_matches(record.block_label, record.block_label_id,
                                record.call_instruction_index, record.callee,
                                block, call_instruction_index, callee)) {
      if (record.result_value != nullptr &&
          route6_values_match(*record.result_value, result_value)) {
        return record;
      }
      auto no_match = record;
      no_match.available = false;
      no_match.status = Route6CallUseStatus::NoMatch;
      return no_match;
    }
  }
  return Route6CallResultSourceRecord{
      .status = route6_missing_call_status(
          index, block, call_instruction_index, callee),
      .call_instruction_index = call_instruction_index,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .callee = callee,
  };
}

Route6CallResultLaneSourceRecord route6_find_call_result_lane_source(
    const Route6CallUseSourceIndex& index,
    const Block& block,
    std::size_t call_instruction_index,
    std::string_view callee,
    const Value& lane_value) {
  for (const auto& record : index.result_lane_records) {
    const auto& source = record.result_source;
    if (route6_call_key_matches(source.block_label, source.block_label_id,
                                source.call_instruction_index, source.callee,
                                block, call_instruction_index, callee)) {
      if (record.status == Route6CallUseStatus::DuplicateResultLane &&
          record.lane_value == nullptr &&
          record.lane_identity.name == lane_value.name &&
          record.lane_identity.type == lane_value.type &&
          record.lane_identity.value_kind == lane_value.kind) {
        return record;
      }
      if (record.lane_value != nullptr &&
          route6_values_match(*record.lane_value, lane_value)) {
        return record;
      }
    }
  }
  return Route6CallResultLaneSourceRecord{
      .status = route6_missing_call_status(
          index, block, call_instruction_index, callee),
      .result_source =
          Route6CallResultSourceRecord{.call_instruction_index =
                                           call_instruction_index,
                                       .block_label = block.label,
                                       .block_label_id = block.label_id,
                                       .callee = callee},
  };
}

std::optional<ComparisonOperandProducer> find_comparison_operand_producer(
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  if (value.kind == Value::Kind::Immediate) {
    return ComparisonOperandProducer{
        .available = true,
        .producer_kind = ComparisonProducerKind::Immediate,
        .integer_constant = value.immediate,
    };
  }

  const auto producer =
      find_unique_comparison_producer(block, value, before_instruction_index);
  if (producer.inst == nullptr || producer.ambiguous) {
    return std::nullopt;
  }

  ComparisonOperandProducer result{
      .available = true,
      .producer_kind = comparison_producer_kind_for_inst(*producer.inst),
      .producer_instruction = producer.inst,
      .producer_instruction_index = producer.instruction_index,
      .produced_value = producer.produced_value,
      .integer_constant = evaluate_comparison_integer_constant(
          block, value, before_instruction_index),
  };
  if (result.producer_kind == ComparisonProducerKind::Unknown) {
    return std::nullopt;
  }
  return result;
}

Route7ComparisonOperandRecord route7_comparison_operand_record(
    const Block* block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role) {
  Route7ComparisonOperandRecord record{
      .status = Route7ComparisonStatus::Unavailable,
      .role = role,
      .before_instruction_index = before_instruction_index,
      .value = route1_source_value_identity(value),
  };
  if (block == nullptr) {
    record.status = Route7ComparisonStatus::MissingBlock;
    return record;
  }
  record.block_label = block->label;
  record.block_label_id = block->label_id;
  if (value.kind == Value::Kind::Immediate) {
    record.available = true;
    record.status = Route7ComparisonStatus::Available;
    record.producer_kind = ComparisonProducerKind::Immediate;
    record.integer_constant = value.immediate;
    return record;
  }
  if (value.kind != Value::Kind::Named || value.name.empty()) {
    record.status = Route7ComparisonStatus::NoMatch;
    return record;
  }
  const auto producer =
      find_unique_comparison_producer(*block, value, before_instruction_index);
  if (producer.ambiguous) {
    record.status = Route7ComparisonStatus::DuplicateProducer;
    return record;
  }
  if (producer.inst == nullptr || producer.produced_value == nullptr) {
    record.status = Route7ComparisonStatus::MissingOperandProducer;
    return record;
  }
  const auto kind = comparison_producer_kind_for_inst(*producer.inst);
  if (kind == ComparisonProducerKind::Unknown) {
    record.status = Route7ComparisonStatus::AbsentProvenance;
    return record;
  }
  record.available = true;
  record.status = Route7ComparisonStatus::Available;
  record.producer_kind = kind;
  record.producer_instruction = producer.inst;
  record.producer_instruction_index = producer.instruction_index;
  record.produced_value = route1_source_value_identity(*producer.produced_value);
  record.integer_constant =
      evaluate_comparison_integer_constant(*block, value, before_instruction_index);
  return record;
}

Route7ComparisonInstructionRecord route7_comparison_instruction_record(
    const Block* block,
    std::size_t instruction_index) {
  Route7ComparisonInstructionRecord record{
      .status = Route7ComparisonStatus::Unavailable,
      .instruction_index = instruction_index,
  };
  if (block == nullptr) {
    record.status = Route7ComparisonStatus::MissingBlock;
    return record;
  }
  record.block_label = block->label;
  record.block_label_id = block->label_id;
  if (instruction_index >= block->insts.size()) {
    record.status = Route7ComparisonStatus::MissingInstruction;
    return record;
  }
  record.instruction = &block->insts[instruction_index];
  const auto* binary = std::get_if<BinaryInst>(record.instruction);
  if (binary == nullptr) {
    record.status = Route7ComparisonStatus::WrongInstruction;
    return record;
  }
  record.binary = binary;
  record.condition_value = route1_source_value_identity(binary->result);
  record.predicate = binary->opcode;
  record.compare_type = binary_operand_type(*binary);
  record.lhs_value = route1_source_value_identity(binary->lhs);
  record.rhs_value = route1_source_value_identity(binary->rhs);
  if (!is_comparison_binary_opcode(binary->opcode)) {
    record.status = Route7ComparisonStatus::NonComparison;
    return record;
  }
  record.lhs = route7_comparison_operand_record(
      block, binary->lhs, instruction_index, Route7ComparisonOperandRole::Lhs);
  record.rhs = route7_comparison_operand_record(
      block, binary->rhs, instruction_index, Route7ComparisonOperandRole::Rhs);
  record.available = true;
  record.status = Route7ComparisonStatus::Available;
  return record;
}

Route7BranchConditionRecord route7_branch_condition_record(const Block* block) {
  Route7BranchConditionRecord record{
      .status = Route7ComparisonStatus::Unavailable,
  };
  if (block == nullptr) {
    record.status = Route7ComparisonStatus::MissingBlock;
    return record;
  }
  record.block_label = block->label;
  record.block_label_id = block->label_id;
  if (block->terminator.kind != TerminatorKind::CondBranch) {
    record.status = Route7ComparisonStatus::AbsentProvenance;
    return record;
  }
  record.condition_value =
      route1_source_value_identity(block->terminator.condition);
  record.true_label = block->terminator.true_label;
  record.true_label_id = block->terminator.true_label_id;
  record.false_label = block->terminator.false_label;
  record.false_label_id = block->terminator.false_label_id;
  if (block->terminator.condition.kind != Value::Kind::Named ||
      block->terminator.condition.name.empty()) {
    record.status = Route7ComparisonStatus::MissingConditionValue;
    return record;
  }
  const auto producer = find_unique_comparison_producer(
      *block, block->terminator.condition, block->insts.size());
  if (producer.ambiguous) {
    record.status = Route7ComparisonStatus::DuplicateProducer;
    return record;
  }
  if (producer.inst == nullptr) {
    record.status = Route7ComparisonStatus::AbsentProvenance;
    return record;
  }
  const auto* binary = std::get_if<BinaryInst>(producer.inst);
  if (binary == nullptr || !is_comparison_binary_opcode(binary->opcode)) {
    record.status = Route7ComparisonStatus::NonComparison;
    return record;
  }
  record.comparison =
      route7_comparison_instruction_record(block, producer.instruction_index);
  if (!record.comparison) {
    record.status = record.comparison.status;
    return record;
  }
  record.available = true;
  record.status = Route7ComparisonStatus::Available;
  record.kind = Route7BranchConditionKind::FusedCompare;
  return record;
}

namespace {

[[nodiscard]] bool route7_block_matches(std::string_view record_label,
                                        BlockLabelId record_label_id,
                                        const Block& block) {
  if (record_label_id != kInvalidBlockLabel ||
      block.label_id != kInvalidBlockLabel) {
    return record_label_id == block.label_id;
  }
  return record_label == block.label;
}

[[nodiscard]] bool route7_value_matches(
    const Route1SourceValueIdentity& identity,
    const Value& value) {
  if (identity.type != value.type || identity.value_kind != value.kind) {
    return false;
  }
  if (value.kind == Value::Kind::Named) {
    return identity.name == value.name;
  }
  if (value.kind == Value::Kind::Immediate) {
    return identity.integer_constant.has_value() &&
           *identity.integer_constant == value.immediate;
  }
  return identity.value != nullptr && *identity.value == value;
}

[[nodiscard]] Route7ComparisonStatus route7_missing_block_status(
    const Route7ComparisonConditionIndex& index,
    const Block& block) {
  if (!index) {
    return Route7ComparisonStatus::MissingBlock;
  }
  for (const auto& record : index.comparison_records) {
    if (route7_block_matches(record.block_label, record.block_label_id,
                             block)) {
      return Route7ComparisonStatus::NoMatch;
    }
  }
  for (const auto& record : index.branch_condition_records) {
    if (route7_block_matches(record.block_label, record.block_label_id,
                             block)) {
      return Route7ComparisonStatus::NoMatch;
    }
  }
  return Route7ComparisonStatus::MissingBlock;
}

[[nodiscard]] RouteIndexOwnerScope route7_index_owner_scope(
    const Route7ComparisonConditionIndex& index) {
  if (index.block != nullptr) {
    return RouteIndexOwnerScope::Block;
  }
  if (index.function != nullptr) {
    return RouteIndexOwnerScope::Function;
  }
  return RouteIndexOwnerScope::None;
}

[[nodiscard]] RouteIndexValidationStatus route7_reference_status(
    Route7ComparisonStatus status) {
  switch (status) {
    case Route7ComparisonStatus::Available:
      return RouteIndexValidationStatus::Valid;
    case Route7ComparisonStatus::MissingBlock:
    case Route7ComparisonStatus::MissingInstruction:
    case Route7ComparisonStatus::MissingConditionValue:
    case Route7ComparisonStatus::MissingOperandProducer:
      return RouteIndexValidationStatus::MissingRecord;
    case Route7ComparisonStatus::WrongInstruction:
    case Route7ComparisonStatus::NonComparison:
      return RouteIndexValidationStatus::WrongRecordCategory;
    case Route7ComparisonStatus::DuplicateProducer:
      return RouteIndexValidationStatus::DuplicateReference;
    case Route7ComparisonStatus::AbsentProvenance:
      return RouteIndexValidationStatus::AbsentProvenance;
    case Route7ComparisonStatus::NoMatch:
      return RouteIndexValidationStatus::NoMatch;
    case Route7ComparisonStatus::Unavailable:
      return RouteIndexValidationStatus::Unavailable;
  }
  return RouteIndexValidationStatus::Unavailable;
}

[[nodiscard]] RouteIndexRecordReference route7_reference_key(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    RouteIndexRecordCategory category,
    RouteIndexRelationshipKind relationship,
    std::size_t instruction_index,
    std::size_t before_instruction_index,
    const Route1SourceValueIdentity& value,
    Route7ComparisonOperandRole role,
    std::size_t record_index) {
  return RouteIndexRecordReference{
      .route = RouteIndexRoute::Route7ComparisonCondition,
      .owner_scope = route7_index_owner_scope(index),
      .record_category = category,
      .relationship = relationship,
      .function = index.function,
      .block = index.block,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .instruction_index = instruction_index,
      .before_instruction_index = before_instruction_index,
      .value = value,
      .operand_role = role,
      .record_index = record_index,
  };
}

[[nodiscard]] bool route7_index_owner_is_stale(
    const Route7ComparisonConditionIndex& index,
    const Block& block) {
  return index.block != nullptr && index.block != &block;
}

[[nodiscard]] bool route7_instruction_record_points_into_block(
    const Route7ComparisonInstructionRecord& record,
    const Block& block) {
  return record.instruction_index < block.insts.size() &&
         record.instruction == &block.insts[record.instruction_index];
}

[[nodiscard]] bool route7_operand_record_points_into_block(
    const Route7ComparisonOperandRecord& record,
    const Block& block) {
  if (record.producer_instruction == nullptr) {
    if (record.status == Route7ComparisonStatus::Available &&
        record.producer_kind == ComparisonProducerKind::Immediate) {
      return true;
    }
    return record.status != Route7ComparisonStatus::Available;
  }
  return record.producer_instruction_index < block.insts.size() &&
         record.producer_instruction ==
             &block.insts[record.producer_instruction_index];
}

[[nodiscard]] Route7IndexReferenceValidation route7_missing_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    RouteIndexRecordCategory category,
    RouteIndexRelationshipKind relationship,
    std::size_t instruction_index,
    std::size_t before_instruction_index,
    const Route1SourceValueIdentity& value,
    Route7ComparisonOperandRole role,
    RouteIndexValidationStatus status,
    Route7ComparisonStatus route_status) {
  return Route7IndexReferenceValidation{
      .valid = false,
      .status = status,
      .route_status = route_status,
      .reference = route7_reference_key(index,
                                        block,
                                        category,
                                        relationship,
                                        instruction_index,
                                        before_instruction_index,
                                        value,
                                        role,
                                        0),
  };
}

}  // namespace

Route7ComparisonConditionIndex route7_build_comparison_condition_index(
    const Function& function) {
  Route7ComparisonConditionIndex index{
      .function = &function,
  };
  for (const auto& block : function.blocks) {
    index.comparison_records.reserve(index.comparison_records.size() +
                                     block.insts.size());
    for (std::size_t instruction_index = 0;
         instruction_index < block.insts.size();
         ++instruction_index) {
      auto record = route7_comparison_instruction_record(
          &block, instruction_index);
      index.comparison_records.push_back(record);
      if (record.status == Route7ComparisonStatus::Available ||
          record.status == Route7ComparisonStatus::NonComparison) {
        if (record.lhs.status != Route7ComparisonStatus::Unavailable) {
          index.operand_records.push_back(record.lhs);
        }
        if (record.rhs.status != Route7ComparisonStatus::Unavailable) {
          index.operand_records.push_back(record.rhs);
        }
      }
    }
    const auto branch_record = route7_branch_condition_record(&block);
    if (branch_record.status != Route7ComparisonStatus::Unavailable) {
      index.branch_condition_records.push_back(branch_record);
    }
  }
  return index;
}

Route7ComparisonConditionIndex route7_build_comparison_condition_index(
    const Block& block) {
  Route7ComparisonConditionIndex index{
      .block = &block,
  };
  index.comparison_records.reserve(block.insts.size());
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    auto record = route7_comparison_instruction_record(&block, instruction_index);
    index.comparison_records.push_back(record);
    if (record.status == Route7ComparisonStatus::Available ||
        record.status == Route7ComparisonStatus::NonComparison) {
      if (record.lhs.status != Route7ComparisonStatus::Unavailable) {
        index.operand_records.push_back(record.lhs);
      }
      if (record.rhs.status != Route7ComparisonStatus::Unavailable) {
        index.operand_records.push_back(record.rhs);
      }
    }
  }
  const auto branch_record = route7_branch_condition_record(&block);
  if (branch_record.status != Route7ComparisonStatus::Unavailable) {
    index.branch_condition_records.push_back(branch_record);
  }
  return index;
}

Route7ComparisonInstructionRecord route7_find_comparison_instruction(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    std::size_t instruction_index) {
  for (const auto& record : index.comparison_records) {
    if (route7_block_matches(record.block_label, record.block_label_id,
                             block) &&
        record.instruction_index == instruction_index) {
      return record;
    }
  }
  return Route7ComparisonInstructionRecord{
      .status = route7_missing_block_status(index, block),
      .block_label = block.label,
      .block_label_id = block.label_id,
      .instruction_index = instruction_index,
  };
}

Route7ComparisonOperandRecord route7_find_comparison_operand(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role) {
  for (const auto& record : index.operand_records) {
    if (route7_block_matches(record.block_label, record.block_label_id,
                             block) &&
        record.before_instruction_index == before_instruction_index &&
        record.role == role &&
        route7_value_matches(record.value, value)) {
      return record;
    }
  }
  return Route7ComparisonOperandRecord{
      .status = route7_missing_block_status(index, block),
      .role = role,
      .block_label = block.label,
      .block_label_id = block.label_id,
      .before_instruction_index = before_instruction_index,
      .value = route1_source_value_identity(value),
  };
}

Route7ComparisonInstructionRecord route7_find_materialized_condition(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index) {
  const Route7ComparisonInstructionRecord* match = nullptr;
  for (const auto& record : index.comparison_records) {
    if (!route7_block_matches(record.block_label, record.block_label_id,
                              block) ||
        record.instruction_index >= before_instruction_index ||
        !route7_value_matches(record.condition_value, condition_value)) {
      continue;
    }
    if (match != nullptr) {
      return Route7ComparisonInstructionRecord{
          .status = Route7ComparisonStatus::DuplicateProducer,
          .block_label = block.label,
          .block_label_id = block.label_id,
          .instruction_index = record.instruction_index,
          .condition_value = route1_source_value_identity(condition_value),
      };
    }
    match = &record;
  }
  if (match != nullptr) {
    return *match;
  }
  return Route7ComparisonInstructionRecord{
      .status = route7_missing_block_status(index, block),
      .block_label = block.label,
      .block_label_id = block.label_id,
      .instruction_index = before_instruction_index,
      .condition_value = route1_source_value_identity(condition_value),
  };
}

Route7BranchConditionRecord route7_find_branch_condition(
    const Route7ComparisonConditionIndex& index,
    const Block& block) {
  for (const auto& record : index.branch_condition_records) {
    if (route7_block_matches(record.block_label, record.block_label_id,
                             block)) {
      return record;
    }
  }
  return Route7BranchConditionRecord{
      .status = route7_missing_block_status(index, block),
      .block_label = block.label,
      .block_label_id = block.label_id,
  };
}

Route7IndexReferenceValidation route7_validate_comparison_instruction_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    std::size_t instruction_index) {
  const Route1SourceValueIdentity empty_value{};
  if (route7_index_owner_is_stale(index, block)) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7ComparisonInstruction,
        RouteIndexRelationshipKind::Route7Instruction,
        instruction_index,
        instruction_index,
        empty_value,
        Route7ComparisonOperandRole::None,
        RouteIndexValidationStatus::StaleOwner,
        Route7ComparisonStatus::MissingBlock);
  }

  const Route7ComparisonInstructionRecord* match = nullptr;
  std::size_t match_index = 0;
  for (std::size_t record_index = 0;
       record_index < index.comparison_records.size();
       ++record_index) {
    const auto& record = index.comparison_records[record_index];
    if (!route7_block_matches(record.block_label, record.block_label_id,
                              block) ||
        record.instruction_index != instruction_index) {
      continue;
    }
    if (match != nullptr) {
      return route7_missing_reference(
          index,
          block,
          RouteIndexRecordCategory::Route7ComparisonInstruction,
          RouteIndexRelationshipKind::Route7Instruction,
          instruction_index,
          instruction_index,
          record.condition_value,
          Route7ComparisonOperandRole::None,
          RouteIndexValidationStatus::DuplicateReference,
          Route7ComparisonStatus::DuplicateProducer);
    }
    match = &record;
    match_index = record_index;
  }
  if (match == nullptr) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7ComparisonInstruction,
        RouteIndexRelationshipKind::Route7Instruction,
        instruction_index,
        instruction_index,
        empty_value,
        Route7ComparisonOperandRole::None,
        RouteIndexValidationStatus::MissingRecord,
        route7_missing_block_status(index, block));
  }

  auto status = route7_reference_status(match->status);
  bool valid = status == RouteIndexValidationStatus::Valid;
  if (valid && !route7_instruction_record_points_into_block(*match, block)) {
    status = RouteIndexValidationStatus::StaleOwner;
    valid = false;
  }
  return Route7IndexReferenceValidation{
      .valid = valid,
      .status = status,
      .route_status = match->status,
      .reference = route7_reference_key(
          index,
          block,
          RouteIndexRecordCategory::Route7ComparisonInstruction,
          RouteIndexRelationshipKind::Route7Instruction,
          instruction_index,
          instruction_index,
          match->condition_value,
          Route7ComparisonOperandRole::None,
          match_index),
      .comparison_record = match,
  };
}

Route7IndexReferenceValidation route7_validate_comparison_operand_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role) {
  const auto value_identity = route1_source_value_identity(value);
  if (route7_index_owner_is_stale(index, block)) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7ComparisonOperand,
        RouteIndexRelationshipKind::Route7Operand,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        role,
        RouteIndexValidationStatus::StaleOwner,
        Route7ComparisonStatus::MissingBlock);
  }

  const Route7ComparisonOperandRecord* match = nullptr;
  std::size_t match_index = 0;
  bool wrong_relationship = false;
  bool wrong_key = false;
  for (std::size_t record_index = 0;
       record_index < index.operand_records.size();
       ++record_index) {
    const auto& record = index.operand_records[record_index];
    if (!route7_block_matches(record.block_label, record.block_label_id,
                              block) ||
        !route7_value_matches(record.value, value)) {
      continue;
    }
    if (record.before_instruction_index != before_instruction_index) {
      wrong_key = true;
      continue;
    }
    if (record.role != role) {
      wrong_relationship = true;
      continue;
    }
    if (match != nullptr) {
      return route7_missing_reference(
          index,
          block,
          RouteIndexRecordCategory::Route7ComparisonOperand,
          RouteIndexRelationshipKind::Route7Operand,
          before_instruction_index,
          before_instruction_index,
          value_identity,
          role,
          RouteIndexValidationStatus::DuplicateReference,
          Route7ComparisonStatus::DuplicateProducer);
    }
    match = &record;
    match_index = record_index;
  }
  if (match == nullptr) {
    const auto status =
        wrong_relationship ? RouteIndexValidationStatus::WrongRelationship
                           : (wrong_key ? RouteIndexValidationStatus::WrongKey
                                        : RouteIndexValidationStatus::MissingRecord);
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7ComparisonOperand,
        RouteIndexRelationshipKind::Route7Operand,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        role,
        status,
        status == RouteIndexValidationStatus::MissingRecord
            ? route7_missing_block_status(index, block)
            : Route7ComparisonStatus::NoMatch);
  }

  auto status = route7_reference_status(match->status);
  bool valid = status == RouteIndexValidationStatus::Valid;
  if (valid && !route7_operand_record_points_into_block(*match, block)) {
    status = RouteIndexValidationStatus::StaleOwner;
    valid = false;
  }
  return Route7IndexReferenceValidation{
      .valid = valid,
      .status = status,
      .route_status = match->status,
      .reference = route7_reference_key(
          index,
          block,
          RouteIndexRecordCategory::Route7ComparisonOperand,
          RouteIndexRelationshipKind::Route7Operand,
          before_instruction_index,
          before_instruction_index,
          match->value,
          role,
          match_index),
      .operand_record = match,
  };
}

Route7IndexReferenceValidation route7_validate_materialized_condition_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index) {
  const auto value_identity = route1_source_value_identity(condition_value);
  if (route7_index_owner_is_stale(index, block)) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7ComparisonInstruction,
        RouteIndexRelationshipKind::Route7MaterializedCondition,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        Route7ComparisonOperandRole::ConditionValue,
        RouteIndexValidationStatus::StaleOwner,
        Route7ComparisonStatus::MissingBlock);
  }

  const Route7ComparisonInstructionRecord* match = nullptr;
  std::size_t match_index = 0;
  for (std::size_t record_index = 0;
       record_index < index.comparison_records.size();
       ++record_index) {
    const auto& record = index.comparison_records[record_index];
    if (!route7_block_matches(record.block_label, record.block_label_id,
                              block) ||
        record.instruction_index >= before_instruction_index ||
        !route7_value_matches(record.condition_value, condition_value)) {
      continue;
    }
    if (match != nullptr) {
      return route7_missing_reference(
          index,
          block,
          RouteIndexRecordCategory::Route7ComparisonInstruction,
          RouteIndexRelationshipKind::Route7MaterializedCondition,
          before_instruction_index,
          before_instruction_index,
          value_identity,
          Route7ComparisonOperandRole::ConditionValue,
          RouteIndexValidationStatus::DuplicateReference,
          Route7ComparisonStatus::DuplicateProducer);
    }
    match = &record;
    match_index = record_index;
  }
  if (match == nullptr) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7ComparisonInstruction,
        RouteIndexRelationshipKind::Route7MaterializedCondition,
        before_instruction_index,
        before_instruction_index,
        value_identity,
        Route7ComparisonOperandRole::ConditionValue,
        RouteIndexValidationStatus::MissingRecord,
        route7_missing_block_status(index, block));
  }

  auto status = route7_reference_status(match->status);
  bool valid = status == RouteIndexValidationStatus::Valid;
  if (valid && !route7_instruction_record_points_into_block(*match, block)) {
    status = RouteIndexValidationStatus::StaleOwner;
    valid = false;
  }
  return Route7IndexReferenceValidation{
      .valid = valid,
      .status = status,
      .route_status = match->status,
      .reference = route7_reference_key(
          index,
          block,
          RouteIndexRecordCategory::Route7ComparisonInstruction,
          RouteIndexRelationshipKind::Route7MaterializedCondition,
          match->instruction_index,
          before_instruction_index,
          match->condition_value,
          Route7ComparisonOperandRole::ConditionValue,
          match_index),
      .comparison_record = match,
  };
}

Route7IndexReferenceValidation route7_validate_branch_condition_reference(
    const Route7ComparisonConditionIndex& index,
    const Block& block) {
  const Route1SourceValueIdentity empty_value{};
  if (route7_index_owner_is_stale(index, block)) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7BranchCondition,
        RouteIndexRelationshipKind::Route7BranchCondition,
        block.insts.size(),
        block.insts.size(),
        empty_value,
        Route7ComparisonOperandRole::ConditionValue,
        RouteIndexValidationStatus::StaleOwner,
        Route7ComparisonStatus::MissingBlock);
  }

  const Route7BranchConditionRecord* match = nullptr;
  std::size_t match_index = 0;
  for (std::size_t record_index = 0;
       record_index < index.branch_condition_records.size();
       ++record_index) {
    const auto& record = index.branch_condition_records[record_index];
    if (!route7_block_matches(record.block_label, record.block_label_id,
                              block)) {
      continue;
    }
    if (match != nullptr) {
      return route7_missing_reference(
          index,
          block,
          RouteIndexRecordCategory::Route7BranchCondition,
          RouteIndexRelationshipKind::Route7BranchCondition,
          block.insts.size(),
          block.insts.size(),
          record.condition_value,
          Route7ComparisonOperandRole::ConditionValue,
          RouteIndexValidationStatus::DuplicateReference,
          Route7ComparisonStatus::DuplicateProducer);
    }
    match = &record;
    match_index = record_index;
  }
  if (match == nullptr) {
    return route7_missing_reference(
        index,
        block,
        RouteIndexRecordCategory::Route7BranchCondition,
        RouteIndexRelationshipKind::Route7BranchCondition,
        block.insts.size(),
        block.insts.size(),
        empty_value,
        Route7ComparisonOperandRole::ConditionValue,
        RouteIndexValidationStatus::MissingRecord,
        route7_missing_block_status(index, block));
  }

  auto status = route7_reference_status(match->status);
  bool valid = status == RouteIndexValidationStatus::Valid;
  if (valid &&
      !route7_instruction_record_points_into_block(match->comparison, block)) {
    status = RouteIndexValidationStatus::StaleOwner;
    valid = false;
  }
  return Route7IndexReferenceValidation{
      .valid = valid,
      .status = status,
      .route_status = match->status,
      .reference = route7_reference_key(
          index,
          block,
          RouteIndexRecordCategory::Route7BranchCondition,
          RouteIndexRelationshipKind::Route7BranchCondition,
          block.insts.size(),
          block.insts.size(),
          match->condition_value,
          Route7ComparisonOperandRole::ConditionValue,
          match_index),
      .branch_condition_record = match,
  };
}

RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications) {
  return RouteIndexReferenceFacade{
      .route4_publications = &route4_publications,
  };
}

RouteIndexReferenceFacade route_index_reference_facade(
    const Route7ComparisonConditionIndex& route7_comparisons) {
  return RouteIndexReferenceFacade{
      .route7_comparisons = &route7_comparisons,
  };
}

RouteIndexReferenceFacade route_index_reference_facade(
    const Route4PublicationAvailabilityIndex& route4_publications,
    const Route7ComparisonConditionIndex& route7_comparisons) {
  return RouteIndexReferenceFacade{
      .route4_publications = &route4_publications,
      .route7_comparisons = &route7_comparisons,
  };
}

Route4IndexReferenceValidation
route_index_validate_current_block_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index) {
  if (facade.route4_publications == nullptr) {
    return Route4IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route4PublicationAvailabilityStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route4PublicationAvailability,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category =
                    RouteIndexRecordCategory::Route4CurrentBlockPublication,
                .relationship =
                    RouteIndexRelationshipKind::Route4CurrentBlockPublication,
                .block = &block,
                .block_label = block.label,
                .block_label_id = block.label_id,
                .instruction_index = before_instruction_index,
                .before_instruction_index = before_instruction_index,
                .value = route1_source_value_identity(value),
            },
    };
  }
  return route4_validate_current_block_publication_reference(
      *facade.route4_publications, block, value, before_instruction_index);
}

Route4IndexReferenceValidation
route_index_validate_block_entry_publication_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& successor_block,
    const Value& destination_value) {
  if (facade.route4_publications == nullptr) {
    return Route4IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route4PublicationAvailabilityStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route4PublicationAvailability,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category =
                    RouteIndexRecordCategory::Route4BlockEntryPublication,
                .relationship =
                    RouteIndexRelationshipKind::Route4BlockEntryPublication,
                .block = &successor_block,
                .block_label = successor_block.label,
                .block_label_id = successor_block.label_id,
                .value = route1_source_value_identity(destination_value),
            },
    };
  }
  return route4_validate_block_entry_publication_reference(
      *facade.route4_publications, successor_block, destination_value);
}

Route7IndexReferenceValidation
route_index_validate_materialized_condition_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index) {
  if (facade.route7_comparisons == nullptr) {
    return Route7IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route7ComparisonStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route7ComparisonCondition,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category =
                    RouteIndexRecordCategory::Route7ComparisonInstruction,
                .relationship =
                    RouteIndexRelationshipKind::Route7MaterializedCondition,
                .block = &block,
                .block_label = block.label,
                .block_label_id = block.label_id,
                .instruction_index = before_instruction_index,
                .before_instruction_index = before_instruction_index,
                .value = route1_source_value_identity(condition_value),
                .operand_role = Route7ComparisonOperandRole::ConditionValue,
            },
    };
  }
  return route7_validate_materialized_condition_reference(
      *facade.route7_comparisons,
      block,
      condition_value,
      before_instruction_index);
}

FusedCompareOperandProducerFacts find_fused_compare_operand_producer_facts(
    const Block& block,
    const Value& lhs,
    const Value& rhs,
    std::size_t before_instruction_index) {
  FusedCompareOperandProducerFacts result{
      .lhs = find_comparison_operand_producer(
          block, lhs, before_instruction_index),
      .rhs = find_comparison_operand_producer(
          block, rhs, before_instruction_index),
  };
  result.available = result.lhs.has_value() || result.rhs.has_value();
  return result;
}

namespace {

[[nodiscard]] std::optional<ComparisonOperandProducer>
route7_operand_record_to_public(const Route7ComparisonOperandRecord& record) {
  if (!record ||
      record.producer_kind == ComparisonProducerKind::Unknown) {
    return std::nullopt;
  }
  return ComparisonOperandProducer{
      .available = true,
      .producer_kind = record.producer_kind,
      .producer_instruction = record.producer_instruction,
      .producer_instruction_index = record.producer_instruction_index,
      .produced_value = record.produced_value.value,
      .integer_constant = record.integer_constant,
  };
}

}  // namespace

MaterializedConditionProducerIdentity find_materialized_condition_producer_identity(
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index) {
  const auto index = route7_build_comparison_condition_index(block);
  const auto facade = route_index_reference_facade(index);
  const auto reference = route_index_validate_materialized_condition_reference(
      facade, block, condition_value, before_instruction_index);
  const auto* record = reference.comparison_record;
  if (!reference || record == nullptr || record->binary == nullptr) {
    return {};
  }
  return MaterializedConditionProducerIdentity{
      .available = true,
      .binary = record->binary,
      .instruction_index = record->instruction_index,
      .condition_value_name = std::string(record->condition_value.name),
      .lhs = route7_operand_record_to_public(record->lhs),
      .rhs = route7_operand_record_to_public(record->rhs),
  };
}

CallResultSourceIdentity find_call_result_source_identity(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index) {
  if (indexed_call_inst(block, call, call_instruction_index) == nullptr ||
      !call.result.has_value() ||
      call.result->kind != Value::Kind::Named ||
      call.result->name.empty()) {
    return {};
  }
  return CallResultSourceIdentity{
      .available = true,
      .call_instruction_index = call_instruction_index,
      .result_value = &*call.result,
  };
}

CallResultLaneSourceIdentity find_call_result_lane_source_identity(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index,
    const Value& value) {
  const auto result_identity =
      find_call_result_source_identity(block, call, call_instruction_index);
  if (!result_identity.available ||
      value.kind != Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }

  CallResultLaneSourceIdentity result;
  if (call.result->kind == value.kind &&
      call.result->name == value.name &&
      call.result->type == value.type) {
    result = CallResultLaneSourceIdentity{
        .available = true,
        .call_instruction_index = call_instruction_index,
        .lane_index = 0,
        .lane_value = &*call.result,
        .aliases_primary_result = true,
    };
  }

  for (std::size_t lane_index = 0; lane_index < call.result_lanes.size();
       ++lane_index) {
    const auto& lane = call.result_lanes[lane_index];
    if (lane.kind != value.kind ||
        lane.name != value.name ||
        lane.type != value.type) {
      continue;
    }
    const bool aliases_primary_result =
        lane_index == 0 &&
        call.result->kind == lane.kind &&
        call.result->name == lane.name &&
        call.result->type == lane.type;
    if (result.available && result.aliases_primary_result &&
        aliases_primary_result) {
      continue;
    }
    if (result.available) {
      return {};
    }
    result = CallResultLaneSourceIdentity{
        .available = true,
        .call_instruction_index = call_instruction_index,
        .lane_index = lane_index,
        .lane_value = &lane,
        .aliases_primary_result = aliases_primary_result,
    };
  }
  return result;
}

CallArgumentPublicationSourceRouting find_call_argument_publication_source_routing(
    const CallInst& call,
    std::size_t arg_index) {
  const auto* relationship =
      find_call_argument_source_relationship(call, arg_index);
  if (relationship == nullptr) {
    return {};
  }

  const auto* selection =
      relationship->source_selection.has_value() &&
              call_argument_source_selection_available(
                  *relationship->source_selection)
          ? &*relationship->source_selection
          : nullptr;
  const auto* dependency =
      relationship->direct_global_select_chain_dependency.has_value() &&
              call_argument_direct_global_select_chain_dependency_available(
                  *relationship->direct_global_select_chain_dependency)
          ? &*relationship->direct_global_select_chain_dependency
          : nullptr;
  const bool available =
      relationship->source_encoding != CallArgumentSourceEncodingKind::None ||
      selection != nullptr ||
      dependency != nullptr;
  if (!available) {
    return {};
  }

  return CallArgumentPublicationSourceRouting{
      .available = true,
      .arg_index = relationship->arg_index,
      .source_encoding = relationship->source_encoding,
      .source_value_id = relationship->source_value_id,
      .source_value_name = relationship->source_value_name,
      .source_base_value_id = relationship->source_base_value_id,
      .source_base_value_name = relationship->source_base_value_name,
      .source_pointer_byte_delta = relationship->source_pointer_byte_delta,
      .source_selection = selection,
      .direct_global_select_chain_dependency = dependency,
  };
}

}  // namespace c4c::backend::bir
