#include "src/backend/prealloc/module.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedControlFlowBlock return_block(c4c::BlockLabelId label) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::Return,
  };
}

prepare::PreparedControlFlowBlock branch_block(c4c::BlockLabelId label,
                                               c4c::BlockLabelId target) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = target,
  };
}

prepare::PreparedControlFlowBlock cond_branch_block(c4c::BlockLabelId label,
                                                    c4c::BlockLabelId true_label,
                                                    c4c::BlockLabelId false_label) {
  return prepare::PreparedControlFlowBlock{
      .block_label = label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
}

bool expect_same(const void* actual, const void* expected, std::string_view message) {
  if (actual != expected) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

int verify_linear_function_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_id = prepared.names.function_names.intern("linear");
  const auto other_function_id = prepared.names.function_names.intern("diamond");
  const auto entry_label = prepared.names.block_labels.intern("linear.entry");
  const auto exit_label = prepared.names.block_labels.intern("linear.exit");
  const auto value_name = prepared.names.value_names.intern("%linear.value");
  const auto other_value_name = prepared.names.value_names.intern("%diamond.value");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_id,
      .blocks = {
          branch_block(entry_label, exit_label),
          return_block(exit_label),
      },
  };

  prepare::PreparedCallPlansFunction calls{
      .function_name = function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 0,
              .instruction_index = 2,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("callee_a"),
          },
          prepare::PreparedCallPlan{
              .block_index = 1,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
              .direct_callee_name = std::string("callee_b"),
          },
      },
  };
  prepared.call_plans.functions.push_back(calls);
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = other_function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 0,
              .instruction_index = 2,
              .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
          },
      },
  });

  prepare::PreparedAddressingFunction addressing{
      .function_name = function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = entry_label,
              .inst_index = 1,
              .kind = prepare::PreparedAddressMaterializationKind::FrameSlot,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = entry_label,
              .inst_index = 3,
              .kind = prepare::PreparedAddressMaterializationKind::DirectGlobal,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = exit_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::Label,
          },
      },
  };
  prepared.addressing.functions.push_back(addressing);
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = other_function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = other_function_id,
              .block_label = entry_label,
              .inst_index = 9,
              .kind = prepare::PreparedAddressMaterializationKind::GotGlobal,
          },
      },
  });

  prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = function_id,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::Register,
              .register_name = std::string("r10"),
          },
          prepare::PreparedValueHome{
              .value_id = 8,
              .function_name = function_id,
              .value_name = c4c::kInvalidValueName,
              .kind = prepare::PreparedValueHomeKind::StackSlot,
              .slot_id = 1,
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BeforeInstruction,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 0,
              .instruction_index = 2,
          },
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::AfterCall,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 1,
              .instruction_index = 0,
          },
      },
  };
  prepared.value_locations.functions.push_back(value_locations);
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = other_function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 4,
              .function_name = other_function_id,
              .value_name = other_value_name,
              .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
              .immediate_i32 = 7,
          },
      },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* call_plans = prepare::find_prepared_call_plans(prepared, function_id);
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_id);
  if (call_plans == nullptr || function_locations == nullptr) {
    return fail("fixture lookup failed for linear function");
  }

  if (!expect_same(prepare::find_indexed_prepared_call_plan(
                       &lookups.call_plans, call_plans, 0, 2),
                   &call_plans->calls[0],
                   "indexed call-plan lookup missed the first linear call")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_call_plan(
                       &lookups.call_plans, call_plans, 1, 0),
                   &call_plans->calls[1],
                   "indexed call-plan lookup missed the second linear call")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_call_plan(&lookups.call_plans, call_plans, 0, 9) !=
      nullptr) {
    return fail("indexed call-plan lookup returned an unmatched linear call");
  }

  const auto* entry_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, entry_label);
  if (entry_materializations == nullptr || entry_materializations->size() != 2 ||
      (*entry_materializations)[0] != &prepared.addressing.functions[0].address_materializations[0] ||
      (*entry_materializations)[1] != &prepared.addressing.functions[0].address_materializations[1]) {
    return fail("indexed address-materialization lookup did not preserve block grouping");
  }
  const auto* exit_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, exit_label);
  if (exit_materializations == nullptr || exit_materializations->size() != 1 ||
      (*exit_materializations)[0] != &prepared.addressing.functions[0].address_materializations[2]) {
    return fail("indexed address-materialization lookup missed exit block materialization");
  }

  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BeforeInstruction,
                       0,
                       2),
                   &function_locations->move_bundles[0],
                   "indexed move-bundle lookup missed the before-instruction bundle")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::AfterCall,
                       1,
                       0),
                   &function_locations->move_bundles[1],
                   "indexed move-bundle lookup missed the after-call bundle")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_move_bundle(&lookups.move_bundles,
                                                 function_locations,
                                                 prepare::PreparedMovePhase::BeforeCall,
                                                 0,
                                                 2) != nullptr) {
    return fail("indexed move-bundle lookup ignored move phase");
  }

  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, function_locations, prepare::PreparedValueId{4}),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed the value-id home")) {
    return 1;
  }
  const auto value_id = prepare::find_indexed_prepared_value_id(
      &lookups.value_homes, nullptr, function_locations, value_name);
  if (!value_id.has_value() || *value_id != 4) {
    return fail("indexed value-id lookup missed the value-name mapping");
  }
  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, nullptr, function_locations, value_name),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed the value-name home")) {
    return 1;
  }
  if (prepare::find_indexed_prepared_value_id(
          &lookups.value_homes, nullptr, function_locations, other_value_name)
      .has_value()) {
    return fail("indexed value-id lookup leaked another function's value home");
  }

  return 0;
}

