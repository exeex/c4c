#include "dispatch_lookup.hpp"

#include "../abi/abi.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../../../prealloc/publication_plans.hpp"

namespace c4c::backend::aarch64::codegen {
namespace abi = c4c::backend::aarch64::abi;
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

}  // namespace

bool is_scalar_call_argument_producer_opcode(bir::BinaryOpcode opcode) {
  return prepare::prepared_call_argument_binary_producer_opcode_is_materializable(
      opcode);
}

namespace {

[[nodiscard]] std::optional<prepare::PreparedCallArgumentSourceProducerMaterialization>
prepared_call_argument_source_producer_materialization(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      context.control_flow_block == nullptr ||
      context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  if (context.function.prepared_lookups != nullptr) {
    return prepare::find_prepared_call_argument_source_producer_materialization(
        context.function.prepared->names,
        &context.function.prepared_lookups->edge_publication_source_producers,
        context.control_flow_block->block_label,
        context.bir_block,
        value,
        before_instruction_index);
  }
  if (context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  const auto source_producers =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          *context.function.prepared,
          *context.function.control_flow);
  return prepare::find_prepared_call_argument_source_producer_materialization(
      context.function.prepared->names,
      &source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      value,
      before_instruction_index);
}

}  // namespace

std::optional<RegisterOperand> make_named_prepared_result_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home = prepare::find_indexed_prepared_value_home(
      context.function.value_home_lookups,
      context.function.regalloc,
      context.function.value_locations,
      *value_name);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(value.type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() ||
      parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(parsed->index, *expected_view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *viewed,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = expected_view,
  };
}

bool emitted_scalar_value_available(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value() &&
         find_emitted_scalar_register(scalar_state, *value_name).has_value();
}

std::optional<std::size_t> find_same_block_scalar_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto materialization =
      prepared_call_argument_source_producer_materialization(
          context, value, before_instruction_index);
  if (!materialization.has_value() ||
      materialization->producer.producer.kind !=
          prepare::PreparedEdgePublicationSourceProducerKind::Binary ||
      materialization->producer.producer.binary == nullptr) {
    return std::nullopt;
  }
  return materialization->producer.instruction_index;
}

bool has_same_block_load_local_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      context.control_flow_block == nullptr ||
      context.function.prepared == nullptr ||
      context.function.prepared_lookups == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  return prepare::find_prepared_same_block_load_local_source_producer(
             context.function.prepared->names,
             context.function.prepared->stack_layout,
             &context.function.prepared_lookups->memory_accesses,
             &context.function.prepared_lookups->edge_publication_source_producers,
             context.control_flow_block->block_label,
             context.bir_block,
             value,
             before_instruction_index)
      .has_value();
}

}  // namespace c4c::backend::aarch64::codegen
