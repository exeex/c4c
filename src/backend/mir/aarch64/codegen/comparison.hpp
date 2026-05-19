#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct ComparisonBranchPrintSpelling {
  std::string_view condition_mnemonic;
  std::string_view branch_mnemonic;
};

struct I128RelationalComparePrintSpelling {
  std::string_view high_true_condition;
  std::string_view high_false_condition;
  std::string_view low_true_condition;
};

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
    module::ModuleLoweringDiagnostics& diagnostics,
    const BlockScalarLoweringState* scalar_state = nullptr);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_prepared_i128_compare_instruction(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] c4c::backend::mir::MachineBlockSuccessor
make_unconditional_branch_successor(const module::BlockLoweringContext& context);
[[nodiscard]] std::vector<c4c::backend::mir::MachineBlockSuccessor>
make_conditional_branch_successors(const module::BlockLoweringContext& context);

[[nodiscard]] std::string_view comparison_unconditional_branch_mnemonic(
    const InstructionRecord& instruction);
[[nodiscard]] std::optional<ComparisonBranchPrintSpelling>
comparison_materialized_bool_branch_spelling(const InstructionRecord& instruction);
[[nodiscard]] std::optional<std::string_view> f128_compare_result_condition(
    prepare::PreparedF128CmpResultZeroTest zero_test);
[[nodiscard]] std::optional<std::string_view> i128_equality_compare_condition(
    bir::BinaryOpcode predicate);
[[nodiscard]] bool is_i128_relational_compare_predicate(bir::BinaryOpcode predicate);
[[nodiscard]] std::optional<I128RelationalComparePrintSpelling>
i128_relational_compare_spelling(bir::BinaryOpcode predicate);

}  // namespace c4c::backend::aarch64::codegen
