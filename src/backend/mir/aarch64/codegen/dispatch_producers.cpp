#include "dispatch_producers.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "dispatch.hpp"

#include "dispatch_publication.hpp"

#include "../../../prealloc/publication_plans.hpp"
#include "../../../prealloc/select_chain_lookups.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <variant>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

[[nodiscard]] const bir::Value* instruction_result_value_ref(const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> const bir::Value* {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                      std::is_same_v<T, bir::CastInst> ||
                      std::is_same_v<T, bir::SelectInst> ||
                      std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          return &typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result.has_value() ? &*typed_inst.result : nullptr;
        }
        return nullptr;
      },
      inst);
}

[[nodiscard]] std::optional<prepare::PreparedValueId>
instruction_result_prepared_value_id(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHomeLookups* value_home_lookups,
    const bir::Inst& inst) {
  const auto* result = instruction_result_value_ref(inst);
  if (result == nullptr ||
      result->kind != bir::Value::Kind::Named ||
      result->name.empty()) {
    return std::nullopt;
  }
  const auto result_value_name = prepared_named_value_id(context, *result);
  if (!result_value_name.has_value()) {
    return std::nullopt;
  }
  return prepare::find_indexed_prepared_value_id(value_home_lookups,
                                                context.function.regalloc,
                                                context.function.value_locations,
                                                *result_value_name);
}

struct CurrentBlockJoinRoutedValueIdentity {
  const prepare::PreparedFunctionLookups* owner = nullptr;
  const prepare::PreparedFunctionLookups* attached = nullptr;
  c4c::BlockLabelId successor_label = c4c::kInvalidBlockLabel;
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
};

[[nodiscard]] std::optional<CurrentBlockJoinRoutedValueIdentity>
current_block_join_routed_value_identity(const module::BlockLoweringContext& context,
                                         const bir::Inst& inst) {
  const auto& function = context.function;
  if (function.prepared_lookups_owner == nullptr ||
      function.prepared_lookups != function.prepared_lookups_owner.get() ||
      function.prepared == nullptr || context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* result = instruction_result_value_ref(inst);
  if (result == nullptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, *result);
  const auto value_id = instruction_result_prepared_value_id(
      context, &function.prepared_lookups->value_homes, inst);
  if (!value_name.has_value() || !value_id.has_value()) {
    return std::nullopt;
  }
  return CurrentBlockJoinRoutedValueIdentity{
      .owner = function.prepared_lookups_owner.get(),
      .attached = function.prepared_lookups,
      .successor_label = context.control_flow_block->block_label,
      .value_id = *value_id,
      .value_name = *value_name,
  };
}

[[nodiscard]] std::optional<prepare::PreparedSameBlockScalarProducer>
prepared_same_block_publication_source_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.prepared_lookups_owner == nullptr ||
      context.function.prepared_lookups !=
          context.function.prepared_lookups_owner.get() ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  return prepare::find_prepared_same_block_scalar_producer(
      context.function.prepared->names,
      &context.function.prepared_lookups->edge_publication_source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      *value_name,
      value.type,
      before_instruction_index);
}

}  // namespace

[[nodiscard]] SameBlockSelectProducer find_prepared_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto producer = prepared_same_block_publication_source_producer(
      context, value, before_instruction_index);
  if (!producer.has_value() ||
      producer->producer.kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization ||
      producer->instruction == nullptr) {
    return {};
  }
  const auto* select = std::get_if<bir::SelectInst>(producer->instruction);
  if (select == nullptr || producer->producer.instruction_index !=
                               producer->instruction_index ||
      select->result.kind != bir::Value::Kind::Named ||
      select->result.name != value.name || select->result.type != value.type) {
    return {};
  }
  return SameBlockSelectProducer{
      .status = bir::BirViewStatus::Available,
      .select = select,
      .produced_value = &select->result,
      .block_label = context.bir_block->label,
      .instruction_index = producer->instruction_index,
  };
}

[[nodiscard]] std::optional<prepare::PreparedEdgePublicationSourceProducer>
prepared_publication_source_producer_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  if (context.function.prepared_lookups_owner == nullptr ||
      context.function.prepared_lookups !=
          context.function.prepared_lookups_owner.get()) {
    return std::nullopt;
  }
  const auto* producer =
      prepare::find_indexed_prepared_edge_publication_source_producer(
          &context.function.prepared_lookups->edge_publication_source_producers,
          *value_name);
  return producer != nullptr
             ? std::optional<prepare::PreparedEdgePublicationSourceProducer>{
                   *producer}
             : std::nullopt;
}

