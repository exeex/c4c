#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <utility>
#include <variant>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_with_return_block() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("return.fn");
  const auto entry_label = prepared.names.block_labels.intern("return.entry");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_immediate_return_value() {
  prepare::PreparedBirModule prepared = prepared_with_return_block();

  const auto function_link_name = prepared.module.names.link_names.intern("return.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("return.entry");

  bir::Block entry;
  entry.label = "return.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(7)};

  bir::Function function;
  function.name = "return.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));
  return prepared;
}

prepare::PreparedBirModule prepared_with_named_rematerialized_return_value() {
  prepare::PreparedBirModule prepared = prepared_with_return_block();

  const auto function_name = prepared.control_flow.functions.front().function_name;
  const auto value_name = prepared.names.value_names.intern("%answer");
  const auto function_link_name = prepared.module.names.link_names.intern("return.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("return.entry");

  bir::Block entry;
  entry.label = "return.entry";
  entry.label_id = bir_entry_label;
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%answer")};

  bir::Function function;
  function.name = "return.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{4},
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
          .immediate_i32 = 9,
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{4},
          .value_name = value_name,
          .encoding = prepare::PreparedStorageEncodingKind::Immediate,
          .immediate_i32 = 9,
      }},
  });
  return prepared;
}

prepare::PreparedRegisterPlacement call_result_gpr(std::size_t slot_index) {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallResult,
      .slot_index = slot_index,
      .contiguous_width = 1,
  };
}

prepare::PreparedValueHome register_home(prepare::PreparedValueId value_id,
                                         c4c::FunctionNameId function_name,
                                         c4c::ValueNameId value_name,
                                         const char* register_name) {
  return prepare::PreparedValueHome{
      .value_id = value_id,
      .function_name = function_name,
      .value_name = value_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = register_name,
  };
}

prepare::PreparedStoragePlanValue register_storage(prepare::PreparedValueId value_id,
                                                   c4c::ValueNameId value_name,
                                                   const char* register_name) {
  return prepare::PreparedStoragePlanValue{
      .value_id = value_id,
      .value_name = value_name,
      .encoding = prepare::PreparedStorageEncodingKind::Register,
      .bank = prepare::PreparedRegisterBank::Gpr,
      .contiguous_width = 1,
      .register_name = register_name,
      .occupied_register_names = {register_name},
  };
}

prepare::PreparedSavedRegister prepared_saved_x19(std::size_t offset_bytes) {
  return prepare::PreparedSavedRegister{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .register_name = "x19",
      .contiguous_width = 1,
      .occupied_register_names = {"x19"},
      .save_index = 0,
      .placement = prepare::PreparedRegisterPlacement{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
          .slot_index = 0,
          .contiguous_width = 1,
      },
      .slot_placement =
          prepare::PreparedSavedRegisterSlotPlacement{
              .bank = prepare::PreparedRegisterBank::Gpr,
              .register_name = "x19",
              .contiguous_width = 1,
              .occupied_register_names = {"x19"},
              .save_index = 0,
              .register_placement = prepare::PreparedRegisterPlacement{
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                  .slot_index = 0,
                  .contiguous_width = 1,
              },
              .slot_id = prepare::PreparedFrameSlotId{19},
              .stack_offset_bytes = offset_bytes,
              .size_bytes = std::size_t{8},
              .align_bytes = std::size_t{8},
              .fixed_location = true,
          },
  };
}

