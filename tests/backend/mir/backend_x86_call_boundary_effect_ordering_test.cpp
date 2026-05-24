#include "src/backend/prealloc/calls.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

namespace prepare = c4c::backend::prepare;

enum class X86CallBoundaryAction {
  Reject,
  MoveIntoCallArgument,
  PreserveLiveValue,
  MoveCallResult,
  RepublishPreservedValue,
};

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedCallPlan make_call_plan() {
  return prepare::PreparedCallPlan{
      .block_index = 4,
      .instruction_index = 12,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .arguments =
          {prepare::PreparedCallArgumentPlan{
              .instruction_index = 12,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .source_value_id = prepare::PreparedValueId{101},
              .source_slot_id = prepare::PreparedFrameSlotId{3},
              .source_stack_offset_bytes = std::size_t{24},
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          }},
      .result =
          prepare::PreparedCallResultPlan{
              .instruction_index = 12,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::StackSlot,
              .destination_value_id = prepare::PreparedValueId{202},
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_slot_id = prepare::PreparedFrameSlotId{5},
              .destination_stack_offset_bytes = std::size_t{40},
          },
      .preserved_values =
          {prepare::PreparedCallPreservedValue{
               .value_id = prepare::PreparedValueId{301},
               .value_name = c4c::ValueNameId{401},
               .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
               .callee_saved_save_index = std::size_t{2},
               .register_bank = prepare::PreparedRegisterBank::Gpr,
           },
           prepare::PreparedCallPreservedValue{
               .value_id = prepare::PreparedValueId{302},
               .value_name = c4c::ValueNameId{402},
               .route = prepare::PreparedCallPreservationRoute::StackSlot,
               .slot_id = prepare::PreparedFrameSlotId{8},
               .stack_offset_bytes = std::size_t{64},
               .stack_size_bytes = std::size_t{8},
               .stack_align_bytes = std::size_t{8},
           }},
  };
}

prepare::PreparedMoveBundle make_before_call_bundle() {
  return prepare::PreparedMoveBundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 4,
      .instruction_index = 12,
      .moves =
          {prepare::PreparedMoveResolution{
              .from_value_id = prepare::PreparedValueId{101},
              .destination_kind =
                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .block_index = 4,
              .instruction_index = 12,
              .reason = "x86 argument move",
          }},
      .abi_bindings =
          {prepare::PreparedAbiBinding{
              .destination_kind =
                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
          }},
  };
}

prepare::PreparedMoveBundle make_after_call_bundle() {
  return prepare::PreparedMoveBundle{
      .phase = prepare::PreparedMovePhase::AfterCall,
      .block_index = 4,
      .instruction_index = 12,
      .moves =
          {prepare::PreparedMoveResolution{
              .to_value_id = prepare::PreparedValueId{202},
              .destination_kind =
                  prepare::PreparedMoveDestinationKind::CallResultAbi,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .block_index = 4,
              .instruction_index = 12,
              .reason = "x86 result move",
          }},
      .abi_bindings =
          {prepare::PreparedAbiBinding{
              .destination_kind =
                  prepare::PreparedMoveDestinationKind::CallResultAbi,
              .destination_storage_kind =
                  prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
          }},
  };
}

