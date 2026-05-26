#include "dispatch.hpp"

#include "dispatch_edge_copies.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "calls_dispatch_bridge.hpp"
#include "calls.hpp"
#include "comparison_branch_fusion.hpp"
#include "constant_materialization.hpp"
#include "dispatch_diagnostics.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_publication_common.hpp"
#include "dispatch_value_materialization.hpp"
#include "memory_dynamic_stack.hpp"
#include "variadic.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

struct PreparedFrameSlotCallArgumentMove {
  const prepare::PreparedMoveBundle* bundle = nullptr;
  const prepare::PreparedMoveResolution* move = nullptr;
  const prepare::PreparedAbiBinding* binding = nullptr;
};

enum class LocalAggregateAddressMaterializationResult {
  NotRequired,
  Materialized,
  Failed,
};

[[nodiscard]] module::MachineInstruction make_dispatch_bridge_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::vector<std::string> materialize_local_aggregate_address_lines(
    abi::RegisterReference address_register,
    const module::BlockLoweringContext& context,
    std::int64_t byte_offset) {
  if (byte_offset < 0) {
    return {};
  }
  const auto offset = static_cast<std::uint64_t>(byte_offset);
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  const std::string address_name = std::string{abi::register_name(address_register)};
  if (offset <= 4095U) {
    return {"add " + address_name + ", " + std::string{base} + ", #" +
            std::to_string(offset)};
  }
  auto lines = materialize_integer_constant_lines(address_register, offset, 64);
  if (lines.empty()) {
    return {};
  }
  lines.push_back("add " + address_name + ", " + std::string{base} + ", " +
                  address_name);
  return lines;
}

[[nodiscard]] LocalAggregateAddressMaterializationResult
materialize_local_aggregate_address_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t before_instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    BlockScalarLoweringState& scalar_state) {
  if (!argument.allows_local_aggregate_address_publication) {
    return LocalAggregateAddressMaterializationResult::NotRequired;
  }
  const auto diagnostic = [&](std::string message) {
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        before_instruction_index,
        std::move(message));
    return LocalAggregateAddressMaterializationResult::Failed;
  };
  if (value.type != bir::TypeKind::Ptr) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a pointer argument");
  }
  auto result_register = make_named_prepared_result_register(context, value);
  if (!result_register.has_value() ||
      !abi::is_gp_register(result_register->reg)) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a prepared GPR result register");
  }
  if (!argument.source_selection.has_value() ||
      argument.source_selection->kind !=
          prepare::PreparedCallArgumentSourceSelectionKind::
              LocalFrameAddressMaterialization) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires prepared LocalFrameAddressMaterialization source selection");
  }
  const auto* source_home =
      argument.source_value_id.has_value()
          ? find_value_home(context, *argument.source_value_id)
          : find_value_home(context, result_register->value_name);
  const auto source = make_selected_call_argument_source(
      context,
      argument,
      source_home,
      *argument.source_selection,
      prepare::PreparedCallArgumentSourceSelectionKind::
          LocalFrameAddressMaterialization,
      before_instruction_index);
  if (!source.has_value()) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires complete prepared LocalFrameAddressMaterialization source selection");
  }
  const auto address_register = abi::x_register(result_register->reg.index);
  result_register->reg = address_register;
  result_register->expected_view = abi::RegisterView::X;
  auto lines = materialize_local_aggregate_address_lines(
      address_register, context, source->byte_offset);
  if (lines.empty()) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a printable prepared frame address");
  }
  auto instruction =
      make_select_chain_materialization_instruction(
          context, before_instruction_index, std::move(lines));
  if (!instruction.has_value()) {
    return diagnostic(
        "AArch64 local aggregate address call argument requires a materializable prepared frame address");
  }
  record_emitted_scalar_register(scalar_state,
                                 result_register->value_name,
                                 *result_register);
  lowered.push_back(std::move(*instruction));
  return LocalAggregateAddressMaterializationResult::Materialized;
}

