#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_with_unsupported_instructions() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.fn");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.entry");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.fn",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "dispatch.entry",
          .insts = {bir::BinaryInst{
                        .opcode = bir::BinaryOpcode::Add,
                        .result = bir::Value::named(bir::TypeKind::I32, "sum"),
                        .operand_type = bir::TypeKind::I32,
                        .lhs = bir::Value::immediate_i32(1),
                        .rhs = bir::Value::immediate_i32(2),
                    },
                    bir::CallInst{
                        .result = bir::Value::named(bir::TypeKind::I32, "call_result"),
                        .callee = "external",
                        .return_type = bir::TypeKind::I32,
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
  return prepared;
}

prepare::PreparedBirModule prepared_with_direct_call_plan() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.call");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.call.entry");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("dispatch.call.entry");
  const auto actual_link = prepared.names.link_names.intern("actual_function");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.call",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.call.entry",
          .insts = {bir::CallInst{
              .callee = "actual_function",
              .callee_link_name_id = actual_link,
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
          .direct_callee_name = std::string{"actual_function"},
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_indirect_call_plan(bool register_callee) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.indirect");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.indirect.entry");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("dispatch.indirect.entry");
  const auto callee_name = prepared.names.value_names.intern("%callee");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.indirect",
      .return_type = bir::TypeKind::Void,
      .blocks = {bir::Block{
          .label = "dispatch.indirect.entry",
          .insts = {bir::CallInst{
              .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%callee"),
              .return_type = bir::TypeKind::Void,
              .calling_convention = bir::CallingConv::C,
              .is_indirect = true,
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
          .wrapper_kind = prepare::PreparedCallWrapperKind::Indirect,
          .is_indirect = true,
          .indirect_callee =
              prepare::PreparedIndirectCalleePlan{
                  .value_name = callee_name,
                  .value_id = prepare::PreparedValueId{31},
                  .encoding = register_callee
                                  ? prepare::PreparedStorageEncodingKind::Register
                                  : prepare::PreparedStorageEncodingKind::FrameSlot,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .register_name = register_callee
                                       ? std::optional<std::string>{"x9"}
                                       : std::nullopt,
                  .slot_id = register_callee
                                 ? std::nullopt
                                 : std::optional<prepare::PreparedFrameSlotId>{4},
                  .stack_offset_bytes = register_callee
                                            ? std::nullopt
                                            : std::optional<std::size_t>{24},
              },
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_reordered_retained_bir() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto first_function = prepared.names.function_names.intern("dispatch.first");
  const auto second_function = prepared.names.function_names.intern("dispatch.second");
  const auto first_entry = prepared.names.block_labels.intern("first.entry");
  const auto second_entry = prepared.names.block_labels.intern("second.entry");
  const auto second_late = prepared.names.block_labels.intern("second.late");

  const auto bir_first_entry = prepared.module.names.block_labels.intern("first.entry");
  const auto bir_second_entry = prepared.module.names.block_labels.intern("second.entry");
  const auto bir_second_late = prepared.module.names.block_labels.intern("second.late");

  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.second",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
                     .label = "second.late",
                     .insts = {bir::CallInst{
                         .result = bir::Value::named(bir::TypeKind::I32, "late_call"),
                         .callee = "external",
                         .return_type = bir::TypeKind::I32,
                     }},
                     .terminator = bir::Terminator{bir::ReturnTerminator{}},
                     .label_id = bir_second_late,
                 },
                 bir::Block{
                     .label = "second.entry",
                     .insts = {bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Mul,
                         .result = bir::Value::named(bir::TypeKind::I32, "entry_product"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::immediate_i32(3),
                         .rhs = bir::Value::immediate_i32(4),
                     }},
                     .terminator = bir::Terminator{bir::ReturnTerminator{}},
                     .label_id = bir_second_entry,
                 }},
  });
  prepared.module.functions.push_back(bir::Function{
      .name = "dispatch.first",
      .return_type = bir::TypeKind::I32,
      .blocks = {bir::Block{
          .label = "first.entry",
          .insts = {bir::CallInst{
              .result = bir::Value::named(bir::TypeKind::I32, "first_call"),
              .callee = "external",
              .return_type = bir::TypeKind::I32,
          }},
          .terminator = bir::Terminator{bir::ReturnTerminator{}},
          .label_id = bir_first_entry,
      }},
  });

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = first_function,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = first_entry,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = second_function,
      .blocks = {prepare::PreparedControlFlowBlock{
                     .block_label = second_entry,
                     .terminator_kind = bir::TerminatorKind::Return,
                 },
                 prepare::PreparedControlFlowBlock{
                     .block_label = second_late,
                     .terminator_kind = bir::TerminatorKind::Return,
                 }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_control_flow_only() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.cf_only");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.cf_entry");
  const auto exit_label = prepared.names.block_labels.intern("dispatch.cf_exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Branch,
          .branch_target_label = exit_label,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_simple_fixed_frame() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("dispatch.frame");
  const auto entry_label = prepared.names.block_labels.intern("dispatch.frame.entry");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 32,
      .frame_alignment_bytes = 16,
  });
  return prepared;
}

