#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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
    const prepare::PreparedBranchCondition& branch_condition);

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

struct DispatchBranchFusionHooks {
  std::optional<abi::RegisterView> (*scalar_view_for_type)(bir::TypeKind type);
  bool (*emit_value_publication_to_register)(
      const module::BlockLoweringContext& context,
      const bir::Value& value,
      std::size_t before_instruction_index,
      std::uint8_t target_index,
      std::uint8_t scratch_index,
      std::vector<std::string>& lines,
      bool reload_current_memory_loads);
  std::optional<RegisterOperand> (*current_block_entry_publication_register)(
      const module::BlockLoweringContext& context,
      const bir::Value& value,
      abi::RegisterView expected_view);
  const bir::Inst* (*find_same_block_named_producer)(
      const module::BlockLoweringContext& context,
      std::string_view value_name,
      std::size_t before_instruction_index);
  std::optional<std::size_t> (*producer_instruction_index)(
      const module::BlockLoweringContext& context,
      const bir::Inst* producer);
  const prepare::PreparedValueHome* (*prepared_value_home_for_value)(
      const module::BlockLoweringContext& context,
      const bir::Value& value);
  bool (*value_has_current_block_entry_publication)(
      const module::BlockLoweringContext& context,
      const prepare::PreparedValueHome& home);
  bool (*emit_prepared_value_home_to_register)(
      const prepare::PreparedStackLayout* stack_layout,
      const prepare::PreparedValueHome& home,
      bir::TypeKind type,
      std::uint8_t target_index,
      std::vector<std::string>& lines,
      bool use_frame_pointer_base);
  bool (*fixed_slots_use_frame_pointer)(const module::FunctionLoweringContext& context);
};

[[nodiscard]] std::optional<std::string_view> branch_condition_suffix(
    bir::BinaryOpcode predicate);
[[nodiscard]] bool is_cmp_immediate_encodable(std::int64_t value);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_fused_compare_branch_from_emitted_cast(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_materialized_compare_condition_branch(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_current_block_entry_fused_compare_branch(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_constant_rhs_fused_compare_branch(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_conditional_branch_from_emitted_condition(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] bool is_fused_compare_branch_support_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_home_fused_compare_branch(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks);
[[nodiscard]] bool fused_compare_uses_selected_operand(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks);

}  // namespace c4c::backend::aarch64::codegen