int verify_diamond_function_lookup() {
  prepare::PreparedBirModule prepared;
  const auto function_id = prepared.names.function_names.intern("diamond");
  const auto entry_label = prepared.names.block_labels.intern("diamond.entry");
  const auto left_label = prepared.names.block_labels.intern("diamond.left");
  const auto right_label = prepared.names.block_labels.intern("diamond.right");
  const auto join_label = prepared.names.block_labels.intern("diamond.join");
  const auto value_name = prepared.names.value_names.intern("%diamond.value");

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_id,
      .blocks = {
          cond_branch_block(entry_label, left_label, right_label),
          branch_block(left_label, join_label),
          branch_block(right_label, join_label),
          return_block(join_label),
      },
  };

  prepare::PreparedCallPlansFunction calls{
      .function_name = function_id,
      .calls = {
          prepare::PreparedCallPlan{
              .block_index = 1,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("left_call"),
          },
          prepare::PreparedCallPlan{
              .block_index = 2,
              .instruction_index = 0,
              .wrapper_kind = prepare::PreparedCallWrapperKind::SameModule,
              .direct_callee_name = std::string("right_call"),
          },
          prepare::PreparedCallPlan{
              .block_index = 3,
              .instruction_index = 1,
              .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternVariadic,
              .direct_callee_name = std::string("join_call"),
          },
      },
  };
  prepared.call_plans.functions.push_back(calls);

  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_id,
      .address_materializations = {
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = left_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::StringConstant,
          },
          prepare::PreparedAddressMaterialization{
              .function_name = function_id,
              .block_label = right_label,
              .inst_index = 0,
              .kind = prepare::PreparedAddressMaterializationKind::TlsGlobal,
          },
      },
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_id,
      .value_homes = {
          prepare::PreparedValueHome{
              .value_id = 11,
              .function_name = function_id,
              .value_name = value_name,
              .kind = prepare::PreparedValueHomeKind::PointerBasePlusOffset,
              .pointer_byte_delta = 16,
          },
      },
      .move_bundles = {
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BlockEntry,
              .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
              .block_index = 3,
              .instruction_index = 0,
              .source_parallel_copy_predecessor_label = left_label,
              .source_parallel_copy_successor_label = join_label,
          },
          prepare::PreparedMoveBundle{
              .function_name = function_id,
              .phase = prepare::PreparedMovePhase::BeforeCall,
              .authority_kind = prepare::PreparedMoveAuthorityKind::None,
              .block_index = 3,
              .instruction_index = 1,
          },
      },
  });

  const auto lookups = prepare::make_prepared_function_lookups(prepared, control_flow);
  const auto* call_plans = prepare::find_prepared_call_plans(prepared, function_id);
  const auto* function_locations =
      prepare::find_prepared_value_location_function(prepared, function_id);
  if (call_plans == nullptr || function_locations == nullptr) {
    return fail("fixture lookup failed for diamond function");
  }

  for (std::size_t index = 0; index < call_plans->calls.size(); ++index) {
    const auto& call = call_plans->calls[index];
    if (!expect_same(prepare::find_indexed_prepared_call_plan(
                         &lookups.call_plans,
                         call_plans,
                         call.block_index,
                         call.instruction_index),
                     &call_plans->calls[index],
                     "indexed call-plan lookup missed a diamond call")) {
      return 1;
    }
  }

  const auto* left_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, left_label);
  const auto* right_materializations =
      prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, right_label);
  if (left_materializations == nullptr || left_materializations->size() != 1 ||
      (*left_materializations)[0] != &prepared.addressing.functions[0].address_materializations[0]) {
    return fail("indexed address-materialization lookup missed diamond left block");
  }
  if (right_materializations == nullptr || right_materializations->size() != 1 ||
      (*right_materializations)[0] != &prepared.addressing.functions[0].address_materializations[1]) {
    return fail("indexed address-materialization lookup missed diamond right block");
  }
  if (prepare::find_indexed_prepared_address_materializations(
          &lookups.address_materializations, join_label) != nullptr) {
    return fail("indexed address-materialization lookup returned an empty diamond join block");
  }

  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BlockEntry,
                       3,
                       0),
                   &function_locations->move_bundles[0],
                   "indexed move-bundle lookup missed diamond block-entry bundle")) {
    return 1;
  }
  if (!expect_same(prepare::find_indexed_prepared_move_bundle(
                       &lookups.move_bundles,
                       function_locations,
                       prepare::PreparedMovePhase::BeforeCall,
                       3,
                       1),
                   &function_locations->move_bundles[1],
                   "indexed move-bundle lookup missed diamond before-call bundle")) {
    return 1;
  }

  if (!expect_same(prepare::find_indexed_prepared_value_home(
                       &lookups.value_homes, function_locations, prepare::PreparedValueId{11}),
                   &function_locations->value_homes[0],
                   "indexed value-home lookup missed diamond home")) {
    return 1;
  }
  const auto value_id = prepare::find_indexed_prepared_value_id(
      &lookups.value_homes, nullptr, function_locations, value_name);
  if (!value_id.has_value() || *value_id != 11) {
    return fail("indexed value-id lookup missed diamond value-name mapping");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int result = verify_linear_function_lookup(); result != 0) {
    return result;
  }
  if (const int result = verify_diamond_function_lookup(); result != 0) {
    return result;
  }
  return EXIT_SUCCESS;
}
