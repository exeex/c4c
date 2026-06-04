#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/calls.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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

bir::CallArgAbiInfo byval_gpr_abi(std::size_t size_bytes, std::size_t align_bytes) {
  bir::CallArgAbiInfo abi{
      .type = bir::TypeKind::Ptr,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_in_register = true,
      .byval_copy = true,
  };
  return abi;
}

prepare::PreparedAggregateTransportPlan byval_register_lanes_transport(
    prepare::PreparedFrameSlotId source_slot_id,
    std::size_t source_offset_bytes,
    std::size_t size_bytes,
    std::size_t align_bytes,
    std::vector<std::string> destination_register_names,
    std::size_t destination_contiguous_width) {
  prepare::PreparedAggregateTransportPlan transport{
      .kind = prepare::PreparedAggregateTransportKind::ByvalRegisterLanes,
      .payload_size_bytes = size_bytes,
      .payload_align_bytes = align_bytes,
      .copy_size_bytes = size_bytes,
      .copy_align_bytes = align_bytes,
      .source_slot_id = source_slot_id,
      .source_stack_offset_bytes = source_offset_bytes,
      .chunks = {},
      .lanes = {},
      .scratch_requirements =
          {prepare::PreparedAggregateTransportScratchRequirement{
              .kind = prepare::PreparedAggregateTransportScratchKind::GeneralPurpose,
              .width_bytes = 8,
          }},
  };
  std::size_t payload_offset = 0;
  for (std::size_t lane_index = 0;
       lane_index < destination_register_names.size() && payload_offset < size_bytes;
       ++lane_index) {
    const std::size_t remaining_bytes = size_bytes - payload_offset;
    const std::size_t lane_size = remaining_bytes < 8 ? remaining_bytes : 8;
    transport.chunks.push_back(prepare::PreparedAggregateTransportChunk{
        .chunk_index = lane_index,
        .kind = prepare::PreparedAggregateTransportChunkKind::RequiredPayload,
        .payload_offset_bytes = payload_offset,
        .source_offset_bytes = source_offset_bytes + payload_offset,
        .destination_offset_bytes = payload_offset,
        .size_bytes = lane_size,
        .align_bytes = align_bytes < lane_size ? align_bytes : lane_size,
        .preferred_width_bytes = lane_size,
    });
    transport.lanes.push_back(prepare::PreparedAggregateTransportLane{
        .lane_index = lane_index,
        .chunk_index = lane_index,
        .lane_payload_offset_bytes = payload_offset,
        .source_offset_bytes = source_offset_bytes + payload_offset,
        .destination_offset_bytes = payload_offset,
        .lane_size_bytes = lane_size,
        .destination_register_name = destination_register_names[lane_index],
        .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
        .destination_contiguous_width = destination_contiguous_width,
        .destination_occupied_register_names = destination_register_names,
        .whole_register = lane_size == 8,
    });
    payload_offset += lane_size;
  }
  return transport;
}

