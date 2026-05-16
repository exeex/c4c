#pragma once

#include "../module/module.hpp"

#include <optional>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_unconditional_branch_record(
    c4c::FunctionNameId function_name,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator);
[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition,
    const bir::Terminator& terminator);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_prepared_branch_terminator(const module::BlockLoweringContext& context,
                                 module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_prepared_conditional_branch_terminator(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] c4c::backend::mir::MachineBlockSuccessor
make_unconditional_branch_successor(const module::BlockLoweringContext& context);
[[nodiscard]] std::vector<c4c::backend::mir::MachineBlockSuccessor>
make_conditional_branch_successors(const module::BlockLoweringContext& context);

}  // namespace c4c::backend::aarch64::codegen
