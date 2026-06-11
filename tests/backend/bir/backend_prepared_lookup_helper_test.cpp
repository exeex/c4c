#include "src/backend/prealloc/comparison.hpp"
#include "src/backend/prealloc/module.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"
#include "src/backend/prealloc/publication_plans.hpp"
#include "src/backend/mir/query.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedControlFlowBlock return_block(c4c::BlockLabelId label) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::Return,
  };
}

prepare::PreparedControlFlowBlock branch_block(c4c::BlockLabelId label,
                                               c4c::BlockLabelId target) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = target,
  };
}

prepare::PreparedControlFlowBlock cond_branch_block(c4c::BlockLabelId label,
                                                    c4c::BlockLabelId true_label,
                                                    c4c::BlockLabelId false_label) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
}

bool route8_return_chain_queries_fail_closed(
    const bir::Route8ReturnChainIndex& route8_index,
    const bir::Route8ReturnChainValueKey& route8_key,
    std::string_view message) {
  if (bir::route8_find_return_chain_terminal_value(route8_index, route8_key) ||
      bir::route8_find_return_chain_next_operand_value(route8_index, route8_key)) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

bool expect_same(const void* actual, const void* expected, std::string_view message) {
  if (actual != expected) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

mir::SameBlockProducerKind expected_bir_producer_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return mir::SameBlockProducerKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return mir::SameBlockProducerKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return mir::SameBlockProducerKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return mir::SameBlockProducerKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return mir::SameBlockProducerKind::Select;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return mir::SameBlockProducerKind::Unknown;
  }
  return mir::SameBlockProducerKind::Unknown;
}

bir::Route1ProducerKind expected_bir_route1_producer_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return bir::Route1ProducerKind::Immediate;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::Route1ProducerKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::Route1ProducerKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::Route1ProducerKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::Route1ProducerKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::Route1ProducerKind::SelectMaterialization;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return bir::Route1ProducerKind::Unknown;
  }
  return bir::Route1ProducerKind::Unknown;
}

bir::Route4PublicationSourceKind expected_bir_route4_publication_source_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return bir::Route4PublicationSourceKind::Immediate;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::Route4PublicationSourceKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::Route4PublicationSourceKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::Route4PublicationSourceKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::Route4PublicationSourceKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::Route4PublicationSourceKind::SelectMaterialization;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return bir::Route4PublicationSourceKind::Unknown;
  }
  return bir::Route4PublicationSourceKind::Unknown;
}

bir::Route5PublicationSourceKind expected_bir_route5_publication_source_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return bir::Route5PublicationSourceKind::Immediate;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::Route5PublicationSourceKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::Route5PublicationSourceKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::Route5PublicationSourceKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::Route5PublicationSourceKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::Route5PublicationSourceKind::SelectMaterialization;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return bir::Route5PublicationSourceKind::Unknown;
  }
  return bir::Route5PublicationSourceKind::Unknown;
}

bir::Route2SelectChainProducerKind expected_bir_route2_select_chain_producer_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::Route2SelectChainProducerKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::Route2SelectChainProducerKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::Route2SelectChainProducerKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::Route2SelectChainProducerKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::Route2SelectChainProducerKind::Select;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      break;
  }
  return bir::Route2SelectChainProducerKind::Unknown;
}

bool bir_route1_producer_record_matches(
    const bir::Block& block,
    std::size_t instruction_index,
    const bir::Value& expected_value,
    prepare::PreparedEdgePublicationSourceProducerKind expected_kind) {
  const auto expected_bir_kind = expected_bir_route1_producer_kind(expected_kind);
  const auto record = bir::route1_producer_record(block, instruction_index);
  return record &&
         record.kind == expected_bir_kind &&
         record.producer_instruction &&
         record.producer_instruction.instruction == &block.insts[instruction_index] &&
         record.producer_instruction.instruction_index == instruction_index &&
         record.producer_instruction.kind == expected_bir_kind &&
         record.producer_instruction.block_label == block.label &&
         record.producer_instruction.block_label_id == block.label_id &&
         record.source_value &&
         record.source_value.value != nullptr &&
         *record.source_value.value == expected_value &&
         record.source_value.value_kind == expected_value.kind &&
         record.source_value.name == expected_value.name &&
         record.source_value.type == expected_value.type &&
         !record.source_value.integer_constant.has_value() &&
         !record.integer_constant &&
         record.materialization &&
         record.materialization.producer_kind == expected_bir_kind &&
         record.materialization.scalar_materialization_available ==
             bir::route1_producer_kind_has_materialization(expected_bir_kind);
}

bir::CallArgumentSourceEncodingKind expected_bir_call_source_encoding(
    prepare::PreparedStorageEncodingKind kind) {
  switch (kind) {
    case prepare::PreparedStorageEncodingKind::None:
      return bir::CallArgumentSourceEncodingKind::None;
    case prepare::PreparedStorageEncodingKind::Register:
      return bir::CallArgumentSourceEncodingKind::Register;
    case prepare::PreparedStorageEncodingKind::FrameSlot:
      return bir::CallArgumentSourceEncodingKind::FrameSlot;
    case prepare::PreparedStorageEncodingKind::Immediate:
      return bir::CallArgumentSourceEncodingKind::Immediate;
    case prepare::PreparedStorageEncodingKind::ComputedAddress:
      return bir::CallArgumentSourceEncodingKind::ComputedAddress;
    case prepare::PreparedStorageEncodingKind::SymbolAddress:
      return bir::CallArgumentSourceEncodingKind::SymbolAddress;
  }
  return bir::CallArgumentSourceEncodingKind::None;
}

bir::CallArgumentSourceSelectionKind expected_bir_call_source_selection_kind(
    prepare::PreparedCallArgumentSourceSelectionKind kind) {
  switch (kind) {
    case prepare::PreparedCallArgumentSourceSelectionKind::None:
      return bir::CallArgumentSourceSelectionKind::None;
    case prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation:
      return bir::CallArgumentSourceSelectionKind::PriorPreservation;
    case prepare::PreparedCallArgumentSourceSelectionKind::
        LocalFrameAddressMaterialization:
      return bir::CallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization;
    case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress:
      return bir::CallArgumentSourceSelectionKind::FrameSlotAddress;
    case prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue:
      return bir::CallArgumentSourceSelectionKind::FrameSlotValue;
    case prepare::PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane:
      return bir::CallArgumentSourceSelectionKind::ByvalRegisterLane;
  }
  return bir::CallArgumentSourceSelectionKind::None;
}

bir::CallArgumentSourceProducerKind
expected_bir_call_argument_source_producer_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::CallArgumentSourceProducerKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::CallArgumentSourceProducerKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
    case prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization:
      return bir::CallArgumentSourceProducerKind::Unknown;
  }
  return bir::CallArgumentSourceProducerKind::Unknown;
}

bool prepared_and_bir_scalar_producers_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    mir::SameBlockValueMaterializationQuery bir_query,
    const bir::Value& value,
    prepare::PreparedEdgePublicationSourceProducerKind expected_kind) {
  const auto prepared =
      prepare::find_prepared_same_block_scalar_producer(
          names, &source_producers, block_label, &block, value,
          bir_query.before_instruction_index);
  const auto bir = mir::find_same_block_scalar_producer(bir_query, value);
  if (!prepared.has_value() || !bir.has_value()) {
    return false;
  }
  const auto expected_bir_kind = expected_bir_producer_kind(expected_kind);
  return prepared->producer.kind == expected_kind &&
         prepared->instruction == bir->instruction &&
         prepared->instruction_index == bir->instruction_index &&
         prepared->value_name == names.value_names.find(value.name) &&
         bir->producer.kind == expected_bir_kind &&
         bir->producer.inst == prepared->instruction &&
         bir->producer.instruction_index == prepared->instruction_index &&
         bir->producer.block_label == block.label &&
         bir->producer.produced_value.name == value.name &&
         bir->producer.produced_value.type == value.type &&
         bir->producer.produced_value.value == bir->produced_value &&
         bir->producer.materialization_available &&
         bir->produced_value != nullptr &&
         bir->produced_value->name == value.name &&
         bir->produced_value->type == value.type;
}

bool prepared_and_route1_scalar_producers_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value,
    prepare::PreparedEdgePublicationSourceProducerKind expected_kind) {
  const auto prepared =
      prepare::find_prepared_same_block_scalar_producer(
          names, &source_producers, block_label, &block, value,
          route1_query.before_instruction_index);
  const auto route1 =
      bir::route1_find_same_block_scalar_producer(route1_query, value);
  if (!prepared.has_value() || !route1.has_value()) {
    return false;
  }
  const auto expected_route1_kind =
      expected_bir_route1_producer_kind(expected_kind);
  return prepared->producer.kind == expected_kind &&
         route1->record != nullptr &&
         route1->record->kind == expected_route1_kind &&
         route1->record->producer_instruction.kind == expected_route1_kind &&
         route1->instruction == prepared->instruction &&
         route1->instruction_index == prepared->instruction_index &&
         route1->produced_value != nullptr &&
         route1->produced_value->name == value.name &&
         route1->produced_value->type == value.type &&
         route1->materialization.available &&
         route1->materialization.producer_kind == expected_route1_kind &&
         route1->materialization.scalar_materialization_available ==
             bir::route1_producer_kind_has_materialization(expected_route1_kind);
}

bool prepared_and_route1_materialization_availability_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value,
    prepare::PreparedEdgePublicationSourceProducerKind expected_kind) {
  const auto prepared =
      prepare::find_prepared_same_block_scalar_producer(
          names, &source_producers, block_label, &block, value,
          route1_query.before_instruction_index);
  const auto route1 =
      bir::route1_find_materialization_availability(route1_query, value);
  if (!prepared.has_value() || !route1) {
    return false;
  }
  const auto expected_route1_kind =
      expected_bir_route1_producer_kind(expected_kind);
  return prepared->producer.kind == expected_kind &&
         route1.producer_kind == expected_route1_kind &&
         route1.scalar_materialization_available ==
             bir::route1_producer_kind_has_materialization(expected_route1_kind);
}

bool prepared_and_route1_materialization_availability_both_fail(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value) {
  return !prepare::find_prepared_same_block_scalar_producer(
              names, &source_producers, block_label, &block, value,
              route1_query.before_instruction_index)
              .has_value() &&
         !bir::route1_find_materialization_availability(route1_query, value);
}

bool prepared_and_bir_call_argument_source_producer_materialization_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    const bir::CallInst& call,
    std::size_t call_instruction_index,
    std::size_t arg_index) {
  if (arg_index >= call.args.size()) {
    return false;
  }
  const auto prepared =
      prepare::find_prepared_call_argument_source_producer_materialization(
          names,
          &source_producers,
          block_label,
          &block,
          call.args[arg_index],
          call_instruction_index);
  const auto bir =
      bir::find_call_argument_source_producer_materialization(
          block, call, call_instruction_index, arg_index);
  if (prepared.has_value() != (bir.available && bir.materializable)) {
    return false;
  }
  if (!prepared.has_value()) {
    return !bir.materializable;
  }
  return bir.available &&
         bir.arg_index == arg_index &&
         bir.producer_kind ==
             expected_bir_call_argument_source_producer_kind(
                 prepared->producer.producer.kind) &&
         bir.producer_instruction == prepared->producer.instruction &&
         bir.producer_instruction_index ==
             prepared->producer.instruction_index &&
         bir.produced_value != nullptr &&
         bir.produced_value->name == call.args[arg_index].name &&
         bir.produced_value->type == call.args[arg_index].type &&
         bir.materializable == prepared->materializable &&
         prepared->producer.value_name ==
             names.value_names.find(call.args[arg_index].name);
}

bir::ComparisonProducerKind expected_bir_comparison_producer_kind(
    prepare::PreparedEdgePublicationSourceProducerKind kind) {
  switch (kind) {
    case prepare::PreparedEdgePublicationSourceProducerKind::Immediate:
      return bir::ComparisonProducerKind::Immediate;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal:
      return bir::ComparisonProducerKind::LoadLocal;
    case prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal:
      return bir::ComparisonProducerKind::LoadGlobal;
    case prepare::PreparedEdgePublicationSourceProducerKind::Cast:
      return bir::ComparisonProducerKind::Cast;
    case prepare::PreparedEdgePublicationSourceProducerKind::Binary:
      return bir::ComparisonProducerKind::Binary;
    case prepare::PreparedEdgePublicationSourceProducerKind::
        SelectMaterialization:
      return bir::ComparisonProducerKind::Select;
    case prepare::PreparedEdgePublicationSourceProducerKind::Unknown:
      return bir::ComparisonProducerKind::Unknown;
  }
  return bir::ComparisonProducerKind::Unknown;
}

bool prepared_and_bir_comparison_operand_producer_match(
    const prepare::PreparedNameTables& names,
    const bir::Block& block,
    const std::optional<prepare::PreparedFusedCompareOperandProducer>& prepared,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto bir =
      bir::find_comparison_operand_producer(block, value, before_instruction_index);
  if (prepared.has_value() != bir.has_value()) {
    return false;
  }
  if (!prepared.has_value()) {
    return true;
  }
  if (!bir.has_value() ||
      bir->producer_kind !=
          expected_bir_comparison_producer_kind(prepared->kind) ||
      bir->integer_constant != prepared->integer_constant) {
    return false;
  }
  if (prepared->kind == prepare::PreparedEdgePublicationSourceProducerKind::
                            Immediate) {
    return bir->producer_instruction == nullptr &&
           bir->produced_value == nullptr;
  }
  return bir->producer_instruction == prepared->instruction &&
         bir->producer_instruction_index == prepared->instruction_index &&
         bir->produced_value != nullptr &&
         bir->produced_value->name == value.name &&
         bir->produced_value->type == value.type &&
         prepared->value_name == names.value_names.find(value.name);
}

bool prepared_and_bir_call_result_source_identity_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedCallResultPlan& prepared_result,
    prepare::PreparedValueId expected_destination_value_id,
    c4c::ValueNameId expected_result_name,
    const bir::Block& block,
    const bir::CallInst& call,
    std::size_t call_instruction_index) {
  const auto bir = bir::find_call_result_source_identity(
      block, call, call_instruction_index);
  if (!prepared_result.destination_value_id.has_value()) {
    return !bir.available;
  }
  return *prepared_result.destination_value_id ==
             expected_destination_value_id &&
         bir.available &&
         bir.call_instruction_index == prepared_result.instruction_index &&
         bir.result_value != nullptr &&
         bir.result_value->kind == bir::Value::Kind::Named &&
         names.value_names.find(bir.result_value->name) ==
             expected_result_name;
}

bool prepared_and_bir_call_result_lane_source_identity_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedAfterCallResultLaneBinding& prepared_lane,
    const bir::Block& block,
    const bir::CallInst& call,
    std::size_t call_instruction_index,
    const bir::Value& expected_lane_value) {
  const auto bir = bir::find_call_result_lane_source_identity(
      block, call, call_instruction_index, expected_lane_value);
  return bir.available &&
         bir.call_instruction_index == prepared_lane.instruction_index &&
         bir.lane_index == prepared_lane.lane_index &&
         bir.lane_value != nullptr &&
         bir.lane_value->kind == bir::Value::Kind::Named &&
         bir.lane_value->name == expected_lane_value.name &&
         bir.lane_value->type == expected_lane_value.type &&
         names.value_names.find(bir.lane_value->name) ==
             prepared_lane.value_name;
}

bool prepared_and_bir_call_argument_publication_source_routing_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedCallArgumentPlan& prepared_argument,
    const bir::CallInst& call,
    std::size_t arg_index) {
  const auto prepared =
      prepare::find_prepared_call_argument_publication_source_routing(
          prepared_argument);
  const auto bir = bir::find_call_argument_publication_source_routing(
      call, arg_index);
  if (prepared.available != bir.available) {
    return false;
  }
  if (!prepared.available) {
    return true;
  }
  if (!bir.available ||
      bir.arg_index != arg_index ||
      bir.source_encoding !=
          expected_bir_call_source_encoding(prepared.source_encoding) ||
      bir.source_value_id != prepared.source_value_id ||
      bir.source_base_value_id != prepared.source_base_value_id ||
      bir.source_pointer_byte_delta != prepared.source_pointer_byte_delta) {
    return false;
  }
  if (prepared.source_base_value_name.has_value()) {
    if (!bir.source_base_value_name.has_value() ||
        *bir.source_base_value_name !=
            prepare::prepared_value_name(
                names, *prepared.source_base_value_name)) {
      return false;
    }
  } else if (bir.source_base_value_name.has_value()) {
    return false;
  }

  if ((prepared.source_selection != nullptr) !=
      (bir.source_selection != nullptr)) {
    return false;
  }
  if (prepared.source_selection != nullptr) {
    const auto& prepared_selection = *prepared.source_selection;
    const auto& bir_selection = *bir.source_selection;
    if (bir_selection.kind !=
            expected_bir_call_source_selection_kind(
                prepared_selection.kind) ||
        bir_selection.source_value_id != prepared_selection.source_value_id ||
        bir_selection.source_base_value_id !=
            prepared_selection.source_base_value_id ||
        bir_selection.source_pointer_byte_delta !=
            prepared_selection.source_pointer_byte_delta ||
        bir_selection.source_stack_offset_bytes !=
            prepared_selection.source_stack_offset_bytes ||
        bir_selection.source_size_bytes !=
            prepared_selection.source_size_bytes ||
        bir_selection.source_align_bytes !=
            prepared_selection.source_align_bytes ||
        bir_selection.address_materialization_block_label !=
            prepared_selection.address_materialization_block_label ||
        bir_selection.address_materialization_inst_index !=
            prepared_selection.address_materialization_inst_index ||
        bir_selection.address_materialization_byte_offset !=
            prepared_selection.address_materialization_byte_offset) {
      return false;
    }
    if (prepared_selection.source_value_name.has_value()) {
      if (!bir_selection.source_value_name.has_value() ||
          *bir_selection.source_value_name !=
              prepare::prepared_value_name(
                  names, *prepared_selection.source_value_name)) {
        return false;
      }
    } else if (bir_selection.source_value_name.has_value()) {
      return false;
    }
    if (prepared_selection.source_slot_id.has_value()) {
      if (!bir_selection.source_slot_id.has_value() ||
          *bir_selection.source_slot_id !=
              static_cast<c4c::SlotNameId>(
                  *prepared_selection.source_slot_id)) {
        return false;
      }
    } else if (bir_selection.source_slot_id.has_value()) {
      return false;
    }
    if (prepared_selection.address_materialization_frame_slot_id.has_value()) {
      if (!bir_selection.address_materialization_frame_slot_id.has_value() ||
          *bir_selection.address_materialization_frame_slot_id !=
              static_cast<c4c::SlotNameId>(
                  *prepared_selection
                       .address_materialization_frame_slot_id)) {
        return false;
      }
    } else if (bir_selection.address_materialization_frame_slot_id.has_value()) {
      return false;
    }
  }

  if ((prepared.direct_global_select_chain_dependency != nullptr) !=
      (bir.direct_global_select_chain_dependency != nullptr)) {
    return false;
  }
  if (prepared.direct_global_select_chain_dependency != nullptr) {
    const auto& prepared_dependency =
        *prepared.direct_global_select_chain_dependency;
    const auto& bir_dependency = *bir.direct_global_select_chain_dependency;
    if (!bir_dependency.available ||
        bir_dependency.source_value_name !=
            prepare::prepared_value_name(
                names, prepared_dependency.source_value_name) ||
        bir_dependency.contains_direct_global_load !=
            prepared_dependency.direct_global_dependency
                .contains_direct_global_load ||
        bir_dependency.root_is_select !=
            prepared_dependency.direct_global_dependency.root_is_select ||
        bir_dependency.root_instruction_index !=
            prepared_dependency.direct_global_dependency
                .root_instruction_index) {
      return false;
    }
  }
  return true;
}

bool prepared_and_bir_current_block_publication_identity_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    c4c::ValueNameId value_name,
    bir::TypeKind value_type,
    std::size_t before_instruction_index) {
  const auto prepared =
      prepare::find_prepared_current_block_publication_consumption(
          names,
          &source_producers,
          block_label,
          &block,
          value_name,
          before_instruction_index);
  const bool route4_same_block =
      prepare::prepared_block_label(names, block_label) == block.label;
  const auto route1_index = bir::route1_build_producer_index(block);
  const auto route1_query = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = before_instruction_index,
  };
  const auto route4_value =
      bir::Value::named(value_type,
                       std::string{prepare::prepared_value_name(names,
                                                                 value_name)});
  const auto route4 =
      route4_same_block
          ? bir::route4_current_block_publication_record(route1_query,
                                                         route4_value,
                                                         value_name)
          : bir::Route4CurrentBlockPublicationRecord{};
  const auto route4_recorded_value =
      route4_same_block
          ? bir::route4_current_block_publication_value_record(route1_query,
                                                               route4_value,
                                                               value_name)
          : bir::Route4PublicationValueRecord{};
  const auto bir = mir::find_bir_current_block_publication_identity(
      mir::BirCurrentBlockPublicationIdentityRequest{
          .block = &block,
          .block_label = prepare::prepared_block_label(names, block_label),
          .root_value_name = prepare::prepared_value_name(names, value_name),
          .root_value_type = value_type,
          .before_instruction_index = before_instruction_index,
      });
  if (prepared.available != bir.available) {
    return false;
  }
  if (!prepared.available) {
    return !route4_same_block ||
           (!route4 &&
            route4.status != bir::Route4PublicationAvailabilityStatus::Available &&
            !route4_recorded_value &&
            route4_recorded_value.scope ==
                bir::Route4PublicationScope::CurrentBlock &&
            route4_recorded_value.status == route4.status);
  }
  return prepared.source_producer != nullptr &&
         prepared.instruction != nullptr &&
         prepared.produced_value != nullptr &&
         route4 &&
         route4.status == bir::Route4PublicationAvailabilityStatus::Available &&
         route4.value_name == prepared.produced_value->name &&
         route4.value_name_id == prepared.value_name &&
         route4.value_type == prepared.produced_value->type &&
         route4.before_instruction_index == before_instruction_index &&
         route4.source_producer_instruction == prepared.instruction &&
         route4.source_producer_instruction_index ==
             prepared.instruction_index &&
         route4.produced_value.value == prepared.produced_value &&
         route4.produced_value.name == prepared.produced_value->name &&
         route4.produced_value.name_id == prepared.value_name &&
         route4.source_producer_kind ==
             expected_bir_route4_publication_source_kind(
                 prepared.source_producer_kind) &&
         route4_recorded_value &&
         route4_recorded_value.scope ==
             bir::Route4PublicationScope::CurrentBlock &&
         route4_recorded_value.value_role ==
             bir::Route4PublicationValueRole::Produced &&
         route4_recorded_value.value.value == prepared.produced_value &&
         route4_recorded_value.current_block.source_producer_instruction ==
             prepared.instruction &&
         bir.source_producer &&
         bir.instruction == prepared.instruction &&
         bir.produced_value == prepared.produced_value &&
         bir.produced_value_identity.value == prepared.produced_value &&
         bir.produced_value_name == prepared.produced_value->name &&
         bir.produced_value_type == prepared.produced_value->type &&
         bir.instruction_index == prepared.instruction_index &&
         bir.value_name == prepared.produced_value->name &&
         names.value_names.find(bir.value_name) == prepared.value_name &&
         bir.source_producer.inst == prepared.instruction &&
         bir.source_producer.instruction_index == prepared.instruction_index &&
         bir.source_producer.kind ==
             expected_bir_producer_kind(prepared.source_producer_kind) &&
         bir.source_producer_kind ==
             expected_bir_producer_kind(prepared.source_producer_kind);
}

bool prepared_and_bir_same_block_global_load_access_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedAddressingFunction& addressing,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    mir::SameBlockValueMaterializationQuery bir_query,
    const bir::Value& value) {
  const auto prepared_producer =
      prepare::find_prepared_same_block_scalar_producer(
          names,
          &source_producers,
          block_label,
          &block,
          value,
          bir_query.before_instruction_index);
  const auto prepared =
      prepared_producer.has_value()
          ? prepare::find_prepared_same_block_global_load_access(
                names, &addressing, *prepared_producer)
          : std::nullopt;
  const auto bir = mir::find_bir_same_block_global_load_access_identity(
      mir::BirSameBlockGlobalLoadAccessRequest{
          .block = &block,
          .block_label = bir_query.block_label,
          .root_value = &value,
          .root_value_name = value.kind == bir::Value::Kind::Named
                                 ? std::string_view(value.name)
                                 : std::string_view{},
          .root_value_type = value.type,
          .before_instruction_index = bir_query.before_instruction_index,
      });
  if (prepared.has_value() != static_cast<bool>(bir)) {
    return false;
  }
  if (!prepared.has_value()) {
    return true;
  }
  const auto* access = prepared->access;
  return access != nullptr &&
         bir.load_global == prepared->load_global &&
         bir.producer.inst == prepared_producer->instruction &&
         bir.producer.kind == mir::SameBlockProducerKind::LoadGlobal &&
         bir.producer.instruction_index == prepared_producer->instruction_index &&
         bir.result_value.name == value.name &&
         bir.result_value.type == value.type &&
         bir.memory_access.block_label == block.label &&
         bir.memory_access.instruction_index == prepared_producer->instruction_index &&
         bir.memory_access.node_kind == mir::BirMemoryAccessNodeKind::LoadGlobal &&
         bir.memory_access.result_value_name == value.name &&
         bir.memory_access.global_name_id ==
             access->address.symbol_name.value_or(c4c::kInvalidLinkName) &&
         bir.memory_access.address_space == access->address_space &&
         bir.memory_access.is_volatile == access->is_volatile &&
         bir.memory_access.base_kind == mir::BirMemoryAccessBaseKind::GlobalSymbol;
}

bool prepared_and_bir_same_block_load_local_source_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedStackLayout& stack_layout,
    const prepare::PreparedMemoryAccessLookups& memory_accesses,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    mir::SameBlockValueMaterializationQuery bir_query,
    const bir::Value& value) {
  const auto prepared =
      prepare::find_prepared_same_block_load_local_source_producer(
          names,
          stack_layout,
          &memory_accesses,
          &source_producers,
          block_label,
          &block,
          value,
          bir_query.before_instruction_index);
  const auto bir = mir::find_bir_same_block_load_local_source_identity(
      mir::BirSameBlockLoadLocalSourceRequest{
          .block = &block,
          .block_label = bir_query.block_label,
          .root_value = &value,
          .root_value_name = value.kind == bir::Value::Kind::Named
                                 ? std::string_view(value.name)
                                 : std::string_view{},
          .root_value_type = value.type,
          .before_instruction_index = bir_query.before_instruction_index,
      });
  if (prepared.has_value() != static_cast<bool>(bir)) {
    return false;
  }
  if (!prepared.has_value()) {
    return true;
  }
  const auto prepared_slot =
      prepared->source_access != nullptr
          ? prepared->source_access->address.frame_slot_id
          : std::optional<prepare::PreparedFrameSlotId>{};
  return prepared->producer != nullptr &&
         prepared->producer->load_local == bir.load_local &&
         prepared->source_access != nullptr &&
         bir.producer.inst == &block.insts[prepared->producer->instruction_index] &&
         bir.producer.kind == mir::SameBlockProducerKind::LoadLocal &&
         bir.producer.instruction_index ==
             prepared->producer->instruction_index &&
         bir.result_value.name == value.name &&
         bir.result_value.type == value.type &&
         bir.memory_access.block_label == block.label &&
         bir.memory_access.instruction_index ==
             prepared->producer->instruction_index &&
         bir.memory_access.node_kind == mir::BirMemoryAccessNodeKind::LoadLocal &&
         bir.memory_access.result_value_name == value.name &&
         bir.memory_access.base_kind == mir::BirMemoryAccessBaseKind::LocalSlot &&
         (!prepared_slot.has_value() ||
          bir.memory_access.local_slot_id ==
              static_cast<c4c::SlotNameId>(*prepared_slot));
}

bool prepared_and_bir_integer_constants_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    mir::SameBlockValueMaterializationQuery bir_query,
    const bir::Value& value,
    std::int64_t expected) {
  const auto prepared =
      prepare::evaluate_prepared_same_block_integer_constant(
          names, &source_producers, block_label, &block, value,
          bir_query.before_instruction_index);
  const auto bir = mir::evaluate_same_block_integer_constant(bir_query, value);
  return prepared.has_value() &&
         bir.has_value() &&
         *prepared == expected &&
         bir->value == *prepared;
}

bool prepared_and_bir_integer_constants_both_fail(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    mir::SameBlockValueMaterializationQuery bir_query,
    const bir::Value& value) {
  return !prepare::evaluate_prepared_same_block_integer_constant(
              names, &source_producers, block_label, &block, value,
              bir_query.before_instruction_index)
              .has_value() &&
         !mir::evaluate_same_block_integer_constant(bir_query, value).has_value();
}

bool prepared_and_route1_integer_constants_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value,
    std::int64_t expected) {
  const auto prepared =
      prepare::evaluate_prepared_same_block_integer_constant(
          names, &source_producers, block_label, &block, value,
          route1_query.before_instruction_index);
  const auto route1 =
      bir::route1_evaluate_same_block_integer_constant(route1_query, value);
  return prepared.has_value() &&
         route1.has_value() &&
         *prepared == expected &&
         route1->value == *prepared;
}

bool prepared_and_route1_integer_constants_both_fail(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value) {
  return !prepare::evaluate_prepared_same_block_integer_constant(
              names, &source_producers, block_label, &block, value,
              route1_query.before_instruction_index)
              .has_value() &&
         !bir::route1_evaluate_same_block_integer_constant(route1_query, value)
              .has_value();
}

bool prepared_and_bir_select_chain_answers_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto prepared_source =
      prepare::find_prepared_select_chain_source_producer(
          names, &source_producers, block_label, &block, value,
          before_instruction_index);
  const auto prepared_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names, &source_producers, block_label, &block, value,
          before_instruction_index);
  const auto prepared_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names, &source_producers, block_label, &block, value,
          before_instruction_index);
  const auto bir_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &value,
      .root_value_name = value.kind == bir::Value::Kind::Named
                             ? std::string_view(value.name)
                             : std::string_view{},
      .root_value_type = value.type,
      .before_instruction_index = before_instruction_index,
  };
  const auto bir_source =
      mir::find_bir_select_chain_source_producer(bir_request);
  const auto bir_dependency =
      mir::find_bir_select_chain_direct_global_dependency(bir_request);
  const auto bir_materialization =
      mir::find_bir_select_chain_scalar_materialization_eligibility(
          bir_request);
  const auto bir_identity = mir::find_bir_select_chain_identity(bir_request);

  if ((prepared_source != nullptr) != static_cast<bool>(bir_source)) {
    return false;
  }
  if (prepared_source == nullptr) {
    return !prepared_dependency.contains_direct_global_load &&
           !prepared_materialization.available &&
           !bir_dependency &&
           !bir_materialization &&
           !bir_identity;
  }
  return bir_identity &&
         bir_identity.root_producer.inst == bir_source.inst &&
         bir_identity.root_instruction_index == prepared_source->instruction_index &&
         bir_identity.root_is_select ==
             (prepared_source->kind ==
              prepare::PreparedEdgePublicationSourceProducerKind::
                  SelectMaterialization) &&
         bir_source.instruction_index == prepared_source->instruction_index &&
         bir_source.inst == &block.insts[prepared_source->instruction_index] &&
         bir_identity.root_value_name == value.name &&
         bir_identity.root_value.type == value.type &&
         prepared_dependency.contains_direct_global_load ==
             static_cast<bool>(bir_dependency) &&
         prepared_dependency.contains_direct_global_load ==
             static_cast<bool>(bir_identity.direct_global_dependency) &&
         (!prepared_dependency.contains_direct_global_load ||
          (prepared_dependency.root_is_select == bir_identity.root_is_select &&
           prepared_dependency.root_instruction_index ==
               bir_identity.root_instruction_index)) &&
         prepared_materialization.available == bir_materialization &&
         prepared_materialization.available ==
             bir_identity.scalar_materialization_available &&
         prepared_materialization.root_value_name ==
             names.value_names.find(value.name) &&
         prepared_materialization.root_is_select == bir_identity.root_is_select &&
         prepared_materialization.root_instruction_index ==
             bir_identity.root_instruction_index &&
         prepared_materialization.direct_global_dependency
                 .contains_direct_global_load ==
             static_cast<bool>(bir_identity.direct_global_dependency);
}

bool prepared_and_route2_select_chain_records_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationSourceProducerLookups& source_producers,
    c4c::BlockLabelId block_label,
    const bir::Block& block,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value) {
  const auto prepared_source =
      prepare::find_prepared_select_chain_source_producer(
          names, &source_producers, block_label, &block, value,
          route1_query.before_instruction_index);
  const auto prepared_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names, &source_producers, block_label, &block, value,
          route1_query.before_instruction_index);
  const auto prepared_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names, &source_producers, block_label, &block, value,
          route1_query.before_instruction_index);
  const auto route2 =
      bir::route2_select_chain_value_record(route1_query, value);

  if (prepared_source == nullptr) {
    return !route2 &&
           !prepared_dependency.contains_direct_global_load &&
           !prepared_materialization.available;
  }
  if (!route2 ||
      !route2.root_producer ||
      route2.root_producer.instruction !=
          &block.insts[prepared_source->instruction_index] ||
      route2.root_producer.instruction_index !=
          prepared_source->instruction_index ||
      route2.root_producer.kind !=
          expected_bir_route2_select_chain_producer_kind(
              prepared_source->kind) ||
      route2.root_producer.produced_value == nullptr ||
      route2.root_value.value != route2.root_producer.produced_value ||
      route2.root_value_name != value.name ||
      route2.root_value.type != value.type ||
      route2.root_is_select !=
          (prepared_source->kind ==
           prepare::PreparedEdgePublicationSourceProducerKind::
               SelectMaterialization) ||
      route2.root_instruction_index != prepared_source->instruction_index ||
      route2.scalar_materialization_available !=
          prepared_materialization.available ||
      route2.direct_global_dependency.available != true ||
      route2.direct_global_dependency.root_is_select != route2.root_is_select ||
      route2.direct_global_dependency.root_instruction_index !=
          route2.root_instruction_index ||
      route2.direct_global_dependency.contains_direct_global_load !=
          prepared_dependency.contains_direct_global_load) {
    return false;
  }
  if (!prepared_dependency.contains_direct_global_load) {
    return !route2.direct_global_dependency &&
           route2.direct_global_dependency.load_global == nullptr;
  }
  if (!route2.direct_global_dependency ||
      route2.direct_global_dependency.root_is_select !=
          prepared_dependency.root_is_select ||
      route2.direct_global_dependency.root_instruction_index !=
          prepared_dependency.root_instruction_index) {
    return false;
  }
  const auto* load_global =
      route2.direct_global_dependency.load_global;
  return load_global != nullptr &&
         route2.direct_global_dependency.direct_load_instruction_index <
             block.insts.size() &&
         std::get_if<bir::LoadGlobalInst>(
             &block.insts[route2.direct_global_dependency
                              .direct_load_instruction_index]) ==
             load_global &&
         route2.direct_global_dependency.global_name_id ==
             load_global->global_name_id &&
         route2.direct_global_dependency.global_name ==
             load_global->global_name;
}

bool route2_select_chain_index_matches_record_lookup(
    bir::Route2SelectChainValueQuery query,
    bir::Route1SameBlockProducerQuery route1_query,
    const bir::Value& value) {
  const auto expected = bir::route2_select_chain_value_record(route1_query, value);
  const auto* indexed = bir::route2_find_select_chain_value_record(query, value);
  if (!expected) {
    return indexed == nullptr;
  }
  return indexed != nullptr &&
         indexed->root_value.value == expected.root_value.value &&
         indexed->root_value_name == expected.root_value_name &&
         indexed->root_value.type == expected.root_value.type &&
         indexed->root_is_select == expected.root_is_select &&
         indexed->root_instruction_index == expected.root_instruction_index &&
         indexed->scalar_materialization_available ==
             expected.scalar_materialization_available &&
         indexed->root_producer.instruction == expected.root_producer.instruction &&
         indexed->root_producer.instruction_index ==
             expected.root_producer.instruction_index &&
         indexed->root_producer.kind == expected.root_producer.kind &&
         indexed->direct_global_dependency.available ==
             expected.direct_global_dependency.available &&
         indexed->direct_global_dependency.contains_direct_global_load ==
             expected.direct_global_dependency.contains_direct_global_load &&
         indexed->direct_global_dependency.root_is_select ==
             expected.direct_global_dependency.root_is_select &&
         indexed->direct_global_dependency.root_instruction_index ==
             expected.direct_global_dependency.root_instruction_index &&
         indexed->direct_global_dependency.load_global ==
             expected.direct_global_dependency.load_global &&
         indexed->direct_global_dependency.direct_load_instruction_index ==
             expected.direct_global_dependency.direct_load_instruction_index &&
         indexed->direct_global_dependency.global_name ==
             expected.direct_global_dependency.global_name &&
         indexed->direct_global_dependency.global_name_id ==
             expected.direct_global_dependency.global_name_id;
}

[[nodiscard]] mir::BirCfgEdgePublicationSourceRequest
make_bir_edge_publication_source_request(
    const bir::Block& predecessor_block,
    c4c::BlockLabelId predecessor_label,
    const bir::Block& successor_block,
    c4c::BlockLabelId successor_label,
    const bir::Value& destination_value,
    prepare::PreparedValueId destination_value_id,
    c4c::ValueNameId destination_value_name) {
  return mir::BirCfgEdgePublicationSourceRequest{
      .predecessor_block = &predecessor_block,
      .predecessor_label = predecessor_block.label,
      .predecessor_label_id = predecessor_label,
      .successor_block = &successor_block,
      .successor_label = successor_block.label,
      .successor_label_id = successor_label,
      .destination_value = &destination_value,
      .destination_value_id = destination_value_id,
      .destination_value_name = destination_value.name,
      .destination_value_name_id = destination_value_name,
      .destination_value_type = destination_value.type,
  };
}

[[nodiscard]] mir::BirCfgEdgePublicationSourceStatus
expected_bir_edge_status(prepare::PreparedEdgeCopySourceFactsStatus status) {
  switch (status) {
    case prepare::PreparedEdgeCopySourceFactsStatus::Available:
      return mir::BirCfgEdgePublicationSourceStatus::Available;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingPredecessorLabel:
      return mir::BirCfgEdgePublicationSourceStatus::MissingPredecessorLabel;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSuccessorLabel:
      return mir::BirCfgEdgePublicationSourceStatus::MissingSuccessorLabel;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingDestinationValue:
      return mir::BirCfgEdgePublicationSourceStatus::MissingDestinationValue;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingPublication:
      return mir::BirCfgEdgePublicationSourceStatus::MissingPublication;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceValue:
      return mir::BirCfgEdgePublicationSourceStatus::MissingSourceValue;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceProducer:
      return mir::BirCfgEdgePublicationSourceStatus::MissingSourceProducer;
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingPreparedLookups:
    case prepare::PreparedEdgeCopySourceFactsStatus::AmbiguousPublication:
    case prepare::PreparedEdgeCopySourceFactsStatus::PublicationUnavailable:
    case prepare::PreparedEdgeCopySourceFactsStatus::EdgeMismatch:
    case prepare::PreparedEdgeCopySourceFactsStatus::UnsupportedMove:
    case prepare::PreparedEdgeCopySourceFactsStatus::MoveEdgeMismatch:
    case prepare::PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceHome:
    case prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceMemoryAccess:
    case prepare::PreparedEdgeCopySourceFactsStatus::IncompleteSourceMemoryAccess:
      return mir::BirCfgEdgePublicationSourceStatus::MissingPublication;
  }
  return mir::BirCfgEdgePublicationSourceStatus::MissingPublication;
}

bool prepared_and_bir_cfg_edge_publication_source_identity_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedEdgePublicationLookups& edge_publications,
    c4c::BlockLabelId predecessor_label,
    c4c::BlockLabelId successor_label,
    prepare::PreparedValueId destination_value_id,
    mir::BirCfgEdgePublicationSourceRequest request) {
  const auto prepared = prepare::prepare_edge_copy_source_facts(
      &edge_publications, predecessor_label, successor_label, destination_value_id);
  const auto bir = mir::find_bir_cfg_edge_publication_source_identity(request);
  if (bir.status != expected_bir_edge_status(prepared.status)) {
    return false;
  }
  if (prepared.status != prepare::PreparedEdgeCopySourceFactsStatus::Available) {
    return !bir;
  }
  const auto expected_source_kind =
      expected_bir_producer_kind(prepared.source_producer_kind);
  const bool expected_memory_source =
      prepared.source_memory_access_status ==
      prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available;
  return bir &&
         bir.predecessor_label_id == prepared.predecessor_label &&
         bir.successor_label_id == prepared.successor_label &&
         bir.destination_value_id == prepared.destination_value_id &&
         bir.destination_value_name_id == prepared.destination_value_name &&
         bir.destination_value_name ==
             prepare::prepared_value_name(names, prepared.destination_value_name) &&
         bir.destination_value_type == prepared.destination_value.type &&
         bir.source_value_kind == prepared.source_value_kind &&
         bir.source_value_name ==
             prepare::prepared_value_name(names, prepared.source_value_name) &&
         bir.source_value_type == prepared.source_value.type &&
         bir.source_producer_kind == expected_source_kind &&
         bir.source_producer.kind == expected_source_kind &&
         bir.source_producer_block_label_id ==
             prepared.source_producer_block_label.value_or(c4c::kInvalidBlockLabel) &&
         bir.source_producer_instruction_index ==
             prepared.source_producer_instruction_index &&
         static_cast<bool>(bir.source_memory_access) == expected_memory_source &&
         (!expected_memory_source ||
          (bir.source_memory_access.node_kind ==
               mir::BirMemoryAccessNodeKind::LoadLocal &&
           bir.source_memory_access.result_value_name ==
               prepare::prepared_value_name(names, prepared.source_value_name) &&
           bir.source_memory_access.base_kind ==
               mir::BirMemoryAccessBaseKind::LocalSlot &&
           bir.source_memory_access.local_slot_id ==
               static_cast<c4c::SlotNameId>(
                   prepared.source_memory_frame_slot_id.value_or(
                       prepare::PreparedFrameSlotId{0})) &&
           bir.source_memory_access.address_space ==
               prepared.source_memory_address_space &&
           bir.source_memory_access.is_volatile ==
               prepared.source_memory_is_volatile));
}

[[nodiscard]] bool bir_value_identities_contain_name(
    const std::vector<mir::SameBlockValueIdentity>& values,
    std::string_view value_name) {
  return std::find_if(values.begin(), values.end(), [&](const auto& value) {
           return value.name == value_name;
         }) != values.end();
}

bool prepared_and_bir_current_block_join_source_identity_match(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedCurrentBlockJoinParallelCopySourceFacts& prepared,
    const mir::BirCurrentBlockJoinSourceIdentity& bir) {
  if (prepared.status !=
      prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::Available) {
    return !bir;
  }
  if (!bir ||
      bir.facts.empty()) {
    return false;
  }
  for (const auto& prepared_fact : prepared.facts) {
    if (prepared_fact.status !=
        prepare::PreparedEdgeCopySourceFactsStatus::Available) {
      continue;
    }
    const auto destination_name =
        prepare::prepared_value_name(names, prepared_fact.destination_value_name);
    const auto source_name =
        prepare::prepared_value_name(names, prepared_fact.source_value_name);
    const auto matches_fact =
        std::find_if(bir.facts.begin(), bir.facts.end(), [&](const auto& fact) {
          if (!fact ||
              fact.predecessor_label_id != prepared_fact.predecessor_label ||
              fact.successor_label_id != prepared_fact.successor_label ||
              fact.destination_value_name != destination_name) {
            return false;
          }
          if (prepared_fact.immediate_source) {
            return fact.source_value_kind == bir::Value::Kind::Immediate &&
                   fact.source_value_identity.immediate_constant.has_value();
          }
          return !source_name.empty() &&
                 fact.source_value_kind == bir::Value::Kind::Named &&
                 fact.source_value_name == source_name &&
                 fact.source_producer &&
                 fact.source_producer_kind != mir::SameBlockProducerKind::Unknown &&
                 fact.source_producer_instruction_index.has_value();
        }) != bir.facts.end();
    if (!matches_fact) {
      return false;
    }
  }
  for (const auto value_name : prepared.incoming_expression_value_names) {
    const auto name = prepare::prepared_value_name(names, value_name);
    if (!name.empty() &&
        !bir_value_identities_contain_name(bir.incoming_expression_values, name)) {
      return false;
    }
  }
  for (const auto value_name : prepared.source_value_names) {
    const auto name = prepare::prepared_value_name(names, value_name);
    if (!name.empty() &&
        !bir_value_identities_contain_name(bir.source_values, name)) {
      return false;
    }
  }
  return true;
}

int verify_prepared_home_same_register_helper() {
  const prepare::PreparedValueHome source_register{
      .value_id = 1,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"shared_register"},
  };
  const prepare::PreparedValueHome destination_register{
      .value_id = 2,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"shared_register"},
  };
  const prepare::PreparedValueHome different_register{
      .value_id = 3,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"other_register"},
  };
  const prepare::PreparedValueHome unnamed_register{
      .value_id = 4,
      .kind = prepare::PreparedValueHomeKind::Register,
  };
  const prepare::PreparedValueHome stack_home{
      .value_id = 5,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{1},
  };

  if (!prepare::prepared_value_homes_share_register_name(source_register,
                                                         destination_register)) {
    return fail("same-register prepared homes should be recognized by shared helper");
  }
  if (prepare::prepared_value_homes_share_register_name(source_register,
                                                        different_register)) {
    return fail("shared helper should reject different prepared register homes");
  }
  if (prepare::prepared_value_homes_share_register_name(source_register,
                                                        unnamed_register)) {
    return fail("shared helper should reject unnamed prepared register homes");
  }
  if (prepare::prepared_value_homes_share_register_name(source_register, stack_home)) {
    return fail("shared helper should reject non-register prepared homes");
  }

  return 0;
}

int verify_prepared_parallel_copy_register_move_helpers() {
  const prepare::PreparedValueHome source_register{
      .value_id = 1,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"shared_register"},
  };
  const prepare::PreparedValueHome destination_register{
      .value_id = 2,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"shared_register"},
  };
  const prepare::PreparedValueHome different_destination_register{
      .value_id = 3,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"other_register"},
  };
  const prepare::PreparedValueHome stack_source{
      .value_id = 1,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{4},
  };
  const prepare::PreparedMoveResolution register_move{
      .from_value_id = 1,
      .to_value_id = 2,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
  };

  if (!prepare::prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
          register_move, prepare::PreparedValueId{2})) {
    return fail("parallel-copy helper should identify register destination values");
  }
  if (prepare::prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
          register_move, prepare::PreparedValueId{1})) {
    return fail("parallel-copy helper should reject non-destination values");
  }
  if (!prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
          register_move, source_register, destination_register)) {
    return fail("parallel-copy helper should identify exact shared-register sources");
  }
  if (prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
          register_move, source_register, different_destination_register)) {
    return fail("parallel-copy helper should reject different destination homes");
  }
  if (prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
          register_move, stack_source, destination_register)) {
    return fail("parallel-copy helper should leave stack-source policy to targets");
  }

  auto stack_destination_move = register_move;
  stack_destination_move.destination_storage_kind =
      prepare::PreparedMoveStorageKind::StackSlot;
  if (prepare::prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
          stack_destination_move, prepare::PreparedValueId{2})) {
    return fail("parallel-copy helper should reject non-register destinations");
  }

  auto temp_save_move = register_move;
  temp_save_move.op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  if (prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
          temp_save_move, source_register, destination_register)) {
    return fail("parallel-copy helper should reject temp-save steps");
  }

  auto immediate_move = register_move;
  immediate_move.source_immediate_i32 = std::int64_t{7};
  if (prepare::prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
          immediate_move, source_register, destination_register)) {
    return fail("parallel-copy helper should reject immediate sources");
  }

  return 0;
}

int verify_linear_function_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_id = prepared.names.function_names.intern("linear");
  const auto other_function_id = prepared.names.function_names.intern("diamond");
  const auto entry_label = prepared.names.block_labels.intern("linear.entry");
  const auto exit_label = prepared.names.block_labels.intern("linear.exit");
  const auto value_name = prepared.names.value_names.intern("%linear.value");
  const auto other_value_name = prepared.names.value_names.intern("%diamond.value");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_id,
      .blocks = {
          branch_block(entry_label, exit_label),
          return_block(exit_label),
      },
  };

  prepare::PreparedCallPlansFunction calls{
      .function_name = function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 0,
              .instruction_index = 2,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("callee_a"),
              .outgoing_stack_argument_area =
                  prepare::PreparedOutgoingStackArgumentArea{.size_bytes = 64},
              .arguments = {
                  prepare::PreparedCallArgumentPlan{
                      .instruction_index = 2,
                      .arg_index = 0,
                      .destination_stack_offset_bytes = std::size_t{16},
                      .destination_stack_size_bytes = std::size_t{8},
                  },
              },
              .preserved_values = {
                  prepare::PreparedCallPreservedValue{
                      .value_id = 4,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
                      .contiguous_width = 1,
                      .register_name = std::string("x19"),
                      .register_bank = prepare::PreparedRegisterBank::Gpr,
                      .occupied_register_names = {"x19"},
                      .register_placement = prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                          .slot_index = 0,
                          .contiguous_width = 1,
                      },
                  },
                  prepare::PreparedCallPreservedValue{
                      .value_id = 9,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{3},
                      .stack_offset_bytes = std::size_t{24},
                      .stack_size_bytes = std::size_t{8},
                      .stack_align_bytes = std::size_t{8},
                  },
                  prepare::PreparedCallPreservedValue{
                      .value_id = 9,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::StackSlot,
                      .slot_id = prepare::PreparedFrameSlotId{4},
                      .stack_offset_bytes = std::size_t{32},
                      .stack_size_bytes = std::size_t{8},
                      .stack_align_bytes = std::size_t{8},
                  },
                  prepare::PreparedCallPreservedValue{
                      .value_id = 10,
                      .value_name = value_name,
                      .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
                  },
              },
          },
          prepare::PreparedCallPlan{
              .block_index = 1,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
              .direct_callee_name = std::string("callee_b"),
          },
      },
  };
  prepared.call_plans.functions.push_back(calls);
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = other_function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 0,
              .instruction_index = 2,
              .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
          },
      },
  });

  prepare::PreparedAddressingFunction addressing{
      .function_name = function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = entry_label,
              .inst_index = 1,
              .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = entry_label,
              .inst_index = 3,
              .kind = prepare::PreparedAddressMaterializationKind::DirectGlobal,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = exit_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::Label,
          },
      },
  };
  prepared.addressing.functions.push_back(addressing);
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = other_function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = other_function_id,
              .block_label = entry_label,
              .inst_index = 9,
              .kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          },
      },
  });

  prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = function_id,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("r10"),
          },
          prepare::PreparedValueHome{
              .value_id = 8,
              .function_name = function_id,
              .value_name = c4c::kInvalidValueName,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 1,
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BeforeInstruction,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 0,
              .instruction_index = 2,
          },
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::AfterCall,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 1,
              .instruction_index = 0,
          },
      },
  };
  prepared.value_locations.functions.push_back(value_locations);
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = other_function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = other_function_id,
              .value_name = other_value_name,
              .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
              .immediate_i32 = 7,
          },
      },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* call_plans = prepare::find_prepared_call_plans(prepared, function_id);
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_id);
  if (call_plans == nullptr || function_locations == nullptr) {
    return fail("fixture lookup failed for linear function");
  }

  if (!expect_same(prepare::find_indexed_prepared_call_plan(
                       &lookups.call_plans, call_plans, 0, 2),
                   &call_plans->calls[0],
                   "indexed call-plan lookup missed the first linear call")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_call_plan(
                       &lookups.call_plans, call_plans, 1, 0),
                   &call_plans->calls[1],
                   "indexed call-plan lookup missed the second linear call")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_call_plan(&lookups.call_plans, call_plans, 0, 9) !=
      nullptr) {
    return fail("indexed call-plan lookup returned an unmatched linear call");
  }
  const auto* outgoing_area =
      prepare::find_indexed_prepared_outgoing_stack_argument_area(
          lookups.call_plans, call_plans, 0, 2);
  if (outgoing_area != &*call_plans->calls[0].outgoing_stack_argument_area ||
      outgoing_area->size_bytes != 64) {
    return fail("indexed outgoing stack argument area did not preserve call-level area");
  }
  if (outgoing_area->size_bytes ==
      *call_plans->calls[0].arguments[0].destination_stack_size_bytes) {
    return fail("indexed outgoing stack argument area was recomputed from one stack lane");
  }
  if (prepare::find_indexed_prepared_outgoing_stack_argument_area(
          lookups.call_plans, call_plans, 1, 0) != nullptr) {
    return fail("indexed outgoing stack argument area fabricated a missing area");
  }
  const prepare::PreparedCallPlan same_block_current{
      .block_index = 0,
      .instruction_index = 4,
  };
  const auto same_block_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, same_block_current, prepare::PreparedValueId{4});
  if (same_block_prior.status !=
          prepare::PreparedPriorPreservedValueLookupStatus::Found ||
      same_block_prior.preserved != &call_plans->calls[0].preserved_values[0] ||
      same_block_prior.entry == nullptr ||
      same_block_prior.entry->block_index != 0 ||
      same_block_prior.entry->instruction_index != 2) {
    return fail("unique prior-preservation lookup missed same-block indexed source");
  }
  const auto cross_block_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, call_plans->calls[1],
          prepare::PreparedValueId{4});
  if (cross_block_prior.status !=
          prepare::PreparedPriorPreservedValueLookupStatus::Found ||
      cross_block_prior.preserved != &call_plans->calls[0].preserved_values[0] ||
      cross_block_prior.entry == nullptr ||
      cross_block_prior.entry->block_index != 0 ||
      cross_block_prior.entry->instruction_index != 2) {
    return fail("unique prior-preservation lookup missed cross-block dominating source");
  }
  const auto no_prior = prepare::find_unique_indexed_prior_preserved_value_source(
      lookups.call_plans, &control_flow, same_block_current, prepare::PreparedValueId{88});
  if (no_prior.status != prepare::PreparedPriorPreservedValueLookupStatus::NotFound ||
      no_prior.preserved != nullptr || no_prior.entry != nullptr) {
    return fail("unique prior-preservation lookup did not explicitly reject no-source case");
  }
  const auto ambiguous_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, call_plans->calls[1],
          prepare::PreparedValueId{9});
  if (ambiguous_prior.status !=
      prepare::PreparedPriorPreservedValueLookupStatus::Ambiguous) {
    return fail("unique prior-preservation lookup did not explicitly reject ambiguity");
  }
  const auto invalid_prior =
      prepare::find_unique_indexed_prior_preserved_value_source(
          lookups.call_plans, &control_flow, call_plans->calls[1],
          prepare::PreparedValueId{10});
  if (invalid_prior.status !=
          prepare::PreparedPriorPreservedValueLookupStatus::InvalidPreservation ||
      invalid_prior.preserved != &call_plans->calls[0].preserved_values[3]) {
    return fail("unique prior-preservation lookup did not explicitly reject incomplete source");
  }

  const auto* entry_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, entry_label);
  if (entry_materializations == nullptr || entry_materializations->size() != 2 ||
      (*entry_materializations)[0] != &prepared.addressing.functions[0].address_materializations[0] ||
      (*entry_materializations)[1] != &prepared.addressing.functions[0].address_materializations[1]) {
    return fail("indexed address-materialization lookup did not preserve block grouping");
  }
  const auto* exit_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, exit_label);
  if (exit_materializations == nullptr || exit_materializations->size() != 1 ||
      (*exit_materializations)[0] != &prepared.addressing.functions[0].address_materializations[2]) {
    return fail("indexed address-materialization lookup missed exit block materialization");
  }

  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BeforeInstruction,
                       0,
                       2),
                   &function_locations->move_bundles[0],
                   "indexed move-bundle lookup missed the before-instruction bundle")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::AfterCall,
                       1,
                       0),
                   &function_locations->move_bundles[1],
                   "indexed move-bundle lookup missed the after-call bundle")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_move_bundle(&lookups.move_bundles,
                                                 function_locations,
                                                 prepare::PreparedMovePhase::BeforeCall,
                                                 0,
                                                 2) != nullptr) {
    return fail("indexed move-bundle lookup ignored move phase");
  }

  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, function_locations, prepare::PreparedValueId{4}),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed the value-id home")) {
    return 1;
  }
  const auto value_id = prepare::find_indexed_prepared_value_id(
      &lookups.value_homes, nullptr, function_locations, value_name);
  if (!value_id.has_value() || *value_id != 4) {
    return fail("indexed value-id lookup missed the value-name mapping");
  }
  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, nullptr, function_locations, value_name),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed the value-name home")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_value_id(
          &lookups.value_homes, nullptr, function_locations, other_value_name)
      .has_value()) {
    return fail("indexed value-id lookup leaked another function's value home");
  }

  return 0;
}

int verify_diamond_function_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_id = prepared.names.function_names.intern("diamond");
  const auto entry_label = prepared.names.block_labels.intern("diamond.entry");
  const auto left_label = prepared.names.block_labels.intern("diamond.left");
  const auto right_label = prepared.names.block_labels.intern("diamond.right");
  const auto join_label = prepared.names.block_labels.intern("diamond.join");
  const auto value_name = prepared.names.value_names.intern("%diamond.value");
  const auto left_source_name = prepared.names.value_names.intern("%diamond.left");
  const auto right_source_name = prepared.names.value_names.intern("%diamond.right");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_id,
      .blocks = {
          cond_branch_block(entry_label, left_label, right_label),
          branch_block(left_label, join_label),
          branch_block(right_label, join_label),
          return_block(join_label),
      },
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_id,
              .join_block_label = join_label,
              .result = bir::Value::named(bir::TypeKind::I32, "%diamond.value"),
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .carrier_kind =
                  prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = left_label,
                      .successor_label = join_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.left"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.value"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = right_label,
                      .successor_label = join_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.right"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%diamond.value"),
                  },
              },
          },
      },
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = left_label,
              .successor_label = join_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          },
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = right_label,
              .successor_label = join_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          },
      },
  };

  prepare::PreparedCallPlansFunction calls{
      .function_name = function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 1,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("left_call"),
          },
          prepare::PreparedCallPlan{
              .block_index = 2,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("right_call"),
          },
          prepare::PreparedCallPlan{
              .block_index = 3,
              .instruction_index = 1,
              .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternVariadic,
              .direct_callee_name = std::string("join_call"),
          },
      },
  };
  prepared.call_plans.functions.push_back(calls);

  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = left_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::StringConstant,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = right_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::TlsGlobal,
          },
      },
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 11,
              .function_name = function_id,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset,
              .pointer_byte_delta = 16,
          },
          prepare::PreparedValueHome{
              .value_id = 12,
              .function_name = function_id,
              .value_name = left_source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("left_source"),
          },
          prepare::PreparedValueHome{
              .value_id = 13,
              .function_name = function_id,
              .value_name = right_source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("right_source"),
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 3,
              .instruction_index = 0,
              .source_parallel_copy_predecessor_label = left_label,
              .source_parallel_copy_successor_label = join_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = 12,
                      .to_value_id = 11,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_stack_offset_bytes = std::size_t{16},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 3,
              .instruction_index = 1,
          },
      },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* call_plans = prepare::find_prepared_call_plans(prepared, function_id);
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_id);
  if (call_plans == nullptr || function_locations == nullptr) {
    return fail("fixture lookup failed for diamond function");
  }

  for (std::size_t index = 0; index < call_plans->calls.size(); ++index) {
    const auto& call = call_plans->calls[index];
    if (!expect_same(prepare::find_indexed_prepared_call_plan(
                         &lookups.call_plans,
                         call_plans,
                         call.block_index,
                         call.instruction_index),
                     &call_plans->calls[index],
                     "indexed call-plan lookup missed a diamond call")) {
      return 1;
    }
  }

  const auto* left_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, left_label);
  const auto* right_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, right_label);
  if (left_materializations == nullptr || left_materializations->size() != 1 ||
      (*left_materializations)[0] != &prepared.addressing.functions[0].address_materializations[0]) {
    return fail("indexed address-materialization lookup missed diamond left block");
  }
  if (right_materializations == nullptr || right_materializations->size() != 1 ||
      (*right_materializations)[0] != &prepared.addressing.functions[0].address_materializations[1]) {
    return fail("indexed address-materialization lookup missed diamond right block");
  }
  if (prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, join_label) != nullptr) {
    return fail("indexed address-materialization lookup returned an empty diamond join block");
  }

  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BlockEntry,
                       3,
                       0),
                   &function_locations->move_bundles[0],
                   "indexed move-bundle lookup missed diamond block-entry bundle")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BeforeCall,
                       3,
                       1),
                   &function_locations->move_bundles[1],
                   "indexed move-bundle lookup missed diamond before-call bundle")) {
    return 1;
  }

  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, function_locations, prepare::PreparedValueId{11}),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed diamond home")) {
    return 1;
  }
  const auto value_id = prepare::find_indexed_prepared_value_id(
      &lookups.value_homes, nullptr, function_locations, value_name);
  if (!value_id.has_value() || *value_id != 11) {
    return fail("indexed value-id lookup missed diamond value-name mapping");
  }

  const auto* left_publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, left_label, join_label, prepare::PreparedValueId{11});
  if (left_publication == nullptr ||
      left_publication->status !=
          prepare::PreparedEdgePublicationLookupStatus::Available ||
      left_publication->predecessor_label != left_label ||
      left_publication->successor_label != join_label ||
      left_publication->destination_value_id != 11 ||
      left_publication->destination_value_name != value_name ||
      !left_publication->source_value_id.has_value() ||
      *left_publication->source_value_id != 12 ||
      left_publication->source_value_name != left_source_name ||
      left_publication->destination_home != &function_locations->value_homes[0] ||
      left_publication->destination_home_kind !=
          prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      left_publication->destination_storage_kind !=
          prepare::PreparedMoveStorageKind::StackSlot ||
      left_publication->phase != prepare::PreparedMovePhase::BlockEntry ||
      left_publication->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      left_publication->join_transfer != &control_flow.join_transfers[0] ||
      left_publication->edge_transfer !=
          &control_flow.join_transfers[0].edge_transfers[0] ||
      left_publication->parallel_copy_bundle !=
          &control_flow.parallel_copy_bundles[0] ||
      left_publication->move_bundle != &function_locations->move_bundles[0] ||
      left_publication->move != &function_locations->move_bundles[0].moves[0]) {
    return fail("edge-publication lookup did not preserve prepared join, source, home, carrier, and move facts");
  }

  const auto* right_publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups.edge_publications, right_label, join_label, prepare::PreparedValueId{11});
  if (right_publication == nullptr ||
      right_publication->source_value_id != prepare::PreparedValueId{13} ||
      right_publication->parallel_copy_bundle !=
          &control_flow.parallel_copy_bundles[1] ||
      right_publication->move != nullptr ||
      right_publication->destination_storage_kind != prepare::PreparedMoveStorageKind::None) {
    return fail("edge-publication lookup should link the published bundle while leaving missing move data absent");
  }

  return 0;
}

int verify_edge_publication_lookup_key_preserves_full_tuple() {
  const auto successor_destination_a = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{10},
      c4c::BlockLabelId{20},
      prepare::PreparedValueId{(std::size_t{1} << 16U) ^ 100U});
  const auto successor_destination_b = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{10},
      c4c::BlockLabelId{21},
      prepare::PreparedValueId{100});
  if (successor_destination_a == successor_destination_b) {
    return fail("edge-publication key collapsed overlapping successor/value-id bits");
  }

  const auto predecessor_successor_a = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{42}, c4c::BlockLabelId{5}, prepare::PreparedValueId{7});
  const auto predecessor_successor_b = prepare::prepared_edge_publication_key(
      c4c::BlockLabelId{43},
      static_cast<c4c::BlockLabelId>((std::size_t{1} << 24U) + 5U),
      prepare::PreparedValueId{7});
  if (predecessor_successor_a == predecessor_successor_b) {
    return fail("edge-publication key collapsed overlapping predecessor/successor bits");
  }

  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("edge_key_growth");
  const auto source_name = names.value_names.intern("%edge_key.source");
  const auto high_destination_name = names.value_names.intern("%edge_key.high_destination");
  const auto low_destination_name = names.value_names.intern("%edge_key.low_destination");

  const c4c::BlockLabelId predecessor_label{10};
  const c4c::BlockLabelId first_successor_label{20};
  const c4c::BlockLabelId second_successor_label{21};
  const prepare::PreparedValueId source_value_id{50};
  const prepare::PreparedValueId high_destination_value_id{(std::size_t{1} << 16U) ^
                                                           100U};
  const prepare::PreparedValueId low_destination_value_id{100};

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = first_successor_label,
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = first_successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_key.source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_key.high_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = second_successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_key.source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_key.low_destination"),
                  },
              },
          },
      },
  };

  const prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = source_value_id,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"source_home"},
          },
          prepare::PreparedValueHome{
              .value_id = high_destination_value_id,
              .function_name = function_name,
              .value_name = high_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"high_home"},
          },
          prepare::PreparedValueHome{
              .value_id = low_destination_value_id,
              .function_name = function_name,
              .value_name = low_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"low_home"},
          },
      },
  };

  const auto lookups = prepare::make_prepared_edge_publication_lookups(
      names, control_flow, &locations);
  const auto* high_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups,
          predecessor_label,
          first_successor_label,
          high_destination_value_id);
  const auto* low_publication = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, second_successor_label, low_destination_value_id);

  if (high_publication == nullptr || low_publication == nullptr) {
    return fail("edge-publication lookup should preserve distinct high-id tuples");
  }
  if (high_publication == low_publication ||
      high_publication->successor_label != first_successor_label ||
      high_publication->destination_value_id != high_destination_value_id ||
      low_publication->successor_label != second_successor_label ||
      low_publication->destination_value_id != low_destination_value_id) {
    return fail("edge-publication lookup returned a colliding edge/destination record");
  }
  if (prepare::find_unique_indexed_prepared_edge_publication(
          &lookups, predecessor_label, first_successor_label, low_destination_value_id) !=
      nullptr) {
    return fail("edge-publication lookup should not cross-match successor/value-id tuples");
  }

  return 0;
}

int verify_edge_publication_shared_source_and_parallel_copy_facts() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("edge_shared_facts");
  const auto predecessor_label = names.block_labels.intern("edge_shared_facts.pred");
  const auto successor_label = names.block_labels.intern("edge_shared_facts.succ");
  const auto execution_label = names.block_labels.intern("edge_shared_facts.edge");
  const auto source_name = names.value_names.intern("%edge_shared.source");
  const auto missing_source_name =
      names.value_names.intern("%edge_shared.missing_source");
  const auto named_destination_name =
      names.value_names.intern("%edge_shared.named_destination");
  const auto immediate_destination_name =
      names.value_names.intern("%edge_shared.immediate_destination");
  const auto missing_destination_name =
      names.value_names.intern("%edge_shared.missing_destination");
  const auto same_value_name = names.value_names.intern("%edge_shared.same");
  const auto cycle_source_name = names.value_names.intern("%edge_shared.cycle_source");
  const auto cycle_destination_name =
      names.value_names.intern("%edge_shared.cycle_destination");
  const auto stack_source_name = names.value_names.intern("%edge_shared.stack_source");
  const auto stack_destination_name =
      names.value_names.intern("%edge_shared.stack_destination");

  const prepare::PreparedValueId source_id{41};
  const prepare::PreparedValueId named_destination_id{42};
  const prepare::PreparedValueId immediate_destination_id{43};
  const prepare::PreparedValueId missing_destination_id{44};
  const prepare::PreparedValueId same_value_id{45};
  const prepare::PreparedValueId cycle_source_id{46};
  const prepare::PreparedValueId cycle_destination_id{47};
  const prepare::PreparedValueId stack_source_id{48};
  const prepare::PreparedValueId stack_destination_id{49};

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_shared.source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.named_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value = bir::Value::immediate_i32(7),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.immediate_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.missing_source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.missing_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_shared.same"),
                      .destination_value =
                          bir::Value::named(bir::TypeKind::I32, "%edge_shared.same"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.cycle_source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.cycle_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.stack_source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%edge_shared.stack_destination"),
                  },
              },
          },
      },
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor_label,
              .successor_label = successor_label,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::CriticalEdge,
              .execution_block_label = execution_label,
              .steps = {
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 0,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 1,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 2,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 3,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind =
                          prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp,
                      .move_index = 4,
                      .uses_cycle_temp_source = true,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 5,
                  },
              },
              .has_cycle = true,
          },
      },
  };

  const prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = source_id,
              .function_name = function_name,
              .value_name = source_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"source_home"},
          },
          prepare::PreparedValueHome{
              .value_id = named_destination_id,
              .function_name = function_name,
              .value_name = named_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"named_destination_home"},
          },
          prepare::PreparedValueHome{
              .value_id = immediate_destination_id,
              .function_name = function_name,
              .value_name = immediate_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"immediate_destination_home"},
          },
          prepare::PreparedValueHome{
              .value_id = missing_destination_id,
              .function_name = function_name,
              .value_name = missing_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"missing_destination_home"},
          },
          prepare::PreparedValueHome{
              .value_id = same_value_id,
              .function_name = function_name,
              .value_name = same_value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"same_home"},
          },
          prepare::PreparedValueHome{
              .value_id = cycle_source_id,
              .function_name = function_name,
              .value_name = cycle_source_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{8},
          },
          prepare::PreparedValueHome{
              .value_id = cycle_destination_id,
              .function_name = function_name,
              .value_name = cycle_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"cycle_destination_home"},
          },
          prepare::PreparedValueHome{
              .value_id = stack_source_id,
              .function_name = function_name,
              .value_name = stack_source_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{12},
              .offset_bytes = std::size_t{16},
              .size_bytes = std::size_t{4},
          },
          prepare::PreparedValueHome{
              .value_id = stack_destination_id,
              .function_name = function_name,
              .value_name = stack_destination_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{13},
              .offset_bytes = std::size_t{24},
              .size_bytes = std::size_t{4},
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 9,
              .source_parallel_copy_predecessor_label = predecessor_label,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = source_id,
                      .to_value_id = named_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .source_parallel_copy_step_index = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .to_value_id = immediate_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .source_parallel_copy_step_index = std::size_t{1},
                      .source_immediate_i32 = std::int64_t{7},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = 999,
                      .to_value_id = missing_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .source_parallel_copy_step_index = std::size_t{2},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = same_value_id,
                      .to_value_id = same_value_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .coalesced_by_assigned_storage = true,
                      .source_parallel_copy_step_index = std::size_t{3},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = cycle_source_id,
                      .to_value_id = cycle_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .uses_cycle_temp_source = true,
                      .source_parallel_copy_step_index = std::size_t{4},
                      .op_kind =
                          prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = stack_source_id,
                      .to_value_id = stack_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_stack_offset_bytes = std::size_t{24},
                      .source_parallel_copy_step_index = std::size_t{5},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
      },
  };

  const auto lookups = prepare::make_prepared_edge_publication_lookups(
      names, control_flow, &locations);
  const auto* named = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, named_destination_id);
  const auto* immediate = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, immediate_destination_id);
  const auto* missing = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, missing_destination_id);
  const auto* same = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, same_value_id);
  const auto* cycle = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, cycle_destination_id);
  const auto* stack = prepare::find_unique_indexed_prepared_edge_publication(
      &lookups, predecessor_label, successor_label, stack_destination_id);

  if (named == nullptr || named->source_value_kind != bir::Value::Kind::Named ||
      named->source_value_id != source_id ||
      named->source_home != &locations.value_homes[0] ||
      named->source_home_kind != prepare::PreparedValueHomeKind::Register) {
    return fail("edge publication should preserve named source home facts");
  }
  const auto* named_move = &locations.move_bundles.front().moves[0];
  if (prepare::find_unique_indexed_block_entry_parallel_copy_edge_publication(
          &lookups, predecessor_label, successor_label, *named_move) != named) {
    return fail(
        "shared block-entry parallel-copy lookup should find the named edge publication");
  }
  if (prepare::find_unique_indexed_block_entry_parallel_copy_edge_publication(
          &lookups, c4c::kInvalidBlockLabel, successor_label, *named_move) != nullptr) {
    return fail(
        "shared block-entry parallel-copy lookup should reject invalid predecessor labels");
  }
  auto non_value_destination_move = *named_move;
  non_value_destination_move.destination_kind =
      prepare::PreparedMoveDestinationKind::CallArgumentAbi;
  if (prepare::find_unique_indexed_block_entry_parallel_copy_edge_publication(
          &lookups, predecessor_label, successor_label, non_value_destination_move) != nullptr) {
    return fail("shared block-entry parallel-copy lookup should reject non-value destinations");
  }
  auto mismatched_edge_move = *named_move;
  mismatched_edge_move.source_parallel_copy_predecessor_label = successor_label;
  if (prepare::find_unique_indexed_block_entry_parallel_copy_edge_publication(
          &lookups, predecessor_label, successor_label, mismatched_edge_move) != nullptr) {
    return fail("shared block-entry parallel-copy lookup should reject mismatched move labels");
  }
  if (!prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          *named, *named_move, locations.value_homes[0])) {
    return fail("edge publication should match its exact prepared move source");
  }
  auto copied_move = *named_move;
  if (prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          *named, copied_move, locations.value_homes[0])) {
    return fail("edge publication source helper should require exact move identity");
  }
  if (prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          *named, *named_move, locations.value_homes[1])) {
    return fail("edge publication source helper should require exact source home identity");
  }
  auto mismatched_source_publication = *named;
  mismatched_source_publication.source_value_id = prepare::PreparedValueId{999};
  if (prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          mismatched_source_publication, *named_move, locations.value_homes[0])) {
    return fail("edge publication source helper should require matching source id facts");
  }
  auto publication_without_step = *named;
  auto move_without_step = *named_move;
  move_without_step.authority_kind = prepare::PreparedMoveAuthorityKind::None;
  move_without_step.source_parallel_copy_step_index.reset();
  publication_without_step.move = &move_without_step;
  publication_without_step.parallel_copy_step_index.reset();
  if (!prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          publication_without_step, move_without_step, locations.value_homes[0])) {
    return fail("edge publication source helper should not require optional step facts");
  }
  if (immediate == nullptr ||
      immediate->source_value_kind != bir::Value::Kind::Immediate ||
      immediate->source_value_id.has_value() || immediate->source_home != nullptr ||
      immediate->source_home_kind != prepare::PreparedValueHomeKind::None) {
    return fail("edge publication should classify immediate sources without fabricating homes");
  }
  if (missing == nullptr || missing->source_value_kind != bir::Value::Kind::Named ||
      missing->source_value_name != missing_source_name ||
      missing->source_value_id.has_value() || missing->source_home != nullptr) {
    return fail("edge publication should classify named sources even when no home exists");
  }
  if (same == nullptr || !same->source_and_destination_same_value_id ||
      !same->matching_move_coalesced_by_assigned_storage ||
      !same->matching_move_redundant_by_assigned_storage) {
    return fail("edge publication should expose same-value and coalesced move facts");
  }
  const auto* same_move = &locations.move_bundles.front().moves[3];
  if (!prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move(
          *same, same_move)) {
    return fail("edge publication should classify its redundant parallel-copy move");
  }
  if (prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move(
          *named, &locations.move_bundles.front().moves[0])) {
    return fail("edge publication should not classify non-redundant moves as redundant");
  }
  auto mismatched_step_publication = *same;
  mismatched_step_publication.parallel_copy_step_index = std::size_t{4};
  if (prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move(
          mismatched_step_publication, same_move)) {
    return fail("edge publication should require the matching parallel-copy step index");
  }
  if (cycle == nullptr || cycle->parallel_copy_step_index != std::size_t{4} ||
      cycle->parallel_copy_step_kind !=
          prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp ||
      !cycle->parallel_copy_step_uses_cycle_temp_source ||
      !cycle->parallel_copy_bundle_has_cycle ||
      cycle->parallel_copy_execution_site !=
          prepare::PreparedParallelCopyExecutionSite::CriticalEdge ||
      cycle->parallel_copy_execution_block_label != execution_label) {
    return fail("edge publication should expose cycle/temp-save parallel-copy ordering facts");
  }
  const auto* stack_move = &locations.move_bundles.front().moves[5];
  if (stack == nullptr || stack->source_value_kind != bir::Value::Kind::Named ||
      stack->source_value_id != stack_source_id ||
      stack->source_home != &locations.value_homes[7] ||
      stack->source_home_kind != prepare::PreparedValueHomeKind::StackSlot ||
      stack->destination_value_id != stack_destination_id ||
      stack->destination_home != &locations.value_homes[8] ||
      stack->destination_home_kind != prepare::PreparedValueHomeKind::StackSlot ||
      stack->destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      stack->move != stack_move ||
      stack->parallel_copy_step_index != std::size_t{5}) {
    return fail("edge publication should preserve stack source and stack destination facts");
  }
  if (!prepare::prepared_edge_publication_matches_parallel_copy_move_source(
          *stack, *stack_move, locations.value_homes[7])) {
    return fail("edge publication should match its stack-source prepared move");
  }
  const auto named_facts = prepare::prepare_edge_copy_source_facts(
      &lookups, predecessor_label, successor_label, named_destination_id);
  if (named_facts.status != prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      named_facts.publication != named ||
      named_facts.source_value_id != source_id ||
      named_facts.source_home != &locations.value_homes[0] ||
      named_facts.destination_home != &locations.value_homes[1] ||
      named_facts.source_memory_access_status !=
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Unavailable) {
    return fail("edge source fact query should expose complete named source facts");
  }
  const auto immediate_facts = prepare::prepare_block_entry_parallel_copy_edge_source_facts(
      &lookups,
      predecessor_label,
      successor_label,
      locations.move_bundles.front().moves[1]);
  if (immediate_facts.status !=
          prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      immediate_facts.publication != immediate ||
      immediate_facts.source_value_kind != bir::Value::Kind::Immediate ||
      immediate_facts.source_home != nullptr ||
      immediate_facts.move != &locations.move_bundles.front().moves[1]) {
    return fail("edge source fact query should expose complete immediate move facts");
  }
  const auto missing_home_facts = prepare::prepare_edge_copy_source_facts(
      &lookups, predecessor_label, successor_label, missing_destination_id);
  if (missing_home_facts.status !=
          prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceValue ||
      missing_home_facts.publication != missing ||
      missing_home_facts.source_value_name != missing_source_name) {
    return fail("edge source fact query should fail closed for missing source values");
  }
  auto facts_mismatched_edge_move = locations.move_bundles.front().moves[0];
  facts_mismatched_edge_move.source_parallel_copy_successor_label = predecessor_label;
  if (prepare::prepare_block_entry_parallel_copy_edge_source_facts(
          &lookups, predecessor_label, successor_label, facts_mismatched_edge_move)
          .status != prepare::PreparedEdgeCopySourceFactsStatus::MoveEdgeMismatch) {
    return fail("edge source fact query should diagnose mismatched move edge labels");
  }
  auto copied_named_move = locations.move_bundles.front().moves[0];
  if (prepare::prepare_block_entry_parallel_copy_edge_source_facts(
          &lookups, predecessor_label, successor_label, copied_named_move)
          .status !=
      prepare::PreparedEdgeCopySourceFactsStatus::PublicationMoveMismatch) {
    return fail("edge source fact query should require exact prepared move identity");
  }
  prepare::PreparedEdgePublication unavailable_publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::MissingDestinationHome,
      .predecessor_label = predecessor_label,
      .successor_label = successor_label,
      .destination_value_id = prepare::PreparedValueId{900},
  };
  prepare::PreparedEdgePublication missing_source_home_publication = *named;
  missing_source_home_publication.destination_value_id = prepare::PreparedValueId{902};
  missing_source_home_publication.source_home = nullptr;
  missing_source_home_publication.source_home_kind =
      prepare::PreparedValueHomeKind::None;
  prepare::PreparedEdgePublication ambiguous_a = *named;
  prepare::PreparedEdgePublication ambiguous_b = *named;
  ambiguous_a.destination_value_id = prepare::PreparedValueId{901};
  ambiguous_b.destination_value_id = prepare::PreparedValueId{901};
  prepare::PreparedEdgePublicationLookups manual_lookups;
  manual_lookups.publications_by_edge_destination.emplace(
      prepare::prepared_edge_publication_key(
          predecessor_label, successor_label, prepare::PreparedValueId{900}),
      std::vector<const prepare::PreparedEdgePublication*>{&unavailable_publication});
  manual_lookups.publications_by_edge_destination.emplace(
      prepare::prepared_edge_publication_key(
          predecessor_label, successor_label, prepare::PreparedValueId{901}),
      std::vector<const prepare::PreparedEdgePublication*>{
          &ambiguous_a,
          &ambiguous_b,
      });
  manual_lookups.publications_by_edge_destination.emplace(
      prepare::prepared_edge_publication_key(
          predecessor_label, successor_label, prepare::PreparedValueId{902}),
      std::vector<const prepare::PreparedEdgePublication*>{
          &missing_source_home_publication,
      });
  if (prepare::prepare_edge_copy_source_facts(
          nullptr, predecessor_label, successor_label, named_destination_id)
          .status !=
      prepare::PreparedEdgeCopySourceFactsStatus::MissingPreparedLookups) {
    return fail("edge source fact query should diagnose missing prepared lookups");
  }
  if (prepare::prepare_edge_copy_source_facts(
          &lookups, predecessor_label, successor_label, prepare::PreparedValueId{777})
          .status != prepare::PreparedEdgeCopySourceFactsStatus::MissingPublication) {
    return fail("edge source fact query should diagnose missing publications");
  }
  if (prepare::prepare_edge_copy_source_facts(
          &manual_lookups,
          predecessor_label,
          successor_label,
          prepare::PreparedValueId{900})
          .status != prepare::PreparedEdgeCopySourceFactsStatus::PublicationUnavailable) {
    return fail("edge source fact query should diagnose unavailable publications");
  }
  if (prepare::prepare_edge_copy_source_facts(
          &manual_lookups,
          predecessor_label,
          successor_label,
          prepare::PreparedValueId{901})
          .status != prepare::PreparedEdgeCopySourceFactsStatus::AmbiguousPublication) {
    return fail("edge source fact query should diagnose ambiguous publications");
  }
  if (prepare::prepare_edge_copy_source_facts(
          &manual_lookups,
          predecessor_label,
          successor_label,
          prepare::PreparedValueId{902})
          .status != prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceHome) {
    return fail("edge source fact query should diagnose missing source homes");
  }

  return 0;
}

int verify_current_block_join_parallel_copy_source_query() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("current_join_query");
  const auto predecessor_label = names.block_labels.intern("current_join.pred");
  const auto successor_label = names.block_labels.intern("current_join.succ");
  const auto incoming_name = names.value_names.intern("%current.incoming");
  const auto operand_name = names.value_names.intern("%current.operand");
  const auto destination_name = names.value_names.intern("%current.destination");
  const auto immediate_destination_name =
      names.value_names.intern("%current.immediate_destination");
  const auto stack_source_name = names.value_names.intern("%current.stack_source");
  const auto stack_destination_name =
      names.value_names.intern("%current.stack_destination");

  const prepare::PreparedValueId incoming_id{101};
  const prepare::PreparedValueId operand_id{102};
  const prepare::PreparedValueId destination_id{103};
  const prepare::PreparedValueId immediate_destination_id{104};
  const prepare::PreparedValueId stack_source_id{105};
  const prepare::PreparedValueId stack_destination_id{106};

  bir::Block block;
  block.label = "current_join.succ";
  block.label_id = successor_label;
  block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%current.destination"),
      .incomings = {
          bir::PhiIncoming{
              .label = "current_join.pred",
              .value =
                  bir::Value::named(bir::TypeKind::I32, "%current.incoming"),
              .label_id = predecessor_label,
          },
      },
  });
  block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32,
                                  "%current.immediate_destination"),
      .incomings = {
          bir::PhiIncoming{
              .label = "current_join.pred",
              .value = bir::Value::immediate_i32(9),
              .label_id = predecessor_label,
          },
      },
  });
  block.insts.push_back(bir::PhiInst{
      .result =
          bir::Value::named(bir::TypeKind::I32, "%current.stack_destination"),
      .incomings = {
          bir::PhiIncoming{
              .label = "current_join.pred",
              .value =
                  bir::Value::named(bir::TypeKind::I32, "%current.stack_source"),
              .label_id = predecessor_label,
          },
      },
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%current.incoming"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%current.operand"),
      .rhs = bir::Value::immediate_i32(1),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%current.destination"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%current.operand"),
      .rhs = bir::Value::immediate_i32(2),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%current.stack_source"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%current.operand"),
      .rhs = bir::Value::immediate_i32(3),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%current.operand"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(4),
      .rhs = bir::Value::immediate_i32(5),
  });

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .kind = prepare::PreparedJoinTransferKind::PhiEdge,
              .edge_transfers = {
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value =
                          bir::Value::named(bir::TypeKind::I32, "%current.incoming"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%current.destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value = bir::Value::immediate_i32(9),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%current.immediate_destination"),
                  },
                  prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = predecessor_label,
                      .successor_label = successor_label,
                      .incoming_value = bir::Value::named(
                          bir::TypeKind::I32, "%current.stack_source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32, "%current.stack_destination"),
                  },
              },
          },
      },
      .parallel_copy_bundles = {
          prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor_label,
              .successor_label = successor_label,
              .steps = {
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 0,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 1,
                  },
                  prepare::PreparedParallelCopyStep{
                      .kind = prepare::PreparedParallelCopyStepKind::Move,
                      .move_index = 2,
                  },
              },
          },
      },
  };

  const prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = incoming_id,
              .function_name = function_name,
              .value_name = incoming_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x10"},
          },
          prepare::PreparedValueHome{
              .value_id = destination_id,
              .function_name = function_name,
              .value_name = destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x12"},
          },
          prepare::PreparedValueHome{
              .value_id = immediate_destination_id,
              .function_name = function_name,
              .value_name = immediate_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x13"},
          },
          prepare::PreparedValueHome{
              .value_id = stack_source_id,
              .function_name = function_name,
              .value_name = stack_source_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = prepare::PreparedFrameSlotId{5},
              .offset_bytes = std::size_t{16},
          },
          prepare::PreparedValueHome{
              .value_id = stack_destination_id,
              .function_name = function_name,
              .value_name = stack_destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"x14"},
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 4,
              .source_parallel_copy_predecessor_label = predecessor_label,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .from_value_id = incoming_id,
                      .to_value_id = destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_register_name = std::string{"w12"},
                      .source_parallel_copy_step_index = std::size_t{0},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .to_value_id = immediate_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .source_parallel_copy_step_index = std::size_t{1},
                      .source_immediate_i32 = std::int64_t{9},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = stack_source_id,
                      .to_value_id = stack_destination_id,
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .source_parallel_copy_step_index = std::size_t{2},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .from_value_id = incoming_id,
                      .to_value_id = destination_id,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
      },
  };
  const prepare::PreparedRegallocFunction regalloc{
      .function_name = function_name,
      .values = {
          prepare::PreparedRegallocValue{
              .value_id = operand_id,
              .function_name = function_name,
              .value_name = operand_name,
              .type = bir::TypeKind::I32,
          },
      },
  };

  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&locations);
  const auto edge_publications =
      prepare::make_prepared_edge_publication_lookups(
          names, control_flow, &locations, &value_home_lookups);
  const auto query =
      prepare::prepare_current_block_join_parallel_copy_source_facts(
          prepare::PreparedCurrentBlockJoinParallelCopySourceQueryInputs{
              .names = &names,
              .regalloc = &regalloc,
              .value_locations = &locations,
              .edge_publications = &edge_publications,
              .block = &block,
              .successor_label = successor_label,
          });
  const auto bir_query = mir::find_bir_current_block_join_source_identity(
      mir::BirCurrentBlockJoinSourceRequest{
          .successor_block = &block,
          .successor_label = "current_join.succ",
          .successor_label_id = successor_label,
      });
  const auto route5_join_records =
      bir::route5_current_block_join_source_records(&block);

  auto contains_value_id = [](const std::vector<prepare::PreparedValueId>& values,
                              prepare::PreparedValueId value_id) {
    return std::find(values.begin(), values.end(), value_id) != values.end();
  };
  auto contains_value_name = [](const std::vector<c4c::ValueNameId>& values,
                                c4c::ValueNameId value_name) {
    return std::find(values.begin(), values.end(), value_name) != values.end();
  };
  if (query.status !=
          prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::Available ||
      query.facts.size() != 4) {
    return fail("current-block join query should expose all matching bundle facts");
  }
  if (!prepared_and_bir_current_block_join_source_identity_match(names, query,
                                                                bir_query)) {
    return fail("BIR current-block join-source identity should match prepared semantic facts");
  }
  if (bir_query.facts.size() != 3) {
    return fail("BIR current-block join-source identity should expose only PHI source facts");
  }
  auto find_route5_join = [&](std::string_view destination_name) {
    return std::find_if(route5_join_records.begin(),
                        route5_join_records.end(),
                        [&](const auto& record) {
                          return record.destination_value_name == destination_name;
                        });
  };
  const auto route5_named_join = find_route5_join("%current.destination");
  const auto route5_immediate_join =
      find_route5_join("%current.immediate_destination");
  const auto route5_stack_join = find_route5_join("%current.stack_destination");
  if (route5_join_records.size() != 3 ||
      route5_named_join == route5_join_records.end() ||
      route5_immediate_join == route5_join_records.end() ||
      route5_stack_join == route5_join_records.end() ||
      !*route5_named_join ||
      route5_named_join->status != bir::Route5PublicationStatus::Available ||
      route5_named_join->predecessor_label_id != predecessor_label ||
      route5_named_join->successor_label_id != successor_label ||
      route5_named_join->source_value_name != "%current.incoming" ||
      route5_named_join->source_producer_kind !=
          bir::Route5PublicationSourceKind::Binary ||
      route5_named_join->source_producer_instruction_index != std::size_t{3} ||
      !*route5_immediate_join ||
      route5_immediate_join->source_value_kind != bir::Value::Kind::Immediate ||
      route5_immediate_join->source_producer_kind !=
          bir::Route5PublicationSourceKind::Immediate ||
      !*route5_stack_join ||
      route5_stack_join->source_value_name != "%current.stack_source" ||
      route5_stack_join->source_producer_instruction_index != std::size_t{5}) {
    return fail("Route 5 current-block join records should expose PHI source identities");
  }
  const auto route5_named_join_destination =
      bir::route5_join_destination_value_record(*route5_named_join);
  const auto route5_named_join_source =
      bir::route5_join_source_value_record(*route5_named_join);
  if (!route5_named_join_destination ||
      route5_named_join_destination.scope !=
          bir::Route5PublicationScope::CurrentBlockJoin ||
      route5_named_join_destination.value_role !=
          bir::Route5PublicationValueRole::Destination ||
      route5_named_join_destination.value.name != "%current.destination" ||
      !route5_named_join_source ||
      route5_named_join_source.value_role !=
          bir::Route5PublicationValueRole::Source ||
      route5_named_join_source.value.name != "%current.incoming") {
    return fail("Route 5 current-block join value records should link destination and source values");
  }
  bir::Function route5_join_function;
  route5_join_function.blocks.push_back(block);
  const auto& route5_join_block = route5_join_function.blocks.front();
  const auto route5_join_index =
      bir::route5_build_edge_join_source_index(route5_join_function);
  const auto indexed_route5_named_join =
      bir::route5_find_current_block_join_source(
          route5_join_index,
          route5_join_block,
          bir::Value::named(bir::TypeKind::I32, "%current.destination"),
          bir::Value::named(bir::TypeKind::I32, "%current.incoming"));
  if (!route5_join_index ||
      !indexed_route5_named_join ||
      indexed_route5_named_join.status !=
          bir::Route5PublicationStatus::Available ||
      indexed_route5_named_join.source_producer_kind !=
          bir::Route5PublicationSourceKind::Binary ||
      indexed_route5_named_join.source_producer_instruction !=
          &route5_join_block.insts[3] ||
      indexed_route5_named_join.source_producer_instruction_index !=
          std::size_t{3}) {
    return fail("Route 5 join index should find named current-block join source");
  }
  const auto indexed_route5_immediate_join =
      bir::route5_find_current_block_join_source(
          route5_join_index,
          route5_join_block,
          bir::Value::named(bir::TypeKind::I32,
                            "%current.immediate_destination"),
          bir::Value::immediate_i32(9));
  if (!indexed_route5_immediate_join ||
      indexed_route5_immediate_join.source_value_kind !=
          bir::Value::Kind::Immediate ||
      indexed_route5_immediate_join.source_producer_kind !=
          bir::Route5PublicationSourceKind::Immediate) {
    return fail("Route 5 join index should find immediate current-block join source");
  }
  const auto indexed_route5_wrong_join_type =
      bir::route5_find_current_block_join_source(
          route5_join_index,
          route5_join_block,
          bir::Value::named(bir::TypeKind::I64, "%current.destination"),
          bir::Value::named(bir::TypeKind::I32, "%current.incoming"));
  if (indexed_route5_wrong_join_type ||
      indexed_route5_wrong_join_type.status !=
          bir::Route5PublicationStatus::NoMatch) {
    return fail("Route 5 join index should fail closed for destination type mismatch");
  }
  const auto indexed_route5_missing_join_source =
      bir::route5_find_current_block_join_source(
          route5_join_index,
          route5_join_block,
          bir::Value::named(bir::TypeKind::I32, "%current.destination"),
          bir::Value::named(bir::TypeKind::I32, "%current.other"));
  if (indexed_route5_missing_join_source ||
      indexed_route5_missing_join_source.status !=
          bir::Route5PublicationStatus::MissingSourceValue) {
    return fail("Route 5 join index should fail closed for missing source value");
  }
  const auto indexed_route5_missing_join_destination =
      bir::route5_find_current_block_join_source(
          route5_join_index,
          route5_join_block,
          bir::Value::named(bir::TypeKind::I32, "%current.missing"),
          bir::Value::named(bir::TypeKind::I32, "%current.incoming"));
  if (indexed_route5_missing_join_destination ||
      indexed_route5_missing_join_destination.status !=
          bir::Route5PublicationStatus::MissingPublication) {
    return fail("Route 5 join index should fail closed for missing destination");
  }
  if (!contains_value_id(query.incoming_expression_value_ids, incoming_id) ||
      !contains_value_id(query.incoming_expression_value_ids, operand_id) ||
      !contains_value_name(query.incoming_expression_value_names, incoming_name) ||
      !contains_value_name(query.incoming_expression_value_names, operand_name)) {
    return fail("current-block join query should expose incoming expression closure");
  }
  if (!bir_value_identities_contain_name(
          bir_query.incoming_expression_values, "%current.incoming") ||
      !bir_value_identities_contain_name(
          bir_query.incoming_expression_values, "%current.operand")) {
    return fail("BIR current-block join query should expose incoming expression closure");
  }
  if (!contains_value_id(query.source_value_ids, destination_id) ||
      !contains_value_id(query.source_value_ids, immediate_destination_id) ||
      !contains_value_id(query.source_value_ids, stack_destination_id) ||
      !contains_value_id(query.source_value_ids, stack_source_id)) {
    return fail("current-block join query should expose source value identities");
  }
  if (!bir_value_identities_contain_name(bir_query.source_values,
                                         "%current.destination") ||
      !bir_value_identities_contain_name(bir_query.source_values,
                                         "%current.immediate_destination") ||
      !bir_value_identities_contain_name(bir_query.source_values,
                                         "%current.stack_destination") ||
      !bir_value_identities_contain_name(bir_query.source_values,
                                         "%current.stack_source")) {
    return fail("BIR current-block join query should expose source value identities");
  }

  const auto& named_fact = query.facts[0];
  if (named_fact.status != prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      named_fact.source_value_id != incoming_id ||
      named_fact.source_home != &locations.value_homes[0] ||
      named_fact.destination_home != &locations.value_homes[1] ||
      named_fact.destination_register_name != std::optional<std::string>{"w12"} ||
      !named_fact.source_is_incoming_expression ||
      !named_fact.destination_is_source_value ||
      named_fact.source_is_source_value ||
      named_fact.move != &locations.move_bundles.front().moves[0] ||
      named_fact.publication == nullptr) {
    return fail("current-block join query should preserve named source provenance");
  }
  const auto& immediate_fact = query.facts[1];
  if (immediate_fact.status !=
          prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      !immediate_fact.immediate_source ||
      immediate_fact.source_is_incoming_expression ||
      !immediate_fact.destination_is_source_value ||
      immediate_fact.source_home != nullptr) {
    return fail("current-block join query should classify immediate sources");
  }
  const auto& stack_fact = query.facts[2];
  if (stack_fact.status != prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      !stack_fact.source_home_is_stack ||
      !stack_fact.source_is_source_value ||
      stack_fact.source_home != &locations.value_homes[3] ||
      stack_fact.destination_home != &locations.value_homes[4]) {
    return fail("current-block join query should expose stack-source source facts");
  }
  if (query.facts[3].status !=
      prepare::PreparedEdgeCopySourceFactsStatus::UnsupportedMove) {
    return fail("current-block join query should fail closed for unsupported moves");
  }
  if (prepare::prepare_current_block_join_parallel_copy_source_facts(
          prepare::PreparedCurrentBlockJoinParallelCopySourceQueryInputs{
              .names = &names,
              .value_locations = &locations,
              .block = &block,
              .successor_label = successor_label,
          })
          .status !=
      prepare::PreparedCurrentBlockJoinParallelCopySourceStatus::
          MissingEdgePublicationLookups) {
    return fail("current-block join query should require shared edge publications");
  }
  if (mir::find_bir_current_block_join_source_identity(
          mir::BirCurrentBlockJoinSourceRequest{})
          .status != mir::BirCurrentBlockJoinSourceStatus::MissingBlock) {
    return fail("BIR current-block join query should fail closed without a successor block");
  }
  const auto route5_missing_successor_join =
      bir::route5_current_block_join_source_records(nullptr);
  if (route5_missing_successor_join.size() != 1 ||
      route5_missing_successor_join.front().status !=
          bir::Route5PublicationStatus::MissingSuccessor) {
    return fail("Route 5 current-block join records should fail closed without a successor block");
  }
  bir::Block no_phi_block;
  no_phi_block.label = "current_join.succ";
  no_phi_block.label_id = successor_label;
  if (mir::find_bir_current_block_join_source_identity(
          mir::BirCurrentBlockJoinSourceRequest{
              .successor_block = &no_phi_block,
              .successor_label = "current_join.succ",
              .successor_label_id = successor_label,
          })
          .status != mir::BirCurrentBlockJoinSourceStatus::MissingPublication) {
    return fail("BIR current-block join query should fail closed without PHIs");
  }
  const auto route5_no_phi_join =
      bir::route5_current_block_join_source_records(&no_phi_block);
  if (route5_no_phi_join.size() != 1 ||
      route5_no_phi_join.front().status !=
          bir::Route5PublicationStatus::MissingPublication) {
    return fail("Route 5 current-block join records should fail closed without PHIs");
  }
  if (mir::find_bir_current_block_join_source_identity(
          mir::BirCurrentBlockJoinSourceRequest{
              .successor_block = &block,
              .successor_label = "current_join.other",
              .successor_label_id = successor_label,
          })
          .status != mir::BirCurrentBlockJoinSourceStatus::MissingSuccessorLabel) {
    return fail("BIR current-block join query should reject mismatched successors");
  }
  bir::Block missing_source_block;
  missing_source_block.label = "current_join.succ";
  missing_source_block.label_id = successor_label;
  missing_source_block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%current.destination"),
      .incomings = {
          bir::PhiIncoming{
              .label = "current_join.pred",
              .value = bir::Value::named(bir::TypeKind::I32,
                                         "%current.missing_source"),
              .label_id = predecessor_label,
          },
      },
  });
  const auto missing_source_query =
      mir::find_bir_current_block_join_source_identity(
          mir::BirCurrentBlockJoinSourceRequest{
              .successor_block = &missing_source_block,
              .successor_label = "current_join.succ",
              .successor_label_id = successor_label,
          });
  if (missing_source_query.status !=
          mir::BirCurrentBlockJoinSourceStatus::MissingSourceProducer ||
      missing_source_query.facts.size() != 1 ||
      missing_source_query.facts.front().status !=
          mir::BirCurrentBlockJoinSourceStatus::MissingSourceProducer) {
    return fail("BIR current-block join query should diagnose missing named source producers");
  }
  const auto route5_missing_join =
      bir::route5_current_block_join_source_records(&missing_source_block);
  if (route5_missing_join.size() != 1 ||
      route5_missing_join.front() ||
      route5_missing_join.front().status !=
          bir::Route5PublicationStatus::MissingSourceProducer ||
      route5_missing_join.front().source_value_name != "%current.missing_source") {
    return fail("Route 5 current-block join records should diagnose missing source producers");
  }
  bir::Function route5_missing_join_function;
  route5_missing_join_function.blocks.push_back(missing_source_block);
  const auto& route5_missing_join_block =
      route5_missing_join_function.blocks.front();
  const auto route5_missing_join_index =
      bir::route5_build_edge_join_source_index(route5_missing_join_function);
  const auto indexed_route5_missing_join_producer =
      bir::route5_find_current_block_join_source(
          route5_missing_join_index,
          route5_missing_join_block,
          bir::Value::named(bir::TypeKind::I32, "%current.destination"),
          bir::Value::named(bir::TypeKind::I32, "%current.missing_source"));
  if (indexed_route5_missing_join_producer ||
      indexed_route5_missing_join_producer.status !=
          bir::Route5PublicationStatus::MissingSourceProducer) {
    return fail("Route 5 join index should preserve missing source producer status");
  }

  return 0;
}

int verify_same_width_i32_stack_source_publication_facts() {
  const auto source_name = c4c::ValueNameId{101};
  const auto destination_name = c4c::ValueNameId{102};
  prepare::PreparedValueHome source_home{
      .value_id = prepare::PreparedValueId{201},
      .value_name = source_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{7},
      .offset_bytes = std::size_t{16},
      .size_bytes = std::size_t{4},
      .align_bytes = std::size_t{4},
  };
  prepare::PreparedMoveResolution move{
      .from_value_id = prepare::PreparedValueId{201},
      .to_value_id = prepare::PreparedValueId{202},
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .destination_register_placement = prepare::PreparedRegisterPlacement{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
          .slot_index = 3,
          .contiguous_width = 1,
      },
  };
  prepare::PreparedEdgePublication publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .destination_value = bir::Value::named(bir::TypeKind::I32, "%typed.dst"),
      .source_value = bir::Value::named(bir::TypeKind::I32, "%typed.src"),
      .destination_value_id = prepare::PreparedValueId{202},
      .destination_value_name = destination_name,
      .source_value_id = prepare::PreparedValueId{201},
      .source_value_name = source_name,
      .source_home = &source_home,
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .move = &move,
  };

  const auto typed =
      prepare::prepare_same_width_i32_stack_source_publication(&publication);
  if (typed.status !=
          prepare::PreparedTypedStackSourcePublicationStatus::Available ||
      typed.publication != &publication || typed.source_home != &source_home ||
      typed.move != &move ||
      typed.source_value_id != prepare::PreparedValueId{201} ||
      typed.destination_value_id != prepare::PreparedValueId{202} ||
      typed.source_type != bir::TypeKind::I32 ||
      typed.destination_type != bir::TypeKind::I32 ||
      typed.extension_policy !=
          prepare::PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension ||
      typed.source_slot_id != prepare::PreparedFrameSlotId{7} ||
      typed.source_stack_offset_bytes != std::size_t{16} ||
      typed.source_stack_size_bytes != std::size_t{4} ||
      typed.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
      typed.destination_register_placement != move.destination_register_placement) {
    return fail("typed stack-source publication should expose shared I32 stack and GPR facts");
  }

  auto missing_type = publication;
  missing_type.source_value.type = bir::TypeKind::F32;
  if (prepare::prepare_same_width_i32_stack_source_publication(&missing_type)
          .status !=
      prepare::PreparedTypedStackSourcePublicationStatus::MissingSameWidthI32Type) {
    return fail("typed stack-source publication should fail closed without same-width I32 types");
  }

  auto missing_offset_home = source_home;
  missing_offset_home.offset_bytes.reset();
  auto missing_concrete = publication;
  missing_concrete.source_home = &missing_offset_home;
  if (prepare::prepare_same_width_i32_stack_source_publication(&missing_concrete)
          .status !=
      prepare::PreparedTypedStackSourcePublicationStatus::MissingConcreteStackSource) {
    return fail("typed stack-source publication should require concrete stack-source facts");
  }

  auto missing_placement_move = move;
  missing_placement_move.destination_register_placement.reset();
  auto missing_placement = publication;
  missing_placement.move = &missing_placement_move;
  if (prepare::prepare_same_width_i32_stack_source_publication(&missing_placement)
          .status !=
      prepare::PreparedTypedStackSourcePublicationStatus::
          MissingDestinationRegisterPlacement) {
    return fail("typed stack-source publication should require destination placement authority");
  }

  auto temp_save_move = move;
  temp_save_move.op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  auto unsupported_move = publication;
  unsupported_move.move = &temp_save_move;
  if (prepare::prepare_same_width_i32_stack_source_publication(&unsupported_move)
          .status !=
      prepare::PreparedTypedStackSourcePublicationStatus::UnsupportedMoveAuthority) {
    return fail("typed stack-source publication should reject unsupported move authority");
  }

  auto fpr_move = move;
  fpr_move.destination_register_placement->bank = prepare::PreparedRegisterBank::Fpr;
  auto missing_gpr = publication;
  missing_gpr.move = &fpr_move;
  if (prepare::prepare_same_width_i32_stack_source_publication(&missing_gpr)
          .status !=
      prepare::PreparedTypedStackSourcePublicationStatus::MissingDestinationGprBank) {
    return fail("typed stack-source publication should require destination GPR bank authority");
  }

  auto missing_view_move = move;
  missing_view_move.destination_register_placement->pool =
      prepare::PreparedRegisterSlotPool::None;
  auto missing_view = publication;
  missing_view.move = &missing_view_move;
  if (prepare::prepare_same_width_i32_stack_source_publication(&missing_view)
          .status !=
      prepare::PreparedTypedStackSourcePublicationStatus::MissingDestinationRegisterView) {
    return fail("typed stack-source publication should require destination register view authority");
  }

  return 0;
}

int verify_aggregate_stack_source_authority_status() {
  const auto source_name = c4c::ValueNameId{111};
  const auto destination_name = c4c::ValueNameId{112};
  prepare::PreparedValueHome source_home{
      .value_id = prepare::PreparedValueId{211},
      .value_name = source_name,
      .kind = prepare::PreparedValueHomeKind::StackSlot,
      .slot_id = prepare::PreparedFrameSlotId{17},
      .offset_bytes = std::size_t{32},
      .size_bytes = std::size_t{16},
      .align_bytes = std::size_t{8},
  };
  prepare::PreparedMoveResolution move{
      .from_value_id = prepare::PreparedValueId{211},
      .to_value_id = prepare::PreparedValueId{212},
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .destination_register_placement = prepare::PreparedRegisterPlacement{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .pool = prepare::PreparedRegisterSlotPool::CallArgument,
          .slot_index = 1,
          .contiguous_width = 2,
      },
  };
  prepare::PreparedEdgePublication publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .destination_value = bir::Value::named(bir::TypeKind::I32, "%agg.dst"),
      .source_value = bir::Value::named(bir::TypeKind::I32, "%agg.src"),
      .destination_value_id = prepare::PreparedValueId{212},
      .destination_value_name = destination_name,
      .source_value_id = prepare::PreparedValueId{211},
      .source_value_name = source_name,
      .source_home = &source_home,
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .destination_home_kind = prepare::PreparedValueHomeKind::Register,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .move = &move,
  };

  const auto aggregate =
      prepare::prepare_aggregate_stack_source_authority(&publication);
  if (aggregate.status !=
          prepare::PreparedAggregateStackSourceAuthorityStatus::
              MissingAggregateCopyAuthority ||
      aggregate.source_value_id != prepare::PreparedValueId{211} ||
      aggregate.destination_value_id != prepare::PreparedValueId{212} ||
      aggregate.source_type != bir::TypeKind::I32 ||
      aggregate.destination_type != bir::TypeKind::I32 ||
      aggregate.source_slot_id != prepare::PreparedFrameSlotId{17} ||
      aggregate.source_stack_offset_bytes != std::size_t{32} ||
      aggregate.source_stack_size_bytes != std::size_t{16} ||
      aggregate.source_stack_align_bytes != std::size_t{8} ||
      aggregate.copy_width_bytes != std::size_t{16} ||
      aggregate.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      aggregate.destination_register_placement != move.destination_register_placement ||
      aggregate.has_destination_lane_mapping ||
      aggregate.has_lane_widths_and_offsets ||
      aggregate.partial_copy_allowed ||
      aggregate.has_abi_layout_reference ||
      aggregate.has_scratch_ownership) {
    return fail("aggregate stack-source authority should expose concrete facts but report the missing copy contract");
  }
  if (prepare::prepared_aggregate_stack_source_authority_status_name(
          aggregate.status) != "missing_aggregate_copy_authority") {
    return fail("aggregate stack-source authority status should have a stable name");
  }

  auto scalar_source = source_home;
  scalar_source.size_bytes = std::size_t{4};
  auto scalar_publication = publication;
  scalar_publication.source_home = &scalar_source;
  if (prepare::prepare_aggregate_stack_source_authority(&scalar_publication)
          .status != prepare::PreparedAggregateStackSourceAuthorityStatus::Unavailable) {
    return fail("scalar stack-source publications should remain outside aggregate authority");
  }

  auto incomplete_source = source_home;
  incomplete_source.align_bytes.reset();
  auto incomplete_publication = publication;
  incomplete_publication.source_home = &incomplete_source;
  if (prepare::prepare_aggregate_stack_source_authority(&incomplete_publication)
          .status !=
      prepare::PreparedAggregateStackSourceAuthorityStatus::
          IncompleteConcreteStackSource) {
    return fail("aggregate stack-source authority should distinguish incomplete concrete source facts");
  }

  auto missing_placement_move = move;
  missing_placement_move.destination_register_placement.reset();
  auto incomplete_mapping = publication;
  incomplete_mapping.move = &missing_placement_move;
  if (prepare::prepare_aggregate_stack_source_authority(&incomplete_mapping)
          .status !=
      prepare::PreparedAggregateStackSourceAuthorityStatus::
          IncompleteDestinationMapping) {
    return fail("aggregate stack-source authority should distinguish incomplete destination mapping");
  }

  auto unsupported_move = move;
  unsupported_move.op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;
  auto unsupported_publication = publication;
  unsupported_publication.move = &unsupported_move;
  if (prepare::prepare_aggregate_stack_source_authority(&unsupported_publication)
          .status !=
      prepare::PreparedAggregateStackSourceAuthorityStatus::UnsupportedMoveAuthority) {
    return fail("aggregate stack-source authority should reject unsupported move authority");
  }

  return 0;
}

int verify_edge_publication_source_producer_facts() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("producer_facts");
  const auto predecessor_label = prepared.names.block_labels.intern("producer.entry");
  const auto successor_label = prepared.names.block_labels.intern("producer.join");

  const bir::Value loaded = bir::Value::named(bir::TypeKind::I32, "%loaded");
  const bir::Value casted = bir::Value::named(bir::TypeKind::I64, "%casted");
  const bir::Value sum = bir::Value::named(bir::TypeKind::I32, "%sum");
  const bir::Value selected = bir::Value::named(bir::TypeKind::I32, "%selected");
  const bir::Value load_destination = bir::Value::named(bir::TypeKind::I32, "%join.load");
  const bir::Value cast_destination = bir::Value::named(bir::TypeKind::I64, "%join.cast");
  const bir::Value binary_destination = bir::Value::named(bir::TypeKind::I32, "%join.binary");
  const bir::Value select_destination = bir::Value::named(bir::TypeKind::I32, "%join.select");
  const bir::Value unavailable_destination =
      bir::Value::named(bir::TypeKind::I32, "%join.unavailable");

  const auto loaded_name = prepared.names.value_names.intern(loaded.name);
  const auto casted_name = prepared.names.value_names.intern(casted.name);
  const auto sum_name = prepared.names.value_names.intern(sum.name);
  const auto selected_name = prepared.names.value_names.intern(selected.name);
  const auto load_destination_name = prepared.names.value_names.intern(load_destination.name);
  const auto cast_destination_name = prepared.names.value_names.intern(cast_destination.name);
  const auto binary_destination_name =
      prepared.names.value_names.intern(binary_destination.name);
  const auto select_destination_name =
      prepared.names.value_names.intern(select_destination.name);
  const auto unavailable_destination_name =
      prepared.names.value_names.intern(unavailable_destination.name);
  const auto unavailable_source_name =
      prepared.names.value_names.intern("%missing.producer");

  prepared.module.functions.push_back(bir::Function{
      .name = "producer_facts",
      .blocks = {
          bir::Block{
              .label = "producer.entry",
              .insts =
                  {
                      bir::LoadLocalInst{
                          .result = loaded,
                          .slot_name = "slot",
                          .address =
                              bir::MemoryAddress{
                                  .base_kind =
                                      bir::MemoryAddress::BaseKind::LocalSlot,
                                  .base_name = "slot",
                                  .byte_offset = 4,
                                  .size_bytes = 4,
                                  .align_bytes = 4,
                                  .base_slot_id = c4c::SlotNameId{31},
                              },
                      },
                      bir::CastInst{
                          .opcode = bir::CastOpcode::SExt,
                          .result = casted,
                          .operand = loaded,
                      },
                      bir::BinaryInst{
                          .opcode = bir::BinaryOpcode::Add,
                          .result = sum,
                          .operand_type = bir::TypeKind::I32,
                          .lhs = loaded,
                          .rhs = bir::Value::immediate_i32(4),
                      },
                      bir::SelectInst{
                          .predicate = bir::BinaryOpcode::Eq,
                          .result = selected,
                          .compare_type = bir::TypeKind::I32,
                          .lhs = loaded,
                          .rhs = sum,
                          .true_value = sum,
                          .false_value = bir::Value::immediate_i32(0),
                      },
                  },
              .terminator =
                  bir::Terminator{bir::BranchTerminator{
                      .target_label = "producer.join",
                      .target_label_id = successor_label,
                  }},
              .label_id = predecessor_label,
          },
          bir::Block{
              .label = "producer.join",
              .insts =
                  {
                      bir::PhiInst{
                          .result = load_destination,
                          .incomings =
                              {
                                  bir::PhiIncoming{
                                      .label = "producer.entry",
                                      .value = loaded,
                                      .label_id = predecessor_label,
                                  },
                              },
                      },
                      bir::PhiInst{
                          .result = cast_destination,
                          .incomings =
                              {
                                  bir::PhiIncoming{
                                      .label = "producer.entry",
                                      .value = casted,
                                      .label_id = predecessor_label,
                                  },
                              },
                      },
                      bir::PhiInst{
                          .result = binary_destination,
                          .incomings =
                              {
                                  bir::PhiIncoming{
                                      .label = "producer.entry",
                                      .value = sum,
                                      .label_id = predecessor_label,
                                  },
                              },
                      },
                      bir::PhiInst{
                          .result = select_destination,
                          .incomings =
                              {
                                  bir::PhiIncoming{
                                      .label = "producer.entry",
                                      .value = selected,
                                      .label_id = predecessor_label,
                                  },
                              },
                      },
                      bir::PhiInst{
                          .result = unavailable_destination,
                          .incomings =
                              {
                                  bir::PhiIncoming{
                                      .label = "producer.entry",
                                      .value = bir::Value::named(
                                          bir::TypeKind::I32,
                                          "%missing.producer"),
                                      .label_id = predecessor_label,
                                  },
                              },
                      },
                  },
              .label_id = successor_label,
          },
      },
  });

  const auto make_transfer =
      [&](const bir::Value& source, const bir::Value& destination) {
        return prepare::PreparedEdgeValueTransfer{
            .predecessor_label = predecessor_label,
            .successor_label = successor_label,
            .incoming_value = source,
            .destination_value = destination,
        };
      };

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {
          branch_block(predecessor_label, successor_label),
          return_block(successor_label),
      },
      .join_transfers = {
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .result = load_destination,
              .edge_transfers = {make_transfer(loaded, load_destination)},
          },
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .result = cast_destination,
              .edge_transfers = {make_transfer(casted, cast_destination)},
          },
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .result = binary_destination,
              .edge_transfers = {make_transfer(sum, binary_destination)},
          },
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .result = select_destination,
              .carrier_kind =
                  prepare::PreparedJoinTransferCarrierKind::SelectMaterialization,
              .edge_transfers = {make_transfer(selected, select_destination)},
          },
          prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = successor_label,
              .result = unavailable_destination,
              .edge_transfers =
                  {make_transfer(
                      bir::Value::named(bir::TypeKind::I32, "%missing.producer"),
                      unavailable_destination)},
          },
      },
  };

  const auto make_home = [&](prepare::PreparedValueId id, c4c::ValueNameId name) {
    return prepare::PreparedValueHome{
        .value_id = id,
        .function_name = function_name,
        .value_name = name,
        .kind = prepare::PreparedValueHomeKind::Register,
        .register_name = std::string{"home"},
    };
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              make_home(1, loaded_name),
              make_home(2, casted_name),
              make_home(3, sum_name),
              make_home(4, selected_name),
              make_home(11, load_destination_name),
              make_home(12, cast_destination_name),
              make_home(13, binary_destination_name),
              make_home(14, select_destination_name),
              make_home(15, unavailable_destination_name),
              make_home(16, unavailable_source_name),
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 80,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = predecessor_label,
              .inst_index = 0,
              .result_value_name = loaded_name,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{31},
                      .byte_offset = 4,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* load_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups.edge_publications, predecessor_label, successor_label, 11);
  const auto* cast_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups.edge_publications, predecessor_label, successor_label, 12);
  const auto* binary_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups.edge_publications, predecessor_label, successor_label, 13);
  const auto* select_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups.edge_publications, predecessor_label, successor_label, 14);
  const auto* unavailable_publication =
      prepare::find_unique_indexed_prepared_edge_publication(
          &lookups.edge_publications, predecessor_label, successor_label, 15);

  if (load_publication == nullptr ||
      load_publication->source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal ||
      load_publication->source_load_local == nullptr ||
      load_publication->source_producer_block_label != predecessor_label ||
      load_publication->source_producer_instruction_index != std::size_t{0}) {
    return fail("edge publication should expose load-local source producer facts");
  }
  if (load_publication->source_memory_access_status !=
          prepare::PreparedEdgePublicationSourceMemoryAccessStatus::Available ||
      load_publication->source_memory_access == nullptr ||
      load_publication->source_memory_base_kind !=
          prepare::PreparedAddressBaseKind::FrameSlot ||
      load_publication->source_memory_frame_slot_id !=
          prepare::PreparedFrameSlotId{31} ||
      load_publication->source_memory_byte_offset != 4 ||
      load_publication->source_memory_size_bytes != 4 ||
      load_publication->source_memory_align_bytes != 4 ||
      !load_publication->source_memory_can_use_base_plus_offset ||
      load_publication->source_memory_access->result_value_name != loaded_name) {
    return fail("edge publication should expose prepared source-memory facts");
  }
  if (!prepare::prepared_edge_publication_source_home_matches_source(
          *load_publication)) {
    return fail(
        "edge publication helper should accept matching prepared source home");
  }
  auto mismatched_source_home_publication = *load_publication;
  mismatched_source_home_publication.source_home_kind =
      prepare::PreparedValueHomeKind::StackSlot;
  if (prepare::prepared_edge_publication_source_home_matches_source(
          mismatched_source_home_publication)) {
    return fail(
        "edge publication helper should reject mismatched source home kind");
  }
  mismatched_source_home_publication = *load_publication;
  mismatched_source_home_publication.source_home = nullptr;
  if (prepare::prepared_edge_publication_source_home_matches_source(
          mismatched_source_home_publication)) {
    return fail("edge publication helper should reject missing source home");
  }
  if (!prepare::prepared_edge_publication_source_memory_matches_access(
          *load_publication, *load_publication->source_memory_access)) {
    return fail(
        "edge publication helper should accept matching prepared source memory");
  }
  auto mismatched_source_memory_publication = *load_publication;
  mismatched_source_memory_publication.source_memory_byte_offset = 8;
  if (prepare::prepared_edge_publication_source_memory_matches_access(
          mismatched_source_memory_publication,
          *load_publication->source_memory_access)) {
    return fail(
        "edge publication helper should reject mismatched source memory offset");
  }
  auto unnamed_source_memory_access = *load_publication->source_memory_access;
  unnamed_source_memory_access.result_value_name.reset();
  if (prepare::prepared_edge_publication_source_memory_matches_access(
          *load_publication, unnamed_source_memory_access)) {
    return fail(
        "edge publication helper should reject source memory without result value");
  }
  if (cast_publication == nullptr ||
      cast_publication->source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Cast ||
      cast_publication->source_cast == nullptr ||
      cast_publication->source_producer_instruction_index != std::size_t{1}) {
    return fail("edge publication should expose cast source producer facts");
  }
  if (binary_publication == nullptr ||
      binary_publication->source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      binary_publication->source_binary == nullptr ||
      binary_publication->source_producer_instruction_index != std::size_t{2}) {
    return fail("edge publication should expose binary source producer facts");
  }
  if (select_publication == nullptr ||
      select_publication->source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization ||
      select_publication->source_select == nullptr ||
      select_publication->carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization ||
      select_publication->source_producer_instruction_index != std::size_t{3}) {
    return fail("edge publication should expose select-style source producer facts");
  }
  const auto load_source_facts = prepare::prepare_edge_copy_source_facts(
      &lookups.edge_publications, predecessor_label, successor_label, 11);
  if (!prepare::prepared_edge_copy_source_facts_have_materializable_producer(
          load_source_facts)) {
    return fail(
        "edge source facts helper should classify load-local producers as materializable");
  }
  auto immediate_source_facts = load_source_facts;
  immediate_source_facts.source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::Immediate;
  if (prepare::prepared_edge_copy_source_facts_have_materializable_producer(
          immediate_source_facts)) {
    return fail("edge source facts helper should not materialize immediate producers");
  }
  auto unknown_source_facts = load_source_facts;
  unknown_source_facts.source_producer_kind =
      prepare::PreparedEdgePublicationSourceProducerKind::Unknown;
  if (prepare::prepared_edge_copy_source_facts_have_materializable_producer(
          unknown_source_facts)) {
    return fail("edge source facts helper should not materialize unknown producers");
  }
  auto unavailable_status_source_facts = load_source_facts;
  unavailable_status_source_facts.status =
      prepare::PreparedEdgeCopySourceFactsStatus::MissingSourceProducer;
  if (prepare::prepared_edge_copy_source_facts_have_materializable_producer(
          unavailable_status_source_facts)) {
    return fail("edge source facts helper should require available source facts");
  }
  auto publicationless_source_facts = load_source_facts;
  publicationless_source_facts.publication = nullptr;
  if (prepare::prepared_edge_copy_source_facts_have_materializable_producer(
          publicationless_source_facts)) {
    return fail("edge source facts helper should require publication authority");
  }
  const auto unavailable_source_facts = prepare::prepare_edge_copy_source_facts(
      &lookups.edge_publications, predecessor_label, successor_label, 15);
  if (unavailable_publication == nullptr ||
      unavailable_source_facts.status !=
          prepare::PreparedEdgeCopySourceFactsStatus::Available ||
      unavailable_source_facts.source_value_name != unavailable_source_name ||
      prepare::prepared_edge_copy_source_facts_have_materializable_producer(
          unavailable_source_facts)) {
    return fail("prepared oracle should classify missing edge source producers as unavailable for materialization");
  }

  const auto& function = prepared.module.functions.front();
  const auto& predecessor_block = function.blocks[0];
  const auto& successor_block = function.blocks[1];
  const auto load_request = make_bir_edge_publication_source_request(
      predecessor_block,
      predecessor_label,
      successor_block,
      successor_label,
      load_destination,
      11,
      load_destination_name);
  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          successor_label,
          11,
          load_request)) {
    return fail("BIR CFG edge source identity should match prepared load-local semantic oracle");
  }
  const auto bir_load_edge =
      mir::find_bir_cfg_edge_publication_source_identity(load_request);
  if (!bir_load_edge ||
      bir_load_edge.status !=
          mir::BirCfgEdgePublicationSourceStatus::Available ||
      bir_load_edge.predecessor_label_id != load_publication->predecessor_label ||
      bir_load_edge.successor_label_id != load_publication->successor_label ||
      bir_load_edge.destination_value_id != load_publication->destination_value_id ||
      bir_load_edge.destination_value_name_id !=
          load_publication->destination_value_name ||
      bir_load_edge.destination_value_name != load_destination.name ||
      bir_load_edge.source_value_kind != load_publication->source_value_kind ||
      prepared.names.value_names.find(bir_load_edge.source_value_name) !=
          load_publication->source_value_name ||
      bir_load_edge.source_value_type != load_publication->source_value.type ||
      bir_load_edge.source_producer_kind !=
          expected_bir_producer_kind(load_publication->source_producer_kind) ||
      bir_load_edge.source_producer_instruction_index !=
          load_publication->source_producer_instruction_index ||
      bir_load_edge.source_producer_block_label_id !=
          load_publication->source_producer_block_label.value_or(
              c4c::kInvalidBlockLabel) ||
      bir_load_edge.source_producer.inst != &predecessor_block.insts[0] ||
      bir_load_edge.source_memory_access.inst != &predecessor_block.insts[0] ||
      bir_load_edge.source_memory_access.node_kind !=
          mir::BirMemoryAccessNodeKind::LoadLocal ||
      bir_load_edge.source_memory_access.result_value_name != loaded.name ||
      bir_load_edge.source_memory_access.base_kind !=
          mir::BirMemoryAccessBaseKind::LocalSlot ||
      bir_load_edge.source_memory_access.address_space !=
          load_publication->source_memory_address_space ||
      bir_load_edge.source_memory_access.is_volatile !=
          load_publication->source_memory_is_volatile) {
    return fail("BIR CFG edge source identity should match prepared load-local semantic oracle");
  }
  const auto route5_load_edge =
      bir::route5_cfg_edge_publication_record(&predecessor_block,
                                              &successor_block,
                                              load_destination,
                                              load_destination_name,
                                              loaded_name);
  const auto route5_load_destination =
      bir::route5_edge_destination_value_record(route5_load_edge);
  const auto route5_load_source =
      bir::route5_edge_source_value_record(route5_load_edge);
  if (!route5_load_edge ||
      route5_load_edge.status != bir::Route5PublicationStatus::MemorySource ||
      route5_load_edge.predecessor_label_id != load_publication->predecessor_label ||
      route5_load_edge.successor_label_id != load_publication->successor_label ||
      route5_load_edge.destination_value_name_id !=
          load_publication->destination_value_name ||
      route5_load_edge.source_value_name_id != load_publication->source_value_name ||
      route5_load_edge.source_value_name != loaded.name ||
      route5_load_edge.source_value_type != load_publication->source_value.type ||
      route5_load_edge.source_producer_kind !=
          expected_bir_route5_publication_source_kind(
              load_publication->source_producer_kind) ||
      route5_load_edge.source_producer_instruction != &predecessor_block.insts[0] ||
      route5_load_edge.source_producer_instruction_index !=
          load_publication->source_producer_instruction_index ||
      !route5_load_edge.source_memory_identity_available ||
      route5_load_edge.source_memory_access.instruction !=
          &predecessor_block.insts[0] ||
      route5_load_edge.source_memory_access.node_kind !=
          bir::Route3MemoryAccessNodeKind::LoadLocal ||
      route5_load_edge.source_memory_access.result_value.name != loaded.name ||
      route5_load_edge.source_memory_access.base_kind !=
          bir::Route3MemoryAccessBaseKind::LocalSlot ||
      route5_load_edge.source_memory_access.address_space !=
          load_publication->source_memory_address_space ||
      route5_load_edge.source_memory_access.is_volatile !=
          load_publication->source_memory_is_volatile ||
      !route5_load_destination ||
      route5_load_destination.scope != bir::Route5PublicationScope::CfgEdge ||
      route5_load_destination.value_role !=
          bir::Route5PublicationValueRole::Destination ||
      route5_load_destination.value.name != load_destination.name ||
      !route5_load_source ||
      route5_load_source.value_role != bir::Route5PublicationValueRole::Source ||
      route5_load_source.value.name != loaded.name) {
    return fail("Route 5 edge record should expose load-local memory-source identity");
  }

  const auto cast_request = make_bir_edge_publication_source_request(
      predecessor_block,
      predecessor_label,
      successor_block,
      successor_label,
      cast_destination,
      12,
      cast_destination_name);
  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          successor_label,
          12,
          cast_request)) {
    return fail("BIR CFG edge source identity should match prepared cast semantic oracle");
  }
  const auto bir_cast_edge =
      mir::find_bir_cfg_edge_publication_source_identity(cast_request);
  if (!bir_cast_edge ||
      bir_cast_edge.source_producer_kind !=
          expected_bir_producer_kind(cast_publication->source_producer_kind) ||
      bir_cast_edge.source_producer_instruction_index !=
          cast_publication->source_producer_instruction_index ||
      bir_cast_edge.source_memory_access) {
    return fail("BIR CFG edge source identity should expose named non-memory producer identity only");
  }
  const auto route5_cast_edge =
      bir::route5_cfg_edge_publication_record(&predecessor_block,
                                              &successor_block,
                                              cast_destination,
                                              cast_destination_name,
                                              casted_name);
  if (!route5_cast_edge ||
      route5_cast_edge.status != bir::Route5PublicationStatus::Available ||
      route5_cast_edge.source_memory_identity_available ||
      route5_cast_edge.source_producer_kind !=
          expected_bir_route5_publication_source_kind(
              cast_publication->source_producer_kind) ||
      route5_cast_edge.source_producer_instruction_index !=
          cast_publication->source_producer_instruction_index) {
    return fail("Route 5 edge record should expose named non-memory source identity");
  }

  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          successor_label,
          13,
          make_bir_edge_publication_source_request(
              predecessor_block,
              predecessor_label,
              successor_block,
              successor_label,
              binary_destination,
              13,
              binary_destination_name)) ||
      !prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          successor_label,
          14,
          make_bir_edge_publication_source_request(
              predecessor_block,
              predecessor_label,
              successor_block,
              successor_label,
              select_destination,
              14,
              select_destination_name))) {
    return fail("BIR CFG edge source identity should match prepared non-memory semantic oracles");
  }

  const auto bir_missing_destination =
      mir::find_bir_cfg_edge_publication_source_identity(
          mir::BirCfgEdgePublicationSourceRequest{
              .predecessor_block = &predecessor_block,
              .predecessor_label_id = predecessor_label,
              .successor_block = &successor_block,
              .successor_label_id = successor_label,
              .destination_value_name = "%producer.missing",
              .destination_value_type = bir::TypeKind::I32,
          });
  if (bir_missing_destination ||
      bir_missing_destination.status !=
          mir::BirCfgEdgePublicationSourceStatus::MissingPublication) {
    return fail("BIR CFG edge source identity should fail closed for missing destination publications");
  }
  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          successor_label,
          99,
          mir::BirCfgEdgePublicationSourceRequest{
              .predecessor_block = &predecessor_block,
              .predecessor_label_id = predecessor_label,
              .successor_block = &successor_block,
              .successor_label_id = successor_label,
              .destination_value_name = "%producer.missing",
              .destination_value_type = bir::TypeKind::I32,
          })) {
    return fail("BIR CFG edge source identity should match prepared missing-destination oracle");
  }

  const auto bir_missing_source =
      mir::find_bir_cfg_edge_publication_source_identity(
          mir::BirCfgEdgePublicationSourceRequest{
              .predecessor_block = &predecessor_block,
              .predecessor_label_id = predecessor_label,
              .successor_block = &successor_block,
              .successor_label_id = successor_label,
              .destination_value = &unavailable_destination,
              .destination_value_id = 15,
              .destination_value_name = unavailable_destination.name,
              .destination_value_name_id = unavailable_destination_name,
              .destination_value_type = unavailable_destination.type,
          });
  if (bir_missing_source ||
      bir_missing_source.status !=
          mir::BirCfgEdgePublicationSourceStatus::MissingSourceProducer ||
      bir_missing_source.destination_value_id != 15 ||
      bir_missing_source.source_value_name != "%missing.producer") {
    return fail("BIR CFG edge source identity should fail closed for unavailable edge sources");
  }
  const auto route5_missing_source =
      bir::route5_cfg_edge_publication_record(&predecessor_block,
                                              &successor_block,
                                              unavailable_destination,
                                              unavailable_destination_name,
                                              unavailable_source_name);
  if (route5_missing_source ||
      route5_missing_source.status !=
          bir::Route5PublicationStatus::MissingSourceProducer ||
      route5_missing_source.destination_value_name_id !=
          unavailable_destination_name ||
      route5_missing_source.source_value_name != "%missing.producer" ||
      route5_missing_source.source_value_name_id != unavailable_source_name) {
    return fail("Route 5 edge record should fail closed with explicit missing-source status");
  }
  auto wrong_predecessor_block = predecessor_block;
  wrong_predecessor_block.label = "edge_producers.other_pred";
  wrong_predecessor_block.label_id = c4c::BlockLabelId{777};
  const auto route5_wrong_edge =
      bir::route5_cfg_edge_publication_record(&wrong_predecessor_block,
                                              &successor_block,
                                              load_destination,
                                              load_destination_name,
                                              loaded_name);
  if (route5_wrong_edge ||
      route5_wrong_edge.status != bir::Route5PublicationStatus::NoSource ||
      !route5_wrong_edge.explicit_no_source) {
    return fail("Route 5 edge record should represent wrong-edge no-source explicitly");
  }
  const auto route5_missing_destination =
      bir::route5_cfg_edge_publication_record(
          &predecessor_block,
          &successor_block,
          bir::Value::named(bir::TypeKind::I32, "%producer.missing"));
  if (route5_missing_destination ||
      route5_missing_destination.status !=
          bir::Route5PublicationStatus::MissingPublication) {
    return fail("Route 5 edge record should fail closed for missing destination");
  }
  bir::Function route5_edge_function;
  route5_edge_function.blocks.push_back(predecessor_block);
  route5_edge_function.blocks.push_back(successor_block);
  const auto& route5_predecessor = route5_edge_function.blocks.front();
  const auto& route5_successor = route5_edge_function.blocks.back();
  const auto route5_edge_index =
      bir::route5_build_edge_join_source_index(route5_edge_function);
  const auto indexed_route5_load_edge =
      bir::route5_find_cfg_edge_publication(route5_edge_index,
                                            route5_predecessor,
                                            route5_successor,
                                            load_destination);
  if (!route5_edge_index ||
      !indexed_route5_load_edge ||
      indexed_route5_load_edge.status !=
          bir::Route5PublicationStatus::MemorySource ||
      indexed_route5_load_edge.source_memory_access.instruction !=
          &route5_predecessor.insts[0] ||
      indexed_route5_load_edge.source_memory_access.node_kind !=
          bir::Route3MemoryAccessNodeKind::LoadLocal ||
      indexed_route5_load_edge.source_value_name != loaded.name ||
      indexed_route5_load_edge.destination_value_name != load_destination.name) {
    return fail("Route 5 edge index should find load-local memory-source edge");
  }
  const auto indexed_bir_load_edge =
      mir::find_bir_cfg_edge_publication_source_identity(
          make_bir_edge_publication_source_request(route5_predecessor,
                                                   predecessor_label,
                                                   route5_successor,
                                                   successor_label,
                                                   load_destination,
                                                   11,
                                                   load_destination_name));
  if (!indexed_bir_load_edge ||
      indexed_bir_load_edge.status !=
          mir::BirCfgEdgePublicationSourceStatus::Available ||
      indexed_bir_load_edge.source_producer.inst !=
          indexed_route5_load_edge.source_producer_instruction ||
      indexed_bir_load_edge.source_producer_instruction_index !=
          indexed_route5_load_edge.source_producer_instruction_index ||
      indexed_bir_load_edge.source_memory_access.inst !=
          indexed_route5_load_edge.source_memory_access.instruction ||
      indexed_bir_load_edge.source_memory_access.instruction_index !=
          indexed_route5_load_edge.source_memory_access.instruction_index ||
      indexed_bir_load_edge.source_memory_access.node_kind !=
          mir::BirMemoryAccessNodeKind::LoadLocal ||
      indexed_bir_load_edge.source_value_name !=
          indexed_route5_load_edge.source_value_name ||
      indexed_bir_load_edge.destination_value_name !=
          indexed_route5_load_edge.destination_value_name) {
    return fail("MIR Route 5 edge lookup should read indexed memory-source records");
  }
  const auto indexed_route5_missing_source =
      bir::route5_find_cfg_edge_publication(route5_edge_index,
                                            route5_predecessor,
                                            route5_successor,
                                            unavailable_destination);
  if (indexed_route5_missing_source ||
      indexed_route5_missing_source.status !=
          bir::Route5PublicationStatus::MissingSourceProducer ||
      indexed_route5_missing_source.source_value_name != "%missing.producer") {
    return fail("Route 5 edge index should preserve missing-source status");
  }
  const auto indexed_bir_missing_source =
      mir::find_bir_cfg_edge_publication_source_identity(
          make_bir_edge_publication_source_request(route5_predecessor,
                                                   predecessor_label,
                                                   route5_successor,
                                                   successor_label,
                                                   unavailable_destination,
                                                   15,
                                                   unavailable_destination_name));
  if (indexed_bir_missing_source ||
      indexed_bir_missing_source.status !=
          mir::BirCfgEdgePublicationSourceStatus::MissingSourceProducer ||
      indexed_bir_missing_source.source_value_name !=
          indexed_route5_missing_source.source_value_name ||
      indexed_bir_missing_source.destination_value_name !=
          indexed_route5_missing_source.destination_value_name) {
    return fail("MIR Route 5 edge lookup should preserve indexed missing-source status");
  }
  const auto indexed_route5_wrong_destination_type =
      bir::route5_find_cfg_edge_publication(
          route5_edge_index,
          route5_predecessor,
          route5_successor,
          bir::Value::named(bir::TypeKind::I64, load_destination.name));
  if (indexed_route5_wrong_destination_type ||
      indexed_route5_wrong_destination_type.status !=
          bir::Route5PublicationStatus::NoMatch) {
    return fail("Route 5 edge index should fail closed for destination type mismatch");
  }
  const auto indexed_route5_missing_destination =
      bir::route5_find_cfg_edge_publication(
          route5_edge_index,
          route5_predecessor,
          route5_successor,
          bir::Value::named(bir::TypeKind::I32, "%producer.missing"));
  if (indexed_route5_missing_destination ||
      indexed_route5_missing_destination.status !=
          bir::Route5PublicationStatus::MissingPublication) {
    return fail("Route 5 edge index should fail closed for missing destination");
  }
  auto route5_wrong_predecessor = route5_predecessor;
  route5_wrong_predecessor.label = "edge_producers.other_pred";
  route5_wrong_predecessor.label_id = c4c::BlockLabelId{777};
  const auto indexed_route5_wrong_predecessor =
      bir::route5_find_cfg_edge_publication(route5_edge_index,
                                            route5_wrong_predecessor,
                                            route5_successor,
                                            load_destination);
  if (indexed_route5_wrong_predecessor ||
      indexed_route5_wrong_predecessor.status !=
          bir::Route5PublicationStatus::NoSource ||
      !indexed_route5_wrong_predecessor.explicit_no_source) {
    return fail("Route 5 edge index should represent wrong predecessor as no-source");
  }
  const auto indexed_bir_wrong_predecessor =
      mir::find_bir_cfg_edge_publication_source_identity(
          make_bir_edge_publication_source_request(route5_wrong_predecessor,
                                                   c4c::BlockLabelId{777},
                                                   route5_successor,
                                                   successor_label,
                                                   load_destination,
                                                   11,
                                                   load_destination_name));
  if (indexed_bir_wrong_predecessor ||
      indexed_bir_wrong_predecessor.status !=
          mir::BirCfgEdgePublicationSourceStatus::MissingSourceValue) {
    return fail("MIR Route 5 edge lookup should fail closed for wrong predecessor records");
  }
  auto route5_wrong_successor = route5_successor;
  route5_wrong_successor.label = "edge_producers.other_succ";
  route5_wrong_successor.label_id = c4c::BlockLabelId{778};
  const auto indexed_route5_wrong_successor =
      bir::route5_find_cfg_edge_publication(route5_edge_index,
                                            route5_predecessor,
                                            route5_wrong_successor,
                                            load_destination);
  if (indexed_route5_wrong_successor ||
      indexed_route5_wrong_successor.status !=
          bir::Route5PublicationStatus::MissingSuccessor) {
    return fail("Route 5 edge index should fail closed for wrong successor");
  }
  bir::Function route5_multi_pred_function;
  bir::Block route5_multi_first_pred;
  route5_multi_first_pred.label = "edge_producers.first_pred";
  route5_multi_first_pred.label_id = c4c::BlockLabelId{901};
  route5_multi_first_pred.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%multi.first"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  bir::Block route5_multi_second_pred;
  route5_multi_second_pred.label = "edge_producers.second_pred";
  route5_multi_second_pred.label_id = c4c::BlockLabelId{902};
  route5_multi_second_pred.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%multi.second"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(3),
      .rhs = bir::Value::immediate_i32(4),
  });
  bir::Block route5_multi_successor;
  route5_multi_successor.label = "edge_producers.multi_succ";
  route5_multi_successor.label_id = c4c::BlockLabelId{903};
  route5_multi_successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%multi.dst"),
      .incomings = {
          bir::PhiIncoming{
              .label = route5_multi_first_pred.label,
              .value = bir::Value::named(bir::TypeKind::I32, "%multi.first"),
              .label_id = route5_multi_first_pred.label_id,
          },
          bir::PhiIncoming{
              .label = route5_multi_second_pred.label,
              .value = bir::Value::named(bir::TypeKind::I32, "%multi.second"),
              .label_id = route5_multi_second_pred.label_id,
          },
      },
  });
  route5_multi_pred_function.blocks.push_back(route5_multi_first_pred);
  route5_multi_pred_function.blocks.push_back(route5_multi_second_pred);
  route5_multi_pred_function.blocks.push_back(route5_multi_successor);
  const auto& route5_indexed_first_pred =
      route5_multi_pred_function.blocks[0];
  const auto& route5_indexed_second_pred =
      route5_multi_pred_function.blocks[1];
  const auto& route5_indexed_multi_successor =
      route5_multi_pred_function.blocks[2];
  const auto route5_multi_pred_index =
      bir::route5_build_edge_join_source_index(route5_multi_pred_function);
  const auto indexed_route5_second_pred =
      bir::route5_find_cfg_edge_publication(
          route5_multi_pred_index,
          route5_indexed_second_pred,
          route5_indexed_multi_successor,
          bir::Value::named(bir::TypeKind::I32, "%multi.dst"));
  if (!indexed_route5_second_pred ||
      indexed_route5_second_pred.predecessor_label_id !=
          route5_multi_second_pred.label_id ||
      indexed_route5_second_pred.source_value_name != "%multi.second" ||
      indexed_route5_second_pred.source_producer_instruction !=
          &route5_indexed_second_pred.insts[0]) {
    return fail("Route 5 edge index should continue past first predecessor and find later matching predecessor");
  }
  auto route5_multi_wrong_pred = route5_indexed_first_pred;
  route5_multi_wrong_pred.label = "edge_producers.missing_pred";
  route5_multi_wrong_pred.label_id = c4c::BlockLabelId{904};
  const auto indexed_route5_multi_wrong_pred =
      bir::route5_find_cfg_edge_publication(
          route5_multi_pred_index,
          route5_multi_wrong_pred,
          route5_indexed_multi_successor,
          bir::Value::named(bir::TypeKind::I32, "%multi.dst"));
  if (indexed_route5_multi_wrong_pred ||
      indexed_route5_multi_wrong_pred.status !=
          bir::Route5PublicationStatus::NoSource ||
      !indexed_route5_multi_wrong_pred.explicit_no_source) {
    return fail("Route 5 edge index should report no-source only after all predecessors miss");
  }

  auto wrong_predecessor_request = load_request;
  wrong_predecessor_request.predecessor_label_id = c4c::BlockLabelId{777};
  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          c4c::kInvalidBlockLabel,
          successor_label,
          11,
          wrong_predecessor_request)) {
    return fail("BIR CFG edge source identity should reject mismatched predecessor keys like the prepared oracle");
  }
  auto wrong_successor_request = load_request;
  wrong_successor_request.successor_label_id = c4c::BlockLabelId{778};
  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          c4c::kInvalidBlockLabel,
          11,
          wrong_successor_request)) {
    return fail("BIR CFG edge source identity should reject mismatched successor keys like the prepared oracle");
  }
  auto missing_destination_key_request = load_request;
  missing_destination_key_request.destination_value = nullptr;
  missing_destination_key_request.destination_value_id = 0;
  missing_destination_key_request.destination_value_name = {};
  missing_destination_key_request.destination_value_name_id =
      c4c::kInvalidValueName;
  if (!prepared_and_bir_cfg_edge_publication_source_identity_match(
          prepared.names,
          lookups.edge_publications,
          predecessor_label,
          successor_label,
          0,
          missing_destination_key_request)) {
    return fail("BIR CFG edge source identity should reject missing destination keys like the prepared oracle");
  }

  return 0;
}

int verify_before_return_abi_move_source_bank_lookup() {
  prepare::PreparedValueLocationFunction locations{
      .function_name = c4c::FunctionNameId{11},
      .move_bundles =
          {prepare::PreparedMoveBundle{
               .function_name = c4c::FunctionNameId{11},
               .phase = prepare::PreparedMovePhase::BeforeReturn,
               .block_index = 3,
               .instruction_index = 9,
               .moves =
                   {prepare::PreparedMoveResolution{
                        .from_value_id = prepare::PreparedValueId{41},
                        .to_value_id = prepare::PreparedValueId{41},
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                        .destination_storage_kind =
                            prepare::PreparedMoveStorageKind::Register,
                        .destination_register_name = std::string{"s0"},
                        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                        .destination_register_placement =
                            prepare::PreparedRegisterPlacement{
                                .bank = prepare::PreparedRegisterBank::Fpr,
                                .pool =
                                    prepare::PreparedRegisterSlotPool::CallResult,
                                .slot_index = 0,
                                .contiguous_width = 1,
                            },
                    },
                    prepare::PreparedMoveResolution{
                        .from_value_id = prepare::PreparedValueId{42},
                        .to_value_id = prepare::PreparedValueId{42},
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                        .destination_storage_kind =
                            prepare::PreparedMoveStorageKind::Register,
                        .destination_register_name = std::string{"x0"},
                        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                        .destination_register_placement =
                            prepare::PreparedRegisterPlacement{
                                .bank = prepare::PreparedRegisterBank::Gpr,
                                .pool =
                                    prepare::PreparedRegisterSlotPool::CallResult,
                                .slot_index = 0,
                                .contiguous_width = 1,
                            },
                    },
                    prepare::PreparedMoveResolution{
                        .from_value_id = prepare::PreparedValueId{43},
                        .to_value_id = prepare::PreparedValueId{43},
                        .destination_kind =
                            prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                        .destination_storage_kind =
                            prepare::PreparedMoveStorageKind::Register,
                        .destination_register_name = std::string{"s1"},
                        .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                    }},
           }},
  };

  const auto lookups = prepare::make_prepared_move_bundle_lookups(&locations);
  const auto* indexed =
      prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          &lookups,
          &locations,
          3,
          prepare::PreparedValueId{41},
          prepare::PreparedRegisterBank::Fpr);
  if (indexed != &locations.move_bundles.front().moves[0]) {
    return fail("before-return FPR ABI lookup should find the prepared source move");
  }

  const auto* fallback =
      prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          nullptr,
          &locations,
          3,
          prepare::PreparedValueId{41},
          prepare::PreparedRegisterBank::Fpr);
  if (fallback != &locations.move_bundles.front().moves[0]) {
    return fail("before-return FPR ABI fallback should use shared lookup semantics");
  }

  if (prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          &lookups,
          &locations,
          3,
          prepare::PreparedValueId{42},
          prepare::PreparedRegisterBank::Fpr) != nullptr) {
    return fail("before-return FPR ABI lookup should reject GPR destinations");
  }

  if (prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          &lookups,
          &locations,
          3,
          prepare::PreparedValueId{43},
          prepare::PreparedRegisterBank::Fpr) != nullptr) {
    return fail("before-return FPR ABI lookup should require prepared placement authority");
  }

  prepare::PreparedMoveBundleLookups empty_lookups;
  if (prepare::find_prepared_before_return_abi_move_by_source_and_destination_bank(
          &empty_lookups,
          &locations,
          3,
          prepare::PreparedValueId{41},
          prepare::PreparedRegisterBank::Fpr) != nullptr) {
    return fail("before-return FPR ABI lookup should fail closed with missing indexed authority");
  }

  return 0;
}

int verify_prepared_frame_address_offset_lookup() {
  const c4c::FunctionNameId function_name{13};
  const c4c::BlockLabelId block_label{17};
  const c4c::ValueNameId aggregate_value{23};
  const c4c::ValueNameId aggregate_lane_value{24};
  constexpr auto aggregate_value_id = prepare::PreparedValueId{230};
  constexpr auto shadow_value_id = prepare::PreparedValueId{231};
  constexpr auto ordinary_spill_value_id = prepare::PreparedValueId{232};
  prepare::PreparedStackLayout stack_layout{
      .objects =
          {prepare::PreparedStackObject{
              .object_id = prepare::PreparedObjectId{5},
              .function_name = function_name,
              .source_kind = "local_slot",
              .size_bytes = 16,
              .align_bytes = 8,
              .address_exposed = true,
              .requires_home_slot = true,
              .permanent_home_slot = true,
          },
           prepare::PreparedStackObject{
              .object_id = prepare::PreparedObjectId{6},
              .function_name = function_name,
              .source_kind = "regalloc.spill_slot",
              .size_bytes = 8,
              .align_bytes = 8,
          }},
      .frame_slots =
          {prepare::PreparedFrameSlot{
              .slot_id = prepare::PreparedFrameSlotId{9},
              .object_id = prepare::PreparedObjectId{5},
              .function_name = function_name,
              .offset_bytes = 32,
              .size_bytes = 16,
              .align_bytes = 8,
          },
           prepare::PreparedFrameSlot{
              .slot_id = prepare::PreparedFrameSlotId{10},
              .object_id = prepare::PreparedObjectId{6},
              .function_name = function_name,
              .offset_bytes = 64,
              .size_bytes = 8,
              .align_bytes = 8,
          }},
  };
  prepare::PreparedAddressingFunction addressing{
      .function_name = function_name,
      .address_materializations =
          {prepare::PreparedAddressMaterialization{
               .function_name = function_name,
               .block_label = block_label,
               .inst_index = 3,
               .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
               .result_value_name = aggregate_value,
               .frame_slot_id = prepare::PreparedFrameSlotId{9},
               .byte_offset = 36,
           },
           prepare::PreparedAddressMaterialization{
               .function_name = function_name,
               .block_label = block_label,
               .inst_index = 6,
               .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
               .result_value_name = aggregate_value,
               .result_value_id = shadow_value_id,
               .frame_slot_id = prepare::PreparedFrameSlotId{9},
               .byte_offset = 44,
           },
           prepare::PreparedAddressMaterialization{
               .function_name = function_name,
               .block_label = block_label,
               .inst_index = 7,
               .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
               .result_value_name = aggregate_value,
               .frame_slot_id = prepare::PreparedFrameSlotId{9},
               .byte_offset = 40,
           },
           prepare::PreparedAddressMaterialization{
               .function_name = function_name,
               .block_label = block_label,
               .inst_index = 9,
               .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
               .result_value_name = aggregate_lane_value,
               .result_value_id = shadow_value_id,
               .frame_slot_id = prepare::PreparedFrameSlotId{9},
           },
           prepare::PreparedAddressMaterialization{
               .function_name = function_name,
               .block_label = block_label,
               .inst_index = 10,
               .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
               .result_value_id = ordinary_spill_value_id,
               .frame_slot_id = prepare::PreparedFrameSlotId{10},
           }},
  };
  prepare::PreparedValueHomeLookups value_home_lookups;
  value_home_lookups.value_ids.emplace(aggregate_value, aggregate_value_id);
  value_home_lookups.value_ids.emplace(aggregate_lane_value, shadow_value_id);
  prepare::PreparedAddressMaterializationLookups lookups;
  for (const auto& materialization : addressing.address_materializations) {
    lookups.materializations_by_block[materialization.block_label].push_back(
        &materialization);
  }

  const auto first =
      prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, &lookups, block_label, aggregate_value, 3);
  if (!first.has_value() ||
      first->materialization != &addressing.address_materializations[0] ||
      first->frame_slot_id != prepare::PreparedFrameSlotId{9} ||
      first->object_id != prepare::PreparedObjectId{5} ||
      first->stack_offset_bytes != std::size_t{36}) {
    return fail("frame-address lookup should resolve prepared materialization offset");
  }

  const auto latest =
      prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, &lookups, block_label, aggregate_value, 8);
  if (!latest.has_value() ||
      latest->materialization != &addressing.address_materializations[2] ||
      latest->stack_offset_bytes != std::size_t{40}) {
    return fail("frame-address lookup should pick latest reaching materialization");
  }

  if (prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, nullptr, block_label, aggregate_value, 8).has_value()) {
    return fail("frame-address lookup should fail closed without indexed authority");
  }
  if (prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, &lookups, block_label, aggregate_value, 2).has_value()) {
    return fail("frame-address lookup should not use future materializations");
  }
  if (prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, &lookups, block_label, aggregate_value).value_or(
              prepare::PreparedFrameAddressOffset{})
          .materialization == &addressing.address_materializations[3]) {
    return fail("frame-address lookup should not recover aggregate authority from lane names");
  }
  const auto id_latest =
      prepare::find_indexed_prepared_frame_address_offset_for_value_id(
          stack_layout, &lookups, &value_home_lookups, block_label, aggregate_value_id, 8);
  if (!id_latest.has_value() ||
      id_latest->materialization != &addressing.address_materializations[2] ||
      id_latest->stack_offset_bytes != std::size_t{40}) {
    return fail("frame-address id lookup should consume prepared value-id authority");
  }
  const auto shadow =
      prepare::find_indexed_prepared_frame_address_offset_for_value_id(
          stack_layout, &lookups, &value_home_lookups, block_label, shadow_value_id, 8);
  if (!shadow.has_value() ||
      shadow->materialization != &addressing.address_materializations[1] ||
      shadow->stack_offset_bytes != std::size_t{44}) {
    return fail("frame-address id lookup should prefer explicit prepared value ids over names");
  }
  if (prepare::find_indexed_prepared_frame_address_offset_for_value_id(
          stack_layout, &lookups, &value_home_lookups, block_label, ordinary_spill_value_id, 10)
          .has_value()) {
    return fail("frame-address id lookup should reject non-addressable spill slots");
  }
  if (prepare::find_indexed_prepared_frame_address_offset_for_value_id(
          stack_layout, &lookups, nullptr, block_label, aggregate_value_id, 8)
          .has_value()) {
    return fail("frame-address id lookup should fail closed without value-id authority");
  }

  auto duplicate_lookups = lookups;
  duplicate_lookups.materializations_by_block[block_label].push_back(
      &addressing.address_materializations[2]);
  const auto duplicate =
      prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, &duplicate_lookups, block_label, aggregate_value, 8);
  if (!duplicate.has_value() ||
      duplicate->materialization != &addressing.address_materializations[2]) {
    return fail("frame-address lookup should accept duplicate identical authority");
  }

  auto conflicting = addressing.address_materializations[2];
  conflicting.byte_offset = 48;
  duplicate_lookups.materializations_by_block[block_label].push_back(&conflicting);
  if (prepare::find_indexed_prepared_frame_address_offset_for_value(
          stack_layout, &duplicate_lookups, block_label, aggregate_value, 8)
          .has_value()) {
    return fail("frame-address lookup should reject conflicting prepared authority");
  }

  auto conflicting_id = addressing.address_materializations[2];
  conflicting_id.result_value_id = aggregate_value_id;
  conflicting_id.byte_offset = 52;
  auto id_conflict_lookups = lookups;
  id_conflict_lookups.materializations_by_block[block_label].push_back(&conflicting_id);
  if (prepare::find_indexed_prepared_frame_address_offset_for_value_id(
          stack_layout,
          &id_conflict_lookups,
          &value_home_lookups,
          block_label,
          aggregate_value_id,
          8)
          .has_value()) {
    return fail("frame-address id lookup should reject conflicting explicit prepared authority");
  }

  return 0;
}

int verify_prepared_memory_access_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_name = prepared.names.function_names.intern("memory_lookup");
  const auto block_label = prepared.names.block_labels.intern("entry");
  const auto loaded_name = prepared.names.value_names.intern("%loaded");
  const auto duplicate_name = prepared.names.value_names.intern("%duplicate");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {return_block(block_label)},
  };
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 42,
                  .function_name = function_name,
                  .value_name = loaded_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x0"},
              },
              prepare::PreparedValueHome{
                  .value_id = 77,
                  .function_name = function_name,
                  .value_name = duplicate_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"x1"},
              },
          },
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 0,
                  .result_value_name = loaded_name,
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 1,
                  .result_value_name = duplicate_name,
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 2,
                  .result_value_name = duplicate_name,
              },
          },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto& accesses = prepared.addressing.functions.front().accesses;
  const auto* named_access =
      prepare::find_unique_indexed_prepared_memory_access_by_result_value_name(
          &lookups.memory_accesses, loaded_name);
  if (named_access != &accesses[0]) {
    return fail("memory-access lookup should find a unique result value name");
  }
  const auto* value_id_access =
      prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
          &lookups.memory_accesses, 42);
  if (value_id_access != &accesses[0]) {
    return fail("memory-access lookup should find a unique result value id");
  }
  if (prepare::find_unique_indexed_prepared_memory_access_by_result_value_name(
          &lookups.memory_accesses, duplicate_name) != nullptr) {
    return fail("memory-access lookup should preserve duplicate-name ambiguity");
  }
  if (prepare::find_unique_indexed_prepared_memory_access_by_result_value_id(
          &lookups.memory_accesses, 77) != nullptr) {
    return fail("memory-access lookup should preserve duplicate-id ambiguity");
  }
  const auto* duplicate_accesses =
      prepare::find_indexed_prepared_memory_accesses_by_result_value_name(
          &lookups.memory_accesses, duplicate_name);
  if (duplicate_accesses == nullptr || duplicate_accesses->size() != 2 ||
      (*duplicate_accesses)[0] != &accesses[1] ||
      (*duplicate_accesses)[1] != &accesses[2]) {
    return fail("memory-access lookup should retain all duplicate entries");
  }
  if (prepare::find_unique_indexed_prepared_memory_access_by_result_value_name(
          &lookups.memory_accesses, c4c::kInvalidValueName) != nullptr) {
    return fail("memory-access lookup should reject invalid result value names");
  }

  return 0;
}

int verify_bir_direct_memory_access_identity_lookup() {
  prepare::PreparedNameTables names;
  const auto block_label_id = names.block_labels.intern("entry");
  const auto loaded_name = names.value_names.intern("%loaded");
  const auto stored_name = names.value_names.intern("%stored");
  const auto global_name = names.link_names.intern("g.value");

  const prepare::PreparedAddressingFunction addressing{
      .function_name = names.function_names.intern("memory_identity"),
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .block_label = block_label_id,
                  .inst_index = 0,
                  .result_value_name = loaded_name,
                  .address_space = bir::AddressSpace::Fs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      },
              },
              prepare::PreparedMemoryAccess{
                  .block_label = block_label_id,
                  .inst_index = 1,
                  .stored_value_name = stored_name,
                  .address_space = bir::AddressSpace::Gs,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                          .symbol_name = global_name,
                      },
              },
          },
  };
  bir::Block block;
  block.label = "entry";
  block.insts = {
      bir::LoadLocalInst{
          .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
          .slot_name = "local",
          .slot_id = c4c::SlotNameId{7},
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = "local",
                  .address_space = bir::AddressSpace::Fs,
                  .is_volatile = true,
                  .base_slot_id = c4c::SlotNameId{7},
              },
      },
      bir::StoreGlobalInst{
          .global_name = "g.value",
          .global_name_id = global_name,
          .value = bir::Value::named(bir::TypeKind::I32, "%stored"),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
                  .base_name = "g.value",
                  .address_space = bir::AddressSpace::Gs,
                  .base_link_name_id = global_name,
              },
      },
  };

  const auto load = mir::find_bir_memory_access_identity(
      mir::BirMemoryAccessIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .instruction_index = 0,
          .node_kind = mir::BirMemoryAccessNodeKind::LoadLocal,
      });
  const auto* prepared_load =
      prepare::find_prepared_memory_access(addressing, block_label_id, 0);
  if (!load || prepared_load == nullptr ||
      names.value_names.find(load.result_value_name) !=
          prepared_load->result_value_name.value_or(c4c::kInvalidValueName) ||
      load.stored_value_name != std::string_view{} ||
      load.address_space != prepared_load->address_space ||
      !load.is_volatile ||
      load.base_kind != mir::BirMemoryAccessBaseKind::LocalSlot ||
      load.local_slot_id != c4c::SlotNameId{7}) {
    return fail("BIR load-local memory identity should match prepared semantic fields");
  }

  const auto store = mir::find_bir_memory_access_identity(
      mir::BirMemoryAccessIdentityRequest{
          .block = &block,
          .block_label = "entry",
          .instruction_index = 1,
          .node_kind = mir::BirMemoryAccessNodeKind::StoreGlobal,
      });
  const auto* prepared_store =
      prepare::find_prepared_memory_access(addressing, block_label_id, 1);
  if (!store || prepared_store == nullptr ||
      names.value_names.find(store.stored_value_name) !=
          prepared_store->stored_value_name.value_or(c4c::kInvalidValueName) ||
      store.result_value_name != std::string_view{} ||
      store.address_space != prepared_store->address_space ||
      store.is_volatile ||
      store.base_kind != mir::BirMemoryAccessBaseKind::GlobalSymbol ||
      store.global_name_id != global_name) {
    return fail("BIR store-global memory identity should match prepared semantic fields");
  }

  if (mir::find_bir_memory_access_identity(
          mir::BirMemoryAccessIdentityRequest{
              .block = &block,
              .block_label = "entry",
              .instruction_index = 0,
              .node_kind = mir::BirMemoryAccessNodeKind::StoreLocal,
          })) {
    return fail("BIR memory identity should reject node-kind mismatches");
  }
  if (mir::find_bir_memory_access_identity(
          mir::BirMemoryAccessIdentityRequest{
              .block = &block,
              .block_label = "other",
              .instruction_index = 0,
              .node_kind = mir::BirMemoryAccessNodeKind::LoadLocal,
          })) {
    return fail("BIR memory identity should reject block-label mismatches");
  }
  if (mir::find_bir_memory_access_identity(
          mir::BirMemoryAccessIdentityRequest{
              .block = &block,
              .block_label = "entry",
              .instruction_index = 9,
              .node_kind = mir::BirMemoryAccessNodeKind::LoadLocal,
          })) {
    return fail("BIR memory identity should reject missing instruction indexes");
  }

  return 0;
}

int verify_direct_global_select_chain_dependency_query() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("query.entry");
  const c4c::LinkNameId global_name{41};
  const bir::Value loaded =
      bir::Value::named(bir::TypeKind::I32, "%query.loaded");
  const bir::Value selected =
      bir::Value::named(bir::TypeKind::I32, "%query.selected");
  const bir::Value direct =
      bir::Value::named(bir::TypeKind::I32, "%query.direct");
  const bir::Value local =
      bir::Value::named(bir::TypeKind::I32, "%query.local");
  const bir::Value casted =
      bir::Value::named(bir::TypeKind::I32, "%query.casted");
  const bir::Value binary =
      bir::Value::named(bir::TypeKind::I32, "%query.binary");
  const bir::Block block{
      .label = "query.entry",
      .insts =
          {bir::LoadGlobalInst{
               .result = loaded,
               .global_name_id = global_name,
           },
           bir::SelectInst{
               .predicate = bir::BinaryOpcode::Eq,
               .result = selected,
               .compare_type = bir::TypeKind::I32,
               .lhs = bir::Value::immediate_i32(1),
               .rhs = bir::Value::immediate_i32(1),
               .true_value = loaded,
               .false_value = bir::Value::immediate_i32(0),
           },
           bir::LoadGlobalInst{
               .result = direct,
               .global_name_id = global_name,
           },
           bir::LoadLocalInst{
               .result = local,
               .slot_name = "query.local.slot",
               .byte_offset = 0,
               .align_bytes = 4,
           },
           bir::CastInst{
               .opcode = bir::CastOpcode::SExt,
               .result = casted,
               .operand = loaded,
           },
           bir::BinaryInst{
               .opcode = bir::BinaryOpcode::Add,
               .result = binary,
               .operand_type = bir::TypeKind::I32,
               .lhs = casted,
               .rhs = bir::Value::immediate_i32(1),
           }},
  };
  const auto* loaded_inst = std::get_if<bir::LoadGlobalInst>(&block.insts[0]);
  const auto* selected_inst = std::get_if<bir::SelectInst>(&block.insts[1]);
  const auto* direct_inst = std::get_if<bir::LoadGlobalInst>(&block.insts[2]);
  const auto* local_inst = std::get_if<bir::LoadLocalInst>(&block.insts[3]);
  const auto* casted_inst = std::get_if<bir::CastInst>(&block.insts[4]);
  const auto* binary_inst = std::get_if<bir::BinaryInst>(&block.insts[5]);
  const auto before_end = block.insts.size();
  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern(loaded.name),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_global = loaded_inst,
      });
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern(selected.name),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind =
              prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization,
          .block_label = block_label,
          .instruction_index = 1,
          .select = selected_inst,
      });
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern(direct.name),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal,
          .block_label = block_label,
          .instruction_index = 2,
          .load_global = direct_inst,
      });
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern(local.name),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 3,
          .load_local = local_inst,
      });
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern(casted.name),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Cast,
          .block_label = block_label,
          .instruction_index = 4,
          .cast = casted_inst,
      });
  source_producers.producers_by_value_name.emplace(
      names.value_names.intern(binary.name),
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 5,
          .binary = binary_inst,
      });
  const auto route1_index = bir::route1_build_producer_index(block);
  const auto route2_end_query = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = before_end,
  };
  const auto route2_index = bir::route2_build_select_chain_value_index(block);
  const auto route2_index_end_query = bir::Route2SelectChainValueQuery{
      .index = &route2_index,
      .before_instruction_index = before_end,
  };

  const auto select_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names, &source_producers, block_label, &block, selected, before_end);
  if (!select_dependency.contains_direct_global_load ||
      !select_dependency.root_is_select ||
      select_dependency.root_instruction_index != std::size_t{1}) {
    return fail("direct-global select-chain query should expose select root facts");
  }
  const prepare::PreparedCallArgumentPlan argument_plan{
      .source_value_id = prepare::PreparedValueId{7},
      .source_selection = std::nullopt,
      .direct_global_select_chain_dependency =
          prepare::PreparedCallArgumentDirectGlobalSelectChainDependency{
              .available = true,
              .source_value_name = names.value_names.find(selected.name),
              .direct_global_dependency = select_dependency,
          },
  };
  const auto* argument_dependency =
      prepare::find_prepared_call_argument_direct_global_select_chain_dependency(
          argument_plan);
  if (argument_dependency == nullptr ||
      argument_dependency->source_value_name != names.value_names.find(selected.name) ||
      !argument_dependency->direct_global_dependency.root_is_select ||
      argument_dependency->direct_global_dependency.root_instruction_index !=
          std::size_t{1}) {
    return fail("call-argument direct-global select-chain query should expose prepared facts");
  }
  const prepare::PreparedCallArgumentPlan missing_argument_plan;
  if (prepare::find_prepared_call_argument_direct_global_select_chain_dependency(
          missing_argument_plan) != nullptr) {
    return fail("call-argument direct-global select-chain query should fail closed");
  }
  const auto select_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names, &source_producers, block_label, &block, selected, before_end);
  if (!select_materialization.available ||
      select_materialization.root_value_name != names.value_names.find(selected.name) ||
      !select_materialization.direct_global_dependency.contains_direct_global_load ||
      !select_materialization.direct_global_dependency.root_is_select ||
      select_materialization.direct_global_dependency.root_instruction_index !=
          std::size_t{1}) {
    return fail("scalar select-chain materialization query should expose root authority");
  }
  const auto bir_select_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &selected,
      .root_value_name = selected.name,
      .root_value_type = selected.type,
      .before_instruction_index = before_end,
  };
  const auto bir_select_root =
      mir::find_bir_select_chain_source_producer(bir_select_request);
  const auto bir_select_dependency =
      mir::find_bir_select_chain_direct_global_dependency(bir_select_request);
  const auto bir_select_identity =
      mir::find_bir_select_chain_identity(bir_select_request);
  if (!bir_select_request ||
      !bir_select_root ||
      bir_select_root.inst != &block.insts[1] ||
      !bir_select_identity ||
      bir_select_identity.root_value.value != &selected_inst->result ||
      bir_select_identity.root_value_name != selected.name ||
      bir_select_identity.root_value.type != selected.type ||
      !bir_select_identity.root_is_select ||
      bir_select_identity.root_instruction_index !=
          select_dependency.root_instruction_index ||
      !bir_select_identity.direct_global_dependency ||
      bir_select_identity.direct_global_dependency.load_global != loaded_inst ||
      bir_select_identity.direct_global_dependency.instruction_index !=
          std::size_t{0} ||
      !bir_select_dependency ||
      bir_select_dependency.load_global != loaded_inst ||
      !mir::find_bir_select_chain_scalar_materialization_eligibility(
          bir_select_request) ||
      bir_select_identity.scalar_materialization_available !=
          select_materialization.available) {
    return fail("BIR select-chain query should expose select-root direct-global facts");
  }

  const auto direct_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names, &source_producers, block_label, &block, direct, before_end);
  if (!direct_dependency.contains_direct_global_load ||
      direct_dependency.root_is_select ||
      direct_dependency.root_instruction_index != std::size_t{2}) {
    return fail("direct-global select-chain query should expose direct load root facts");
  }
  const auto direct_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names, &source_producers, block_label, &block, direct, before_end);
  const auto bir_direct_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &direct,
      .root_value_name = direct.name,
      .root_value_type = direct.type,
      .before_instruction_index = before_end,
  };
  const auto bir_direct_root =
      mir::find_bir_select_chain_source_producer(bir_direct_request);
  const auto bir_direct_identity =
      mir::find_bir_select_chain_identity(bir_direct_request);
  if (!bir_direct_request ||
      !bir_direct_root ||
      bir_direct_root.inst != &block.insts[2] ||
      !bir_direct_identity ||
      bir_direct_identity.root_value.value != &direct_inst->result ||
      bir_direct_identity.root_value_name != direct.name ||
      bir_direct_identity.root_value.type != direct.type ||
      bir_direct_identity.root_is_select ||
      bir_direct_identity.root_instruction_index !=
          direct_dependency.root_instruction_index ||
      !bir_direct_identity.direct_global_dependency ||
      bir_direct_identity.direct_global_dependency.load_global != direct_inst ||
      bir_direct_identity.direct_global_dependency.instruction_index !=
          direct_dependency.root_instruction_index ||
      bir_direct_identity.scalar_materialization_available !=
          direct_materialization.available) {
    return fail("BIR select-chain query should expose direct-load root facts");
  }

  const auto local_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names, &source_producers, block_label, &block, local, before_end);
  const auto local_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names, &source_producers, block_label, &block, local, before_end);
  const auto bir_local_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &local,
      .root_value_name = local.name,
      .root_value_type = local.type,
      .before_instruction_index = before_end,
  };
  const auto bir_local_identity =
      mir::find_bir_select_chain_identity(bir_local_request);
  if (local_dependency.contains_direct_global_load ||
      !local_materialization.available ||
      !bir_local_identity ||
      bir_local_identity.root_producer.inst != &block.insts[3] ||
      bir_local_identity.root_is_select ||
      bir_local_identity.root_instruction_index != std::size_t{3} ||
      bir_local_identity.direct_global_dependency ||
      !bir_local_identity.scalar_materialization_available) {
    return fail("BIR select-chain query should expose local no-dependency facts");
  }

  if (!prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, selected, before_end) ||
      !prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, direct, before_end) ||
      !prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, local, before_end) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_end_query,
          selected) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_end_query,
          direct) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_end_query,
          local) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query, route2_end_query, selected) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query, route2_end_query, direct) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query, route2_end_query, local)) {
    return fail("BIR select-chain queries and Route 2 records should match prepared oracle for root positives and no-dependency paths");
  }
  const auto* indexed_select =
      bir::route2_find_select_chain_value_record(route2_index_end_query, selected);
  const auto* indexed_direct =
      bir::route2_find_select_chain_value_record(route2_index_end_query, direct);
  const auto* indexed_local =
      bir::route2_find_select_chain_value_record(route2_index_end_query, local);
  if (!route2_index ||
      route2_index.block != &block ||
      route2_index.records.size() != std::size_t{6} ||
      indexed_select == nullptr ||
      indexed_select->root_producer.instruction != &block.insts[1] ||
      !indexed_select->root_is_select ||
      !indexed_select->direct_global_dependency ||
      indexed_select->direct_global_dependency.load_global != loaded_inst ||
      indexed_direct == nullptr ||
      indexed_direct->root_producer.instruction != &block.insts[2] ||
      indexed_direct->root_is_select ||
      !indexed_direct->direct_global_dependency ||
      indexed_direct->direct_global_dependency.load_global != direct_inst ||
      indexed_local == nullptr ||
      indexed_local->root_producer.instruction != &block.insts[3] ||
      indexed_local->root_is_select ||
      !indexed_local->direct_global_dependency.available ||
      indexed_local->direct_global_dependency.contains_direct_global_load ||
      indexed_local->direct_global_dependency.load_global != nullptr) {
    return fail("Route 2 select-chain index should expose target-neutral BIR records");
  }
  const auto nested_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names, &source_producers, block_label, &block, binary, before_end);
  const auto nested_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names, &source_producers, block_label, &block, binary, before_end);
  const auto bir_nested_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &binary,
      .root_value_name = binary.name,
      .root_value_type = binary.type,
      .before_instruction_index = before_end,
  };
  const auto bir_nested_identity =
      mir::find_bir_select_chain_identity(bir_nested_request);
  if (!nested_dependency.contains_direct_global_load ||
      nested_dependency.root_is_select ||
      nested_dependency.root_instruction_index != std::size_t{5} ||
      !nested_materialization.available ||
      !bir_nested_identity ||
      bir_nested_identity.root_is_select ||
      bir_nested_identity.root_instruction_index !=
          nested_dependency.root_instruction_index ||
      !bir_nested_identity.direct_global_dependency ||
      bir_nested_identity.direct_global_dependency.load_global != loaded_inst ||
      bir_nested_identity.direct_global_dependency.instruction_index !=
          std::size_t{0} ||
      !bir_nested_identity.scalar_materialization_available ||
      !prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, binary, before_end) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_end_query,
          binary) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query, route2_end_query, binary)) {
    return fail("BIR select-chain queries and Route 2 records should match prepared oracle for nested non-select direct-global paths");
  }
  const auto route2_before_select = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = 1,
  };
  const auto route2_before_direct = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = 2,
  };
  const auto route2_index_before_select = bir::Route2SelectChainValueQuery{
      .index = &route2_index,
      .before_instruction_index = 1,
  };
  const auto route2_index_before_direct = bir::Route2SelectChainValueQuery{
      .index = &route2_index,
      .before_instruction_index = 2,
  };
  if (!prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, selected, 1) ||
      !prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, direct, 2) ||
      !prepared_and_bir_select_chain_answers_match(
          names,
          source_producers,
          block_label,
          block,
          bir::Value::named(bir::TypeKind::I64, selected.name),
          before_end) ||
      !prepared_and_bir_select_chain_answers_match(
          names,
          source_producers,
          block_label,
          block,
          bir::Value::named(bir::TypeKind::I32, "%query.mismatched_root"),
          before_end) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_before_select,
          selected) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_before_direct,
          direct) ||
      !prepared_and_route2_select_chain_records_match(
          names,
          source_producers,
          block_label,
          block,
          route2_end_query,
          bir::Value::named(bir::TypeKind::I64, selected.name)) ||
      !prepared_and_route2_select_chain_records_match(
          names,
          source_producers,
          block_label,
          block,
          route2_end_query,
          bir::Value::named(bir::TypeKind::I32, "%query.mismatched_root")) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_before_select, route2_before_select, selected) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_before_direct, route2_before_direct, direct) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query,
          route2_end_query,
          bir::Value::named(bir::TypeKind::I64, selected.name)) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query,
          route2_end_query,
          bir::Value::named(bir::TypeKind::I32, "%query.mismatched_root"))) {
    return fail("BIR select-chain queries and Route 2 records should match prepared fail-closed boundary and mismatch paths");
  }
  const auto explicit_mismatched_type_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &selected,
      .root_value_name = selected.name,
      .root_value_type = bir::TypeKind::I64,
      .before_instruction_index = before_end,
  };
  const auto explicit_mismatched_type_identity =
      mir::find_bir_select_chain_identity(explicit_mismatched_type_request);
  if (mir::find_bir_select_chain_source_producer(
          explicit_mismatched_type_request) ||
      mir::find_bir_select_chain_direct_global_dependency(
          explicit_mismatched_type_request) ||
      mir::find_bir_select_chain_scalar_materialization_eligibility(
          explicit_mismatched_type_request) ||
      explicit_mismatched_type_identity ||
      explicit_mismatched_type_identity.direct_global_dependency ||
      explicit_mismatched_type_identity.scalar_materialization_available ||
      explicit_mismatched_type_identity.root_instruction_index.has_value()) {
    return fail("BIR select-chain Route 2 lookup should honor explicit mismatched request types");
  }

  const auto immediate_root = bir::Value::immediate_i32(7);
  const auto immediate_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value = &immediate_root,
      .before_instruction_index = before_end,
  };
  const auto immediate_identity =
      mir::find_bir_select_chain_identity(immediate_request);
  if (!prepared_and_bir_select_chain_answers_match(
          names, source_producers, block_label, block, immediate_root,
          before_end) ||
      !prepared_and_route2_select_chain_records_match(
          names, source_producers, block_label, block, route2_end_query,
          immediate_root) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query, route2_end_query, immediate_root) ||
      mir::find_bir_select_chain_source_producer(immediate_request) ||
      mir::find_bir_select_chain_direct_global_dependency(immediate_request) ||
      mir::find_bir_select_chain_scalar_materialization_eligibility(
          immediate_request) ||
      immediate_identity ||
      immediate_identity.direct_global_dependency ||
      immediate_identity.scalar_materialization_available ||
      immediate_identity.root_instruction_index.has_value()) {
    return fail("BIR select-chain Route 2 lookup should reject scalar-ineligible immediate roots");
  }

  const auto missing_dependency =
      prepare::find_prepared_direct_global_select_chain_dependency(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%missing"),
          before_end);
  if (missing_dependency.contains_direct_global_load ||
      missing_dependency.root_instruction_index.has_value()) {
    return fail("direct-global select-chain query should fail closed on missing roots");
  }
  const auto missing_materialization =
      prepare::find_prepared_scalar_select_chain_materialization(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I32, "%missing"),
          before_end);
  if (missing_materialization.available ||
      missing_materialization.root_value_name != c4c::kInvalidValueName ||
      missing_materialization.direct_global_dependency.contains_direct_global_load) {
    return fail("scalar select-chain materialization query should fail closed");
  }
  const auto missing_request = mir::BirSelectChainIdentityRequest{
      .block = &block,
      .block_label = block.label,
      .root_value_name = "%missing",
      .root_value_type = bir::TypeKind::I32,
      .before_instruction_index = before_end,
  };
  const auto missing_identity =
      mir::find_bir_select_chain_identity(missing_request);
  if (missing_identity ||
      missing_identity.direct_global_dependency ||
      missing_identity.root_instruction_index.has_value() ||
      missing_identity.scalar_materialization_available ||
      mir::find_bir_select_chain_source_producer(missing_request) ||
      mir::find_bir_select_chain_direct_global_dependency(missing_request) ||
      mir::find_bir_select_chain_scalar_materialization_eligibility(
          missing_request) ||
      !prepared_and_route2_select_chain_records_match(
          names,
          source_producers,
          block_label,
          block,
          route2_end_query,
          bir::Value::named(bir::TypeKind::I32, "%missing")) ||
      !route2_select_chain_index_matches_record_lookup(
          route2_index_end_query,
          route2_end_query,
          bir::Value::named(bir::TypeKind::I32, "%missing"))) {
    return fail("BIR select-chain query and Route 2 records should fail closed on missing roots");
  }

  return 0;
}

int verify_prepared_same_block_scalar_source_facts() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("source_facts");
  const auto block_label = names.block_labels.intern("entry");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  const auto sum_name = names.value_names.intern("%sum");
  const auto wide_name = names.value_names.intern("%wide");
  const auto from_slot_name = names.value_names.intern("%from_slot");
  const auto from_global_name = names.value_names.intern("%from_global");
  const auto choice_name = names.value_names.intern("%choice");
  const auto product_name = names.value_names.intern("%product");
  const auto global_name = names.link_names.intern("global0");
  const auto slot_id = prepare::PreparedFrameSlotId{12};

  bir::Block block;
  block.label = "entry";
  block.label_id = block_label;
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I64, "%lhs"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(2),
      .rhs = bir::Value::immediate_i64(3),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Shl,
      .result = bir::Value::named(bir::TypeKind::I64, "%rhs"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(1),
      .rhs = bir::Value::immediate_i64(4),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I64, "%sum"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%lhs"),
      .rhs = bir::Value::named(bir::TypeKind::I64, "%rhs"),
  });
  block.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = bir::Value::named(bir::TypeKind::I64, "%wide"),
      .operand = bir::Value::named(bir::TypeKind::I64, "%sum"),
  });
  block.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%from_slot"),
      .slot_name = "local0",
      .slot_id = static_cast<c4c::SlotNameId>(slot_id),
      .byte_offset = 8,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "local0",
              .byte_offset = 8,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Fs,
              .is_volatile = true,
              .base_slot_id = static_cast<c4c::SlotNameId>(slot_id),
          },
  });
  block.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I64, "%from_global"),
      .global_name = "global0",
      .global_name_id = global_name,
      .byte_offset = 16,
      .align_bytes = 8,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
              .base_name = "global0",
              .byte_offset = 16,
              .size_bytes = 8,
              .align_bytes = 8,
              .address_space = bir::AddressSpace::Gs,
              .is_volatile = true,
              .base_link_name_id = global_name,
          },
  });
  block.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I64, "%choice"),
      .compare_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%sum"),
      .rhs = bir::Value::immediate_i64(21),
      .true_value = bir::Value::named(bir::TypeKind::I64, "%wide"),
      .false_value = bir::Value::named(bir::TypeKind::I64, "%from_slot"),
  });
  block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Mul,
      .result = bir::Value::named(bir::TypeKind::I64, "%product"),
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%sum"),
      .rhs = bir::Value::immediate_i64(2),
  });

  const auto* lhs = std::get_if<bir::BinaryInst>(&block.insts[0]);
  const auto* rhs = std::get_if<bir::BinaryInst>(&block.insts[1]);
  const auto* sum = std::get_if<bir::BinaryInst>(&block.insts[2]);
  const auto* wide = std::get_if<bir::CastInst>(&block.insts[3]);
  const auto* from_slot = std::get_if<bir::LoadLocalInst>(&block.insts[4]);
  const auto* from_global = std::get_if<bir::LoadGlobalInst>(&block.insts[5]);
  const auto* choice = std::get_if<bir::SelectInst>(&block.insts[6]);
  const auto* product = std::get_if<bir::BinaryInst>(&block.insts[7]);
  if (lhs == nullptr || rhs == nullptr || sum == nullptr || wide == nullptr ||
      from_slot == nullptr || from_global == nullptr || choice == nullptr ||
      product == nullptr) {
    return fail("same-block scalar source fixture is malformed");
  }

  const auto immediate_value = bir::Value::immediate_i64(17);
  const auto immediate_identity =
      bir::route1_source_value_identity(immediate_value);
  const auto immediate_constant =
      bir::route1_immediate_integer_constant(immediate_value, 2U);
  if (!immediate_identity ||
      immediate_identity.value_kind != bir::Value::Kind::Immediate ||
      immediate_identity.type != bir::TypeKind::I64 ||
      immediate_identity.name_id != c4c::kInvalidValueName ||
      immediate_identity.name != std::string_view{} ||
      immediate_identity.integer_constant != std::optional<std::int64_t>{17} ||
      !immediate_constant ||
      immediate_constant.value != 17 ||
      immediate_constant.type != bir::TypeKind::I64 ||
      immediate_constant.depth != 2U ||
      bir::route1_producer_kind_name(bir::Route1ProducerKind::Immediate) !=
          "immediate" ||
      bir::route1_producer_kind_has_materialization(
          bir::Route1ProducerKind::Immediate)) {
    return fail(
        "BIR Route 1 value records should expose immediate integer constants without materialization");
  }

  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      lhs_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 0,
          .binary = lhs,
      });
  source_producers.producers_by_value_name.emplace(
      rhs_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 1,
          .binary = rhs,
      });
  source_producers.producers_by_value_name.emplace(
      sum_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 2,
          .binary = sum,
      });
  source_producers.producers_by_value_name.emplace(
      wide_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Cast,
          .block_label = block_label,
          .instruction_index = 3,
          .cast = wide,
      });
  source_producers.producers_by_value_name.emplace(
      from_slot_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 4,
          .load_local = from_slot,
      });
  source_producers.producers_by_value_name.emplace(
      from_global_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal,
          .block_label = block_label,
          .instruction_index = 5,
          .load_global = from_global,
      });
  source_producers.producers_by_value_name.emplace(
      choice_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind =
              prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization,
          .block_label = block_label,
          .instruction_index = 6,
          .select = choice,
      });
  source_producers.producers_by_value_name.emplace(
      product_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 7,
          .binary = product,
      });

  if (!bir_route1_producer_record_matches(
          block,
          0,
          lhs->result,
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !bir_route1_producer_record_matches(
          block,
          3,
          wide->result,
          prepare::PreparedEdgePublicationSourceProducerKind::Cast) ||
      !bir_route1_producer_record_matches(
          block,
          4,
          from_slot->result,
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) ||
      !bir_route1_producer_record_matches(
          block,
          5,
          from_global->result,
          prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal) ||
      !bir_route1_producer_record_matches(
          block,
          6,
          choice->result,
          prepare::PreparedEdgePublicationSourceProducerKind::
              SelectMaterialization)) {
    return fail(
        "BIR Route 1 producer records should cover source value, instruction index, kind, and materialization availability");
  }
  if (bir::route1_producer_record(block, block.insts.size()) ||
      bir::route1_producer_record(
          bir::Block{
              .label = "entry",
              .insts = {bir::StoreLocalInst{
                  .slot_name = "local0",
                  .value = bir::Value::named(bir::TypeKind::I64, "%sum"),
              }},
              .label_id = block_label,
          },
          0)) {
    return fail(
        "BIR Route 1 producer records should fail closed for missing and non-producer instructions");
  }
  const prepare::PreparedStackLayout stack_layout{
      .frame_slots = {
          prepare::PreparedFrameSlot{
              .slot_id = slot_id,
              .object_id = 2,
              .function_name = function_name,
              .offset_bytes = 48,
              .size_bytes = 8,
              .align_bytes = 8,
          },
      },
  };
  const prepare::PreparedAddressingFunction addressing{
      .function_name = function_name,
      .accesses =
          {
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 4,
                  .result_value_name = from_slot_name,
                  .address_space = bir::AddressSpace::Fs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                          .frame_slot_id = slot_id,
                          .byte_offset = 8,
                          .size_bytes = 8,
                          .align_bytes = 8,
                          .can_use_base_plus_offset = true,
                      },
              },
              prepare::PreparedMemoryAccess{
                  .function_name = function_name,
                  .block_label = block_label,
                  .inst_index = 5,
                  .result_value_name = from_global_name,
                  .address_space = bir::AddressSpace::Gs,
                  .is_volatile = true,
                  .address =
                      prepare::PreparedAddress{
                          .base_kind = prepare::PreparedAddressBaseKind::GlobalSymbol,
                          .symbol_name = global_name,
                          .byte_offset = 16,
                          .size_bytes = 8,
                          .align_bytes = 8,
                          .can_use_base_plus_offset = true,
                      },
              },
          },
  };

  const auto prepared_sum =
      prepare::find_prepared_same_block_scalar_producer(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          block.insts.size());
  if (!prepared_sum.has_value() ||
      prepared_sum->producer.binary != sum ||
      prepared_sum->instruction_index != 2 ||
      prepared_sum->value_name != sum_name) {
    return fail("value-based scalar source facade should expose prepared producer facts");
  }
  const auto bir_query = mir::SameBlockValueMaterializationQuery{
      .block = &block,
      .block_label = "entry",
      .before_instruction_index = block.insts.size(),
  };
  const auto route1_index = bir::route1_build_producer_index(block);
  const auto route1_query = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = block.insts.size(),
  };
  if (!route1_index ||
      route1_index.records.size() != 8U) {
    return fail("BIR Route 1 producer index should contain one record per scalar producer instruction");
  }
  const auto memory_accesses =
      prepare::make_prepared_memory_access_lookups(&addressing);
  const auto bir_sum =
      mir::find_same_block_scalar_producer(
          bir_query, bir::Value::named(bir::TypeKind::I64, "%sum"));
  if (!bir_sum.has_value() ||
      bir_sum->producer.kind != mir::SameBlockProducerKind::Binary ||
      bir_sum->producer.inst != prepared_sum->instruction ||
      bir_sum->instruction != prepared_sum->instruction ||
      bir_sum->instruction_index != prepared_sum->instruction_index ||
      bir_sum->produced_value == nullptr ||
      bir_sum->produced_value->name != "%sum") {
    return fail("BIR same-block scalar producer query should match prepared scalar producer oracle");
  }
  const auto route1_sum =
      bir::route1_find_same_block_scalar_producer(
          route1_query, bir::Value::named(bir::TypeKind::I64, "%sum"));
  if (!route1_sum.has_value() ||
      route1_sum->record == nullptr ||
      route1_sum->record->kind != bir::Route1ProducerKind::Binary ||
      route1_sum->instruction != prepared_sum->instruction ||
      route1_sum->instruction_index != prepared_sum->instruction_index ||
      route1_sum->produced_value == nullptr ||
      route1_sum->produced_value->name != "%sum") {
    return fail("BIR Route 1 same-block scalar producer query should match prepared scalar producer oracle");
  }
  const auto route1_sum_materialization =
      bir::route1_find_materialization_availability(
          route1_query, bir::Value::named(bir::TypeKind::I64, "%sum"));
  if (!route1_sum_materialization ||
      !route1_sum_materialization.scalar_materialization_available ||
      route1_sum_materialization.producer_kind !=
          bir::Route1ProducerKind::Binary) {
    return fail("BIR Route 1 materialization query should use producer record payloads");
  }
  if (!prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%lhs"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%rhs"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%wide"),
          prepare::PreparedEdgePublicationSourceProducerKind::Cast) ||
      !prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot"),
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) ||
      !prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global"),
          prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal) ||
      !prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%choice"),
          prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization) ||
      !prepared_and_bir_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%product"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary)) {
    return fail("BIR scalar producer query should match prepared oracle across supported producer kinds");
  }
  if (!prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%lhs"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%rhs"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%wide"),
          prepare::PreparedEdgePublicationSourceProducerKind::Cast) ||
      !prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot"),
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) ||
      !prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global"),
          prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal) ||
      !prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%choice"),
          prepare::PreparedEdgePublicationSourceProducerKind::
              SelectMaterialization) ||
      !prepared_and_route1_scalar_producers_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%product"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary)) {
    return fail("BIR Route 1 scalar producer query should match prepared oracle across supported producer kinds");
  }
  if (!prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%lhs"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%rhs"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) ||
      !prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%wide"),
          prepare::PreparedEdgePublicationSourceProducerKind::Cast) ||
      !prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot"),
          prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal) ||
      !prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global"),
          prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal) ||
      !prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%choice"),
          prepare::PreparedEdgePublicationSourceProducerKind::
              SelectMaterialization) ||
      !prepared_and_route1_materialization_availability_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%product"),
          prepare::PreparedEdgePublicationSourceProducerKind::Binary)) {
    return fail("BIR Route 1 materialization availability should match prepared scalar producer oracle across supported producer kinds");
  }
  if (!prepared_and_bir_same_block_global_load_access_match(
          names,
          addressing,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global"))) {
    return fail("BIR same-block global-load access query should match prepared oracle");
  }
  auto before_global_query = bir_query;
  before_global_query.before_instruction_index = 5;
  if (!prepared_and_bir_same_block_global_load_access_match(
          names,
          addressing,
          source_producers,
          block_label,
          block,
          before_global_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global"))) {
    return fail("BIR/prepared same-block global-load access should fail closed before the producer");
  }
  if (!prepared_and_bir_same_block_global_load_access_match(
          names,
          addressing,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot")) ||
      mir::find_bir_same_block_global_load_access_identity(
          mir::BirSameBlockGlobalLoadAccessRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%from_global",
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = block.insts.size(),
          })) {
    return fail("BIR/prepared same-block global-load access should fail closed for non-global or mismatched roots");
  }
  if (!prepared_and_bir_same_block_load_local_source_match(
          names,
          stack_layout,
          memory_accesses,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot"))) {
    return fail("BIR same-block load-local source query should match prepared oracle");
  }
  auto before_local_query = bir_query;
  before_local_query.before_instruction_index = 4;
  if (!prepared_and_bir_same_block_load_local_source_match(
          names,
          stack_layout,
          memory_accesses,
          source_producers,
          block_label,
          block,
          before_local_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot")) ||
      mir::find_bir_same_block_load_local_source_identity(
          mir::BirSameBlockLoadLocalSourceRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%from_slot",
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = block.insts.size(),
          })) {
    return fail("BIR/prepared same-block load-local source should fail closed before producer or for mismatched roots");
  }
  auto invalidated_block = block;
  invalidated_block.insts.insert(
      invalidated_block.insts.begin() + 5,
      bir::StoreLocalInst{
          .slot_name = "local0",
          .slot_id = static_cast<c4c::SlotNameId>(slot_id),
          .value = bir::Value::named(bir::TypeKind::I64, "%sum"),
          .byte_offset = 8,
          .align_bytes = 8,
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = "local0",
                  .byte_offset = 8,
                  .size_bytes = 8,
                  .align_bytes = 8,
                  .address_space = bir::AddressSpace::Fs,
                  .base_slot_id = static_cast<c4c::SlotNameId>(slot_id),
              },
      });
  auto invalidated_source_producers = source_producers;
  invalidated_source_producers.producers_by_value_name[lhs_name]
      .binary = std::get_if<bir::BinaryInst>(&invalidated_block.insts[0]);
  invalidated_source_producers.producers_by_value_name[rhs_name]
      .binary = std::get_if<bir::BinaryInst>(&invalidated_block.insts[1]);
  invalidated_source_producers.producers_by_value_name[sum_name]
      .binary = std::get_if<bir::BinaryInst>(&invalidated_block.insts[2]);
  invalidated_source_producers.producers_by_value_name[wide_name]
      .cast = std::get_if<bir::CastInst>(&invalidated_block.insts[3]);
  invalidated_source_producers.producers_by_value_name[from_slot_name]
      .load_local = std::get_if<bir::LoadLocalInst>(&invalidated_block.insts[4]);
  invalidated_source_producers.producers_by_value_name[from_global_name]
      .instruction_index = 6;
  invalidated_source_producers.producers_by_value_name[from_global_name]
      .load_global = std::get_if<bir::LoadGlobalInst>(&invalidated_block.insts[6]);
  invalidated_source_producers.producers_by_value_name[choice_name]
      .instruction_index = 7;
  invalidated_source_producers.producers_by_value_name[choice_name]
      .select = std::get_if<bir::SelectInst>(&invalidated_block.insts[7]);
  invalidated_source_producers.producers_by_value_name[product_name]
      .instruction_index = 8;
  invalidated_source_producers.producers_by_value_name[product_name]
      .binary = std::get_if<bir::BinaryInst>(&invalidated_block.insts[8]);
  prepare::PreparedAddressingFunction invalidated_addressing = addressing;
  for (auto& access : invalidated_addressing.accesses) {
    if (access.inst_index == 5) {
      access.inst_index = 6;
    }
  }
  invalidated_addressing.accesses.push_back(prepare::PreparedMemoryAccess{
      .function_name = function_name,
      .block_label = block_label,
      .inst_index = 5,
      .stored_value_name = sum_name,
      .address_space = bir::AddressSpace::Fs,
      .address =
          prepare::PreparedAddress{
              .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
              .frame_slot_id = slot_id,
              .byte_offset = 8,
              .size_bytes = 8,
              .align_bytes = 8,
              .can_use_base_plus_offset = true,
          },
  });
  const auto invalidated_accesses =
      prepare::make_prepared_memory_access_lookups(&invalidated_addressing);
  auto after_intervening_store_query = bir_query;
  after_intervening_store_query.block = &invalidated_block;
  after_intervening_store_query.before_instruction_index =
      invalidated_block.insts.size();
  if (!prepared_and_bir_same_block_load_local_source_match(
          names,
          stack_layout,
          invalidated_accesses,
          invalidated_source_producers,
          block_label,
          invalidated_block,
          after_intervening_store_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot"))) {
    return fail("BIR/prepared load-local source should fail closed after same-slot store");
  }
  const auto current_block_sum =
      prepare::find_prepared_current_block_publication_consumption(
          names,
          &source_producers,
          block_label,
          &block,
          sum_name,
          block.insts.size());
  if (!current_block_sum.available ||
      current_block_sum.source_producer == nullptr ||
      current_block_sum.source_producer->binary != sum ||
      current_block_sum.produced_value == nullptr ||
      current_block_sum.produced_value->name != "%sum" ||
      current_block_sum.instruction_index != 2 ||
      current_block_sum.value_name != sum_name ||
      current_block_sum.source_producer_kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary) {
    return fail("current-block publication consumption query should expose prepared producer fact");
  }
  if (!prepared_and_bir_current_block_publication_identity_match(
          names,
          source_producers,
          block_label,
          block,
          sum_name,
          bir::TypeKind::I64,
          block.insts.size())) {
    return fail("BIR current-block publication identity should match prepared semantic producer fact");
  }
  const auto missing_name = names.value_names.intern("%missing");
  if (!prepared_and_bir_current_block_publication_identity_match(
          names,
          source_producers,
          block_label,
          block,
          missing_name,
          bir::TypeKind::I64,
          block.insts.size())) {
    return fail("BIR/prepared current-block publication identity should fail closed for missing value");
  }
  const auto other_block_label = names.block_labels.intern("other");
  if (!prepared_and_bir_current_block_publication_identity_match(
          names,
          source_producers,
          other_block_label,
          block,
          sum_name,
          bir::TypeKind::I64,
          block.insts.size())) {
    return fail("BIR/prepared current-block publication identity should fail closed for wrong block");
  }
  if (mir::find_bir_current_block_publication_identity(
          mir::BirCurrentBlockPublicationIdentityRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%sum",
              .root_value_type = bir::TypeKind::I32,
              .before_instruction_index = block.insts.size(),
          })) {
    return fail("BIR current-block publication identity should fail closed for mismatched value type");
  }
  const auto route1_index_for_current = bir::route1_build_producer_index(block);
  const auto route4_type_mismatch =
      bir::route4_current_block_publication_record(
          bir::Route1SameBlockProducerQuery{
              .index = &route1_index_for_current,
              .before_instruction_index = block.insts.size(),
          },
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          sum_name);
  if (route4_type_mismatch ||
      route4_type_mismatch.status !=
          bir::Route4PublicationAvailabilityStatus::NoMatch) {
    return fail("Route 4 current-block publication record should fail closed for mismatched value type");
  }
  bir::Function route4_current_function;
  route4_current_function.blocks.push_back(block);
  const auto& route4_current_block = route4_current_function.blocks.front();
  const auto route4_publications =
      bir::route4_build_publication_availability_index(route4_current_function);
  const auto route4_facade =
      bir::route_index_reference_facade(route4_publications);
  const auto indexed_current_sum =
      bir::route4_find_current_block_publication(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          route4_current_block.insts.size());
  const auto indexed_current_sum_ref =
      bir::route4_validate_current_block_publication_reference(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          route4_current_block.insts.size());
  const auto facade_current_sum_ref =
      bir::route_index_validate_current_block_publication_reference(
          route4_facade,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          route4_current_block.insts.size());
  if (!route4_publications ||
      !indexed_current_sum ||
      indexed_current_sum.status !=
          bir::Route4PublicationAvailabilityStatus::Available ||
      indexed_current_sum.value_name != "%sum" ||
      indexed_current_sum.source_producer_kind !=
          bir::Route4PublicationSourceKind::Binary ||
      indexed_current_sum.source_producer_instruction !=
          &route4_current_block.insts[2] ||
      indexed_current_sum.source_producer_instruction_index != std::size_t{2} ||
      !indexed_current_sum_ref ||
      indexed_current_sum_ref.status !=
          bir::RouteIndexValidationStatus::Valid ||
      indexed_current_sum_ref.reference.route !=
          bir::RouteIndexRoute::Route4PublicationAvailability ||
      indexed_current_sum_ref.reference.owner_scope !=
          bir::RouteIndexOwnerScope::Function ||
      indexed_current_sum_ref.reference.record_category !=
          bir::RouteIndexRecordCategory::Route4CurrentBlockPublication ||
      indexed_current_sum_ref.reference.relationship !=
          bir::RouteIndexRelationshipKind::Route4CurrentBlockPublication ||
      indexed_current_sum_ref.current_block_record == nullptr ||
      indexed_current_sum_ref.current_block_record->source_producer_instruction !=
          &route4_current_block.insts[2] ||
      !facade_current_sum_ref ||
      facade_current_sum_ref.current_block_record !=
          indexed_current_sum_ref.current_block_record ||
      facade_current_sum_ref.reference.record_index !=
          indexed_current_sum_ref.reference.record_index) {
    return fail("Route 4 current-block publication index/reference should find available BIR record");
  }
  const auto bir_current_sum_from_route4 =
      mir::find_bir_current_block_publication_identity(
          mir::BirCurrentBlockPublicationIdentityRequest{
              .block = &route4_current_block,
              .block_label = route4_current_block.label,
              .root_value_name = "%sum",
              .root_value_type = bir::TypeKind::I64,
              .before_instruction_index = route4_current_block.insts.size(),
          });
  if (!bir_current_sum_from_route4 ||
      bir_current_sum_from_route4.instruction !=
          indexed_current_sum.source_producer_instruction ||
      bir_current_sum_from_route4.instruction_index !=
          indexed_current_sum.source_producer_instruction_index ||
      bir_current_sum_from_route4.produced_value_name !=
          indexed_current_sum.value_name ||
      bir_current_sum_from_route4.produced_value_type !=
          indexed_current_sum.value_type ||
      bir_current_sum_from_route4.source_producer_kind !=
          mir::SameBlockProducerKind::Binary) {
    return fail("MIR current-block publication identity should answer from Route 4 indexed BIR record");
  }
  const auto indexed_current_before_sum =
      bir::route4_find_current_block_publication(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          2);
  if (indexed_current_before_sum ||
      indexed_current_before_sum.status !=
          bir::Route4PublicationAvailabilityStatus::MissingPublication) {
    return fail("Route 4 current-block publication index should fail closed before producer");
  }
  const auto indexed_current_type_mismatch =
      bir::route4_find_current_block_publication(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          route4_current_block.insts.size());
  const auto indexed_current_type_mismatch_ref =
      bir::route4_validate_current_block_publication_reference(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          route4_current_block.insts.size());
  const auto indexed_current_missing_ref =
      bir::route4_validate_current_block_publication_reference(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          route4_current_block.insts.size());
  const auto indexed_current_stale_ref =
      bir::route4_validate_current_block_publication_reference(
          route4_publications,
          block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          block.insts.size());
  auto duplicate_route4_publications = route4_publications;
  duplicate_route4_publications.current_block_records.push_back(
      indexed_current_sum);
  const auto indexed_current_duplicate_ref =
      bir::route4_validate_current_block_publication_reference(
          duplicate_route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          route4_current_block.insts.size());
  const auto duplicate_route4_facade =
      bir::route_index_reference_facade(duplicate_route4_publications);
  const auto facade_current_duplicate_ref =
      bir::route_index_validate_current_block_publication_reference(
          duplicate_route4_facade,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          route4_current_block.insts.size());
  const auto indexed_block_entry_wrong_relationship_ref =
      bir::route4_validate_block_entry_publication_reference(
          route4_publications,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"));
  const auto facade_current_type_mismatch_ref =
      bir::route_index_validate_current_block_publication_reference(
          route4_facade,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I32, "%sum"),
          route4_current_block.insts.size());
  const auto facade_current_missing_ref =
      bir::route_index_validate_current_block_publication_reference(
          route4_facade,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          route4_current_block.insts.size());
  const auto facade_current_stale_ref =
      bir::route_index_validate_current_block_publication_reference(
          route4_facade,
          block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          block.insts.size());
  const auto facade_block_entry_wrong_relationship_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route4_facade,
          route4_current_block,
          bir::Value::named(bir::TypeKind::I64, "%sum"));
  if (indexed_current_type_mismatch ||
      indexed_current_type_mismatch.status !=
          bir::Route4PublicationAvailabilityStatus::NoMatch ||
      indexed_current_type_mismatch_ref ||
      indexed_current_type_mismatch_ref.status !=
          bir::RouteIndexValidationStatus::WrongKey ||
      indexed_current_missing_ref ||
      indexed_current_missing_ref.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      indexed_current_stale_ref ||
      indexed_current_stale_ref.status !=
          bir::RouteIndexValidationStatus::StaleOwner ||
      indexed_current_duplicate_ref ||
      indexed_current_duplicate_ref.status !=
          bir::RouteIndexValidationStatus::DuplicateReference ||
      indexed_block_entry_wrong_relationship_ref ||
      indexed_block_entry_wrong_relationship_ref.status !=
          bir::RouteIndexValidationStatus::WrongRelationship ||
      facade_current_type_mismatch_ref.status !=
          indexed_current_type_mismatch_ref.status ||
      facade_current_missing_ref.status != indexed_current_missing_ref.status ||
      facade_current_stale_ref.status != indexed_current_stale_ref.status ||
      facade_current_duplicate_ref.status !=
          indexed_current_duplicate_ref.status ||
      facade_block_entry_wrong_relationship_ref.status !=
          indexed_block_entry_wrong_relationship_ref.status) {
    return fail("Route 4 current-block publication facade should match direct reference validation for type, missing, stale, duplicate, and wrong-relationship cases");
  }

  const auto product_constant =
      prepare::evaluate_prepared_same_block_integer_constant(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%product"),
          block.insts.size());
  if (!product_constant.has_value() || *product_constant != 42) {
    return fail("prepared integer constant query should fold prepared same-block producers");
  }
  const auto bir_product_constant =
      mir::evaluate_same_block_integer_constant(
          bir_query, bir::Value::named(bir::TypeKind::I64, "%product"));
  if (!bir_product_constant.has_value() ||
      bir_product_constant->value != *product_constant ||
      bir_product_constant->depth != 0U) {
    return fail("BIR same-block integer constant query should match prepared oracle");
  }
  const auto route1_product_constant =
      bir::route1_evaluate_same_block_integer_constant(
          route1_query, bir::Value::named(bir::TypeKind::I64, "%product"));
  if (!route1_product_constant.has_value() ||
      route1_product_constant->value != *product_constant ||
      route1_product_constant->type != bir::TypeKind::I64 ||
      route1_product_constant->depth != 0U) {
    return fail("BIR Route 1 integer constant query should match prepared oracle");
  }
  if (!prepared_and_bir_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::immediate_i64(7),
          7) ||
      !prepared_and_bir_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%lhs"),
          5) ||
      !prepared_and_bir_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%rhs"),
          16) ||
      !prepared_and_bir_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          21)) {
    return fail("BIR integer constant query should match prepared oracle for immediate and binary constants");
  }
  if (!prepared_and_route1_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::immediate_i64(7),
          7) ||
      !prepared_and_route1_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%lhs"),
          5) ||
      !prepared_and_route1_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%rhs"),
          16) ||
      !prepared_and_route1_integer_constants_match(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          21)) {
    return fail("BIR Route 1 integer constant query should match prepared oracle for immediate and binary constants");
  }
  if (!prepared_and_bir_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%wide")) ||
      !prepared_and_bir_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot")) ||
      !prepared_and_bir_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global")) ||
      !prepared_and_bir_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I64, "%choice"))) {
    return fail("integer constant equivalence should fail closed for nonconstant scalar producers");
  }
  if (!prepared_and_route1_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%wide")) ||
      !prepared_and_route1_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%from_slot")) ||
      !prepared_and_route1_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%from_global")) ||
      !prepared_and_route1_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%choice"))) {
    return fail("Route 1 integer constant equivalence should fail closed for nonconstant scalar producers");
  }

  const prepare::PreparedBranchCondition fused_compare_condition{
      .function_name = function_name,
      .block_label = block_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = bir::Value::named(bir::TypeKind::I1, "%condition"),
      .predicate = bir::BinaryOpcode::Slt,
      .compare_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%sum"),
      .rhs = bir::Value::immediate_i64(7),
      .can_fuse_with_branch = true,
  };
  const auto operand_producer_facts =
      prepare::find_prepared_fused_compare_operand_producer_facts(
          names,
          &source_producers,
          block_label,
          &block,
          fused_compare_condition,
          block.insts.size());
  if (!operand_producer_facts.has_value() ||
      !operand_producer_facts->lhs.has_value() ||
      !operand_producer_facts->rhs.has_value() ||
      operand_producer_facts->lhs->binary != sum ||
      operand_producer_facts->lhs->instruction_index != 2 ||
      operand_producer_facts->rhs->kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Immediate ||
      !operand_producer_facts->rhs->integer_constant.has_value() ||
      *operand_producer_facts->rhs->integer_constant != 7) {
    return fail("fused compare operand producer query should expose lhs/rhs facts");
  }

  auto materialized_condition = fused_compare_condition;
  materialized_condition.kind =
      prepare::PreparedBranchConditionKind::MaterializedBool;
  if (prepare::find_prepared_fused_compare_operand_producer_facts(
          names,
          &source_producers,
          block_label,
          &block,
          materialized_condition,
          block.insts.size())
          .has_value()) {
    return fail("fused compare operand producer query should fail closed by kind");
  }

  if (prepare::find_prepared_same_block_scalar_producer(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%product"),
          7)
          .has_value()) {
    return fail("scalar source facade should fail closed for future producers");
  }
  const auto before_product_query = mir::SameBlockValueMaterializationQuery{
      .block = &block,
      .block_label = "entry",
      .before_instruction_index = 7,
  };
  const auto route1_before_product_query = bir::Route1SameBlockProducerQuery{
      .index = &route1_index,
      .before_instruction_index = 7,
  };
  if (mir::find_same_block_scalar_producer(
          before_product_query, bir::Value::named(bir::TypeKind::I64, "%product"))
          .has_value() ||
      mir::evaluate_same_block_integer_constant(
          before_product_query, bir::Value::named(bir::TypeKind::I64, "%product"))
          .has_value()) {
    return fail("BIR same-block queries should fail closed for future producers like prepared oracle");
  }
  if (bir::route1_find_same_block_scalar_producer(
          route1_before_product_query,
          bir::Value::named(bir::TypeKind::I64, "%product"))
          .has_value() ||
      bir::route1_find_materialization_availability(
          route1_before_product_query,
          bir::Value::named(bir::TypeKind::I64, "%product")) ||
      bir::route1_evaluate_same_block_integer_constant(
          route1_before_product_query,
          bir::Value::named(bir::TypeKind::I64, "%product"))
          .has_value()) {
    return fail("BIR Route 1 same-block queries should fail closed for future producers like prepared oracle");
  }
  if (!prepared_and_route1_materialization_availability_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_before_product_query,
          bir::Value::named(bir::TypeKind::I64, "%product"))) {
    return fail("BIR Route 1 materialization availability should match prepared future-producer fail-closed oracle");
  }
  if (prepare::find_prepared_current_block_publication_consumption(
          names,
          &source_producers,
          block_label,
          &block,
          product_name,
          7)
          .available) {
    return fail("current-block publication consumption query should fail closed for future producers");
  }
  if (!prepared_and_bir_current_block_publication_identity_match(
          names,
          source_producers,
          block_label,
          block,
          product_name,
          bir::TypeKind::I64,
          7)) {
    return fail("BIR/prepared current-block publication identity should fail closed before producer");
  }
  if (prepare::evaluate_prepared_same_block_integer_constant(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          block.insts.size())
          .has_value()) {
    return fail("prepared integer constant query should fail closed for missing facts");
  }
  if (prepare::find_prepared_same_block_scalar_producer(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          block.insts.size())
          .has_value() ||
      mir::find_same_block_scalar_producer(
          bir_query, bir::Value::named(bir::TypeKind::I64, "%missing"))
          .has_value()) {
    return fail("same-block scalar producer queries should both fail closed for missing facts");
  }
  if (bir::route1_find_same_block_scalar_producer(
          route1_query, bir::Value::named(bir::TypeKind::I64, "%missing"))
          .has_value() ||
      bir::route1_find_materialization_availability(
          route1_query, bir::Value::named(bir::TypeKind::I64, "%missing")) ||
      bir::route1_evaluate_same_block_integer_constant(
          route1_query, bir::Value::named(bir::TypeKind::I64, "%missing"))
          .has_value()) {
    return fail("Route 1 same-block scalar producer queries should fail closed for missing facts");
  }
  if (!prepared_and_route1_materialization_availability_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I64, "%missing"))) {
    return fail("BIR Route 1 materialization availability should match prepared missing-fact fail-closed oracle");
  }
  if (mir::find_same_block_scalar_producer(
          bir_query, bir::Value::named(bir::TypeKind::I32, "%sum"))
          .has_value()) {
    return fail("BIR scalar producer query should fail closed for mismatched value type");
  }
  if (bir::route1_find_same_block_scalar_producer(
          route1_query, bir::Value::named(bir::TypeKind::I32, "%sum"))
          .has_value()) {
    return fail("BIR Route 1 scalar producer query should fail closed for mismatched value type");
  }
  if (!prepared_and_route1_materialization_availability_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I32, "%sum"))) {
    return fail("BIR Route 1 materialization availability should match prepared mismatched-type fail-closed oracle");
  }
  if (!prepared_and_bir_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          bir_query,
          bir::Value::named(bir::TypeKind::I32, "%sum")) ||
      !prepared_and_route1_integer_constants_both_fail(
          names,
          source_producers,
          block_label,
          block,
          route1_query,
          bir::Value::named(bir::TypeKind::I32, "%sum"))) {
    return fail("integer constant equivalence should fail closed for mismatched value type");
  }

  auto mismatched_source_producers = source_producers;
  mismatched_source_producers.producers_by_value_name[sum_name]
      .instruction_index = 1;
  if (prepare::find_prepared_same_block_scalar_producer(
          names,
          &mismatched_source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%sum"),
          block.insts.size())
          .has_value()) {
    return fail("prepared scalar producer query should fail closed on mismatched producer facts");
  }
  if (prepare::find_prepared_current_block_publication_consumption(
          names,
          &mismatched_source_producers,
          block_label,
          &block,
          sum_name,
          block.insts.size())
          .available) {
    return fail("current-block publication consumption query should fail closed on mismatched producer facts");
  }
  const auto bir_sum_after_mismatched_prepared_fact =
      mir::find_bir_current_block_publication_identity(
          mir::BirCurrentBlockPublicationIdentityRequest{
              .block = &block,
              .block_label = block.label,
              .root_value_name = "%sum",
              .root_value_type = bir::TypeKind::I64,
              .before_instruction_index = block.insts.size(),
          });
  if (!bir_sum_after_mismatched_prepared_fact.available ||
      bir_sum_after_mismatched_prepared_fact.instruction != &block.insts[2] ||
      bir_sum_after_mismatched_prepared_fact.source_producer_kind !=
          mir::SameBlockProducerKind::Binary) {
    return fail("BIR current-block publication identity should remain block-derived when prepared facts are mismatched");
  }
  if (prepare::evaluate_prepared_same_block_integer_constant(
          names,
          &mismatched_source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I64, "%product"),
          block.insts.size())
          .has_value()) {
    return fail("prepared integer constant query should fail closed on mismatched producer facts");
  }

  return 0;
}

int verify_bir_block_entry_publication_identity_lookup() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("entry_publication");
  const auto successor_label = names.block_labels.intern("entry_publication.join");
  const auto wrong_successor_label =
      names.block_labels.intern("entry_publication.other");
  const auto destination_name = names.value_names.intern("%entry.dst");
  const auto unpublished_name = names.value_names.intern("%entry.unpublished");
  const auto stack_only_name = names.value_names.intern("%entry.stack");

  prepare::PreparedValueLocationFunction locations{
      .function_name = function_name,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 101,
              .function_name = function_name,
              .value_name = destination_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"r10"},
          },
          prepare::PreparedValueHome{
              .value_id = 102,
              .function_name = function_name,
              .value_name = unpublished_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string{"r11"},
          },
          prepare::PreparedValueHome{
              .value_id = 103,
              .function_name = function_name,
              .value_name = stack_only_name,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 7,
              .offset_bytes = 56,
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_name,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind =
                  prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .source_parallel_copy_successor_label = successor_label,
              .moves = {
                  prepare::PreparedMoveResolution{
                      .to_value_id = 101,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::Register,
                      .destination_register_name = std::string{"r10"},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
                  prepare::PreparedMoveResolution{
                      .to_value_id = 103,
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind =
                          prepare::PreparedMoveStorageKind::StackSlot,
                      .destination_stack_offset_bytes = std::size_t{56},
                      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                      .authority_kind =
                          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  },
              },
          },
      },
  };
  const auto value_home_lookups =
      prepare::make_prepared_value_home_lookups(&locations);
  const prepare::PreparedCurrentBlockEntryPublicationQueryInputs query{
      .names = &names,
      .value_locations = &locations,
      .value_home_lookups = &value_home_lookups,
      .successor_label = successor_label,
  };

  bir::Block successor;
  successor.label = "entry_publication.join";
  successor.label_id = successor_label;
  successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%entry.dst"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry_publication.left",
              .value = bir::Value::named(bir::TypeKind::I32, "%entry.src"),
          },
      },
  });
  successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%entry.unpublished"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry_publication.left",
              .value = bir::Value::named(bir::TypeKind::I32, "%entry.other_src"),
          },
      },
  });
  successor.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%entry.stack"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry_publication.left",
              .value = bir::Value::named(bir::TypeKind::I32, "%entry.stack_src"),
          },
      },
  });

  const auto prepared_available =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{101});
  const auto& available_destination =
      std::get<bir::PhiInst>(successor.insts.front()).result;
  const auto bir_available = mir::find_bir_block_entry_publication_identity(
      mir::BirBlockEntryPublicationIdentityRequest{
          .successor_block = &successor,
          .successor_label = successor.label,
          .successor_label_id = successor_label,
          .destination_value = &available_destination,
          .destination_value_id = prepared_available.destination_value_id,
          .destination_value_name = "%entry.dst",
          .destination_value_name_id = destination_name,
          .destination_value_type = bir::TypeKind::I32,
      });
  const auto route4_available =
      bir::route4_block_entry_publication_record(&successor,
                                                 available_destination,
                                                 destination_name);
  const auto route4_available_value =
      bir::route4_block_entry_publication_value_record(&successor,
                                                       available_destination,
                                                       destination_name);
  if (prepared_available.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      !bir_available.available ||
      bir_available.status != mir::BirBlockEntryPublicationStatus::Available ||
      !route4_available ||
      route4_available.status !=
          bir::Route4PublicationAvailabilityStatus::Available ||
      route4_available.destination_value.value != &available_destination ||
      route4_available.destination_value.name != "%entry.dst" ||
      route4_available.destination_value.name_id != destination_name ||
      route4_available.destination_value_type != bir::TypeKind::I32 ||
      route4_available.destination_instruction != &successor.insts.front() ||
      route4_available.phi !=
          std::get_if<bir::PhiInst>(&successor.insts.front()) ||
      route4_available_value.scope != bir::Route4PublicationScope::BlockEntry ||
      route4_available_value.value_role !=
          bir::Route4PublicationValueRole::Consumed ||
      route4_available_value.value.value == nullptr ||
      route4_available_value.value.name != "%entry.dst" ||
      bir_available.destination_value_id != prepared_available.destination_value_id ||
      bir_available.destination_value_name_id !=
          prepared_available.destination_value_name ||
      bir_available.destination_value == nullptr ||
      bir_available.destination_value_identity.value !=
          bir_available.destination_value ||
      bir_available.destination_value_identity.name !=
          bir_available.destination_value_name ||
      bir_available.destination_value_name != "%entry.dst") {
    return fail("BIR block-entry publication identity should match prepared available destination semantics");
  }

  bir::Block no_phi_successor;
  no_phi_successor.label = "entry_publication.join";
  no_phi_successor.label_id = successor_label;
  const auto missing_phi_destination =
      bir::Value::named(bir::TypeKind::I32, "%entry.dst");
  const auto bir_missing_phi_for_prepared_ready =
      mir::find_bir_block_entry_publication_identity(
          mir::BirBlockEntryPublicationIdentityRequest{
              .successor_block = &no_phi_successor,
              .successor_label = no_phi_successor.label,
              .successor_label_id = successor_label,
              .destination_value_id = prepared_available.destination_value_id,
              .destination_value_name = "%entry.dst",
              .destination_value_name_id =
                  prepared_available.destination_value_name,
              .destination_value_type = bir::TypeKind::I32,
          });
  const auto route4_missing_phi_for_prepared_ready =
      bir::route4_block_entry_publication_record(
          &no_phi_successor, missing_phi_destination, destination_name);
  if (prepared_available.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::Available ||
      bir_missing_phi_for_prepared_ready.available ||
      bir_missing_phi_for_prepared_ready.status !=
          mir::BirBlockEntryPublicationStatus::MissingPublication ||
      route4_missing_phi_for_prepared_ready ||
      route4_missing_phi_for_prepared_ready.status !=
          bir::Route4PublicationAvailabilityStatus::MissingPublication) {
    return fail("prepared move/register readiness should not imply BIR PHI-entry identity");
  }
  bir::Function route4_entry_function;
  route4_entry_function.blocks.push_back(successor);
  route4_entry_function.blocks.push_back(no_phi_successor);
  const auto& route4_entry_successor = route4_entry_function.blocks.front();
  const auto& route4_no_phi_successor = route4_entry_function.blocks.back();
  const auto route4_entry_publications =
      bir::route4_build_publication_availability_index(route4_entry_function);
  const auto route4_entry_facade =
      bir::route_index_reference_facade(route4_entry_publications);
  const auto indexed_entry_available_ref =
      bir::route4_validate_block_entry_publication_reference(
          route4_entry_publications,
          route4_entry_successor,
          available_destination);
  const auto facade_entry_available_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route4_entry_facade,
          route4_entry_successor,
          available_destination);
  const auto* available_block_entry_record =
      indexed_entry_available_ref.block_entry_record;
  if (!route4_entry_publications ||
      !indexed_entry_available_ref ||
      indexed_entry_available_ref.status !=
          bir::RouteIndexValidationStatus::Valid ||
      indexed_entry_available_ref.route_status !=
          bir::Route4PublicationAvailabilityStatus::Available ||
      indexed_entry_available_ref.reference.record_category !=
          bir::RouteIndexRecordCategory::Route4BlockEntryPublication ||
      indexed_entry_available_ref.reference.relationship !=
          bir::RouteIndexRelationshipKind::Route4BlockEntryPublication ||
      available_block_entry_record == nullptr ||
      available_block_entry_record->destination_value_name != "%entry.dst" ||
      available_block_entry_record->destination_value_type !=
          bir::TypeKind::I32 ||
      available_block_entry_record->destination_instruction_index !=
          std::size_t{0} ||
      available_block_entry_record->destination_instruction !=
          &route4_entry_successor.insts[0] ||
      !facade_entry_available_ref ||
      facade_entry_available_ref.block_entry_record !=
          available_block_entry_record ||
      facade_entry_available_ref.reference.record_index !=
          indexed_entry_available_ref.reference.record_index) {
    return fail("Route 4 block-entry publication index/reference should find available PHI record");
  }
  const auto indexed_entry_type_mismatch_ref =
      bir::route4_validate_block_entry_publication_reference(
          route4_entry_publications,
          route4_entry_successor,
          bir::Value::named(bir::TypeKind::I64, "%entry.dst"));
  const auto indexed_entry_missing_ref =
      bir::route4_validate_block_entry_publication_reference(
          route4_entry_publications,
          route4_no_phi_successor,
          missing_phi_destination);
  const auto indexed_entry_missing_value_ref =
      bir::route4_validate_block_entry_publication_reference(
          route4_entry_publications,
          route4_entry_successor,
          bir::Value::named(bir::TypeKind::I32, ""));
  const auto indexed_entry_stale_ref =
      bir::route4_validate_block_entry_publication_reference(
          route4_entry_publications, successor, available_destination);
  const auto indexed_entry_wrong_relationship_ref =
      bir::route4_validate_current_block_publication_reference(
          route4_entry_publications,
          route4_entry_successor,
          available_destination,
          route4_entry_successor.insts.size());
  const auto facade_missing_index_ref =
      bir::route_index_validate_block_entry_publication_reference(
          bir::RouteIndexReferenceFacade{},
          route4_entry_successor,
          available_destination);
  const bir::Route4PublicationAvailabilityIndex empty_route4_entry_publications;
  const auto empty_route4_entry_facade =
      bir::route_index_reference_facade(empty_route4_entry_publications);
  const auto facade_empty_index_ref =
      bir::route_index_validate_block_entry_publication_reference(
          empty_route4_entry_facade,
          route4_entry_successor,
          available_destination);
  auto duplicate_route4_entry_publications = route4_entry_publications;
  duplicate_route4_entry_publications.block_entry_records.push_back(
      *available_block_entry_record);
  const auto indexed_entry_duplicate_ref =
      bir::route4_validate_block_entry_publication_reference(
          duplicate_route4_entry_publications,
          route4_entry_successor,
          available_destination);
  const auto duplicate_route4_entry_facade =
      bir::route_index_reference_facade(duplicate_route4_entry_publications);
  auto diverged_route4_entry_publications = route4_entry_publications;
  diverged_route4_entry_publications.block_entry_records.front()
      .destination_instruction = nullptr;
  const auto diverged_route4_entry_facade =
      bir::route_index_reference_facade(diverged_route4_entry_publications);
  const auto indexed_entry_diverged_ref =
      bir::route4_validate_block_entry_publication_reference(
          diverged_route4_entry_publications,
          route4_entry_successor,
          available_destination);
  const auto facade_entry_type_mismatch_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route4_entry_facade,
          route4_entry_successor,
          bir::Value::named(bir::TypeKind::I64, "%entry.dst"));
  const auto facade_entry_missing_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route4_entry_facade,
          route4_no_phi_successor,
          missing_phi_destination);
  const auto facade_entry_missing_value_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route4_entry_facade,
          route4_entry_successor,
          bir::Value::named(bir::TypeKind::I32, ""));
  const auto facade_entry_stale_ref =
      bir::route_index_validate_block_entry_publication_reference(
          route4_entry_facade, successor, available_destination);
  const auto facade_entry_wrong_relationship_ref =
      bir::route_index_validate_current_block_publication_reference(
          route4_entry_facade,
          route4_entry_successor,
          available_destination,
          route4_entry_successor.insts.size());
  const auto facade_entry_duplicate_ref =
      bir::route_index_validate_block_entry_publication_reference(
          duplicate_route4_entry_facade,
          route4_entry_successor,
          available_destination);
  const auto facade_entry_diverged_ref =
      bir::route_index_validate_block_entry_publication_reference(
          diverged_route4_entry_facade,
          route4_entry_successor,
          available_destination);
  if (indexed_entry_type_mismatch_ref ||
      indexed_entry_type_mismatch_ref.status !=
          bir::RouteIndexValidationStatus::WrongKey ||
      indexed_entry_type_mismatch_ref.route_status !=
          bir::Route4PublicationAvailabilityStatus::NoMatch ||
      indexed_entry_missing_ref ||
      indexed_entry_missing_ref.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      indexed_entry_missing_ref.route_status !=
          bir::Route4PublicationAvailabilityStatus::MissingPublication ||
      indexed_entry_missing_value_ref ||
      indexed_entry_missing_value_ref.route_status !=
          bir::Route4PublicationAvailabilityStatus::MissingValue ||
      indexed_entry_stale_ref ||
      indexed_entry_stale_ref.status !=
          bir::RouteIndexValidationStatus::StaleOwner ||
      indexed_entry_wrong_relationship_ref ||
      indexed_entry_wrong_relationship_ref.status !=
          bir::RouteIndexValidationStatus::WrongRelationship ||
      facade_missing_index_ref ||
      facade_missing_index_ref.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      facade_empty_index_ref ||
      facade_empty_index_ref.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      indexed_entry_duplicate_ref ||
      indexed_entry_duplicate_ref.status !=
          bir::RouteIndexValidationStatus::DuplicateReference ||
      indexed_entry_diverged_ref ||
      indexed_entry_diverged_ref.status !=
          bir::RouteIndexValidationStatus::Diverged ||
      facade_entry_type_mismatch_ref.status !=
          indexed_entry_type_mismatch_ref.status ||
      facade_entry_missing_ref.status != indexed_entry_missing_ref.status ||
      facade_entry_missing_value_ref.status !=
          indexed_entry_missing_value_ref.status ||
      facade_entry_stale_ref.status != indexed_entry_stale_ref.status ||
      facade_entry_wrong_relationship_ref.status !=
          indexed_entry_wrong_relationship_ref.status ||
      facade_entry_duplicate_ref.status != indexed_entry_duplicate_ref.status ||
      facade_entry_diverged_ref.status != indexed_entry_diverged_ref.status) {
    return fail("Route 4 block-entry publication facade should match direct reference validation for missing, type, stale, wrong-relationship, duplicate, and diverged cases");
  }

  const auto prepared_missing =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{102});
  const auto bir_missing = mir::find_bir_block_entry_publication_identity(
      mir::BirBlockEntryPublicationIdentityRequest{
          .successor_block = &no_phi_successor,
          .successor_label = no_phi_successor.label,
          .successor_label_id = successor_label,
          .destination_value_id = prepared_missing.destination_value_id,
          .destination_value_name = "%entry.unpublished",
          .destination_value_name_id = prepared_missing.destination_value_name,
          .destination_value_type = bir::TypeKind::I32,
      });
  if (prepared_missing.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication ||
      bir_missing.available ||
      bir_missing.status != mir::BirBlockEntryPublicationStatus::MissingPublication ||
      bir_missing.destination_value_id != prepared_missing.destination_value_id ||
      bir_missing.destination_value_name_id !=
          prepared_missing.destination_value_name) {
    return fail("BIR block-entry publication identity should match prepared missing-publication category");
  }

  const auto bir_unpublished_phi = mir::find_bir_block_entry_publication_identity(
      mir::BirBlockEntryPublicationIdentityRequest{
          .successor_block = &successor,
          .successor_label = successor.label,
          .successor_label_id = successor_label,
          .destination_value_id = prepared_missing.destination_value_id,
          .destination_value_name = "%entry.unpublished",
          .destination_value_name_id = prepared_missing.destination_value_name,
          .destination_value_type = bir::TypeKind::I32,
      });
  const auto unpublished_destination =
      bir::Value::named(bir::TypeKind::I32, "%entry.unpublished");
  const auto route4_unpublished_phi =
      bir::route4_block_entry_publication_record(
          &successor, unpublished_destination, prepared_missing.destination_value_name);
  if (prepared_missing.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication ||
      !bir_unpublished_phi.available ||
      !route4_unpublished_phi ||
      route4_unpublished_phi.destination_value_name != "%entry.unpublished" ||
      route4_unpublished_phi.destination_value_name_id !=
          prepared_missing.destination_value_name ||
      bir_unpublished_phi.destination_value_name != "%entry.unpublished" ||
      bir_unpublished_phi.destination_value_name_id !=
          prepared_missing.destination_value_name) {
    return fail("BIR PHI-entry identity should not imply prepared move publication readiness");
  }

  const auto prepared_stack_only =
      prepare::find_prepared_current_block_entry_publication(
          query, prepare::PreparedValueId{103});
  const auto bir_stack_phi = mir::find_bir_block_entry_publication_identity(
      mir::BirBlockEntryPublicationIdentityRequest{
          .successor_block = &successor,
          .successor_label = successor.label,
          .successor_label_id = successor_label,
          .destination_value_id = prepared_stack_only.destination_value_id,
          .destination_value_name = "%entry.stack",
          .destination_value_name_id = prepared_stack_only.destination_value_name,
          .destination_value_type = bir::TypeKind::I32,
      });
  const auto stack_destination =
      bir::Value::named(bir::TypeKind::I32, "%entry.stack");
  const auto route4_stack_phi =
      bir::route4_block_entry_publication_record(
          &successor, stack_destination, prepared_stack_only.destination_value_name);
  if (prepared_stack_only.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::
              PublicationUnavailable ||
      !bir_stack_phi.available ||
      !route4_stack_phi ||
      route4_stack_phi.status !=
          bir::Route4PublicationAvailabilityStatus::Available ||
      bir_stack_phi.destination_value_name_id !=
          prepared_stack_only.destination_value_name) {
    return fail("BIR PHI-entry identity should not import prepared destination storage readiness");
  }

  const auto prepared_wrong_successor =
      prepare::find_prepared_current_block_entry_publication(
          prepare::PreparedCurrentBlockEntryPublicationQueryInputs{
              .value_locations = &locations,
              .value_home_lookups = &value_home_lookups,
              .successor_label = wrong_successor_label,
          },
          prepare::PreparedValueId{101});
  const auto bir_wrong_successor =
      mir::find_bir_block_entry_publication_identity(
          mir::BirBlockEntryPublicationIdentityRequest{
              .successor_block = &successor,
              .successor_label = "entry_publication.other",
              .destination_value_id = prepared_wrong_successor.destination_value_id,
              .destination_value_name = "%entry.dst",
              .destination_value_name_id =
                  prepared_wrong_successor.destination_value_name,
              .destination_value_type = bir::TypeKind::I32,
          });
  if (prepared_wrong_successor.status !=
          prepare::PreparedCurrentBlockEntryPublicationStatus::MissingPublication ||
      bir_wrong_successor.available ||
      bir_wrong_successor.status !=
          mir::BirBlockEntryPublicationStatus::MissingSuccessorLabel) {
    return fail("BIR block-entry publication identity should fail closed for wrong successor");
  }

  const auto bir_missing_destination_key =
      mir::find_bir_block_entry_publication_identity(
          mir::BirBlockEntryPublicationIdentityRequest{
              .successor_block = &successor,
              .successor_label = successor.label,
              .successor_label_id = successor_label,
              .destination_value_type = bir::TypeKind::I32,
          });
  if (bir_missing_destination_key.available ||
      bir_missing_destination_key.status !=
          mir::BirBlockEntryPublicationStatus::MissingDestinationValue) {
    return fail("BIR block-entry publication identity should reject missing destination keys");
  }

  const auto bir_destination_type_mismatch =
      mir::find_bir_block_entry_publication_identity(
          mir::BirBlockEntryPublicationIdentityRequest{
              .successor_block = &successor,
              .successor_label = successor.label,
              .successor_label_id = successor_label,
              .destination_value_name = "%entry.dst",
              .destination_value_name_id = destination_name,
              .destination_value_type = bir::TypeKind::I64,
          });
  if (bir_destination_type_mismatch.available ||
      bir_destination_type_mismatch.status !=
          mir::BirBlockEntryPublicationStatus::MissingDestinationValue ||
      bir_destination_type_mismatch.destination_value_name != "%entry.dst" ||
      bir_destination_type_mismatch.destination_value_type != bir::TypeKind::I64) {
    return fail("BIR block-entry publication identity should fail closed for destination type mismatch");
  }

  const auto bir_wrong_value = mir::find_bir_block_entry_publication_identity(
      mir::BirBlockEntryPublicationIdentityRequest{
          .successor_block = &successor,
          .successor_label = successor.label,
          .successor_label_id = successor_label,
          .destination_value_name = "%entry.wrong",
          .destination_value_type = bir::TypeKind::I32,
      });
  const auto route4_wrong_value =
      bir::route4_block_entry_publication_record(
          &successor,
          bir::Value::named(bir::TypeKind::I32, "%entry.wrong"));
  if (bir_wrong_value.available ||
      bir_wrong_value.status !=
          mir::BirBlockEntryPublicationStatus::MissingPublication ||
      route4_wrong_value ||
      route4_wrong_value.status !=
          bir::Route4PublicationAvailabilityStatus::MissingPublication) {
    return fail("BIR block-entry publication identity should fail closed for wrong destination value");
  }

  return 0;
}

int verify_bir_call_argument_publication_source_routing_lookup() {
  prepare::PreparedNameTables names;
  const auto base_name = names.value_names.intern("%base.ptr");
  const auto frame_value_name = names.value_names.intern("%frame.value");
  const auto selected_global_name = names.value_names.intern("%selected.global");
  const auto block_label = names.block_labels.intern("entry");

  const prepare::PreparedCallArgumentPlan scalar_argument{
      .arg_index = 0,
      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
      .source_value_id = prepare::PreparedValueId{101},
  };
  const prepare::PreparedCallArgumentPlan computed_argument{
      .arg_index = 1,
      .source_encoding = prepare::PreparedStorageEncodingKind::ComputedAddress,
      .source_value_id = prepare::PreparedValueId{102},
      .source_base_value_id = prepare::PreparedValueId{201},
      .source_base_value_name = base_name,
      .source_pointer_byte_delta = std::int64_t{12},
  };
  const prepare::PreparedCallArgumentPlan frame_argument{
      .arg_index = 2,
      .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
      .source_value_id = prepare::PreparedValueId{103},
      .source_selection =
          prepare::PreparedCallArgumentSourceSelection{
              .kind =
                  prepare::PreparedCallArgumentSourceSelectionKind::
                      FrameSlotValue,
              .source_value_id = prepare::PreparedValueId{103},
              .source_value_name = frame_value_name,
              .source_slot_id = prepare::PreparedFrameSlotId{7},
              .source_stack_offset_bytes = std::size_t{32},
              .source_size_bytes = std::size_t{8},
              .source_align_bytes = std::size_t{8},
          },
  };
  const prepare::PreparedCallArgumentPlan direct_global_argument{
      .arg_index = 3,
      .source_encoding = prepare::PreparedStorageEncodingKind::Register,
      .source_value_id = prepare::PreparedValueId{104},
      .direct_global_select_chain_dependency =
          prepare::PreparedCallArgumentDirectGlobalSelectChainDependency{
              .available = true,
              .source_value_name = selected_global_name,
              .direct_global_dependency =
                  prepare::PreparedDirectGlobalSelectChainDependency{
                      .contains_direct_global_load = true,
                      .root_is_select = true,
                      .root_instruction_index = std::size_t{5},
                  },
          },
  };
  const prepare::PreparedCallArgumentPlan local_frame_address_argument{
      .arg_index = 4,
      .source_selection =
          prepare::PreparedCallArgumentSourceSelection{
              .kind =
                  prepare::PreparedCallArgumentSourceSelectionKind::
                      LocalFrameAddressMaterialization,
              .source_value_id = prepare::PreparedValueId{105},
              .source_base_value_id = prepare::PreparedValueId{205},
              .source_pointer_byte_delta = std::int64_t{-4},
              .address_materialization_block_label = block_label,
              .address_materialization_inst_index = std::size_t{9},
              .address_materialization_frame_slot_id =
                  prepare::PreparedFrameSlotId{8},
              .address_materialization_byte_offset = std::int64_t{40},
          },
  };

  bir::CallInst call{
      .callee = "consume_sources",
      .args =
          {
              bir::Value::named(bir::TypeKind::I64, "%scalar"),
              bir::Value::named(bir::TypeKind::Ptr, "%computed"),
              bir::Value::named(bir::TypeKind::I64, "%frame"),
              bir::Value::named(bir::TypeKind::Ptr, "%selected.global"),
              bir::Value::named(bir::TypeKind::Ptr, "%frame.addr"),
          },
      .arg_types =
          {
              bir::TypeKind::I64,
              bir::TypeKind::Ptr,
              bir::TypeKind::I64,
              bir::TypeKind::Ptr,
              bir::TypeKind::Ptr,
          },
      .arg_sources =
          {
              bir::CallArgumentSourceRelationship{
                  .arg_index = 0,
                  .source_encoding =
                      bir::CallArgumentSourceEncodingKind::Register,
                  .source_value_id = std::size_t{101},
              },
              bir::CallArgumentSourceRelationship{
                  .arg_index = 1,
                  .source_encoding =
                      bir::CallArgumentSourceEncodingKind::ComputedAddress,
                  .source_value_id = std::size_t{102},
                  .source_base_value_id = std::size_t{201},
                  .source_base_value_name = std::string{"%base.ptr"},
                  .source_pointer_byte_delta = std::int64_t{12},
              },
              bir::CallArgumentSourceRelationship{
                  .arg_index = 2,
                  .source_encoding =
                      bir::CallArgumentSourceEncodingKind::FrameSlot,
                  .source_value_id = std::size_t{103},
                  .source_selection =
                      bir::CallArgumentSourceSelection{
                          .kind =
                              bir::CallArgumentSourceSelectionKind::
                                  FrameSlotValue,
                          .source_value_id = std::size_t{103},
                          .source_value_name = std::string{"%frame.value"},
                          .source_slot_id = c4c::SlotNameId{7},
                          .source_stack_offset_bytes = std::size_t{32},
                          .source_size_bytes = std::size_t{8},
                          .source_align_bytes = std::size_t{8},
                      },
              },
              bir::CallArgumentSourceRelationship{
                  .arg_index = 3,
                  .source_encoding =
                      bir::CallArgumentSourceEncodingKind::Register,
                  .source_value_id = std::size_t{104},
                  .direct_global_select_chain_dependency =
                      bir::CallArgumentDirectGlobalSelectChainDependency{
                          .available = true,
                          .source_value_name =
                              std::string{"%selected.global"},
                          .contains_direct_global_load = true,
                          .root_is_select = true,
                          .root_instruction_index = std::size_t{5},
                      },
              },
              bir::CallArgumentSourceRelationship{
                  .arg_index = 4,
                  .source_selection =
                      bir::CallArgumentSourceSelection{
                          .kind =
                              bir::CallArgumentSourceSelectionKind::
                                  LocalFrameAddressMaterialization,
                          .source_value_id = std::size_t{105},
                          .source_base_value_id = std::size_t{205},
                          .source_pointer_byte_delta = std::int64_t{-4},
                          .address_materialization_block_label = block_label,
                          .address_materialization_inst_index =
                              std::size_t{9},
                          .address_materialization_frame_slot_id =
                              c4c::SlotNameId{8},
                          .address_materialization_byte_offset =
                              std::int64_t{40},
                      },
              },
          },
      .return_type = bir::TypeKind::Void,
  };

  if (!call.arg_abi.empty()) {
    return fail(
        "BIR call-argument source routing fixture should not require ABI placement records");
  }
  if (!prepared_and_bir_call_argument_publication_source_routing_match(
          names, scalar_argument, call, 0) ||
      !prepared_and_bir_call_argument_publication_source_routing_match(
          names, computed_argument, call, 1) ||
      !prepared_and_bir_call_argument_publication_source_routing_match(
          names, frame_argument, call, 2) ||
      !prepared_and_bir_call_argument_publication_source_routing_match(
          names, direct_global_argument, call, 3) ||
      !prepared_and_bir_call_argument_publication_source_routing_match(
          names, local_frame_address_argument, call, 4)) {
    return fail(
        "BIR call-argument source routing should match prepared semantic source oracle fields");
  }

  bir::Block route6_call_block{
      .label = "entry",
      .insts = {call},
      .label_id = block_label,
  };
  const bir::Function route6_call_function{
      .name = "route6_call_sources",
      .blocks = {route6_call_block},
  };
  const auto& route6_indexed_block = route6_call_function.blocks.front();
  const auto* route6_call =
      std::get_if<bir::CallInst>(&route6_indexed_block.insts[0]);
  if (route6_call == nullptr) {
    return fail("Route 6 call-source fixture is malformed");
  }
  const auto route6_call_index =
      bir::route6_build_call_use_source_index(route6_call_function);
  const auto route6_producer_index =
      bir::route1_build_producer_index(route6_indexed_block);
  const auto route6_query = bir::Route1SameBlockProducerQuery{
      .index = &route6_producer_index,
      .before_instruction_index = 0,
  };
  const auto route6_direct_global =
      bir::route6_call_argument_direct_global_dependency_record(
          route6_query, route6_indexed_block, *route6_call, 0, 3);
  const auto indexed_route6_direct_global =
      bir::route6_find_call_argument_direct_global_dependency(
          route6_call_index,
          route6_indexed_block,
          0,
          "consume_sources",
          3);
  if (!route6_direct_global ||
      route6_direct_global.status != bir::Route6CallUseStatus::Available ||
      route6_direct_global.argument_source.source_kind !=
          bir::Route6CallUseSourceKind::DirectGlobalSelectChain ||
      route6_direct_global.source_value_name != "%selected.global" ||
      !route6_direct_global.direct_global_dependency.available ||
      !route6_direct_global.direct_global_dependency
           .contains_direct_global_load ||
      !route6_direct_global.direct_global_dependency.root_is_select ||
      route6_direct_global.direct_global_dependency.root_instruction_index !=
          std::optional<std::size_t>{5} ||
      !indexed_route6_direct_global ||
      indexed_route6_direct_global.direct_global_dependency
              .root_instruction_index != std::optional<std::size_t>{5}) {
    return fail(
        "Route 6 call argument record/index should expose direct-global dependency facts");
  }
  const auto route6_frame_argument_source =
      bir::route6_call_argument_source_record(
          route6_indexed_block, *route6_call, 0, 2);
  const auto indexed_route6_frame_argument_source =
      bir::route6_find_call_argument_source(
          route6_call_index,
          route6_indexed_block,
          0,
          "consume_sources",
          2);
  if (route6_frame_argument_source ||
      route6_frame_argument_source.status !=
          bir::Route6CallUseStatus::AbiBoundExcluded ||
      route6_frame_argument_source.source_kind !=
          bir::Route6CallUseSourceKind::AbiBoundExcluded ||
      indexed_route6_frame_argument_source ||
      indexed_route6_frame_argument_source.status !=
          bir::Route6CallUseStatus::AbiBoundExcluded) {
    return fail(
        "Route 6 call argument source record/index should explicitly exclude ABI-bound source selection facts");
  }
  const auto route6_frame_source =
      bir::route6_call_argument_publication_source_record(
          route6_query, route6_indexed_block, *route6_call, 0, 2);
  const auto indexed_route6_frame_source =
      bir::route6_find_call_argument_publication_source(
          route6_call_index,
          route6_indexed_block,
          0,
          "consume_sources",
          2);
  if (route6_frame_source ||
      route6_frame_source.status !=
          bir::Route6CallUseStatus::AbiBoundExcluded ||
      route6_frame_source.source_kind !=
          bir::Route6CallUseSourceKind::AbiBoundExcluded ||
      !route6_frame_source.abi_bound_excluded ||
      indexed_route6_frame_source ||
      indexed_route6_frame_source.status !=
          bir::Route6CallUseStatus::AbiBoundExcluded ||
      !indexed_route6_frame_source.abi_bound_excluded) {
    return fail(
        "Route 6 call argument publication record/index should explicitly exclude ABI-bound source selection facts");
  }

  auto duplicate_call = call;
  duplicate_call.arg_sources.push_back(call.arg_sources.front());
  if (bir::find_call_argument_source_relationship(duplicate_call, 0) !=
          nullptr ||
      bir::find_call_argument_publication_source_routing(duplicate_call, 0)
          .available) {
    return fail(
        "BIR call-argument source routing should fail closed for duplicate argument source records");
  }
  auto route6_duplicate_block = route6_call_block;
  auto* route6_duplicate_call =
      std::get_if<bir::CallInst>(&route6_duplicate_block.insts[0]);
  if (route6_duplicate_call == nullptr) {
    return fail("Route 6 duplicate call-source fixture is malformed");
  }
  route6_duplicate_call->arg_sources.push_back(
      route6_duplicate_call->arg_sources.front());
  const auto route6_duplicate_source =
      bir::route6_call_argument_source_record(
          route6_duplicate_block, *route6_duplicate_call, 0, 0);
  const bir::Function route6_duplicate_function{
      .name = "route6_duplicate_sources",
      .blocks = {route6_duplicate_block},
  };
  const auto route6_duplicate_index =
      bir::route6_build_call_use_source_index(route6_duplicate_function);
  const auto indexed_route6_duplicate_source =
      bir::route6_find_call_argument_source(
          route6_duplicate_index,
          route6_duplicate_function.blocks.front(),
          0,
          "consume_sources",
          0);
  if (route6_duplicate_source ||
      route6_duplicate_source.status !=
          bir::Route6CallUseStatus::DuplicateRelationship ||
      indexed_route6_duplicate_source ||
      indexed_route6_duplicate_source.status !=
          bir::Route6CallUseStatus::DuplicateRelationship) {
    return fail(
        "Route 6 call argument source record/index should fail closed for duplicate argument source records");
  }

  if (bir::find_call_argument_source_relationship(call, 99) != nullptr ||
      bir::find_call_argument_publication_source_routing(call, 99)
          .available) {
    return fail(
        "BIR call-argument source routing should fail closed for out-of-range argument indexes");
  }
  const auto route6_missing_argument =
      bir::route6_call_argument_source_record(
          route6_indexed_block, *route6_call, 0, 99);
  const auto indexed_route6_wrong_call =
      bir::route6_find_call_argument_source(
          route6_call_index,
          route6_indexed_block,
          0,
          "consume_wrong",
          0);
  if (route6_missing_argument ||
      route6_missing_argument.status !=
          bir::Route6CallUseStatus::MissingArgument ||
      indexed_route6_wrong_call ||
      indexed_route6_wrong_call.status != bir::Route6CallUseStatus::WrongCall) {
    return fail(
        "Route 6 call argument source record/index should fail closed for missing arguments and wrong calls");
  }

  bir::CallInst unavailable_call{
      .callee = "consume_unavailable",
      .args = {bir::Value::named(bir::TypeKind::I32, "%arg")},
      .arg_sources =
          {
              bir::CallArgumentSourceRelationship{
                  .arg_index = 0,
              },
          },
  };
  const prepare::PreparedCallArgumentPlan unavailable_argument{
      .arg_index = 0,
  };
  if (!prepared_and_bir_call_argument_publication_source_routing_match(
          names, unavailable_argument, unavailable_call, 0)) {
    return fail(
        "BIR call-argument source routing should match prepared unavailable source routing");
  }

  return 0;
}

int verify_bir_call_argument_source_producer_materialization_lookup() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("entry");
  const auto loaded_name = names.value_names.intern("%loaded.arg");
  const auto sum_name = names.value_names.intern("%sum.arg");
  const auto quotient_name = names.value_names.intern("%quotient.arg");
  const auto missing_name = names.value_names.intern("%missing.arg");
  const auto after_name = names.value_names.intern("%after.arg");

  bir::Block block{
      .label = "entry",
      .insts =
          {
              bir::LoadLocalInst{
                  .result =
                      bir::Value::named(bir::TypeKind::I64, "%loaded.arg"),
                  .slot_name = "slot0",
                  .slot_id = c4c::SlotNameId{3},
                  .byte_offset = 0,
                  .align_bytes = 8,
                  .address =
                      bir::MemoryAddress{
                          .base_kind =
                              bir::MemoryAddress::BaseKind::LocalSlot,
                          .base_name = "slot0",
                          .byte_offset = 0,
                          .size_bytes = 8,
                          .align_bytes = 8,
                          .base_slot_id = c4c::SlotNameId{3},
                      },
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result =
                      bir::Value::named(bir::TypeKind::I64, "%sum.arg"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%loaded.arg"),
                  .rhs = bir::Value::immediate_i64(4),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::UDiv,
                  .result = bir::Value::named(
                      bir::TypeKind::I64, "%quotient.arg"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%sum.arg"),
                  .rhs = bir::Value::immediate_i64(2),
              },
              bir::CallInst{
                  .callee = "consume_materialized_sources",
                  .args =
                      {
                          bir::Value::named(
                              bir::TypeKind::I64, "%loaded.arg"),
                          bir::Value::named(bir::TypeKind::I64, "%sum.arg"),
                          bir::Value::named(
                              bir::TypeKind::I64, "%quotient.arg"),
                          bir::Value::named(
                              bir::TypeKind::I64, "%missing.arg"),
                          bir::Value::named(bir::TypeKind::I64, "%after.arg"),
                      },
                  .arg_sources =
                      {
                          bir::CallArgumentSourceRelationship{
                              .arg_index = 0,
                              .source_encoding =
                                  bir::CallArgumentSourceEncodingKind::
                                      Register,
                              .source_value_id = std::size_t{100},
                              .source_value_name =
                                  std::string{"%loaded.arg"},
                          },
                          bir::CallArgumentSourceRelationship{
                              .arg_index = 1,
                              .source_encoding =
                                  bir::CallArgumentSourceEncodingKind::
                                      Register,
                              .source_value_id = std::size_t{101},
                              .source_value_name = std::string{"%sum.arg"},
                          },
                          bir::CallArgumentSourceRelationship{
                              .arg_index = 2,
                              .source_encoding =
                                  bir::CallArgumentSourceEncodingKind::
                                      Register,
                              .source_value_id = std::size_t{102},
                              .source_value_name =
                                  std::string{"%quotient.arg"},
                          },
                          bir::CallArgumentSourceRelationship{
                              .arg_index = 3,
                              .source_encoding =
                                  bir::CallArgumentSourceEncodingKind::
                                      Register,
                              .source_value_id = std::size_t{103},
                              .source_value_name =
                                  std::string{"%missing.arg"},
                          },
                          bir::CallArgumentSourceRelationship{
                              .arg_index = 4,
                              .source_encoding =
                                  bir::CallArgumentSourceEncodingKind::
                                      Register,
                              .source_value_id = std::size_t{104},
                              .source_value_name =
                                  std::string{"%after.arg"},
                          },
                      },
                  .return_type = bir::TypeKind::Void,
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result =
                      bir::Value::named(bir::TypeKind::I64, "%after.arg"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%sum.arg"),
                  .rhs = bir::Value::immediate_i64(8),
              },
          },
      .label_id = block_label,
  };

  constexpr std::size_t call_instruction_index = 3;
  const auto* loaded = std::get_if<bir::LoadLocalInst>(&block.insts[0]);
  const auto* sum = std::get_if<bir::BinaryInst>(&block.insts[1]);
  const auto* quotient = std::get_if<bir::BinaryInst>(&block.insts[2]);
  const auto* call =
      std::get_if<bir::CallInst>(&block.insts[call_instruction_index]);
  const auto* after = std::get_if<bir::BinaryInst>(&block.insts[4]);
  if (loaded == nullptr || sum == nullptr || quotient == nullptr ||
      call == nullptr || after == nullptr) {
    return fail("BIR call-argument materialization fixture is malformed");
  }

  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      loaded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal,
          .block_label = block_label,
          .instruction_index = 0,
          .load_local = loaded,
      });
  source_producers.producers_by_value_name.emplace(
      sum_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 1,
          .binary = sum,
      });
  source_producers.producers_by_value_name.emplace(
      quotient_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 2,
          .binary = quotient,
      });
  source_producers.producers_by_value_name.emplace(
      missing_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Unknown,
          .block_label = block_label,
      });
  source_producers.producers_by_value_name.emplace(
      after_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 4,
          .binary = after,
      });

  if (!prepared_and_bir_call_argument_source_producer_materialization_match(
          names, source_producers, block_label, block, *call,
          call_instruction_index, 0) ||
      !prepared_and_bir_call_argument_source_producer_materialization_match(
          names, source_producers, block_label, block, *call,
          call_instruction_index, 1)) {
    return fail(
        "BIR call-argument source-producer materialization should match prepared load-local and materializable binary oracle facts");
  }
  const auto route6_producer_index = bir::route1_build_producer_index(block);
  const auto route6_query = bir::Route1SameBlockProducerQuery{
      .index = &route6_producer_index,
      .before_instruction_index = call_instruction_index,
  };
  const bir::Function route6_materialization_function{
      .name = "route6_materialized_sources",
      .blocks = {block},
  };
  const auto& route6_materialization_block =
      route6_materialization_function.blocks.front();
  const auto* route6_materialization_call = std::get_if<bir::CallInst>(
      &route6_materialization_block.insts[call_instruction_index]);
  if (route6_materialization_call == nullptr) {
    return fail("Route 6 materialization index fixture is malformed");
  }
  const auto route6_materialization_index =
      bir::route6_build_call_use_source_index(route6_materialization_function);
  const auto route6_load_producer =
      bir::route6_call_argument_source_producer_record(
          block, *call, call_instruction_index, 0);
  const auto route6_sum_producer =
      bir::route6_call_argument_source_producer_record(
          block, *call, call_instruction_index, 1);
  const auto indexed_route6_load_producer =
      bir::route6_find_call_argument_source_producer(
          route6_materialization_index,
          route6_materialization_block,
          call_instruction_index,
          "consume_materialized_sources",
          0);
  const auto migrated_indexed_load =
      bir::find_call_argument_source_producer_materialization(
          route6_materialization_block,
          *route6_materialization_call,
          call_instruction_index,
          0);
  if (!route6_load_producer ||
      route6_load_producer.status != bir::Route6CallUseStatus::Available ||
      route6_load_producer.argument_source.call != call ||
      route6_load_producer.argument_source.call_instruction_index !=
          call_instruction_index ||
      route6_load_producer.argument_source.block_label != block.label ||
      route6_load_producer.argument_source.block_label_id != block_label ||
      route6_load_producer.argument_source.callee !=
          "consume_materialized_sources" ||
      route6_load_producer.argument_source.arg_index != std::size_t{0} ||
      route6_load_producer.argument_source.argument_value != &call->args[0] ||
      route6_load_producer.argument_source.source_value.value !=
          &call->args[0] ||
      route6_load_producer.argument_source.source_value.name !=
          "%loaded.arg" ||
      route6_load_producer.argument_source.source_value_id !=
          std::optional<std::size_t>{100} ||
      route6_load_producer.argument_source.source_value_name !=
          std::optional<std::string_view>{"%loaded.arg"} ||
      route6_load_producer.argument_source.source_encoding !=
          bir::CallArgumentSourceEncodingKind::Register ||
      route6_load_producer.argument_source.source_kind !=
          bir::Route6CallUseSourceKind::LoadLocal ||
      route6_load_producer.producer.kind != bir::Route1ProducerKind::LoadLocal ||
      route6_load_producer.producer.producer_instruction.instruction !=
          &block.insts[0] ||
      route6_load_producer.producer.producer_instruction.instruction_index !=
          std::size_t{0} ||
      route6_load_producer.producer.source_value.value != &loaded->result ||
      route6_load_producer.producer.source_value.name != "%loaded.arg" ||
      route6_load_producer.materialization.available != true ||
      route6_load_producer.materialization.producer_kind !=
          bir::Route1ProducerKind::LoadLocal ||
      !route6_load_producer.materialization.scalar_materialization_available ||
      !indexed_route6_load_producer ||
      indexed_route6_load_producer.argument_source.call !=
          route6_materialization_call ||
      indexed_route6_load_producer.argument_source.call_instruction_index !=
          call_instruction_index ||
      indexed_route6_load_producer.argument_source.callee !=
          "consume_materialized_sources" ||
      indexed_route6_load_producer.argument_source.arg_index !=
          std::size_t{0} ||
      indexed_route6_load_producer.argument_source.source_value_name !=
          std::optional<std::string_view>{"%loaded.arg"} ||
      indexed_route6_load_producer.producer.kind !=
          bir::Route1ProducerKind::LoadLocal ||
      indexed_route6_load_producer.producer.producer_instruction.instruction !=
          &route6_materialization_block.insts[0] ||
      !migrated_indexed_load.available ||
      migrated_indexed_load.producer_instruction !=
          indexed_route6_load_producer.producer.producer_instruction.instruction ||
      !migrated_indexed_load.materializable ||
      !route6_sum_producer ||
      route6_sum_producer.status != bir::Route6CallUseStatus::Available ||
      route6_sum_producer.argument_source.call == nullptr ||
      route6_sum_producer.argument_source.call_instruction_index !=
          call_instruction_index ||
      route6_sum_producer.argument_source.callee !=
          "consume_materialized_sources" ||
      route6_sum_producer.argument_source.arg_index != std::size_t{1} ||
      route6_sum_producer.argument_source.argument_value != &call->args[1] ||
      route6_sum_producer.argument_source.source_value.value !=
          &call->args[1] ||
      route6_sum_producer.argument_source.source_value.name != "%sum.arg" ||
      route6_sum_producer.argument_source.source_value_id !=
          std::optional<std::size_t>{101} ||
      route6_sum_producer.argument_source.source_value_name !=
          std::optional<std::string_view>{"%sum.arg"} ||
      route6_sum_producer.argument_source.source_encoding !=
          bir::CallArgumentSourceEncodingKind::Register ||
      route6_sum_producer.argument_source.source_kind !=
          bir::Route6CallUseSourceKind::Binary ||
      route6_sum_producer.producer.kind != bir::Route1ProducerKind::Binary ||
      route6_sum_producer.producer.producer_instruction.instruction !=
          &block.insts[1] ||
      route6_sum_producer.producer.producer_instruction.instruction_index !=
          std::size_t{1} ||
      route6_sum_producer.producer.source_value.value != &sum->result ||
      route6_sum_producer.producer.source_value.name != "%sum.arg" ||
      route6_sum_producer.materialization.available != true ||
      route6_sum_producer.materialization.producer_kind !=
          bir::Route1ProducerKind::Binary ||
      !route6_sum_producer.materialization.scalar_materialization_available) {
    return fail(
        "Route 6 call argument producer records should expose call-source identity, source producer, and materialization facts");
  }
  const auto route6_load_publication =
      bir::route6_call_argument_publication_source_record(
          route6_query, block, *call, call_instruction_index, 0);
  const auto indexed_route6_load_publication =
      bir::route6_find_call_argument_publication_source(
          route6_materialization_index,
          route6_materialization_block,
          call_instruction_index,
          "consume_materialized_sources",
          0);
  if (!route6_load_publication ||
      route6_load_publication.status != bir::Route6CallUseStatus::Available ||
      route6_load_publication.source_kind !=
          bir::Route6CallUseSourceKind::MemorySource ||
      route6_load_publication.memory_source.instruction != &block.insts[0] ||
      route6_load_publication.memory_source.node_kind !=
          bir::Route3MemoryAccessNodeKind::LoadLocal ||
      !indexed_route6_load_publication ||
      indexed_route6_load_publication.source_kind !=
          bir::Route6CallUseSourceKind::MemorySource ||
      indexed_route6_load_publication.memory_source.instruction !=
          &route6_materialization_block.insts[0]) {
    return fail(
        "Route 6 call argument source record/index should reuse Route 3 memory source identity");
  }

  const auto nonmaterializable =
      bir::find_call_argument_source_producer_materialization(
          block, *call, call_instruction_index, 2);
  const auto route6_nonmaterializable =
      bir::route6_call_argument_source_producer_record(
          block, *call, call_instruction_index, 2);
  const auto indexed_route6_nonmaterializable =
      bir::route6_find_call_argument_source_producer(
          route6_materialization_index,
          route6_materialization_block,
          call_instruction_index,
          "consume_materialized_sources",
          2);
  const auto migrated_indexed_nonmaterializable =
      bir::find_call_argument_source_producer_materialization(
          route6_materialization_block,
          *route6_materialization_call,
          call_instruction_index,
          2);
  if (!prepared_and_bir_call_argument_source_producer_materialization_match(
          names, source_producers, block_label, block, *call,
          call_instruction_index, 2) ||
      !nonmaterializable.available ||
      nonmaterializable.producer_kind !=
          bir::CallArgumentSourceProducerKind::Binary ||
      nonmaterializable.materializable ||
      !route6_nonmaterializable ||
      route6_nonmaterializable.materialization.scalar_materialization_available ||
      !indexed_route6_nonmaterializable ||
      indexed_route6_nonmaterializable.materialization
          .scalar_materialization_available ||
      !migrated_indexed_nonmaterializable.available ||
      migrated_indexed_nonmaterializable.producer_instruction !=
          indexed_route6_nonmaterializable.producer.producer_instruction
              .instruction ||
      migrated_indexed_nonmaterializable.materializable) {
    return fail(
        "BIR call-argument materialization should expose nonmaterializable binary producer eligibility");
  }

  const auto missing =
      bir::find_call_argument_source_producer_materialization(
          block, *call, call_instruction_index, 3);
  const auto after_call =
      bir::find_call_argument_source_producer_materialization(
          block, *call, call_instruction_index, 4);
  if (!prepared_and_bir_call_argument_source_producer_materialization_match(
          names, source_producers, block_label, block, *call,
          call_instruction_index, 3) ||
      !prepared_and_bir_call_argument_source_producer_materialization_match(
          names, source_producers, block_label, block, *call,
          call_instruction_index, 4) ||
      missing.available ||
      after_call.available) {
    return fail(
        "BIR call-argument materialization should fail closed for missing and producer-after-call paths");
  }
  const auto route6_missing_producer =
      bir::route6_call_argument_source_producer_record(
          block, *call, call_instruction_index, 3);
  const auto route6_after_producer =
      bir::route6_call_argument_source_producer_record(
          block, *call, call_instruction_index, 4);
  const auto indexed_route6_missing_producer =
      bir::route6_find_call_argument_source_producer(
          route6_materialization_index,
          route6_materialization_block,
          call_instruction_index,
          "consume_materialized_sources",
          3);
  const auto migrated_indexed_missing =
      bir::find_call_argument_source_producer_materialization(
          route6_materialization_block,
          *route6_materialization_call,
          call_instruction_index,
          3);
  if (route6_missing_producer ||
      route6_missing_producer.status !=
          bir::Route6CallUseStatus::MissingSourceProducer ||
      route6_after_producer ||
      route6_after_producer.status !=
          bir::Route6CallUseStatus::MissingSourceProducer ||
      indexed_route6_missing_producer ||
      indexed_route6_missing_producer.status !=
          bir::Route6CallUseStatus::MissingSourceProducer ||
      migrated_indexed_missing.available) {
    return fail(
        "Route 6 call argument producer record/index should explicitly report missing source producers");
  }

  auto missing_relationship_block = block;
  auto* missing_relationship_call = std::get_if<bir::CallInst>(
      &missing_relationship_block.insts[call_instruction_index]);
  if (missing_relationship_call == nullptr) {
    return fail("BIR missing-relationship materialization fixture is malformed");
  }
  missing_relationship_call->arg_sources.erase(
      missing_relationship_call->arg_sources.begin());
  const auto missing_relationship =
      bir::route6_call_argument_source_producer_record(
          missing_relationship_block,
          *missing_relationship_call,
          call_instruction_index,
          0);
  if (missing_relationship ||
      missing_relationship.status !=
          bir::Route6CallUseStatus::MissingSourceRelationship ||
      bir::find_call_argument_source_producer_materialization(
          missing_relationship_block,
          *missing_relationship_call,
          call_instruction_index,
          0)
          .available) {
    return fail(
        "Route 6 call argument producer record should fail closed for missing source relationships");
  }

  auto unnamed_source_block = block;
  auto* unnamed_source_call =
      std::get_if<bir::CallInst>(&unnamed_source_block.insts[call_instruction_index]);
  if (unnamed_source_call == nullptr) {
    return fail("BIR unnamed-source materialization fixture is malformed");
  }
  unnamed_source_call->args[0] = bir::Value::immediate_i64(17);
  const auto unnamed_source =
      bir::route6_call_argument_source_producer_record(
          unnamed_source_block, *unnamed_source_call, call_instruction_index, 0);
  if (unnamed_source ||
      unnamed_source.status !=
          bir::Route6CallUseStatus::MissingSourceProducer ||
      bir::find_call_argument_source_producer_materialization(
          unnamed_source_block, *unnamed_source_call, call_instruction_index, 0)
          .available) {
    return fail(
        "Route 6 call argument producer record should fail closed when the argument has no named source value");
  }

  const auto indexed_route6_wrong_call =
      bir::route6_find_call_argument_source_producer(
          route6_materialization_index,
          route6_materialization_block,
          call_instruction_index,
          "consume_wrong_sources",
          0);
  const auto indexed_route6_no_match =
      bir::route6_find_call_argument_source_producer(
          route6_materialization_index,
          route6_materialization_block,
          call_instruction_index,
          "consume_materialized_sources",
          99);
  if (indexed_route6_wrong_call ||
      indexed_route6_wrong_call.status != bir::Route6CallUseStatus::WrongCall ||
      indexed_route6_no_match ||
      indexed_route6_no_match.status != bir::Route6CallUseStatus::NoMatch) {
    return fail(
        "Route 6 call argument producer index should fail closed for wrong-call and no-match lookups");
  }

  auto abi_bound_block = block;
  auto* abi_bound_call =
      std::get_if<bir::CallInst>(&abi_bound_block.insts[call_instruction_index]);
  if (abi_bound_call == nullptr) {
    return fail("BIR ABI-bound materialization fixture is malformed");
  }
  abi_bound_call->arg_sources[0].source_selection =
      bir::CallArgumentSourceSelection{
          .kind = bir::CallArgumentSourceSelectionKind::FrameSlotValue,
          .source_value_id = std::size_t{100},
          .source_value_name = std::string{"%loaded.arg"},
          .source_slot_id = c4c::SlotNameId{3},
          .source_stack_offset_bytes = std::size_t{32},
          .source_size_bytes = std::size_t{8},
          .source_align_bytes = std::size_t{8},
      };
  const auto abi_bound_producer =
      bir::route6_call_argument_source_producer_record(
          abi_bound_block, *abi_bound_call, call_instruction_index, 0);
  if (abi_bound_producer ||
      abi_bound_producer.status !=
          bir::Route6CallUseStatus::AbiBoundExcluded ||
      abi_bound_producer.argument_source.source_kind !=
          bir::Route6CallUseSourceKind::AbiBoundExcluded ||
      bir::find_call_argument_source_producer_materialization(
          abi_bound_block, *abi_bound_call, call_instruction_index, 0)
          .available) {
    return fail(
        "Route 6 call argument producer record should exclude ABI-bound source-selection facts");
  }

  auto duplicate_block = block;
  auto* duplicate_call =
      std::get_if<bir::CallInst>(&duplicate_block.insts[call_instruction_index]);
  if (duplicate_call == nullptr) {
    return fail("BIR duplicate materialization fixture is malformed");
  }
  duplicate_call->arg_sources.push_back(duplicate_call->arg_sources.front());
  if (bir::find_call_argument_source_producer_materialization(
          duplicate_block, *duplicate_call, call_instruction_index, 0)
          .available) {
    return fail(
        "BIR call-argument materialization should fail closed for duplicate argument source records");
  }
  const auto route6_duplicate_producer =
      bir::route6_call_argument_source_producer_record(
          duplicate_block, *duplicate_call, call_instruction_index, 0);
  const bir::Function route6_duplicate_function{
      .name = "route6_duplicate_sources",
      .blocks = {duplicate_block},
  };
  const auto& route6_duplicate_block =
      route6_duplicate_function.blocks.front();
  const auto* route6_duplicate_call = std::get_if<bir::CallInst>(
      &route6_duplicate_block.insts[call_instruction_index]);
  if (route6_duplicate_call == nullptr) {
    return fail("Route 6 duplicate materialization index fixture is malformed");
  }
  const auto route6_duplicate_index =
      bir::route6_build_call_use_source_index(route6_duplicate_function);
  const auto indexed_route6_duplicate =
      bir::route6_find_call_argument_source_producer(
          route6_duplicate_index,
          route6_duplicate_block,
          call_instruction_index,
          "consume_materialized_sources",
          0);
  const auto migrated_indexed_duplicate =
      bir::find_call_argument_source_producer_materialization(
          route6_duplicate_block,
          *route6_duplicate_call,
          call_instruction_index,
          0);
  if (route6_duplicate_producer ||
      route6_duplicate_producer.status !=
          bir::Route6CallUseStatus::DuplicateRelationship ||
      indexed_route6_duplicate ||
      indexed_route6_duplicate.status !=
          bir::Route6CallUseStatus::DuplicateRelationship ||
      migrated_indexed_duplicate.available) {
    return fail(
        "Route 6 call argument producer record should fail closed for duplicate source records");
  }

  return 0;
}

int verify_bir_comparison_condition_producer_identity_lookup() {
  bir::Block block{
      .label = "entry",
      .insts =
          {
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I64, "%loaded"),
                  .slot_name = "slot0",
                  .slot_id = c4c::SlotNameId{11},
                  .byte_offset = 0,
                  .align_bytes = 8,
              },
              bir::LoadGlobalInst{
                  .result = bir::Value::named(bir::TypeKind::I64, "%global"),
                  .global_name = "g0",
                  .byte_offset = 0,
                  .align_bytes = 8,
              },
              bir::CastInst{
                  .opcode = bir::CastOpcode::SExt,
                  .result = bir::Value::named(bir::TypeKind::I64, "%casted"),
                  .operand = bir::Value::named(bir::TypeKind::I32, "%param"),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I64, "%folded"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::immediate_i64(7),
                  .rhs = bir::Value::immediate_i64(5),
              },
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I64, "%selected"),
                  .compare_type = bir::TypeKind::I1,
                  .lhs = bir::Value::named(bir::TypeKind::I1, "%flag"),
                  .rhs = bir::Value::immediate_i1(false),
                  .true_value = bir::Value::named(bir::TypeKind::I64, "%loaded"),
                  .false_value = bir::Value::named(bir::TypeKind::I64, "%global"),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Slt,
                  .result = bir::Value::named(bir::TypeKind::I1, "%cond"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%selected"),
                  .rhs = bir::Value::immediate_i64(12),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I1, "%not.compare"),
                  .operand_type = bir::TypeKind::I1,
                  .lhs = bir::Value::immediate_i1(true),
                  .rhs = bir::Value::immediate_i1(false),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::named(bir::TypeKind::I64, "%after"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::immediate_i64(1),
                  .rhs = bir::Value::immediate_i64(2),
              },
          },
      .terminator =
          bir::CondBranchTerminator{
              .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
              .true_label = "then",
              .false_label = "else",
          },
  };

  const auto immediate = bir::find_comparison_operand_producer(
      block, bir::Value::immediate_i64(42), block.insts.size());
  if (!immediate.has_value() ||
      immediate->producer_kind != bir::ComparisonProducerKind::Immediate ||
      immediate->producer_instruction != nullptr ||
      immediate->integer_constant != std::optional<std::int64_t>{42}) {
    return fail(
        "BIR comparison producer query should expose immediate constants");
  }

  const auto loaded = bir::find_comparison_operand_producer(
      block, bir::Value::named(bir::TypeKind::I64, "%loaded"), 5);
  const auto global = bir::find_comparison_operand_producer(
      block, bir::Value::named(bir::TypeKind::I64, "%global"), 5);
  const auto casted = bir::find_comparison_operand_producer(
      block, bir::Value::named(bir::TypeKind::I64, "%casted"), 5);
  const auto selected = bir::find_comparison_operand_producer(
      block, bir::Value::named(bir::TypeKind::I64, "%selected"), 5);
  const auto folded = bir::find_comparison_operand_producer(
      block, bir::Value::named(bir::TypeKind::I64, "%folded"), 5);
  if (!loaded.has_value() ||
      loaded->producer_kind != bir::ComparisonProducerKind::LoadLocal ||
      loaded->producer_instruction_index != 0 ||
      loaded->produced_value == nullptr ||
      loaded->produced_value->name != "%loaded" ||
      !global.has_value() ||
      global->producer_kind != bir::ComparisonProducerKind::LoadGlobal ||
      !casted.has_value() ||
      casted->producer_kind != bir::ComparisonProducerKind::Cast ||
      !selected.has_value() ||
      selected->producer_kind != bir::ComparisonProducerKind::Select ||
      !folded.has_value() ||
      folded->producer_kind != bir::ComparisonProducerKind::Binary ||
      folded->integer_constant != std::optional<std::int64_t>{12}) {
    return fail(
        "BIR comparison producer query should expose same-block producer kinds and folded constants");
  }

  const auto facts = bir::find_fused_compare_operand_producer_facts(
      block,
      bir::Value::named(bir::TypeKind::I64, "%selected"),
      bir::Value::named(bir::TypeKind::I64, "%folded"),
      5);
  if (!facts.available ||
      !facts.lhs.has_value() ||
      facts.lhs->producer_kind != bir::ComparisonProducerKind::Select ||
      !facts.rhs.has_value() ||
      facts.rhs->integer_constant != std::optional<std::int64_t>{12}) {
    return fail(
        "BIR fused-compare operand producer facts should expose lhs/rhs semantic producers");
  }

  const auto condition = bir::find_materialized_condition_producer_identity(
      block, bir::Value::named(bir::TypeKind::I1, "%cond"), 6);
  const auto route7_condition_instruction =
      bir::route7_comparison_instruction_record(&block, 5);
  const auto route7_condition_operand =
      bir::route7_comparison_operand_record(
          &block,
          bir::Value::named(bir::TypeKind::I1, "%cond"),
          6,
          bir::Route7ComparisonOperandRole::ConditionValue);
  const bir::Function route7_function{
      .name = "route7_comparison_condition_records",
      .blocks = {block},
  };
  const auto& route7_block = route7_function.blocks.front();
  const auto route7_index =
      bir::route7_build_comparison_condition_index(route7_function);
  const auto route7_facade = bir::route_index_reference_facade(route7_index);
  const auto indexed_route7_condition_instruction =
      bir::route7_find_comparison_instruction(route7_index, route7_block, 5);
  const auto indexed_route7_materialized_condition =
      bir::route7_find_materialized_condition(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I1, "%cond"),
          6);
  const auto indexed_route7_lhs_operand =
      bir::route7_find_comparison_operand(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I64, "%selected"),
          5,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto route7_instruction_ref =
      bir::route7_validate_comparison_instruction_reference(
          route7_index, route7_block, 5);
  const auto route7_materialized_ref =
      bir::route7_validate_materialized_condition_reference(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I1, "%cond"),
          6);
  const auto facade_route7_materialized_ref =
      bir::route_index_validate_materialized_condition_reference(
          route7_facade,
          route7_block,
          bir::Value::named(bir::TypeKind::I1, "%cond"),
          6);
  const auto route7_lhs_ref =
      bir::route7_validate_comparison_operand_reference(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I64, "%selected"),
          5,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto route7_rhs_immediate_ref =
      bir::route7_validate_comparison_operand_reference(
          route7_index,
          route7_block,
          bir::Value::immediate_i64(12),
          5,
          bir::Route7ComparisonOperandRole::Rhs);
  const auto route7_branch_ref =
      bir::route7_validate_branch_condition_reference(
          route7_index, route7_block);
  if (!condition.available ||
      condition.binary != std::get_if<bir::BinaryInst>(&block.insts[5]) ||
      condition.instruction_index != 5 ||
      condition.condition_value_name != "%cond" ||
      !condition.lhs.has_value() ||
      condition.lhs->producer_kind != bir::ComparisonProducerKind::Select ||
      !condition.rhs.has_value() ||
      condition.rhs->integer_constant != std::optional<std::int64_t>{12} ||
      !route7_condition_instruction ||
      route7_condition_instruction.status !=
          bir::Route7ComparisonStatus::Available ||
      route7_condition_instruction.binary !=
          std::get_if<bir::BinaryInst>(&block.insts[5]) ||
      route7_condition_instruction.condition_value.name != "%cond" ||
      route7_condition_instruction.predicate != bir::BinaryOpcode::Slt ||
      route7_condition_instruction.compare_type != bir::TypeKind::I64 ||
      route7_condition_instruction.lhs.producer_kind !=
          bir::ComparisonProducerKind::Select ||
      route7_condition_instruction.rhs.integer_constant !=
          std::optional<std::int64_t>{12} ||
      !route7_condition_operand ||
      route7_condition_operand.producer_kind !=
          bir::ComparisonProducerKind::Binary ||
      route7_condition_operand.producer_instruction != &block.insts[5] ||
      !indexed_route7_condition_instruction ||
      indexed_route7_condition_instruction.binary !=
          std::get_if<bir::BinaryInst>(&route7_block.insts[5]) ||
      !indexed_route7_materialized_condition ||
      indexed_route7_materialized_condition.instruction_index != 5 ||
      condition.instruction_index !=
          indexed_route7_materialized_condition.instruction_index ||
      condition.condition_value_name !=
          indexed_route7_materialized_condition.condition_value.name ||
      condition.lhs->producer_kind !=
          indexed_route7_materialized_condition.lhs.producer_kind ||
      condition.rhs->integer_constant !=
          indexed_route7_materialized_condition.rhs.integer_constant ||
      !indexed_route7_lhs_operand ||
      indexed_route7_lhs_operand.producer_kind !=
          bir::ComparisonProducerKind::Select ||
      indexed_route7_lhs_operand.producer_instruction !=
          &route7_block.insts[4] ||
      !route7_instruction_ref ||
      route7_instruction_ref.status !=
          bir::RouteIndexValidationStatus::Valid ||
      route7_instruction_ref.reference.route !=
          bir::RouteIndexRoute::Route7ComparisonCondition ||
      route7_instruction_ref.reference.owner_scope !=
          bir::RouteIndexOwnerScope::Function ||
      route7_instruction_ref.reference.record_category !=
          bir::RouteIndexRecordCategory::Route7ComparisonInstruction ||
      route7_instruction_ref.reference.relationship !=
          bir::RouteIndexRelationshipKind::Route7Instruction ||
      route7_instruction_ref.comparison_record == nullptr ||
      route7_instruction_ref.comparison_record->instruction_index != 5 ||
      !route7_materialized_ref ||
      route7_materialized_ref.reference.relationship !=
          bir::RouteIndexRelationshipKind::Route7MaterializedCondition ||
      route7_materialized_ref.reference.value.name != "%cond" ||
      !facade_route7_materialized_ref ||
      facade_route7_materialized_ref.comparison_record !=
          route7_materialized_ref.comparison_record ||
      facade_route7_materialized_ref.reference.record_index !=
          route7_materialized_ref.reference.record_index ||
      !route7_lhs_ref ||
      route7_lhs_ref.reference.record_category !=
          bir::RouteIndexRecordCategory::Route7ComparisonOperand ||
      route7_lhs_ref.reference.operand_role !=
          bir::Route7ComparisonOperandRole::Lhs ||
      route7_lhs_ref.operand_record == nullptr ||
      route7_lhs_ref.operand_record->producer_instruction !=
          &route7_block.insts[4] ||
      !route7_rhs_immediate_ref ||
      route7_rhs_immediate_ref.operand_record == nullptr ||
      route7_rhs_immediate_ref.operand_record->producer_kind !=
          bir::ComparisonProducerKind::Immediate ||
      route7_rhs_immediate_ref.operand_record->producer_instruction != nullptr ||
      route7_rhs_immediate_ref.operand_record->integer_constant !=
          std::optional<std::int64_t>{12} ||
      !route7_branch_ref ||
      route7_branch_ref.reference.record_category !=
          bir::RouteIndexRecordCategory::Route7BranchCondition ||
      route7_branch_ref.branch_condition_record == nullptr ||
      route7_branch_ref.branch_condition_record->comparison.instruction_index !=
          5) {
    return fail(
        "BIR materialized condition producer query and Route 7 records/index references should expose comparison binary and operand producers");
  }

  const auto route7_block_index =
      bir::route7_build_comparison_condition_index(route7_block);
  const auto stale_block_ref =
      bir::route7_validate_comparison_instruction_reference(
          route7_block_index, block, 5);
  const auto wrong_role_ref =
      bir::route7_validate_comparison_operand_reference(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I64, "%selected"),
          5,
          bir::Route7ComparisonOperandRole::Rhs);
  auto divergent_route7_index = route7_index;
  divergent_route7_index.operand_records.push_back(indexed_route7_lhs_operand);
  const auto duplicate_lhs_ref =
      bir::route7_validate_comparison_operand_reference(
          divergent_route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I64, "%selected"),
          5,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto missing_condition_ref =
      bir::route7_validate_materialized_condition_reference(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I1, "%missing.cond"),
          route7_block.insts.size());
  const auto facade_missing_condition_ref =
      bir::route_index_validate_materialized_condition_reference(
          route7_facade,
          route7_block,
          bir::Value::named(bir::TypeKind::I1, "%missing.cond"),
          route7_block.insts.size());
  if (stale_block_ref ||
      stale_block_ref.status != bir::RouteIndexValidationStatus::StaleOwner ||
      wrong_role_ref ||
      wrong_role_ref.status !=
          bir::RouteIndexValidationStatus::WrongRelationship ||
      duplicate_lhs_ref ||
      duplicate_lhs_ref.status !=
          bir::RouteIndexValidationStatus::DuplicateReference ||
      missing_condition_ref ||
      missing_condition_ref.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      missing_condition_ref.route_status !=
          bir::Route7ComparisonStatus::NoMatch ||
      facade_missing_condition_ref.status != missing_condition_ref.status ||
      facade_missing_condition_ref.route_status !=
          missing_condition_ref.route_status) {
    return fail(
        "Route 7 index reference facade should reject stale owner, wrong role, duplicate reference, and missing condition divergence");
  }

  const auto route7_noncompare =
      bir::route7_comparison_instruction_record(&block, 6);
  const auto indexed_route7_noncompare =
      bir::route7_find_comparison_instruction(route7_index, route7_block, 6);
  const auto indexed_route7_missing_condition =
      bir::route7_find_materialized_condition(
          route7_index,
          route7_block,
          bir::Value::named(bir::TypeKind::I1, "%missing.cond"),
          route7_block.insts.size());
  auto missing_producer_block = block;
  auto* missing_producer_compare =
      std::get_if<bir::BinaryInst>(&missing_producer_block.insts[5]);
  if (missing_producer_compare == nullptr) {
    return fail("Route 7 missing-producer comparison fixture is malformed");
  }
  missing_producer_compare->lhs =
      bir::Value::named(bir::TypeKind::I64, "%missing.lhs");
  const bir::Function route7_missing_producer_function{
      .name = "route7_missing_producer_comparison_records",
      .blocks = {missing_producer_block},
  };
  const auto& route7_missing_producer_block =
      route7_missing_producer_function.blocks.front();
  const auto route7_missing_producer_index =
      bir::route7_build_comparison_condition_index(
          route7_missing_producer_function);
  const auto indexed_route7_missing_producer =
      bir::route7_find_comparison_operand(
          route7_missing_producer_index,
          route7_missing_producer_block,
          bir::Value::named(bir::TypeKind::I64, "%missing.lhs"),
          5,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto indexed_route7_missing_producer_ref =
      bir::route7_validate_comparison_operand_reference(
          route7_missing_producer_index,
          route7_missing_producer_block,
          bir::Value::named(bir::TypeKind::I64, "%missing.lhs"),
          5,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto route7_after_operand =
      bir::route7_comparison_operand_record(
          &block,
          bir::Value::named(bir::TypeKind::I64, "%after"),
          7,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto route7_missing_operand =
      bir::route7_comparison_operand_record(
          &block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          8,
          bir::Route7ComparisonOperandRole::Rhs);
  if (bir::find_materialized_condition_producer_identity(
          block, bir::Value::named(bir::TypeKind::I1, "%not.compare"), 7)
          .available ||
      bir::find_comparison_operand_producer(
          block, bir::Value::named(bir::TypeKind::I64, "%after"), 7)
          .has_value() ||
      bir::find_comparison_operand_producer(
          block, bir::Value::named(bir::TypeKind::I64, "%missing"), 8)
          .has_value() ||
      route7_noncompare ||
      route7_noncompare.status != bir::Route7ComparisonStatus::NonComparison ||
      indexed_route7_noncompare ||
      indexed_route7_noncompare.status !=
          bir::Route7ComparisonStatus::NonComparison ||
      indexed_route7_missing_condition ||
      indexed_route7_missing_condition.status !=
          bir::Route7ComparisonStatus::NoMatch ||
      indexed_route7_missing_producer ||
      indexed_route7_missing_producer.status !=
          bir::Route7ComparisonStatus::MissingOperandProducer ||
      indexed_route7_missing_producer_ref ||
      indexed_route7_missing_producer_ref.status !=
          bir::RouteIndexValidationStatus::MissingRecord ||
      route7_after_operand ||
      route7_after_operand.status !=
          bir::Route7ComparisonStatus::MissingOperandProducer ||
      route7_missing_operand ||
      route7_missing_operand.status !=
          bir::Route7ComparisonStatus::MissingOperandProducer) {
    return fail(
        "BIR comparison producer queries and Route 7 records should fail closed for non-comparison, after-index, and missing producers");
  }

  auto duplicate_block = block;
  duplicate_block.insts.insert(
      duplicate_block.insts.begin() + 1,
      bir::LoadLocalInst{
          .result = bir::Value::named(bir::TypeKind::I64, "%loaded"),
          .slot_name = "slot1",
          .slot_id = c4c::SlotNameId{12},
          .byte_offset = 0,
          .align_bytes = 8,
      });
  const auto route7_duplicate_operand =
      bir::route7_comparison_operand_record(
          &duplicate_block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          duplicate_block.insts.size(),
          bir::Route7ComparisonOperandRole::Lhs);
  auto duplicate_compare_block = duplicate_block;
  auto* duplicate_compare =
      std::get_if<bir::BinaryInst>(&duplicate_compare_block.insts[6]);
  if (duplicate_compare == nullptr) {
    return fail("Route 7 duplicate comparison fixture is malformed");
  }
  duplicate_compare->lhs = bir::Value::named(bir::TypeKind::I64, "%loaded");
  const bir::Function route7_duplicate_function{
      .name = "route7_duplicate_comparison_records",
      .blocks = {duplicate_compare_block},
  };
  const auto& route7_duplicate_block =
      route7_duplicate_function.blocks.front();
  const auto route7_duplicate_index =
      bir::route7_build_comparison_condition_index(route7_duplicate_function);
  const auto indexed_route7_duplicate =
      bir::route7_find_comparison_operand(
          route7_duplicate_index,
          route7_duplicate_block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          6,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto indexed_route7_duplicate_ref =
      bir::route7_validate_comparison_operand_reference(
          route7_duplicate_index,
          route7_duplicate_block,
          bir::Value::named(bir::TypeKind::I64, "%loaded"),
          6,
          bir::Route7ComparisonOperandRole::Lhs);
  if (bir::find_comparison_operand_producer(
          duplicate_block, bir::Value::named(bir::TypeKind::I64, "%loaded"),
          duplicate_block.insts.size())
          .has_value() ||
      route7_duplicate_operand ||
      route7_duplicate_operand.status !=
          bir::Route7ComparisonStatus::DuplicateProducer ||
      indexed_route7_duplicate ||
      indexed_route7_duplicate.status !=
          bir::Route7ComparisonStatus::DuplicateProducer ||
      indexed_route7_duplicate_ref ||
      indexed_route7_duplicate_ref.status !=
          bir::RouteIndexValidationStatus::DuplicateReference) {
    return fail(
        "BIR comparison producer query and Route 7 records/index references should fail closed for duplicate same-block producers");
  }

  return 0;
}

int verify_prepared_bir_comparison_condition_producer_equivalence() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("entry");
  const auto selected_name = names.value_names.intern("%selected");
  const auto folded_name = names.value_names.intern("%folded");
  const auto cond_name = names.value_names.intern("%cond");
  const auto missing_name = names.value_names.intern("%missing");

  bir::Block block{
      .label = "entry",
      .insts =
          {
              bir::LoadLocalInst{
                  .result = bir::Value::named(bir::TypeKind::I64, "%loaded"),
                  .slot_name = "slot0",
                  .slot_id = c4c::SlotNameId{21},
                  .byte_offset = 0,
                  .align_bytes = 8,
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::UDiv,
                  .result = bir::Value::named(bir::TypeKind::I64, "%folded"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::immediate_i64(24),
                  .rhs = bir::Value::immediate_i64(2),
              },
              bir::SelectInst{
                  .predicate = bir::BinaryOpcode::Ne,
                  .result = bir::Value::named(bir::TypeKind::I64, "%selected"),
                  .compare_type = bir::TypeKind::I1,
                  .lhs = bir::Value::named(bir::TypeKind::I1, "%flag"),
                  .rhs = bir::Value::immediate_i1(false),
                  .true_value = bir::Value::named(bir::TypeKind::I64, "%loaded"),
                  .false_value = bir::Value::named(bir::TypeKind::I64, "%folded"),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Sge,
                  .result = bir::Value::named(bir::TypeKind::I1, "%cond"),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::named(bir::TypeKind::I64, "%selected"),
                  .rhs = bir::Value::named(bir::TypeKind::I64, "%folded"),
              },
          },
      .terminator =
          bir::CondBranchTerminator{
              .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
              .true_label = "then",
              .false_label = "else",
          },
      .label_id = block_label,
  };

  const auto* folded = std::get_if<bir::BinaryInst>(&block.insts[1]);
  const auto* selected = std::get_if<bir::SelectInst>(&block.insts[2]);
  const auto* condition_binary = std::get_if<bir::BinaryInst>(&block.insts[3]);
  if (folded == nullptr || selected == nullptr || condition_binary == nullptr) {
    return fail("prepared/BIR comparison producer fixture is malformed");
  }

  prepare::PreparedEdgePublicationSourceProducerLookups source_producers;
  source_producers.producers_by_value_name.emplace(
      folded_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 1,
          .binary = folded,
      });
  source_producers.producers_by_value_name.emplace(
      selected_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::
              SelectMaterialization,
          .block_label = block_label,
          .instruction_index = 2,
          .select = selected,
      });
  source_producers.producers_by_value_name.emplace(
      cond_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Binary,
          .block_label = block_label,
          .instruction_index = 3,
          .binary = condition_binary,
      });
  source_producers.producers_by_value_name.emplace(
      missing_name,
      prepare::PreparedEdgePublicationSourceProducer{
          .kind = prepare::PreparedEdgePublicationSourceProducerKind::Unknown,
          .block_label = block_label,
      });

  const prepare::PreparedBranchCondition fused_condition{
      .function_name = c4c::FunctionNameId{7},
      .block_label = block_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = bir::Value::named(bir::TypeKind::I1, "%cond"),
      .predicate = bir::BinaryOpcode::Sge,
      .compare_type = bir::TypeKind::I64,
      .lhs = bir::Value::named(bir::TypeKind::I64, "%selected"),
      .rhs = bir::Value::named(bir::TypeKind::I64, "%folded"),
      .can_fuse_with_branch = true,
      .true_label = c4c::BlockLabelId{30},
      .false_label = c4c::BlockLabelId{31},
  };
  const auto prepared_fused =
      prepare::find_prepared_fused_compare_operand_producer_facts(
          names, &source_producers, block_label, &block, fused_condition, 3);
  const auto bir_fused = bir::find_fused_compare_operand_producer_facts(
      block, *fused_condition.lhs, *fused_condition.rhs, 3);
  const auto route7_index =
      bir::route7_build_comparison_condition_index(block);
  const auto route7_fused = bir::route7_find_fused_compare_operand_producer_facts(
      route7_index, block, *fused_condition.lhs, *fused_condition.rhs, 3);
  const auto route7_consumer_boundary =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_index,
          block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          block.insts.size());
  const auto route7_fused_instruction =
      bir::route7_comparison_instruction_record(&block, 3);
  if (!prepared_fused.has_value() ||
      !bir_fused.available ||
      !route7_fused.available ||
      !route7_consumer_boundary.available ||
      !prepared_and_bir_comparison_operand_producer_match(
          names, block, prepared_fused->lhs, *fused_condition.lhs, 3) ||
      !prepared_and_bir_comparison_operand_producer_match(
          names, block, prepared_fused->rhs, *fused_condition.rhs, 3) ||
      !bir_fused.lhs.has_value() ||
      bir_fused.lhs->producer_kind != bir::ComparisonProducerKind::Select ||
      !bir_fused.rhs.has_value() ||
      bir_fused.rhs->integer_constant != std::optional<std::int64_t>{12} ||
      !route7_fused.lhs.has_value() ||
      route7_fused.lhs->producer_kind != bir::ComparisonProducerKind::Select ||
      route7_fused.lhs->producer_instruction !=
          prepared_fused->lhs->instruction ||
      route7_fused.lhs->producer_instruction_index !=
          prepared_fused->lhs->instruction_index ||
      !route7_fused.rhs.has_value() ||
      route7_fused.rhs->integer_constant !=
          bir_fused.rhs->integer_constant ||
      !route7_consumer_boundary.lhs.has_value() ||
      route7_consumer_boundary.lhs->producer_kind !=
          bir::ComparisonProducerKind::Select ||
      route7_consumer_boundary.lhs->producer_instruction !=
          prepared_fused->lhs->instruction ||
      route7_consumer_boundary.lhs->producer_instruction_index !=
          prepared_fused->lhs->instruction_index ||
      !route7_consumer_boundary.rhs.has_value() ||
      route7_consumer_boundary.rhs->integer_constant !=
          prepared_fused->rhs->integer_constant ||
      !route7_fused_instruction ||
      route7_fused_instruction.status !=
          bir::Route7ComparisonStatus::Available ||
      route7_fused_instruction.condition_value.name != "%cond" ||
      route7_fused_instruction.predicate != *fused_condition.predicate ||
      route7_fused_instruction.compare_type != *fused_condition.compare_type ||
      route7_fused_instruction.lhs.producer_instruction !=
          prepared_fused->lhs->instruction ||
      route7_fused_instruction.lhs.producer_instruction_index !=
          prepared_fused->lhs->instruction_index ||
      route7_fused_instruction.rhs.integer_constant !=
          prepared_fused->rhs->integer_constant) {
    return fail(
        "prepared and BIR fused compare operand producer facts and Route 7 records should match for select and folded-constant operands");
  }

  auto immediate_condition = fused_condition;
  immediate_condition.lhs = bir::Value::immediate_i64(9);
  const auto prepared_immediate =
      prepare::find_prepared_fused_compare_operand_producer_facts(
          names, &source_producers, block_label, &block, immediate_condition, 3);
  const auto bir_immediate = bir::find_fused_compare_operand_producer_facts(
      block, *immediate_condition.lhs, *immediate_condition.rhs, 3);
  auto immediate_compare_block = block;
  auto* immediate_compare =
      std::get_if<bir::BinaryInst>(&immediate_compare_block.insts[3]);
  if (immediate_compare == nullptr) {
    return fail("Route 7 immediate fused compare fixture is malformed");
  }
  immediate_compare->lhs = *immediate_condition.lhs;
  const auto route7_immediate_index =
      bir::route7_build_comparison_condition_index(immediate_compare_block);
  const auto route7_immediate =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_immediate_index,
          immediate_compare_block,
          *immediate_condition.lhs,
          *immediate_condition.rhs,
          3);
  const auto route7_immediate_operand =
      bir::route7_comparison_operand_record(
          &block,
          *immediate_condition.lhs,
          3,
          bir::Route7ComparisonOperandRole::Lhs);
  if (!prepared_immediate.has_value() ||
      !prepared_and_bir_comparison_operand_producer_match(
          names, block, prepared_immediate->lhs, *immediate_condition.lhs, 3) ||
      !bir_immediate.lhs.has_value() ||
      bir_immediate.lhs->producer_kind != bir::ComparisonProducerKind::Immediate ||
      bir_immediate.lhs->integer_constant != std::optional<std::int64_t>{9} ||
      !route7_immediate.lhs.has_value() ||
      route7_immediate.lhs->producer_kind !=
          bir::ComparisonProducerKind::Immediate ||
      route7_immediate.lhs->integer_constant !=
          prepared_immediate->lhs->integer_constant ||
      !route7_immediate_operand ||
      route7_immediate_operand.producer_kind !=
          bir::ComparisonProducerKind::Immediate ||
      route7_immediate_operand.integer_constant !=
          prepared_immediate->lhs->integer_constant) {
    return fail(
        "prepared and BIR fused compare operand producer facts and Route 7 records should match immediate operands");
  }

  auto rhs_only_condition = fused_condition;
  rhs_only_condition.lhs = bir::Value::named(bir::TypeKind::I64, "%missing");
  const auto prepared_rhs_only =
      prepare::find_prepared_fused_compare_operand_producer_facts(
          names, &source_producers, block_label, &block, rhs_only_condition, 3);
  const auto bir_rhs_only = bir::find_fused_compare_operand_producer_facts(
      block, *rhs_only_condition.lhs, *rhs_only_condition.rhs, 3);
  const auto route7_rhs_only =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_index,
          block,
          *rhs_only_condition.lhs,
          *rhs_only_condition.rhs,
          3);
  if (!prepared_rhs_only.has_value() ||
      prepared_rhs_only->lhs.has_value() ||
      !prepared_rhs_only->rhs.has_value() ||
      !bir_rhs_only.available ||
      bir_rhs_only.lhs.has_value() ||
      !bir_rhs_only.rhs.has_value() ||
      !route7_rhs_only.available ||
      route7_rhs_only.lhs.has_value() ||
      !route7_rhs_only.rhs.has_value() ||
      route7_rhs_only.rhs->integer_constant !=
          bir_rhs_only.rhs->integer_constant) {
    return fail(
        "prepared, BIR, and Route 7 fused compare producer facts should match rhs-only availability");
  }

  auto materialized_bool = fused_condition;
  materialized_bool.kind = prepare::PreparedBranchConditionKind::MaterializedBool;
  const auto prepared_materialized_bool =
      prepare::find_prepared_fused_compare_operand_producer_facts(
          names, &source_producers, block_label, &block, materialized_bool, 3);
  if (prepared_materialized_bool.has_value() ||
      bir::find_fused_compare_operand_producer_facts(
          block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          bir::Value::named(bir::TypeKind::I64, "%also.missing"),
          3)
          .available ||
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_index,
          block,
          bir::Value::named(bir::TypeKind::I64, "%missing"),
          bir::Value::named(bir::TypeKind::I64, "%also.missing"),
          3)
          .available) {
    return fail(
        "prepared, BIR, and Route 7 fused compare producer facts should fail closed for unavailable/non-fused paths");
  }
  const auto wrong_key_route7_ref =
      bir::route_index_validate_comparison_operand_reference(
          bir::route_index_reference_facade(route7_index),
          block,
          *fused_condition.lhs,
          2,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto wrong_role_route7_ref =
      bir::route_index_validate_comparison_operand_reference(
          bir::route_index_reference_facade(route7_index),
          block,
          *fused_condition.lhs,
          3,
          bir::Route7ComparisonOperandRole::Rhs);
  if (wrong_key_route7_ref ||
      wrong_key_route7_ref.status !=
          bir::RouteIndexValidationStatus::WrongKey ||
      wrong_role_route7_ref ||
      wrong_role_route7_ref.status !=
          bir::RouteIndexValidationStatus::WrongRelationship) {
    return fail(
        "Route 7 fused compare operand route-index validation should reject wrong-key and wrong-role references");
  }

  const auto bir_consumer_boundary =
      bir::find_fused_compare_operand_producer_facts(
          block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          block.insts.size());
  bir::Route7ComparisonConditionIndex unavailable_route7_index;
  const auto unavailable_route7_consumer =
      bir::route7_find_fused_compare_operand_producer_facts(
          unavailable_route7_index,
          block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          block.insts.size());
  auto missing_producer_index = route7_index;
  for (auto& record : missing_producer_index.operand_records) {
    if (record.role == bir::Route7ComparisonOperandRole::Lhs &&
        record.value.name == "%selected") {
      record.available = false;
      record.status = bir::Route7ComparisonStatus::MissingOperandProducer;
      record.producer_kind = bir::ComparisonProducerKind::Unknown;
      record.producer_instruction = nullptr;
      record.producer_instruction_index = 0;
      record.produced_value = {};
    }
  }
  for (auto& record : missing_producer_index.branch_condition_records) {
    if (record.kind == bir::Route7BranchConditionKind::FusedCompare) {
      record.comparison.lhs.available = false;
      record.comparison.lhs.status =
          bir::Route7ComparisonStatus::MissingOperandProducer;
      record.comparison.lhs.producer_kind =
          bir::ComparisonProducerKind::Unknown;
      record.comparison.lhs.producer_instruction = nullptr;
      record.comparison.lhs.producer_instruction_index = 0;
      record.comparison.lhs.produced_value = {};
    }
  }
  const auto missing_producer_route7_consumer =
      bir::route7_find_fused_compare_operand_producer_facts(
          missing_producer_index,
          block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          block.insts.size());
  auto duplicate_reference_index = route7_index;
  if (!duplicate_reference_index.branch_condition_records.empty()) {
    duplicate_reference_index.branch_condition_records.push_back(
        duplicate_reference_index.branch_condition_records.front());
  }
  const auto duplicate_reference_route7_consumer =
      bir::route7_find_fused_compare_operand_producer_facts(
          duplicate_reference_index,
          block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          block.insts.size());
  auto wrong_key_index = route7_index;
  for (auto& record : wrong_key_index.operand_records) {
    if (record.role == bir::Route7ComparisonOperandRole::Lhs &&
        record.value.name == "%selected") {
      record.before_instruction_index = 2;
    }
  }
  wrong_key_index.branch_condition_records.clear();
  const auto wrong_key_route7_consumer =
      bir::route7_find_fused_compare_operand_producer_facts(
          wrong_key_index, block, *fused_condition.lhs, *fused_condition.rhs, 3);
  auto wrong_relationship_index = route7_index;
  for (auto& record : wrong_relationship_index.operand_records) {
    if (record.role == bir::Route7ComparisonOperandRole::Lhs &&
        record.value.name == "%selected") {
      record.role = bir::Route7ComparisonOperandRole::Rhs;
    }
  }
  wrong_relationship_index.branch_condition_records.clear();
  const auto wrong_relationship_route7_consumer =
      bir::route7_find_fused_compare_operand_producer_facts(
          wrong_relationship_index,
          block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          3);
  auto stale_owner_block = block;
  const auto stale_owner_route7_consumer =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_index,
          stale_owner_block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          stale_owner_block.insts.size());
  const auto bir_stale_owner_consumer =
      bir::find_fused_compare_operand_producer_facts(
          stale_owner_block,
          *fused_condition.lhs,
          *fused_condition.rhs,
          stale_owner_block.insts.size());
  if (!bir_consumer_boundary.available ||
      !bir_consumer_boundary.lhs.has_value() ||
      !bir_consumer_boundary.rhs.has_value() ||
      unavailable_route7_consumer.available ||
      missing_producer_route7_consumer.lhs.has_value() ||
      !missing_producer_route7_consumer.rhs.has_value() ||
      duplicate_reference_route7_consumer.available ||
      wrong_key_route7_consumer.lhs.has_value() ||
      !wrong_key_route7_consumer.rhs.has_value() ||
      wrong_relationship_route7_consumer.lhs.has_value() ||
      !wrong_relationship_route7_consumer.rhs.has_value() ||
      !bir_stale_owner_consumer.available ||
      stale_owner_route7_consumer.available) {
    return fail(
        "Route 7 fused compare consumer-boundary facts should fail closed for unavailable, missing-producer, duplicate-reference, wrong-key, wrong-relationship, and stale-reference cases without broad BIR fallback");
  }

  const auto prepared_condition =
      prepare::find_prepared_materialized_condition_producer(
          names,
          &source_producers,
          block_label,
          &block,
          bir::Value::named(bir::TypeKind::I1, "%cond"),
          block.insts.size());
  const auto bir_condition = bir::find_materialized_condition_producer_identity(
      block, bir::Value::named(bir::TypeKind::I1, "%cond"), block.insts.size());
  const auto route7_condition =
      bir::route7_comparison_instruction_record(&block, 3);
  if (!prepared_condition.has_value() ||
      !bir_condition.available ||
      bir_condition.binary != prepared_condition->binary ||
      bir_condition.instruction_index != prepared_condition->instruction_index ||
      names.value_names.find(bir_condition.condition_value_name) !=
          prepared_condition->condition_value_name ||
      !bir_condition.lhs.has_value() ||
      !bir_condition.rhs.has_value() ||
      !route7_condition ||
      route7_condition.binary != prepared_condition->binary ||
      route7_condition.instruction_index !=
          prepared_condition->instruction_index ||
      names.value_names.find(route7_condition.condition_value.name) !=
          prepared_condition->condition_value_name) {
    return fail(
        "prepared and BIR materialized condition producer facts and Route 7 records should match binary condition producers");
  }

  return 0;
}

int verify_production_bir_comparison_condition_producer_equivalence() {
  bir::Module module;
  module.functions.push_back(bir::Function{
      .name = "production_compare_facts",
      .return_type = bir::TypeKind::I32,
      .blocks =
          {
              bir::Block{
                  .label = "entry",
                  .insts =
                      {
                          bir::BinaryInst{
                              .opcode = bir::BinaryOpcode::Add,
                              .result = bir::Value::named(bir::TypeKind::I64, "%lhs"),
                              .operand_type = bir::TypeKind::I64,
                              .lhs = bir::Value::immediate_i64(10),
                              .rhs = bir::Value::immediate_i64(5),
                          },
                          bir::BinaryInst{
                              .opcode = bir::BinaryOpcode::Mul,
                              .result = bir::Value::named(bir::TypeKind::I64, "%rhs"),
                              .operand_type = bir::TypeKind::I64,
                              .lhs = bir::Value::immediate_i64(3),
                              .rhs = bir::Value::immediate_i64(7),
                          },
                          bir::BinaryInst{
                              .opcode = bir::BinaryOpcode::Slt,
                              .result = bir::Value::named(bir::TypeKind::I1, "%cond"),
                              .operand_type = bir::TypeKind::I64,
                              .lhs = bir::Value::named(bir::TypeKind::I64, "%lhs"),
                              .rhs = bir::Value::named(bir::TypeKind::I64, "%rhs"),
                          },
                      },
                  .terminator =
                      bir::Terminator{bir::CondBranchTerminator{
                          .condition = bir::Value::named(bir::TypeKind::I1, "%cond"),
                          .true_label = "then",
                          .false_label = "else",
                      }},
              },
              bir::Block{
                  .label = "then",
                  .terminator =
                      bir::Terminator{bir::ReturnTerminator{
                          .value = bir::Value::immediate_i32(1),
                      }},
              },
              bir::Block{
                  .label = "else",
                  .terminator =
                      bir::Terminator{bir::ReturnTerminator{
                          .value = bir::Value::immediate_i32(0),
                      }},
              },
          },
  });

  prepare::BirPreAlloc prealloc(
      module, c4c::default_target_profile(c4c::TargetArch::X86_64));
  prealloc.run_legalize();
  prealloc.run_stack_layout();
  prealloc.run_liveness();

  const auto& prepared = prealloc.prepared();
  const auto* function = prepared.module.functions.empty()
                             ? nullptr
                             : &prepared.module.functions.front();
  const auto* control_flow = prepared.control_flow.functions.empty()
                                 ? nullptr
                                 : &prepared.control_flow.functions.front();
  if (function == nullptr ||
      control_flow == nullptr ||
      function->blocks.empty() ||
      control_flow->branch_conditions.size() != 1U) {
    return fail(
        "production comparison fixture should publish one prepared branch condition");
  }

  const auto& block = function->blocks.front();
  const auto& branch_condition = control_flow->branch_conditions.front();
  const auto source_producers =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          prepared, *control_flow);
  const auto prepared_fused =
      prepare::find_prepared_fused_compare_operand_producer_facts(
          prepared.names,
          &source_producers,
          branch_condition.block_label,
          &block,
          branch_condition,
          block.insts.size());
  const auto bir_fused = bir::find_fused_compare_operand_producer_facts(
      block, *branch_condition.lhs, *branch_condition.rhs, block.insts.size());
  const auto route7_branch = bir::route7_branch_condition_record(&block);
  const auto route7_production_index =
      bir::route7_build_comparison_condition_index(*function);
  const auto route7_fused =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_production_index,
          block,
          *branch_condition.lhs,
          *branch_condition.rhs,
          block.insts.size());
  const auto indexed_route7_branch =
      bir::route7_find_branch_condition(route7_production_index, block);
  if (!prepared_fused.has_value() ||
      !bir_fused.available ||
      !route7_fused.available ||
      !branch_condition.can_fuse_with_branch ||
      branch_condition.kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !prepared_and_bir_comparison_operand_producer_match(
          prepared.names, block, prepared_fused->lhs, *branch_condition.lhs,
          block.insts.size()) ||
      !prepared_and_bir_comparison_operand_producer_match(
          prepared.names, block, prepared_fused->rhs, *branch_condition.rhs,
          block.insts.size()) ||
      !bir_fused.lhs.has_value() ||
      !bir_fused.rhs.has_value() ||
      bir_fused.lhs->integer_constant != std::optional<std::int64_t>{15} ||
      bir_fused.rhs->integer_constant != std::optional<std::int64_t>{21} ||
      !route7_fused.lhs.has_value() ||
      !route7_fused.rhs.has_value() ||
      route7_fused.lhs->integer_constant !=
          bir_fused.lhs->integer_constant ||
      route7_fused.rhs->integer_constant !=
          bir_fused.rhs->integer_constant ||
      !route7_branch ||
      route7_branch.status != bir::Route7ComparisonStatus::Available ||
      route7_branch.kind != bir::Route7BranchConditionKind::FusedCompare ||
      route7_branch.condition_value.name != branch_condition.condition_value.name ||
      route7_branch.true_label != "then" ||
      route7_branch.false_label != "else" ||
      route7_branch.comparison.instruction_index != 2 ||
      route7_branch.comparison.lhs.integer_constant !=
          std::optional<std::int64_t>{15} ||
      route7_branch.comparison.rhs.integer_constant !=
          std::optional<std::int64_t>{21} ||
      !indexed_route7_branch ||
      indexed_route7_branch.kind !=
          bir::Route7BranchConditionKind::FusedCompare ||
      indexed_route7_branch.condition_value.name !=
          branch_condition.condition_value.name ||
      indexed_route7_branch.comparison.instruction_index != 2 ||
      indexed_route7_branch.comparison.lhs.integer_constant !=
          std::optional<std::int64_t>{15} ||
      indexed_route7_branch.comparison.rhs.integer_constant !=
          std::optional<std::int64_t>{21}) {
    return fail(
        "production BIR fused compare producer facts and Route 7 branch records/index should match prepared branch-condition oracle");
  }

  const auto prepared_condition =
      prepare::find_prepared_materialized_condition_producer(
          prepared.names,
          &source_producers,
          branch_condition.block_label,
          &block,
          branch_condition.condition_value,
          block.insts.size());
  const auto bir_condition = bir::find_materialized_condition_producer_identity(
      block, branch_condition.condition_value, block.insts.size());
  if (!prepared_condition.has_value() ||
      !bir_condition.available ||
      bir_condition.binary != prepared_condition->binary ||
      bir_condition.instruction_index != prepared_condition->instruction_index ||
      prepared.names.value_names.find(bir_condition.condition_value_name) !=
          prepared_condition->condition_value_name ||
      !bir_condition.lhs.has_value() ||
      !bir_condition.rhs.has_value()) {
    return fail(
        "production BIR materialized condition facts should match prepared condition oracle");
  }

  return 0;
}

int verify_bir_call_result_source_identity_lookup() {
  prepare::PreparedNameTables names;
  const auto block_label = names.block_labels.intern("entry");
  const auto result_name = names.value_names.intern("%call.result");
  const auto high_lane_name = names.value_names.intern("%call.result.high");
  const auto result_value =
      bir::Value::named(bir::TypeKind::I128, "%call.result");
  const auto high_lane =
      bir::Value::named(bir::TypeKind::I64, "%call.result.high");

  bir::Block block{
      .label = "entry",
      .insts =
          {
              bir::CallInst{
                  .result = result_value,
                  .result_lanes =
                      {
                          result_value,
                          high_lane,
                      },
                  .callee = "produce_result_identity",
                  .return_type = bir::TypeKind::I128,
              },
          },
      .label_id = block_label,
  };
  constexpr std::size_t call_instruction_index = 0;
  const auto* call =
      std::get_if<bir::CallInst>(&block.insts[call_instruction_index]);
  if (call == nullptr) {
    return fail("BIR call-result identity fixture is malformed");
  }

  const prepare::PreparedCallResultPlan prepared_result{
      .instruction_index = call_instruction_index,
      .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
      .destination_value_id = prepare::PreparedValueId{700},
      .source_register_name =
          std::string{"abi-source-register-must-not-be-required"},
      .source_register_bank = prepare::PreparedRegisterBank::Gpr,
      .destination_slot_id = prepare::PreparedFrameSlotId{42},
      .destination_stack_offset_bytes = std::size_t{128},
  };
  const auto late_publication =
      prepare::find_prepared_call_result_late_publication(prepared_result);
  if (!late_publication.source_register_publication_available ||
      !prepared_result.destination_slot_id.has_value() ||
      call->result_abi.has_value()) {
    return fail(
        "BIR call-result identity fixture should keep prepared ABI/publication details outside the BIR query");
  }
  if (!prepared_and_bir_call_result_source_identity_match(
          names, prepared_result, prepare::PreparedValueId{700},
          result_name, block, *call, call_instruction_index)) {
    return fail(
        "BIR call-result source identity should match prepared destination value identity only");
  }
  const bir::Function route6_result_function{
      .name = "route6_result_sources",
      .blocks = {block},
  };
  const auto& route6_result_block = route6_result_function.blocks.front();
  const auto route6_result_index =
      bir::route6_build_call_use_source_index(route6_result_function);
  const auto* route6_indexed_result_call =
      std::get_if<bir::CallInst>(
          &route6_result_block.insts[call_instruction_index]);
  if (route6_indexed_result_call == nullptr) {
    return fail("Route 6 result index fixture is malformed");
  }
  const auto route6_result =
      bir::route6_call_result_source_record(block, *call, call_instruction_index);
  const auto indexed_route6_result =
      bir::route6_find_call_result_source(
          route6_result_index,
          route6_result_block,
          call_instruction_index,
          "produce_result_identity",
          result_value);
  if (!route6_result ||
      route6_result.status != bir::Route6CallUseStatus::Available ||
      route6_result.call != call ||
      route6_result.result_value != &*call->result ||
      route6_result.result_identity.name != "%call.result" ||
      route6_result.value_role != bir::Route6CallUseValueRole::Result ||
      !indexed_route6_result ||
      indexed_route6_result.result_value !=
          &*route6_indexed_result_call->result) {
    return fail(
        "Route 6 call-result record/index should expose result value provenance without ABI placement");
  }

  const prepare::PreparedAfterCallResultLaneBinding primary_lane{
      .value_id = prepare::PreparedValueId{700},
      .value_name = result_name,
      .block_index = 0,
      .instruction_index = call_instruction_index,
      .lane_index = 0,
  };
  const prepare::PreparedAfterCallResultLaneBinding high_prepared_lane{
      .value_id = prepare::PreparedValueId{701},
      .value_name = high_lane_name,
      .block_index = 0,
      .instruction_index = call_instruction_index,
      .lane_index = 1,
  };
  if (!prepared_and_bir_call_result_lane_source_identity_match(
          names, primary_lane, block, *call, call_instruction_index,
          result_value) ||
      !prepared_and_bir_call_result_lane_source_identity_match(
          names, high_prepared_lane, block, *call, call_instruction_index,
          high_lane)) {
    return fail(
        "BIR call-result lane identity should match prepared after-call result lane bindings");
  }
  const auto route6_primary_lane =
      bir::route6_call_result_lane_source_record(
          block, *call, call_instruction_index, result_value);
  const auto route6_high_lane =
      bir::route6_call_result_lane_source_record(
          block, *call, call_instruction_index, high_lane);
  const auto indexed_route6_high_lane =
      bir::route6_find_call_result_lane_source(
          route6_result_index,
          route6_result_block,
          call_instruction_index,
          "produce_result_identity",
          high_lane);
  const auto indexed_route6_missing_lane =
      bir::route6_find_call_result_lane_source(
          route6_result_index,
          route6_result_block,
          call_instruction_index,
          "produce_result_identity",
          bir::Value::named(bir::TypeKind::I64, "%missing.lane"));
  if (!route6_primary_lane ||
      route6_primary_lane.lane_index != 0 ||
      !route6_primary_lane.aliases_primary_result ||
      route6_primary_lane.lane_identity.name != "%call.result" ||
      !route6_high_lane ||
      route6_high_lane.lane_index != 1 ||
      route6_high_lane.aliases_primary_result ||
      route6_high_lane.lane_identity.name != "%call.result.high" ||
      !indexed_route6_high_lane ||
      indexed_route6_high_lane.lane_index != 1 ||
      indexed_route6_missing_lane ||
      indexed_route6_missing_lane.status != bir::Route6CallUseStatus::NoMatch) {
    return fail(
        "Route 6 call-result lane record/index should expose lane provenance and no-match behavior");
  }

  auto duplicate_lane_block = block;
  auto* duplicate_lane_call =
      std::get_if<bir::CallInst>(&duplicate_lane_block.insts[0]);
  if (duplicate_lane_call == nullptr) {
    return fail("BIR duplicate result-lane fixture is malformed");
  }
  duplicate_lane_call->result_lanes.push_back(high_lane);
  if (bir::find_call_result_lane_source_identity(
          duplicate_lane_block, *duplicate_lane_call, call_instruction_index,
          high_lane)
          .available) {
    return fail(
        "BIR call-result lane identity should fail closed for duplicate lane value identities");
  }
  const auto route6_duplicate_lane =
      bir::route6_call_result_lane_source_record(
          duplicate_lane_block,
          *duplicate_lane_call,
          call_instruction_index,
          high_lane);
  const bir::Function route6_duplicate_lane_function{
      .name = "route6_duplicate_lanes",
      .blocks = {duplicate_lane_block},
  };
  const auto route6_duplicate_lane_index =
      bir::route6_build_call_use_source_index(route6_duplicate_lane_function);
  const auto indexed_route6_duplicate_lane =
      bir::route6_find_call_result_lane_source(
          route6_duplicate_lane_index,
          route6_duplicate_lane_function.blocks.front(),
          call_instruction_index,
          "produce_result_identity",
          high_lane);
  const auto indexed_route6_duplicate_missing_lane =
      bir::route6_find_call_result_lane_source(
          route6_duplicate_lane_index,
          route6_duplicate_lane_function.blocks.front(),
          call_instruction_index,
          "produce_result_identity",
          bir::Value::named(bir::TypeKind::I64, "%not.duplicated"));
  if (route6_duplicate_lane ||
      route6_duplicate_lane.status !=
          bir::Route6CallUseStatus::DuplicateResultLane ||
      route6_duplicate_lane.lane_value != nullptr ||
      route6_duplicate_lane.lane_identity.name != "%call.result.high" ||
      indexed_route6_duplicate_lane ||
      indexed_route6_duplicate_lane.status !=
          bir::Route6CallUseStatus::DuplicateResultLane ||
      indexed_route6_duplicate_missing_lane ||
      indexed_route6_duplicate_missing_lane.status !=
          bir::Route6CallUseStatus::NoMatch) {
    return fail(
        "Route 6 call-result lane record/index should report duplicate status only for the duplicated lane identity");
  }

  bir::Block no_result_block{
      .label = "entry",
      .insts =
          {
              bir::CallInst{
                  .callee = "abi_only_result_identity_is_unavailable",
                  .return_type = bir::TypeKind::I64,
                  .result_abi =
                      bir::CallResultAbiInfo{
                          .type = bir::TypeKind::I64,
                          .primary_class = bir::AbiValueClass::Integer,
                      },
              },
          },
  };
  const auto* no_result_call =
      std::get_if<bir::CallInst>(&no_result_block.insts[0]);
  if (no_result_call == nullptr ||
      bir::find_call_result_source_identity(no_result_block, *no_result_call, 0)
          .available) {
    return fail(
        "BIR call-result source identity should require a result value, not result ABI placement");
  }
  const auto route6_missing_result =
      no_result_call == nullptr
          ? bir::Route6CallResultSourceRecord{}
          : bir::route6_call_result_source_record(
                no_result_block, *no_result_call, 0);
  if (route6_missing_result ||
      route6_missing_result.status !=
          bir::Route6CallUseStatus::MissingResult) {
    return fail(
        "Route 6 call-result record should explicitly report missing result values");
  }
  auto detached_call = *call;
  if (bir::find_call_result_source_identity(
          block, detached_call, call_instruction_index)
          .available ||
      bir::find_call_result_source_identity(block, *call, 1).available) {
    return fail(
        "BIR call-result source identity should fail closed for mismatched call boundaries");
  }
  const auto route6_detached_result =
      bir::route6_call_result_source_record(
          block, detached_call, call_instruction_index);
  const auto route6_missing_call =
      bir::route6_call_result_source_record(block, *call, 1);
  if (route6_detached_result ||
      route6_detached_result.status != bir::Route6CallUseStatus::WrongCall ||
      route6_missing_call ||
      route6_missing_call.status != bir::Route6CallUseStatus::MissingCall) {
    return fail(
        "Route 6 call-result record should distinguish wrong-call and missing-call boundaries");
  }

  return 0;
}

int verify_bir_return_chain_schema_and_index_lookup() {
  const bir::Value seed = bir::Value::named(bir::TypeKind::I64, "%seed");
  const bir::Value named_next =
      bir::Value::named(bir::TypeKind::I64, "%named.next");
  const bir::Value ret = bir::Value::named(bir::TypeKind::I64, "%ret");
  const bir::Block block{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = seed,
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::immediate_i64(10),
                  .rhs = bir::Value::immediate_i64(5),
              },
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Sub,
                  .result = ret,
                  .operand_type = bir::TypeKind::I64,
                  .lhs = seed,
                  .rhs = named_next,
              },
          },
      .terminator =
          bir::Terminator{bir::ReturnTerminator{
              .value = ret,
          }},
  };
  const bir::Function function{
      .name = "route8_return_chain",
      .blocks = {block},
  };
  const auto& indexed_block = function.blocks.front();
  const auto* indexed_seed_binary =
      std::get_if<bir::BinaryInst>(&indexed_block.insts[0]);
  const auto* indexed_ret_binary =
      std::get_if<bir::BinaryInst>(&indexed_block.insts[1]);
  if (indexed_seed_binary == nullptr || indexed_ret_binary == nullptr) {
    return fail("Route 8 positive fixture is malformed");
  }
  const auto index = bir::route8_build_return_chain_index(function);
  const auto first_key =
      bir::route8_return_chain_value_key(&function, indexed_block, 0, seed);
  const auto terminal_key =
      bir::route8_return_chain_value_key(&function, indexed_block, 1, ret);
  const auto first_record =
      bir::route8_find_return_chain_record(index, first_key);
  const auto terminal_record =
      bir::route8_find_return_chain_record(index, terminal_key);
  const auto terminal_identity =
      bir::route8_find_return_chain_terminal_value(index, first_key);
  const auto next_identity =
      bir::route8_find_return_chain_next_operand_value(index, first_key);
  if (!first_record ||
      first_record.status != bir::Route8ReturnChainStatus::Available ||
      first_record.key.instruction_index != 0 ||
      first_record.key.chain_value.name != "%seed" ||
      first_record.terminal_return_value.name != "%ret" ||
      first_record.next_operand_value.name != "%named.next" ||
      !terminal_record ||
      terminal_record.status != bir::Route8ReturnChainStatus::Available ||
      terminal_record.key.instruction_index != 1 ||
      terminal_record.terminal_return_value.name != "%ret" ||
      terminal_record.next_operand_value ||
      terminal_identity.name != "%ret" ||
      next_identity.name != "%named.next") {
    return fail(
        "Route 8 return-chain index should expose terminal and immediate named next-operand identities");
  }

  const auto block_index = bir::route8_build_return_chain_index(indexed_block);
  const auto block_key =
      bir::route8_return_chain_value_key(nullptr, indexed_block, 0, seed);
  const auto block_record =
      bir::route8_find_return_chain_record(block_index, block_key);
  if (!block_record ||
      block_record.terminal_return_value.name != "%ret" ||
      block_record.next_operand_value.name != "%named.next") {
    return fail(
        "Route 8 block-local return-chain index should expose the same schema identities");
  }

  const auto route8_ret_terminal =
      bir::route8_find_return_chain_terminal_value(index, terminal_key);
  const auto route8_ret_next =
      bir::route8_find_return_chain_next_operand_value(index, terminal_key);
  if (!route8_ret_terminal || route8_ret_terminal.name != "%ret") {
    return fail(
        "Route 8 return-chain helper should expose accepted terminal answers");
  }
  if (!next_identity || next_identity.name != "%named.next" || route8_ret_next) {
    return fail(
        "Route 8 return-chain helper should expose accepted next-operand answers");
  }

  const bir::Block unsupported_opcode_block{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Eq,
                  .result = seed,
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::immediate_i64(1),
                  .rhs = bir::Value::immediate_i64(1),
              },
          },
      .terminator =
          bir::Terminator{bir::ReturnTerminator{
              .value = seed,
          }},
  };
  const auto unsupported_index =
      bir::route8_build_return_chain_index(unsupported_opcode_block);
  const auto unsupported_record = bir::route8_find_return_chain_record(
      unsupported_index,
      bir::route8_return_chain_value_key(
          nullptr, unsupported_opcode_block, 0, seed));
  if (unsupported_record ||
      unsupported_record.status != bir::Route8ReturnChainStatus::NoMatch) {
    return fail(
        "Route 8 return-chain index should reject unsupported binary opcodes");
  }
  if (!route8_return_chain_queries_fail_closed(
          unsupported_index,
          bir::route8_return_chain_value_key(
              nullptr, unsupported_opcode_block, 0, seed),
          "Route 8 return-chain helpers should fail closed for unsupported opcodes")) {
    return 1;
  }

  const bir::Block unnamed_chain_block{
      .label = "entry",
      .insts =
          {
              bir::BinaryInst{
                  .opcode = bir::BinaryOpcode::Add,
                  .result = bir::Value::immediate_i64(15),
                  .operand_type = bir::TypeKind::I64,
                  .lhs = bir::Value::immediate_i64(10),
                  .rhs = bir::Value::immediate_i64(5),
              },
          },
      .terminator =
          bir::Terminator{bir::ReturnTerminator{
              .value = bir::Value::immediate_i64(15),
          }},
  };
  const auto unnamed_chain_index =
      bir::route8_build_return_chain_index(unnamed_chain_block);
  const auto unnamed_chain_record = bir::route8_find_return_chain_record(
      unnamed_chain_index,
      bir::route8_return_chain_value_key(
          nullptr, unnamed_chain_block, 0, bir::Value::immediate_i64(15)));
  if (unnamed_chain_record ||
      unnamed_chain_record.status !=
          bir::Route8ReturnChainStatus::NoMatch) {
    return fail(
        "Route 8 return-chain index should reject unnamed chain values");
  }
  if (!route8_return_chain_queries_fail_closed(
          unnamed_chain_index,
          bir::route8_return_chain_value_key(
              nullptr, unnamed_chain_block, 0, bir::Value::immediate_i64(15)),
          "Route 8 return-chain helpers should fail closed for unnamed chain values")) {
    return 1;
  }

  auto unnamed_terminal_block = block;
  unnamed_terminal_block.terminator.value = bir::Value::immediate_i64(25);
  const auto unnamed_terminal_index =
      bir::route8_build_return_chain_index(unnamed_terminal_block);
  const auto unnamed_terminal_record = bir::route8_find_return_chain_record(
      unnamed_terminal_index,
      bir::route8_return_chain_value_key(
          nullptr, unnamed_terminal_block, 0, seed));
  if (unnamed_terminal_record ||
      unnamed_terminal_record.status != bir::Route8ReturnChainStatus::NoMatch) {
    return fail(
        "Route 8 return-chain index should reject unnamed terminal return values");
  }
  if (!route8_return_chain_queries_fail_closed(
          unnamed_terminal_index,
          bir::route8_return_chain_value_key(
              nullptr, unnamed_terminal_block, 0, seed),
          "Route 8 return-chain helpers should fail closed for unnamed terminal return values")) {
    return 1;
  }

  auto broken_walk_block = block;
  auto* broken_next =
      std::get_if<bir::BinaryInst>(&broken_walk_block.insts[1]);
  if (broken_next == nullptr) {
    return fail("Route 8 broken-walk fixture is malformed");
  }
  broken_next->lhs = bir::Value::named(bir::TypeKind::I64, "%not.seed");
  const auto broken_walk_index =
      bir::route8_build_return_chain_index(broken_walk_block);
  const auto broken_walk_record = bir::route8_find_return_chain_record(
      broken_walk_index,
      bir::route8_return_chain_value_key(
          nullptr, broken_walk_block, 0, seed));
  if (broken_walk_record ||
      broken_walk_record.status != bir::Route8ReturnChainStatus::NoMatch) {
    return fail(
        "Route 8 return-chain index should fail closed for broken same-block walks");
  }
  if (!route8_return_chain_queries_fail_closed(
          broken_walk_index,
          bir::route8_return_chain_value_key(
              nullptr, broken_walk_block, 0, seed),
          "Route 8 return-chain helpers should fail closed for broken same-block walks")) {
    return 1;
  }

  auto non_return_block = block;
  non_return_block.terminator = bir::Terminator{bir::BranchTerminator{
      .target_label = "exit",
  }};
  const auto non_return_index =
      bir::route8_build_return_chain_index(non_return_block);
  const auto non_return_record = bir::route8_find_return_chain_record(
      non_return_index,
      bir::route8_return_chain_value_key(
          nullptr, non_return_block, 0, seed));
  if (non_return_record ||
      non_return_record.status != bir::Route8ReturnChainStatus::NoMatch) {
    return fail(
        "Route 8 return-chain index should reject non-return terminators");
  }
  if (!route8_return_chain_queries_fail_closed(
          non_return_index,
          bir::route8_return_chain_value_key(
              nullptr, non_return_block, 0, seed),
          "Route 8 return-chain helpers should fail closed for non-return terminators")) {
    return 1;
  }

  const bir::Function cross_block_function{
      .name = "route8_cross_block",
      .blocks =
          {
              bir::Block{
                  .label = "entry",
                  .insts =
                      {
                          bir::BinaryInst{
                              .opcode = bir::BinaryOpcode::Add,
                              .result = seed,
                              .operand_type = bir::TypeKind::I64,
                              .lhs = bir::Value::immediate_i64(10),
                              .rhs = bir::Value::immediate_i64(5),
                          },
                      },
                  .terminator =
                      bir::Terminator{bir::BranchTerminator{
                          .target_label = "exit",
                      }},
              },
              bir::Block{
                  .label = "exit",
                  .terminator =
                      bir::Terminator{bir::ReturnTerminator{
                          .value = seed,
                      }},
              },
          },
  };
  const auto cross_block_index =
      bir::route8_build_return_chain_index(cross_block_function);
  const auto cross_block_record = bir::route8_find_return_chain_record(
      cross_block_index,
      bir::route8_return_chain_value_key(
          &cross_block_function,
          cross_block_function.blocks.front(),
          0,
          seed));
  if (cross_block_record ||
      cross_block_record.status != bir::Route8ReturnChainStatus::NoMatch) {
    return fail(
        "Route 8 return-chain index should not stitch return chains across blocks");
  }
  if (!route8_return_chain_queries_fail_closed(
          cross_block_index,
          bir::route8_return_chain_value_key(
              &cross_block_function,
              cross_block_function.blocks.front(),
              0,
              seed),
          "Route 8 return-chain helpers should fail closed for cross-block return relationships")) {
    return 1;
  }

  const auto missing_instruction_record = bir::route8_find_return_chain_record(
      index,
      bir::route8_return_chain_value_key(
          &function, indexed_block, indexed_block.insts.size(), seed));
  if (missing_instruction_record ||
      missing_instruction_record.status !=
          bir::Route8ReturnChainStatus::MissingInstruction) {
    return fail(
        "Route 8 return-chain index should report missing instruction keys");
  }
  if (bir::route8_find_return_chain_terminal_value(
          index,
          bir::route8_return_chain_value_key(
              &function, indexed_block, indexed_block.insts.size(), seed)) ||
      bir::route8_find_return_chain_next_operand_value(
          index,
          bir::route8_return_chain_value_key(
              &function, indexed_block, indexed_block.insts.size(), seed))) {
    return fail(
        "Route 8 return-chain helpers should fail closed for missing instruction keys");
  }

  auto manual_duplicate_index = index;
  const bir::Value conflicting_terminal =
      bir::Value::named(bir::TypeKind::I64, "%conflicting.ret");
  auto terminal_conflict_record = first_record;
  terminal_conflict_record.terminal_return_value =
      bir::route1_source_value_identity(conflicting_terminal);
  manual_duplicate_index.records.push_back(terminal_conflict_record);
  const auto manual_duplicate_terminal_record =
      bir::route8_find_return_chain_record(manual_duplicate_index, first_key);
  if (manual_duplicate_terminal_record ||
      manual_duplicate_terminal_record.status !=
          bir::Route8ReturnChainStatus::DuplicateRecord ||
      bir::route8_find_return_chain_terminal_value(
          manual_duplicate_index, first_key) ||
      bir::route8_find_return_chain_next_operand_value(
          manual_duplicate_index, first_key)) {
    return fail(
        "Route 8 return-chain helpers should fail closed for duplicate terminal conflicts");
  }

  auto manual_next_duplicate_index = index;
  const bir::Value conflicting_next =
      bir::Value::named(bir::TypeKind::I64, "%conflicting.next");
  auto next_conflict_record = first_record;
  next_conflict_record.next_operand_value =
      bir::route1_source_value_identity(conflicting_next);
  manual_next_duplicate_index.records.push_back(next_conflict_record);
  const auto manual_duplicate_next_record = bir::route8_find_return_chain_record(
      manual_next_duplicate_index, first_key);
  if (manual_duplicate_next_record ||
      manual_duplicate_next_record.status !=
          bir::Route8ReturnChainStatus::DuplicateRecord ||
      bir::route8_find_return_chain_terminal_value(
          manual_next_duplicate_index, first_key) ||
      bir::route8_find_return_chain_next_operand_value(
          manual_next_duplicate_index, first_key)) {
    return fail(
        "Route 8 return-chain helpers should fail closed for duplicate next-operand conflicts");
  }

  auto manual_duplicate_record = bir::route8_find_return_chain_record(
      manual_duplicate_index, first_key);
  if (manual_duplicate_record ||
      manual_duplicate_record.status !=
          bir::Route8ReturnChainStatus::DuplicateRecord ||
      bir::route8_find_return_chain_terminal_value(
          manual_duplicate_index, first_key) ||
      bir::route8_find_return_chain_next_operand_value(
          manual_duplicate_index, first_key)) {
    return fail(
        "Route 8 return-chain lookups should fail closed for duplicate conflicting records");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = verify_prepared_home_same_register_helper(); result != 0) {
    return result;
  }
  if (const int result = verify_prepared_parallel_copy_register_move_helpers();
      result != 0) {
    return result;
  }
  if (const int result = verify_linear_function_lookup(); result != 0) {
    return result;
  }
  if (const int result = verify_diamond_function_lookup(); result != 0) {
    return result;
  }
  if (const int result = verify_edge_publication_lookup_key_preserves_full_tuple();
      result != 0) {
    return result;
  }
  if (const int result =
          verify_edge_publication_shared_source_and_parallel_copy_facts();
      result != 0) {
    return result;
  }
  if (const int result = verify_current_block_join_parallel_copy_source_query();
      result != 0) {
    return result;
  }
  if (const int result = verify_same_width_i32_stack_source_publication_facts();
      result != 0) {
    return result;
  }
  if (const int result = verify_aggregate_stack_source_authority_status();
      result != 0) {
    return result;
  }
  if (const int result = verify_edge_publication_source_producer_facts();
      result != 0) {
    return result;
  }
  if (const int result = verify_before_return_abi_move_source_bank_lookup();
      result != 0) {
    return result;
  }
  if (const int result = verify_prepared_frame_address_offset_lookup();
      result != 0) {
    return result;
  }
  if (const int result = verify_prepared_memory_access_lookup(); result != 0) {
    return result;
  }
  if (const int result = verify_bir_direct_memory_access_identity_lookup();
      result != 0) {
    return result;
  }
  if (const int result = verify_direct_global_select_chain_dependency_query();
      result != 0) {
    return result;
  }
  if (const int result = verify_prepared_same_block_scalar_source_facts();
      result != 0) {
    return result;
  }
  if (const int result = verify_bir_block_entry_publication_identity_lookup();
      result != 0) {
    return result;
  }
  if (const int result =
          verify_bir_call_argument_publication_source_routing_lookup();
      result != 0) {
    return result;
  }
  if (const int result =
          verify_bir_call_argument_source_producer_materialization_lookup();
      result != 0) {
    return result;
  }
  if (const int result =
          verify_bir_comparison_condition_producer_identity_lookup();
      result != 0) {
    return result;
  }
  if (const int result =
          verify_prepared_bir_comparison_condition_producer_equivalence();
      result != 0) {
    return result;
  }
  if (const int result =
          verify_production_bir_comparison_condition_producer_equivalence();
      result != 0) {
    return result;
  }
  if (const int result = verify_bir_call_result_source_identity_lookup();
      result != 0) {
    return result;
  }
  if (const int result = verify_bir_return_chain_schema_and_index_lookup();
      result != 0) {
    return result;
  }
  return EXIT_SUCCESS;
}