prepare::PreparedBirModule prepared_with_return_selected_scalar_value() {
  prepare::PreparedBirModule prepared = prepared_with_return_block();

  const auto function_name = prepared.control_flow.functions.front().function_name;
  const auto result_name = prepared.names.value_names.intern("%sum");
  const auto function_link_name = prepared.module.names.link_names.intern("return.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("return.entry");

  bir::Block entry;
  entry.label = "return.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%sum")};

  bir::Function function;
  function.name = "return.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{5},
          .function_name = function_name,
          .value_name = result_name,
          .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
          .immediate_i32 = 5,
      }},
      .move_bundles = {prepare::PreparedMoveBundle{
          .function_name = function_name,
          .phase = prepare::PreparedMovePhase::BeforeReturn,
          .block_index = 0,
          .instruction_index = 1,
          .moves = {prepare::PreparedMoveResolution{
              .from_value_id = prepare::PreparedValueId{5},
              .to_value_id = prepare::PreparedValueId{5},
              .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_contiguous_width = 1,
              .destination_register_placement = call_result_gpr(0),
          }},
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{5},
          .value_name = result_name,
          .encoding = prepare::PreparedStorageEncodingKind::Immediate,
          .immediate_i32 = 5,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_return_selected_register_scalar_value() {
  prepare::PreparedBirModule prepared = prepared_with_return_block();

  const auto function_name = prepared.control_flow.functions.front().function_name;
  const auto result_name = prepared.names.value_names.intern("%sum");
  const auto function_link_name = prepared.module.names.link_names.intern("return.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("return.entry");

  bir::Block entry;
  entry.label = "return.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%sum")};

  bir::Function function;
  function.name = "return.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {register_home(prepare::PreparedValueId{7},
                                    function_name,
                                    result_name,
                                    "x19")},
      .move_bundles = {prepare::PreparedMoveBundle{
          .function_name = function_name,
          .phase = prepare::PreparedMovePhase::BeforeReturn,
          .block_index = 0,
          .instruction_index = 1,
          .moves = {prepare::PreparedMoveResolution{
              .from_value_id = prepare::PreparedValueId{7},
              .to_value_id = prepare::PreparedValueId{7},
              .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_contiguous_width = 1,
              .destination_register_placement = call_result_gpr(0),
          }},
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {register_storage(prepare::PreparedValueId{7}, result_name, "x19")},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_void_direct_call_then_return() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("return.nonleaf");
  const auto entry_label = prepared.names.block_labels.intern("return.nonleaf.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("return.nonleaf.entry");
  const auto callee_link = prepared.names.link_names.intern("callee");

  prepared.module.functions.push_back(bir::Function{
      .name = "return.nonleaf",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "return.nonleaf.entry",
          .insts = {bir::CallInst{
              .callee = "callee",
              .callee_link_name_id = callee_link,
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_entry_label,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"callee"},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_saved_register_direct_call_then_return() {
  auto prepared = prepared_with_void_direct_call_then_return();
  const auto function_name = prepared.control_flow.functions.front().function_name;
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 0,
      .frame_alignment_bytes = 16,
      .saved_callee_registers = {prepared_saved_x19(0)},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_return_selected_memory_load_value() {
  prepare::PreparedBirModule prepared = prepared_with_return_block();

  const auto function_name = prepared.control_flow.functions.front().function_name;
  const auto loaded_name = prepared.names.value_names.intern("%loaded");
  const auto function_link_name = prepared.module.names.link_names.intern("return.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("return.entry");

  prepared.stack_layout = prepare::PreparedStackLayout{
      .frame_slots =
          {prepare::PreparedFrameSlot{
              .slot_id = prepare::PreparedFrameSlotId{20},
              .object_id = prepare::PreparedObjectId{20},
              .function_name = function_name,
              .offset_bytes = 0,
              .size_bytes = 4,
              .align_bytes = 4,
              .fixed_location = true,
          }},
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 16,
  };

  bir::Block entry;
  entry.label = "return.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%loaded"),
      .slot_id = c4c::SlotNameId{5},
      .byte_offset = 0,
      .align_bytes = 4,
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .byte_offset = 0,
              .size_bytes = 4,
              .align_bytes = 4,
              .base_slot_id = c4c::SlotNameId{5},
          },
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%loaded")};

  bir::Function function;
  function.name = "return.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {register_home(prepare::PreparedValueId{8},
                                    function_name,
                                    loaded_name,
                                    "w19")},
      .move_bundles = {prepare::PreparedMoveBundle{
          .function_name = function_name,
          .phase = prepare::PreparedMovePhase::BeforeReturn,
          .block_index = 0,
          .instruction_index = 1,
          .moves = {prepare::PreparedMoveResolution{
              .from_value_id = prepare::PreparedValueId{8},
              .to_value_id = prepare::PreparedValueId{8},
              .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_contiguous_width = 1,
              .destination_register_placement = call_result_gpr(0),
          }},
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {register_storage(prepare::PreparedValueId{8}, loaded_name, "w19")},
  });
  prepared.addressing.functions.push_back(prepare::PreparedAddressingFunction{
      .function_name = function_name,
      .frame_size_bytes = 16,
      .frame_alignment_bytes = 16,
      .accesses =
          {prepare::PreparedMemoryAccess{
              .function_name = function_name,
              .block_label = prepared.control_flow.functions.front().blocks.front().block_label,
              .inst_index = 0,
              .result_value_name = loaded_name,
              .address =
                  prepare::PreparedAddress{
                      .base_kind = prepare::PreparedAddressBaseKind::FrameSlot,
                      .frame_slot_id = prepare::PreparedFrameSlotId{20},
                      .byte_offset = 0,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .can_use_base_plus_offset = true,
                  },
          }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_return_selected_scalar_chain() {
  prepare::PreparedBirModule prepared = prepared_with_return_block();

  const auto function_name = prepared.control_flow.functions.front().function_name;
  const auto t0_name = prepared.names.value_names.intern("%t0");
  const auto t1_name = prepared.names.value_names.intern("%t1");
  const auto function_link_name = prepared.module.names.link_names.intern("return.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("return.entry");

  bir::Block entry;
  entry.label = "return.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "%t1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t0"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator =
      bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "%t1")};

  bir::Function function;
  function.name = "return.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::I32;
  function.blocks.push_back(std::move(entry));
  prepared.module.functions.push_back(std::move(function));

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{5},
                  .function_name = function_name,
                  .value_name = t0_name,
                  .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
                  .immediate_i32 = 5,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{6},
                  .function_name = function_name,
                  .value_name = t1_name,
                  .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
                  .immediate_i32 = 4,
              },
          },
      .move_bundles =
          {
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BeforeInstruction,
                  .block_index = 0,
                  .instruction_index = 1,
                  .moves = {prepare::PreparedMoveResolution{
                      .from_value_id = prepare::PreparedValueId{5},
                      .to_value_id = prepare::PreparedValueId{6},
                      .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                      .destination_contiguous_width = 1,
                  }},
              },
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BeforeReturn,
                  .block_index = 0,
                  .instruction_index = 2,
                  .moves = {prepare::PreparedMoveResolution{
                      .from_value_id = prepare::PreparedValueId{6},
                      .to_value_id = prepare::PreparedValueId{6},
                      .destination_kind =
                          prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                      .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                      .destination_contiguous_width = 1,
                      .destination_register_placement = call_result_gpr(0),
                  }},
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{5},
                  .value_name = t0_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Immediate,
                  .immediate_i32 = 5,
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{6},
                  .value_name = t1_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Immediate,
                  .immediate_i32 = 4,
              },
          },
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_conditional_branch_block() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("branch.fn");
  const auto entry_label = prepared.names.block_labels.intern("branch.entry");
  const auto then_label = prepared.names.block_labels.intern("branch.then");
  const auto else_label = prepared.names.block_labels.intern("branch.else");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  return prepared;
}

int direct_dispatch_lowers_prepared_return_to_canonical_machine_instruction() {
  auto prepared = prepared_with_return_block();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 7);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected direct return dispatch to emit exactly one instruction");
  }
  if (!block.successors.empty()) {
    return fail("expected direct return dispatch to record no block successors");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected return instruction origin to preserve prepared identity");
  }
  if (instruction.target.family !=
          aarch64_module::codegen::InstructionFamily::Return ||
      instruction.target.surface !=
          aarch64_module::codegen::RecordSurfaceKind::MachineInstructionNode ||
      instruction.target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 7 ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          instruction.target.payload)) {
    return fail("expected return instruction target to be canonical selected return record");
  }
  return 0;
}