int block_dispatch_visits_prepared_terminator_without_bir_block_mapping() {
  auto prepared = prepared_with_control_flow_only();
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
      result.emitted_instructions != 0 || !block.instructions.empty()) {
    return fail("expected control-flow-only dispatch to visit terminator only");
  }
  if (block.block_label != block_cf.block_label || block.index != 0) {
    return fail("expected control-flow-only dispatch to preserve prepared block identity");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected unsupported terminator diagnostic without BIR mapping");
  }
  return 0;
}

int block_dispatch_visits_prepared_instructions_in_order_and_fails_closed() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block{
      .block_label = c4c::kInvalidBlockLabel,
      .index = 99,
      .instructions = {},
  };
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 2 || !result.visited_terminator ||
      result.emitted_instructions != 1) {
    return fail("expected dispatch to visit prepared instructions and emit return");
  }
  if (block.block_label != block_cf.block_label || block.index != 0 ||
      block.instructions.size() != 1) {
    return fail("expected dispatch to preserve identity and emit one return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.front().target.payload)) {
    return fail("expected dispatch to emit canonical return instruction target");
  }
  if (diagnostics.entries.size() != 2) {
    return fail("expected unsupported instruction diagnostics only");
  }
  if (diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      diagnostics.entries[0].instruction_index != 0 ||
      diagnostics.entries[0].instruction_family !=
          aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected first diagnostic to describe scalar instruction zero");
  }
  if (diagnostics.entries[1].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingPreparedCallPlan ||
      diagnostics.entries[1].instruction_index != 1 ||
      diagnostics.entries[1].instruction_family !=
          aarch64_module::InstructionLoweringFamily::Call) {
    return fail("expected second diagnostic to describe missing prepared call plan");
  }
  return 0;
}

int block_dispatch_lowers_prepared_direct_call_without_reclassifying_abi() {
  auto prepared = prepared_with_direct_call_plan();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  if (function_context.call_plans == nullptr ||
      function_context.frame_plan != nullptr ||
      function_context.dynamic_stack_plan != nullptr) {
    return fail("expected function context to thread prepared call plans without fake frame plans");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      !diagnostics.empty()) {
    return fail("expected prepared direct call dispatch to emit call plus return");
  }

  const auto& call_instruction = block.instructions.front();
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &call_instruction.target.payload);
  if (call == nullptr ||
      call_instruction.target.family != aarch64_module::codegen::InstructionFamily::Call ||
      call_instruction.target.opcode != aarch64_module::codegen::MachineOpcode::DirectCall ||
      call_instruction.target.function_name != function_cf.function_name ||
      call_instruction.target.block_label != block_cf.block_label ||
      call_instruction.target.instruction_index != 0 ||
      !call->direct_callee.has_value() ||
      call->direct_callee_label != "actual_function" ||
      call->wrapper_kind != prepare::PreparedCallWrapperKind::DirectExternFixedArity ||
      call->source_call != &function_context.call_plans->calls.front() ||
      !call->arguments.empty() || call->result.has_value()) {
    return fail("expected direct call node to preserve prepared call provenance only");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          block.instructions.back().target.payload)) {
    return fail("expected return terminator to remain lowered after direct call");
  }
  return 0;
}

