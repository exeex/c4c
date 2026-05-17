#pragma once

#include "../regalloc.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;

namespace regalloc_detail {

[[nodiscard]] PreparedRegisterClass classify_register_class(const PreparedLivenessValue& value);

[[nodiscard]] PreparedRegisterClass resolve_register_class(const PreparedBirModule& prepared,
                                                           const PreparedLivenessValue& value);

[[nodiscard]] std::size_t resolve_register_group_width(const PreparedBirModule& prepared,
                                                       const PreparedLivenessValue& value);

[[nodiscard]] PreparedRegisterBank register_bank_from_class(PreparedRegisterClass reg_class);

[[nodiscard]] std::vector<std::string> materialize_register_names(
    const std::vector<std::string_view>& register_names);

[[nodiscard]] std::vector<PreparedRegisterPlacement> materialize_register_placements(
    const std::vector<PreparedRegisterCandidateSpan>& spans);

[[nodiscard]] std::size_t published_register_group_width(const PreparedRegallocValue& value);

[[nodiscard]] std::optional<PreparedRegisterPlacement> assigned_register_placement(
    const PreparedRegallocValue& value);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
