#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"
#include "mir/printer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

// calls_common

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

// calls_printing effects

[[nodiscard]] std::vector<MachineEffectResource> effects_from_prepared_call_clobbers(
    const std::vector<prepare::PreparedClobberedRegister>& clobbers);

// calls_argument_sources

[[nodiscard]] const prepare::PreparedCallArgumentPlan* find_prepared_argument_plan(
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveResolution& move);
[[nodiscard]] const prepare::PreparedAbiBinding* find_prepared_argument_binding(
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move);
[[nodiscard]] const prepare::PreparedAbiBinding* find_prepared_result_binding(
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedMoveResolution& move);
[[nodiscard]] bool complete_full_width_f128_carrier(
    const prepare::PreparedF128Carrier* carrier);
[[nodiscard]] bool complete_f128_constant_carrier(
    const prepare::PreparedF128Carrier* carrier);
[[nodiscard]] const prepare::PreparedF128Carrier*
find_prepared_f128_carrier_in_module(const prepare::PreparedBirModule& prepared,
                                     prepare::PreparedValueId value_id);
[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_view_from_register_name(
    const std::optional<std::string>& register_name);
[[nodiscard]] std::optional<abi::RegisterView> scalar_view_from_register_name(
    const std::optional<std::string>& register_name);
[[nodiscard]] std::size_t scalar_size_from_register_view(
    std::optional<abi::RegisterView> view);
[[nodiscard]] std::optional<std::string> register_name_with_expected_view(
    const std::optional<std::string>& register_name,
    std::optional<abi::RegisterView> expected_view);
[[nodiscard]] std::optional<RegisterOperand> make_register_operand_from_prepared_authority(
    const std::optional<std::string>& register_name,
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    const std::optional<prepare::PreparedRegisterBank>& bank,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_registers,
    std::optional<abi::RegisterView> expected_view,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<RegisterOperand>
make_f128_q_register_operand_from_carrier(
    const prepare::PreparedF128Carrier& carrier,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<ImmediateOperand> make_scalar_call_argument_immediate(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id);
[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view(
    bir::TypeKind type);
[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view_from_size(
    std::size_t size_bytes);
[[nodiscard]] std::optional<bir::TypeKind> scalar_integer_type_from_size(
    std::size_t size_bytes);
[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot_by_id(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id);
[[nodiscard]] std::optional<MemoryOperand> make_sret_memory_return_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_frame_slot_call_argument_address_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_local_frame_address_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_stack_call_argument_destination(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedAbiBinding* binding,
    const MemoryOperand& source,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_aggregate_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const RegisterOperand& source_register,
    std::size_t size_bytes,
    std::int64_t byte_offset,
    std::size_t instruction_index);
[[nodiscard]] std::optional<RegisterOperand> make_indirect_callee_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedIndirectCalleePlan& callee,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<MemoryOperand> make_memory_return_storage(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryReturnPlan& memory_return,
    std::size_t instruction_index);

// calls_byval_aggregates

[[nodiscard]] std::string_view aggregate_stack_copy_load_mnemonic(
    std::size_t width_bytes);
[[nodiscard]] std::string_view aggregate_stack_copy_store_mnemonic(
    std::size_t width_bytes);
[[nodiscard]] std::optional<abi::RegisterReference> aggregate_stack_copy_scratch(
    std::size_t width_bytes);
[[nodiscard]] std::vector<std::size_t> aggregate_stack_copy_chunks(
    std::size_t size_bytes);
[[nodiscard]] std::string_view aggregate_register_lane_load_mnemonic(
    std::size_t width_bytes);
[[nodiscard]] abi::RegisterReference aggregate_register_lane_load_register(
    abi::RegisterReference reg,
    std::size_t width_bytes);
[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_scratch(
    const RegisterOperand& destination);
[[nodiscard]] MemoryOperand aggregate_register_lane_memory(MemoryOperand memory,
                                                          std::size_t byte_offset,
                                                          std::size_t width_bytes);
[[nodiscard]] bool aggregate_register_lane_memory_is_printable(
    const MemoryOperand& memory,
    std::size_t width_bytes);
[[nodiscard]] std::optional<std::size_t> aggregate_register_lane_printable_chunk(
    const MemoryOperand& memory,
    std::size_t source_offset,
    std::size_t remaining);
[[nodiscard]] std::optional<abi::RegisterReference> aggregate_register_lane_destination(
    const RegisterOperand& destination,
    std::size_t lane_index);
[[nodiscard]] bool is_aggregate_register_lane_publication(
    const CallBoundaryMoveInstructionRecord& move);

struct AggregateRegisterLaneStore {
  std::size_t source_offset = 0;
  std::int64_t stack_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 1;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
};

[[nodiscard]] bool is_aarch64_byval_register_lane_move(
    const prepare::PreparedMoveResolution& move);
[[nodiscard]] std::optional<std::size_t> byval_register_lane_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveResolution& move,
    std::size_t instruction_index);
[[nodiscard]] std::vector<AggregateRegisterLaneStore> collect_byval_register_lane_stores(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& source_home,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> make_byval_register_lane_prepared_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    std::size_t size_bytes,
    std::size_t instruction_index);
[[nodiscard]] std::optional<MemoryOperand> aggregate_lane_store_memory(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome& source_home,
    const AggregateRegisterLaneStore& store,
    std::size_t byte_delta,
    std::size_t width_bytes,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> aarch64_indirect_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> aarch64_stack_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] std::optional<std::size_t> aarch64_register_byval_argument_size_bytes(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);
[[nodiscard]] bool aarch64_indirect_register_byval_argument(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index);

// calls

class ScopedPreparedCallPreserveEffectPublication {
 public:
  explicit ScopedPreparedCallPreserveEffectPublication(bool enabled);
  ~ScopedPreparedCallPreserveEffectPublication();

  ScopedPreparedCallPreserveEffectPublication(
      const ScopedPreparedCallPreserveEffectPublication&) = delete;
  ScopedPreparedCallPreserveEffectPublication& operator=(
      const ScopedPreparedCallPreserveEffectPublication&) = delete;

 private:
  bool previous_enabled_ = true;
};

[[nodiscard]] const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] const prepare::PreparedCallPlan* require_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<module::MachineInstruction> lower_prepared_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper,
    module::ModuleLoweringDiagnostics& diagnostics);

// calls_moves

[[nodiscard]] std::vector<module::MachineInstruction> lower_before_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_after_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_before_return_moves(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_value_moves(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

// calls_preservation

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
[[nodiscard]] const prepare::PreparedMoveBundle* find_move_bundle(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);
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

// calls_printing

[[nodiscard]] InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_instruction(CallInstructionRecord instruction);
[[nodiscard]] mir::TargetInstructionPrintResult print_call(
    const InstructionRecord& instruction,
    const CallInstructionRecord& call);
[[nodiscard]] mir::TargetInstructionPrintResult print_call_boundary_move(
    const InstructionRecord& instruction,
    const CallBoundaryMoveInstructionRecord& move);
[[nodiscard]] mir::TargetInstructionPrintResult print_call_boundary_abi_binding(
    const InstructionRecord& instruction,
    const CallBoundaryAbiBindingInstructionRecord& binding);

}  // namespace c4c::backend::aarch64::codegen
