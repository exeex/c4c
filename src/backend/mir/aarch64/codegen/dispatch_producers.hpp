#pragma once

#include "../module/module.hpp"
#include "../../query.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;

using SameBlockSelectProducer = c4c::backend::mir::SameBlockSelectProducer;

[[nodiscard]] SameBlockSelectProducer find_prepared_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0);

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer);

[[nodiscard]] const bir::Global* find_load_global_target(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global);

[[nodiscard]] std::string load_global_symbol_label(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const bir::Global* target_global);

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

}  // namespace c4c::backend::aarch64::codegen