int byval_caller_publishes_composite_gpr_lanes_not_object_pointer() {
  constexpr auto function_name = c4c::FunctionNameId{4101};
  constexpr auto block_label = c4c::BlockLabelId{4102};
  constexpr auto aggregate_value_id = prepare::PreparedValueId{4103};
  constexpr auto aggregate_value_name = c4c::ValueNameId{4104};
  constexpr auto aggregate_slot_id = prepare::PreparedFrameSlotId{4105};

  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = aggregate_value_id,
          .function_name = function_name,
          .value_name = aggregate_value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"x20"},
          .size_bytes = std::size_t{16},
          .align_bytes = std::size_t{8},
      }},
      .move_bundles = {prepare::PreparedMoveBundle{
          .function_name = function_name,
          .phase = prepare::PreparedMovePhase::BeforeCall,
          .block_index = 0,
          .instruction_index = 0,
          .moves = {prepare::PreparedMoveResolution{
              .from_value_id = aggregate_value_id,
              .to_value_id = aggregate_value_id,
              .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .destination_register_name = std::string{"x0"},
              .destination_contiguous_width = 2,
              .destination_occupied_register_names = {"x0", "x1"},
              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              .reason = "call_arg_byval_aggregate_register_lanes",
          }},
      }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 0,
      .arguments = {prepare::PreparedCallArgumentPlan{
          .instruction_index = 0,
          .arg_index = 0,
          .value_bank = prepare::PreparedRegisterBank::Gpr,
          .source_encoding = prepare::PreparedStorageEncodingKind::Register,
          .source_value_id = aggregate_value_id,
          .source_register_name = std::string{"x20"},
          .source_register_bank = prepare::PreparedRegisterBank::AggregateAddress,
          .destination_register_name = std::string{"x0"},
          .destination_contiguous_width = 2,
          .destination_occupied_register_names = {"x0", "x1"},
          .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
          .source_selection =
              prepare::PreparedCallArgumentSourceSelection{
                  .kind =
                      prepare::PreparedCallArgumentSourceSelectionKind::
                          ByvalRegisterLane,
                  .source_value_id = aggregate_value_id,
                  .source_value_name = aggregate_value_name,
                  .source_home_kind = prepare::PreparedValueHomeKind::Register,
                  .source_slot_id = aggregate_slot_id,
                  .source_stack_offset_bytes = std::size_t{128},
                  .source_size_bytes = std::size_t{16},
                  .source_align_bytes = std::size_t{8},
                  .byval_lane_extent_bytes = std::size_t{16},
              },
          .aggregate_transport = byval_register_lanes_transport(
              aggregate_slot_id,
              std::size_t{128},
              std::size_t{16},
              std::size_t{8},
              {"x0", "x1"},
              2),
      }},
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered =
      aarch64_codegen::lower_before_call_moves(block_context, call_plan, 0, diagnostics);
  if (!diagnostics.empty() || lowered.size() != 1) {
    return fail("expected byval caller publication to lower one call-boundary move");
  }
  const auto* move = std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(
      &lowered.front().target.payload);
  if (move == nullptr || move->source_memory_materializes_address ||
      move->source_register.has_value() || !move->source_memory.has_value() ||
      !move->destination_register.has_value() ||
      move->destination_register->reg != aarch64_abi::x_register(0) ||
      move->move.reason != "call_arg_byval_aggregate_register_lanes") {
    return fail("expected byval caller to publish aggregate payload lanes, not forward the object pointer in x20");
  }
  return 0;
}

prepare::PreparedBirModule prepared_with_byval_entry_formal() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("dispatch.boundary.byval.entry");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.boundary.byval.entry.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.boundary.byval.entry.entry");
  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.boundary.byval.entry",
      .return_type = bir::TypeKind::Void,
      .params = {bir::Param{
          .type = bir::TypeKind::Ptr,
          .name = "%p.f",
          .size_bytes = 16,
          .align_bytes = 8,
          .abi = byval_gpr_abi(16, 8),
          .is_byval = true,
      }},
      .blocks = {bir::Block{
          .label = "dispatch.boundary.byval.entry.entry",
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
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{4201},
          .function_name = function_name,
          .value_name = prepared.names.value_names.intern("%p.f"),
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .offset_bytes = std::size_t{0},
          .size_bytes = std::size_t{16},
          .align_bytes = std::size_t{8},
      }},
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 48,
      .frame_alignment_bytes = 16,
  });
  return prepared;
}

int byval_callee_entry_consumes_byval_frame_slot() {
  auto prepared = prepared_with_byval_entry_formal();
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
  if (!diagnostics.empty() || !result.visited_terminator || block.instructions.size() < 2) {
    return fail("expected byval callee entry to publish fixed byval formal before return");
  }
  const auto* publication = std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
      &block.instructions.front().target.payload);
  if (publication == nullptr || !publication->has_inline_asm_payload ||
      publication->inline_asm_template.find("str x0, [sp]") == std::string::npos ||
      publication->inline_asm_template.find("str x1, [sp, #8]") == std::string::npos) {
    return fail("expected byval callee entry to consume ABI GPR lanes into the byval frame slot");
  }
  return 0;
}

