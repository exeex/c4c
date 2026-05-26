#include "src/backend/prealloc/calls.hpp"

#include <cstddef>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace {

using namespace c4c::backend;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
    return false;
  }
  return true;
}

prepare::PreparedCallPlan make_call_plan() {
  return prepare::PreparedCallPlan{
      .block_index = 2,
      .instruction_index = 9,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .arguments =
          {prepare::PreparedCallArgumentPlan{
              .instruction_index = 9,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .source_value_id = prepare::PreparedValueId{11},
              .source_slot_id = prepare::PreparedFrameSlotId{5},
              .source_stack_offset_bytes = std::size_t{32},
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          }},
      .result =
          prepare::PreparedCallResultPlan{
              .instruction_index = 9,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
              .destination_value_id = prepare::PreparedValueId{21},
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_slot_id = prepare::PreparedFrameSlotId{6},
              .destination_stack_offset_bytes = std::size_t{48},
          },
      .preserved_values =
          {prepare::PreparedCallPreservedValue{
               .value_id = prepare::PreparedValueId{31},
               .value_name = c4c::ValueNameId{41},
               .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
               .callee_saved_save_index = std::size_t{3},
               .contiguous_width = 2,
               .register_name = std::string{"x19"},
               .register_bank = prepare::PreparedRegisterBank::Gpr,
               .occupied_register_names = {std::string{"x19"}, std::string{"x20"}},
               .register_placement =
                   prepare::PreparedRegisterPlacement{
                       .bank = prepare::PreparedRegisterBank::Gpr,
                       .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                       .slot_index = 0,
                       .contiguous_width = 2,
                   },
               .preservation_source =
                   prepare::PreparedCallBoundaryEffectEndpoint{
                       .encoding = prepare::PreparedStorageEncodingKind::Register,
                       .storage_kind = prepare::PreparedMoveStorageKind::Register,
                       .value_id = prepare::PreparedValueId{31},
                       .value_name = c4c::ValueNameId{41},
                       .register_name = std::string{"x19"},
                       .register_bank = prepare::PreparedRegisterBank::Gpr,
                       .contiguous_width = 2,
                       .occupied_register_names = {std::string{"x19"}, std::string{"x20"}},
                   },
               .preservation_destination =
                   prepare::PreparedCallBoundaryEffectEndpoint{
                       .encoding = prepare::PreparedStorageEncodingKind::Register,
                       .storage_kind = prepare::PreparedMoveStorageKind::Register,
                       .value_id = prepare::PreparedValueId{31},
                       .value_name = c4c::ValueNameId{41},
                       .register_name = std::string{"x19"},
                       .register_bank = prepare::PreparedRegisterBank::Gpr,
                       .contiguous_width = 2,
                       .occupied_register_names = {std::string{"x19"}, std::string{"x20"}},
                       .callee_saved_save_index = std::size_t{3},
                       .register_placement =
                           prepare::PreparedRegisterPlacement{
                               .bank = prepare::PreparedRegisterBank::Gpr,
                               .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                               .slot_index = 0,
                               .contiguous_width = 2,
                           },
                   },
               .preservation_reason = "callee_saved_register_preservation",
           },
           prepare::PreparedCallPreservedValue{
               .value_id = prepare::PreparedValueId{32},
               .value_name = c4c::ValueNameId{42},
               .route = prepare::PreparedCallPreservationRoute::StackSlot,
               .slot_id = prepare::PreparedFrameSlotId{7},
               .stack_offset_bytes = std::size_t{80},
               .stack_size_bytes = std::size_t{8},
               .stack_align_bytes = std::size_t{8},
               .spill_slot_placement =
                   prepare::PreparedSpillSlotPlacement{
                       .slot_id = prepare::PreparedFrameSlotId{7},
                       .offset_bytes = 80,
                   },
               .preservation_source =
                   prepare::PreparedCallBoundaryEffectEndpoint{
                       .encoding = prepare::PreparedStorageEncodingKind::Register,
                       .storage_kind = prepare::PreparedMoveStorageKind::Register,
                       .value_id = prepare::PreparedValueId{32},
                       .value_name = c4c::ValueNameId{42},
                       .register_name = std::string{"x9"},
                       .register_bank = prepare::PreparedRegisterBank::Gpr,
                       .contiguous_width = 1,
                       .occupied_register_names = {std::string{"x9"}},
                   },
               .preservation_destination =
                   prepare::PreparedCallBoundaryEffectEndpoint{
                       .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                       .storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
                       .value_id = prepare::PreparedValueId{32},
                       .value_name = c4c::ValueNameId{42},
                       .slot_id = prepare::PreparedFrameSlotId{7},
                       .stack_offset_bytes = std::size_t{80},
                       .stack_size_bytes = std::size_t{8},
                       .stack_align_bytes = std::size_t{8},
                       .spill_slot_placement =
                           prepare::PreparedSpillSlotPlacement{
                               .slot_id = prepare::PreparedFrameSlotId{7},
                               .offset_bytes = 80,
                           },
                   },
               .preservation_reason = "caller_saved_clobber_reuse_stack_preservation",
           }},
  };
}