X86CallBoundaryAction choose_x86_call_boundary_action(
    const prepare::PreparedCallBoundaryEffectPlan& effect) {
  if (effect.classification_status !=
      prepare::PreparedCallBoundaryMoveClassificationStatus::Available) {
    return X86CallBoundaryAction::Reject;
  }

  switch (effect.effect_kind) {
    case prepare::PreparedCallBoundaryEffectKind::ExplicitMove:
      if (effect.phase == prepare::PreparedMovePhase::BeforeCall &&
          effect.destination_kind ==
              prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
          effect.source.storage_kind ==
              prepare::PreparedMoveStorageKind::StackSlot &&
          effect.destination.storage_kind ==
              prepare::PreparedMoveStorageKind::Register) {
        return X86CallBoundaryAction::MoveIntoCallArgument;
      }
      if (effect.phase == prepare::PreparedMovePhase::AfterCall &&
          effect.destination_kind ==
              prepare::PreparedMoveDestinationKind::CallResultAbi &&
          effect.source.storage_kind ==
              prepare::PreparedMoveStorageKind::Register &&
          effect.destination.storage_kind ==
              prepare::PreparedMoveStorageKind::StackSlot) {
        return X86CallBoundaryAction::MoveCallResult;
      }
      return X86CallBoundaryAction::Reject;
    case prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation:
      return effect.phase == prepare::PreparedMovePhase::BeforeCall &&
                     effect.destination.storage_kind !=
                         prepare::PreparedMoveStorageKind::None
                 ? X86CallBoundaryAction::PreserveLiveValue
                 : X86CallBoundaryAction::Reject;
    case prepare::PreparedCallBoundaryEffectKind::PreservationRepublication:
      return effect.phase == prepare::PreparedMovePhase::AfterCall &&
                     effect.source.storage_kind !=
                         prepare::PreparedMoveStorageKind::None
                 ? X86CallBoundaryAction::RepublishPreservedValue
                 : X86CallBoundaryAction::Reject;
  }
  return X86CallBoundaryAction::Reject;
}

int x86_consumer_observes_neutral_effect_ordering() {
  const auto call_plan = make_call_plan();
  const auto before = make_before_call_bundle();
  const auto after = make_after_call_bundle();
  const auto effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, &before, &after);

  const std::vector<X86CallBoundaryAction> expected{
      X86CallBoundaryAction::MoveIntoCallArgument,
      X86CallBoundaryAction::PreserveLiveValue,
      X86CallBoundaryAction::PreserveLiveValue,
      X86CallBoundaryAction::MoveCallResult,
      X86CallBoundaryAction::RepublishPreservedValue,
      X86CallBoundaryAction::RepublishPreservedValue,
  };

  if (effects.size() != expected.size()) {
    return fail("expected x86 consumer to see every call-boundary effect");
  }
  for (std::size_t index = 0; index < effects.size(); ++index) {
    if (effects[index].order_index != index) {
      return fail("expected contiguous neutral effect order indexes");
    }
    if (effects[index].block_index != 4 || effects[index].instruction_index != 12) {
      return fail("expected call-site identity on every effect");
    }
    if (choose_x86_call_boundary_action(effects[index]) != expected[index]) {
      return fail("expected x86 consumer action to match neutral effect order");
    }
  }

  if (effects[1].destination.callee_saved_save_index !=
          std::optional<std::size_t>{2} ||
      effects[2].destination.slot_id !=
          std::optional<prepare::PreparedFrameSlotId>{8} ||
      effects[4].destination.value_name != c4c::ValueNameId{401} ||
      effects[5].destination.value_id !=
          std::optional<prepare::PreparedValueId>{302}) {
    return fail("expected preservation endpoints to survive cross-target reuse");
  }

  return 0;
}

int x86_consumer_rejects_unavailable_explicit_effects() {
  const auto call_plan = make_call_plan();
  auto before = make_before_call_bundle();
  before.abi_bindings.clear();

  const auto effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, &before, nullptr);
  if (effects.empty()) {
    return fail("expected explicit effect for unavailable before-call move");
  }
  if (effects.front().classification_status !=
      prepare::PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding) {
    return fail("expected missing ABI binding classification");
  }
  if (choose_x86_call_boundary_action(effects.front()) !=
      X86CallBoundaryAction::Reject) {
    return fail("expected x86 consumer to reject unavailable explicit move");
  }
  return 0;
}

}  // namespace

int main() {
  if (int rc = x86_consumer_observes_neutral_effect_ordering(); rc != 0) {
    return rc;
  }
  return x86_consumer_rejects_unavailable_explicit_effects();
}