int block_dispatch_lowers_prepared_indirect_call_only_with_register_authority() {
  auto prepared = prepared_with_indirect_call_plan(true);
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

  if (result.visited_operations != 1 || !result.visited_terminator ||
      result.emitted_instructions != 2 || block.instructions.size() != 2 ||
      !diagnostics.empty()) {
    return fail("expected prepared register indirect call dispatch to emit call plus return");
  }

  const auto& call_instruction = block.instructions.front();
  const auto* call = std::get_if<aarch64_module::codegen::CallInstructionRecord>(
      &call_instruction.target.payload);
  if (call == nullptr ||
      call_instruction.target.family != aarch64_module::codegen::InstructionFamily::Call ||
      call_instruction.target.opcode != aarch64_module::codegen::MachineOpcode::IndirectCall ||
      call_instruction.target.function_name != function_cf.function_name ||
      call_instruction.target.block_label != block_cf.block_label ||
      call_instruction.target.instruction_index != 0 ||
      !call->is_indirect ||
      !call->indirect_callee.has_value() ||
      !call->prepared_indirect_callee.has_value() ||
      call->prepared_indirect_callee->register_name != std::optional<std::string>{"x9"} ||
      call->source_call != &function_context.call_plans->calls.front()) {
    return fail("expected indirect call node to preserve prepared register callee provenance");
  }
  const auto* callee =
      std::get_if<aarch64_module::codegen::RegisterOperand>(&call->indirect_callee->payload);
  if (call->indirect_callee->kind != aarch64_module::codegen::OperandKind::Register ||
      callee == nullptr || callee->reg != aarch64_module::abi::x_register(9) ||
      callee->role != aarch64_module::codegen::RegisterOperandRole::CallAbi ||
      callee->value_id != prepare::PreparedValueId{31}) {
    return fail("expected indirect call callee to use prepared x9 register authority");
  }

  auto stack_callee_prepared = prepared_with_indirect_call_plan(false);
  const auto& stack_function_cf = stack_callee_prepared.control_flow.functions.front();
  const auto& stack_block_cf = stack_function_cf.blocks.front();
  const auto stack_function_context = aarch64_codegen::make_function_lowering_context(
      stack_callee_prepared, stack_callee_prepared.target_profile, stack_function_cf);
  const auto stack_block_context =
      aarch64_codegen::make_block_lowering_context(stack_function_context, stack_block_cf, 0);
  aarch64_module::MachineBlock stack_block;
  aarch64_module::ModuleLoweringDiagnostics stack_diagnostics;
  const auto stack_result = aarch64_codegen::dispatch_prepared_block(
      stack_block_context, stack_block, stack_diagnostics);
  if (stack_result.visited_operations != 1 || !stack_result.visited_terminator ||
      stack_result.emitted_instructions != 1 || stack_block.instructions.size() != 1 ||
      stack_diagnostics.entries.size() != 1 ||
      stack_diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily ||
      stack_diagnostics.entries.front().message.find("explicit prepared GPR callee register") ==
          std::string::npos ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          stack_block.instructions.front().target.payload)) {
    return fail("expected stack indirect callee to fail closed without scratch selection");
  }

  return 0;
}

int block_dispatch_maps_retained_bir_by_prepared_identity_not_index() {
  auto prepared = prepared_with_reordered_retained_bir();
  const auto& function_cf = prepared.control_flow.functions[1];
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);
  if (block_context.bir_block == nullptr ||
      block_context.bir_block->label != "second.entry") {
    return fail("expected block context to map retained BIR by prepared identity");
  }

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 1) {
    return fail("expected retained BIR dispatch to visit the identity-mapped block instruction");
  }
  if (!result.visited_terminator) {
    return fail("expected retained BIR dispatch to visit the prepared terminator");
  }
  if (result.emitted_instructions != 1 || block.instructions.size() != 1) {
    return fail("expected retained BIR dispatch to emit one return instruction");
  }
  if (diagnostics.entries.size() != 1) {
    return fail("expected identity-mapped retained BIR dispatch to record operation diagnostic only");
  }
  if (diagnostics.entries[0].kind !=
      aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily) {
    return fail("expected identity-mapped retained BIR dispatch instruction diagnostic");
  }
  if (diagnostics.entries[0].instruction_family !=
      aarch64_module::InstructionLoweringFamily::Scalar) {
    return fail("expected identity-mapped retained BIR dispatch to classify the instruction");
  }
  if (diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected identity-mapped retained BIR dispatch diagnostic block identity");
  }
  return 0;
}

int missing_bir_block_mapping_is_diagnostic_only() {
  auto prepared = prepared_with_unsupported_instructions();
  prepared.module.functions.front().blocks.front().label_id = c4c::kInvalidBlockLabel;
  prepared.module.functions.front().blocks.front().label = "dispatch.unmatched";

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
      result.emitted_instructions != 1 || block.instructions.size() != 1) {
    return fail("expected missing mapping to skip operations but still lower return terminator");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries[0].kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping ||
      diagnostics.entries[0].block_label != block_cf.block_label) {
    return fail("expected typed missing block mapping diagnostic");
  }
  return 0;
}

int module_build_dispatch_scaffold_lowers_return_and_keeps_machine_nodes_empty() {
  auto prepared = prepared_with_unsupported_instructions();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared module with dispatch scaffold to build");
  }

  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 1 || function.blocks.front().instructions.size() != 1) {
    return fail("expected dispatch scaffold to emit one return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          function.blocks.front().instructions.front().target.payload)) {
    return fail("expected module build to preserve canonical return target");
  }
  for (const auto& record : result.module->functions) {
    if (!record.machine_nodes.empty()) {
      return fail("expected dispatch scaffold not to fake flat compatibility machine nodes");
    }
  }
  return 0;
}

