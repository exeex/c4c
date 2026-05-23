#pragma once

#include "alu.hpp"
#include "../abi/abi.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

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
