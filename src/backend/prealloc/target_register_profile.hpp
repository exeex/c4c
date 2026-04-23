#pragma once

#include "prealloc.hpp"

#include "../../target_profile.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

[[nodiscard]] std::optional<std::string> call_arg_destination_register_name(
    const c4c::TargetProfile& target_profile,
    const bir::CallArgAbiInfo& abi,
    std::size_t arg_index);

[[nodiscard]] std::optional<std::string> call_result_destination_register_name(
    const c4c::TargetProfile& target_profile,
    const bir::CallResultAbiInfo& abi);

[[nodiscard]] std::vector<PreparedRegisterCandidateSpan> caller_saved_register_spans(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width = 1);

[[nodiscard]] std::vector<PreparedRegisterCandidateSpan> callee_saved_register_spans(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t contiguous_width = 1);

[[nodiscard]] std::vector<std::string_view> caller_saved_registers(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class);

[[nodiscard]] std::vector<std::string_view> callee_saved_registers(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class);

}  // namespace c4c::backend::prepare
