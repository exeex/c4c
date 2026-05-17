#pragma once

#include "../regalloc.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

void append_spill_reload_ops(const PreparedLivenessFunction& liveness_function,
                             const std::vector<std::optional<std::size_t>>& spill_points,
                             PreparedRegallocFunction& regalloc_function);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
