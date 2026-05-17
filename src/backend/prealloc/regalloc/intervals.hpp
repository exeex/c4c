#pragma once

#include "../regalloc.hpp"

#include <cstddef>
#include <optional>

namespace c4c::backend::prepare {

namespace regalloc_detail {

struct ProgramPointLocation {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

[[nodiscard]] bool intervals_overlap(const PreparedLiveInterval& lhs,
                                     const PreparedLiveInterval& rhs);

[[nodiscard]] std::size_t value_priority(const PreparedLivenessValue& value);

[[nodiscard]] std::optional<ProgramPointLocation> locate_program_point(
    const PreparedLivenessFunction& function,
    std::size_t point);

[[nodiscard]] std::size_t weighted_use_score(const PreparedLivenessFunction& function,
                                             const PreparedLivenessValue& value);

[[nodiscard]] std::size_t interval_start_sort_key(const PreparedRegallocValue& value);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
