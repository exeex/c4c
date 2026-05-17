#pragma once

#include "../regalloc.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] std::string storage_transfer_reason(std::string_view prefix,
                                                  PreparedMoveStorageKind from_kind,
                                                  PreparedMoveStorageKind to_kind);

[[nodiscard]] std::string storage_transfer_reason(std::string_view prefix,
                                                  const PreparedRegallocValue& from,
                                                  const PreparedRegallocValue& to);

void append_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    const PreparedRegallocValue& source,
    const PreparedRegallocValue& destination,
    std::size_t block_index,
    std::size_t instruction_index,
    bool uses_cycle_temp_source,
    bool coalesced_by_assigned_storage,
    std::optional<std::size_t> source_parallel_copy_step_index,
    PreparedMoveResolutionOpKind op_kind,
    PreparedMoveAuthorityKind authority_kind,
    std::string reason,
    std::optional<BlockLabelId> source_parallel_copy_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> source_parallel_copy_successor_label = std::nullopt);

void append_move_resolution_record(
    PreparedRegallocFunction& regalloc_function,
    PreparedValueId from_value_id,
    PreparedValueId to_value_id,
    PreparedMoveDestinationKind destination_kind,
    PreparedMoveStorageKind from_kind,
    PreparedMoveStorageKind to_kind,
    std::optional<std::size_t> destination_abi_index,
    std::optional<std::string> destination_register_name,
    std::size_t destination_contiguous_width,
    std::vector<std::string> destination_occupied_register_names,
    std::optional<std::size_t> destination_stack_offset_bytes,
    std::size_t block_index,
    std::size_t instruction_index,
    bool uses_cycle_temp_source,
    bool coalesced_by_assigned_storage,
    std::optional<std::size_t> source_parallel_copy_step_index,
    PreparedMoveResolutionOpKind op_kind,
    PreparedMoveAuthorityKind authority_kind,
    std::string reason,
    std::optional<BlockLabelId> source_parallel_copy_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> source_parallel_copy_successor_label = std::nullopt,
    std::optional<PreparedRegisterPlacement> destination_register_placement = std::nullopt);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
