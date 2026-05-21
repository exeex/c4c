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

int byval_caller_publishes_composite_gpr_lanes_not_object_pointer() {
  constexpr auto function_name = c4c::FunctionNameId{4101};
  constexpr auto block_label = c4c::BlockLabelId{4102};
  constexpr auto aggregate_value_id = prepare::PreparedValueId{4103};
  constexpr auto aggregate_value_name = c4c::ValueNameId{4104};

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

}  // namespace

int main() {
  int status = 0;
  status |= byval_caller_publishes_composite_gpr_lanes_not_object_pointer();
  status |= byval_callee_entry_consumes_byval_frame_slot();
  status |= f128_hfa_call_boundary_requires_structured_q_register_authority();
  return status == 0 ? 0 : 1;
}
