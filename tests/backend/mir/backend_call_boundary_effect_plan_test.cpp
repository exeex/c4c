#include "src/backend/prealloc/call_plans.hpp"
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
                       .register_name = std::string{"x0"},
                       .register_bank = prepare::PreparedRegisterBank::Gpr,
                       .contiguous_width = 1,
                       .occupied_register_names = {std::string{"x0"}},
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

prepare::PreparedCallPlan make_register_source_call(std::size_t block_index,
                                                    std::size_t instruction_index,
                                                    prepare::PreparedValueId value_id,
                                                    std::string source_register) {
  return prepare::PreparedCallPlan{
      .block_index = block_index,
      .instruction_index = instruction_index,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .arguments =
          {prepare::PreparedCallArgumentPlan{
              .instruction_index = instruction_index,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Gpr,
              .source_encoding = prepare::PreparedStorageEncodingKind::Register,
              .source_value_id = value_id,
              .source_register_name = std::move(source_register),
              .source_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_name = std::string{"a0"},
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          }},
  };
}

prepare::PreparedCallPreservedValue make_supported_callee_saved_preservation(
    prepare::PreparedValueId value_id,
    std::string source_register) {
  return prepare::PreparedCallPreservedValue{
      .value_id = value_id,
      .value_name = c4c::ValueNameId{51},
      .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
      .callee_saved_save_index = std::size_t{0},
      .contiguous_width = 1,
      .register_name = std::string{"s1"},
      .register_bank = prepare::PreparedRegisterBank::Gpr,
      .occupied_register_names = {std::string{"s1"}},
      .register_placement =
          prepare::PreparedRegisterPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
              .slot_index = 0,
              .contiguous_width = 1,
          },
      .preservation_source =
          prepare::PreparedCallBoundaryEffectEndpoint{
              .encoding = prepare::PreparedStorageEncodingKind::Register,
              .storage_kind = prepare::PreparedMoveStorageKind::Register,
              .value_id = value_id,
              .value_name = c4c::ValueNameId{51},
              .register_name = std::move(source_register),
              .register_bank = prepare::PreparedRegisterBank::Gpr,
              .contiguous_width = 1,
              .occupied_register_names = {std::string{"a0"}},
          },
      .preservation_destination =
          prepare::PreparedCallBoundaryEffectEndpoint{
              .encoding = prepare::PreparedStorageEncodingKind::Register,
              .storage_kind = prepare::PreparedMoveStorageKind::Register,
              .value_id = value_id,
              .value_name = c4c::ValueNameId{51},
              .register_name = std::string{"s1"},
              .register_bank = prepare::PreparedRegisterBank::Gpr,
              .contiguous_width = 1,
              .occupied_register_names = {std::string{"s1"}},
              .callee_saved_save_index = std::size_t{0},
              .register_placement =
                  prepare::PreparedRegisterPlacement{
                      .bank = prepare::PreparedRegisterBank::Gpr,
                      .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                      .slot_index = 0,
                      .contiguous_width = 1,
                  },
          },
      .preservation_reason = "callee_saved_register_preservation",
  };
}

