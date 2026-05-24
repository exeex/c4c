#include "src/backend/prealloc/call_plans.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace prepare = c4c::backend::prepare;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

prepare::PreparedRegisterPlacement sample_gpr_placement() {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
      .slot_index = 2,
      .contiguous_width = 1,
  };
}

prepare::PreparedCallPlan sample_call_plan() {
  const auto placement = sample_gpr_placement();
  return prepare::PreparedCallPlan{
      .block_index = 3,
      .instruction_index = 9,
      .arguments = {
          prepare::PreparedCallArgumentPlan{
              .instruction_index = 9,
              .arg_index = 0,
              .source_encoding = prepare::PreparedStorageEncodingKind::Register,
              .source_value_id = 10,
              .destination_register_name = "x0",
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_placement = placement,
          },
          prepare::PreparedCallArgumentPlan{
              .instruction_index = 9,
              .arg_index = 1,
              .source_encoding = prepare::PreparedStorageEncodingKind::Immediate,
              .destination_register_name = "x1",
              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
              .destination_register_placement = placement,
          },
          prepare::PreparedCallArgumentPlan{
              .instruction_index = 9,
              .arg_index = 2,
              .source_encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
              .source_value_id = 12,
              .destination_stack_offset_bytes = 16,
          },
      },
      .result = prepare::PreparedCallResultPlan{
          .instruction_index = 9,
          .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_value_id = 20,
          .source_register_name = "x0",
          .source_register_bank = prepare::PreparedRegisterBank::Gpr,
          .destination_register_name = "x3",
          .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          .source_register_placement = placement,
      },
  };
}

prepare::PreparedAbiBinding binding_for(const prepare::PreparedMoveResolution& move) {
  return prepare::PreparedAbiBinding{
      .destination_kind = move.destination_kind,
      .destination_storage_kind = move.destination_storage_kind,
      .destination_abi_index = move.destination_abi_index,
      .destination_register_name = move.destination_register_name,
      .destination_contiguous_width = move.destination_contiguous_width,
      .destination_occupied_register_names = move.destination_occupied_register_names,
      .destination_stack_offset_bytes = move.destination_stack_offset_bytes,
      .destination_register_placement = move.destination_register_placement,
  };
}

