#include "bir.hpp"
#include "bir_private.hpp"

namespace c4c::backend::bir {

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
  if (!materialization.available ||
      materialization.producer_instruction == nullptr ||
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

  auto route2 = route2_select_chain_value_record(query, call.args[arg_index]);
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
      route4_current_block_publication_record(query, call.args[arg_index]);
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
    for (std::size_t instruction_index = 0;
         instruction_index < block.insts.size();
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