[[nodiscard]] const bir::Inst* prepared_source_producer_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducer& producer) {
  if (context.bir_block == nullptr || context.control_flow_block == nullptr ||
      producer.block_label != context.control_flow_block->block_label ||
      producer.instruction_index >= context.bir_block->insts.size() ||
      producer.kind == prepare::PreparedEdgePublicationSourceProducerKind::Unknown ||
      producer.kind == prepare::PreparedEdgePublicationSourceProducerKind::Immediate) {
    return nullptr;
  }
  return &context.bir_block->insts[producer.instruction_index];
}

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth [[maybe_unused]]) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  const auto dependency = mir::find_bir_select_chain_direct_global_dependency(
      mir::BirSelectChainIdentityRequest{
          .block = context.bir_block,
          .block_label = context.bir_block->label,
          .root_value = &value,
          .before_instruction_index = before_instruction_index,
      });
  return dependency.contains_direct_global_load;
}

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer) {
  return mir::producer_instruction_index(context.bir_block, producer);
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] bool prepared_query_current_block_join_parallel_copy_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  return current_block_join_prepared_query_source(context, inst);
}

[[nodiscard]] bool current_block_join_prepared_query_incoming_expression(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  const auto identity = current_block_join_routed_value_identity(context, inst);
  return identity.has_value() && static_cast<bool>(
      prepare::query_attached_prepared_current_block_join_incoming_expression(
          identity->owner,
          identity->attached,
          identity->successor_label,
          identity->value_id,
          identity->value_name));
}

[[nodiscard]] bool current_block_join_prepared_query_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  const auto identity = current_block_join_routed_value_identity(context, inst);
  return identity.has_value() && static_cast<bool>(
      prepare::query_attached_prepared_current_block_join_source(
          identity->owner,
          identity->attached,
          identity->successor_label,
          identity->value_id,
          identity->value_name));
}

[[nodiscard]] bool block_entry_move_clobbers_current_join_publication(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr || !move_record->destination_register.has_value()) {
    return false;
  }
  const auto publications = prepare::collect_prepared_block_entry_publications(
      context.function.value_locations, context.control_flow_block->block_label);
  for (const auto& publication : publications) {
    if (!prepare::prepared_block_entry_publication_available(publication) ||
        !publication.destination_register_name.has_value()) {
      continue;
    }
    const auto parsed =
        abi::parse_aarch64_register_name(*publication.destination_register_name);
    if (!parsed.has_value()) {
      continue;
    }
    RegisterOperand published{
        .reg = *parsed,
        .role = RegisterOperandRole::StoragePlan,
        .value_id =
            publication.home != nullptr
                ? std::optional<prepare::PreparedValueId>{publication.home->value_id}
                : std::nullopt,
        .value_name = publication.home != nullptr ? publication.home->value_name
                                                  : c4c::kInvalidValueName,
        .expected_view = parsed->view,
    };
    if (registers_alias(published, *move_record->destination_register)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool prepared_value_home_reads_register_index(
    const prepare::PreparedValueHome& home,
    std::uint8_t register_index) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return false;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
  return parsed.has_value() && parsed->bank == abi::RegisterBank::GeneralPurpose &&
         parsed->index == register_index;
}

[[nodiscard]] bool value_publication_may_read_register_index(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t register_index,
    unsigned depth) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const auto* home = prepared_value_home_for_value(context, value);
  if (home != nullptr && value_has_current_block_entry_publication(context, *home)) {
    return prepared_value_home_reads_register_index(*home, register_index);
  }
  const auto producer_record = prepared_same_block_publication_source_producer(
      context, value, before_instruction_index);
  if (!producer_record.has_value()) {
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }
  const bir::Inst* producer = producer_record->instruction;
  const auto producer_index = producer_record->instruction_index;
  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return value_publication_may_read_register_index(
        context, operand, producer_index, register_index, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer); binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    return value_publication_may_read_register_index(
               context, lhs, producer_index, register_index, depth + 1) ||
           value_publication_may_read_register_index(
               context, rhs, producer_index, register_index, depth + 1);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer); select != nullptr) {
    return value_publication_may_read_register_index(
               context, select->true_value, producer_index, register_index, depth + 1) ||
           value_publication_may_read_register_index(
               context, select->false_value, producer_index, register_index, depth + 1);
  }
  return false;
}

}  // namespace c4c::backend::aarch64::codegen