prepare::PreparedBirModule prepared_with_f128_hfa_call_boundary() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("dispatch.boundary.f128.hfa");
  const auto entry_label =
      prepared.names.block_labels.intern("dispatch.boundary.f128.hfa.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.boundary.f128.hfa.entry");
  const auto actual_link = prepared.names.link_names.intern("consume_hfa");
  const auto lane_name = prepared.names.value_names.intern("%hfa.f128.0");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.boundary.f128.hfa",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.boundary.f128.hfa.entry",
          .insts = {bir::CallInst{
              .callee = "consume_hfa",
              .callee_link_name_id = actual_link,
              .args = {bir::Value::named(bir::TypeKind::F128, "%hfa.f128.0")},
              .arg_types = {bir::TypeKind::F128},
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
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{4301},
          .function_name = function_name,
          .value_name = lane_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"d13"},
      }},
      .move_bundles = {prepare::PreparedMoveBundle{
          .function_name = function_name,
          .phase = prepare::PreparedMovePhase::BeforeCall,
          .block_index = 0,
          .instruction_index = 0,
          .moves = {prepare::PreparedMoveResolution{
              .from_value_id = prepare::PreparedValueId{4301},
              .to_value_id = prepare::PreparedValueId{4301},
              .destination_kind = prepare::PreparedMoveDestinationKind::CallArgumentAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_abi_index = std::size_t{0},
              .destination_register_name = std::string{"q0"},
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {"q0"},
              .block_index = 0,
              .instruction_index = 0,
              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              .reason = "f128_hfa_call_arg_q_register_authority",
          }},
      }},
  });
  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls = {prepare::PreparedCallPlan{
          .block_index = 0,
          .instruction_index = 0,
          .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
          .direct_callee_name = std::string{"consume_hfa"},
          .arguments = {prepare::PreparedCallArgumentPlan{
              .instruction_index = 0,
              .arg_index = 0,
              .value_bank = prepare::PreparedRegisterBank::Vreg,
              .source_encoding = prepare::PreparedStorageEncodingKind::Register,
              .source_value_id = prepare::PreparedValueId{4301},
              .source_register_name = std::string{"d13"},
              .source_register_bank = prepare::PreparedRegisterBank::Fpr,
              .destination_register_name = std::string{"q0"},
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {"q0"},
              .destination_register_bank = prepare::PreparedRegisterBank::Vreg,
          }},
      }},
  });
  prepared.f128_carriers.functions.push_back(prepare::PreparedF128CarrierFunction{
      .function_name = function_name,
      .carriers = {prepare::PreparedF128Carrier{
          .function_name = function_name,
          .value_id = prepare::PreparedValueId{4301},
          .value_name = lane_name,
          .source_type = bir::TypeKind::F128,
          .kind = prepare::PreparedF128CarrierKind::FullWidthRegister,
          .total_size_bytes = 16,
          .total_align_bytes = 16,
          .register_bank = prepare::PreparedRegisterBank::Vreg,
          .register_class = prepare::PreparedRegisterClass::Vector,
          .contiguous_width = 1,
          .register_name = std::string{"d13"},
          .occupied_register_names = {"d13"},
      }},
  });
  return prepared;
}

int f128_hfa_call_boundary_requires_structured_q_register_authority() {
  auto prepared = prepared_with_f128_hfa_call_boundary();
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
  if (!diagnostics.empty() || result.emitted_instructions < 2 || block.instructions.empty()) {
    return fail("expected f128 HFA call-boundary move to select structured q-register authority");
  }
  const auto* move = std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(
      &block.instructions.front().target.payload);
  if (move == nullptr || !move->source_register.has_value() ||
      !move->destination_register.has_value() ||
      move->source_register->expected_view != aarch64_abi::RegisterView::Q ||
      move->destination_register->expected_view != aarch64_abi::RegisterView::Q ||
      move->destination_register->reg != aarch64_abi::q_register(0) ||
      move->source_f128_carrier == nullptr ||
      move->source_f128_carrier->total_size_bytes != 16) {
    return fail("expected f128 HFA call-boundary move to carry q-register source and destination authority");
  }
  return 0;
}

