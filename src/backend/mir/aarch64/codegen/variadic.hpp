#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"
#include "mir/printer.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::optional<prepare::PreparedVariadicEntryHelperKind>
variadic_entry_helper_kind(std::string_view callee);

[[nodiscard]] const prepare::PreparedVariadicEntryPlanFunction*
require_prepared_variadic_entry_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

[[nodiscard]] bool variadic_helper_operand_homes_complete(
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes);

[[nodiscard]] std::string variadic_helper_missing_consumption_fact_message(
    prepare::PreparedVariadicEntryHelperKind helper);

void complete_variadic_call_record(CallInstructionRecord& instruction);

[[nodiscard]] std::optional<MachineNodeStatusRecord> variadic_call_selection_status(
    const CallInstructionRecord& instruction);

[[nodiscard]] std::optional<mir::TargetInstructionPrintResult> print_variadic_call(
    const CallInstructionRecord& call,
    std::string bad_header,
    std::string_view mnemonic);

}  // namespace c4c::backend::aarch64::codegen
