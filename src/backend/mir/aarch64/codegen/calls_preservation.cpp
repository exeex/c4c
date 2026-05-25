#include "calls.hpp"
#include "dispatch.hpp"
#include "dispatch_lookup.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
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

[[nodiscard]] std::optional<prepare::PreparedValueId> effect_value_id(
    const prepare::PreparedCallBoundaryEffectEndpoint& primary,
    const prepare::PreparedCallBoundaryEffectEndpoint& fallback) {
  return primary.value_id.has_value() ? primary.value_id : fallback.value_id;
}

[[nodiscard]] ValueNameId effect_value_name(
    const prepare::PreparedCallBoundaryEffectEndpoint& primary,
    const prepare::PreparedCallBoundaryEffectEndpoint& fallback) {
  return primary.value_name != c4c::kInvalidValueName ? primary.value_name
                                                       : fallback.value_name;
}

}  // namespace

[[nodiscard]] const prepare::PreparedMoveBundle* find_move_bundle(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index) {
  return prepare::find_indexed_prepared_move_bundle(context.function.move_bundle_lookups,
                                                    context.function.value_locations,
                                                    phase,
                                                    block_index,
                                                    instruction_index);
}

[[nodiscard]] const prepare::PreparedCallPreservedValue*
find_prior_preserved_value_for_call_argument(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& current_call_plan,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedMoveResolution& move) {
  const auto value_id = argument.source_value_id.value_or(move.from_value_id);
  if (context.function.call_plan_lookups != nullptr) {
    return prepare::find_latest_indexed_prior_preserved_value(
        *context.function.call_plan_lookups, current_call_plan, value_id);
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
  if (context.function.call_plan_lookups == nullptr) {
    return nullptr;
  }
  return prepare::find_dominating_indexed_prior_preserved_value(
      *context.function.call_plan_lookups,
      context.function.control_flow,
      current_call_plan,
      value_id);
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
  if (source_home != nullptr &&
      (make_local_frame_address_call_argument_source(
           context, argument, *source_home, instruction_index)
           .has_value() ||
       prepared_indirect_byval_extent_bytes(context, move, argument, *source_home)
           .has_value())) {
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
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index,
    std::string reason,
    module::ModuleLoweringDiagnostics& diagnostics) {
  // Emission-only: prepared lookups select eligibility and block-entry reachability.
  const auto& storage = effect.source;
  const auto& value = effect.destination;
  const auto value_id = effect_value_id(value, storage);
  const auto value_name = effect_value_name(value, storage);
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      effect.effect_kind !=
          prepare::PreparedCallBoundaryEffectKind::PreservationRepublication ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !value_id.has_value() ||
      value_name == c4c::kInvalidValueName ||
      storage.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !storage.register_name.has_value() ||
      !storage.register_bank.has_value()) {
    return std::nullopt;
  }

  const auto* source_home =
      find_value_home(context, *value_id);
  const auto expected_view =
      source_home != nullptr && source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(storage.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto source = make_register_operand_from_prepared_authority(
      storage.register_name,
      storage.register_placement,
      storage.register_bank,
      RegisterOperandRole::StoragePlan,
      *value_id,
      value_name,
      storage.contiguous_width,
      storage.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  auto destination_register_name = storage.register_name;
  auto destination_register_placement = storage.register_placement;
  auto destination_register_bank = storage.register_bank;
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
      *value_id,
      value_name,
      storage.contiguous_width,
      storage.occupied_register_names,
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
      .from_value_id = *value_id,
      .to_value_id = *value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = destination_register_name,
      .destination_contiguous_width = storage.contiguous_width,
      .destination_occupied_register_names = storage.occupied_register_names,
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
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  return make_callee_saved_preservation_home_republication_instruction(
      context,
      bundle,
      effect,
      prepare::PreparedMovePhase::BeforeInstruction,
      call_plan.block_index,
      call_plan.instruction_index,
      effect.reason.empty() ? "callee_saved_preservation_home_republication"
                            : effect.reason,
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_callee_saved_preservation_home_population(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const prepare::PreparedMoveBundle& bundle,
    const prepare::PreparedCallBoundaryEffectPlan& effect,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto& value = effect.source;
  const auto& storage = effect.destination;
  const auto value_id = effect_value_id(value, storage);
  const auto value_name = effect_value_name(value, storage);
  if (context.function.value_locations == nullptr ||
      context.function.control_flow == nullptr ||
      effect.effect_kind !=
          prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation ||
      effect.preservation_route !=
          prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !value_id.has_value() ||
      value_name == c4c::kInvalidValueName ||
      storage.storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !storage.register_name.has_value() ||
      !storage.register_bank.has_value() ||
      find_prior_preserved_value_for_value(context, call_plan, *value_id) != nullptr) {
    return std::nullopt;
  }
  const auto* source_home =
      find_value_home(context, *value_id);
  if (source_home == nullptr || source_home->value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  const auto expected_view =
      source_home->size_bytes.has_value()
          ? scalar_integer_register_view_from_size(*source_home->size_bytes)
          : scalar_view_from_register_name(storage.register_name);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }

  auto destination = make_register_operand_from_prepared_authority(
      storage.register_name,
      storage.register_placement,
      storage.register_bank,
      RegisterOperandRole::CallAbi,
      *value_id,
      value_name,
      storage.contiguous_width,
      storage.occupied_register_names,
      expected_view,
      diagnostics,
      context,
      instruction_index);
  if (!destination.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedMoveResolution synthetic_move{
      .from_value_id = *value_id,
      .to_value_id = *value_id,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = storage.register_name,
      .destination_contiguous_width = storage.contiguous_width,
      .destination_occupied_register_names = storage.occupied_register_names,
      .block_index = call_plan.block_index,
      .instruction_index = call_plan.instruction_index,
      .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
      .reason = "callee_saved_preservation_home_population",
      .destination_register_placement = storage.register_placement,
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