int module_build_lowers_prepared_return_without_flat_compatibility_nodes() {
  auto prepared = prepared_with_return_block();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared return module to build");
  }

  const auto& module = *result.module;
  if (module.mir.functions.size() != 1 || module.mir.functions.front().blocks.size() != 1 ||
      module.mir.functions.front().blocks.front().instructions.size() != 1 ||
      !module.mir.functions.front().blocks.front().successors.empty()) {
    return fail("expected module build to lower one prepared return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          module.mir.functions.front().blocks.front().instructions.front().target.payload)) {
    return fail("expected module MIR to carry return instruction target record");
  }
  if (module.functions.size() != 1 ||
      module.functions.front().mir.blocks.front().instructions.size() != 1 ||
      !module.functions.front().machine_nodes.empty() ||
      !module.compatibility.functions.front().machine_nodes.empty()) {
    return fail("expected compatibility projection to mirror MIR but not fake machine nodes");
  }
  return 0;
}

int direct_dispatch_attaches_immediate_return_value() {
  auto prepared = prepared_with_immediate_return_value();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);
  if (result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected immediate return value dispatch to emit one selected return");
  }
  const auto* ret = std::get_if<aarch64_module::codegen::ReturnInstructionRecord>(
      &block.instructions.front().target.payload);
  if (ret == nullptr || !ret->value.has_value() ||
      ret->value_type != bir::TypeKind::I32) {
    return fail("expected immediate return to carry typed return payload");
  }
  const auto* immediate =
      std::get_if<aarch64_module::codegen::ImmediateOperand>(&ret->value->payload);
  if (ret->value->kind != aarch64_module::codegen::OperandKind::Immediate ||
      immediate == nullptr || immediate->signed_value != 7 ||
      immediate->type != bir::TypeKind::I32) {
    return fail("expected immediate return payload to preserve integer value");
  }
  return 0;
}

