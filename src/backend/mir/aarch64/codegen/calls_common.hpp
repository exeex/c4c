#pragma once

#include "calls.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment);
[[nodiscard]] std::size_t incoming_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi);
[[nodiscard]] std::size_t incoming_stack_argument_alignment_bytes(
    const bir::CallArgAbiInfo& abi);
[[nodiscard]] std::size_t outgoing_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi);
[[nodiscard]] std::size_t outgoing_stack_argument_bytes(
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan);
[[nodiscard]] abi::RegisterReference outgoing_stack_argument_base_register();
[[nodiscard]] bool entry_param_uses_incoming_stack(const bir::Param& param);
[[nodiscard]] std::size_t named_incoming_stack_bytes(const bir::Function& function,
                                                     std::size_t named_parameter_count);
[[nodiscard]] bool function_has_call(const bir::Function& function);
[[nodiscard]] std::optional<std::size_t> fixed_frame_adjustment_bytes(
    const module::FunctionLoweringContext& context);
[[nodiscard]] std::optional<std::size_t> va_start_overflow_area_stack_offset(
    const module::BlockLoweringContext& context,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper);
[[nodiscard]] prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank);
[[nodiscard]] std::string_view register_display_name(abi::RegisterReference reg);
[[nodiscard]] std::vector<std::string_view> occupied_register_views(
    abi::RegisterReference reg);
[[nodiscard]] std::vector<std::string_view> occupied_register_views(
    const std::vector<abi::RegisterReference>& regs);
[[nodiscard]] std::optional<abi::RegisterView> prepared_clobber_expected_view(
    prepare::PreparedRegisterBank bank);
void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message);

}  // namespace c4c::backend::aarch64::codegen