int verify_argument_classification() {
  const auto call_plan = sample_call_plan();
  const auto placement = sample_gpr_placement();
  const prepare::PreparedMoveResolution register_arg{
      .from_value_id = 10,
      .to_value_id = 10,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_abi_index = 0,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution immediate_arg{
      .from_value_id = 99,
      .to_value_id = 99,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_abi_index = 1,
      .destination_register_name = "x1",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution byval_lane{
      .from_value_id = 777,
      .to_value_id = 777,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
      .destination_abi_index = 2,
      .destination_stack_offset_bytes = 16,
      .reason = "target-local-reason-string-must-not-control-classification",
  };
  const prepare::PreparedMoveBundle bundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 3,
      .instruction_index = 9,
      .moves = {register_arg, immediate_arg, byval_lane},
      .abi_bindings = {
          binding_for(register_arg),
          binding_for(immediate_arg),
          binding_for(byval_lane),
      },
  };

  const auto reg = prepare::classify_prepared_call_boundary_move(
      call_plan, bundle, bundle.moves[0]);
  const auto imm = prepare::classify_prepared_call_boundary_move(
      call_plan, bundle, bundle.moves[1]);
  const auto byval = prepare::classify_prepared_call_boundary_move(
      call_plan, bundle, bundle.moves[2]);
  if (!expect(prepare::prepared_call_boundary_move_classification_available(reg),
              "register argument classification should be available") ||
      !expect(reg.phase == prepare::PreparedMovePhase::BeforeCall,
              "argument phase was not preserved") ||
      !expect(reg.destination_kind ==
                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              "argument destination role mismatch") ||
      !expect(reg.storage_kind == prepare::PreparedMoveStorageKind::Register,
              "argument storage kind mismatch") ||
      !expect(reg.abi_index == 0, "argument ABI index mismatch") ||
      !expect(reg.argument_plan == &call_plan.arguments[0],
              "register argument plan was not matched") ||
      !expect(reg.abi_binding == &bundle.abi_bindings[0],
              "register argument binding was not matched") ||
      !expect(imm.argument_plan == &call_plan.arguments[1],
              "immediate argument plan without source value was not matched") ||
      !expect(byval.argument_plan == &call_plan.arguments[2],
              "byval lane fallback argument plan was not matched")) {
    return 1;
  }

  return 0;
}

int verify_result_and_non_call_classification() {
  const auto call_plan = sample_call_plan();
  const auto placement = sample_gpr_placement();
  const prepare::PreparedMoveResolution result_move{
      .from_value_id = 0,
      .to_value_id = 20,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution return_move{
      .from_value_id = 21,
      .to_value_id = 21,
      .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution value_move{
      .from_value_id = 22,
      .to_value_id = 23,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
      .destination_stack_offset_bytes = 32,
  };
  const prepare::PreparedMoveBundle bundle{
      .phase = prepare::PreparedMovePhase::AfterCall,
      .block_index = 3,
      .instruction_index = 9,
      .moves = {result_move, return_move, value_move},
      .abi_bindings = {binding_for(result_move)},
  };

  const auto result = prepare::classify_prepared_call_boundary_move(
      call_plan, bundle, bundle.moves[0]);
  const auto ret = prepare::classify_prepared_call_boundary_move(
      call_plan, bundle, bundle.moves[1]);
  const auto value = prepare::classify_prepared_call_boundary_move(
      call_plan, bundle, bundle.moves[2]);
  if (!expect(prepare::prepared_call_boundary_move_classification_available(result),
              "call result classification should be available") ||
      !expect(result.result_plan == &*call_plan.result,
              "result plan was not preserved") ||
      !expect(result.abi_binding == &bundle.abi_bindings[0],
              "result binding was not matched") ||
      !expect(prepare::prepared_call_boundary_move_classification_available(ret),
              "function return classification should not require a call binding") ||
      !expect(ret.destination_kind ==
                  prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
              "function return destination role mismatch") ||
      !expect(prepare::prepared_call_boundary_move_classification_available(value),
              "ordinary value classification should not require call authority") ||
      !expect(value.storage_kind == prepare::PreparedMoveStorageKind::StackSlot,
              "ordinary value storage kind was not preserved")) {
    return 1;
  }

  return 0;
}

int verify_missing_and_mismatched_statuses() {
  const auto call_plan = sample_call_plan();
  const auto placement = sample_gpr_placement();
  const prepare::PreparedMoveBundle empty_bundle{
      .phase = prepare::PreparedMovePhase::BeforeCall,
      .block_index = 3,
      .instruction_index = 9,
  };

  const prepare::PreparedMoveResolution missing_index{
      .from_value_id = 10,
      .to_value_id = 10,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution missing_argument{
      .from_value_id = 999,
      .to_value_id = 999,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_abi_index = 0,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution missing_binding{
      .from_value_id = 10,
      .to_value_id = 10,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_abi_index = 0,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution mismatched_result{
      .from_value_id = 0,
      .to_value_id = 777,
      .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .destination_register_name = "x0",
      .destination_register_placement = placement,
  };
  const prepare::PreparedMoveResolution unsupported_op{
      .from_value_id = 10,
      .to_value_id = 10,
      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
      .op_kind = prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp,
  };
  const prepare::PreparedCallPlan no_result_call_plan{
      .block_index = 3,
      .instruction_index = 9,
  };

  const auto no_index = prepare::classify_prepared_call_boundary_move(
      call_plan, empty_bundle, missing_index);
  const auto no_arg = prepare::classify_prepared_call_boundary_move(
      call_plan, empty_bundle, missing_argument);
  const auto no_binding = prepare::classify_prepared_call_boundary_move(
      call_plan, empty_bundle, missing_binding);
  const auto bad_result = prepare::classify_prepared_call_boundary_move(
      call_plan, empty_bundle, mismatched_result);
  const auto no_result = prepare::classify_prepared_call_boundary_move(
      no_result_call_plan, empty_bundle, mismatched_result);
  const auto temp_op = prepare::classify_prepared_call_boundary_move(
      call_plan, empty_bundle, unsupported_op);

  if (!expect(no_index.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::MissingAbiIndex,
              "missing argument ABI index status mismatch") ||
      !expect(no_arg.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::
                      MissingCallArgumentPlan,
              "missing argument plan status mismatch") ||
      !expect(no_binding.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::MissingAbiBinding,
              "missing argument binding status mismatch") ||
      !expect(no_binding.argument_plan == &call_plan.arguments[0],
              "missing binding should preserve matched argument plan") ||
      !expect(bad_result.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::
                      MismatchedCallResultPlan,
              "mismatched result plan status mismatch") ||
      !expect(bad_result.result_plan == &*call_plan.result,
              "mismatched result should preserve result plan authority") ||
      !expect(no_result.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::
                      MissingCallResultPlan,
              "missing result plan status mismatch") ||
      !expect(temp_op.status ==
                  prepare::PreparedCallBoundaryMoveClassificationStatus::UnsupportedOpKind,
              "unsupported op-kind status mismatch")) {
    return 1;
  }

  return 0;
}

}  // namespace

int main() {
  if (const int rc = verify_argument_classification(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_result_and_non_call_classification(); rc != 0) {
    return rc;
  }
  if (const int rc = verify_missing_and_mismatched_statuses(); rc != 0) {
    return rc;
  }
  return EXIT_SUCCESS;
}