int module_build_attaches_named_rematerialized_return_value() {
  auto prepared = prepared_with_named_rematerialized_return_value();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected named rematerialized return module to build");
  }
  const auto& instruction =
      result.module->mir.functions.front().blocks.front().instructions.front();
  const auto* ret =
      std::get_if<aarch64_module::codegen::ReturnInstructionRecord>(&instruction.target.payload);
  if (ret == nullptr || !ret->value.has_value()) {
    return fail("expected named rematerialized return to carry a value");
  }
  const auto* immediate =
      std::get_if<aarch64_module::codegen::ImmediateOperand>(&ret->value->payload);
  if (immediate == nullptr || immediate->signed_value != 9 ||
      immediate->source_value_id != prepare::PreparedValueId{4}) {
    return fail("expected named rematerialized return to resolve through prepared value authority");
  }
  return 0;
}

int module_build_selects_scalar_result_before_return() {
  auto prepared = prepared_with_return_selected_scalar_value();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected return-selected scalar module to build");
  }
  const auto& instructions = result.module->mir.functions.front().blocks.front().instructions;
  if (instructions.size() != 2) {
    return fail("expected scalar result and return instructions");
  }
  if (instructions[0].target.family != aarch64_module::codegen::InstructionFamily::Scalar ||
      instructions[1].target.family != aarch64_module::codegen::InstructionFamily::Return) {
    return fail("expected scalar instruction to precede return instruction");
  }
  const auto* ret =
      std::get_if<aarch64_module::codegen::ReturnInstructionRecord>(
          &instructions[1].target.payload);
  if (ret == nullptr || !ret->value.has_value()) {
    return fail("expected scalar result return to carry a value");
  }
  const auto* reg =
      std::get_if<aarch64_module::codegen::RegisterOperand>(&ret->value->payload);
  if (reg == nullptr || reg->value_id != prepare::PreparedValueId{5} ||
      reg->reg != aarch64_abi::w_register(0) ||
      result.module->functions.front().machine_nodes.size() != 1 ||
      result.module->functions.front().machine_nodes.front().family !=
          aarch64_module::codegen::InstructionFamily::Scalar) {
    return fail("expected scalar return to attach emitted result register and selected scalar node");
  }
  return 0;
}

