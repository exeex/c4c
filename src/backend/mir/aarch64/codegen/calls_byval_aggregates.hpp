#pragma once

#include "calls.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::string_view aggregate_stack_copy_load_mnemonic(
    std::size_t width_bytes);
[[nodiscard]] std::string_view aggregate_stack_copy_store_mnemonic(
    std::size_t width_bytes);
[[nodiscard]] std::optional<abi::RegisterReference> aggregate_stack_copy_scratch(
    std::size_t width_bytes);
[[nodiscard]] std::vector<std::size_t> aggregate_stack_copy_chunks(
    std::size_t size_bytes);
[[nodiscard]] std::string_view aggregate_register_lane_load_mnemonic(
    std::size_t width_bytes);
[[nodiscard]] abi::RegisterReference aggregate_register_lane_load_register(
    abi::RegisterReference reg,
    std::size_t width_bytes);
[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_scratch(
    const RegisterOperand& destination);
[[nodiscard]] MemoryOperand aggregate_register_lane_memory(MemoryOperand memory,
                                                          std::size_t byte_offset,
                                                          std::size_t width_bytes);
[[nodiscard]] bool aggregate_register_lane_memory_is_printable(
    const MemoryOperand& memory,
    std::size_t width_bytes);
[[nodiscard]] std::optional<std::size_t> aggregate_register_lane_printable_chunk(
    const MemoryOperand& memory,
    std::size_t source_offset,
    std::size_t remaining);
[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_destination(
    const RegisterOperand& destination,
    std::size_t lane_index);
[[nodiscard]] bool is_aggregate_register_lane_publication(
    const CallBoundaryMoveInstructionRecord& move);

struct AggregateRegisterLaneStore {
  std::size_t source_offset = 0;
  std::int64_t stack_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 1;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
};

[[nodiscard]] bool is_aarch64_byval_register_lane_move(
    const prepare::PreparedMoveResolution& move);
[[nodiscard]] std::optional<std::size_t> byval_register_lane_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index);
[[nodiscard]] std::vector<AggregateRegisterLaneStore> collect_byval_register_lane_stores(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> aggregate_lane_store_memory(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const AggregateRegisterLaneStore& store,
    std::size_t byte_delta,
    std::size_t width_bytes,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> aarch64_indirect_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> aarch64_stack_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> aarch64_register_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] bool aarch64_indirect_register_byval_argument(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);

}  // namespace c4c::backend::aarch64::codegen