int module_build_lowers_simple_fixed_frame_around_function_stream() {
  auto prepared = prepared_with_simple_fixed_frame();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared module with simple fixed frame to build");
  }

  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 1 || function.blocks.front().instructions.size() != 3) {
    return fail("expected simple fixed frame to emit setup, teardown, and return");
  }
  const auto& block = function.blocks.front();
  const auto& setup_instruction = block.instructions[0];
  const auto& teardown_instruction = block.instructions[1];
  const auto& return_instruction = block.instructions[2];
  const auto* setup =
      std::get_if<aarch64_module::codegen::FrameInstructionRecord>(
          &setup_instruction.target.payload);
  const auto* teardown =
      std::get_if<aarch64_module::codegen::FrameInstructionRecord>(
          &teardown_instruction.target.payload);
  if (setup == nullptr || teardown == nullptr ||
      setup_instruction.target.family !=
          aarch64_module::codegen::InstructionFamily::Frame ||
      teardown_instruction.target.family !=
          aarch64_module::codegen::InstructionFamily::Frame ||
      setup_instruction.target.opcode !=
          aarch64_module::codegen::MachineOpcode::FrameSetup ||
      teardown_instruction.target.opcode !=
          aarch64_module::codegen::MachineOpcode::FrameTeardown ||
      setup->source_frame != &prepared.frame_plan.functions.front() ||
      teardown->source_frame != &prepared.frame_plan.functions.front() ||
      setup->frame_size_bytes != 32 || teardown->frame_size_bytes != 32 ||
      setup->frame_alignment_bytes != 16 || teardown->frame_alignment_bytes != 16 ||
      setup_instruction.target.function_name !=
          prepared.control_flow.functions.front().function_name ||
      setup_instruction.target.block_label != block.block_label ||
      teardown_instruction.target.block_label != block.block_label ||
      !setup_instruction.origin.has_value() || !teardown_instruction.origin.has_value() ||
      setup_instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::Synthetic ||
      teardown_instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::Synthetic) {
    return fail("expected fixed-frame setup/teardown to preserve prepared frame provenance");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          return_instruction.target.payload)) {
    return fail("expected fixed-frame teardown to be inserted before return");
  }

  auto dynamic_frame = prepared_with_simple_fixed_frame();
  dynamic_frame.frame_plan.functions.front().has_dynamic_stack = true;
  const auto dynamic_result = aarch64_api::build_prepared_module(dynamic_frame);
  if (dynamic_result.error.has_value() || !dynamic_result.module.has_value() ||
      dynamic_result.module->mir.functions.front().blocks.front().instructions.size() != 1 ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          dynamic_result.module->mir.functions.front()
              .blocks.front()
              .instructions.front()
              .target.payload)) {
    return fail("expected dynamic-stack frame to remain explicitly deferred");
  }

  auto callee_save_frame = prepared_with_simple_fixed_frame();
  callee_save_frame.frame_plan.functions.front().saved_callee_registers.push_back(
      prepare::PreparedSavedRegister{
          .bank = prepare::PreparedRegisterBank::Gpr,
          .register_name = "x19",
          .contiguous_width = 1,
          .occupied_register_names = {"x19"},
          .save_index = 0,
      });
  const auto callee_save_result = aarch64_api::build_prepared_module(callee_save_frame);
  if (callee_save_result.error.has_value() || !callee_save_result.module.has_value() ||
      callee_save_result.module->mir.functions.front().blocks.front().instructions.size() != 1 ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          callee_save_result.module->mir.functions.front()
              .blocks.front()
              .instructions.front()
              .target.payload)) {
    return fail("expected callee-save frame to remain explicitly deferred");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status =
          block_dispatch_visits_prepared_terminator_without_bir_block_mapping();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_visits_prepared_instructions_in_order_and_fails_closed();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_direct_call_without_reclassifying_abi();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_lowers_prepared_indirect_call_only_with_register_authority();
      status != 0) {
    return status;
  }
  if (const int status =
          block_dispatch_maps_retained_bir_by_prepared_identity_not_index();
      status != 0) {
    return status;
  }
  if (const int status = missing_bir_block_mapping_is_diagnostic_only();
      status != 0) {
    return status;
  }
  if (const int status = module_build_dispatch_scaffold_lowers_return_and_keeps_machine_nodes_empty();
      status != 0) {
    return status;
  }
  if (const int status = module_build_lowers_simple_fixed_frame_around_function_stream();
      status != 0) {
    return status;
  }
  return 0;
}