int module_build_retains_return_abi_for_register_scalar_result() {
  auto prepared = prepared_with_return_selected_register_scalar_value();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected register-backed return-selected scalar module to build");
  }
  const auto& instructions = result.module->mir.functions.front().blocks.front().instructions;
  if (instructions.size() != 2 ||
      instructions[0].target.family != aarch64_module::codegen::InstructionFamily::Scalar ||
      instructions[1].target.family != aarch64_module::codegen::InstructionFamily::Return) {
    return fail("expected scalar result and return instructions for register-backed value");
  }
  const auto* scalar = std::get_if<aarch64_module::codegen::ScalarInstructionRecord>(
      &instructions[0].target.payload);
  const auto* ret = std::get_if<aarch64_module::codegen::ReturnInstructionRecord>(
      &instructions[1].target.payload);
  const auto* ret_reg =
      ret != nullptr && ret->value.has_value()
          ? std::get_if<aarch64_module::codegen::RegisterOperand>(&ret->value->payload)
          : nullptr;
  if (scalar == nullptr || ret_reg == nullptr ||
      !scalar->result_register.has_value() ||
      scalar->result_register->reg != aarch64_abi::w_register(0) ||
      ret_reg->reg != aarch64_abi::w_register(0) ||
      ret_reg->value_id != prepare::PreparedValueId{7}) {
    return fail("expected register-backed scalar return to honor FunctionReturnAbi w0");
  }
  return 0;
}

int module_build_preserves_link_register_around_non_leaf_return() {
  auto prepared = prepared_with_saved_register_direct_call_then_return();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected non-leaf prepared return module to build");
  }

  const auto& instructions =
      result.module->mir.functions.front().blocks.front().instructions;
  if (instructions.size() != 4) {
    return fail("expected non-leaf return block to contain LR frame, call, and return");
  }
  const auto* setup =
      std::get_if<aarch64_codegen::FrameInstructionRecord>(&instructions[0].target.payload);
  const auto* call =
      std::get_if<aarch64_codegen::CallInstructionRecord>(&instructions[1].target.payload);
  const auto* teardown =
      std::get_if<aarch64_codegen::FrameInstructionRecord>(&instructions[2].target.payload);
  const auto* ret =
      std::get_if<aarch64_codegen::ReturnInstructionRecord>(&instructions[3].target.payload);
  if (setup == nullptr || call == nullptr || teardown == nullptr || ret == nullptr ||
      setup->frame_kind != aarch64_codegen::FrameInstructionKind::PrologueSetup ||
      teardown->frame_kind != aarch64_codegen::FrameInstructionKind::EpilogueTeardown ||
      !setup->preserves_link_register || !teardown->preserves_link_register ||
      setup->link_register_save_offset_bytes != std::optional<std::size_t>{8} ||
      teardown->link_register_save_offset_bytes != std::optional<std::size_t>{8} ||
      setup->frame_size_bytes != 32 || teardown->frame_size_bytes != 32 ||
      setup->saved_callee_registers.size() != 1 ||
      instructions[0].target.uses.size() != 2 ||
      instructions[0].target.uses.front().reg != aarch64_abi::link_register() ||
      instructions[0].target.uses[1].reg != aarch64_abi::x_register(19) ||
      instructions[2].target.defs.size() != 2 ||
      instructions[2].target.defs.front().reg != aarch64_abi::link_register() ||
      instructions[2].target.defs[1].reg != aarch64_abi::x_register(19)) {
    return fail("expected non-leaf return lowering to preserve LR with saved registers");
  }
  return 0;
}

int module_build_retains_return_abi_for_memory_load_result() {
  auto prepared = prepared_with_return_selected_memory_load_value();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);
  if (!diagnostics.empty() || result.visited_operations != 1 ||
      result.emitted_instructions != 2 || block.instructions.size() != 2) {
    return fail("expected memory-load return dispatch to select load plus return");
  }
  const auto& instructions = block.instructions;
  if (instructions.size() != 2 ||
      instructions[0].target.family != aarch64_module::codegen::InstructionFamily::Memory ||
      instructions[1].target.family != aarch64_module::codegen::InstructionFamily::Return) {
    return fail("expected memory load and return instructions for loaded return value");
  }
  const auto* memory = std::get_if<aarch64_module::codegen::MemoryInstructionRecord>(
      &instructions[0].target.payload);
  const auto* ret = std::get_if<aarch64_module::codegen::ReturnInstructionRecord>(
      &instructions[1].target.payload);
  const auto* ret_reg =
      ret != nullptr && ret->value.has_value()
          ? std::get_if<aarch64_module::codegen::RegisterOperand>(&ret->value->payload)
          : nullptr;
  if (memory == nullptr || !memory->result_register.has_value() ||
      memory->result_register->reg != aarch64_abi::w_register(0) ||
      memory->result_register->role !=
          aarch64_module::codegen::RegisterOperandRole::CallAbi ||
      ret_reg == nullptr || ret_reg->reg != aarch64_abi::w_register(0) ||
      ret_reg->value_id != prepare::PreparedValueId{8}) {
    return fail("expected memory-loaded return value to honor FunctionReturnAbi w0");
  }
  return 0;
}