prepare::PreparedMoveBundle make_before_call_bundle() {
  return prepare::PreparedMoveBundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 2,
      .instruction_index = 9,
      .moves =
          {prepare::PreparedMoveResolution{
              .from_value_id = prepare::PreparedValueId{11},
              .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .block_index = 2,
              .instruction_index = 9,
              .reason = "argument abi move",
          }},
      .abi_bindings =
          {prepare::PreparedAbiBinding{
              .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
          }},
  };
}

prepare::PreparedMoveBundle make_after_call_bundle() {
  return prepare::PreparedMoveBundle{
      .phase = prepare::PreparedMovePhase::AfterCall,
      .block_index = 2,
      .instruction_index = 9,
      .moves =
          {prepare::PreparedMoveResolution{
              .to_value_id = prepare::PreparedValueId{21},
              .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .block_index = 2,
              .instruction_index = 9,
              .reason = "result abi move",
          }},
      .abi_bindings =
          {prepare::PreparedAbiBinding{
              .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
          }},
  };
}

bool records_explicit_before_and_after_moves() {
  const auto call_plan = make_call_plan();
  const auto before = make_before_call_bundle();
  const auto after = make_after_call_bundle();
  const auto effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, &before, &after);

  if (!expect(effects.size() == 6, "expected explicit and preservation effects")) {
    return false;
  }

  const auto& before_move = effects[0];
  if (!expect(before_move.effect_kind ==
                  prepare::PreparedCallBoundaryEffectKind::ExplicitMove,
              "expected first effect to be explicit before-call move") ||
      !expect(before_move.phase == prepare::PreparedMovePhase::BeforeCall,
              "expected before-call phase") ||
      !expect(before_move.classification_status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::Available,
              "expected available argument move classification") ||
      !expect(before_move.abi_index == std::optional<std::size_t>{0},
              "expected argument abi index") ||
      !expect(before_move.source.value_id ==
                  std::optional<prepare::PreparedValueId>{11},
              "expected source value id from argument plan") ||
      !expect(before_move.source.storage_kind ==
                  prepare::PreparedMoveStorageKind::StackSlot,
              "expected source stack storage") ||
      !expect(before_move.destination.storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "expected destination register storage")) {
    return false;
  }

  const auto& after_move = effects[3];
  return expect(after_move.effect_kind ==
                    prepare::PreparedCallBoundaryEffectKind::ExplicitMove,
                "expected after-call explicit move") &&
         expect(after_move.phase == prepare::PreparedMovePhase::AfterCall,
                "expected after-call phase") &&
         expect(after_move.source.storage_kind ==
                    prepare::PreparedMoveStorageKind::Register,
                "expected result source register storage") &&
         expect(after_move.destination.value_id ==
                    std::optional<prepare::PreparedValueId>{21},
                "expected result destination value id") &&
         expect(after_move.destination.slot_id ==
                    std::optional<prepare::PreparedFrameSlotId>{6},
                "expected result destination slot id");
}

