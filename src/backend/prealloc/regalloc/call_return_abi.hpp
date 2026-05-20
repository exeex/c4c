#pragma once

#include "../regalloc.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

namespace regalloc_detail {

[[nodiscard]] std::vector<std::string> call_arg_destination_register_names(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::size_t arg_index,
    std::string_view register_name,
    std::size_t contiguous_width);

[[nodiscard]] std::optional<PreparedRegisterPlacement> f128_call_arg_destination_placement(
    std::optional<PreparedRegisterPlacement> placement,
    bir::TypeKind arg_type);

[[nodiscard]] std::vector<std::string> call_result_destination_register_names(
    const c4c::TargetProfile& target_profile,
    PreparedRegisterClass reg_class,
    std::string_view register_name,
    std::size_t contiguous_width);

[[nodiscard]] std::optional<bir::CallArgAbiInfo> resolve_call_arg_abi(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index);

[[nodiscard]] std::optional<std::size_t> call_arg_abi_register_index(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index);

[[nodiscard]] PreparedMoveStorageKind call_arg_storage_kind(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index);

[[nodiscard]] std::optional<std::size_t> call_arg_destination_stack_offset_bytes(
    const c4c::TargetProfile& target_profile,
    const bir::CallInst& call,
    std::size_t arg_index);

[[nodiscard]] PreparedMoveStorageKind call_result_storage_kind(const bir::CallInst& call);

[[nodiscard]] std::optional<bir::CallResultAbiInfo> infer_scalar_function_return_abi(
    const bir::Function& function);

[[nodiscard]] PreparedMoveStorageKind function_return_storage_kind(const bir::Function& function);

}  // namespace regalloc_detail

}  // namespace c4c::backend::prepare