int scalar_call_result_publishes_gpr_to_prepared_stack_home() {
  constexpr auto function_name = c4c::FunctionNameId{4401};
  constexpr auto block_label = c4c::BlockLabelId{4402};
  constexpr auto result_value_id = prepare::PreparedValueId{4403};
  constexpr auto result_value_name = c4c::ValueNameId{4404};
  constexpr auto result_slot = prepare::PreparedFrameSlotId{4405};

  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = result_value_id,
          .function_name = function_name,
          .value_name = result_value_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = result_slot,
          .offset_bytes = std::size_t{40},
          .size_bytes = std::size_t{8},
          .align_bytes = std::size_t{8},
      }},
      .move_bundles = {prepare::PreparedMoveBundle{
          .function_name = function_name,
          .phase = prepare::PreparedMovePhase::AfterCall,
          .block_index = 0,
          .instruction_index = 3,
          .moves = {prepare::PreparedMoveResolution{
              .from_value_id = result_value_id,
              .to_value_id = result_value_id,
              .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_register_name = std::string{"x0"},
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {"x0"},
              .block_index = 0,
              .instruction_index = 3,
              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
              .reason = "call_result_gpr_to_stack_home",
          }},
          .abi_bindings = {prepare::PreparedAbiBinding{
              .destination_kind = prepare::PreparedMoveDestinationKind::CallResultAbi,
              .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
              .destination_register_name = std::string{"x0"},
              .destination_contiguous_width = 1,
              .destination_occupied_register_names = {"x0"},
          }},
      }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 3,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"produce_count"},
      .result = prepare::PreparedCallResultPlan{
          .instruction_index = 3,
          .value_bank = prepare::PreparedRegisterBank::Gpr,
          .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .destination_value_id = result_value_id,
          .source_register_name = std::string{"x0"},
          .source_contiguous_width = 1,
          .source_occupied_register_names = {"x0"},
          .source_register_bank = prepare::PreparedRegisterBank::Gpr,
          .destination_slot_id = result_slot,
          .destination_stack_offset_bytes = std::size_t{40},
      },
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered =
      aarch64_codegen::lower_after_call_moves(block_context, call_plan, 3, diagnostics);
  if (!diagnostics.empty() || lowered.size() != 1) {
    return fail("expected scalar call result publication to lower one stack-home store");
  }
  const auto* store =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&lowered.front().target.payload);
  if (store == nullptr ||
      store->memory_kind != aarch64_codegen::MemoryInstructionKind::Store ||
      store->value_type != bir::TypeKind::I64 ||
      !store->value.has_value() ||
      store->value->kind != aarch64_codegen::OperandKind::Register ||
      store->address.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      store->address.frame_slot_id != result_slot ||
      store->address.byte_offset != 40 ||
      store->address.size_bytes != 8 ||
      store->address.stored_value_id != result_value_id ||
      store->address.stored_value_name != result_value_name) {
    return fail("expected structured store into the prepared call-result stack home");
  }
  const auto* source =
      std::get_if<aarch64_codegen::RegisterOperand>(&store->value->payload);
  if (source == nullptr || source->reg != aarch64_abi::x_register(0) ||
      source->role != aarch64_codegen::RegisterOperandRole::CallAbi ||
      source->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      source->value_id != result_value_id) {
    return fail("expected stack-home publication to source the ABI GPR result register");
  }
  return 0;
}

int hfa_lane0_call_result_publishes_fpr_to_prepared_stack_home_without_move_bundle() {
  constexpr auto function_name = c4c::FunctionNameId{4451};
  constexpr auto block_label = c4c::BlockLabelId{4452};
  constexpr auto result_value_id = prepare::PreparedValueId{4453};
  constexpr auto result_value_name = c4c::ValueNameId{4454};
  constexpr auto result_slot = prepare::PreparedFrameSlotId{4455};

  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = result_value_id,
          .function_name = function_name,
          .value_name = result_value_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = result_slot,
          .offset_bytes = std::size_t{56},
          .size_bytes = std::size_t{8},
          .align_bytes = std::size_t{8},
      }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 7,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"make_hfa_double3"},
      .result = prepare::PreparedCallResultPlan{
          .instruction_index = 7,
          .value_bank = prepare::PreparedRegisterBank::Fpr,
          .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .destination_value_id = result_value_id,
          .source_register_name = std::string{"d0"},
          .source_contiguous_width = 1,
          .source_occupied_register_names = {"d0"},
          .source_register_bank = prepare::PreparedRegisterBank::Fpr,
          .destination_slot_id = result_slot,
          .destination_stack_offset_bytes = std::size_t{56},
      },
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered =
      aarch64_codegen::lower_after_call_moves(block_context, call_plan, 7, diagnostics);
  if (!diagnostics.empty() || lowered.size() != 1) {
    return fail("expected HFA lane-0 call result to synthesize one stack-home FPR publication");
  }
  const auto* store =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&lowered.front().target.payload);
  if (store == nullptr ||
      store->memory_kind != aarch64_codegen::MemoryInstructionKind::Store ||
      store->value_type != bir::TypeKind::F64 ||
      !store->value.has_value() ||
      store->value->kind != aarch64_codegen::OperandKind::Register ||
      store->address.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      store->address.frame_slot_id != result_slot ||
      store->address.byte_offset != 56 ||
      store->address.size_bytes != 8 ||
      store->address.stored_value_id != result_value_id ||
      store->address.stored_value_name != result_value_name) {
    return fail("expected HFA lane-0 publication to store into the aggregate result home");
  }
  const auto* source =
      std::get_if<aarch64_codegen::RegisterOperand>(&store->value->payload);
  if (source == nullptr ||
      source->reg != aarch64_abi::fp_simd_register(0, aarch64_abi::RegisterView::D) ||
      source->role != aarch64_codegen::RegisterOperandRole::CallAbi ||
      source->prepared_bank != prepare::PreparedRegisterBank::Fpr ||
      source->expected_view != aarch64_abi::RegisterView::D ||
      source->value_id != result_value_id) {
    return fail("expected HFA lane-0 stack-home publication to source ABI FPR result d0");
  }
  return 0;
}

