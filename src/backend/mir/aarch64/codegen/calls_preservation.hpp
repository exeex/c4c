#pragma once

#include "calls.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

struct PreservedCallArgumentSource {
  const prepare::PreparedCallPreservedValue* preserved = nullptr;
  std::optional<RegisterOperand> source_register;
  std::optional<MemoryOperand> source_memory;
};

[[nodiscard]] std::optional<std::size_t> argument_source_prepared_block_index_by_label(
    const prepare::PreparedControlFlowFunction& function,
    c4c::BlockLabelId label);
[[nodiscard]] std::vector<std::size_t> argument_source_prepared_block_successors(
    const prepare::PreparedControlFlowFunction& function,
    const prepare::PreparedControlFlowBlock& block);
[[nodiscard]] bool prepared_block_dominates(
    const prepare::PreparedControlFlowFunction& function,
    std::size_t dominator_index,
    std::size_t block_index);
[[nodiscard]] std::size_t argument_source_move_bundle_position_key(
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);
[[nodiscard]] const prepare::PreparedMoveBundle* find_move_bundle(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);
[[nodiscard]] bool argument_source_prior_preserved_entry_position_less(
    const module::PriorPreservedValueEntry& lhs,
    const module::PriorPreservedValueEntry& rhs);
[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_latest_prior_preserved_value_by_position(
    const module::PreparedCallPlanIndexes& call_plan_indexes,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id);
[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_by_dominating_position(
    const module::PreparedCallPlanIndexes& call_plan_indexes,
    const prepare::PreparedControlFlowFunction* control_flow,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id);
[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_call_argument(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move);
[[nodiscard]] const prepare::PreparedCallPreservedValue* find_prior_preserved_value_for_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id);
[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_stack_preserved_value_before_instruction(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    std::size_t instruction_index);
[[nodiscard]] bool value_spelling_matches(const bir::Value& value,
                                          std::string_view spelling);
[[nodiscard]] bool non_call_instruction_uses_value(const bir::Inst& inst,
                                                   std::string_view spelling);
[[nodiscard]] bool terminator_uses_value(const bir::Terminator& terminator,
                                         std::string_view spelling);
[[nodiscard]] bool branch_condition_uses_value(
    const module::BlockLoweringContext& context,
    std::string_view spelling);
[[nodiscard]] bool preserved_value_has_later_non_call_use(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallPreservedValue& preserved);
[[nodiscard]] bool preserved_value_has_block_entry_non_call_use(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPreservedValue& preserved);
[[nodiscard]] std::optional<PreservedCallArgumentSource>
make_prior_preserved_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index,
    std::string reason,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_population(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen
