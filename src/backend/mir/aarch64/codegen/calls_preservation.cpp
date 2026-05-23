#include "calls.hpp"
#include "dispatch_lookup.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

[[nodiscard]] module::MachineInstruction make_preservation_machine_instruction(
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

}  // namespace

[[nodiscard]] std::optional<std::size_t> argument_source_prepared_block_index_by_label(
    const prepare::PreparedControlFlowFunction& function,
    c4c::BlockLabelId label) {
  for (std::size_t index = 0; index < function.blocks.size(); ++index) {
    if (function.blocks[index].block_label == label) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::size_t> argument_source_prepared_block_successors(
    const prepare::PreparedControlFlowFunction& function,
    const prepare::PreparedControlFlowBlock& block) {
  std::vector<std::size_t> successors;
  auto append_label = [&](c4c::BlockLabelId label) {
    if (label == c4c::kInvalidBlockLabel) {
      return;
    }
    const auto index = argument_source_prepared_block_index_by_label(function, label);
    if (index.has_value() &&
        std::find(successors.begin(), successors.end(), *index) ==
            successors.end()) {
      successors.push_back(*index);
    }
  };

  if (block.terminator_kind == bir::TerminatorKind::Branch) {
    append_label(block.branch_target_label);
  } else if (block.terminator_kind == bir::TerminatorKind::CondBranch) {
    append_label(block.true_label);
    append_label(block.false_label);
  }
  return successors;
}

[[nodiscard]] bool prepared_block_dominates(
    const prepare::PreparedControlFlowFunction& function,
    std::size_t dominator_index,
    std::size_t block_index) {
  const std::size_t count = function.blocks.size();
  if (dominator_index >= count || block_index >= count) {
    return false;
  }
  if (dominator_index == block_index) {
    return true;
  }
  std::vector<std::vector<std::size_t>> predecessors(count);
  for (std::size_t index = 0; index < count; ++index) {
    for (const auto successor :
         argument_source_prepared_block_successors(function, function.blocks[index])) {
      if (successor < count) {
        predecessors[successor].push_back(index);
      }
    }
  }

  std::vector<std::vector<bool>> dominates(
      count, std::vector<bool>(count, true));
  if (count != 0) {
    std::fill(dominates.front().begin(), dominates.front().end(), false);
    dominates.front().front() = true;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t index = 1; index < count; ++index) {
      std::vector<bool> next(count, !predecessors[index].empty());
      if (predecessors[index].empty()) {
        std::fill(next.begin(), next.end(), false);
      } else {
        for (const auto predecessor : predecessors[index]) {
          for (std::size_t candidate = 0; candidate < count; ++candidate) {
            next[candidate] = next[candidate] && dominates[predecessor][candidate];
          }
        }
      }
      next[index] = true;
      if (next != dominates[index]) {
        dominates[index] = std::move(next);
        changed = true;
      }
    }
  }
  return dominates[block_index][dominator_index];
}

[[nodiscard]] std::size_t argument_source_move_bundle_position_key(
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  return (static_cast<std::size_t>(phase) << 56U) ^
         (block_index << 32U) ^
         instruction_index;
}

[[nodiscard]] const prepare::PreparedMoveBundle* find_move_bundle(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (context.function.move_bundle_indexes != nullptr) {
    const auto it = context.function.move_bundle_indexes->bundles_by_position.find(
        argument_source_move_bundle_position_key(phase, block_index, instruction_index));
    if (it != context.function.move_bundle_indexes->bundles_by_position.end()) {
      return it->second;
    }
    return nullptr;
  }
  return context.function.value_locations == nullptr
             ? nullptr
             : prepare::find_prepared_move_bundle(*context.function.value_locations,
                                                  phase,
                                                  block_index,
                                                  instruction_index);
}

[[nodiscard]] bool argument_source_prior_preserved_entry_position_less(
    const module::PriorPreservedValueEntry& lhs,
    const module::PriorPreservedValueEntry& rhs) {
  if (lhs.block_index != rhs.block_index) {
    return lhs.block_index < rhs.block_index;
  }
  return lhs.instruction_index < rhs.instruction_index;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_latest_prior_preserved_value_by_position(
    const module::PreparedCallPlanIndexes& call_plan_indexes,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id) {
  if (value_id >= call_plan_indexes.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = call_plan_indexes.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const module::PriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(
      entries.begin(), entries.end(), current, argument_source_prior_preserved_entry_position_less);
  if (it == entries.begin()) {
    return nullptr;
  }
  --it;
  return it->preserved;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_by_dominating_position(
    const module::PreparedCallPlanIndexes& call_plan_indexes,
    const prepare::PreparedControlFlowFunction* control_flow,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id) {
  if (value_id >= call_plan_indexes.prior_preserved_by_value.size()) {
    return nullptr;
  }
  const auto& entries = call_plan_indexes.prior_preserved_by_value[value_id];
  if (entries.empty()) {
    return nullptr;
  }
  const module::PriorPreservedValueEntry current{
      .block_index = current_call_plan.block_index,
      .instruction_index = current_call_plan.instruction_index,
      .preserved = nullptr,
  };
  auto it = std::lower_bound(
      entries.begin(), entries.end(), current, argument_source_prior_preserved_entry_position_less);
  while (it != entries.begin()) {
    --it;
    if (it->block_index == current_call_plan.block_index) {
      if (it->instruction_index < current_call_plan.instruction_index) {
        return it->preserved;
      }
      continue;
    }
    if (control_flow != nullptr &&
        prepared_block_dominates(*control_flow, it->block_index, current_call_plan.block_index)) {
      return it->preserved;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_call_argument(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move) {
  const auto value_id = argument.source_value_id.value_or(move.from_value_id);
  if (context.function.call_plan_indexes != nullptr) {
    return find_latest_prior_preserved_value_by_position(
        *context.function.call_plan_indexes, current_call_plan, value_id);
  }
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return nullptr;
  }
  const prepare::PreparedCallPreservedValue* selected = nullptr;
  for (const auto& call : call_plans->calls) {
    if (call.block_index > current_call_plan.block_index ||
        (call.block_index == current_call_plan.block_index &&
         call.instruction_index >= current_call_plan.instruction_index)) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.value_id == value_id &&
          preserved.route != prepare::PreparedCallPreservationRoute::Unknown) {
        selected = &preserved;
      }
    }
  }
  return selected;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_value(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    prepare::PreparedValueId value_id) {
  if (context.function.call_plan_indexes != nullptr) {
    return find_prior_preserved_value_by_dominating_position(
        *context.function.call_plan_indexes,
        context.function.control_flow,
        current_call_plan,
        value_id);
  }
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return nullptr;
  }

  for (auto call_it = call_plans->calls.rbegin();
       call_it != call_plans->calls.rend();
       ++call_it) {
    const auto& call = *call_it;
    if (call.block_index == current_call_plan.block_index) {
      if (call.instruction_index >= current_call_plan.instruction_index) {
        continue;
      }
    } else if (context.function.control_flow == nullptr ||
               !prepared_block_dominates(*context.function.control_flow,
                                         call.block_index,
                                         current_call_plan.block_index)) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.value_id == value_id &&
          preserved.route != prepare::PreparedCallPreservationRoute::Unknown) {
        return &preserved;
      }
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_stack_preserved_value_before_instruction(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id,
    std::size_t instruction_index) {
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr && context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared, context.function.control_flow->function_name)
                 : nullptr);
  if (call_plans == nullptr) {
    return nullptr;
  }

  for (auto call_it = call_plans->calls.rbegin();
       call_it != call_plans->calls.rend();
       ++call_it) {
    const auto& call = *call_it;
    if (call.block_index != context.block_index ||
        call.instruction_index >= instruction_index) {
      continue;
    }
    for (const auto& preserved : call.preserved_values) {
      if (preserved.value_id == value_id &&
          preserved.route == prepare::PreparedCallPreservationRoute::StackSlot &&
          preserved.slot_id.has_value() &&
          preserved.stack_offset_bytes.has_value() &&
          preserved.stack_size_bytes.has_value() &&
          *preserved.stack_size_bytes != 0) {
        return &preserved;
      }
    }
  }
  return nullptr;
}

[[nodiscard]] bool value_spelling_matches(const bir::Value& value,
                                          std::string_view spelling) {
  return value.kind == bir::Value::Kind::Named && value.name == spelling;
}

[[nodiscard]] bool non_call_instruction_uses_value(const bir::Inst& inst,
                                                   std::string_view spelling) {
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst)) {
    return value_spelling_matches(binary->lhs, spelling) ||
           value_spelling_matches(binary->rhs, spelling);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(&inst)) {
    return value_spelling_matches(select->lhs, spelling) ||
           value_spelling_matches(select->rhs, spelling) ||
           value_spelling_matches(select->true_value, spelling) ||
           value_spelling_matches(select->false_value, spelling);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst)) {
    return value_spelling_matches(cast->operand, spelling);
  }
  if (const auto* store_global = std::get_if<bir::StoreGlobalInst>(&inst)) {
    return value_spelling_matches(store_global->value, spelling);
  }
  if (const auto* store_local = std::get_if<bir::StoreLocalInst>(&inst)) {
    return value_spelling_matches(store_local->value, spelling);
  }
  return false;
}

[[nodiscard]] bool terminator_uses_value(const bir::Terminator& terminator,
                                         std::string_view spelling) {
  if (terminator.value.has_value() &&
      value_spelling_matches(*terminator.value, spelling)) {
    return true;
  }
  for (const auto& lane : terminator.return_lanes) {
    if (value_spelling_matches(lane, spelling)) {
      return true;
    }
  }
  return terminator.kind == bir::TerminatorKind::CondBranch &&
         value_spelling_matches(terminator.condition, spelling);
}

[[nodiscard]] bool branch_condition_uses_value(
    const module::BlockLoweringContext& context,
    std::string_view spelling) {
  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    return false;
  }
  const auto* condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (condition == nullptr) {
    return false;
  }
  return value_spelling_matches(condition->condition_value, spelling) ||
         (condition->lhs.has_value() &&
          value_spelling_matches(*condition->lhs, spelling)) ||
         (condition->rhs.has_value() &&
          value_spelling_matches(*condition->rhs, spelling));
}

[[nodiscard]] bool preserved_value_has_later_non_call_use(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedCallPreservedValue& preserved) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      preserved.value_name == c4c::kInvalidValueName ||
      call_plan.instruction_index >= context.bir_block->insts.size()) {
    return false;
  }
  const auto spelling =
      prepare::prepared_value_name(context.function.prepared->names, preserved.value_name);
  if (spelling.empty()) {
    return false;
  }

  for (std::size_t index = call_plan.instruction_index + 1;
       index < context.bir_block->insts.size();
       ++index) {
    if (std::holds_alternative<bir::CallInst>(context.bir_block->insts[index])) {
      return false;
    }
    if (non_call_instruction_uses_value(context.bir_block->insts[index], spelling)) {
      return true;
    }
  }
  return terminator_uses_value(context.bir_block->terminator, spelling) ||
         branch_condition_uses_value(context, spelling);
}

[[nodiscard]] bool preserved_value_has_block_entry_non_call_use(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPreservedValue& preserved) {
  if (context.function.prepared == nullptr || context.bir_block == nullptr ||
      preserved.value_name == c4c::kInvalidValueName) {
    return false;
  }
  const auto spelling =
      prepare::prepared_value_name(context.function.prepared->names, preserved.value_name);
  if (spelling.empty()) {
    return false;
  }

  for (const auto& inst : context.bir_block->insts) {
    if (std::holds_alternative<bir::CallInst>(inst)) {
      return false;
    }
    if (non_call_instruction_uses_value(inst, spelling)) {
      return true;
    }
  }
  return terminator_uses_value(context.bir_block->terminator, spelling) ||
         branch_condition_uses_value(context, spelling);
}

[[nodiscard]] std::optional<PreservedCallArgumentSource>
make_prior_preserved_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move,
    const prepare::PreparedValueHome* source_home,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* preserved = find_prior_preserved_value_for_call_argument(
      context, current_call_plan, argument, move);
  if (preserved == nullptr) {
    return std::nullopt;
  }
  if ((source_home != nullptr &&
       make_local_frame_address_call_argument_source(
           context, argument, *source_home, instruction_index).has_value()) ||
      aarch64_indirect_register_byval_argument(context, argument, instruction_index)) {
    return std::nullopt;
  }

  const auto source_view =
      source_home != nullptr && source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved->register_name);
  PreservedCallArgumentSource result{.preserved = preserved};
  if (preserved->route == prepare::PreparedCallPreservationRoute::CalleeSavedRegister) {
    auto source = make_register_operand_from_prepared_authority(
        preserved->register_name,
        preserved->register_placement,
        preserved->register_bank,
        RegisterOperandRole::CallAbi,
        preserved->value_id,
        preserved->value_name,
        preserved->contiguous_width,
        preserved->occupied_register_names,
        source_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value()) {
      return std::nullopt;
    }
    source->value_name = c4c::kInvalidValueName;
    result.source_register = *source;
    return result;
  }

  if (preserved->route == prepare::PreparedCallPreservationRoute::StackSlot &&
      preserved->slot_id.has_value() && preserved->stack_offset_bytes.has_value() &&
      preserved->stack_size_bytes.has_value() && *preserved->stack_size_bytes != 0 &&
      preserved->stack_align_bytes.has_value() && *preserved->stack_align_bytes != 0) {
    result.source_memory = MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .result_value_id = argument.source_value_id,
        .result_value_name = std::nullopt,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = preserved->slot_id,
        .byte_offset = static_cast<std::int64_t>(*preserved->stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *preserved->stack_size_bytes,
        .align_bytes = *preserved->stack_align_bytes,
        .can_use_base_plus_offset = true,
    };
    return result;
  }

  return std::nullopt;
}


[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index,
    std::string reason,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.value_name == c4c::kInvalidValueName ||
      !preserved.register_name.has_value() ||
      !preserved.register_bank.has_value()) {
    return std::nullopt;
  }

  const auto* source_home =
      find_value_home(context, preserved.value_id);
  const auto expected_view =
      source_home != nullptr && source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto source = make_register_operand_from_prepared_authority(
      preserved.register_name,
      preserved.register_placement,
      preserved.register_bank,
      RegisterOperandRole::StoragePlan,
      preserved.value_id,
      preserved.value_name,
      preserved.contiguous_width,
      preserved.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  auto destination_register_name = preserved.register_name;
  auto destination_register_placement = preserved.register_placement;
  auto destination_register_bank = preserved.register_bank;
  if (source_home != nullptr &&
      source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value()) {
    destination_register_name =
        register_name_with_expected_view(source_home->register_name, expected_view);
    destination_register_placement = std::nullopt;
    if (destination_register_name.has_value()) {
      const auto parsed =
          abi::parse_aarch64_register_name(*destination_register_name);
      if (parsed.has_value()) {
        if (parsed->bank == abi::RegisterBank::GeneralPurpose) {
          destination_register_bank = prepare::PreparedRegisterBank::Gpr;
        } else if (parsed->bank == abi::RegisterBank::FpSimd) {
          destination_register_bank =
              parsed->view == abi::RegisterView::Q
                  ? prepare::PreparedRegisterBank::Vreg
                  : prepare::PreparedRegisterBank::Fpr;
        }
      }
    }
  }
  auto destination = make_register_operand_from_prepared_authority(
      destination_register_name,
      destination_register_placement,
      destination_register_bank,
      RegisterOperandRole::StoragePlan,
      preserved.value_id,
      preserved.value_name,
      preserved.contiguous_width,
      preserved.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!source.has_value() || !destination.has_value() ||
      (source->reg.bank == destination->reg.bank &&
       source->reg.index == destination->reg.index)) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = preserved.value_id,
      .to_value_id = preserved.value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = destination_register_name,
      .destination_contiguous_width = preserved.contiguous_width,
      .destination_occupied_register_names = preserved.occupied_register_names,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = std::move(reason),
      .destination_register_placement = destination_register_placement,
  };
  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow->function_name,
      .phase = phase,
      .authority_kind = bundle.authority_kind,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .move = synthetic_move,
      .source_register = *source,
      .destination_register = *destination,
      .source_bundle = &bundle,
      .source_move = &synthetic_move,
  };
  return make_preservation_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_republication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  return make_callee_saved_preservation_home_republication_instruction(
      context,
      bundle,
      preserved,
      prepare::PreparedMovePhase::BeforeInstruction,
      call_plan.block_index,
      call_plan.instruction_index,
      "callee_saved_preservation_home_republication",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_population(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallPreservedValue& preserved,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved.value_name == c4c::kInvalidValueName ||
      !preserved.register_name.has_value() ||
      !preserved.register_bank.has_value() ||
      find_prior_preserved_value_for_value(context, call_plan, preserved.value_id) != nullptr) {
    return std::nullopt;
  }
  const auto* source_home =
      find_value_home(context, preserved.value_id);
  if (source_home == nullptr || source_home->value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto expected_view =
      source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(preserved.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      preserved.register_name,
      preserved.register_placement,
      preserved.register_bank,
      RegisterOperandRole::CallAbi,
      preserved.value_id,
      preserved.value_name,
      preserved.contiguous_width,
      preserved.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = preserved.value_id,
      .to_value_id = preserved.value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = preserved.register_name,
      .destination_contiguous_width = preserved.contiguous_width,
      .destination_occupied_register_names = preserved.occupied_register_names,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "callee_saved_preservation_home_population",
      .destination_register_placement = preserved.register_placement,
  };
  CallBoundaryMoveInstructionRecord move_record{
      .function_name = context.function.control_flow->function_name,
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .authority_kind = bundle.authority_kind,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .move = synthetic_move,
      .destination_register = *destination,
      .source_bundle = &bundle,
      .source_move = &synthetic_move,
  };

  if (source_home->kind == prepare::PreparedValueHomeKind::Register &&
      source_home->register_name.has_value()) {
    const auto source_register_name =
        register_name_with_expected_view(source_home->register_name, expected_view);
    const auto source_parsed =
        source_register_name.has_value()
            ? abi::parse_aarch64_register_name(*source_register_name)
            : std::optional<abi::RegisterReference>{};
    std::optional<prepare::PreparedRegisterBank> source_bank;
    if (source_parsed.has_value()) {
      if (source_parsed->bank == abi::RegisterBank::GeneralPurpose) {
        source_bank = prepare::PreparedRegisterBank::Gpr;
      } else if (source_parsed->bank == abi::RegisterBank::FpSimd) {
        source_bank = source_parsed->view == abi::RegisterView::Q
                          ? prepare::PreparedRegisterBank::Vreg
                          : prepare::PreparedRegisterBank::Fpr;
      }
    }
    auto source = make_register_operand_from_prepared_authority(
        source_register_name,
        std::nullopt,
        source_bank,
        RegisterOperandRole::StoragePlan,
        source_home->value_id,
        source_home->value_name,
        1,
        {},
        expected_view,
        diagnostics,
        context,
        instruction_index);
    if (!source.has_value() ||
        (source->reg.bank == destination->reg.bank &&
         source->reg.index == destination->reg.index)) {
      return std::nullopt;
    }
    move_record.source_register = *source;
  } else if (source_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             source_home->offset_bytes.has_value()) {
    move_record.source_memory = MemoryOperand{
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .support = MemoryOperandSupportKind::Prepared,
        .function_name = context.function.control_flow->function_name,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_index = instruction_index,
        .result_value_id = source_home->value_id,
        .result_value_name = source_home->value_name,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = source_home->slot_id,
        .byte_offset = static_cast<std::int64_t>(*source_home->offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = source_home->size_bytes.value_or(
            scalar_size_from_register_view(expected_view)),
        .align_bytes = source_home->align_bytes.value_or(
            scalar_size_from_register_view(expected_view)),
        .can_use_base_plus_offset = true,
    };
  } else {
    return std::nullopt;
  }

  return make_preservation_machine_instruction(
      context,
      instruction_index,
      make_call_boundary_move_instruction(std::move(move_record)));
}


}  // namespace c4c::backend::aarch64::codegen