int f128_hfa_lane0_call_result_publishes_q_register_to_prepared_stack_home() {
  constexpr auto function_name = c4c::FunctionNameId{4481};
  constexpr auto block_label = c4c::BlockLabelId{4482};
  constexpr auto result_value_id = prepare::PreparedValueId{4483};
  constexpr auto result_value_name = c4c::ValueNameId{4484};
  constexpr auto result_slot = prepare::PreparedFrameSlotId{4485};

  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = result_value_id,
          .function_name = function_name,
          .value_name = result_value_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = result_slot,
          .offset_bytes = std::size_t{80},
          .size_bytes = std::size_t{16},
          .align_bytes = std::size_t{16},
      }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 11,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"make_hfa_f128"},
      .result = prepare::PreparedCallResultPlan{
          .instruction_index = 11,
          .value_bank = prepare::PreparedRegisterBank::Vreg,
          .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
          .destination_value_id = result_value_id,
          .source_register_name = std::string{"q0"},
          .source_contiguous_width = 1,
          .source_occupied_register_names = {"q0"},
          .source_register_bank = prepare::PreparedRegisterBank::Vreg,
          .destination_slot_id = result_slot,
          .destination_stack_offset_bytes = std::size_t{80},
      },
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto lowered =
      aarch64_codegen::lower_after_call_moves(block_context, call_plan, 11, diagnostics);
  if (!diagnostics.empty() || lowered.size() != 1) {
    return fail("expected F128 HFA lane-0 call result to synthesize one stack-home q-register publication");
  }
  const auto* store =
      std::get_if<aarch64_codegen::MemoryInstructionRecord>(&lowered.front().target.payload);
  if (store == nullptr ||
      store->memory_kind != aarch64_codegen::MemoryInstructionKind::Store ||
      store->value_type != bir::TypeKind::F128 ||
      !store->value.has_value() ||
      store->value->kind != aarch64_codegen::OperandKind::Register ||
      store->address.base_kind != aarch64_codegen::MemoryBaseKind::FrameSlot ||
      store->address.frame_slot_id != result_slot ||
      store->address.byte_offset != 80 ||
      store->address.size_bytes != 16 ||
      store->address.align_bytes != 16 ||
      store->address.stored_value_id != result_value_id ||
      store->address.stored_value_name != result_value_name) {
    return fail("expected F128 HFA lane-0 publication to store into the aggregate result home");
  }
  const auto* source =
      std::get_if<aarch64_codegen::RegisterOperand>(&store->value->payload);
  if (source == nullptr ||
      source->reg != aarch64_abi::fp_simd_register(0, aarch64_abi::RegisterView::Q) ||
      source->role != aarch64_codegen::RegisterOperandRole::CallAbi ||
      source->prepared_bank != prepare::PreparedRegisterBank::Vreg ||
      source->prepared_class != prepare::PreparedRegisterClass::Vector ||
      source->expected_view != aarch64_abi::RegisterView::Q ||
      source->value_id != result_value_id) {
    return fail("expected F128 HFA lane-0 stack-home publication to source ABI q0");
  }
  return 0;
}

