#include "bir.hpp"
#include "bir_private.hpp"

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

bool route6_call_argument_source_matches_argument_value_record(
    const Route6CallArgumentSourceRecord& source,
    const Value& value) {
  if (!source ||
      source.source_kind != Route6CallUseSourceKind::ArgumentValue ||
      source.argument_value != &value ||
      source.source_value.value != &value ||
      source.source_value.name != value.name) {
    return false;
  }
  if (source.source_value_name.has_value() &&
      *source.source_value_name != value.name) {
    return false;
  }
  return true;
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

Route7IndexReferenceValidation route_index_validate_comparison_operand_reference(
    const RouteIndexReferenceFacade& facade,
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role) {
  if (facade.route7_comparisons == nullptr) {
    return Route7IndexReferenceValidation{
        .valid = false,
        .status = RouteIndexValidationStatus::MissingRecord,
        .route_status = Route7ComparisonStatus::MissingBlock,
        .reference =
            RouteIndexRecordReference{
                .route = RouteIndexRoute::Route7ComparisonCondition,
                .owner_scope = RouteIndexOwnerScope::None,
                .record_category = RouteIndexRecordCategory::Route7ComparisonOperand,
                .relationship = RouteIndexRelationshipKind::Route7Operand,
                .block = &block,
                .block_label = block.label,
                .block_label_id = block.label_id,
                .instruction_index = before_instruction_index,
                .before_instruction_index = before_instruction_index,
                .value = route1_source_value_identity(value),
                .operand_role = role,
            },
    };
  }
  return route7_validate_comparison_operand_reference(
      *facade.route7_comparisons, block, value, before_instruction_index, role);
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
