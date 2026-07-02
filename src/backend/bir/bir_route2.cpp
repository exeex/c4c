#include "bir.hpp"

#include <type_traits>

namespace c4c::backend::bir {

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

}  // namespace c4c::backend::bir