int callee_saved_preservation_uses_shared_boundary_effects() {
  constexpr auto function_name = c4c::FunctionNameId{4501};
  constexpr auto block_label = c4c::BlockLabelId{4502};
  constexpr auto value_id = prepare::PreparedValueId{4503};
  constexpr auto value_name = c4c::ValueNameId{4504};

  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const prepare::PreparedControlFlowFunction control_flow{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{.block_label = block_label}},
  };
  const prepare::PreparedValueLocationFunction value_locations{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = value_id,
          .function_name = function_name,
          .value_name = value_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"x8"},
          .size_bytes = std::size_t{8},
          .align_bytes = std::size_t{8},
      }},
  };
  const prepare::PreparedCallPlan call_plan{
      .block_index = 0,
      .instruction_index = 2,
      .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
      .direct_callee_name = std::string{"clobber_live"},
      .preserved_values = {prepare::PreparedCallPreservedValue{
          .value_id = value_id,
          .value_name = value_name,
          .route = prepare::PreparedCallPreservationRoute::CalleeSavedRegister,
          .callee_saved_save_index = std::size_t{0},
          .contiguous_width = 1,
          .register_name = std::string{"x19"},
          .register_bank = prepare::PreparedRegisterBank::Gpr,
          .occupied_register_names = {"x19"},
          .register_placement = prepare::PreparedRegisterPlacement{
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
                  .value_name = value_name,
                  .register_name = std::string{"x9"},
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .occupied_register_names = {"x9"},
              },
          .preservation_destination =
              prepare::PreparedCallBoundaryEffectEndpoint{
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .value_id = value_id,
                  .value_name = value_name,
                  .register_name = std::string{"x19"},
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .occupied_register_names = {"x19"},
                  .callee_saved_save_index = std::size_t{0},
                  .register_placement =
                      prepare::PreparedRegisterPlacement{
                          .bank = prepare::PreparedRegisterBank::Gpr,
                          .pool = prepare::PreparedRegisterSlotPool::CalleeSaved,
                          .slot_index = 0,
                          .contiguous_width = 1,
                      },
              },
      }},
  };
  const aarch64_module::FunctionLoweringContext function_context{
      .prepared = &prepared,
      .control_flow = &control_flow,
      .value_locations = &value_locations,
  };
  const aarch64_module::BlockLoweringContext block_context{
      .function = function_context,
      .control_flow_block = &control_flow.blocks.front(),
      .block_index = 0,
  };

  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto before =
      aarch64_codegen::lower_before_call_moves(block_context, call_plan, 2, diagnostics);
  const auto after =
      aarch64_codegen::lower_after_call_moves(block_context, call_plan, 2, diagnostics);
  if (!diagnostics.empty() || before.size() != 1 || after.size() != 1) {
    return fail("expected callee-saved preservation population and republication to lower");
  }

  const auto* population = std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(
      &before.front().target.payload);
  if (population == nullptr || !population->source_register.has_value() ||
      !population->destination_register.has_value() ||
      population->source_register->reg != aarch64_abi::x_register(9) ||
      population->destination_register->reg != aarch64_abi::x_register(19) ||
      population->move.reason != "callee_saved_preservation_home_population") {
    return fail("expected population to consume the prepared preservation effect destination inside the selected printable subset");
  }

  const auto* republication = std::get_if<aarch64_codegen::CallBoundaryMoveInstructionRecord>(
      &after.front().target.payload);
  if (republication == nullptr || !republication->source_register.has_value() ||
      !republication->destination_register.has_value() ||
      republication->source_register->reg != aarch64_abi::x_register(19) ||
      republication->destination_register->reg != aarch64_abi::x_register(9) ||
      republication->move.reason != "preservation_republication") {
    return fail("expected republication to consume the prepared preservation effect source");
  }
  return 0;
}

}  // namespace

int main() {
  int status = 0;
  status |= byval_caller_publishes_composite_gpr_lanes_not_object_pointer();
  status |= byval_callee_entry_consumes_byval_frame_slot();
  status |= f128_hfa_call_boundary_requires_structured_q_register_authority();
  status |= scalar_call_result_publishes_gpr_to_prepared_stack_home();
  status |= hfa_lane0_call_result_publishes_fpr_to_prepared_stack_home_without_move_bundle();
  status |= f128_hfa_lane0_call_result_publishes_q_register_to_prepared_stack_home();
  status |= callee_saved_preservation_uses_shared_boundary_effects();
  return status == 0 ? 0 : 1;
}