prepare::PreparedCallPreservedValue make_unsupported_non_register_source_preservation(
    prepare::PreparedValueId value_id) {
  auto preserved =
      make_supported_callee_saved_preservation(value_id, std::string{"a0"});
  preserved.preservation_source =
      prepare::PreparedCallBoundaryEffectEndpoint{
          .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
          .storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .value_id = value_id,
          .value_name = c4c::ValueNameId{51},
          .slot_id = prepare::PreparedFrameSlotId{4},
          .stack_offset_bytes = std::size_t{24},
          .stack_size_bytes = std::size_t{8},
          .stack_align_bytes = std::size_t{8},
      };
  return preserved;
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

bool seeds_supported_prior_register_source_preservation() {
  const auto value_id = prepare::PreparedValueId{101};
  prepare::PreparedCallPlansFunction function_plan{
      .function_name = c4c::FunctionNameId{7},
      .calls =
          {make_register_source_call(0, 0, value_id, std::string{"a1"}),
           make_register_source_call(0, 1, value_id, std::string{"a0"}),
           make_register_source_call(0, 2, value_id, std::string{"a0"})},
  };
  function_plan.calls.back().preserved_values.push_back(
      make_supported_callee_saved_preservation(value_id, std::string{"a0"}));

  prepare::seed_supported_prior_call_preservations_from_current_call(
      function_plan, function_plan.calls.back());

  const auto& different_source_call = function_plan.calls[0];
  if (!expect(different_source_call.preserved_values.empty(),
              "expected different source register prior call to remain unseeded")) {
    return false;
  }

  const auto& seeded_call = function_plan.calls[1];
  if (!expect(seeded_call.preserved_values.size() == 1,
              "expected matching register-source prior call to receive preservation") ||
      !expect(seeded_call.preserved_values.front().value_id == value_id,
              "expected seeded preserved value id") ||
      !expect(seeded_call.preserved_values.front().preservation_source.storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "expected seeded preservation source to remain register-backed") ||
      !expect(seeded_call.preserved_values.front().preservation_source.register_name ==
                  std::optional<std::string>{"a0"},
              "expected seeded preservation source register") ||
      !expect(seeded_call.preserved_values.front().preservation_destination.storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "expected seeded preservation destination to remain register-backed") ||
      !expect(seeded_call.preserved_values.front().preservation_destination.register_name ==
                  std::optional<std::string>{"s1"},
              "expected seeded preservation destination register")) {
    return false;
  }

  const auto effects =
      prepare::plan_prepared_call_boundary_effects(seeded_call, nullptr, nullptr);
  const auto population_it = std::find_if(
      effects.begin(), effects.end(), [value_id](const auto& effect) {
        return effect.effect_kind ==
                   prepare::PreparedCallBoundaryEffectKind::
                       PreservationHomePopulation &&
               effect.source.value_id ==
                   std::optional<prepare::PreparedValueId>{value_id} &&
               effect.source.register_name == std::optional<std::string>{"a0"} &&
               effect.destination.register_name ==
                   std::optional<std::string>{"s1"};
      });
  return expect(population_it != effects.end(),
                "expected seeded prior call to yield preservation home population");
}

bool does_not_seed_unsupported_non_register_preservation_source() {
  const auto value_id = prepare::PreparedValueId{102};
  prepare::PreparedCallPlansFunction function_plan{
      .function_name = c4c::FunctionNameId{8},
      .calls =
          {make_register_source_call(0, 0, value_id, std::string{"a0"}),
           make_register_source_call(0, 1, value_id, std::string{"a0"})},
  };
  function_plan.calls.back().preserved_values.push_back(
      make_unsupported_non_register_source_preservation(value_id));

  prepare::seed_supported_prior_call_preservations_from_current_call(
      function_plan, function_plan.calls.back());

  return expect(function_plan.calls.front().preserved_values.empty(),
                "expected unsupported non-register preservation source to stay unseeded");
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
      !expect(callee_saved_population.source.storage_kind ==
                  prepare::PreparedMoveStorageKind::Register,
              "expected register preservation source") ||
      !expect(callee_saved_population.source.register_name ==
                  std::optional<std::string>{"x0"},
              "expected live pre-call preservation source register") ||
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
         expect(callee_saved_republication.destination.storage_kind ==
                    prepare::PreparedMoveStorageKind::Register,
                "expected callee-saved republication destination storage") &&
         expect(callee_saved_republication.destination.register_name ==
                    std::optional<std::string>{"x0"},
                "expected callee-saved republication destination register") &&
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
                "expected stack republication destination value") &&
         expect(stack_republication.destination.storage_kind ==
                    prepare::PreparedMoveStorageKind::Register,
                "expected stack republication destination home") &&
         expect(stack_republication.destination.register_name ==
                    std::optional<std::string>{"x9"},
                "expected stack republication destination register");
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
      !seeds_supported_prior_register_source_preservation() ||
      !does_not_seed_unsupported_non_register_preservation_source() ||
      !records_preservation_and_republication_intent() ||
      !records_unavailable_explicit_move_without_target_operands()) {
    return 1;
  }
  return 0;
}
