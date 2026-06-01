#pragma once

#include "alu.hpp"
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

struct EdgeProducerContext {
  module::BlockLoweringContext context;
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
};

[[nodiscard]] bool prepared_edge_select_source_is_destination_register(
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home);

[[nodiscard]] bool edge_value_publication_may_read_register_index(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t register_index,
    const prepare::PreparedEdgePublication* prepared_publication = nullptr,
    unsigned depth = 0);

[[nodiscard]] bool emit_edge_load_local_to_register(
    const module::BlockLoweringContext& edge_context,
    const EdgeProducerContext& producer,
    const bir::LoadLocalInst& load,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication = nullptr);

[[nodiscard]] bool emit_edge_value_publication_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    const prepare::PreparedEdgePublication* prepared_publication = nullptr);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_predecessor_join_source_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedEdgePublication& publication,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool should_emit_block_entry_edge_copy_move(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction);

[[nodiscard]] std::vector<module::MachineInstruction>
lower_predecessor_select_parallel_copy_sources(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