int module_build_selects_scalar_chain_before_return() {
  auto prepared = prepared_with_return_selected_scalar_chain();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected return-selected scalar chain module to build");
  }
  const auto& instructions = result.module->mir.functions.front().blocks.front().instructions;
  if (instructions.size() != 3 ||
      instructions[0].target.family != aarch64_module::codegen::InstructionFamily::Scalar ||
      instructions[1].target.family != aarch64_module::codegen::InstructionFamily::Scalar ||
      instructions[2].target.family != aarch64_module::codegen::InstructionFamily::Return) {
    return fail("expected two scalar chain instructions to precede return instruction");
  }

  const auto* first = std::get_if<aarch64_module::codegen::ScalarInstructionRecord>(
      &instructions[0].target.payload);
  const auto* second = std::get_if<aarch64_module::codegen::ScalarInstructionRecord>(
      &instructions[1].target.payload);
  const auto* ret =
      std::get_if<aarch64_module::codegen::ReturnInstructionRecord>(
          &instructions[2].target.payload);
  if (first == nullptr || second == nullptr || ret == nullptr ||
      !first->result_register.has_value() || !second->result_register.has_value() ||
      first->result_register->reg != aarch64_abi::w_register(0) ||
      second->result_register->reg != aarch64_abi::w_register(0) ||
      second->source_binary_opcode != bir::BinaryOpcode::Sub ||
      second->inputs.size() != 2) {
    return fail("expected add/sub chain to use the return ABI accumulator register");
  }
  const auto* second_lhs =
      std::get_if<aarch64_module::codegen::RegisterOperand>(&second->inputs[0].payload);
  const auto* second_rhs =
      std::get_if<aarch64_module::codegen::ImmediateOperand>(&second->inputs[1].payload);
  const auto* ret_reg =
      ret->value.has_value()
          ? std::get_if<aarch64_module::codegen::RegisterOperand>(&ret->value->payload)
          : nullptr;
  if (second_lhs == nullptr || second_lhs->value_id != prepare::PreparedValueId{5} ||
      second_lhs->reg != aarch64_abi::w_register(0) || second_rhs == nullptr ||
      second_rhs->signed_value != 1 || ret_reg == nullptr ||
      ret_reg->value_id != prepare::PreparedValueId{6} ||
      ret_reg->reg != aarch64_abi::w_register(0) ||
      result.module->functions.front().machine_nodes.size() != 2) {
    return fail("expected chain operands and return to reference selected scalar records");
  }
  return 0;
}

int unsupported_conditional_branch_terminator_stays_diagnostic_only() {
  auto prepared = prepared_with_conditional_branch_block();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty() ||
      !block.successors.empty()) {
    return fail("expected unsupported conditional branch terminator to emit no machine instruction");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().function_name != function_cf.function_name ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected unsupported conditional branch to remain a typed diagnostic");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status =
          direct_dispatch_lowers_prepared_return_to_canonical_machine_instruction();
      status != 0) {
    return status;
  }
  if (const int status =
          module_build_lowers_prepared_return_without_flat_compatibility_nodes();
      status != 0) {
    return status;
  }
  if (const int status = direct_dispatch_attaches_immediate_return_value();
      status != 0) {
    return status;
  }
  if (const int status = module_build_attaches_named_rematerialized_return_value();
      status != 0) {
    return status;
  }
  if (const int status = module_build_selects_scalar_result_before_return();
      status != 0) {
    return status;
  }
  if (const int status = module_build_retains_return_abi_for_register_scalar_result();
      status != 0) {
    return status;
  }
  if (const int status = module_build_preserves_link_register_around_non_leaf_return();
      status != 0) {
    return status;
  }
  if (const int status = module_build_retains_return_abi_for_memory_load_result();
      status != 0) {
    return status;
  }
  if (const int status = module_build_selects_scalar_chain_before_return();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_conditional_branch_terminator_stays_diagnostic_only();
      status != 0) {
    return status;
  }
  return 0;
}
