#pragma once

#include "dispatch_producers.hpp"
#include "dispatch_publication_common.hpp"
#include "dispatch_value_materialization.hpp"

#include "alu.hpp"
#include "../abi/abi.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

struct EdgeProducerContext {
  module::BlockLoweringContext context;
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
};

[[nodiscard]] bool prepared_edge_select_source_is_destination_register(
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home);

[[nodiscard]] std::optional<module::BlockLoweringContext> unique_branch_predecessor_context(
    const module::BlockLoweringContext& context);

[[nodiscard]] std::optional<EdgeProducerContext> find_edge_named_producer(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    std::string_view value_name,
    std::size_t successor_before_instruction_index);

[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_memory_access(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] bool prepared_memory_access_matches_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryAccess* access,
    const bir::Inst& inst);

[[nodiscard]] bool edge_value_publication_may_read_register_index(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t register_index,
    unsigned depth = 0);

[[nodiscard]] bool emit_edge_load_local_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const EdgeProducerContext& producer,
    const bir::LoadLocalInst& load,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool emit_edge_value_publication_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_predecessor_join_source_publication(
    const module::BlockLoweringContext& context,
    const bir::Block& successor,
    std::size_t source_index,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::string select_chain_label(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    c4c::ValueNameId root_value_name,
    std::uint8_t target_index,
    std::size_t label_index,
    std::string_view suffix);

[[nodiscard]] bool emit_select_chain_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::size_t root_instruction_index,
    c4c::ValueNameId root_value_name,
    std::vector<std::string>& lines,
    std::size_t& label_index,
    std::vector<std::string_view>& active_values,
    bool reload_current_memory_loads = false);

[[nodiscard]] std::optional<module::MachineInstruction>
make_select_chain_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines);

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_direct_global_select_chain_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen
