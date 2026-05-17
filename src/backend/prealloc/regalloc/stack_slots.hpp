#pragma once

#include "../regalloc.hpp"

#include <cstddef>
#include <optional>

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] std::size_t normalized_value_size(const PreparedRegallocValue& value);

[[nodiscard]] std::size_t normalized_value_alignment(const PreparedRegallocValue& value);

[[nodiscard]] std::optional<PreparedStackSlotAssignment> existing_stack_slot_assignment(
    const PreparedStackLayout& stack_layout,
    const PreparedRegallocValue& value);

[[nodiscard]] PreparedStackSlotAssignment allocate_stack_slot(const PreparedRegallocValue& value,
                                                              const PreparedStackLayout& stack_layout,
                                                              PreparedFrameSlotId& next_slot_id,
                                                              std::size_t& next_offset_bytes,
                                                              std::size_t& frame_alignment_bytes);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
