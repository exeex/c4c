#pragma once

#include "../module/module.hpp"
#include "../../query.hpp"
#include "../../../prealloc/publication_plans.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

using SameBlockSelectProducer = c4c::backend::mir::SameBlockSelectProducer;

[[nodiscard]] SameBlockSelectProducer find_prepared_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<prepare::PreparedEdgePublicationSourceProducer>
prepared_publication_source_producer_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value);

[[nodiscard]] const bir::Inst* prepared_source_producer_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublicationSourceProducer& producer);

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0);

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer);

[[nodiscard]] bool prepared_query_current_block_join_parallel_copy_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst);

struct CurrentBlockJoinPreparedQueryRouting {
  const module::BlockLoweringContext* context = nullptr;
  std::vector<bool> incoming_expressions;
  std::vector<bool> sources;
};

[[nodiscard]] CurrentBlockJoinPreparedQueryRouting
build_current_block_join_prepared_query_routing(
    const module::BlockLoweringContext& context);

[[nodiscard]] bool current_block_join_prepared_query_incoming_expression(
    const CurrentBlockJoinPreparedQueryRouting& routing,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst);

[[nodiscard]] bool current_block_join_prepared_query_source(
    const CurrentBlockJoinPreparedQueryRouting& routing,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst);

[[nodiscard]] bool block_entry_move_clobbers_current_join_publication(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction);

[[nodiscard]] bool prepared_value_home_reads_register_index(
    const prepare::PreparedValueHome& home,
    std::uint8_t register_index);

enum class Route1PublicationSourceProducerStatus {
  Unavailable,
  NoProducer,
  Available,
};

enum class Route1PublicationIntegerConstantStatus {
  Unavailable,
  NoProducer,
  NonConstant,
  Constant,
};

struct Route1PublicationSourceProducerView {
  Route1PublicationSourceProducerStatus status =
      Route1PublicationSourceProducerStatus::Unavailable;
  const bir::Inst* instruction = nullptr;
  const bir::Value* produced_value = nullptr;
  std::size_t instruction_index = 0;
  bir::Route1ProducerKind producer_kind = bir::Route1ProducerKind::Unknown;
  bir::Route1MaterializationAvailability materialization;
  Route1PublicationIntegerConstantStatus integer_constant_status =
      Route1PublicationIntegerConstantStatus::Unavailable;
  bir::Route1ImmediateIntegerConstant integer_constant;

  [[nodiscard]] explicit operator bool() const {
    return status == Route1PublicationSourceProducerStatus::Available &&
           instruction != nullptr && produced_value != nullptr;
  }
};

[[nodiscard]] Route1PublicationSourceProducerView
route1_publication_source_producer_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] bool value_publication_may_read_register_index(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t register_index,
    unsigned depth = 0);

}  // namespace c4c::backend::aarch64::codegen
