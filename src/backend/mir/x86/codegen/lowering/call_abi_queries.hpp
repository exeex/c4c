#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace c4c::backend::prepare {
struct PreparedMoveResolution;
struct PreparedValueHome;
struct PreparedValueLocationFunction;
}  // namespace c4c::backend::prepare

namespace c4c::backend::x86 {

struct PreparedI32CallResultAbiSelection {
  const c4c::backend::prepare::PreparedMoveResolution* move = nullptr;
  std::string abi_register;
};

struct PreparedCallResultAbiSelection {
  const c4c::backend::prepare::PreparedMoveResolution* move = nullptr;
  std::string abi_register;
};

// Compatibility holdout until the reviewed prepared call-bundle render seam
// lands in step 3. Keep these selectors live under `lowering/` ownership
// rather than on the transitional shared x86 surface.
std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::string> select_prepared_i32_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<std::string> select_prepared_i32_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index);

std::optional<PreparedCallResultAbiSelection> select_prepared_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

std::optional<PreparedCallResultAbiSelection> select_prepared_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

std::optional<PreparedI32CallResultAbiSelection> select_prepared_i32_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

std::optional<PreparedI32CallResultAbiSelection> select_prepared_i32_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home);

}  // namespace c4c::backend::x86