bool records_preservation_and_republication_intent() {
  const auto call_plan = make_call_plan();
  const auto before = make_before_call_bundle();
  const auto after = make_after_call_bundle();
  const auto effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, &before, &after);

  const auto& callee_saved_population = effects[1];
  if (!expect(callee_saved_population.effect_kind ==
                  prepare::PreparedCallBoundaryEffectKind::PreservationHomePopulation,
              "expected callee-saved population effect") ||
      !expect(callee_saved_population.destination.storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "expected register preservation destination") ||
      !expect(callee_saved_population.destination.callee_saved_save_index ==
                  std::optional<std::size_t>{3},
              "expected callee-saved index") ||
      !expect(callee_saved_population.destination.register_name ==
                  std::optional<std::string>{"x19"},
              "expected callee-saved register name") ||
      !expect(callee_saved_population.destination.register_placement ==
                  std::optional<prepare::PreparedRegisterPlacement>{
                      prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                          .slot_index = 0,
                          .contiguous_width = 2,
                      }},
              "expected callee-saved register placement") ||
      !expect(callee_saved_population.destination.occupied_register_names ==
                  std::vector<std::string>{std::string{"x19"}, std::string{"x20"}},
              "expected callee-saved occupied register names") ||
      !expect(callee_saved_population.preservation_route ==
                  prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
              "expected callee-saved preservation route")) {
    return false;
  }

  const auto& stack_population = effects[2];
  if (!expect(stack_population.source.storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "expected stack preservation source register") ||
      !expect(stack_population.source.register_name ==
                  std::optional<std::string>{"x9"},
              "expected stack preservation caller-saved source register") ||
      !expect(stack_population.destination.storage_kind ==
                  prepare::PreparedMoveStorageKind::StackSlot,
              "expected stack preservation destination") ||
      !expect(stack_population.destination.slot_id ==
                  std::optional<prepare::PreparedFrameSlotId>{7},
              "expected stack preservation slot") ||
      !expect(stack_population.destination.stack_size_bytes ==
                  std::optional<std::size_t>{8},
              "expected stack preservation size")) {
    return false;
  }
  if (!expect(stack_population.destination.spill_slot_placement ==
                  std::optional<prepare::PreparedSpillSlotPlacement>{
                      prepare::PreparedSpillSlotPlacement{
                          .slot_id = prepare::PreparedFrameSlotId{7},
                          .offset_bytes = 80,
                      }},
              "expected stack preservation spill slot placement")) {
    return false;
  }

  const auto& callee_saved_republication = effects[4];
  const auto& stack_republication = effects[5];
  return expect(callee_saved_republication.effect_kind ==
                    prepare::PreparedCallBoundaryEffectKind::PreservationRepublication,
                "expected callee-saved republication effect") &&
         expect(callee_saved_republication.source.storage_kind ==
                    prepare::PreparedMoveStorageKind::Register,
                "expected callee-saved republication source") &&
         expect(callee_saved_republication.source.register_name ==
                    std::optional<std::string>{"x19"},
                "expected callee-saved republication register name") &&
         expect(callee_saved_republication.source.register_placement ==
                    std::optional<prepare::PreparedRegisterPlacement>{
                        prepare::PreparedRegisterPlacement{
                            .bank = prepare::PreparedRegisterBank::Gpr,
                            .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                            .slot_index = 0,
                            .contiguous_width = 2,
                        }},
                "expected callee-saved republication register placement") &&
         expect(callee_saved_republication.source.occupied_register_names ==
                    std::vector<std::string>{std::string{"x19"}, std::string{"x20"}},
                "expected callee-saved republication occupied register names") &&
         expect(callee_saved_republication.destination.value_name ==
                    c4c::ValueNameId{41},
                "expected callee-saved republication value name") &&
         expect(stack_republication.source.storage_kind ==
                    prepare::PreparedMoveStorageKind::StackSlot,
                "expected stack republication source") &&
         expect(stack_republication.source.spill_slot_placement ==
                    std::optional<prepare::PreparedSpillSlotPlacement>{
                        prepare::PreparedSpillSlotPlacement{
                            .slot_id = prepare::PreparedFrameSlotId{7},
                            .offset_bytes = 80,
                        }},
                "expected stack republication spill slot placement") &&
         expect(stack_republication.destination.value_id ==
                    std::optional<prepare::PreparedValueId>{32},
                "expected stack republication destination value");
}

bool records_unavailable_explicit_move_without_target_operands() {
  const auto call_plan = make_call_plan();
  auto before = make_before_call_bundle();
  before.moves.front().op_kind =
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp;

  const auto effects =
      prepare::plan_prepared_call_boundary_effects(call_plan, &before, nullptr);
  return expect(!effects.empty(), "expected unavailable effect record") &&
         expect(effects.front().classification_status ==
                    prepare::PreparedCallBoundaryMoveClassificationStatus::UnsupportedOpKind,
                "expected unsupported op classification") &&
         expect(effects.front().destination.storage_kind ==
                    prepare::PreparedMoveStorageKind::Register,
                "expected storage fact without target operand record");
}

}  // namespace

int main() {
  if (!records_explicit_before_and_after_moves() ||
      !records_preservation_and_republication_intent() ||
      !records_unavailable_explicit_move_without_target_operands()) {
    return 1;
  }
  return 0;
}