[[nodiscard]] const prepare::PreparedCallArgumentPlan* find_prepared_call_argument_plan(
    const prepare::PreparedCallPlan* call_plan,
    std::size_t argument_index) {
  if (call_plan == nullptr) {
    return nullptr;
  }
  for (const auto& argument : call_plan->arguments) {
    if (argument.arg_index == argument_index) {
      return &argument;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<PreparedFrameSlotCallArgumentMove>
find_prepared_frame_slot_call_argument_move(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    std::size_t instruction_index) {
  const auto* bundle = find_move_bundle(context,
                                        prepare::PreparedMovePhase::BeforeCall,
                                        context.block_index,
                                        instruction_index);
  if (bundle == nullptr) {
    return std::nullopt;
  }
  for (const auto& move : bundle->moves) {
    const auto classification =
        prepare::classify_prepared_call_boundary_move(call_plan, *bundle, move);
    if (!prepare::prepared_call_boundary_move_classification_available(classification) ||
        classification.argument_plan != &argument ||
        classification.destination_kind !=
            prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        classification.storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        argument.source_value_id !=
            std::optional<prepare::PreparedValueId>{move.from_value_id}) {
      continue;
    }
    return PreparedFrameSlotCallArgumentMove{
        .bundle = bundle,
        .move = &move,
        .binding = classification.abi_binding,
    };
  }
  return std::nullopt;
}

[[nodiscard]] bool materialize_scalar_call_argument_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    const prepare::PreparedCallArgumentPlan* local_aggregate_address_argument,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    std::vector<std::string_view>& active_values) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return true;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  if (local_aggregate_address_argument != nullptr) {
    const auto local_address =
        materialize_local_aggregate_address_call_argument(
            context,
            value,
            *local_aggregate_address_argument,
            before_instruction_index,
            diagnostics,
            lowered,
            scalar_state);
    switch (local_address) {
      case LocalAggregateAddressMaterializationResult::Materialized:
        return true;
      case LocalAggregateAddressMaterializationResult::Failed:
        return false;
      case LocalAggregateAddressMaterializationResult::NotRequired:
        break;
    }
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  const auto producer_index =
      find_same_block_scalar_producer(context, value.name, before_instruction_index);
  if (!producer_index.has_value() || context.bir_block == nullptr) {
    if (has_same_block_load_local_producer(context, value, before_instruction_index)) {
      return true;
    }
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(&context.bir_block->insts[*producer_index]);
  if (binary == nullptr ||
      !is_scalar_call_argument_producer_opcode(binary->opcode)) {
    return true;
  }

  active_values.push_back(value.name);
  const bool lhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->lhs,
                                             *producer_index,
                                             nullptr,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  const bool rhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->rhs,
                                             *producer_index,
                                             nullptr,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  active_values.pop_back();
  if (!lhs_ready || !rhs_ready) {
    return false;
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  if (auto instruction = lower_scalar_instruction(context,
                                                  context.bir_block->insts[*producer_index],
                                                  *producer_index,
                                                  scalar_state,
                                                  diagnostics)) {
    const auto expected_result =
        make_named_prepared_result_register(context, binary->result);
    if (expected_result.has_value()) {
      if (auto* scalar =
              std::get_if<ScalarInstructionRecord>(&instruction->target.payload)) {
        scalar->result_register = *expected_result;
        if (scalar->scalar_alu.has_value()) {
          scalar->scalar_alu->result_register = *expected_result;
          if (const auto lhs_name = prepared_named_value_id(context, binary->lhs);
              lhs_name.has_value()) {
            if (const auto lhs_register =
                find_emitted_scalar_register(scalar_state, *lhs_name);
                lhs_register.has_value()) {
              auto lhs_operand = make_register_operand(*lhs_register);
              scalar->scalar_alu->lhs = lhs_operand;
              if (!scalar->inputs.empty()) {
                scalar->inputs[0] = std::move(lhs_operand);
              }
            }
          }
          if (const auto rhs_name = prepared_named_value_id(context, binary->rhs);
              rhs_name.has_value()) {
            if (const auto rhs_register =
                    find_emitted_scalar_register(scalar_state, *rhs_name);
                rhs_register.has_value()) {
              auto rhs_operand = make_register_operand(*rhs_register);
              scalar->scalar_alu->rhs = rhs_operand;
              if (scalar->inputs.size() > 1) {
                scalar->inputs[1] = std::move(rhs_operand);
              }
            }
          }
        }
        record_emitted_scalar_register(scalar_state,
                                       expected_result->value_name,
                                       *expected_result);
      }
    }
    lowered.push_back(std::move(*instruction));
    return true;
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_scalar_call_argument_producers(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const std::vector<bir::Value>& arguments,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  for (std::size_t argument_index = 0; argument_index < arguments.size(); ++argument_index) {
    const auto& argument = arguments[argument_index];
    if (auto select_chain =
            materialize_direct_global_select_chain_call_argument(context,
                                                                 argument,
                                                                 instruction_index,
                                                                 scalar_state)) {
      lowered.push_back(std::move(*select_chain));
      continue;
    }
    std::vector<std::string_view> active_values;
    const auto* argument_plan =
        find_prepared_call_argument_plan(&call_plan, argument_index);
    const auto* local_aggregate_address_argument =
        argument_plan != nullptr &&
                argument_plan->allows_local_aggregate_address_publication
            ? argument_plan
            : nullptr;
    if (!materialize_scalar_call_argument_value(context,
                                                argument,
                                                instruction_index,
                                                local_aggregate_address_argument,
                                                scalar_state,
                                                diagnostics,
                                                lowered,
                                                active_values)) {
      return {};
    }
  }
  return lowered;
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto dynamic_stack = lower_dynamic_stack_helper_call(
          context, call_inst, instruction_index, diagnostics);
      dynamic_stack.has_value()) {
    return dynamic_stack;
  }

  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}











[[nodiscard]] std::optional<bir::Value> find_bir_value_for_prepared_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const auto result = instruction_result_value(context.bir_block->insts[index - 1]);
    if (!result.has_value()) {
      continue;
    }
    const auto prepared_name = prepared_named_value_id(context, *result);
    if (prepared_name == std::optional<c4c::ValueNameId>{value_name}) {
      return result;
    }
  }
  if (before_instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  if (const auto* call =
          std::get_if<bir::CallInst>(&context.bir_block->insts[before_instruction_index]);
      call != nullptr) {
    for (const auto& argument : call->args) {
      const auto prepared_name = prepared_named_value_id(context, argument);
      if (prepared_name == std::optional<c4c::ValueNameId>{value_name}) {
        return argument;
      }
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_call_boundary_source_to_destination(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      !move_record->source_memory.has_value() ||
      move_record->source_memory_materializes_address ||
      !move_record->destination_register.has_value() ||
      !move_record->source_memory->result_value_name.has_value() ||
      move_record->source_memory->base_kind != MemoryBaseKind::FrameSlot ||
      find_emitted_scalar_register(
          scalar_state, *move_record->source_memory->result_value_name).has_value() ||
      !abi::is_gp_register(move_record->destination_register->reg)) {
    return std::nullopt;
  }

  const auto source_value =
      find_bir_value_for_prepared_name(
          context, *move_record->source_memory->result_value_name, instruction_index);
  if (!source_value.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const std::uint8_t target_index = move_record->destination_register->reg.index;
  const std::uint8_t scratch_index =
      scratches.front().index == target_index && scratches.size() > 1
          ? scratches[1].index
          : scratches.front().index;
  if (scratch_index == target_index) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *source_value,
                                          instruction_index,
                                          target_index,
                                          scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted = *move_record->destination_register;
  emitted.value_id = move_record->source_memory->result_value_id;
  emitted.value_name = *move_record->source_memory->result_value_name;
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));

}

void retarget_call_boundary_source_to_emitted_scalar(
    module::MachineInstruction& instruction,
    const BlockScalarLoweringState& scalar_state) {
  auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr) {
    return;
  }
  std::optional<c4c::ValueNameId> source_value_name;
  std::optional<prepare::PreparedValueId> source_value_id;
  if (move_record->source_memory.has_value() &&
      !move_record->source_memory_materializes_address &&
      move_record->source_memory->result_value_name.has_value()) {
    source_value_name = *move_record->source_memory->result_value_name;
    source_value_id = move_record->source_memory->result_value_id;
  } else if (move_record->source_register.has_value() &&
             move_record->source_register->value_name != c4c::kInvalidValueName) {
    source_value_name = move_record->source_register->value_name;
    source_value_id = move_record->source_register->value_id;
  } else if (move_record->source_register.has_value()) {
    source_value_id = move_record->source_register->value_id;
  }
  if (!source_value_name.has_value() && !source_value_id.has_value()) {
    return;
  }
  auto emitted = source_value_name.has_value()
                     ? find_emitted_scalar_register(scalar_state, *source_value_name)
                     : std::nullopt;
  if (!emitted.has_value() && source_value_id.has_value()) {
    const bool floating_preserved_source =
        move_record->source_register.has_value() &&
        move_record->source_register->reg.bank == abi::RegisterBank::FpSimd;
    if (floating_preserved_source) {
      for (const auto& [_, candidate] : scalar_state.emitted_registers) {
        if (candidate.value_id == source_value_id &&
            candidate.reg.bank == abi::RegisterBank::FpSimd) {
          emitted = candidate;
          break;
        }
      }
    }
  }
  if (!emitted.has_value()) {
    return;
  }
  move_record->source_register = *emitted;
  move_record->source_memory.reset();
  if (move_record->destination_register.has_value() &&
      emitted->reg.bank == abi::RegisterBank::GeneralPurpose &&
      move_record->destination_register->reg.bank == abi::RegisterBank::GeneralPurpose &&
      emitted->expected_view.has_value()) {
    const auto retargeted_destination =
        abi::gp_register(move_record->destination_register->reg.index,
                         *emitted->expected_view);
    if (retargeted_destination.has_value()) {
      move_record->destination_register->reg = *retargeted_destination;
      move_record->destination_register->expected_view = emitted->expected_view;
    }
  }
}

void record_call_boundary_destination(
    const module::MachineInstruction& instruction,
    BlockScalarLoweringState& scalar_state) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record != nullptr && move_record->destination_register.has_value()) {
    record_emitted_scalar_register(scalar_state,
                                   move_record->destination_register->value_name,
                                   *move_record->destination_register);
  }
}

void record_call_boundary_source_in_destination(
    const module::MachineInstruction& instruction,
    BlockScalarLoweringState& scalar_state) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr || !move_record->destination_register.has_value()) {
    return;
  }
  std::optional<prepare::PreparedValueId> source_value_id;
  c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
  if (move_record->source_memory.has_value() &&
      move_record->source_memory->result_value_name.has_value()) {
    source_value_id = move_record->source_memory->result_value_id;
    source_value_name = *move_record->source_memory->result_value_name;
  } else if (move_record->source_register.has_value() &&
             move_record->source_register->value_name != c4c::kInvalidValueName) {
    source_value_id = move_record->source_register->value_id;
    source_value_name = move_record->source_register->value_name;
  }
  if (source_value_name == c4c::kInvalidValueName) {
    return;
  }
  auto source_alias = *move_record->destination_register;
  source_alias.value_id = source_value_id;
  source_alias.value_name = source_value_name;
  record_emitted_scalar_register(scalar_state, source_value_name, source_alias);
}

[[nodiscard]] bool call_boundary_move_reloads_prepared_stack_source(
    const module::MachineInstruction& instruction) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  return move_record != nullptr &&
         move_record->phase == prepare::PreparedMovePhase::BeforeInstruction &&
         move_record->source_memory.has_value() &&
         move_record->source_memory->support == MemoryOperandSupportKind::Prepared &&
         move_record->source_memory->base_kind == MemoryBaseKind::FrameSlot &&
         move_record->source_memory->byte_offset_is_prepared_snapshot;
}

[[nodiscard]] bool source_value_is_materialized_address(
    const CallBoundaryMoveInstructionRecord& move_record,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  for (const auto& materialized : materialized_addresses) {
    const auto* address_record =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address_record == nullptr) {
      continue;
    }
    if (address_record->result_value_id.has_value() &&
        *address_record->result_value_id == move_record.move.from_value_id) {
      return true;
    }
    if (move_record.source_register.has_value() &&
        address_record->result_value_name != c4c::kInvalidValueName &&
        move_record.source_register->value_name == address_record->result_value_name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool source_register_conflicts_with_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      !move_record->source_register.has_value() ||
      source_value_is_materialized_address(*move_record, materialized_addresses)) {
    return false;
  }
  for (const auto& materialized : materialized_addresses) {
    const auto* address_record =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address_record == nullptr || !address_record->result_register.has_value()) {
      continue;
    }
    if (registers_alias(*move_record->source_register,
                        *address_record->result_register)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] c4c::SlotNameId local_load_effective_slot_id(
    const bir::LoadLocalInst& load) {
  if (load.address.has_value() &&
      load.address->base_slot_id != c4c::kInvalidSlotName) {
    return load.address->base_slot_id;
  }
  return load.slot_id;
}

[[nodiscard]] c4c::SlotNameId local_store_effective_slot_id(
    const bir::StoreLocalInst& store) {
  if (store.address.has_value() &&
      store.address->base_slot_id != c4c::kInvalidSlotName) {
    return store.address->base_slot_id;
  }
  return store.slot_id;
}

[[nodiscard]] std::int64_t local_load_effective_byte_offset(
    const bir::LoadLocalInst& load) {
  return load.address.has_value()
             ? load.address->byte_offset
             : static_cast<std::int64_t>(load.byte_offset);
}

[[nodiscard]] std::int64_t local_store_effective_byte_offset(
    const bir::StoreLocalInst& store) {
  return store.address.has_value()
             ? store.address->byte_offset
             : static_cast<std::int64_t>(store.byte_offset);
}

[[nodiscard]] bool same_local_scalar_slot_access(
    const bir::LoadLocalInst& load,
    const bir::StoreLocalInst& store) {
  const auto load_slot_id = local_load_effective_slot_id(load);
  const auto store_slot_id = local_store_effective_slot_id(store);
  if (load_slot_id != c4c::kInvalidSlotName ||
      store_slot_id != c4c::kInvalidSlotName) {
    if (load_slot_id == c4c::kInvalidSlotName ||
        store_slot_id == c4c::kInvalidSlotName ||
        load_slot_id != store_slot_id) {
      return false;
    }
  } else if (!load.slot_name.empty() || !store.slot_name.empty()) {
    if (load.slot_name.empty() || store.slot_name.empty() ||
        load.slot_name != store.slot_name) {
      return false;
    }
  }

  const auto load_size = dispatch_publication_scalar_type_size_bytes(load.result.type);
  const auto store_size = dispatch_publication_scalar_type_size_bytes(store.value.type);
  return load_size.has_value() &&
         store_size.has_value() &&
         *load_size == *store_size &&
         local_load_effective_byte_offset(load) ==
             local_store_effective_byte_offset(store);
}

struct LocalLoadStoredValue {
  bir::Value value;
  std::size_t store_instruction_index = 0;
};

[[nodiscard]] std::optional<LocalLoadStoredValue>
find_latest_same_block_local_load_store(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  const std::size_t limit =
      std::min(load_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto store_index = index - 1;
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[store_index]);
    if (store != nullptr && same_local_scalar_slot_access(load, *store)) {
      return LocalLoadStoredValue{
          .value = store->value,
          .store_instruction_index = store_index,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<LocalLoadStoredValue>
resolve_indirect_callee_local_load_store(
    const module::BlockLoweringContext& context,
    const bir::Value& callee_value,
    std::size_t call_instruction_index) {
  if (callee_value.kind != bir::Value::Kind::Named || callee_value.name.empty()) {
    return std::nullopt;
  }
  const auto* producer =
      find_same_block_named_producer(context, callee_value.name, call_instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer)
                                         : nullptr;
  const auto load_index = producer_instruction_index(context, producer);
  if (load == nullptr || !load_index.has_value()) {
    return std::nullopt;
  }
  auto stored =
      find_latest_same_block_local_load_store(context, *load, *load_index);
  if (!stored.has_value()) {
    return std::nullopt;
  }
  return stored;
}

[[nodiscard]] std::vector<std::uint8_t> indirect_callee_materialization_scratch_indices() {
  std::vector<std::uint8_t> indices;
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    indices.push_back(scratch.index);
  }
  for (const auto scratch : abi::indirect_call_scratch_registers()) {
    indices.push_back(scratch.index);
  }
  return indices;
}

[[nodiscard]] bool scratch_index_is_available(
    std::uint8_t candidate,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  return candidate != target_index &&
         std::find(occupied_indices.begin(), occupied_indices.end(), candidate) ==
             occupied_indices.end();
}

[[nodiscard]] std::optional<std::uint8_t> choose_scratch_index(
    const std::vector<std::uint8_t>& scratch_indices,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  for (const auto candidate : scratch_indices) {
    if (scratch_index_is_available(candidate, target_index, occupied_indices)) {
      return candidate;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool emit_indirect_callee_value_to_register_with_csel(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& scratch_indices,
    std::vector<std::uint8_t> occupied_indices,
    std::vector<std::string>& lines,
    unsigned depth = 0) {
  if (depth > 16U) {
    return false;
  }
  const auto* producer =
      value.kind == bir::Value::Kind::Named
          ? find_same_block_named_producer(context, value.name, before_instruction_index)
          : nullptr;
  const auto* select =
      producer != nullptr ? std::get_if<bir::SelectInst>(producer) : nullptr;
  if (select == nullptr) {
    const auto scratch_index =
        choose_scratch_index(scratch_indices, target_index, occupied_indices);
    return scratch_index.has_value() &&
           emit_value_publication_to_register(context,
                                              value,
                                              before_instruction_index,
                                              target_index,
                                              *scratch_index,
                                              lines,
                                              true);
  }

  const auto condition = branch_condition_suffix(select->predicate);
  const auto compare_view = scalar_view_for_type(select->compare_type);
  const auto result_view = scalar_view_for_type(value.type);
  if (!condition.has_value() || !compare_view.has_value() ||
      !result_view.has_value()) {
    return false;
  }

  const auto producer_index =
      producer_instruction_index(context, producer).value_or(before_instruction_index);
  const auto true_index =
      choose_scratch_index(scratch_indices, target_index, occupied_indices);
  if (!true_index.has_value()) {
    return false;
  }

  auto false_value = select->false_value;
  false_value.type = value.type;
  auto true_value = select->true_value;
  true_value.type = value.type;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        false_value,
                                                        producer_index,
                                                        target_index,
                                                        scratch_indices,
                                                        occupied_indices,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto true_occupied = occupied_indices;
  true_occupied.push_back(target_index);
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        true_value,
                                                        producer_index,
                                                        *true_index,
                                                        scratch_indices,
                                                        true_occupied,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto compare_occupied = occupied_indices;
  compare_occupied.push_back(target_index);
  compare_occupied.push_back(*true_index);
  const auto lhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!lhs_index.has_value()) {
    return false;
  }
  compare_occupied.push_back(*lhs_index);
  const auto rhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!rhs_index.has_value()) {
    return false;
  }

  auto lhs = select->lhs;
  lhs.type = select->compare_type;
  if (!emit_value_publication_to_register(context,
                                          lhs,
                                          producer_index,
                                          *lhs_index,
                                          *rhs_index,
                                          lines,
                                          true)) {
    return false;
  }
  std::string rhs_name;
  if (select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(select->rhs.immediate);
  } else {
    auto rhs = select->rhs;
    rhs.type = select->compare_type;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            producer_index,
                                            *rhs_index,
                                            *lhs_index,
                                            lines,
                                            true)) {
      return false;
    }
    const auto rhs_register = abi::gp_register(*rhs_index, *compare_view);
    if (!rhs_register.has_value()) {
      return false;
    }
    rhs_name = abi::register_name(*rhs_register);
  }

  const auto lhs_register = abi::gp_register(*lhs_index, *compare_view);
  const auto target_register = abi::gp_register(target_index, *result_view);
  const auto true_register = abi::gp_register(*true_index, *result_view);
  if (!lhs_register.has_value() || !target_register.has_value() ||
      !true_register.has_value()) {
    return false;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_register)} + ", " +
                  rhs_name);
  lines.push_back("csel " + std::string{abi::register_name(*target_register)} + ", " +
                  std::string{abi::register_name(*true_register)} + ", " +
                  std::string{abi::register_name(*target_register)} + ", " +
                  std::string{*condition});
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_indirect_call_callee_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  if (!call.is_indirect || !call.callee_value.has_value() ||
      !call_plan.is_indirect || !call_plan.indirect_callee.has_value()) {
    return std::nullopt;
  }
  const auto& callee = *call_plan.indirect_callee;
  if (callee.value_name == c4c::kInvalidValueName ||
      callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      callee.bank != prepare::PreparedRegisterBank::Gpr ||
      !callee.register_name.has_value()) {
    return std::nullopt;
  }

  bir::Value source_value = *call.callee_value;
  std::size_t source_before_index = instruction_index;
  if (const auto stored = resolve_indirect_callee_local_load_store(
          context, *call.callee_value, instruction_index);
      stored.has_value() &&
      select_chain_contains_direct_global_load(
          context, stored->value, stored->store_instruction_index)) {
    source_value = stored->value;
    source_before_index = stored->store_instruction_index;
  }

  const auto target_register = abi::parse_aarch64_register_name(*callee.register_name);
  if (!target_register.has_value() ||
      target_register->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto result_register = abi::gp_register(target_register->index, abi::RegisterView::X);
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = indirect_callee_materialization_scratch_indices();
  std::vector<std::string> lines;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        source_value,
                                                        source_before_index,
                                                        result_register->index,
                                                        scratches,
                                                        {},
                                                        lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::CallAbi,
      .value_id = callee.value_id,
      .value_name = callee.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_register_references = {*result_register},
      .occupied_registers = {abi::register_name(*result_register)},
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));

}

void record_call_result_source_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.value_locations != nullptr) {
    const auto* bundle = prepare::find_prepared_move_bundle(
        *context.function.value_locations,
        prepare::PreparedMovePhase::AfterCall,
        call_plan.block_index,
        call_plan.instruction_index);
    if (bundle != nullptr) {
      for (const auto& move : bundle->moves) {
        if (move.destination_kind !=
                prepare::PreparedMoveDestinationKind::CallResultAbi ||
            move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
            move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
          continue;
        }
        const auto* home =
            find_value_home(context,
move.to_value_id);
        if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
          continue;
        }
        const auto expected_view =
            move.destination_register_name.has_value()
                ? abi::parse_aarch64_register_name(*move.destination_register_name)
                : std::nullopt;
        if (!expected_view.has_value() ||
            expected_view->bank != abi::RegisterBank::FpSimd) {
          continue;
        }
        abi::PreparedRegisterConversionResult converted;
        if (move.destination_register_placement.has_value()) {
          converted = abi::convert_prepared_register(
              *move.destination_register_placement,
              prepare::PreparedRegisterClass::Float,
              expected_view->view);
        } else if (move.destination_register_name.has_value()) {
          converted = abi::convert_prepared_register(
              *move.destination_register_name,
              prepare::PreparedRegisterBank::Fpr,
              prepare::PreparedRegisterClass::Float,
              expected_view->view);
        } else {
          continue;
        }
        if (!converted.reg.has_value() ||
            converted.reg->bank != abi::RegisterBank::FpSimd) {
          continue;
        }
        record_emitted_scalar_register(
            scalar_state,
            home->value_name,
            RegisterOperand{
                .reg = *converted.reg,
                .role = RegisterOperandRole::CallAbi,
                .value_id = home->value_id,
                .value_name = home->value_name,
                .prepared_class = prepare::PreparedRegisterClass::Float,
                .prepared_bank = prepare::PreparedRegisterBank::Fpr,
                .expected_view = converted.reg->view,
                .contiguous_width = move.destination_contiguous_width,
                .occupied_register_references = {*converted.reg},
                .occupied_registers = {abi::register_name(*converted.reg)},
            });
      }
    }
  }

  if (!call_plan.result.has_value() ||
      !call_plan.result->destination_value_id.has_value() ||
      !call_plan.result->source_register_name.has_value() ||
      call_plan.result->source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      context.function.value_locations == nullptr) {
    return;
  }
  const auto* home =
      find_value_home(context,
*call_plan.result->destination_value_id);
  if (home == nullptr) {
    return;
  }
  const auto parsed = abi::parse_aarch64_register_name(
      *call_plan.result->source_register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return;
  }
  auto viewed = *parsed;
  if (call_plan.result->source_register_placement.has_value()) {
    if (const auto converted =
            abi::convert_prepared_register(*call_plan.result->source_register_placement,
                                           prepare::PreparedRegisterClass::General,
                                           parsed->view);
        converted.reg.has_value()) {
      viewed = *converted.reg;
    }
  }
  record_emitted_scalar_register(
      scalar_state,
      home->value_name,
      RegisterOperand{
          .reg = viewed,
          .role = RegisterOperandRole::CallAbi,
          .value_id = home->value_id,
          .value_name = home->value_name,
          .prepared_class = prepare::PreparedRegisterClass::General,
          .prepared_bank = prepare::PreparedRegisterBank::Gpr,
          .expected_view = viewed.view,
          .contiguous_width = call_plan.result->source_contiguous_width,
          .occupied_register_references = {viewed},
          .occupied_registers = {abi::register_name(viewed)},
      });
}

void retarget_fpr_call_result_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      (store->value.type != bir::TypeKind::F32 &&
       store->value.type != bir::TypeKind::F64)) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != store->value.type) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value() ||
      emitted->role != RegisterOperandRole::CallAbi ||
      emitted->prepared_bank != prepare::PreparedRegisterBank::Fpr ||
      emitted->reg.bank != abi::RegisterBank::FpSimd) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}

[[nodiscard]] std::vector<module::MachineInstruction>
materialize_missing_frame_slot_call_arguments(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  for (const auto& argument : call_plan.arguments) {
    if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
        !argument.source_value_id.has_value() ||
        argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        argument.destination_contiguous_width > 1) {
      continue;
    }
    const auto* home = find_value_home(context, *argument.source_value_id);
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        find_emitted_scalar_register(scalar_state, home->value_name).has_value()) {
      continue;
    }
    const auto prepared_move =
        find_prepared_frame_slot_call_argument_move(
            context, call_plan, argument, instruction_index);
    if (!prepared_move.has_value()) {
      continue;
    }
    std::optional<MemoryOperand> address_source;
    std::optional<MemoryOperand> source;
    const auto* source_selection = argument.source_selection.has_value()
                                       ? &*argument.source_selection
                                       : nullptr;
    if (source_selection != nullptr) {
      if (source_selection->kind ==
          prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress) {
        address_source = make_selected_call_argument_source(
            context,
            argument,
            home,
            *source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            instruction_index);
        source = address_source;
      } else if (source_selection->kind ==
                     prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue ||
                 source_selection->kind ==
                     prepare::PreparedCallArgumentSourceSelectionKind::PriorPreservation) {
        address_source = make_selected_call_argument_source(
            context,
            argument,
            home,
            *source_selection,
            prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotAddress,
            instruction_index);
        source =
            address_source.has_value()
                ? address_source
                : make_selected_call_argument_source(
                      context,
                      argument,
                      home,
                      *source_selection,
                      prepare::PreparedCallArgumentSourceSelectionKind::FrameSlotValue,
                      instruction_index);
      }
    }
    if (!source.has_value()) {
      continue;
    }
    const auto expected_view =
        address_source.has_value()
            ? std::optional<abi::RegisterView>{abi::RegisterView::X}
            : scalar_integer_register_view_from_size(source->size_bytes);
    if (!expected_view.has_value()) {
      continue;
    }
    const auto& move = *prepared_move->move;
    const auto* binding = prepared_move->binding;
    const auto destination_register_placement =
        binding != nullptr && binding->destination_register_placement.has_value()
            ? binding->destination_register_placement
            : (move.destination_register_placement.has_value()
                   ? move.destination_register_placement
                   : argument.destination_register_placement);
    const auto destination_register_name =
        destination_register_placement.has_value()
            ? std::optional<std::string>{}
            : (binding != nullptr && binding->destination_register_name.has_value()
                   ? binding->destination_register_name
                   : move.destination_register_name);
    const auto prepared_class = prepare::PreparedRegisterClass::General;
    abi::PreparedRegisterConversionResult converted;
    if (destination_register_placement.has_value()) {
      converted =
          abi::convert_prepared_register(*destination_register_placement,
                                         prepared_class,
                                         expected_view);
    } else if (destination_register_name.has_value()) {
      converted =
          abi::convert_prepared_register(*destination_register_name,
                                         argument.destination_register_bank,
                                         prepared_class,
                                         expected_view);
    } else {
      continue;
    }
    if (!converted.reg.has_value()) {
      continue;
    }
    RegisterOperand destination{
        .reg = *converted.reg,
        .role = RegisterOperandRole::CallAbi,
        .value_id = move.to_value_id != 0
                        ? std::optional<prepare::PreparedValueId>{move.to_value_id}
                        : argument.source_value_id,
        .value_name = home->value_name,
        .prepared_class = prepared_class,
        .prepared_bank = argument.destination_register_bank.value_or(
            prepare::PreparedRegisterBank::None),
        .expected_view = *expected_view,
        .contiguous_width = binding != nullptr ? binding->destination_contiguous_width
                                               : move.destination_contiguous_width,
        .occupied_register_references = {*converted.reg},
        .occupied_registers =
            binding != nullptr && !binding->destination_occupied_register_names.empty()
                ? std::vector<std::string_view>(
                      binding->destination_occupied_register_names.begin(),
                      binding->destination_occupied_register_names.end())
                : (!move.destination_occupied_register_names.empty()
                       ? std::vector<std::string_view>(
                             move.destination_occupied_register_names.begin(),
                             move.destination_occupied_register_names.end())
                       : std::vector<std::string_view>{abi::register_name(*converted.reg)}),
    };
    CallBoundaryMoveInstructionRecord move_record{
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .phase = prepared_move->bundle->phase,
        .authority_kind = prepared_move->bundle->authority_kind,
        .block_index = prepared_move->bundle->block_index,
        .instruction_index = prepared_move->bundle->instruction_index,
        .source_parallel_copy_predecessor_label =
            prepared_move->bundle->source_parallel_copy_predecessor_label,
        .source_parallel_copy_successor_label =
            prepared_move->bundle->source_parallel_copy_successor_label,
        .move = move,
        .source_memory = *source,
        .source_memory_materializes_address = address_source.has_value(),
        .destination_register = destination,
        .source_bundle = prepared_move->bundle,
        .source_move = prepared_move->move,
    };
    record_emitted_scalar_register(scalar_state, destination.value_name, destination);
    lowered.push_back(make_dispatch_bridge_machine_instruction(
        context,
        instruction_index,
        make_call_boundary_move_instruction(std::move(move_record))));
  }
  return lowered;
}

[[nodiscard]] std::vector<module::MachineInstruction>
publish_stack_preserved_call_values(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr &&
                     context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared,
                       context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr || context.function.call_plan_lookups == nullptr) {
    return lowered;
  }
  const auto* first_stack_values =
      prepare::first_indexed_stack_preserved_values_for_call(
          *context.function.call_plan_lookups, *call_plans, call_plan);
  if (first_stack_values == nullptr) {
    return lowered;
  }
  for (const auto* preserved_ptr : *first_stack_values) {
    if (preserved_ptr == nullptr) {
      continue;
    }
    const auto& preserved = *preserved_ptr;
    if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
        preserved.value_name == c4c::kInvalidValueName ||
        !preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0) {
      continue;
    }
    const auto emitted =
        find_emitted_scalar_register(scalar_state, preserved.value_name);
    std::optional<abi::RegisterView> expected_view;
    if (*preserved.stack_size_bytes == 8) {
      expected_view = abi::RegisterView::X;
    } else if (*preserved.stack_size_bytes == 4 ||
               *preserved.stack_size_bytes == 2 ||
               *preserved.stack_size_bytes == 1) {
      expected_view = abi::RegisterView::W;
    } else {
      continue;
    }
    std::optional<abi::RegisterReference> source_register;
    if (emitted.has_value() && abi::is_gp_register(emitted->reg)) {
      source_register = emitted->reg;
    } else if (context.function.value_locations != nullptr) {
      const auto* home =
          find_value_home(context,
preserved.value_id);
      if (home != nullptr &&
          home->kind == prepare::PreparedValueHomeKind::Register &&
          home->register_name.has_value()) {
        const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
        if (parsed.has_value() &&
            parsed->bank == abi::RegisterBank::GeneralPurpose) {
          source_register = *parsed;
        }
      }
    }
    if (!source_register.has_value()) {
      continue;
    }
    const auto source_reg = abi::gp_register(source_register->index, *expected_view);
    if (!source_reg.has_value()) {
      continue;
    }

    std::vector<std::string> lines;
    lines.push_back("str " + std::string(abi::register_name(*source_reg)) + ", " +
                    frame_slot_address(context.function, *preserved.stack_offset_bytes));
    if (auto published =
            make_select_chain_materialization_instruction(
                context, instruction_index, std::move(lines))) {
      lowered.push_back(std::move(*published));
    }
  }
  return lowered;
}

}  // namespace c4c::backend::aarch64::codegen
