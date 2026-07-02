#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_lookups.hpp"
#include "../../../prealloc/stack_layout/stack_layout.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

[[nodiscard]] std::optional<std::uint32_t>
gpr_register_number_for_prior_preserved_selection(
    const c4c::backend::prepare::PreparedCallArgumentSourceSelection& selection);

[[nodiscard]] const c4c::backend::prepare::PreparedVariadicVaListField*
rv64_variadic_va_list_overflow_arg_area_field(
    const c4c::backend::prepare::PreparedVariadicEntryPlanFunction& entry_plan);

[[nodiscard]] bool rv64_variadic_helper_free_entry_contract_is_complete(
    const c4c::backend::prepare::PreparedVariadicEntryPlanFunction& entry_plan);

[[nodiscard]] bool prepared_move_is_before_return_stack_to_register_abi_move(
    const c4c::backend::prepare::PreparedMoveBundle& move_bundle,
    const c4c::backend::prepare::PreparedMoveResolution& move);

[[nodiscard]] std::optional<std::int32_t>
prepared_frame_slot_call_argument_offset(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedCallArgumentPlan& argument,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<std::int32_t>
stack_slot_offset_for_prior_preserved_gpr_selection(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedCallArgumentSourceSelection& selection,
    c4c::backend::bir::TypeKind argument_type,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_call(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    c4c::FunctionNameId function_name,
    const c4c::backend::bir::CallInst& call,
    std::size_t block_index,
    const PreparedCurrentInstructionContext& context);

}  // namespace c4c::backend::riscv::codegen
