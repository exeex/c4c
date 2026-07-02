#include "bir.hpp"
#include "bir_private.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace c4c::backend::bir {

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
    unsigned depth) {
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

namespace {

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
    if (route_block_matches(record.block_label, record.block_label_id, block)) {
      return Route7ComparisonStatus::NoMatch;
    }
  }
  for (const auto& record : index.branch_condition_records) {
    if (route_block_matches(record.block_label, record.block_label_id, block)) {
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

Route7ComparisonInstructionRecord route7_find_comparison_instruction(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    std::size_t instruction_index) {
  for (const auto& record : index.comparison_records) {
    if (route_block_matches(record.block_label, record.block_label_id, block) &&
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
    if (route_block_matches(record.block_label, record.block_label_id, block) &&
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
    if (!route_block_matches(record.block_label, record.block_label_id, block) ||
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
    if (route_block_matches(record.block_label, record.block_label_id, block)) {
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
    if (!route_block_matches(record.block_label, record.block_label_id, block) ||
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
    if (!route_block_matches(record.block_label, record.block_label_id, block) ||
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
    if (!route_block_matches(record.block_label, record.block_label_id, block) ||
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
    if (!route_block_matches(record.block_label, record.block_label_id, block)) {
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

FusedCompareOperandProducerFacts route7_find_fused_compare_operand_producer_facts(
    const Route7ComparisonConditionIndex& index,
    const Block& block,
    const Value& lhs,
    const Value& rhs,
    std::size_t before_instruction_index) {
  const auto facade = route_index_reference_facade(index);
  const auto lhs_reference = route_index_validate_comparison_operand_reference(
      facade,
      block,
      lhs,
      before_instruction_index,
      Route7ComparisonOperandRole::Lhs);
  const auto rhs_reference = route_index_validate_comparison_operand_reference(
      facade,
      block,
      rhs,
      before_instruction_index,
      Route7ComparisonOperandRole::Rhs);
  FusedCompareOperandProducerFacts result{
      .lhs = lhs_reference.operand_record == nullptr
                 ? std::nullopt
                 : route7_operand_record_to_public(*lhs_reference.operand_record),
      .rhs = rhs_reference.operand_record == nullptr
                 ? std::nullopt
                 : route7_operand_record_to_public(*rhs_reference.operand_record),
  };
  result.available = result.lhs.has_value() || result.rhs.has_value();
  if (result.available || before_instruction_index != block.insts.size()) {
    return result;
  }

  const auto branch_reference =
      route7_validate_branch_condition_reference(index, block);
  const auto* branch_record = branch_reference.branch_condition_record;
  if (!branch_reference || branch_record == nullptr ||
      branch_record->kind != Route7BranchConditionKind::FusedCompare ||
      !route7_value_matches(branch_record->comparison.lhs_value, lhs) ||
      !route7_value_matches(branch_record->comparison.rhs_value, rhs)) {
    return result;
  }
  result.lhs = route7_operand_record_to_public(branch_record->comparison.lhs);
  result.rhs = route7_operand_record_to_public(branch_record->comparison.rhs);
  result.available = result.lhs.has_value() || result.rhs.has_value();
  return result;
}

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

}  // namespace c4c::backend::bir
