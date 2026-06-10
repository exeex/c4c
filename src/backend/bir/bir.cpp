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

CallArgumentSourceProducerMaterialization
find_call_argument_source_producer_materialization(
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

MaterializedConditionProducerIdentity find_materialized_condition_producer_identity(
    const Block& block,
    const Value& condition_value,
    std::size_t before_instruction_index) {
  if (condition_value.kind != Value::Kind::Named ||
      condition_value.name.empty()) {
    return {};
  }
  const auto producer =
      find_unique_comparison_producer(block, condition_value,
                                      before_instruction_index);
  if (producer.inst == nullptr || producer.ambiguous) {
    return {};
  }
  const auto* binary = std::get_if<BinaryInst>(producer.inst);
  if (binary == nullptr || !is_comparison_binary_opcode(binary->opcode)) {
    return {};
  }
  return MaterializedConditionProducerIdentity{
      .available = true,
      .binary = binary,
      .instruction_index = producer.instruction_index,
      .condition_value_name = condition_value.name,
      .lhs = find_comparison_operand_producer(
          block, binary->lhs, producer.instruction_index),
      .rhs = find_comparison_operand_producer(
          block, binary->rhs, producer.instruction_index),
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
