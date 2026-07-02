#include "bir.hpp"

#include <algorithm>
#include <type_traits>

namespace c4c::backend::bir {
namespace {

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
  const auto lhs = route1_evaluate_same_block_integer_constant_impl(
      nested_query, binary->lhs, depth + 1);
  const auto rhs = route1_evaluate_same_block_integer_constant_impl(
      nested_query, binary->rhs, depth + 1);
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

}  // namespace c4c::backend::bir
